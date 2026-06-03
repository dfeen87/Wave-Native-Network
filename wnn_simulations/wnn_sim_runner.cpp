#include "../src/wave_state.hpp"
#include "../src/routing_engine.hpp"
#include "../src/ambient_verifier.hpp"
#include "../src/pll_controller.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

using wave_native::core::AmbientVerifier;
using wave_native::core::DuffingOscillator;
using wave_native::core::PllController;
using wave_native::core::WaveState;
using wave_native::core::generate_phase_salt;

constexpr double kTwoPi = 2.0 * M_PI;
constexpr double kDt = 0.002;
constexpr int kSteps = 2400;
constexpr int kStepMs = 2;

enum class SimMode {
    Baseline,
    Overlay,
    Handoff,
    Replay,
    Ablation
};

struct Node {
    int id{};
    WaveState psi{};
    DuffingOscillator oscillator{};
    PllController pll{1.2};
    AmbientVerifier verifier{};
};

struct SimStats {
    std::uint64_t total_dropped = 0;
    std::vector<int> failure_timestamps_ms;
};

struct PacketRecord {
    std::uint64_t packet_id = 0;
    double ts = 0.0;
    double theta = 0.0;
};

std::string mode_to_string(SimMode mode) {
    switch (mode) {
        case SimMode::Baseline: return "baseline";
        case SimMode::Overlay: return "wnn_overlay";
        case SimMode::Handoff: return "handoff";
        case SimMode::Replay: return "replay_attack";
        case SimMode::Ablation: return "ablation_no_delta_phi";
    }
    return "unknown";
}

double wrap_phase(double x) {
    while (x > M_PI) x -= kTwoPi;
    while (x < -M_PI) x += kTwoPi;
    return x;
}

double hybrid_cost(const Node& a, const Node& b, double loss_rate, bool delta_phi_enabled) {
    const double w1 = 0.15;
    const double w2 = 0.15;
    const double w3 = 0.30;
    const double w4 = 0.20;
    const double w5 = delta_phi_enabled ? 0.20 : 0.0;

    const double L_ij = std::clamp(loss_rate + (std::abs(a.id - b.id) * 0.001), 0.0, 1.0);
    const double J_ij = std::clamp(std::abs(a.psi.x_dot - b.psi.x_dot) * 0.05, 0.0, 1.0);
    const double dtheta = std::abs(wrap_phase(a.psi.theta - b.psi.theta));
    const double domega = std::abs(a.psi.omega - b.psi.omega);
    const double dphi = std::abs(a.psi.A - b.psi.A) + std::abs(a.psi.x_dot - b.psi.x_dot);

    return (w1 * L_ij) + (w2 * J_ij) + (w3 * dtheta) + (w4 * domega) + (w5 * dphi);
}

void initialize_logs(const std::string& out_dir) {
    std::ofstream convergence(out_dir + "/convergence_log.csv", std::ios::trunc);
    convergence << "timestamp_ms,node_i_id,node_j_id,phase_diff_rads,freq_diff_hz\n";

    std::ofstream security(out_dir + "/security_log.csv", std::ios::trunc);
    security << "timestamp_ms,packet_id,is_replay_attack,delta_phi_score,action_taken\n";

    std::ofstream handoff(out_dir + "/handoff_log.csv", std::ios::trunc);
    handoff << "timestamp_ms,mobile_node_id,active_peer_id,continuity_score,connection_status\n";

    std::ofstream scalability(out_dir + "/scalability_log.csv", std::ios::trunc);
    scalability << "node_count,loss_rate,mode,mean_time_between_sync_failures,total_dropped\n";
}

std::vector<double> make_phase_stream(double theta_ref, double omega_ref, int samples = 32) {
    std::vector<double> stream;
    stream.reserve(static_cast<size_t>(samples));
    for (int i = 0; i < samples; ++i) {
        const double t = (static_cast<double>(i) * kDt) / static_cast<double>(samples);
        stream.push_back(std::sin(theta_ref + omega_ref * t));
    }
    return stream;
}

double mean_time_between_failures(const std::vector<int>& failures) {
    if (failures.size() < 2) return static_cast<double>(kSteps * kStepMs);
    double sum = 0.0;
    for (size_t i = 1; i < failures.size(); ++i) {
        sum += static_cast<double>(failures[i] - failures[i - 1]);
    }
    return sum / static_cast<double>(failures.size() - 1);
}

