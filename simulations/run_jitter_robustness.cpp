#include "simulation_common.hpp"

#include <iostream>

int main() {
    using namespace wnn_sim;
    std::ofstream out(data_path("jitter_robustness.csv"));
    out << "seed,node_count,loss_rate,mode,mean_phase_error,sync_failure_bool\n";
    out << std::fixed << std::setprecision(6);

    const std::vector<int> node_counts{10, 50, 100, 500};
    const std::vector<double> loss_rates{0.00, 0.02, 0.05, 0.10};

    for (uint32_t seed : seeds()) {
        std::mt19937 rng(seed);
        std::normal_distribution<double> jitter_noise(0.0, 0.01);
        std::uniform_real_distribution<double> bernoulli(0.0, 1.0);

        for (int node_count : node_counts) {
            for (double loss_rate : loss_rates) {
                for (const std::string mode : {"Baseline", "WNN"}) {
                    PairSimState s;
                    s.b.theta = 0.6;
                    double error_sum = 0.0;
                    int samples = 0;

                    for (int step = 0; step <= static_cast<int>(kRuntimeSec / kDt); ++step) {
                        const bool dropped = bernoulli(rng) < loss_rate;
                        const double network_jitter = jitter_noise(rng) * (1.0 + std::log10(static_cast<double>(node_count)));

                        if (!dropped) {
                            if (mode == "WNN") {
                                const double err = wrap_pi(s.b.theta - s.a.theta + network_jitter);
                                s.b.omega += (-0.05 * err - 0.01 * network_jitter) * kDt;
                            } else {
                                s.b.omega += 0.004 * network_jitter * kDt;
                            }
                            s.b.omega = std::clamp(s.b.omega, 0.8, 1.8);
                        }

                        rk4_pair_step(s, kDt);

                        const double err = std::abs(wrap_pi(s.b.theta - s.a.theta));
                        error_sum += err;
                        ++samples;
                    }

                    const double mean_error = error_sum / std::max(1, samples);
                    const bool failed = mean_error > (mode == "WNN" ? 0.40 : 0.65);
                    out << seed << ',' << node_count << ',' << loss_rate << ',' << mode << ',' << mean_error << ',' << (failed ? 1 : 0) << '\n';
                }
            }
        }
    }

    std::cout << "Wrote " << data_path("jitter_robustness.csv") << "\n";
    return 0;
}
