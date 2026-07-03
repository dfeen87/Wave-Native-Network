#include "mesh_discovery.hpp"

namespace wave_native {
namespace core {

DiscoveryState MeshDiscovery::classify_node(const MeshNodeInfo& info) {
    if (!info.is_wan_candidate) {
        return DiscoveryState::Rejected;
    }

    // High epsilon means high variation/chaos, likely rejected for stability
    if (info.epsilon > 0.5) {
        return DiscoveryState::Rejected;
    }

    // Between 0.1 and 0.5 could be candidate
    if (info.epsilon > 0.1) {
        return DiscoveryState::Candidate;
    }

    // Very stable, accept
    return DiscoveryState::Accepted;
}

double MeshDiscovery::estimate_mesh_density(const std::vector<MeshNodeInfo>& nodes) {
    if (nodes.empty()) return 0.0;

    double total_density = 0.0;
    for (const auto& node : nodes) {
        total_density += node.neighbor_density;
    }

    return total_density / nodes.size();
}

} // namespace core
} // namespace wave_native
