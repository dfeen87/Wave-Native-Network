#include "space_segment/PhaseCoherenceAnchor.hpp"
#include <cmath>

namespace wnn {
namespace space {

PhaseCoherenceAnchor::PhaseCoherenceAnchor(const PhaseAnchorConfig& cfg)
    : config_(cfg) {}

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

std::vector<CoherenceCluster> build_coherence_clusters(std::vector<PhaseCoherenceAnchor>& anchors) {
    std::vector<CoherenceCluster> clusters;

    // Very simple clustering: group anchors with high trust scores together
    CoherenceCluster main_cluster;
    main_cluster.cluster_coherence_score = 0.0;
    main_cluster.average_phase_deg = 0.0;

    double total_score = 0.0;
    for (auto& anchor : anchors) {
        if (anchor.compute_trust_score() > 0.6) {
            main_cluster.anchors.push_back(&anchor);
            total_score += anchor.compute_trust_score();
            // Placeholder for real phase math:
            main_cluster.average_phase_deg += 0.0;
        }
    }

    if (!main_cluster.anchors.empty()) {
        main_cluster.cluster_coherence_score = total_score / main_cluster.anchors.size();
        clusters.push_back(main_cluster);
    }

    return clusters;
}

} // namespace space
} // namespace wnn
