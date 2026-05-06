#pragma once

#include <vector>

namespace wave_native {
namespace core {

class PllController {
public:
    PllController(double omega_base);

    // Update the PLL loop with a new batch of amplitude samples from the PHY listener
    // and the current local phase (theta_local)
    // Returns the corrected frequency (omega_corrected)
    double step(const std::vector<double>& stream, double theta_local, double dt);

    bool is_locked() const;
    double get_phase_error() const;

    // Trigger a manual reset (e.g., quarantine)
    void reset();

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
};

} // namespace core
} // namespace wave_native
