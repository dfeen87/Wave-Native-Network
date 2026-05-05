#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
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

private:
    void listener_loop();
    static void packet_handler(u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* packet);

    std::string interface_name_;
    pcap_t* pcap_handle_;

    std::thread listener_thread_;
    std::atomic<bool> running_;

    std::mutex stream_mutex_;
    std::vector<double> amplitude_stream_;
};

} // namespace interceptor
} // namespace wave_native
