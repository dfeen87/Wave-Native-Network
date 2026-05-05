#include "zk_verifier.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace wave_native {
namespace mesh_legacy {

    std::unique_ptr<ZKVerifier> ZKVerifier::create() {
        return std::unique_ptr<ZKVerifier>(new ZKVerifier());
    }

    double AileeTrustScore::aggregate_score() const {
        const double SAFETY_WEIGHT = 0.35;
        const double CONSISTENCY_WEIGHT = 0.30;
        const double CONFIDENCE_WEIGHT = 0.20;
        const double DETERMINISM_WEIGHT = 0.15;

        double agg = (safety_score * SAFETY_WEIGHT) +
                     (consistency_score * CONSISTENCY_WEIGHT) +
                     (confidence_score * CONFIDENCE_WEIGHT) +
                     (determinism_score * DETERMINISM_WEIGHT);
        return std::max(0.0, std::min(1.0, agg));
    }

    bool ZKVerifier::verify_proof(const ZKProof& proof, double& out_drift) const {
        // Forward-integrated simulation of Duffing Equation
        // ẍ + δẋ + αx + βx³ = F cos(ωt)
        double dt = 0.01;
        double t_end = 1.0;

        double local_x = 0.0;
        double local_v = 0.0;

        double delta = 0.1;
        double alpha = -1.0;
        double beta = 1.0;
        double F = 0.3;
        double omega = proof.frequency;

        for (double t = 0; t < t_end; t += dt) {
            double a = F * std::cos(omega * t) - delta * local_v - alpha * local_x - beta * local_x * local_x * local_x;
            local_x += local_v * dt;
            local_v += a * dt;
        }

        double peer_x = proof.amplitude * std::cos(proof.phase_angle);
        double peer_v = proof.velocity;

        out_drift = std::sqrt(std::pow(local_x - peer_x, 2) + std::pow(local_v - peer_v, 2));

        return out_drift <= get_epsilon();
    }

} // namespace mesh_legacy
} // namespace wave_native
