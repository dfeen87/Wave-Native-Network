#include "zk_verifier.hpp"
#include <iostream>

namespace wave_native {
namespace mesh_legacy {

    ZKVerifier::ZKVerifier(VerificationKey verification_key)
        : verification_key_(std::move(verification_key)) {}

    std::unique_ptr<ZKVerifier> ZKVerifier::create(const VerificationKey& verification_key) {
        // In a real implementation with cryptographic libraries:
        // Attempt to deserialize the verification key.
        // If it fails, return nullptr.

        if (verification_key.key_data.empty()) {
            // Mock failure condition for invalid key data
            return nullptr;
        }

        return std::unique_ptr<ZKVerifier>(new ZKVerifier(verification_key));
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

    bool ZKVerifier::verify_proof(const ZKProof& proof, const std::vector<uint8_t>& public_inputs) const {
        // Mock verification logic.
        // In a real implementation:
        // 1. Deserialize proof from proof.proof_data
        // 2. Deserialize public_inputs into Fr elements
        // 3. Verify using Groth16::verify(verification_key_, public_inputs_fe, ark_proof)

        if (proof.proof_data.empty()) {
            return false;
        }

        // Simulating the check for 2 public inputs (module_hash and input_hash)
        // as seen in the Rust code, though here we just return true for the stub.
        if (public_inputs.empty()) {
            return false;
        }

        return true;
    }

    bool ZKVerifier::verify_ailee_trust(const ZKProof& proof, const std::vector<uint8_t>& public_inputs, const AileeTrustScore& score) const {
        if (!verify_proof(proof, public_inputs)) {
            return false;
        }

        if (score.aggregate_score() < 0.70) {
            return false;
        }

        return true;
    }

    size_t ZKVerifier::proof_size(const ZKProof& proof) const {
        return proof.size();
    }

} // namespace mesh_legacy
} // namespace wave_native
