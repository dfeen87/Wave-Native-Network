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
        record_incident(SafetyIncidentType::GoDarkActivated, "Carrier opacity detected, entered stealth");
        
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

void SafetyGuardrails::record_incident(SafetyIncidentType type, const std::string& description) {
    SafetyIncident incident;
    incident.timestamp = std::chrono::steady_clock::now();
    incident.type = type;
    incident.description = description;

    recent_incidents_.push_back(incident);
    if (recent_incidents_.size() > 100) {
        recent_incidents_.pop_front();
    }
}

SafetyReport SafetyGuardrails::generate_report() const {
    SafetyReport report;
    report.recent_incidents = std::vector<SafetyIncident>(recent_incidents_.begin(), recent_incidents_.end());
    report.trust_score_threshold = trust_score_threshold_;
    report.coherence_failure_threshold = coherence_failure_threshold_;
    report.go_dark_active = go_dark_active_.load(std::memory_order_acquire);
    return report;
}

void SafetyGuardrails::report_unstable_cluster(const wnn::space::CoherenceCluster& cluster) {
    if (!cluster.is_stable && cluster.average_trust_score < trust_score_threshold_) {
        std::string description = "Unstable Coherence Cluster detected. Average phase error: " +
                                  std::to_string(cluster.average_phase_deg) + " deg, average trust: " +
                                  std::to_string(cluster.average_trust_score) + " with " +
                                  std::to_string(cluster.anchors.size()) + " anchors.";
        record_incident(SafetyIncidentType::CoherenceClusterUnstable, description);
    }
}

void SafetyGuardrails::update_pll_network_state(bool locked, double timestamp_ms, double pll_unstable_incident_threshold_ms) {
    if (!locked && last_pll_locked_) {
        // Just became unlocked
        pll_unstable_start_ms_ = timestamp_ms;
    } else if (!locked && !last_pll_locked_) {
        // Sustained unlock, check threshold
        if ((timestamp_ms - pll_unstable_start_ms_) > pll_unstable_incident_threshold_ms) {
            std::string desc = "Distributed PLL network unstable for over " + std::to_string(pll_unstable_incident_threshold_ms) + " ms";
            record_incident(SafetyIncidentType::PllNetworkUnstable, desc);
            // Reset start time so we don't spam incidents every tick, or we could leave it
            // For now, reset it so it warns again after another threshold duration
            pll_unstable_start_ms_ = timestamp_ms;
        }
    } else if (locked) {
        // Locked
        pll_unstable_start_ms_ = 0.0;
    }
    last_pll_locked_ = locked;
}

void SafetyGuardrails::update_mesh_health(bool healthy, double timestamp_ms, double mesh_health_degraded_threshold_ms) {
    if (!healthy && last_mesh_healthy_) {
        // Just became unhealthy
        mesh_unhealthy_start_ms_ = timestamp_ms;
    } else if (!healthy && !last_mesh_healthy_) {
        // Sustained unhealthy state, check threshold
        if ((timestamp_ms - mesh_unhealthy_start_ms_) > mesh_health_degraded_threshold_ms) {
            std::string desc = "Mesh health degraded for over " + std::to_string(mesh_health_degraded_threshold_ms) + " ms (low trust, high phase error, or unstable coherence)";
            record_incident(SafetyIncidentType::MeshHealthDegraded, desc);
            // Reset to prevent spamming
            mesh_unhealthy_start_ms_ = timestamp_ms;
        }
    } else if (healthy) {
        // Healthy
        mesh_unhealthy_start_ms_ = 0.0;
    }
    last_mesh_healthy_ = healthy;
}

} // namespace core
} // namespace wave_native
