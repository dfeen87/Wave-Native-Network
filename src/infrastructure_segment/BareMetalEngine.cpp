#include "infrastructure_segment/BareMetalEngine.hpp"
#include <cmath>
#include <algorithm>

namespace wnn {
namespace infra {

HandoffMetric BareMetalEngine::evaluate_handoff(
    const VehicleState& v,
    const BeamState& current,
    const BeamState& next
) const {
    constexpr double pi = 3.141592653589793;

    // Phase mismatch (absolute difference, normalized to [0, 1] by pi)
    double dphi = std::fabs(next.phase - current.phase);
    double phase_mismatch = dphi / pi;
    phase_mismatch = std::clamp(phase_mismatch, 0.0, 1.0);

    // Frequency mismatch (relative difference, normalized to [0, 1])
    double df = std::fabs(next.frequency_hz - current.frequency_hz);
    double freq_mismatch = (current.frequency_hz > 0.0)
        ? df / current.frequency_hz
        : 1.0;
    freq_mismatch = std::clamp(freq_mismatch, 0.0, 1.0);

    // Composite continuity score
    double continuity = 1.0 - (0.7 * phase_mismatch + 0.3 * freq_mismatch);

    HandoffMetric metric{};
    metric.continuity_score = continuity;
    metric.phase_mismatch   = phase_mismatch;
    metric.candidate_valid  = (continuity >= 0.950);

    return metric;
}

} // namespace infra
} // namespace wnn
