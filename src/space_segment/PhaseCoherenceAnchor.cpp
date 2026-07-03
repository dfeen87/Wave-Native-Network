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

// Free function — matches header exactly
bool verify_orthogonal_handshake(const AnchorLink& link) {
    return std::fabs(link.angle_deg - 90.0) <= 5.0;
}

} // namespace space
} // namespace wnn
