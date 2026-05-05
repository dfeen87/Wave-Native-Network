#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#include <cstdint>

namespace wave_native {
namespace mesh_legacy {

    enum class NodeKind {
        Universal,
        Open,
        Standard
    };

    enum class NodeConnectivityStatus {
        Online,
        Offline,
        Unknown
    };

    struct RoutingHop {
        std::string node_id;
        NodeKind kind;
    };

    struct PeerRoute {
        std::string source_node_id;
        std::vector<RoutingHop> hops;

        bool is_direct() const {
            return hops.empty();
        }
    };

    class PeerRouter {
    public:
        PeerRouter() = default;

        void update_node(const std::string& node_id, const std::string& node_type, NodeConnectivityStatus status);
        void remove_node(const std::string& node_id);

        NodeConnectivityStatus connectivity_status(const std::string& node_id) const;
        std::vector<std::string> online_nodes() const;

        std::optional<PeerRoute> find_route(const std::string& source_node_id) const;

        static NodeKind from_node_type(const std::string& node_type);
        static bool can_relay(NodeKind kind);

    private:
        std::map<std::string, NodeConnectivityStatus> connectivity_;
        std::map<std::string, NodeKind> kinds_;
    };

} // namespace mesh_legacy
} // namespace wave_native
