#include "BareMetalEngine.hpp"
#include <cmath>
#include <algorithm>

namespace wnn::infra {

HandoffMetric BareMetalEngine::evaluate_handoff(const VehicleState& v, const BeamState& current, const BeamState& next) const {
    double delta_phi = std::fabs(next.phase - current.phase);
    double m_phi = std::clamp(delta_phi / 3.14159265358979323846, 0.0, 1.0);
    double delta_f = std::fabs(next.frequency_hz - current.frequency_hz);
    double m_f = (current.frequency_hz != 0.0) ? std::clamp(delta_f / current.frequency_hz, 0.0, 1.0) : 0.0;
    double continuity_score = 1.0 - (0.7 * m_phi + 0.3 * m_f);

    HandoffMetric metric;
    metric.continuity_score = continuity_score;
    metric.phase_mismatch = delta_phi;
    metric.candidate_valid = continuity_score >= 0.950;
    return metric;
}

} // namespace wnn::infra
