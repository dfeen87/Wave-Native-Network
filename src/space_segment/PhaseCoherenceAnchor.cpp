#include "space_segment/PhaseCoherenceAnchor.hpp"
#include <cmath>

namespace wnn {
namespace space {

PhaseCoherenceAnchor::PhaseCoherenceAnchor(AnchorId id, const PhaseAnchorConfig& cfg)
    : id_(id), config_(cfg) {}

PhaseAnchorSignal PhaseCoherenceAnchor::compute_signal(const OrbitalState& state) const {
    // Start with baseline signal
    PhaseAnchorSignal sig{};
    sig.power_dbm = 40.0;                 // deterministic baseline power
    sig.omega_effective = config_.base_omega;
    sig.phase = 0.0;

    // Apply relativistic + Doppler tuning
    apply_relativistic_tuning(state, sig);

    return sig;
}

void PhaseCoherenceAnchor::apply_relativistic_tuning(
    const OrbitalState& state,
    PhaseAnchorSignal& sig
) const {
    constexpr double c = 299792458.0;

    double v = state.velocity_mps;

    // Relativistic gamma approximation
    double gamma = 1.0 + 0.5 * (v / c) * (v / c);

    // Doppler shift approximation
    double omega_doppler = config_.base_omega * (1.0 + v / c);

    // Effective frequency after relativistic correction
    sig.omega_effective = omega_doppler / gamma;

    // Phase accumulation using timestamp
    sig.phase += omega_doppler * state.timestamp;
}

bool PhaseCoherenceAnchor::verify_orthogonal_handshake(const AnchorLink& link) {
    bool is_orthogonal = std::fabs(link.angle_deg - 90.0) <= 5.0;
    record_handshake_success(is_orthogonal);
    return is_orthogonal;
}

void PhaseCoherenceAnchor::record_handshake_success(bool success) {
    total_handshakes_++;
    if (success) {
        successful_handshakes_++;
    }
}

void PhaseCoherenceAnchor::record_coherence_failure() {
    coherence_failures_++;
}

void PhaseCoherenceAnchor::update_phase_stability(double stability) {
    // Expected stability in [0, 1] range
    phase_stability_score_ = std::clamp(stability, 0.0, 1.0);
}

double PhaseCoherenceAnchor::compute_trust_score() const {
    double handshake_success_rate = total_handshakes_ > 0 ? static_cast<double>(successful_handshakes_) / total_handshakes_ : 1.0;

    // Penalize score by coherence failures, using an exponential decay model
    double failure_penalty = std::exp(-0.1 * coherence_failures_);

    // Combine phase stability, handshake success rate, and failure penalty
    return 0.4 * phase_stability_score_ + 0.4 * handshake_success_rate + 0.2 * failure_penalty;
}

CoherenceEngine::CoherenceEngine(const wave_native::core::WnnConfig& config)
    : config_(config) {}

void CoherenceEngine::add_anchor(const PhaseCoherenceAnchor& anchor) {
    anchors_.push_back(anchor);
}

bool CoherenceEngine::has_anchor(AnchorId anchor_id) const {
    for (const auto& anchor : anchors_) {
        if (anchor.id() == anchor_id) {
            return true;
        }
    }
    return false;
}

void CoherenceEngine::update_anchor_stability(AnchorId anchor_id, double stability) {
    for (auto& anchor : anchors_) {
        if (anchor.id() == anchor_id) {
            anchor.update_phase_stability(stability);
            return;
        }
    }
}

std::vector<CoherenceCluster> CoherenceEngine::build_coherence_clusters() const {
    std::vector<CoherenceCluster> clusters;

    // Very simple clustering logic for this implementation:
    // We'll put anchors into a single main cluster for now, filtering by trust threshold.
    // In a real system, you would group by phase difference.

    CoherenceCluster main_cluster;
    main_cluster.average_phase_deg = 0.0;
    main_cluster.cluster_coherence_score = 0.0;
    main_cluster.average_trust_score = 0.0;
    main_cluster.is_stable = false;

    double total_score = 0.0;
    double total_phase_error = 0.0; // Simulated phase error

    for (const auto& anchor : anchors_) {
        double trust = compute_trust_score(anchor.id());

        // As a simplification, group all valid anchors together
        main_cluster.anchors.push_back(anchor.id());
        total_score += trust;

        // Placeholder for real phase difference calculation
        total_phase_error += 0.0; // Assume perfect phase for this demo
    }

    if (!main_cluster.anchors.empty()) {
        main_cluster.average_trust_score = total_score / main_cluster.anchors.size();
        main_cluster.cluster_coherence_score = main_cluster.average_trust_score; // Alias for simplicity

        // Simulated average phase error
        double average_phase_error = total_phase_error / main_cluster.anchors.size();

        // Feed the simulated average phase error back to the cluster struct so others can track it
        main_cluster.average_phase_deg = average_phase_error;

        main_cluster.is_stable = (average_phase_error < config_.coherence_phase_error_threshold) &&
                                 (main_cluster.average_trust_score > config_.coherence_trust_threshold) &&
                                 (main_cluster.anchors.size() >= config_.coherence_min_cluster_size);

        clusters.push_back(main_cluster);
    }

    return clusters;
}

double CoherenceEngine::compute_trust_score(AnchorId anchor_id) const {
    for (const auto& anchor : anchors_) {
        if (anchor.id() == anchor_id) {
            return anchor.compute_trust_score();
        }
    }
    return 0.0; // Not found
}

double CoherenceEngine::compute_multi_anchor_trust(const std::vector<AnchorId>& anchor_ids) const {
    if (anchor_ids.empty()) return 0.0;

    double total_trust = 0.0;
    std::size_t valid_anchors = 0;

    for (AnchorId id : anchor_ids) {
        double trust = compute_trust_score(id);
        if (trust > 0.0) { // Assume 0.0 means not found or fully untrusted
            total_trust += trust;
            valid_anchors++;
        }
    }

    if (valid_anchors == 0) return 0.0;
    return total_trust / valid_anchors;
}

} // namespace space
} // namespace wnn
