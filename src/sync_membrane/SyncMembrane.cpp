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

double SyncMembrane::compute_phase_drift(const SyncState& state) {
    // Basic modeling of temporal fabric drift based on state properties
    // Integrates state drift with local accumulator
    double computed_drift = state.drift_ppm + (state.jitter_ns / 1000.0) + (state.skew_ns / 10000.0);

    if (state.epoch_id == epoch_id_) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - epoch_start_time_;
        // Accumulate drift over time
        epoch_drift_accumulator_ += computed_drift * elapsed.count() * 0.001;
    }

    // Update internal health metrics for tracking
    jitter_ns_ = 0.9 * jitter_ns_ + 0.1 * state.jitter_ns;
    skew_ns_ = 0.9 * skew_ns_ + 0.1 * state.skew_ns;
    drift_ppm_ = 0.9 * drift_ppm_ + 0.1 * state.drift_ppm;

    return epoch_drift_accumulator_;
}

void SyncMembrane::trigger_resync(const SyncEvent& event) {
    // Reset temporal state for new epoch
    epoch_id_ = event.epoch_id;
    epoch_start_time_ = std::chrono::steady_clock::now();

    // Apply requested corrections
    epoch_drift_accumulator_ -= event.requested_drift_correction_ppm;
    drift_ppm_ -= event.requested_drift_correction_ppm;

    // Partially clear jitter/skew assuming sync corrects them
    jitter_ns_ *= 0.5;
    skew_ns_ *= 0.5;
}

} // namespace sync
} // namespace wnn
