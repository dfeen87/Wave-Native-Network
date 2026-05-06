#pragma once

#include <vector>
#include <map>
#include <cstdint>
#include <cmath>
#include <mutex>
#include <chrono>
#include <thread>
#include <iostream>

#include "../mesh_legacy/zk_verifier.hpp"
#include "wave_state.hpp"
#include "interceptor/phy_listener.hpp"

namespace wave_native {
namespace core {

enum class TransportVector {
    VectorA_Resonant,
    VectorB_Transduction
};

struct KnownPeer {
    std::vector<uint8_t> signature;
    double spectral_frequency;
    mesh_legacy::AileeTrustScore trust_score;
    double path_fitness;
};

class RoutingEngine {
public:
    RoutingEngine(wave_native::interceptor::PhyListener* phy_listener)
        : phy_listener_(phy_listener), baseline_entropy_(0.0), entropy_samples_(0) {}

    // Add or update a peer in the routing table
    void add_or_update_peer(const std::vector<uint8_t>& signature,
                            double spectral_frequency,
                            const mesh_legacy::AileeTrustScore& score);

    // Main entry point for propagating the state to all known peers
    void propagate_state(const wave_native::core::WaveState& local_state, const std::vector<double>& current_stream);

    // Multi-hop routing: Refracts an incoming state intended for Vector B
    void refract_wavefront(const wave_native::core::WaveState& state_in,
                           const wave_native::core::WaveState& state_out,
                           const std::vector<double>& current_stream);

    // AILEE Guardrails
    void set_transduction_allowed(bool allowed) { transduction_allowed_ = allowed; }
    bool is_vector_a_viable() const;

private:
    // Decide between Vector A and Vector B
    TransportVector select_vector(double local_omega, const KnownPeer& peer) const;

    // Convert phase shift into an Inter-Arrival Time (IAT) delay
    // Returns true if transduction was successful, false if snap-back is triggered
    bool modulate_iat(double delta_theta, const std::vector<double>& current_stream);

    // Adaptive Mesh Map
    std::map<std::vector<uint8_t>, KnownPeer> mesh_map_;
    mutable std::mutex map_mutex_;

    wave_native::interceptor::PhyListener* phy_listener_;

    // Baseline entropy tracking for Translucency
    double baseline_entropy_;
    int entropy_samples_;
    const double K_iat = 1000000.0; // Nanoseconds per radian

    bool transduction_allowed_ = true;
};

} // namespace core
} // namespace wave_native
