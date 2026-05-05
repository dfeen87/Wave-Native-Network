#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <optional>

namespace wave_native {
namespace mesh_legacy {

    struct ZKProof {
        double amplitude;
        double phase_angle;
        double frequency;
        double velocity;

        ZKProof(double a, double p, double f, double v)
            : amplitude(a), phase_angle(p), frequency(f), velocity(v) {}
    };

    struct AileeTrustScore {
        double confidence_score;
        double safety_score;
        double consistency_score;
        double determinism_score;

        double aggregate_score() const;
    };

    class ZKVerifier {
    public:
        static std::unique_ptr<ZKVerifier> create();

        bool verify_proof(const ZKProof& proof, double& out_drift) const;
        double get_epsilon() const { return 0.1; }

    private:
        ZKVerifier() = default;
    };

} // namespace mesh_legacy
} // namespace wave_native
