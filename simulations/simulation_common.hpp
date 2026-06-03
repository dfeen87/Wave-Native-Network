#pragma once

#include "wave_state.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <string>
#include <vector>

namespace wnn_sim {

inline constexpr double kRuntimeSec = 60.0;
inline constexpr double kDt = 0.01;
inline constexpr int kSeedCount = 20;

inline std::array<uint32_t, kSeedCount> seeds() {
    std::array<uint32_t, kSeedCount> out{};
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<uint32_t>(1337 + i * 7919);
    }
    return out;
}

inline std::filesystem::path project_root() {
#ifdef WNN_PROJECT_ROOT
    return std::filesystem::path(WNN_PROJECT_ROOT);
#else
    return std::filesystem::current_path();
#endif
}

inline std::filesystem::path data_path(const std::string& filename) {
    auto path = project_root() / "data";
    std::filesystem::create_directories(path);
    return path / filename;
}

inline double wrap_pi(double x) {
    while (x > M_PI) x -= 2.0 * M_PI;
    while (x < -M_PI) x += 2.0 * M_PI;
    return x;
}

inline double clamp01(double v) {
    return std::max(0.0, std::min(1.0, v));
}

struct PairSimState {
    wave_native::core::DuffingOscillator osc_a{};
    wave_native::core::DuffingOscillator osc_b{};
    wave_native::core::WaveState a{0.2, 0.0, 1.20, 0.0, 0.0};
    wave_native::core::WaveState b{0.2, 0.0, 1.20, 0.0, 0.0};
};

inline void rk4_pair_step(PairSimState& s, double dt) {
    s.osc_a.step(s.a, dt);
    s.osc_b.step(s.b, dt);
}

}  // namespace wnn_sim
