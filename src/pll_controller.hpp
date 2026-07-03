#pragma once

#include <vector>

namespace wave_native {
namespace core {

class PllController {
public:
    PllController(double omega_base, double lock_time_ms = 100.0, double overshoot_deg = 5.0, double settling_time_ms = 50.0);

    // Update the PLL loop with a new batch of amplitude samples from the PHY listener
    // and the current local phase (theta_local)
    // Returns the corrected frequency (omega_corrected)
    double step(const std::vector<double>& stream, double theta_local, long double dt, long double ts);

    bool is_locked() const;
    double get_phase_error() const;
    double compute_phase_error() const;

    bool is_replay_detected() const { return is_replay_detected_; }
    void clear_replay_flag() { is_replay_detected_ = false; }

    // Trigger a manual reset (e.g., quarantine)
    void reset();

    void enable_diagnostics(bool enabled) { diagnostics_enabled_ = enabled; }
    void set_lock_time_ms(double ms) { lock_time_ms_ = ms; }
    void set_overshoot_deg(double deg) { overshoot_deg_ = deg; }
    void set_settling_time_ms(double ms) { settling_time_ms_ = ms; }

    // AILEE Guardrails
    void freeze_integral() {
        integral_frozen_ = true;
        integral_accumulator_ = 0.0;
    }
    void unfreeze_integral() {
        integral_frozen_ = false;
    }

private:
    double omega_base_;
    double integral_accumulator_;

    // PI Control Gains
    const double K_p_ = 0.02;
    const double K_i_ = 0.005;

    // Anti-windup limit
    const double i_limit_ = 0.5;

    // Lock-in state
    bool is_locked_;
    int lock_counter_;
    const int lock_threshold_ = 2000;
    const double lock_epsilon_ = 0.01;

    // Break-lock state
    const double break_lock_threshold_ = 1.57079632679; // pi/2

    double last_peak_time_;
    double current_time_;
    double delta_theta_;

    // To detect peaks
    double last_amp_;
    bool was_increasing_;

    bool integral_frozen_ = false;
    bool is_replay_detected_ = false;

    // Stability modeling parameters
    double lock_time_ms_;
    double overshoot_deg_;
    double settling_time_ms_;
    bool diagnostics_enabled_ = false;
};

} // namespace core
} // namespace wave_native
