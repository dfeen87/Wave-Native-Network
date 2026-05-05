#include <mutex>
#include "mesh_discovery.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

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

        if (gatekeeper_->process_incoming_peer(mock_phase_signature, mock_proof, good_score)) {
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
        if (gatekeeper_->process_incoming_peer(mock_phase_signature, bad_proof, bad_score)) {
            InterfaceInfo wlan0;
            wlan0.name = "wlan0";
            wlan0.iface_type = InterfaceType::WiFi;
            wlan0.is_up = true;
            wlan0.has_carrier = false;

            registry_->register_interface(wlan0);
        }

        return true;
    }

    PeerGatekeeper::PeerGatekeeper(std::shared_ptr<ZKVerifier> verifier)
        : verifier_(std::move(verifier)) {}

    bool PeerGatekeeper::process_incoming_peer(const std::vector<uint8_t>& phase_signature,
                                               const ZKProof& proof,
                                               AileeTrustScore& score) {
        if (phase_signature.empty()) {
            return false;
        }

        double drift = 0.0;
        if (!verifier_ || !verifier_->verify_proof(proof, drift)) {
            return false;
        }

        double epsilon = verifier_->get_epsilon();

        // Determinism
        score.determinism_score = 1.0 - (drift / epsilon);

        // Consistency
        auto& history = drift_history_[phase_signature];
        history.push_back(drift);
        if (history.size() > 3) {
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
            score.consistency_score = std::max(0.0, 1.0 - stddev);
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

        if (score.aggregate_score() < 0.70) {
            return false; // Phase Decoloration
        }

        return true;
    }

} // namespace mesh_legacy
} // namespace wave_native
