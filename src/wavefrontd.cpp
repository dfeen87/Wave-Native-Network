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

void signal_handler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down...\n";
    global_running = false;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

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

    // Initialize Safety Guardrails
    core::SafetyGuardrails safety_guardrails(&router, &pll, &duffing, config.safety_mode);

    // Initialize Resonant Peer Table and Spectral Monitor
    mesh_legacy::ResonantPeerTable peer_table;
    mesh_legacy::AdaptiveSpectralMonitor spectral_monitor;

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

            // Propagate Wave-State through Adaptive Mesh
            router.propagate_state(state, stream);

            tick_count++;
            last_tick = start_time + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(tick_count * dt));

            // Print status every ~1 second (100 ticks at 0.01s dt)
            if (tick_count % 100 == 0) {
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
