#pragma once

#include <vector>
#include <map>
#include <cstdint>
#include <cmath>
#include <mutex>
#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>
#include <optional>

#include "../mesh_legacy/zk_verifier.hpp"
#include "wave_state.hpp"
#include "interceptor/phy_listener.hpp"
#include "space_segment/PhaseCoherenceAnchor.hpp"

namespace wave_native {
namespace core {

enum class TransportVector {
    VectorA_Resonant,
    VectorB_Transduction
};

enum class RoutingMode {
    LOW_LATENCY,
    HIGH_STABILITY,
    BALANCED
};

struct RouteDecision {
    TransportVector selected_vector;
    std::optional<std::vector<uint8_t>> destination_peer;
    RoutingMode mode_used;
    double estimated_latency_ms;
    double estimated_stability_score;
    bool is_valid;
};

struct HybridWeights {
    double w_L = 0.15;
    double w_J = 0.15;
    double w_phi = 0.30;
    double w_omega = 0.20;
    double w_DPhi = 0.20;
};

struct KnownPeer {
    std::vector<uint8_t> signature;
    double spectral_frequency;
    mesh_legacy::AileeTrustScore trust_score;
    std::atomic<double> latency_ema{0.0};
    std::atomic<double> jitter_ema{0.0};
    long double phase_theta = 0.0L;

    KnownPeer() = default;

    KnownPeer(const KnownPeer& other)
        : signature(other.signature),
          spectral_frequency(other.spectral_frequency),
          trust_score(other.trust_score),
          latency_ema(other.latency_ema.load()),
          jitter_ema(other.jitter_ema.load()),
          phase_theta(other.phase_theta) {}

    KnownPeer& operator=(const KnownPeer& other) {
        if (this != &other) {
            signature = other.signature;
            spectral_frequency = other.spectral_frequency;
            trust_score = other.trust_score;
            latency_ema.store(other.latency_ema.load());
            jitter_ema.store(other.jitter_ema.load());
            phase_theta = other.phase_theta;
        }
        return *this;
    }
};

class RoutingEngine {
public:
    RoutingEngine(wave_native::interceptor::PhyListener* phy_listener)
        : phy_listener_(phy_listener), baseline_entropy_(0.0), entropy_samples_(0) {}

    // Add or update a peer in the routing table
    void add_or_update_peer(const std::vector<uint8_t>& signature,
                            double spectral_frequency,
                            const mesh_legacy::AileeTrustScore& score,
                            long double phase_theta,
                            const std::vector<double>& iat_stream);

    // Main entry point for propagating the state to all known peers
    void propagate_state(const wave_native::core::WaveState& local_state, const std::vector<double>& current_stream);

    // Propagate state explicitly using a computed RouteDecision
    void propagate_route(const wave_native::core::WaveState& local_state, const std::vector<double>& current_stream, const RouteDecision& decision);

    // Multi-hop routing: Refracts an incoming state intended for Vector B
    void refract_wavefront(const wave_native::core::WaveState& state_in,
                           const wave_native::core::WaveState& state_out,
                           const std::vector<double>& current_stream);

    /**
     * @brief Refracts an incoming wavefront to an outgoing state.
     * @param state_in The incoming wave state.
     * @param state_out The outgoing wave state to be updated.
     */
    void refract_wavefront(const wave_native::core::WaveState& state_in,
                           wave_native::core::WaveState& state_out);

    /**
     * @brief Computes a route decision based on a wave state and routing mode.
     * @param state The target wave state.
     * @param mode The desired routing mode.
     * @return A route decision detailing the path.
     */
    RouteDecision compute_route(const wave_native::core::WaveState& state, RoutingMode mode);

    /**
     * @brief Sets the internal mesh density.
     * @param density The mesh density to set.
     */
    void set_mesh_density(double density) { mesh_density_ = density; }

    /**
     * @brief Sets the trust scores for Phase Coherence Anchors.
     * @param scores The trust scores to set.
     */
    void set_anchor_trust_scores(const std::vector<double>& scores) { anchor_trust_scores_ = scores; }

    /**
     * @brief Sets the optional coherence clusters.
     * @param clusters The coherence clusters from the CoherenceEngine.
     */
    void set_coherence_clusters(const std::vector<wnn::space::CoherenceCluster>& clusters);

    // AILEE Guardrails
    void set_transduction_allowed(bool allowed) { transduction_allowed_ = allowed; }
    void flush_transduction_queue() {
        if (phy_listener_) phy_listener_->flush_delay_queue();
    }
    bool is_vector_a_viable() const;

private:
    double calculate_hybrid_distance(const KnownPeer& peer, const wave_native::core::WaveState& local_state) const;

    // Decide between Vector A and Vector B
    TransportVector select_vector(double local_omega, const KnownPeer& peer) const;

    // Convert phase shift into an Inter-Arrival Time (IAT) delay
    // Returns true if transduction was successful, false if snap-back is triggered
    bool modulate_iat(double delta_theta, const std::vector<double>& current_stream);

    // Adaptive Mesh Map
    std::map<std::vector<uint8_t>, KnownPeer> mesh_map_;
    mutable std::mutex map_mutex_;

    HybridWeights hybrid_weights_;
    wave_native::core::WaveState last_state_;

    wave_native::interceptor::PhyListener* phy_listener_;

    // Baseline entropy tracking for Translucency
    double baseline_entropy_;
    int entropy_samples_;
    const double K_iat = 1000000.0; // Nanoseconds per radian

    bool transduction_allowed_ = true;
    double mesh_density_ = 0.0;
    std::vector<double> anchor_trust_scores_;
    std::vector<wnn::space::CoherenceCluster> coherence_clusters_;
};

} // namespace core
} // namespace wave_native
