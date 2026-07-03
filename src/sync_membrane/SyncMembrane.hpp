#ifndef WNN_SYNC_MEMBRANE_SYNC_MEMBRANE_HPP
#define WNN_SYNC_MEMBRANE_SYNC_MEMBRANE_HPP

#include <cstdint>
#include <chrono>
#include <string>
#include "../infrastructure_segment/BareMetalEngine.hpp"

namespace wnn {
namespace sync {

enum class SafetyMode { Strict, Relaxed };

struct AtmosphereState {
    double cloud_density;
    double rain_intensity;
    double turbulence_index;
};

struct PhaseShimmerTolerance {
    double max_latency_ms;
    double max_phase_variation;
};

struct SaltEpoch {
    uint64_t epoch_id;
    double delta_salt;
};

struct StabilityDivergence {
    double delta_phi;
    bool suspicious;
};

struct SyncState {
    double phase_deg;
    double drift_ppm;
    double jitter_ns;
    double skew_ns;
    std::uint64_t epoch_id;
};

struct SyncEvent {
    std::uint64_t epoch_id;
    double requested_phase_adjust_deg;
    double requested_drift_correction_ppm;
    std::string reason;
};

class SyncMembrane {
public:
    SyncMembrane(SafetyMode mode, const PhaseShimmerTolerance& tol);

    bool tolerate_phase_shimmer(const AtmosphereState& atm, const wnn::infra::BeamState& beam) const;

    StabilityDivergence compute_stability_divergence(const SaltEpoch& salt, const wnn::infra::BeamState& beam) const;
    bool detect_replay_attack(const StabilityDivergence& div) const;

    double compute_phase_drift(const SyncState& state);
    void trigger_resync(const SyncEvent& event);

    // Getters for membrane health metrics
    double get_jitter_ns() const { return jitter_ns_; }
    double get_skew_ns() const { return skew_ns_; }
    double get_drift_ppm() const { return drift_ppm_; }

private:
    std::uint64_t epoch_id_ = 0;
    std::chrono::steady_clock::time_point epoch_start_time_;
    double epoch_drift_accumulator_ = 0.0;

    double jitter_ns_ = 0.0;
    double skew_ns_ = 0.0;
    double drift_ppm_ = 0.0;

    SafetyMode mode_;
    PhaseShimmerTolerance tolerance_;

    // Empirical or configurable constants for expected phase deviation
    static constexpr double K_CLOUD = 0.05;
    static constexpr double K_RAIN = 0.1;
    static constexpr double K_TURB = 0.15;

    // Threshold for detecting suspicious divergence based on mode
    double phi_threshold() const;
};

} // namespace sync
} // namespace wnn

#endif // WNN_SYNC_MEMBRANE_SYNC_MEMBRANE_HPP
