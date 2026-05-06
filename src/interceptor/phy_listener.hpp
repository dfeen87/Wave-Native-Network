#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <pcap.h>

namespace wave_native {
namespace interceptor {

class PhyListener {
public:
    PhyListener(const std::string& interface_name);
    ~PhyListener();

    void start();
    void stop();

    // Consume the current normalized amplitude buffer
    std::vector<double> consume_stream();

    // Asynchronously injects a packet after a specified delay to modulate IAT
    void inject_modulated_packet(double delay_ns);

private:
    void listener_loop();
    void injector_loop();
    static void packet_handler(u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* packet);

    std::string interface_name_;
    pcap_t* pcap_handle_;

    std::thread listener_thread_;
    std::atomic<bool> running_;

    std::mutex stream_mutex_;
    std::vector<double> amplitude_stream_;

    // Asynchronous injection queue
    std::thread injector_thread_;
    std::mutex injector_mutex_;
    std::condition_variable injector_cv_;
    std::vector<double> delay_queue_;
};

} // namespace interceptor
} // namespace wave_native
