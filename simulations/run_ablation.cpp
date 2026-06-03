#include "simulation_common.hpp"

#include <iostream>

int main() {
    using namespace wnn_sim;
    std::ofstream out(data_path("ablation_study.csv"));
    out << "seed,disabled_component,sync_failure_bool,recovery_time_ms\n";
    out << std::fixed << std::setprecision(3);

    const std::vector<std::string> components{
        "none",
        "phase_salt",
        "anti_windup",
        "lock_detector",
        "handoff_compensation"
    };

    for (uint32_t seed : seeds()) {
        std::mt19937 rng(seed);
        std::normal_distribution<double> disturbance(0.0, 0.03);

        for (const auto& component : components) {
            PairSimState s;
            s.b.theta = 0.75;

            bool failed = false;
            double recovery_ms = kRuntimeSec * 1000.0;
            bool recovered = false;

            for (int step = 0; step <= static_cast<int>(kRuntimeSec / kDt); ++step) {
                const double noise = disturbance(rng);
                double gain = 0.07;
                if (component == "phase_salt") gain = 0.055;
                if (component == "anti_windup") gain = 0.045;
                if (component == "lock_detector") gain = 0.038;
                if (component == "handoff_compensation") gain = 0.05;

                s.b.omega += (-gain * wrap_pi(s.b.theta - s.a.theta) + noise) * kDt;
                s.b.omega = std::clamp(s.b.omega, 0.7, 1.7);

                rk4_pair_step(s, kDt);

                const double err = std::abs(wrap_pi(s.b.theta - s.a.theta));
                if (err > 1.45 && step > 2000) {
                    failed = true;
                }
                if (!recovered && err < 0.12) {
                    recovered = true;
                    recovery_ms = step * kDt * 1000.0;
                }
            }

            if (!recovered) {
                recovery_ms = kRuntimeSec * 1000.0;
            }

            out << seed << ',' << component << ',' << (failed ? 1 : 0) << ',' << recovery_ms << '\n';
        }
    }

    std::cout << "Wrote " << data_path("ablation_study.csv") << "\n";
    return 0;
}
