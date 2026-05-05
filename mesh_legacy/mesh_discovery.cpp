#include <mutex>
#include "mesh_discovery.hpp"
#include <iostream>
#include <algorithm>

namespace wave_native {
namespace mesh_legacy {

    void InterfaceRegistry::register_interface(const InterfaceInfo& info) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        interfaces_[info.name] = info;
    }

    void InterfaceRegistry::unregister_interface(const std::string& name) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        interfaces_.erase(name);
    }

    std::vector<InterfaceInfo> InterfaceRegistry::get_wan_candidates() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<InterfaceInfo> candidates;
        for (const auto& [name, info] : interfaces_) {
            if (info.is_wan_candidate()) {
                candidates.push_back(info);
            }
        }
        return candidates;
    }

    std::optional<InterfaceInfo> InterfaceRegistry::get_interface(const std::string& name) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = interfaces_.find(name);
        if (it != interfaces_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::vector<InterfaceInfo> InterfaceRegistry::get_all_interfaces() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<InterfaceInfo> all_interfaces;
        all_interfaces.reserve(interfaces_.size());
        for (const auto& [name, info] : interfaces_) {
            all_interfaces.push_back(info);
        }
        return all_interfaces;
    }

    InterfaceDiscovery::InterfaceDiscovery(std::shared_ptr<InterfaceRegistry> registry)
        : registry_(std::move(registry)) {}

    void InterfaceDiscovery::start_monitoring() {
        // In a full implementation, this would spawn a thread to periodically call discover_interfaces()
        discover_interfaces();
    }

    bool InterfaceDiscovery::discover_interfaces() {
        // Simplified implementation using mock data
        return use_mock_interfaces();
    }

    bool InterfaceDiscovery::discover_linux_interfaces() {
        // Simplified placeholder for actual /sys/class/net parsing
        return false;
    }

    bool InterfaceDiscovery::use_mock_interfaces() {
        if (!registry_) return false;

        InterfaceInfo eth0;
        eth0.name = "eth0";
        eth0.iface_type = InterfaceType::Ethernet;
        eth0.is_up = true;
        eth0.has_carrier = true;
        eth0.has_address = true;
        eth0.mtu = 1500;
        eth0.mac_address = "00:11:22:33:44:55";
        eth0.ipv4_addresses.push_back("192.168.1.100");

        InterfaceInfo wlan0;
        wlan0.name = "wlan0";
        wlan0.iface_type = InterfaceType::WiFi;
        wlan0.is_up = true;
        wlan0.has_carrier = false;
        wlan0.has_address = false;
        wlan0.mtu = 1500;
        wlan0.mac_address = "AA:BB:CC:DD:EE:FF";

        registry_->register_interface(eth0);
        registry_->register_interface(wlan0);

        return true;
    }

} // namespace mesh_legacy
} // namespace wave_native
