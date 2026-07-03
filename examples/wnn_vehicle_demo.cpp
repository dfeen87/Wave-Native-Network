#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <nlohmann/json.hpp>

#include "wave_native_network.hpp"

using json = nlohmann::json;

void run_demo() {
    std::cout << "--- Wave-Native Network (WNN) Vehicle Demo ---" << std::endl;

    // Load configuration
    std::ifstream config_file("wnn_config.json");
    if (!config_file.is_open()) {
        std::cerr << "Failed to open wnn_config.json!" << std::endl;
        return;
    }
    json config;
    config_file >> config;

    // 1. Space Segment - LEO Phase Coherence Anchor
    // Config values for anchor
    double base_omega = config["space_segment"]["phase_anchors"][0]["base_omega"];
    double rel_comp = config["space_segment"]["phase_anchors"][0]["relativistic_comp_factor"];
    wnn::space::PhaseAnchorConfig anchor_cfg{base_omega, rel_comp};
    wnn::space::PhaseCoherenceAnchor anchor(1 /* id */, anchor_cfg);

    // Simulate LEO satellite at 17,000 mph (~ 7598 m/s)
    wnn::space::OrbitalState leo_state{500.0, 7598.0, 97.5, 0.0};

    wnn::space::PhaseAnchorSignal signal = anchor.compute_signal(leo_state);
    std::cout << "\n[Space Segment] Satellite computed signal:" << std::endl;
    std::cout << "  - Effective Omega: " << std::fixed << std::setprecision(2) << signal.omega_effective << " rad/s" << std::endl;
    std::cout << "  - Phase: " << std::setprecision(4) << signal.phase << " rad" << std::endl;

    // 2. Infrastructure Segment - V2X Handoff Core
    wnn::infra::BareMetalEngine engine;

    // Simulate vehicle at 70 mph (~ 31 m/s)
    wnn::infra::VehicleState vehicle{31.0, 90.0, 40.7128, -74.0060, 0.0};

    // Simulate beams for handoff
    wnn::infra::BeamState current_beam{0.1, 1e6, 20.0, 0.0};
    wnn::infra::BeamState next_beam{0.15, 1.0005e6, 25.0, 0.0}; // slight phase & freq mismatch

    wnn::infra::HandoffMetric metric = engine.evaluate_handoff(vehicle, current_beam, next_beam);
    std::cout << "\n[Infrastructure Segment] Handoff evaluation:" << std::endl;
    std::cout << "  - Continuity Score: " << std::setprecision(4) << metric.continuity_score << std::endl;
    std::cout << "  - Valid Candidate: " << (metric.candidate_valid ? "YES (>= 0.950)" : "NO") << std::endl;

    // 3. Sync Membrane - Atmospheric Noise & Replay Defense
    // Configure Relaxed mode for demo purposes
    wnn::sync::PhaseShimmerTolerance tol;
    tol.max_latency_ms = config["sync_membrane"]["phase_shimmer_tolerance"]["max_latency_ms"];
    tol.max_phase_variation = config["sync_membrane"]["phase_shimmer_tolerance"]["max_phase_variation"];

    wnn::sync::SyncMembrane sync_membrane(wnn::sync::SafetyMode::Relaxed, tol);

    wnn::sync::AtmosphereState atm{0.5, 0.2, 0.1}; // Moderate weather

    // Test shimmer tolerance
    bool shimmer_tolerated = sync_membrane.tolerate_phase_shimmer(atm, next_beam);
    std::cout << "\n[Sync Membrane] Atmospheric evaluation:" << std::endl;
    std::cout << "  - Tolerate Shimmer (Relaxed): " << (shimmer_tolerated ? "YES" : "NO") << std::endl;

    // Test replay attack detection
    wnn::sync::SaltEpoch salt{12345, 0.0}; // Epoch with 0.0 expected divergence
    wnn::infra::BeamState spoofed_beam{0.8, 1e6, 15.0, 0.0}; // large phase diff vs salt

    wnn::sync::StabilityDivergence div = sync_membrane.compute_stability_divergence(salt, spoofed_beam);
    std::cout << "\n[Sync Membrane] Replay attack detection:" << std::endl;
    std::cout << "  - Delta Phi: " << std::setprecision(4) << div.delta_phi << " rad" << std::endl;
    std::cout << "  - Suspicious Activity: " << (div.suspicious ? "DETECTED" : "CLEAR") << std::endl;
}

int main() {
    run_demo();
    return 0;
}
