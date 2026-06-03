#include "simulation_common.hpp"

#include <iostream>

int main() {
    using namespace wnn_sim;
    std::ofstream out(data_path("phase_locking.csv"));
    out << "seed,mode,time_ms,phase_error_rad\n";
    out << std::fixed << std::setprecision(6);

    std::uniform_real_distribution<double> phase_mismatch(-1.6, 1.6);
    std::uniform_real_distribution<double> freq_jitter(-0.03, 0.03);

    for (uint32_t seed : seeds()) {
        std::mt19937 rng(seed);
        const double mismatch = phase_mismatch(rng);
        const double drift = freq_jitter(rng);

        for (const std::string mode : {"Baseline", "WNN"}) {
            PairSimState s;
            s.a.theta = 0.0;
            s.b.theta = mismatch;
            s.a.omega = 1.2;
            s.b.omega = 1.2 + drift;

            for (int step = 0; step <= static_cast<int>(kRuntimeSec / kDt); ++step) {
                const double t = step * kDt;
                if (mode == "WNN") {
                    const double err = wrap_pi(s.b.theta - s.a.theta);
                    s.b.omega += -0.08 * err * kDt;
                    s.b.omega = std::clamp(s.b.omega, 0.8, 1.6);
                }

                rk4_pair_step(s, kDt);

                if (step % 10 == 0) {
                    const double err = wrap_pi(s.b.theta - s.a.theta);
                    out << seed << ',' << mode << ',' << static_cast<int>(t * 1000.0) << ',' << err << '\n';
                }
            }
        }
    }

    std::cout << "Wrote " << data_path("phase_locking.csv") << "\n";
    return 0;
}
