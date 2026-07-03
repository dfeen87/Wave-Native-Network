#include "PhaseCoherenceAnchor.hpp"
#include <cmath>
#include <numbers>

namespace wnn {
namespace space {

// Speed of light in meters per second
constexpr double C_MPS = 299792458.0;

PhaseCoherenceAnchor::PhaseCoherenceAnchor(const PhaseAnchorConfig& cfg)
    : config_(cfg) {}

void PhaseCoherenceAnchor::apply_relativistic_tuning(const OrbitalState& state, PhaseAnchorSignal& sig) const {
    // Time dilation (special relativity, low-velocity approximation): gamma ≈ 1 + 1/2 * (v/c)^2
    double v_c = state.velocity_mps / C_MPS;
    double gamma = 1.0 + 0.5 * (v_c * v_c);

    // Apply the relativistic compensation factor
    gamma *= config_.relativistic_comp_factor;

    // Adjust effective omega based on time dilation
    sig.omega_effective = config_.base_omega / gamma;

    // Doppler shift (line-of-sight component approximation)
    // Assuming simple radial velocity relationship for demonstration, v_radial roughly scales with velocity_mps
    // in a full model this depends on receiver position relative to orbital state
    double v_radial = state.velocity_mps * 0.1; // Placeholder radial component
    double omega_doppler = sig.omega_effective * (1.0 + v_radial / C_MPS);

    // Update phase with doppler component over arbitrary delta_t (using timestamp as an accumulated delta)
    sig.phase += omega_doppler * state.timestamp;

    // Normalize phase to [0, 2pi)
    sig.phase = std::fmod(sig.phase, 2.0 * std::numbers::pi);
    if (sig.phase < 0) {
        sig.phase += 2.0 * std::numbers::pi;
    }
}

PhaseAnchorSignal PhaseCoherenceAnchor::compute_signal(const OrbitalState& state) const {
    PhaseAnchorSignal sig;
    sig.omega_effective = config_.base_omega;
    sig.phase = 0.0; // Base phase
    sig.power_dbm = 40.0; // Simulated nominal broadcast power

    apply_relativistic_tuning(state, sig);

    return sig;
}

bool verify_orthogonal_handshake(const AnchorLink& link) {
    // Enforce ~90° cross-plane verification for “perpendicular mesh” integrity.
    // Tolerance of +/- 2.0 degrees
    double diff = std::abs(link.angle_deg - 90.0);
    return diff <= 2.0;
}

} // namespace space
} // namespace wnn
