#include "BareMetalEngine.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace wnn {
namespace infra {

HandoffMetric BareMetalEngine::evaluate_handoff(const VehicleState& v, const BeamState& current, const BeamState& next) const {
    HandoffMetric metric;

    // Calculate absolute phase mismatch
    metric.phase_mismatch = std::abs(current.phase - next.phase);

    // Normalize phase mismatch to [0, 1] based on pi
    double m_phi = metric.phase_mismatch / std::numbers::pi;
    m_phi = std::clamp(m_phi, 0.0, 1.0);

    // Calculate normalized frequency mismatch to [0, 1]
    double delta_f = std::abs(current.frequency_hz - next.frequency_hz);
    double m_f = 0.0;
    if (current.frequency_hz > 0.0) {
        m_f = delta_f / current.frequency_hz;
    }
    m_f = std::clamp(m_f, 0.0, 1.0);

    // Calculate composite continuity score
    // Weight parameters (can be adjusted or configurable later)
    constexpr double w_phi = 0.7;
    constexpr double w_f = 0.3;

    metric.continuity_score = 1.0 - (w_phi * m_phi + w_f * m_f);

    // Valid if continuity score meets target
    metric.candidate_valid = (metric.continuity_score >= 0.950);

    return metric;
}

} // namespace infra
} // namespace wnn
