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
#include <string.h>

using namespace wave_native;

std::atomic<bool> global_running{true};

void signal_handler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down...\n";
    global_running = false;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::string interface = "eth0"; // Default interface
    bool calibrate_mode = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--calibrate") == 0) {
            calibrate_mode = true;
        } else {
            interface = argv[i];
        }
    }

    std::cout << "Starting Wavefront Daemon on interface: " << interface << "\n";

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

    const double dt = 0.01; // Time step for RK4

    auto last_tick = std::chrono::steady_clock::now();
    uint64_t tick_count = 0;

    // run_physics_engine loop
    while (global_running) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - last_tick;

        if (elapsed.count() >= dt) {
            last_tick = now;
            tick_count++;

            // 1. Ingest raw physical noise
            std::vector<double> stream = phy.consume_stream();

            // 2. Trust Validation - analyze signal entropy
            verifier.analyze_signal_entropy(stream);

            // PLL Update step
            double omega_corrected = pll.step(stream, state.theta, dt);

            // Check for break lock and quarantine
            if (std::abs(pll.get_phase_error()) > M_PI / 2.0) {
                verifier.trigger_quarantine();
            } else if (pll.is_locked()) {
                verifier.set_pll_locked(true);
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

            // Print status every ~1 second (100 ticks at 0.01s dt)
            if (tick_count % 100 == 0) {
                std::cout << std::fixed << std::setprecision(4)
                          << "[Wavefrontd] t=" << state.ts
                          << " | Amp(A): " << state.A
                          << " | Vel(x'): " << state.x_dot
                          << " | Stream Size: " << stream.size()
                          << " | AILEE Integrity: " << (trust_score * 100.0) << "%"
                          << " | PLL Lock: " << (pll.is_locked() ? "LOCKED" : "SEARCHING")
                          << " | Δθ: " << pll.get_phase_error()
                          << " | ω: " << state.omega
                          << std::endl;
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
