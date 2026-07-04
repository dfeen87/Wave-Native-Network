#pragma once

#include <vector>
#include <cstdint>

namespace wave_native {
namespace core {

using AnchorId = std::uint32_t;

struct PllNodeState {
    AnchorId anchor_id;
    double phase_error_deg;
    double lock_time_ms;
    double overshoot_deg;
    double settling_time_ms;
    bool is_locked;
};

class DistributedPllController {
public:
    DistributedPllController(double global_lock_tolerance_deg, double global_settling_tolerance_ms);

    void set_nodes(const std::vector<AnchorId>& anchors);
    void update_phase_error(AnchorId anchor_id, double phase_error_deg, double timestamp_ms);
    void step(double timestamp_ms);

    const std::vector<PllNodeState>& node_states() const noexcept;
    bool is_network_locked() const;

private:
    std::vector<PllNodeState> nodes_;
    double global_lock_tolerance_deg_;
    double global_settling_tolerance_ms_;
    double last_timestamp_ms_;
    bool first_step_;
};

} // namespace core
} // namespace wave_native
