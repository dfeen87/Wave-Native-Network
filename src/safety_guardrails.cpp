#include "safety_guardrails.hpp"
#include <numeric>
#include <iomanip>

namespace wave_native {
namespace core {

SafetyGuardrails::SafetyGuardrails(RoutingEngine* routing_engine, PllController* pll, DuffingOscillator* duffing, SafetyMode mode)
    : routing_engine_(routing_engine), pll_(pll), duffing_(duffing),
      baseline_variance_(0.0), baseline_established_(false),
      current_safety_score_(1.0), go_dark_active_(false), recovery_counter_(0), safety_mode_(mode) {
    if (safety_mode_ == SafetyMode::STRICT) {
        s_crit_ = 0.40;
        omega_thresh_ = 2.5;
    } else if (safety_mode_ == SafetyMode::RELAXED) {
        s_crit_ = 0.15;
        omega_thresh_ = 4.5;
    } else if (safety_mode_ == SafetyMode::OFF) {
        s_crit_ = 0.0;
        omega_thresh_ = 2.5;
    }
}

void SafetyGuardrails::process_iat_samples(const std::vector<double>& iats) {
    if (iats.empty()) return;

    for (double iat : iats) {
        iat_window_.push_back(iat);
        if (iat_window_.size() > window_size_) {
            iat_window_.pop_front();
        }

        if (go_dark_active_.load(std::memory_order_acquire)) {
            recovery_counter_++;
        }
    }

    if (iat_window_.size() < window_size_) {
        return; // Wait for enough samples
    }

    // Calculate variance
    double sum = std::accumulate(iat_window_.begin(), iat_window_.end(), 0.0);
    double mean = sum / iat_window_.size();

    double sq_diff_sum = 0.0;
    for (double val : iat_window_) {
        double diff = val - mean;
        sq_diff_sum += diff * diff;
    }
    double current_variance = sq_diff_sum / iat_window_.size();

    if (current_variance <= 0) return;

    // Establish baseline if not done
    if (!baseline_established_) {
        baseline_variance_ = current_variance;
        baseline_established_ = true;
        std::cout << "[SAFETY] Baseline IAT Variance established: " << baseline_variance_ << "\n";
        return;
    }

    // Continuously drift baseline slowly if we are safe
    if (!go_dark_active_.load(std::memory_order_acquire)) {
        baseline_variance_ = 0.999 * baseline_variance_ + 0.001 * current_variance;
    }

    // 1. Opacity Detection
    double omega = current_variance / baseline_variance_;

    // 2. AILEE Dimension Integration
    double s = 1.0 - std::tanh(kappa_ * (omega - omega_thresh_));
    current_safety_score_ = std::max(0.0, std::min(1.0, s));

    // 3. The "Go-Dark" Protocol
    if (safety_mode_ != SafetyMode::OFF && !go_dark_active_.load(std::memory_order_acquire) && current_safety_score_ < s_crit_) {
        // Initiate hardware lockdown
        go_dark_active_.store(true, std::memory_order_release);
        
        // Strict fence to ensure flush guarantees propagate to all cores
        std::atomic_thread_fence(std::memory_order_seq_cst);

        std::cout << "\n[SAFETY] Carrier Opacity Detected (Ω=" << std::fixed << std::setprecision(2) << omega << "). Entering Deep Stealth.\n";

        routing_engine_->set_transduction_allowed(false);
        routing_engine_->flush_transduction_queue();
        recovery_counter_ = 0;

        if (!routing_engine_->is_vector_a_viable()) {
            std::cout << "[SAFETY] Vector A non-viable. Triggering full dormancy.\n";
            pll_->freeze_integral();
            duffing_->set_ambient_mode(true);
        }
    }

    // 4. Autonomous Re-Emergence
    if (go_dark_active_.load(std::memory_order_acquire)) {
        if (recovery_counter_ >= recovery_threshold_) {
            if (current_safety_score_ > 0.85) {
                std::cout << "\n[SAFETY] Carrier translucency restored (S=" << std::fixed << std::setprecision(2) << current_safety_score_ << "). Re-emerging from stealth.\n";
                go_dark_active_.store(false, std::memory_order_release);
                std::atomic_thread_fence(std::memory_order_seq_cst);
                
                routing_engine_->set_transduction_allowed(true);
                pll_->unfreeze_integral();
                duffing_->set_ambient_mode(false);
            } else {
                // Reset counter, keep observing
                recovery_counter_ = 0;
            }
        }
    }
}

} // namespace core
} // namespace wave_native
