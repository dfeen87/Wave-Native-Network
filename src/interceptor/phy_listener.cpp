#include "phy_listener.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <numeric>
#include <cmath>

namespace wave_native {
namespace interceptor {

PhyListener::PhyListener(const std::string& interface_name)
    : interface_name_(interface_name), pcap_handle_(nullptr, [](pcap_t* p){ if (p) pcap_close(p); }), running_(false) {
}

PhyListener::~PhyListener() {
    stop();
}

void PhyListener::start() {
    if (running_) return;

    char errbuf[PCAP_ERRBUF_SIZE];

    // Open the interface in promiscuous mode (1)
    // Snaplen 65535 to capture whole packets
    // Timeout 1ms (we want near real-time, but avoid burning 100% CPU purely on polling if possible,
    // though for continuous wave we will just sample as fast as it comes)
    pcap_t* raw_pcap = pcap_open_live(interface_name_.c_str(), 65535, 1, 1, errbuf);

    if (raw_pcap == nullptr) {
        std::cerr << "Warning: Could not open device " << interface_name_ << " for live capture: " << errbuf << std::endl;
        std::cerr << "Running in mock/silent mode." << std::endl;
    } else {
        pcap_handle_.reset(raw_pcap);
        // Set non-blocking mode if desired, but here we run in a dedicated thread
        // using pcap_loop or pcap_dispatch
    }

    running_ = true;
    listener_thread_ = std::jthread([this](std::stop_token st) { listener_loop(st); });
    injector_thread_ = std::jthread([this](std::stop_token st) { injector_loop(st); });
}

void PhyListener::stop() {
    running_ = false;
    if (pcap_handle_) {
        pcap_breakloop(pcap_handle_.get());
    }

    if (listener_thread_.joinable()) {
        listener_thread_.request_stop();
    }
    if (injector_thread_.joinable()) {
        injector_thread_.request_stop();
    }

    injector_cv_.notify_all();

    if (listener_thread_.joinable()) {
        listener_thread_.join();
    }
    if (injector_thread_.joinable()) {
        injector_thread_.join();
    }
}

void PhyListener::listener_loop(std::stop_token stoken) {
    if (pcap_handle_) {
        while (running_ && !stoken.stop_requested()) {
            // Process whatever is available, blocks until timeout
            pcap_dispatch(pcap_handle_.get(), -1, packet_handler, reinterpret_cast<u_char*>(this));
        }
    } else {
        // Mock mode if no interface
        while (running_ && !stoken.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // Generate some fake "noise" amplitude
            spsc_amplitude_queue_.push(((double)rand() / RAND_MAX) * 2.0 - 1.0);
        }
    }
}

void PhyListener::packet_handler(u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
    auto* listener = reinterpret_cast<PhyListener*>(user);
    if (!listener->running_.load(std::memory_order_acquire)) return;

    // Strict boundary enforcement via std::span, capped to 2048 MTU to prevent unbounded loops
    std::span<const u_char> payload(packet, std::min(pkthdr->caplen, static_cast<bpf_u_int32>(2048)));

    for (u_char byte : payload) {
        double amp = (static_cast<double>(byte) - 127.5) / 127.5;
        // Zero-allocation, lock-free push
        listener->spsc_amplitude_queue_.push(amp); 
    }

    // Calculate Inter-Arrival Time (IAT)
    double iat_ns = 0.0;
    if (listener->last_packet_time_.tv_sec != 0 || listener->last_packet_time_.tv_usec != 0) {
        double current_sec = pkthdr->ts.tv_sec + pkthdr->ts.tv_usec / 1e6;
        double last_sec = listener->last_packet_time_.tv_sec + listener->last_packet_time_.tv_usec / 1e6;
        iat_ns = (current_sec - last_sec) * 1e9;
    }
    listener->last_packet_time_ = pkthdr->ts;

    if (iat_ns > 0.0) {
        listener->spsc_iat_queue_.push(iat_ns);
    }
}

std::vector<double> PhyListener::consume_stream() {
    std::vector<double> stream;
    double item;
    while (spsc_amplitude_queue_.pop(item)) {
        stream.push_back(item);
    }
    return stream;
}

std::vector<double> PhyListener::consume_iats() {
    std::vector<double> stream;
    double item;
    while (spsc_iat_queue_.pop(item)) {
        stream.push_back(item);
    }
    return stream;
}

void PhyListener::inject_modulated_packet(double delay_ns) {
    {
        std::lock_guard<std::mutex> lock(injector_mutex_);
        delay_queue_.push_back(delay_ns);
    }
    injector_cv_.notify_one();
}

void PhyListener::injector_loop(std::stop_token stoken) {
    // Minimal raw ethernet packet (broadcast MAC, simple ethertype)
    uint8_t dummy_packet[64];
    std::memset(dummy_packet, 0, sizeof(dummy_packet));

    // Set destination MAC to broadcast
    for (int i = 0; i < 6; ++i) dummy_packet[i] = 0xFF;
    // Set source MAC to some dummy
    for (int i = 6; i < 12; ++i) dummy_packet[i] = 0xAA;
    // Set EtherType (0x0800 for IP, or custom)
    dummy_packet[12] = 0x08;
    dummy_packet[13] = 0x00;

    while (running_ && !stoken.stop_requested()) {
        double delay_ns = 0.0;
        {
            std::unique_lock<std::mutex> lock(injector_mutex_);
            injector_cv_.wait(lock, [this, &stoken]() { return !delay_queue_.empty() || !running_ || stoken.stop_requested(); });

            if (!running_ || stoken.stop_requested()) break;

            delay_ns = delay_queue_.front();
            delay_queue_.erase(delay_queue_.begin());
        }

        // Execute the transduction delay (Nudge)
        if (delay_ns > 0.0) {
            if (delay_ns < 50000.0) {
                auto start = std::chrono::high_resolution_clock::now();
                while (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() < delay_ns) {
                    // spin
                }
            } else {
                std::this_thread::sleep_for(std::chrono::nanoseconds(static_cast<long long>(delay_ns)));
            }
        }

        // Inject into libpcap to actually modulate the network jitter
        if (pcap_handle_) {
            if (pcap_inject(pcap_handle_.get(), dummy_packet, sizeof(dummy_packet)) == -1) {
                // Silently drop if we don't have permissions or interface is down,
                // but in a real system we'd log: pcap_geterr(pcap_handle_)
            }
        }
    }
}

} // namespace interceptor
} // namespace wave_native

namespace wave_native {
namespace interceptor {

void PhyListener::flush_delay_queue() {
    std::lock_guard<std::mutex> lock(injector_mutex_);
    delay_queue_.clear();
}

} // namespace interceptor
} // namespace wave_native
