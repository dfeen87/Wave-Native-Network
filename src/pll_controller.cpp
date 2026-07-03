#include "pll_controller.hpp"
#include "wave_state.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace wave_native {
namespace core {

PllController::PllController(double omega_base, double lock_time_ms, double overshoot_deg, double settling_time_ms)
    : omega_base_(omega_base),
      integral_accumulator_(0.0),
      is_locked_(false),
      lock_counter_(0),
      last_peak_time_(0.0),
      current_time_(0.0),
      delta_theta_(0.0),
      last_amp_(0.0),
      was_increasing_(true),
      lock_time_ms_(lock_time_ms),
      overshoot_deg_(overshoot_deg),
      settling_time_ms_(settling_time_ms) {
}

double PllController::step(const std::vector<double>& stream, double theta_local, long double dt, long double ts) {
    double expected_salt = wave_native::core::generate_phase_salt(ts, omega_base_);
    double omega_active = omega_base_ + expected_salt;

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
                if (is_locked_ && last_peak_time_ > 0.0) {
                    double time_diff = current_time_ - last_peak_time_;
                    if (time_diff > 0.0) {
                        double incoming_omega = (2.0 * M_PI) / time_diff;
                        if (std::abs(incoming_omega - omega_active) > 0.0001) {
                            std::cout << "[SECURITY] Perfect resonance detected, but Phase-Salt invalid. Dropping Ghost Lock.\n";
                            is_replay_detected_ = true;
                            break;
                        }
                    }
                }
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
        theta_incoming = std::fmod(omega_active * time_since_peak, 2.0 * M_PI);
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
        if (!integral_frozen_) {
            integral_accumulator_ += delta_theta_ * dt;
            integral_accumulator_ = std::max(-i_limit_, std::min(i_limit_, integral_accumulator_));
        }

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

    // Physical modeling of overshoot and settling time
    // If not locked, or just locked, apply a simulated overshoot perturbation
    double physical_perturbation = 0.0;
    if (lock_counter_ > 0 && lock_counter_ < lock_threshold_ * 2) {
        // Simulating an underdamped settling characteristic
        double t_ms = (lock_counter_ * dt * 1000.0);
        if (t_ms < settling_time_ms_) {
            double damping_factor = std::exp(-t_ms / (settling_time_ms_ / 3.0));
            double oscillation = std::sin(2.0 * M_PI * t_ms / lock_time_ms_);
            double overshoot_rad = overshoot_deg_ * (M_PI / 180.0);
            physical_perturbation = overshoot_rad * damping_factor * oscillation;
        }
    }

    // PI control output
    double omega_corrected = omega_active + (K_p_ * delta_theta_) + (K_i_ * integral_accumulator_) + physical_perturbation;

    if (diagnostics_enabled_) {
        std::cout << "[PLL Diagnostics] dt=" << dt << " err=" << delta_theta_ << " corr=" << omega_corrected << " locked=" << is_locked_ << "\n";
    }

    return omega_corrected;
}

bool PllController::is_locked() const {
    return is_locked_;
}

double PllController::get_phase_error() const {
    return delta_theta_;
}

double PllController::compute_phase_error() const {
    return delta_theta_;
}

void PllController::reset() {
    integral_accumulator_ = 0.0;
    is_locked_ = false;
    lock_counter_ = 0;
}

} // namespace core
} // namespace wave_native
