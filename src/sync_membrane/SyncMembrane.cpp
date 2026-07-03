#include "SyncMembrane.hpp"
#include <cmath>

namespace wnn::sync {

SyncMembrane::SyncMembrane(SafetyMode mode, const PhaseShimmerTolerance& tol) : mode_(mode), tolerance_(tol) {}

bool SyncMembrane::tolerate_phase_shimmer(const AtmosphereState& atm, const wnn::infra::BeamState& beam) const {
    double expected = K_CLOUD * atm.cloud_density + K_RAIN * atm.rain_intensity + K_TURB * atm.turbulence_index;
    double observed = std::fabs(beam.phase);
    return observed <= expected + tolerance_.max_phase_variation;
}

double SyncMembrane::phi_threshold() const {
    return mode_ == SafetyMode::Strict ? 0.1 : 0.5;
}

StabilityDivergence SyncMembrane::compute_stability_divergence(const SaltEpoch& salt, const wnn::infra::BeamState& beam) const {
    double expected = salt.delta_salt;
    double observed = beam.phase;
    double delta_phi = std::fabs(observed - expected);
    StabilityDivergence div;
    div.delta_phi = delta_phi;
    div.suspicious = delta_phi > phi_threshold();
    return div;
}

bool SyncMembrane::detect_replay_attack(const StabilityDivergence& div) const {
    return div.suspicious;
}

} // namespace wnn::sync
