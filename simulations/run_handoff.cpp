#include "simulation_common.hpp"

#include <iostream>

int main() {
    using namespace wnn_sim;
    std::ofstream out(data_path("handoff_continuity.csv"));
    out << "time_ms,mode,continuity_score,handoff_event_bool\n";
    out << std::fixed << std::setprecision(6);

    const int total_steps = static_cast<int>(kRuntimeSec / kDt);
    const int handoff_step = static_cast<int>(30.0 / kDt);

    for (const std::string mode : {"Baseline", "WNN"}) {
        std::vector<double> continuity_acc(total_steps + 1, 0.0);

        for (uint32_t seed : seeds()) {
            std::mt19937 rng(seed);
            std::normal_distribution<double> disturbance(0.0, 0.015);

            PairSimState s;
            s.b.theta = 0.4;
            double peer_a = 1.18;
            double peer_b = 1.22;

            for (int step = 0; step <= total_steps; ++step) {
                const bool handoff_event = step >= handoff_step;
                const double target_omega = handoff_event ? peer_b : peer_a;
                const double d = disturbance(rng);

                if (mode == "WNN") {
                    s.b.omega += (target_omega - s.b.omega) * 0.08 + (-0.06 * wrap_pi(s.b.theta - s.a.theta) + d) * kDt;
                } else {
                    s.b.omega += (target_omega - s.b.omega) * 0.02 + 2.5 * d * kDt;
                }
                s.b.omega = std::clamp(s.b.omega, 0.8, 1.6);

                rk4_pair_step(s, kDt);

                const double err = std::abs(wrap_pi(s.b.theta - s.a.theta));
                const double continuity = clamp01(1.0 - err / M_PI);
                continuity_acc[step] += continuity;
            }
        }

        for (int step = 0; step <= total_steps; step += 10) {
            const double avg = continuity_acc[step] / static_cast<double>(kSeedCount);
            const bool handoff_event = (step == handoff_step);
            out << static_cast<int>(step * kDt * 1000.0) << ',' << mode << ',' << avg << ',' << (handoff_event ? 1 : 0) << '\n';
        }
    }

    std::cout << "Wrote " << data_path("handoff_continuity.csv") << "\n";
    return 0;
}
