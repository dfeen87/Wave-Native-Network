#include <mutex>
#include "mesh_discovery.hpp"
#include <iostream>
#include <algorithm>

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

        // Mock a peer attempting to connect
        std::vector<uint8_t> mock_phase_signature = {0x01, 0x02, 0x03};
        std::vector<uint8_t> mock_proof_data = {0xaa, 0xbb, 0xcc};
        std::vector<uint8_t> mock_public_inputs = {0x00};
        ZKProof mock_proof(mock_proof_data, mock_public_inputs, "mock_circuit");

        AileeTrustScore good_score = {0.8, 0.9, 0.8, 0.9}; // High enough to pass aggregate 0.70 threshold

        if (gatekeeper_->process_incoming_peer(mock_phase_signature, mock_proof, mock_public_inputs, good_score)) {
            // Only if it passes, do we register
            InterfaceInfo eth0;
            eth0.name = "eth0";
            eth0.iface_type = InterfaceType::Ethernet;
            eth0.is_up = true;
            eth0.has_carrier = true;

            registry_->register_interface(eth0);
        }

        AileeTrustScore bad_score = {0.2, 0.1, 0.3, 0.2}; // Fails aggregate threshold
        if (gatekeeper_->process_incoming_peer(mock_phase_signature, mock_proof, mock_public_inputs, bad_score)) {
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
                                               const std::vector<uint8_t>& public_inputs,
                                               const AileeTrustScore& score) {
        // Step 1: Immediately pause connection processing (implied by this synchronous, blocking validation step)
        if (phase_signature.empty()) {
            return false; // Ruthlessly drop
        }

        // Step 2 & 3: Strict Enforcement and AILEE integration
        // The incoming peer MUST supply a valid ZKProof and pass the AILEE strict threshold (0.70).
        if (!verifier_ || !verifier_->verify_ailee_trust(proof, public_inputs, score)) {
            // Ruthlessly and silently drop the connection. No fallback, no retry loop.
            return false;
        }

        // Step 4: Accept
        return true;
    }

} // namespace mesh_legacy
} // namespace wave_native
