#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <cstring>

namespace wave_native {
namespace core {

enum class SafetyMode {
    STRICT,
    RELAXED,
    OFF
};

struct WnnConfig {
    std::string interface = "eth0";
    bool calibrate_mode = false;
    SafetyMode safety_mode = SafetyMode::STRICT;
    bool demo_mode = false;
    bool diagnostic_mode = false;
    bool stress_mode = false;

    // Phase Coherence Cluster thresholds
    double coherence_phase_error_threshold = 5.0;
    double coherence_trust_threshold = 0.6;
    std::size_t coherence_min_cluster_size = 2;

    // Distributed PLL thresholds
    double pll_global_lock_tolerance_deg = 5.0;
    double pll_global_settling_tolerance_ms = 50.0;
    double pll_unstable_incident_threshold_ms = 1000.0;

    static WnnConfig parse(int argc, char** argv) {
        WnnConfig config;

        // Parse wnn.conf if it exists (defaults)
        std::ifstream conf_file("wnn.conf");
        if (conf_file.is_open()) {
            std::string line;
            while (std::getline(conf_file, line)) {
                if (line.find("safety=strict") != std::string::npos) {
                    config.safety_mode = SafetyMode::STRICT;
                } else if (line.find("safety=relaxed") != std::string::npos) {
                    config.safety_mode = SafetyMode::RELAXED;
                } else if (line.find("safety=off") != std::string::npos) {
                    config.safety_mode = SafetyMode::OFF;
                }
            }
        }

        // Parse CLI arguments (overrides)
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--calibrate") == 0) {
                config.calibrate_mode = true;
            } else if (std::strcmp(argv[i], "--safety") == 0 && i + 1 < argc) {
                std::string mode_str = argv[++i];
                if (mode_str == "strict") {
                    config.safety_mode = SafetyMode::STRICT;
                } else if (mode_str == "relaxed") {
                    config.safety_mode = SafetyMode::RELAXED;
                } else if (mode_str == "off") {
                    config.safety_mode = SafetyMode::OFF;
                } else {
                    std::cerr << "Warning: Unknown safety mode '" << mode_str << "', defaulting to strict.\n";
                }
            } else if (std::strcmp(argv[i], "--demo") == 0) {
                config.demo_mode = true;
            } else if (std::strcmp(argv[i], "--diagnostic") == 0) {
                config.diagnostic_mode = true;
            } else if (std::strcmp(argv[i], "--stress") == 0) {
                config.stress_mode = true;
            } else {
                config.interface = argv[i];
            }
        }

        return config;
    }

    std::string safety_mode_to_string() const {
        switch (safety_mode) {
            case SafetyMode::STRICT: return "STRICT";
            case SafetyMode::RELAXED: return "RELAXED";
            case SafetyMode::OFF: return "OFF";
            default: return "UNKNOWN";
        }
    }
};

} // namespace core
} // namespace wave_native
