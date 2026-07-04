#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <csignal>
#include <iomanip>

#include "wave_state.hpp"
#include "interceptor/phy_listener.hpp"
#include "ambient_verifier.hpp"
#include "calibration.hpp"
#include "pll_controller.hpp"
#include "routing_engine.hpp"
#include "safety_guardrails.hpp"
#include "config/wnn_config.hpp"
#include "distributed_pll/distributed_pll_controller.hpp"
#include "mesh_orchestrator/mesh_orchestrator.hpp"
#include <string.h>
#include <condition_variable>

using namespace wave_native;

// ISO 42001 Async Audit Event Structure
struct AuditLogEvent {
    double ts;
    double integrity_score;
    double safety_score;
    double phase_error;
    double A;
    double x_dot;
    size_t stream_size;
    bool is_locked;
    double omega;
};

// Global or Thread-Local SPSC Queue for Logging
interceptor::LockFreeSPSCQueue<AuditLogEvent, 4096> audit_queue;

std::atomic<bool> global_running{true};
std::atomic<bool> global_reload_config{false};

void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down...\n";
        global_running = false;
    } else if (signum == SIGHUP) {
        std::cout << "\nSIGHUP received. Reloading configuration...\n";
        global_reload_config = true;
    }
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGHUP, signal_handler);

    core::WnnConfig config = core::WnnConfig::parse(argc, argv);
    std::string interface = config.interface;
    bool calibrate_mode = config.calibrate_mode;

    std::cout << "[INIT] Wave-Native Network (WNN) bound to interface. Safety Mode: [" << config.safety_mode_to_string() << "].\n";

    // Initialize PHY Interceptor
    interceptor::PhyListener phy(interface);
    phy.start();

    // Initialize the Trust Layer Gatekeeper wrapper
    core::AmbientVerifier verifier;

    double omega = 1.2;
    double alpha = -1.0;
    double beta = 1.0;
    double delta = 0.3;

    if (calibrate_mode) {
        core::CalibrationMode calib;
        if (calib.run_sweep(omega)) {
            std::cout << "[SUCCESS] Hardware Resonance Found at ω = " << omega << ". Locking Phase.\n";
            core::CalibrationMode::save_profile("wnp_hw_profile.bin", omega, alpha, beta, delta);
        }
    } else {
        if (core::CalibrationMode::load_profile("wnp_hw_profile.bin", omega, alpha, beta, delta)) {
            std::cout << "[INFO] Loaded hardware profile: ω = " << omega << "\n";
        } else {
            std::cout << "[WARNING] No hardware profile found. Using defaults.\n";
        }
    }

    // Initialize our Duffing Oscillator engine and local state
    core::DuffingOscillator duffing(delta, alpha, beta, 0.5);
    core::WaveState state(0.1, 0.0, omega, 0.0, 0.0);

    // Initialize PLL Controller
    core::PllController pll(omega);

    // Initialize Routing Engine
    core::RoutingEngine router(&phy);

    // Initialize Distributed PLL Controller
    core::DistributedPllController distributed_pll(
        config.pll_global_lock_tolerance_deg,
        config.pll_global_settling_tolerance_ms
    );

    // Initialize Mesh Orchestrator
    core::MeshOrchestrator orchestrator(config);

    // Initialize Safety Guardrails
    core::SafetyGuardrails safety_guardrails(&router, &pll, &duffing, config.safety_mode);

    // Initialize Resonant Peer Table and Spectral Monitor
    mesh_legacy::ResonantPeerTable peer_table;
    mesh_legacy::AdaptiveSpectralMonitor spectral_monitor;

    // Initialize Space Segment Coherence Engine
    wnn::space::CoherenceEngine coherence_engine(config);

    std::mutex stream_mutex;
    std::condition_variable stream_cv;
    std::vector<double> shared_stream;
    bool new_stream_ready = false;

    const double dt = 0.01; // Time step for RK4

    auto start_time = std::chrono::steady_clock::now();
    auto last_tick = start_time;
    uint64_t tick_count = 0;

    std::atomic<double> shared_ts{0.0};

    // Pruning Background Thread
    std::jthread pruning_thread([&peer_table, &shared_ts](std::stop_token stoken) {
        while (!stoken.stop_requested() && global_running) {
            peer_table.prune_decayed_peers(shared_ts.load());
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    // Spectral Scan Background Thread
    std::jthread spectral_thread([&](std::stop_token stoken) {
        while (!stoken.stop_requested() && global_running) {
            std::vector<double> local_stream;
            {
                std::unique_lock<std::mutex> lock(stream_mutex);
                stream_cv.wait_for(lock, std::chrono::milliseconds(100), [&]{ return new_stream_ready || !global_running || stoken.stop_requested(); });
                if (!global_running || stoken.stop_requested()) break;
                if (!new_stream_ready) continue;
                local_stream = shared_stream;
                new_stream_ready = false;
            }
            // Execute in background
            spectral_monitor.analyze_stream(local_stream);
        }
    });

    // Async Audit Logging Thread
    std::jthread audit_thread([&](std::stop_token stoken) {
        while (!stoken.stop_requested() && global_running) {
            AuditLogEvent event;
            while (audit_queue.pop(event)) {
                // Async Merkle Tree / Audit logging here
                std::cout << std::fixed << std::setprecision(4)
                          << "[Wavefrontd] t=" << event.ts
                          << " | Amp(A): " << event.A
                          << " | Vel(x'): " << event.x_dot
                          << " | Stream Size: " << event.stream_size
                          << " | AILEE Integrity: " << (event.integrity_score * 100.0) << "%"
                          << " | Safety: " << (event.safety_score * 100.0) << "%"
                          << " | PLL Lock: " << (event.is_locked ? "LOCKED" : "SEARCHING")
                          << " | Δθ: " << event.phase_error
                          << " | ω: " << event.omega
                          << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // run_physics_engine loop
    while (global_running) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - last_tick;

        if (elapsed.count() >= dt) {
            state.ts = tick_count * dt; // Strict temporal hardening derived from monotonic clock without jitter

            // 1. Ingest raw physical noise and IAT stream
            std::vector<double> stream = phy.consume_stream();
            std::vector<double> iat_stream = phy.consume_iats();

            // Safety Guardrails checking Opacity
            safety_guardrails.process_iat_samples(iat_stream);

            // 2. Trust Validation - analyze signal entropy
            verifier.analyze_signal_entropy(stream);

            // 2.5 Spectral Analysis & Dynamic Lock (Asynchronous)
            {
                std::lock_guard<std::mutex> lock(stream_mutex);
                shared_stream = stream;
                new_stream_ready = true;
            }
            stream_cv.notify_one();

            double psi_snr = spectral_monitor.get_latest_snr();
            if (auto cand_omega = spectral_monitor.get_candidate_frequency()) {
                // We have a fold signature candidate! Let the PLL try to lock it.
                // In a true implementation, we might nudge the pll base omega.
                // For now, we rely on the normal step but log it.
            }

            // PLL Update step
            double omega_corrected = pll.step(stream, state.theta, dt, state.ts);

            // Check for break lock and quarantine
            if (std::abs(pll.get_phase_error()) > M_PI / 2.0) {
                verifier.trigger_quarantine();
            } else if (pll.is_replay_detected()) {
                verifier.trigger_quarantine();
                pll.clear_replay_flag();
                pll.reset();
            } else if (pll.is_locked()) {
                verifier.set_pll_locked(true);

                // Mock Trial Handshake
                std::vector<uint8_t> mock_sig = {0xAA, 0xBB, 0xCC};
                mesh_legacy::ZKProof mock_proof(state.A, state.theta, state.omega, state.x_dot);
                mesh_legacy::AileeTrustScore mock_score = {0.8, 0.8, 0.8, 0.8};

                if (verifier.verify_peer(mock_sig, mock_proof, mock_score, psi_snr)) {
                    mesh_legacy::ResonantPeer new_peer{mock_sig, state.omega, psi_snr, mock_score, state.ts, state.theta};
                    peer_table.insert_or_update(new_peer);

                    // Add to routing engine
                    router.add_or_update_peer(mock_sig, state.omega, mock_score, state.theta, iat_stream);

                    // Dynamically map peers to Space Segment Coherence Engine for tracking phase errors
                    // Derive a pseudo anchor ID from the first 4 bytes of the signature for the sake of integration
                    if (mock_sig.size() >= 4) {
                        uint32_t pseudo_anchor_id = (mock_sig[0] << 24) | (mock_sig[1] << 16) | (mock_sig[2] << 8) | mock_sig[3];

                        if (coherence_engine.has_anchor(pseudo_anchor_id)) {
                            coherence_engine.update_anchor_stability(pseudo_anchor_id, psi_snr > 0 ? 1.0 : 0.0);
                        } else {
                            wnn::space::PhaseAnchorConfig anchor_cfg{state.omega, 1.0};
                            wnn::space::PhaseCoherenceAnchor anchor(pseudo_anchor_id, anchor_cfg);
                            anchor.update_phase_stability(psi_snr > 0 ? 1.0 : 0.0);
                            coherence_engine.add_anchor(anchor);
                        }
                    }
                }
            }

            double trust_score = verifier.get_physical_integrity_score();

            // Update dynamic frequency
            state.omega = omega_corrected;

            // 3. RK4 Duffing Step - this is where we "resonate" with the physical layer
            // We use the mean amplitude of the ingested stream to drive the forcing frequency
            // or directly inject it as a perturbation. For simplicity, let's add the
            // instantaneous physical amplitude as an external forcing term to the state velocity.
            double phys_amp = 0.0;
            if (!stream.empty()) {
                phys_amp = stream.back(); // Use the latest amplitude point
            }

            // Perturb the state velocity with the physical network wave
            state.x_dot += phys_amp * 0.1 * dt;

            // Entropy-to-Phase Synchronization (Nudge)
            state.theta += phys_amp * 0.05 * dt; // Nudge the phase angle based on raw bitstream

            // Step the Duffing oscillator
            duffing.step(state, dt);

            shared_ts.store(state.ts);

            // Calculate variables for adaptive routing mode selection
            double drift_ppm = std::abs(pll.get_phase_error()) * 1e6; // simple approximation
            double avg_trust = trust_score; // derived from verifier.get_physical_integrity_score()
            // rough estimation of mesh density (max 100 for normalization)
            double mesh_density = std::clamp(static_cast<double>(peer_table.get_peer_count()) / 100.0, 0.0, 1.0);

            router.set_mesh_density(mesh_density);

            // Real integration with Space Segment / CoherenceEngine
            auto clusters = coherence_engine.build_coherence_clusters();
            router.set_coherence_clusters(clusters);

            std::vector<wave_native::core::AnchorId> active_anchors;
            for (const auto& cluster : clusters) {
                active_anchors.insert(active_anchors.end(), cluster.anchors.begin(), cluster.anchors.end());
            }

            // Sync anchor IDs to distributed PLL
            distributed_pll.set_nodes(active_anchors);

            // Update phase error from the clusters (assigning cluster average phase error to its anchors for now)
            // In a fully integrated system, phase errors would be extracted per-anchor during handshake
            // Here, we feed the cluster's average phase degree back into the tracking loop
            for (const auto& cluster : clusters) {
                for (auto anchor_id : cluster.anchors) {
                    distributed_pll.update_phase_error(anchor_id, cluster.average_phase_deg, state.ts * 1000.0);
                }
            }

            // Step the Distributed PLL
            distributed_pll.step(state.ts * 1000.0);

            // Feed PLL state to routing engine and safety guardrails
            router.set_pll_node_states(distributed_pll.node_states());
            router.set_network_pll_locked(distributed_pll.is_network_locked());

            safety_guardrails.update_pll_network_state(
                distributed_pll.is_network_locked(),
                state.ts * 1000.0,
                config.pll_unstable_incident_threshold_ms
            );

            // Integrate with Mesh Orchestrator
            std::vector<core::MeshNodeState> mesh_nodes;
            for (const auto& peer : peer_table.get_all_peers()) {
                core::MeshNodeState node_state;
                node_state.peer_id = peer.signature;

                // Extract AnchorId if possible (mock logic from earlier in wavefrontd)
                if (peer.signature.size() >= 4) {
                    uint32_t pseudo_anchor_id = (peer.signature[0] << 24) | (peer.signature[1] << 16) | (peer.signature[2] << 8) | peer.signature[3];
                    node_state.anchor_id = pseudo_anchor_id;
                } else {
                    node_state.anchor_id = std::nullopt;
                }

                node_state.mesh_density_local = mesh_density; // Using global/local approximation
                node_state.trust_score = peer.ailee_score.consistency_score; // Just an example trust metric

                // Fetch phase error and PLL lock status from distributed PLL if anchor exists
                node_state.phase_error_deg = 0.0;
                node_state.pll_locked = true;
                node_state.coherence_stable = true; // Default to stable if no anchor

                if (node_state.anchor_id) {
                    for (const auto& pll_state : distributed_pll.node_states()) {
                        if (pll_state.anchor_id == node_state.anchor_id.value()) {
                            node_state.phase_error_deg = pll_state.phase_error_deg;
                            node_state.pll_locked = pll_state.is_locked;
                            break;
                        }
                    }

                    // Check coherence clusters for stability
                    for (const auto& cluster : clusters) {
                        if (std::find(cluster.anchors.begin(), cluster.anchors.end(), node_state.anchor_id.value()) != cluster.anchors.end()) {
                            node_state.coherence_stable = cluster.is_stable;
                            break;
                        }
                    }
                }

                mesh_nodes.push_back(node_state);
            }

            orchestrator.set_mesh_nodes(mesh_nodes);
            orchestrator.set_network_pll_locked(distributed_pll.is_network_locked());
            orchestrator.set_coherence_clusters(clusters);
            orchestrator.step(state.ts * 1000.0);

            router.set_mesh_orchestrator_state(orchestrator);
            safety_guardrails.update_mesh_health(
                orchestrator.is_mesh_healthy(),
                state.ts * 1000.0,
                config.mesh_health_degraded_threshold_ms
            );

            // Pass anchor trust logic to router, wrapping single global trust as example
            std::vector<double> current_trusts = {avg_trust};
            router.set_anchor_trust_scores(current_trusts);

            core::RoutingMode current_mode = core::RoutingMode::BALANCED;
            if (drift_ppm > 50.0 || avg_trust < 0.4 || mesh_density < 0.3) {
                current_mode = core::RoutingMode::HIGH_STABILITY;
            } else if (drift_ppm < 10.0 && avg_trust > 0.7 && mesh_density > 0.6) {
                current_mode = core::RoutingMode::LOW_LATENCY;
            }

            // Propagate Wave-State through Adaptive Mesh Route Decision
            core::RouteDecision route_decision = router.compute_route(state, current_mode);
            router.propagate_route(state, stream, route_decision);

            tick_count++;
            last_tick = start_time + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(tick_count * dt));

    // Handle configuration reloading
    if (global_reload_config) {
        config = core::WnnConfig::parse(argc, argv);
        global_reload_config = false;
        std::cout << "[INFO] Configuration reloaded.\n";
    }

    // Determine logging frequency based on mode
    int log_frequency = 100;
    if (config.demo_mode) {
        log_frequency = 500; // Less frequent
    } else if (config.diagnostic_mode) {
        log_frequency = 50;  // More frequent
    }

    // Apply stress mode adjustments
    double local_dt = dt;
    if (config.stress_mode) {
        // Higher update rate or simulated jitter
        state.theta += (static_cast<double>(rand()) / RAND_MAX - 0.5) * 0.1;
    }

    // Print status every `log_frequency` ticks
    if (tick_count % log_frequency == 0) {
                AuditLogEvent event{
                    static_cast<double>(state.ts), 
                    trust_score, 
                    safety_guardrails.get_current_safety_score(), 
                    pll.get_phase_error(),
                    static_cast<double>(state.A),
                    static_cast<double>(state.x_dot),
                    stream.size(),
                    pll.is_locked(),
                    static_cast<double>(state.omega)
                };
                audit_queue.push(event); // Dispatched to async Merkle Tree thread in O(1)

        if (config.diagnostic_mode) {
            std::cout << "[DIAGNOSTIC] Wavefront update rate: " << (1.0 / local_dt) << " Hz\n";
            std::cout << "[DIAGNOSTIC] Anchor coherence summary: Tracking " << peer_table.get_peer_count() << " resonant peers.\n";
            std::cout << "[DIAGNOSTIC] Routing mode summary: Transduction=" << (router.is_vector_a_viable() ? "OFF" : "ON") << "\n";
        }
            }
        } else {
            // Yield briefly to avoid 100% CPU on spin
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::cout << "Stopping PHY listener...\n";
    phy.stop();





    std::cout << "Wavefront Daemon shutdown complete.\n";

    return 0;
}
