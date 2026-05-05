#pragma once

#include <memory>
#include <vector>
#include "../mesh_legacy/mesh_discovery.hpp"
#include "../mesh_legacy/zk_verifier.hpp"

namespace wave_native {
namespace core {

class AmbientVerifier {
public:
    AmbientVerifier();

    // Analyzes the continuous amplitude stream and updates the overall entropy metric
    void analyze_signal_entropy(const std::vector<double>& stream);

    // Get the current local physical entropy metric (0.0 = pure chaos/noise, 1.0 = highly deterministic)
    double get_physical_integrity_score() const;

    // Use the underlying PeerGatekeeper to process a peer, injecting our physical score
    bool verify_peer(const std::vector<uint8_t>& phase_signature,
                     const mesh_legacy::ZKProof& proof,
                     mesh_legacy::AileeTrustScore& score);

private:
    std::shared_ptr<mesh_legacy::ZKVerifier> zk_verifier_;
    std::shared_ptr<mesh_legacy::PeerGatekeeper> gatekeeper_;

    double current_integrity_score_;
};

} // namespace core
} // namespace wave_native
