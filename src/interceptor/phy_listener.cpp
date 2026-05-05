#include "phy_listener.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <numeric>
#include <cmath>

namespace wave_native {
namespace interceptor {

PhyListener::PhyListener(const std::string& interface_name)
    : interface_name_(interface_name), pcap_handle_(nullptr), running_(false) {
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
    pcap_handle_ = pcap_open_live(interface_name_.c_str(), 65535, 1, 1, errbuf);

    if (pcap_handle_ == nullptr) {
        std::cerr << "Warning: Could not open device " << interface_name_ << " for live capture: " << errbuf << std::endl;
        std::cerr << "Running in mock/silent mode." << std::endl;
    } else {
        // Set non-blocking mode if desired, but here we run in a dedicated thread
        // using pcap_loop or pcap_dispatch
    }

    running_ = true;
    listener_thread_ = std::thread(&PhyListener::listener_loop, this);
}

void PhyListener::stop() {
    running_ = false;
    if (pcap_handle_) {
        pcap_breakloop(pcap_handle_);
    }
    if (listener_thread_.joinable()) {
        listener_thread_.join();
    }
    if (pcap_handle_) {
        pcap_close(pcap_handle_);
        pcap_handle_ = nullptr;
    }
}

void PhyListener::listener_loop() {
    if (pcap_handle_) {
        while (running_) {
            // Process whatever is available, blocks until timeout
            pcap_dispatch(pcap_handle_, -1, packet_handler, reinterpret_cast<u_char*>(this));
        }
    } else {
        // Mock mode if no interface
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::lock_guard<std::mutex> lock(stream_mutex_);
            // Generate some fake "noise" amplitude
            amplitude_stream_.push_back(((double)rand() / RAND_MAX) * 2.0 - 1.0);
        }
    }
}

void PhyListener::packet_handler(u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
    PhyListener* listener = reinterpret_cast<PhyListener*>(user);
    if (!listener->running_) return;

    // Convert raw binary noise into floating-point amplitude stream
    // Treat the packet as continuous wave
    std::vector<double> new_amplitudes;
    new_amplitudes.reserve(pkthdr->caplen);

    for (bpf_u_int32 i = 0; i < pkthdr->caplen; ++i) {
        // Normalize 0-255 to -1.0 to 1.0
        double amp = (static_cast<double>(packet[i]) - 127.5) / 127.5;
        new_amplitudes.push_back(amp);
    }

    std::lock_guard<std::mutex> lock(listener->stream_mutex_);
    listener->amplitude_stream_.insert(listener->amplitude_stream_.end(), new_amplitudes.begin(), new_amplitudes.end());

    // Cap size to avoid out-of-memory if not consumed fast enough
    if (listener->amplitude_stream_.size() > 100000) {
        listener->amplitude_stream_.erase(listener->amplitude_stream_.begin(),
            listener->amplitude_stream_.begin() + (listener->amplitude_stream_.size() - 100000));
    }
}

std::vector<double> PhyListener::consume_stream() {
    std::lock_guard<std::mutex> lock(stream_mutex_);
    std::vector<double> stream = std::move(amplitude_stream_);
    amplitude_stream_.clear();
    return stream;
}

} // namespace interceptor
} // namespace wave_native
