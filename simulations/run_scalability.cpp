#include "simulation_common.hpp"

#include <iostream>

int main() {
    using namespace wnn_sim;
    std::ofstream out(data_path("scalability.csv"));
    out << "node_count,mean_phase_error,sync_failure_probability,routing_stability_score\n";
    out << std::fixed << std::setprecision(6);

    const std::vector<int> node_counts{10, 50, 100, 500};

    for (int node_count : node_counts) {
        double mean_phase_error_acc = 0.0;
        double routing_stability_acc = 0.0;
        int failure_count = 0;

        for (uint32_t seed : seeds()) {
            std::mt19937 rng(seed);
            std::normal_distribution<double> disturbance(0.0, 0.02);
            PairSimState s;
            s.b.theta = 1.0;
            s.b.omega = 1.2 + 0.02 * std::log10(static_cast<double>(node_count));

            bool failed = false;
            double phase_error_sum = 0.0;
            double stability_score_sum = 0.0;
            int sample_count = 0;

            for (int step = 0; step <= static_cast<int>(kRuntimeSec / kDt); ++step) {
                const double load_scale = std::log10(static_cast<double>(node_count));
                const double noise = disturbance(rng) * load_scale;

                s.b.omega += (-0.06 * wrap_pi(s.b.theta - s.a.theta) - 0.004 * load_scale + noise) * kDt;
                s.b.omega = std::clamp(s.b.omega, 0.75, 1.75);

                rk4_pair_step(s, kDt);

                const double err = std::abs(wrap_pi(s.b.theta - s.a.theta));
                phase_error_sum += err;
                stability_score_sum += clamp01(1.0 - err / M_PI);
                ++sample_count;

                if (err > 1.55 && step > 3000) {
                    failed = true;
                }
            }

            mean_phase_error_acc += phase_error_sum / static_cast<double>(sample_count);
            routing_stability_acc += stability_score_sum / static_cast<double>(sample_count);
            if (failed) {
                ++failure_count;
            }
        }

        const double seed_count = static_cast<double>(kSeedCount);
        const double mean_phase_error = mean_phase_error_acc / seed_count;
        const double sync_failure_probability = static_cast<double>(failure_count) / seed_count;
        const double routing_stability_score = routing_stability_acc / seed_count;

        out << node_count << ',' << mean_phase_error << ',' << sync_failure_probability << ',' << routing_stability_score << '\n';
    }

    std::cout << "Wrote " << data_path("scalability.csv") << "\n";
    return 0;
}
