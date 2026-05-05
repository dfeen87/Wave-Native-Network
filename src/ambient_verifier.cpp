#include "ambient_verifier.hpp"
#include <cmath>
#include <numeric>

namespace wave_native {
namespace core {

AmbientVerifier::AmbientVerifier() : current_integrity_score_(1.0), is_pll_locked_(false) {
    zk_verifier_ = std::shared_ptr<mesh_legacy::ZKVerifier>(mesh_legacy::ZKVerifier::create().release());
    gatekeeper_ = std::make_shared<mesh_legacy::PeerGatekeeper>(zk_verifier_);
}

void AmbientVerifier::analyze_signal_entropy(const std::vector<double>& stream) {
    if (stream.empty()) return;

    // Calculate a rough "entropy" / chaos metric using standard deviation
    double mean = std::accumulate(stream.begin(), stream.end(), 0.0) / stream.size();

    double sq_sum = std::inner_product(stream.begin(), stream.end(), stream.begin(), 0.0,
        std::plus<>(), [mean](double a, double b) { return (a - mean) * (b - mean); });

    double variance = sq_sum / stream.size();

    // Variance maxes out around 1.0/3.0 for uniform distribution [-1, 1], meaning high chaos.
    // If variance is very high, signal is mostly noise. If very low, it's a flatline.
    // For a structured wave, variance should be somewhere in between.

    // We want to penalize extreme noise (high variance) as low integrity
    // A simplified metric:
    double chaos = std::min(variance * 3.0, 1.0); // Normalize roughly to [0, 1]

    // Smooth the score
    double new_score = 1.0 - chaos;
    current_integrity_score_ = 0.9 * current_integrity_score_ + 0.1 * new_score;
}

double AmbientVerifier::get_physical_integrity_score() const {
    return current_integrity_score_;
}

bool AmbientVerifier::verify_peer(const std::vector<uint8_t>& phase_signature,
                                   const mesh_legacy::ZKProof& proof,
                                   mesh_legacy::AileeTrustScore& score) {

    // Instantly reject if system is in quarantine state
    if (current_integrity_score_ == 0.0) {
        score.confidence_score = 0.0;
        score.consistency_score = 0.0;
        score.determinism_score = 0.0;
        score.safety_score = 0.0;
        return false;
    }

    // Inject our physical layer integrity into the mathematical AILEE trust score
    // In actual implementation this might be a complex multi-dimensional mapping
    score.confidence_score *= current_integrity_score_;
    score.consistency_score *= current_integrity_score_;
    score.determinism_score *= current_integrity_score_;

    // Let the gatekeeper do the hard auth
    return gatekeeper_->process_incoming_peer(phase_signature, proof, score);
}

void AmbientVerifier::set_pll_locked(bool locked) {
    is_pll_locked_ = locked;
    if (locked) {
        // Dynamically increase Confidence and Determinism scores when phase-locked
        // This is a simplified boost; in a real system it might interact with `gatekeeper_`
        // or directly scale incoming scores, but here we adjust the local physical integrity score
        // to boost future trust validations.
        current_integrity_score_ = std::min(1.0, current_integrity_score_ * 1.2);
    }
}

void AmbientVerifier::trigger_quarantine() {
    is_pll_locked_ = false;
    current_integrity_score_ = 0.0;
    // Dropping the score to 0.00 will effectively quarantine connections
}

} // namespace core
} // namespace wave_native
