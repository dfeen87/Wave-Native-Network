#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <pcap.h>
#include <memory>
#include <span>
#include <array>

namespace wave_native {
namespace interceptor {

template <typename T, size_t Size>
class LockFreeSPSCQueue {
    std::array<T, Size> buffer_;
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
public:
    bool push(const T& item) {
        auto current_tail = tail_.load(std::memory_order_relaxed);
        auto next_tail = (current_tail + 1) % Size;
        if (next_tail == head_.load(std::memory_order_acquire)) return false; // Overflow drop
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) {
        auto current_head = head_.load(std::memory_order_relaxed);
        if (current_head == tail_.load(std::memory_order_acquire)) return false; // Empty
        item = buffer_[current_head];
        head_.store((current_head + 1) % Size, std::memory_order_release);
        return true;
    }
};

class PhyListener {
public:
    PhyListener(const std::string& interface_name);
    void flush_delay_queue();
    ~PhyListener();

    void start();
    void stop();

    // Consume the current normalized amplitude buffer
    std::vector<double> consume_stream();

    // Consume the stream of Inter-Arrival Times (IAT)
    std::vector<double> consume_iats();

    // Asynchronously injects a packet after a specified delay to modulate IAT
    void inject_modulated_packet(double delay_ns);

private:
    void listener_loop(std::stop_token stoken);
    void injector_loop(std::stop_token stoken);
    static void packet_handler(u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* packet);

    std::string interface_name_;
    std::unique_ptr<pcap_t, void(*)(pcap_t*)> pcap_handle_;

    std::jthread listener_thread_;
    std::atomic<bool> running_;

    LockFreeSPSCQueue<double, 100000> spsc_amplitude_queue_;

    LockFreeSPSCQueue<double, 100000> spsc_iat_queue_;
    struct timeval last_packet_time_ = {0, 0};

    // Asynchronous injection queue
    std::jthread injector_thread_;
    std::mutex injector_mutex_;
    std::condition_variable injector_cv_;
    std::vector<double> delay_queue_;
};

} // namespace interceptor
} // namespace wave_native
