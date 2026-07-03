#ifndef WNN_SYNC_MEMBRANE_SYNC_MEMBRANE_HPP
#define WNN_SYNC_MEMBRANE_SYNC_MEMBRANE_HPP

#include <cstdint>
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

class SyncMembrane {
public:
    SyncMembrane(SafetyMode mode, const PhaseShimmerTolerance& tol);

    bool tolerate_phase_shimmer(const AtmosphereState& atm, const wnn::infra::BeamState& beam) const;

    StabilityDivergence compute_stability_divergence(const SaltEpoch& salt, const wnn::infra::BeamState& beam) const;
    bool detect_replay_attack(const StabilityDivergence& div) const;

private:
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
