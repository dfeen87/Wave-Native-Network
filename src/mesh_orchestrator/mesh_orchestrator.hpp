#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include "../space_segment/PhaseCoherenceAnchor.hpp"
#include "../config/wnn_config.hpp"

namespace wave_native {
namespace core {

using PeerId = std::vector<uint8_t>;

struct MeshNodeState {
    PeerId peer_id;
    std::optional<wnn::space::AnchorId> anchor_id;
    double mesh_density_local;
    double trust_score;
    double phase_error_deg;
    bool pll_locked;
    bool coherence_stable;
};

class MeshOrchestrator {
public:
    MeshOrchestrator(const WnnConfig& config);

    void set_mesh_nodes(const std::vector<MeshNodeState>& nodes);
    void set_network_pll_locked(bool locked);
    void set_coherence_clusters(const std::vector<wnn::space::CoherenceCluster>& clusters);

    void step(double timestamp_ms);

    const std::vector<MeshNodeState>& nodes() const noexcept;
    bool is_mesh_healthy() const noexcept;

private:
    bool evaluate_node_health(const MeshNodeState& node) const;
    bool evaluate_global_health() const;

    std::vector<MeshNodeState> nodes_;
    bool network_pll_locked_ = true;
    bool mesh_healthy_ = true;
    std::vector<wnn::space::CoherenceCluster> coherence_clusters_;
    const WnnConfig& config_;
};

} // namespace core
} // namespace wave_native
