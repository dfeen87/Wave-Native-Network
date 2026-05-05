#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <optional>

namespace wave_native {
namespace mesh_legacy {

    struct VerificationKey {
        std::vector<uint8_t> key_data;
    };

    struct ZKProof {
        std::vector<uint8_t> proof_data;
        std::vector<uint8_t> public_inputs;
        std::string circuit_id;
        std::string proof_system;

        ZKProof(std::vector<uint8_t> p_data, std::vector<uint8_t> p_inputs, std::string c_id)
            : proof_data(std::move(p_data)),
              public_inputs(std::move(p_inputs)),
              circuit_id(std::move(c_id)),
              proof_system("groth16-bn254") {}

        size_t size() const {
            return proof_data.size();
        }
    };

    struct ExecutionTrace {
        std::string module_hash;
        std::string function_name;
        std::vector<uint8_t> inputs;
        std::vector<uint8_t> outputs;
        uint64_t execution_time_ms;
        uint64_t gas_used;
        uint64_t timestamp;
    };

    class ZKVerifier {
    public:
        // Returns nullptr if verification key is invalid
        static std::unique_ptr<ZKVerifier> create(const VerificationKey& verification_key);

        bool verify_proof(const ZKProof& proof, const std::vector<uint8_t>& public_inputs) const;
        size_t proof_size(const ZKProof& proof) const;

    private:
        explicit ZKVerifier(VerificationKey verification_key);

        VerificationKey verification_key_;

        // In a real implementation, this would hold the parsed Arkworks Groth16 VerificationKey
        // e.g., ark_groth16::VerifyingKey<ark_bn254::Bn254> ark_vk_;
    };

} // namespace mesh_legacy
} // namespace wave_native
