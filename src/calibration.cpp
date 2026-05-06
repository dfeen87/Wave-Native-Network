#include "calibration.hpp"
#include "wave_state.hpp"
#include "../mesh_legacy/zk_verifier.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <thread>
#include <chrono>

namespace wave_native {
namespace core {

bool CalibrationMode::run_sweep(long double& out_omega) {
    std::cout << "[Calibration] Starting non-linear frequency sweep (500 Hz -> 20 kHz)...\n";

    double best_omega = 500.0;
    double max_aggregate_score = -1.0;

    const double start_freq = 500.0;
    const double end_freq = 20000.0;
    const double d_omega = 100.0; // 100 Hz steps
    const int window_size = 5000; // 5000 samples
    const double dt = 0.01;

    double alpha = -1.0;
    double beta = 1.0;
    double delta = 0.3;
    double F = 0.5;

    for (double freq = start_freq; freq <= end_freq; freq += d_omega) {
        DuffingOscillator duffing(delta, alpha, beta, F);
        WaveState state(0.1, 0.0, freq, 0.0, 0.0);

        double sum_aggregate = 0.0;

        for (int i = 0; i < window_size; ++i) {
            // Step oscillator
            duffing.step(state, dt);

            // For fitness, we simulate AileeTrustScore components based on state.
            mesh_legacy::AileeTrustScore score;
            // Map amplitude and velocity to "consistency" and "confidence".
            // High energy -> higher confidence (bounded to [0,1])
            double energy = 0.5 * std::pow(state.x_dot, 2) + 0.5 * alpha * std::pow(state.A, 2) + 0.25 * beta * std::pow(state.A, 4);
            score.confidence_score = std::min(1.0, std::abs(energy) * 0.5);
            score.consistency_score = std::min(1.0, static_cast<double>(1.0 / (1.0 + std::abs(state.x_dot))));
            score.safety_score = 1.0;
            score.determinism_score = std::min(1.0, static_cast<double>(1.0 / (1.0 + std::abs(state.A))));

            sum_aggregate += score.aggregate_score();
        }

        if (sum_aggregate > max_aggregate_score) {
            max_aggregate_score = sum_aggregate;
            best_omega = freq;
        }

        // Real-time console visualization
        int bar_width = 50;
        double normalized_score = sum_aggregate / window_size; // ~ [0, 1]
        int pos = static_cast<int>(bar_width * normalized_score);
        if (pos > bar_width) pos = bar_width; // clamp to prevent overflow

        std::cout << "\r[Sweep] freq=" << std::setw(6) << freq << " Hz | "
                  << "Score=" << std::fixed << std::setprecision(4) << normalized_score << " [";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "]" << std::flush;

        // Use std::chrono for microsecond-precision timing to pace the sweep and make visualization real-time
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start_time < std::chrono::microseconds(10000)) {
            // Wait for 10ms per frequency step to simulate a physical hardware response delay
        }
    }

    std::cout << "\n";
    out_omega = best_omega;
    return true;
}

bool CalibrationMode::save_profile(const std::string& path, double omega, double alpha, double beta, double delta) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(&omega), sizeof(omega));
    out.write(reinterpret_cast<const char*>(&alpha), sizeof(alpha));
    out.write(reinterpret_cast<const char*>(&beta), sizeof(beta));
    out.write(reinterpret_cast<const char*>(&delta), sizeof(delta));
    return true;
}

bool CalibrationMode::load_profile(const std::string& path, long double& omega, long double& alpha, long double& beta, long double& delta) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    in.read(reinterpret_cast<char*>(&omega), sizeof(omega));
    in.read(reinterpret_cast<char*>(&alpha), sizeof(alpha));
    in.read(reinterpret_cast<char*>(&beta), sizeof(beta));
    in.read(reinterpret_cast<char*>(&delta), sizeof(delta));
    return true;
}

} // namespace core
} // namespace wave_native
