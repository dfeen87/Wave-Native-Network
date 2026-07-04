#include "routing_engine.hpp"

namespace wave_native {
namespace core {

void RoutingEngine::add_or_update_peer(const std::vector<uint8_t>& signature,
                                       double spectral_frequency,
                                       const mesh_legacy::AileeTrustScore& score,
                                       long double phase_theta,
                                       const std::vector<double>& iat_stream) {
    std::lock_guard<std::mutex> lock(map_mutex_);

    double prev_latency_ema = 0.0;
    double prev_jitter_ema = 0.0;

    auto it = mesh_map_.find(signature);
    if (it != mesh_map_.end()) {
        prev_latency_ema = it->second.latency_ema.load();
        prev_jitter_ema = it->second.jitter_ema.load();
    }

    double new_latency_ema = prev_latency_ema;
    double new_jitter_ema = prev_jitter_ema;

    for (double dt : iat_stream) {
        new_latency_ema = 0.8 * new_latency_ema + 0.2 * dt;
        new_jitter_ema = 0.8 * new_jitter_ema + 0.2 * std::abs(dt - new_latency_ema);
    }

    KnownPeer new_peer;
    new_peer.signature = signature;
    new_peer.spectral_frequency = spectral_frequency;
    new_peer.trust_score = score;
    new_peer.phase_theta = phase_theta;
    new_peer.latency_ema.store(new_latency_ema);
    new_peer.jitter_ema.store(new_jitter_ema);

    mesh_map_[signature] = new_peer;
}

double RoutingEngine::calculate_hybrid_distance(const KnownPeer& peer, const wave_native::core::WaveState& local_state) const {
    double L_norm = std::clamp(peer.latency_ema.load() / 100000000.0, 0.0, 1.0);
    double J_norm = std::clamp(peer.jitter_ema.load() / 10000000.0, 0.0, 1.0);
    long double delta_phi = std::abs(peer.phase_theta - local_state.theta);
    long double delta_omega = std::abs(peer.spectral_frequency - local_state.omega);
    double divergence = 1.0 - peer.trust_score.consistency_score;
    return hybrid_weights_.w_L * L_norm + hybrid_weights_.w_J * J_norm +
           hybrid_weights_.w_phi * static_cast<double>(delta_phi) +
           hybrid_weights_.w_omega * static_cast<double>(delta_omega) +
           hybrid_weights_.w_DPhi * divergence;
}

TransportVector RoutingEngine::select_vector(double local_omega, const KnownPeer& peer) const {
    double spectral_distance = std::abs(peer.spectral_frequency - local_omega);

    // If consistency score is low or spectral distance is too high,
    // it's safer/more effective to use Transduction (carrier mapping)
    if (peer.trust_score.consistency_score < 0.5 || spectral_distance > 0.5) {
        return TransportVector::VectorB_Transduction;
    }

    return TransportVector::VectorA_Resonant;
}

bool RoutingEngine::is_vector_a_viable() const {
    std::lock_guard<std::mutex> lock(map_mutex_);
    for (const auto& [signature, peer] : mesh_map_) {
        if (calculate_hybrid_distance(peer, last_state_) < 1.0) return true;
    }
    return false;
}

bool RoutingEngine::modulate_iat(double delta_theta, const std::vector<double>& current_stream) {
    if (!transduction_allowed_) {
        return false;
    }

    // Encode phase shift into Inter-Arrival Time (IAT)
    double delay_ns = K_iat * std::abs(delta_theta);

    // Calculate stream entropy via variance of physical layer amplitudes
    double stream_variance = 0.0;
    if (current_stream.size() > 1) {
        double sum = 0.0;
        for (double val : current_stream) sum += val;
        double mean = sum / current_stream.size();

        double sq_diff_sum = 0.0;
        for (double val : current_stream) {
            double diff = val - mean;
            sq_diff_sum += diff * diff;
        }
        stream_variance = sq_diff_sum / current_stream.size();
    }

    // Use variance scaled as a stand-in for baseline entropy in jitter
    double baseline_jitter = stream_variance * K_iat;

    // Snap-Back Heuristic: If the requested delay drastically exceeds the
    // baseline jitter derived from the physical stream, it destroys translucency.
    if (baseline_jitter > 0 && delay_ns > baseline_jitter * 5.0) {
        std::cout << "[RoutingEngine] Transduction delay exceeds baseline entropy! Snapping back to Vector A.\n";
        return false; // Snap-back to Vector A
    }

    // Queue the modulated packet injection via libpcap
    if (phy_listener_) {
        phy_listener_->inject_modulated_packet(delay_ns);
    }

    return true; // Transduction successful
}

void RoutingEngine::refract_wavefront(const wave_native::core::WaveState& state_in,
                                      const wave_native::core::WaveState& state_out,
                                      const std::vector<double>& current_stream) {
    // Zero-Copy Propagation via Phase Splice
    // The physical state shift (Ψ_in → Ψ_out) is extracted
    double delta_theta = state_out.theta - state_in.theta;

    // Attempt to transduce the state shift into the carrier
    if (!transduction_allowed_ || !modulate_iat(delta_theta, current_stream)) {
        // Fallback to Resonant if Transduction fails or is disabled
        std::cout << "[RoutingEngine] Refraction failed or disabled, fallback to physical transmission (Vector A).\n";
        // Simulate falling back to Vector A (e.g. queueing for physical broadcast)
    }
}

void RoutingEngine::refract_wavefront(const wave_native::core::WaveState& state_in,
                                      wave_native::core::WaveState& state_out) {
    // Apply a simple refraction logic modifying the outgoing state
    // based on incoming phase.
    double phase_shift = state_in.theta * 0.1;
    state_out.theta += phase_shift;
    state_out.omega += (state_in.omega - state_out.omega) * 0.05;
}

void RoutingEngine::set_coherence_clusters(const std::vector<wnn::space::CoherenceCluster>& clusters) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    coherence_clusters_ = clusters;
}

void RoutingEngine::set_pll_node_states(const std::vector<wave_native::core::PllNodeState>& states) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    pll_states_ = states;
}

