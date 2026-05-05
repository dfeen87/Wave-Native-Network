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
    if (argc > 1) {
        interface = argv[1];
    }

    std::cout << "Starting Wavefront Daemon on interface: " << interface << "\n";

    // Initialize PHY Interceptor
    interceptor::PhyListener phy(interface);
    phy.start();

    // Initialize the Trust Layer Gatekeeper wrapper
    core::AmbientVerifier verifier;

    // Initialize our Duffing Oscillator engine and local state
    core::DuffingOscillator duffing(0.3, -1.0, 1.0, 0.5);
    core::WaveState state(0.1, 0.0, 1.2, 0.0, 0.0);

    const double dt = 0.01; // Time step for RK4

    auto last_tick = std::chrono::steady_clock::now();
    uint64_t tick_count = 0;

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
            double trust_score = verifier.get_physical_integrity_score();

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
