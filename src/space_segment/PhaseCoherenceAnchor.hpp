#ifndef WNN_SPACE_SEGMENT_PHASE_COHERENCE_ANCHOR_HPP
#define WNN_SPACE_SEGMENT_PHASE_COHERENCE_ANCHOR_HPP

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "../config/wnn_config.hpp"

namespace wnn {
namespace space {

using AnchorId = std::uint32_t;

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
    PhaseCoherenceAnchor(AnchorId id, const PhaseAnchorConfig& cfg);

    AnchorId id() const noexcept { return id_; }

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
    AnchorId id_;
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
    std::vector<AnchorId> anchors;
    double average_phase_deg;
    double cluster_coherence_score;
    double average_trust_score;
    bool is_stable;
};

class CoherenceEngine {
public:
    explicit CoherenceEngine(const wave_native::core::WnnConfig& config);

    void add_anchor(const PhaseCoherenceAnchor& anchor);

    std::vector<CoherenceCluster> build_coherence_clusters() const;
    double compute_trust_score(AnchorId anchor_id) const;
    double compute_multi_anchor_trust(const std::vector<AnchorId>& anchor_ids) const;

private:
    wave_native::core::WnnConfig config_;
    std::vector<PhaseCoherenceAnchor> anchors_;
};

} // namespace space
} // namespace wnn

#endif // WNN_SPACE_SEGMENT_PHASE_COHERENCE_ANCHOR_HPP
