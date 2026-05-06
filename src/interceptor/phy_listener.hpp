#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <pcap.h>
#include <memory>

namespace wave_native {
namespace interceptor {

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

    std::mutex stream_mutex_;
    std::vector<double> amplitude_stream_;

    std::mutex iat_mutex_;
    std::vector<double> iat_stream_;
    struct timeval last_packet_time_ = {0, 0};

    // Asynchronous injection queue
    std::jthread injector_thread_;
    std::mutex injector_mutex_;
    std::condition_variable injector_cv_;
    std::vector<double> delay_queue_;
};

} // namespace interceptor
} // namespace wave_native
