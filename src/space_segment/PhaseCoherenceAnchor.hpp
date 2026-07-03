#ifndef WNN_SPACE_SEGMENT_PHASE_COHERENCE_ANCHOR_HPP
#define WNN_SPACE_SEGMENT_PHASE_COHERENCE_ANCHOR_HPP

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

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

    double compute_trust_score() const;
    bool verify_orthogonal_handshake(const struct AnchorLink& link);

    // Test mutators to adjust internal state
    void record_handshake_success(bool success);
    void record_coherence_failure();
    void update_phase_stability(double stability);

    double get_phase_stability_score() const { return phase_stability_score_; }

private:
    PhaseAnchorConfig config_;

    // Internal metrics for trust score
    double phase_stability_score_ = 1.0;
    std::uint32_t successful_handshakes_ = 0;
    std::uint32_t total_handshakes_ = 0;
    std::uint32_t coherence_failures_ = 0;
};

struct AnchorLink {
    PhaseCoherenceAnchor* a;
    PhaseCoherenceAnchor* b;
    double angle_deg;
    bool orthogonal_verified;
};

struct CoherenceCluster {
    std::vector<PhaseCoherenceAnchor*> anchors;
    double cluster_coherence_score;
    double average_phase_deg;
};

std::vector<CoherenceCluster> build_coherence_clusters(std::vector<PhaseCoherenceAnchor>& anchors);

} // namespace space
} // namespace wnn

#endif // WNN_SPACE_SEGMENT_PHASE_COHERENCE_ANCHOR_HPP
