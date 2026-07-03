#ifndef WNN_SPACE_SEGMENT_PHASE_COHERENCE_ANCHOR_HPP
#define WNN_SPACE_SEGMENT_PHASE_COHERENCE_ANCHOR_HPP

namespace wnn {
namespace space {

struct OrbitalState {
    double altitude_km;
    double velocity_mps;
    double inclination_deg;
    double timestamp;
};

struct PhaseAnchorConfig {
    double base_omega;
    double relativistic_comp_factor;
};

struct PhaseAnchorSignal {
    double omega_effective;
    double phase;
    double power_dbm;
};

class PhaseCoherenceAnchor {
public:
    explicit PhaseCoherenceAnchor(const PhaseAnchorConfig& cfg);

    PhaseAnchorSignal compute_signal(const OrbitalState& state) const;
    void apply_relativistic_tuning(const OrbitalState& state, PhaseAnchorSignal& sig) const;

private:
    PhaseAnchorConfig config_;
};

struct AnchorLink {
    PhaseCoherenceAnchor* a;
    PhaseCoherenceAnchor* b;
    double angle_deg;
    bool orthogonal_verified;
};

bool verify_orthogonal_handshake(const AnchorLink& link);

} // namespace space
} // namespace wnn

#endif // WNN_SPACE_SEGMENT_PHASE_COHERENCE_ANCHOR_HPP
