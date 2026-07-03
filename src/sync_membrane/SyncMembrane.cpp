#include "SyncMembrane.hpp"
#include <cmath>

namespace wnn {
namespace sync {

SyncMembrane::SyncMembrane(SafetyMode mode, const PhaseShimmerTolerance& tol)
    : mode_(mode), tolerance_(tol) {}

bool SyncMembrane::tolerate_phase_shimmer(const AtmosphereState& atm, const wnn::infra::BeamState& beam) const {
    // Calculate expected phase deviation from AtmosphereState
    double delta_phi_expected = (K_CLOUD * atm.cloud_density) +
                                (K_RAIN * atm.rain_intensity) +
                                (K_TURB * atm.turbulence_index);

    // As a simplification, we map the observed phase variation to the absolute phase.
    // In a real scenario, this would be computed against a local reference oscillator's phase state.
    // Here we use the beam.phase directly as the observed relative phase for the sake of tolerance logic.
    double delta_phi_observed = std::abs(beam.phase);

    // If in strict mode, any latency/shimmer that significantly exceeds strictly defined limits is rejected
    if (mode_ == SafetyMode::Strict) {
        if (delta_phi_observed > delta_phi_expected + tolerance_.max_phase_variation) {
            return false;
        }
    } else {
        // Relaxed mode, potentially allow slightly higher threshold
        // As a prototype, relaxed mode doubles the allowed maximum variation margin
        if (delta_phi_observed > delta_phi_expected + 2.0 * tolerance_.max_phase_variation) {
            return false;
        }
    }

    return true;
}

double SyncMembrane::phi_threshold() const {
    // Determine stability threshold based on safety mode
    if (mode_ == SafetyMode::Strict) {
        return 0.1; // Tight tolerance for strict mode (< 100 microseconds intent)
    }
    return 0.5; // Looser tolerance for relaxed
}

StabilityDivergence SyncMembrane::compute_stability_divergence(const SaltEpoch& salt, const wnn::infra::BeamState& beam) const {
    StabilityDivergence div;

    // A surrogate for expected phase divergence using the phase salt
    double delta_phi_expected = salt.delta_salt;

    // A simplified model of the observed divergence, normally you would calculate
    // divergence against the WNN state vector over time. We approximate by taking
    // the difference between incoming phase and salt signature.
    double delta_phi_observed = beam.phase;

    div.delta_phi = std::abs(delta_phi_observed - delta_phi_expected);
    div.suspicious = detect_replay_attack(div);

    return div;
}

bool SyncMembrane::detect_replay_attack(const StabilityDivergence& div) const {
    return div.delta_phi > phi_threshold();
}

} // namespace sync
} // namespace wnn
