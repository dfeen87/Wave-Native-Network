#pragma once

#include <vector>

namespace wave_native {
namespace core {

struct MeshNodeInfo {
    bool is_wan_candidate;
    double epsilon;
    double neighbor_density;
    // ...
};

enum class DiscoveryState {
    Probing,
    Candidate,
    Accepted,
    Rejected
};

class MeshDiscovery {
public:
    DiscoveryState classify_node(const MeshNodeInfo& info);
    double estimate_mesh_density(const std::vector<MeshNodeInfo>& nodes);
};

} // namespace core
} // namespace wave_native
