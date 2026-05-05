#include "peer_routing.hpp"

namespace wave_native {
namespace mesh_legacy {

    NodeKind PeerRouter::from_node_type(const std::string& node_type) {
        std::string type = node_type;
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);

        if (type == "universal") {
            return NodeKind::Universal;
        } else if (type == "open" || type == "gateway") {
            return NodeKind::Open;
        } else {
            return NodeKind::Standard;
        }
    }

    bool PeerRouter::can_relay(NodeKind kind) {
        return kind == NodeKind::Universal || kind == NodeKind::Open;
    }

    void PeerRouter::update_node(const std::string& node_id, const std::string& node_type, NodeConnectivityStatus status) {
        connectivity_[node_id] = status;
        kinds_[node_id] = from_node_type(node_type);
    }

    void PeerRouter::remove_node(const std::string& node_id) {
        connectivity_.erase(node_id);
        kinds_.erase(node_id);
    }

    NodeConnectivityStatus PeerRouter::connectivity_status(const std::string& node_id) const {
        auto it = connectivity_.find(node_id);
        if (it != connectivity_.end()) {
            return it->second;
        }
        return NodeConnectivityStatus::Unknown;
    }

    std::vector<std::string> PeerRouter::online_nodes() const {
        std::vector<std::string> nodes;
        for (const auto& [id, status] : connectivity_) {
            if (status == NodeConnectivityStatus::Online) {
                nodes.push_back(id);
            }
        }
        return nodes;
    }

    std::optional<PeerRoute> PeerRouter::find_route(const std::string& source_node_id) const {
        NodeConnectivityStatus source_status = connectivity_status(source_node_id);

        if (source_status == NodeConnectivityStatus::Online) {
            PeerRoute route;
            route.source_node_id = source_node_id;
            return route;
        }

        std::vector<std::pair<std::string, NodeKind>> candidates;
        for (const auto& [id, status] : connectivity_) {
            if (id != source_node_id && status == NodeConnectivityStatus::Online) {
                auto kind_it = kinds_.find(id);
                NodeKind kind = (kind_it != kinds_.end()) ? kind_it->second : NodeKind::Standard;

                if (can_relay(kind)) {
                    candidates.push_back({id, kind});
                }
            }
        }

        if (candidates.empty()) {
            return std::nullopt;
        }

        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            auto rank = [](NodeKind k) -> uint8_t {
                switch (k) {
                    case NodeKind::Universal: return 0;
                    case NodeKind::Open: return 1;
                    case NodeKind::Standard: return 2;
                    default: return 2;
                }
            };
            uint8_t rank_a = rank(a.second);
            uint8_t rank_b = rank(b.second);
            if (rank_a != rank_b) {
                return rank_a < rank_b;
            }
            return a.first < b.first;
        });

        PeerRoute route;
        route.source_node_id = source_node_id;

        RoutingHop hop;
        hop.node_id = candidates[0].first;
        hop.kind = candidates[0].second;
        route.hops.push_back(hop);

        return route;
    }

} // namespace mesh_legacy
} // namespace wave_native
