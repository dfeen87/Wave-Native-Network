#pragma once

#include <vector>
#include <deque>
#include <mutex>
#include <iostream>
#include <cmath>
#include <algorithm>

#include "routing_engine.hpp"
#include "pll_controller.hpp"
#include "wave_state.hpp"

namespace wave_native {
namespace core {

class SafetyGuardrails {
public:
    SafetyGuardrails(RoutingEngine* routing_engine, PllController* pll, DuffingOscillator* duffing);

    // Process a batch of newly observed Inter-Arrival Times (IAT)
    void process_iat_samples(const std::vector<double>& iats);

    bool is_go_dark_active() const { return go_dark_active_; }
    double get_current_safety_score() const { return current_safety_score_; }

private:
    RoutingEngine* routing_engine_;
    PllController* pll_;
    DuffingOscillator* duffing_;

    std::deque<double> iat_window_;
    const size_t window_size_ = 1000;

    double baseline_variance_;
    bool baseline_established_;

    double current_safety_score_;
    const double omega_thresh_ = 2.5;
    const double kappa_ = 2.0; // Sensitivity constant
    const double s_crit_ = 0.40;

    bool go_dark_active_;
    size_t recovery_counter_;
    const size_t recovery_threshold_ = 20000;
};

} // namespace core
} // namespace wave_native