void RoutingEngine::set_network_pll_locked(bool locked) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    network_pll_locked_ = locked;
}

void RoutingEngine::set_mesh_orchestrator_state(const MeshOrchestrator& orchestrator) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    mesh_healthy_ = orchestrator.is_mesh_healthy();
}

RouteDecision RoutingEngine::compute_route(const wave_native::core::WaveState& state, RoutingMode mode) {
    std::lock_guard<std::mutex> lock(map_mutex_);

    RouteDecision decision;
    decision.mode_used = mode;
    decision.is_valid = false;
    decision.estimated_latency_ms = 9999.0;
    decision.estimated_stability_score = 0.0;

    if (mesh_map_.empty()) {
        return decision;
    }

    double D = std::clamp(mesh_density_, 0.0, 1.0);
    double density_factor = 0.5 + 0.5 * D;

    double avg_anchor_trust = 1.0;
    if (!anchor_trust_scores_.empty()) {
        double sum = 0.0;
        for (double t : anchor_trust_scores_) sum += t;
        avg_anchor_trust = sum / anchor_trust_scores_.size();
    }

    // Incorporate cluster awareness
    double cluster_trust_boost = 1.0;
    if (!coherence_clusters_.empty()) {
        // Average trust of stable clusters, penalize if unstable clusters exist
        double stable_trust_sum = 0.0;
        std::size_t stable_count = 0;
        bool has_unstable = false;

        for (const auto& cluster : coherence_clusters_) {
            if (cluster.is_stable) {
                stable_trust_sum += cluster.average_trust_score;
                stable_count++;
            } else {
                has_unstable = true;
            }
        }

        if (stable_count > 0) {
            cluster_trust_boost = stable_trust_sum / stable_count;
        }

        if (has_unstable) {
            cluster_trust_boost *= 0.8; // Penalize routes if there's instability in the coherence anchor mesh
        }
    }

    // Heuristic: Use global PLL state to penalize stability
    if (!network_pll_locked_) {
        cluster_trust_boost *= 0.8; // Penalize routing score when the distributed PLL is not locked
        // Optionally bias mode: if we were BALANCED or LOW_LATENCY, we might want to prioritize stability
        if (mode != RoutingMode::HIGH_STABILITY) {
            mode = RoutingMode::HIGH_STABILITY;
        }
    }

    // Incorporate Mesh Orchestrator health
    if (!mesh_healthy_) {
        cluster_trust_boost *= 0.8; // Additional penalization for overall mesh degradation
        if (mode != RoutingMode::HIGH_STABILITY) {
            mode = RoutingMode::HIGH_STABILITY;
        }
    }

    struct CandidateScore {
        double score;
        const KnownPeer* peer;
        TransportVector vector;
        double latency_ms;
        double trust;
    };

    std::vector<CandidateScore> candidates;

    for (const auto& [signature, peer] : mesh_map_) {
        // For each peer, we consider both VectorA and VectorB as potential candidates
        for (TransportVector vec : {TransportVector::VectorA_Resonant, TransportVector::VectorB_Transduction}) {
            if (vec == TransportVector::VectorB_Transduction && !transduction_allowed_) {
                continue; // Skip Transduction if not allowed globally
            }

            double latency_ms = peer.latency_ema.load() / 1000000.0;
            // Add a small arbitrary overhead for transduction in our estimation
            if (vec == TransportVector::VectorB_Transduction) {
                latency_ms += 5.0; // 5ms baseline overhead
            }

            // Adjust trust slightly based on vector choice and average anchor trust
            double trust = peer.trust_score.consistency_score;
            if (vec == TransportVector::VectorB_Transduction) {
                trust = trust * avg_anchor_trust * cluster_trust_boost;
            }

            // Normalization
            double latency_score = 1.0 / (1.0 + latency_ms);
            double trust_score = std::clamp(trust, 0.0, 1.0);

            double composite_score = 0.0;
            if (mode == RoutingMode::LOW_LATENCY) {
                composite_score = latency_score;
            } else if (mode == RoutingMode::HIGH_STABILITY) {
                composite_score = trust_score;
            } else { // BALANCED
                composite_score = 0.4 * latency_score + 0.6 * trust_score;
            }

            double final_score = composite_score * density_factor;

            // Only add valid candidates
            if (calculate_hybrid_distance(peer, state) < 1.0) {
                candidates.push_back({final_score, &peer, vec, latency_ms, trust});
            }
        }
    }

    if (candidates.empty()) {
        return decision;
    }

    // Sort candidates by final score descending
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });

    const CandidateScore& best = candidates.front();

    decision.is_valid = true;
    decision.destination_peer = best.peer->signature;
    decision.selected_vector = best.vector;
    decision.estimated_latency_ms = best.latency_ms;
    decision.estimated_stability_score = best.trust;
    decision.mode_used = mode;

    return decision;
}

