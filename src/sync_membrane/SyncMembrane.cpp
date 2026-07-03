#include "sync_membrane/SyncMembrane.hpp"
#include <cmath>

namespace wnn {
namespace sync {

SyncMembrane::SyncMembrane(SafetyMode mode, const PhaseShimmerTolerance& tol)
    : mode_(mode), tolerance_(tol) {}

double SyncMembrane::phi_threshold() const {
    // Strict mode: tighter threshold, Relaxed: looser
    if (mode_ == SafetyMode::Strict) {
        return tolerance_.max_phase_variation;
    }
    return tolerance_.max_phase_variation * 1.5;
}

bool SyncMembrane::tolerate_phase_shimmer(
    const AtmosphereState& atm,
    const wnn::infra::BeamState& beam
) const {
    // Expected phase deviation from atmosphere
    double expected =
        K_CLOUD * atm.cloud_density +
        K_RAIN  * atm.rain_intensity +
        K_TURB  * atm.turbulence_index;

    // Observed phase deviation (simple model: phase vs SNR)
    double observed = std::fabs(beam.phase) / (beam.snr_db > 0.0 ? beam.snr_db : 1.0);

    // Allow within expected + tolerance
    return observed <= (expected + tolerance_.max_phase_variation);
}

StabilityDivergence SyncMembrane::compute_stability_divergence(
    const SaltEpoch& salt,
    const wnn::infra::BeamState& beam
) const {
    // Expected phase drift from salt epoch
    double expected = salt.delta_salt;

    // Observed drift from timestamp (simple scalar model)
    double observed = beam.phase;

    double delta = std::fabs(observed - expected);
    double threshold = phi_threshold();

    StabilityDivergence div{};
    div.delta_phi = delta;
    div.suspicious = (delta > threshold);
    return div;
}

bool SyncMembrane::detect_replay_attack(const StabilityDivergence& div) const {
    return div.suspicious;
}

} // namespace sync
} // namespace wnn
