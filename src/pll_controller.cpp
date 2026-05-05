#include "pll_controller.hpp"
#include <cmath>
#include <algorithm>

namespace wave_native {
namespace core {

PllController::PllController(double omega_base)
    : omega_base_(omega_base),
      integral_accumulator_(0.0),
      is_locked_(false),
      lock_counter_(0),
      last_peak_time_(0.0),
      current_time_(0.0),
      delta_theta_(0.0),
      last_amp_(0.0),
      was_increasing_(true) {
}

double PllController::step(const std::vector<double>& stream, double theta_local, double dt) {
    // Process stream to find the latest peak to estimate incoming phase
    if (stream.empty()) {
        current_time_ += dt;
    } else {
        double sample_dt = dt / stream.size();
        for (double amp : stream) {
            current_time_ += sample_dt;

            bool is_increasing = (amp > last_amp_);

            // Simple peak detection: was increasing, now decreasing
            if (was_increasing_ && !is_increasing) {
                last_peak_time_ = current_time_;
            }

            was_increasing_ = is_increasing;
            last_amp_ = amp;
        }
    }

    // If we haven't found a peak yet, assume incoming phase is 0
    double theta_incoming = 0.0;
    if (last_peak_time_ > 0.0) {
        // Estimate phase based on time elapsed since the last peak
        double time_since_peak = current_time_ - last_peak_time_;
        theta_incoming = std::fmod(omega_base_ * time_since_peak, 2.0 * M_PI);
    }

    // Calculate phase difference
    delta_theta_ = theta_incoming - theta_local;

    // Wrap to [-pi, pi]
    while (delta_theta_ > M_PI) delta_theta_ -= 2.0 * M_PI;
    while (delta_theta_ < -M_PI) delta_theta_ += 2.0 * M_PI;

    // Break-lock logic
    if (std::abs(delta_theta_) > break_lock_threshold_) {
        reset();
    } else {
        // Update integral accumulator with anti-windup
        integral_accumulator_ += delta_theta_ * dt;
        integral_accumulator_ = std::max(-i_limit_, std::min(i_limit_, integral_accumulator_));

        // Lock-in logic
        if (std::abs(delta_theta_) < lock_epsilon_) {
            lock_counter_++;
            if (lock_counter_ >= lock_threshold_) {
                is_locked_ = true;
            }
        } else {
            lock_counter_ = 0;
            is_locked_ = false;
        }
    }

    // PI control output
    double omega_corrected = omega_base_ + (K_p_ * delta_theta_) + (K_i_ * integral_accumulator_);

    return omega_corrected;
}

bool PllController::is_locked() const {
    return is_locked_;
}

double PllController::get_phase_error() const {
    return delta_theta_;
}

void PllController::reset() {
    integral_accumulator_ = 0.0;
    is_locked_ = false;
    lock_counter_ = 0;
}

} // namespace core
} // namespace wave_native
