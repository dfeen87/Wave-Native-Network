#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <shared_mutex>
#include <optional>

namespace wave_native {
namespace mesh_legacy {

    enum class InterfaceType {
        Ethernet,
        WiFi,
        LteModem,
        UsbTether,
        BluetoothPan,
        Unknown
    };

    struct InterfaceInfo {
        std::string name;
        InterfaceType iface_type;
        bool is_up;
        bool has_carrier;
        bool has_address;
        uint32_t mtu;
        std::optional<std::string> mac_address;
        std::vector<std::string> ipv4_addresses;
        std::vector<std::string> ipv6_addresses;

        bool is_wan_candidate() const {
            return is_up && has_carrier && has_address && iface_type != InterfaceType::Unknown;
        }
    };

    class InterfaceRegistry {
    public:
        InterfaceRegistry() = default;

        void register_interface(const InterfaceInfo& info);
        void unregister_interface(const std::string& name);

        std::vector<InterfaceInfo> get_wan_candidates() const;
        std::optional<InterfaceInfo> get_interface(const std::string& name) const;
        std::vector<InterfaceInfo> get_all_interfaces() const;

    private:
        mutable std::shared_mutex mutex_;
        std::map<std::string, InterfaceInfo> interfaces_;
    };

    class InterfaceDiscovery {
    public:
        explicit InterfaceDiscovery(std::shared_ptr<InterfaceRegistry> registry);

        void start_monitoring();
        bool discover_interfaces();

    private:
        std::shared_ptr<InterfaceRegistry> registry_;

        // Mock method for discovering interfaces.
        bool discover_linux_interfaces();
        bool use_mock_interfaces();
    };

} // namespace mesh_legacy
} // namespace wave_native
