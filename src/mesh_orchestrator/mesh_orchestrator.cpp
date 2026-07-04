#include "mesh_orchestrator.hpp"
#include <algorithm>
#include <iostream>

namespace wave_native {
namespace core {

MeshOrchestrator::MeshOrchestrator(const WnnConfig& config) : config_(config) {}

void MeshOrchestrator::set_mesh_nodes(const std::vector<MeshNodeState>& nodes) {
    nodes_ = nodes;
}

void MeshOrchestrator::set_network_pll_locked(bool locked) {
    network_pll_locked_ = locked;
}

void MeshOrchestrator::set_coherence_clusters(const std::vector<wnn::space::CoherenceCluster>& clusters) {
    coherence_clusters_ = clusters;
}

void MeshOrchestrator::step(double /*timestamp_ms*/) {
    mesh_healthy_ = evaluate_global_health();
}

const std::vector<MeshNodeState>& MeshOrchestrator::nodes() const noexcept {
    return nodes_;
}

bool MeshOrchestrator::is_mesh_healthy() const noexcept {
    return mesh_healthy_;
}

bool MeshOrchestrator::evaluate_node_health(const MeshNodeState& node) const {
    if (node.trust_score < config_.mesh_min_trust_threshold) {
        return false;
    }
    if (std::abs(node.phase_error_deg) > config_.mesh_max_phase_error_deg) {
        return false;
    }
    if (!node.pll_locked) {
        return false;
    }
    if (!node.coherence_stable) {
        return false;
    }
    return true;
}

bool MeshOrchestrator::evaluate_global_health() const {
    if (!network_pll_locked_) {
        return false;
    }

    // Check if any critical coherence clusters are unstable
    // For now, if *any* coherence cluster is unstable, we consider the mesh potentially unhealthy,
    // or we could check if a majority are unstable. Wnn config defines a threshold.
    // Given the prompt: "no critical coherence clusters marked unstable"
    for (const auto& cluster : coherence_clusters_) {
        if (!cluster.is_stable) {
            return false;
        }
    }

    if (nodes_.empty()) {
        return true; // Vacuously healthy if no nodes
    }

    std::size_t healthy_count = 0;
    for (const auto& node : nodes_) {
        if (evaluate_node_health(node)) {
            healthy_count++;
        }
    }

    double healthy_ratio = static_cast<double>(healthy_count) / nodes_.size();
    if (healthy_ratio < config_.mesh_min_healthy_node_ratio) {
        return false;
    }

    return true;
}

} // namespace core
} // namespace wave_native
