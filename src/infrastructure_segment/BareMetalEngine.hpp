#ifndef WNN_INFRASTRUCTURE_SEGMENT_BARE_METAL_ENGINE_HPP
#define WNN_INFRASTRUCTURE_SEGMENT_BARE_METAL_ENGINE_HPP

namespace wnn {
namespace infra {

struct VehicleState {
    double speed_mps;
    double heading_deg;
    double position_lat;
    double position_lon;
    double timestamp;
};

struct BeamState {
    double phase;
    double frequency_hz;
    double snr_db;
    double timestamp;
};

struct HandoffMetric {
    double continuity_score;   // target >= 0.950
    double phase_mismatch;     // |Delta_phi|
    bool candidate_valid;
};

class BareMetalEngine {
public:
    HandoffMetric evaluate_handoff(const VehicleState& v, const BeamState& current, const BeamState& next) const;
};

} // namespace infra
} // namespace wnn

#endif // WNN_INFRASTRUCTURE_SEGMENT_BARE_METAL_ENGINE_HPP
