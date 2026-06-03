#include "simulation_common.hpp"

#include <iostream>

int main() {
    using namespace wnn_sim;
    std::ofstream out(data_path("scalability.csv"));
    out << "seed,node_count,convergence_time_ms,failure_observed\n";
    out << std::fixed << std::setprecision(3);

    const std::vector<int> node_counts{10, 50, 100, 500};

    for (uint32_t seed : seeds()) {
        std::mt19937 rng(seed);
        std::normal_distribution<double> disturbance(0.0, 0.02);

        for (int node_count : node_counts) {
            PairSimState s;
            s.b.theta = 1.0;
            s.b.omega = 1.2 + 0.02 * std::log10(static_cast<double>(node_count));

            bool failed = false;
            double convergence_ms = kRuntimeSec * 1000.0;
            int stable_count = 0;

            for (int step = 0; step <= static_cast<int>(kRuntimeSec / kDt); ++step) {
                const double load_scale = std::log10(static_cast<double>(node_count));
                const double noise = disturbance(rng) * load_scale;

                s.b.omega += (-0.06 * wrap_pi(s.b.theta - s.a.theta) - 0.004 * load_scale + noise) * kDt;
                s.b.omega = std::clamp(s.b.omega, 0.75, 1.75);

                rk4_pair_step(s, kDt);

                const double err = std::abs(wrap_pi(s.b.theta - s.a.theta));
                if (err < 0.12) {
                    ++stable_count;
                } else {
                    stable_count = 0;
                }

                if (stable_count >= 300 && convergence_ms >= kRuntimeSec * 1000.0) {
                    convergence_ms = step * kDt * 1000.0;
                }

                if (err > 1.55 && step > 3000) {
                    failed = true;
                }
            }

            out << seed << ',' << node_count << ',' << convergence_ms << ',' << (failed ? 1 : 0) << '\n';
        }
    }

    std::cout << "Wrote " << data_path("scalability.csv") << "\n";
    return 0;
}
