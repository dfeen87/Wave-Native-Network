#include <mutex>
#include "mesh_discovery.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace wave_native {
namespace mesh_legacy {

    void InterfaceRegistry::register_interface(const InterfaceInfo& info) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        interfaces_[info.name] = info;
    }

    void InterfaceRegistry::unregister_interface(const std::string& name) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        interfaces_.erase(name);
    }

    std::vector<InterfaceInfo> InterfaceRegistry::get_wan_candidates() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<InterfaceInfo> candidates;
        for (const auto& [name, info] : interfaces_) {
            if (info.is_wan_candidate()) {
                candidates.push_back(info);
            }
        }
        return candidates;
    }

    std::optional<InterfaceInfo> InterfaceRegistry::get_interface(const std::string& name) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = interfaces_.find(name);
        if (it != interfaces_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::vector<InterfaceInfo> InterfaceRegistry::get_all_interfaces() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<InterfaceInfo> all_interfaces;
        all_interfaces.reserve(interfaces_.size());
        for (const auto& [name, info] : interfaces_) {
            all_interfaces.push_back(info);
        }
        return all_interfaces;
    }

    InterfaceDiscovery::InterfaceDiscovery(std::shared_ptr<InterfaceRegistry> registry, std::shared_ptr<PeerGatekeeper> gatekeeper)
        : registry_(std::move(registry)), gatekeeper_(std::move(gatekeeper)) {}

    void InterfaceDiscovery::start_monitoring() {
        // In a full implementation, this would spawn a thread to periodically call discover_interfaces()
        discover_interfaces();
    }

    bool InterfaceDiscovery::discover_interfaces() {
        // Simplified implementation using mock data
        return use_mock_interfaces();
    }

    bool InterfaceDiscovery::discover_linux_interfaces() {
        // Simplified placeholder for actual /sys/class/net parsing
        return false;
    }

    bool InterfaceDiscovery::use_mock_interfaces() {
        if (!registry_ || !gatekeeper_) return false;

        std::vector<uint8_t> mock_phase_signature = {0x01, 0x02, 0x03};

        // Compute expected state for omega=1.0 to generate a passing mock_proof
        double dt = 0.01;
        double t_end = 1.0;
        double x = 0.0, v = 0.0;
        double delta = 0.1, alpha = -1.0, beta = 1.0, F = 0.3, omega = 1.0;
        for (double t = 0; t < t_end; t += dt) {
            double a = F * std::cos(omega * t) - delta * v - alpha * x - beta * x * x * x;
            x += v * dt;
            v += a * dt;
        }

        // Good proof: amplitude=x, phase_angle=0, frequency=1.0, velocity=v
        ZKProof mock_proof(x, 0.0, 1.0, v);
        AileeTrustScore good_score = {0.8, 0.9, 0.8, 0.9};

        if (gatekeeper_->process_incoming_peer(mock_phase_signature, mock_proof, good_score, 10.0)) {
            InterfaceInfo eth0;
            eth0.name = "eth0";
            eth0.iface_type = InterfaceType::Ethernet;
            eth0.is_up = true;
            eth0.has_carrier = true;

            registry_->register_interface(eth0);
        }

        // Bad proof that fails threshold
        ZKProof bad_proof(100.0, 0.0, 1.0, 100.0);
        AileeTrustScore bad_score = {0.2, 0.1, 0.3, 0.2};
        if (gatekeeper_->process_incoming_peer(mock_phase_signature, bad_proof, bad_score, 0.5)) {
            InterfaceInfo wlan0;
            wlan0.name = "wlan0";
            wlan0.iface_type = InterfaceType::WiFi;
            wlan0.is_up = true;
            wlan0.has_carrier = false;

            registry_->register_interface(wlan0);
        }

        return true;
    }

    void ResonantPeerTable::insert_or_update(const ResonantPeer& peer) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        peers_[peer.signature] = peer;
    }

    std::optional<ResonantPeer> ResonantPeerTable::get_peer(const std::vector<uint8_t>& signature) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = peers_.find(signature);
        if (it != peers_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void ResonantPeerTable::prune_decayed_peers(double current_time) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        for (auto it = peers_.begin(); it != peers_.end(); ) {
            if (it->second.psi_snr < 0.1 || (current_time - it->second.t_s) > 5.0) {
                it = peers_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void AdaptiveSpectralMonitor::analyze_stream(const std::vector<double>& stream) {
        if (stream.size() < 3) {
            latest_snr_.store(0.0);
            std::lock_guard<std::mutex> lock(cand_mutex_);
            candidate_frequency_ = std::nullopt;
            return;
        }

        // Apply Hann Window to reduce sidelobe spectral leakage
        std::vector<double> windowed_stream(stream.size());
        size_t N = stream.size();
        for (size_t i = 0; i < N; ++i) {
            double hann_multiplier = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * i / (N - 1)));
            windowed_stream[i] = stream[i] * hann_multiplier;
        }

        // Calculate Signal Power (Variance) and Mean from windowed stream
        double mean = 0.0;
        for (double val : windowed_stream) {
            mean += val;
        }
        mean /= windowed_stream.size();

        double variance = 0.0;
        for (double val : windowed_stream) {
            variance += (val - mean) * (val - mean);
        }
        variance /= windowed_stream.size();

        // Estimate noise via local differences (high-frequency components)
        double noise_variance = 0.0;
        for (size_t i = 1; i < windowed_stream.size(); ++i) {
            double diff = windowed_stream[i] - windowed_stream[i - 1];
            noise_variance += diff * diff;
        }
        noise_variance /= (2.0 * (windowed_stream.size() - 1));

        // Prevent division by zero
        if (noise_variance < 1e-9) {
            noise_variance = 1e-9;
        }

        latest_snr_.store(variance / noise_variance);

        // Discard stochastic Gaussian noise or rigid framing
        // If variance is extremely low, it's flatline.
        // If SNR is ~1.0, it's purely stochastic.
        if (latest_snr_.load() < 2.0 || variance < 1e-4) {
            std::lock_guard<std::mutex> lock(cand_mutex_);
            candidate_frequency_ = std::nullopt;
            return;
        }

        // Resonance Filtering: ẍ + 0.3ẋ - 1.0x + 1.0x³ = 0.5 cos(ωt)
        // We'll estimate x, v (ẋ), a (ẍ) using finite differences.
        // Assuming unit time step dt for the sampled sequence, though in reality dt is given by PHY rate.
        // For detection, we just check the structural error against the nonlinear constraint.
        double dt = 0.01;
        double error_sum = 0.0;
        int valid_points = 0;

        for (size_t i = 1; i < windowed_stream.size() - 1; ++i) {
            double x = windowed_stream[i];
            double v = (windowed_stream[i + 1] - windowed_stream[i - 1]) / (2.0 * dt);
            double a = (windowed_stream[i + 1] - 2.0 * windowed_stream[i] + windowed_stream[i - 1]) / (dt * dt);

            // Left side of equation: ẍ + 0.3ẋ - 1.0x + 1.0x³
            double lhs = a + 0.3 * v - 1.0 * x + 1.0 * x * x * x;

            // We expect lhs to match 0.5 cos(ωt).
            // Thus lhs must be bounded approximately within [-0.5, 0.5].
            if (std::abs(lhs) <= 0.6) {
                // Heuristic for periodicity / driving match
                error_sum += std::abs(lhs);
                valid_points++;
            }
        }

        if (valid_points > windowed_stream.size() / 2) {
            // Found non-linear fold signature. Estimate ω.
            // Simplified estimation: track zero crossings for frequency.
            int crossings = 0;
            for (size_t i = 1; i < windowed_stream.size(); ++i) {
                if ((windowed_stream[i-1] < mean && windowed_stream[i] >= mean) ||
                    (windowed_stream[i-1] > mean && windowed_stream[i] <= mean)) {
                    crossings++;
                }
            }
            double estimated_omega = (crossings * std::numbers::pi) / (windowed_stream.size() * dt);
            std::lock_guard<std::mutex> lock(cand_mutex_);
            candidate_frequency_ = estimated_omega;
        } else {
            std::lock_guard<std::mutex> lock(cand_mutex_);
            candidate_frequency_ = std::nullopt;
        }
    }

    PeerGatekeeper::PeerGatekeeper(std::shared_ptr<ZKVerifier> verifier)
        : verifier_(std::move(verifier)) {}

    bool PeerGatekeeper::process_incoming_peer(const std::vector<uint8_t>& phase_signature,
                                               const ZKProof& proof,
                                               AileeTrustScore& score,
                                               double psi_snr) {
        if (phase_signature.empty()) {
            return false;
        }

        double drift = 0.0;
        if (!verifier_ || !verifier_->verify_proof(proof, drift)) {
            return false;
        }

        double epsilon = verifier_->get_epsilon();

        if (drift > epsilon) {
            drift_history_.erase(phase_signature);
            return false;
        }

        // Determinism
        score.determinism_score = 1.0 - (drift / epsilon);

        // Dynamic Trial Validation Window based on SNR
        double w_val = std::clamp(5000.0 * (1.0 / std::max(psi_snr, 0.001)), 1000.0, 20000.0);
        size_t required_history_points = std::max(3.0, w_val / 1000.0);

        // Consistency
        auto& history = drift_history_[phase_signature];
        history.push_back(drift);
        while (history.size() > required_history_points) {
            history.erase(history.begin());
        }

        if (history.size() >= 2) {
            double mean = 0.0;
            for (double d : history) mean += d;
            mean /= history.size();

            double variance = 0.0;
            for (double d : history) variance += (d - mean) * (d - mean);
            variance /= (history.size() - 1);

            double stddev = std::sqrt(variance);
            double raw_consistency = std::max(0.0, 1.0 - stddev);

            // EMA for consistency smoothing
            if (ema_consistency_.find(phase_signature) == ema_consistency_.end()) {
                ema_consistency_[phase_signature] = raw_consistency;
            } else {
                ema_consistency_[phase_signature] = 0.2 * raw_consistency + 0.8 * ema_consistency_[phase_signature];
            }
            score.consistency_score = ema_consistency_[phase_signature];
        } else {
            score.consistency_score = 1.0;
        }

        // Confidence
        double driving_omega = 1.0;
        if (std::abs(proof.frequency - driving_omega) < 0.05) {
            score.confidence_score = 1.0;
        } else {
            score.confidence_score = 0.0;
        }

        // Return false until observation duration is met
        if (history.size() < required_history_points) {
            return false; // Still in trial
        }

        double agg_score = score.aggregate_score();
        bool is_trusted = trust_state_[phase_signature];

        // Schmitt Trigger Logic for hysteresis
        if (is_trusted) {
            if (agg_score < 0.65) {
                drift_history_.erase(phase_signature);
                trust_state_.erase(phase_signature);
                ema_consistency_.erase(phase_signature);
                return false; // Drop-out
            }
        } else {
            if (agg_score >= 0.75) {
                trust_state_[phase_signature] = true;
            } else {
                return false; // Not locked in yet
            }
        }

        return true;
    }

} // namespace mesh_legacy
} // namespace wave_native
