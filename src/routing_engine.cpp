#include "routing_engine.hpp"

namespace wave_native {
namespace core {

void RoutingEngine::add_or_update_peer(const std::vector<uint8_t>& signature,
                                       double spectral_frequency,
                                       const mesh_legacy::AileeTrustScore& score) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    double fitness = score.determinism_score + score.safety_score;
    mesh_map_[signature] = KnownPeer{signature, spectral_frequency, score, fitness};
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
        if (peer.path_fitness >= 0.2) return true;
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

void RoutingEngine::propagate_state(const wave_native::core::WaveState& local_state, const std::vector<double>& current_stream) {
    std::lock_guard<std::mutex> lock(map_mutex_);

    for (const auto& [signature, peer] : mesh_map_) {
        // Skip poorly performing paths
        if (peer.path_fitness < 0.2) continue;

        TransportVector vector = select_vector(local_state.omega, peer);

        if (vector == TransportVector::VectorB_Transduction && transduction_allowed_) {
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
}

} // namespace core
} // namespace wave_native