void run_mode(
    int node_count,
    double loss_rate,
    SimMode mode,
    std::ofstream& convergence_log,
    std::ofstream& security_log,
    std::ofstream& handoff_log,
    std::ofstream& scalability_log,
    std::mt19937& rng
) {
    std::uniform_real_distribution<double> unit01(0.0, 1.0);

    std::vector<Node> nodes(static_cast<size_t>(node_count));
    for (int i = 0; i < node_count; ++i) {
        nodes[static_cast<size_t>(i)].id = i;
        nodes[static_cast<size_t>(i)].psi = WaveState(
            0.05 * std::sin(0.1 * i),
            std::fmod(0.03 * i, kTwoPi),
            1.2 + 0.0005 * i,
            0.0,
            0.0
        );
    }

    SimStats stats{};
    std::deque<PacketRecord> replay_history;
    std::uint64_t packet_counter = 0;

    const int mobile_id = node_count / 2;
    const int peer_a = 0;
    const int peer_b = node_count - 1;
    int active_peer = peer_a;

    for (int step = 0; step < kSteps; ++step) {
        const int timestamp_ms = step * kStepMs;
        const bool coherence_enabled = (mode != SimMode::Baseline);
        const bool delta_phi_enabled = (mode != SimMode::Ablation);

        if (mode == SimMode::Handoff && step == (kSteps / 2)) {
            active_peer = peer_b;
            const double continuity = 1.0 - std::abs(wrap_phase(
                nodes[static_cast<size_t>(mobile_id)].psi.theta -
                nodes[static_cast<size_t>(active_peer)].psi.theta));
            handoff_log << timestamp_ms << "," << mobile_id << "," << active_peer << ","
                        << std::fixed << std::setprecision(6) << continuity
                        << ",connected\n";
        }

        for (int i = 0; i < node_count; ++i) {
            auto& node = nodes[static_cast<size_t>(i)];
            node.oscillator.step(node.psi, kDt);

            if (coherence_enabled && i > 0) {
                int ref = i - 1;
                if (mode == SimMode::Handoff && i == mobile_id) ref = active_peer;

                auto stream = make_phase_stream(
                    nodes[static_cast<size_t>(ref)].psi.theta,
                    nodes[static_cast<size_t>(ref)].psi.omega
                );
                const double omega_corrected = node.pll.step(
                    stream,
                    node.psi.theta,
                    kDt,
                    node.psi.ts
                );
                node.psi.omega = omega_corrected;
                node.verifier.analyze_signal_entropy(stream);
            }
        }

        for (int i = 0; i + 1 < node_count; ++i) {
            auto& ni = nodes[static_cast<size_t>(i)];
            auto& nj = nodes[static_cast<size_t>(i + 1)];

            const double dtheta = std::abs(wrap_phase(ni.psi.theta - nj.psi.theta));
            const double domega = std::abs(ni.psi.omega - nj.psi.omega);
            const double route_cost = hybrid_cost(ni, nj, loss_rate, delta_phi_enabled);

            if (step % 120 == 0) {
                convergence_log << timestamp_ms << "," << i << "," << (i + 1) << ","
                                << std::fixed << std::setprecision(8) << dtheta << ","
                                << domega << "\n";
            }

            if (dtheta > 1.20 || route_cost > 1.10) {
                stats.failure_timestamps_ms.push_back(timestamp_ms);
            }

            if (unit01(rng) < loss_rate) {
                ++stats.total_dropped;
            }
        }

        if (mode == SimMode::Replay) {
            ++packet_counter;
            replay_history.push_back(PacketRecord{
                packet_counter,
                nodes.front().psi.ts,
                nodes.front().psi.theta
            });
            if (replay_history.size() > 240) replay_history.pop_front();

            if (step > 240 && (step % 240 == 0) && !replay_history.empty()) {
                const PacketRecord delayed = replay_history.front();
                const double current_salt = generate_phase_salt(nodes.front().psi.ts, nodes.front().psi.omega);
                const double delayed_salt = generate_phase_salt(delayed.ts, nodes.front().psi.omega);
                const double delta_phi = std::abs(current_salt - delayed_salt) +
                                         std::abs(wrap_phase(nodes.front().psi.theta - delayed.theta));
                const bool replay_attack = delta_phi > 0.0010;
                const char* action = replay_attack ? "reject_phase_salt_mismatch" : "accept";

                if (replay_attack) {
                    ++stats.total_dropped;
                }

                security_log << timestamp_ms << "," << delayed.packet_id << ","
                             << (replay_attack ? "true" : "false") << ","
                             << std::fixed << std::setprecision(8) << delta_phi << ","
                             << action << "\n";
            }
        }

        if (mode == SimMode::Handoff && step % 180 == 0) {
            const double continuity = 1.0 - std::abs(wrap_phase(
                nodes[static_cast<size_t>(mobile_id)].psi.theta -
                nodes[static_cast<size_t>(active_peer)].psi.theta));
            handoff_log << timestamp_ms << "," << mobile_id << "," << active_peer << ","
                        << std::fixed << std::setprecision(6) << continuity
                        << ",connected\n";
        }
    }

    const double mtbsf = mean_time_between_failures(stats.failure_timestamps_ms);
    scalability_log << node_count << ","
                    << std::fixed << std::setprecision(2) << loss_rate << ","
                    << mode_to_string(mode) << ","
                    << std::fixed << std::setprecision(3) << mtbsf << ","
                    << stats.total_dropped << "\n";
}

} // namespace

int main() {
    const std::string out_dir = WNN_SIM_OUTPUT_DIR;
    initialize_logs(out_dir);

    std::ofstream convergence_log(out_dir + "/convergence_log.csv", std::ios::app);
    std::ofstream security_log(out_dir + "/security_log.csv", std::ios::app);
    std::ofstream handoff_log(out_dir + "/handoff_log.csv", std::ios::app);
    std::ofstream scalability_log(out_dir + "/scalability_log.csv", std::ios::app);

    std::mt19937 rng(20260603u);

    const std::array<int, 3> node_counts = {10, 50, 100};
    const std::array<double, 4> loss_rates = {0.0, 0.05, 0.10, 0.25};
    const std::array<SimMode, 5> modes = {
        SimMode::Baseline,
        SimMode::Overlay,
        SimMode::Handoff,
        SimMode::Replay,
        SimMode::Ablation
    };

    for (int n : node_counts) {
        for (double loss : loss_rates) {
            for (SimMode mode : modes) {
                run_mode(
                    n,
                    loss,
                    mode,
                    convergence_log,
                    security_log,
                    handoff_log,
                    scalability_log,
                    rng
                );
            }
        }
    }

    return 0;
}
