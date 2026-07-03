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
#include "config/wnn_config.hpp"
#include "space_segment/PhaseCoherenceAnchor.hpp"

#include <chrono>

namespace wave_native {
namespace core {

enum class SafetyIncidentType {
    GoDarkActivated,
    TrustThresholdBreach,
    CoherenceFailure,
    CoherenceClusterUnstable,
    Unknown
};

struct SafetyIncident {
    std::chrono::steady_clock::time_point timestamp;
    SafetyIncidentType type;
    std::string description;
};

struct SafetyReport {
    std::vector<SafetyIncident> recent_incidents;
    double trust_score_threshold;
    double coherence_failure_threshold;
    bool go_dark_active;
};

class SafetyGuardrails {
public:
    SafetyGuardrails(RoutingEngine* routing_engine, PllController* pll, DuffingOscillator* duffing, SafetyMode mode = SafetyMode::STRICT);

    // Process a batch of newly observed Inter-Arrival Times (IAT)
    void process_iat_samples(const std::vector<double>& iats);

    bool is_go_dark_active() const { return go_dark_active_.load(std::memory_order_acquire); }
    double get_current_safety_score() const { return current_safety_score_; }

    SafetyReport generate_report() const;
    void record_incident(SafetyIncidentType type, const std::string& description);

    void report_unstable_cluster(const wnn::space::CoherenceCluster& cluster);

private:
    RoutingEngine* routing_engine_;
    PllController* pll_;
    DuffingOscillator* duffing_;

    std::deque<double> iat_window_;
    const size_t window_size_ = 1000;

    double baseline_variance_;
    bool baseline_established_;

    double current_safety_score_;
    double omega_thresh_ = 2.5;
    const double kappa_ = 2.0; // Sensitivity constant
    double s_crit_ = 0.40;
    SafetyMode safety_mode_;

    std::atomic<bool> go_dark_active_{false};
    size_t recovery_counter_;
    const size_t recovery_threshold_ = 20000;

    double trust_score_threshold_ = 0.5;
    double coherence_failure_threshold_ = 5.0;
    std::deque<SafetyIncident> recent_incidents_;
};

} // namespace core
} // namespace wave_native
