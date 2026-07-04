#include "distributed_pll_controller.hpp"
#include <cmath>
#include <algorithm>

namespace wave_native {
namespace core {

DistributedPllController::DistributedPllController(double global_lock_tolerance_deg, double global_settling_tolerance_ms)
    : global_lock_tolerance_deg_(global_lock_tolerance_deg),
      global_settling_tolerance_ms_(global_settling_tolerance_ms),
      last_timestamp_ms_(0.0),
      first_step_(true) {}

void DistributedPllController::set_nodes(const std::vector<AnchorId>& anchors) {
    std::vector<PllNodeState> new_nodes;
    new_nodes.reserve(anchors.size());

    for (AnchorId anchor_id : anchors) {
        // Try to preserve existing state if possible
        auto it = std::find_if(nodes_.begin(), nodes_.end(),
            [anchor_id](const PllNodeState& state) { return state.anchor_id == anchor_id; });

        if (it != nodes_.end()) {
            new_nodes.push_back(*it);
        } else {
            // New node initialization
            PllNodeState new_state{};
            new_state.anchor_id = anchor_id;
            new_state.phase_error_deg = 0.0;
            new_state.lock_time_ms = 0.0;
            new_state.overshoot_deg = 0.0;
            new_state.settling_time_ms = 0.0;
            new_state.is_locked = false;
            new_nodes.push_back(new_state);
        }
    }

    nodes_ = std::move(new_nodes);
}

void DistributedPllController::update_phase_error(AnchorId anchor_id, double phase_error_deg, double timestamp_ms) {
    (void)timestamp_ms; // Using step() timestamp_ms for progression instead
    for (auto& node : nodes_) {
        if (node.anchor_id == anchor_id) {
            node.phase_error_deg = phase_error_deg;
            // Update overshoot when updating phase error
            if (std::abs(phase_error_deg) > node.overshoot_deg) {
                node.overshoot_deg = std::abs(phase_error_deg);
            }
            break;
        }
    }
}

void DistributedPllController::step(double timestamp_ms) {
    if (first_step_) {
        last_timestamp_ms_ = timestamp_ms;
        first_step_ = false;
        return;
    }

    double dt_ms = timestamp_ms - last_timestamp_ms_;
    last_timestamp_ms_ = timestamp_ms;

    if (dt_ms <= 0.0) return;

    for (auto& node : nodes_) {
        bool currently_within_tolerance = std::abs(node.phase_error_deg) <= global_lock_tolerance_deg_;

        if (currently_within_tolerance) {
            if (!node.is_locked) {
                // Was not locked, now might be accumulating settling time
                node.settling_time_ms += dt_ms;
                node.is_locked = true;
            } else {
                // Is locked, keep accumulating lock time
                node.lock_time_ms += dt_ms;
            }
        } else {
            // Broke tolerance
            node.is_locked = false;
            node.lock_time_ms = 0.0;
            node.settling_time_ms += dt_ms;
            // Leave overshoot as the max seen
        }
    }
}

const std::vector<PllNodeState>& DistributedPllController::node_states() const noexcept {
    return nodes_;
}

bool DistributedPllController::is_network_locked() const {
    if (nodes_.empty()) {
        return false;
    }

    for (const auto& node : nodes_) {
        if (!node.is_locked) {
            return false;
        }
        if (std::abs(node.phase_error_deg) > global_lock_tolerance_deg_) {
            return false;
        }
        if (node.settling_time_ms > global_settling_tolerance_ms_) {
            return false;
        }
    }

    return true;
}

} // namespace core
} // namespace wave_native