void RoutingEngine::propagate_state(const wave_native::core::WaveState& local_state, const std::vector<double>& current_stream) {
    // Left as legacy wrapper if still used directly
    RouteDecision decision = compute_route(local_state, RoutingMode::BALANCED);
    propagate_route(local_state, current_stream, decision);
}

void RoutingEngine::propagate_route(const wave_native::core::WaveState& local_state, const std::vector<double>& current_stream, const RouteDecision& decision) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    last_state_ = local_state;

    if (!decision.is_valid || !decision.destination_peer.has_value()) {
        return;
    }

    if (decision.selected_vector == TransportVector::VectorB_Transduction && transduction_allowed_) {
        // Attempt to map continuous wave-state to discrete IAT
        // Here delta_theta is derived from current state derivative for simplicity
        double delta_theta = local_state.x_dot * 0.01; // Example transduction mapping

        if (!modulate_iat(delta_theta, current_stream)) {
            // Snap-back triggered, fallback to Vector A
            std::cout << "[RoutingEngine] Snap-Back triggered for peer. Re-routing via Vector A.\n";
        }
    } else {
        // Simulate Vector A direct broadcast
        // std::cout << "[RoutingEngine] Propagating via Vector A to peer.\n";
    }
}

} // namespace core
} // namespace wave_native
