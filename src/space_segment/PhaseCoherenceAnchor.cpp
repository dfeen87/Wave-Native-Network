#include "PhaseCoherenceAnchor.hpp"
#include <cmath>

namespace wnn::space {

PhaseCoherenceAnchor::PhaseCoherenceAnchor(const PhaseAnchorConfig& cfg) : config_(cfg) {}

PhaseAnchorSignal PhaseCoherenceAnchor::compute_signal(const OrbitalState& state) const {
    PhaseAnchorSignal sig;
    sig.power_dbm = 40.0;
    sig.omega_effective = config_.base_omega;
    sig.phase = 0.0;
    apply_relativistic_tuning(state, sig);
    return sig;
}

void PhaseCoherenceAnchor::apply_relativistic_tuning(const OrbitalState& state, PhaseAnchorSignal& sig) const {
    double v = state.velocity_mps;
    double c = 299792458.0;
    double gamma = 1.0 + 0.5 * (v / c) * (v / c);
    double omega_doppler = config_.base_omega * (1.0 + v / c);
    sig.omega_effective = omega_doppler / gamma;
    sig.phase = sig.phase + omega_doppler * state.timestamp;
}

bool verify_orthogonal_handshake(const AnchorLink& link) {
    return std::fabs(link.angle_deg - 90.0) <= 5.0;
}

} // namespace wnn::space
