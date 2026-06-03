#include "simulation_common.hpp"

#include <iostream>

int main() {
    using namespace wnn_sim;
    std::ofstream out(data_path("replay_attack.csv"));
    out << "time_ms,packet_delay_ms,normalized_delta_phi,is_replay,detected_by_threshold\n";
    out << std::fixed << std::setprecision(6);

    constexpr double threshold = 0.62;

    for (uint32_t seed : seeds()) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> delay_dist(5.0, 180.0);
        std::normal_distribution<double> noise(0.0, 0.02);

        PairSimState s;
        s.b.theta = 0.1;

        for (int step = 0; step <= static_cast<int>(kRuntimeSec / kDt); ++step) {
            const double t = step * kDt;
            const bool replay = (t >= 20.0 && t <= 45.0 && (step % 50 == 0));
            const double delay_ms = replay ? delay_dist(rng) : delay_dist(rng) * 0.05;

            s.b.omega += (-0.03 * wrap_pi(s.b.theta - s.a.theta) + noise(rng)) * kDt;
            if (replay) {
                s.b.theta += 0.25 + (delay_ms / 180.0) * 0.45;
            }

            rk4_pair_step(s, kDt);

            if (step % 10 == 0) {
                const double delta = std::abs(wrap_pi(s.b.theta - s.a.theta));
                const double normalized = clamp01(delta / M_PI);
                const bool detected = normalized >= threshold;
                out << static_cast<int>(t * 1000.0) << ',' << delay_ms << ',' << normalized << ',' << (replay ? 1 : 0) << ',' << (detected ? 1 : 0) << '\n';
            }
        }
    }

    std::cout << "Wrote " << data_path("replay_attack.csv") << "\n";
    return 0;
}
