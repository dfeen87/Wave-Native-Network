#pragma once

#include <cmath>
#include <cstring>
#include <functional>
#include <cstdint>

namespace wave_native {
namespace core {

inline double generate_phase_salt(double ts, double omega) {
    uint64_t epoch = static_cast<uint64_t>(std::floor(ts / 2.5));
    uint64_t w_bits;
    std::memcpy(&w_bits, &omega, sizeof(omega));
    uint64_t hash = std::hash<uint64_t>{}(epoch ^ w_bits);
    return (hash % 1000) * 1e-5;
}

struct WaveState {
    long double A;   // Amplitude (x)
    long double theta; // Phase (not strictly part of standard duffing state, but kept for signature)
    long double omega; // Forcing frequency
    long double x_dot; // Velocity
    long double ts;  // Time

    WaveState() : A(0.0L), theta(0.0L), omega(1.2L), x_dot(0.0L), ts(0.0L) {}
    WaveState(long double a, long double th, long double om, long double xd, long double t)
        : A(a), theta(th), omega(om), x_dot(xd), ts(t) {}
};

class DuffingOscillator {
public:
    DuffingOscillator(long double damping = 0.3L, long double alpha = -1.0L, long double beta = 1.0L, long double gamma = 0.5L)
        : delta_(damping), alpha_(alpha), beta_(beta), gamma_(gamma), original_gamma_(gamma) {}

    void set_ambient_mode(bool ambient) {
        ambient_mode_ = ambient;
        if (ambient_mode_) {
            gamma_ = original_gamma_ * 0.01; // Scale down heavily for Low-Energy Ambient Mode
        } else {
            gamma_ = original_gamma_;
        }
    }

    // Duffing equation: x'' + delta * x' + alpha * x + beta * x^3 = gamma * cos(omega * t)
    // Here: x'' = -delta * x' - alpha * x - beta * x^3 + gamma * cos(omega * t)
    // As a system of first-order ODEs:
    // dx/dt = v
    // dv/dt = -delta * v - alpha * x - beta * x^3 + gamma * cos(omega * t)

    long double f1(long double /*t*/, long double /*x*/, long double v) const {
        return v;
    }

    long double f2(long double t, long double x, long double v, long double omega) const {
        return -delta_ * v - alpha_ * x - beta_ * x * x * x + gamma_ * std::cos((double)(omega * t));
    }

    void step(WaveState& state, long double dt) {
        long double t = state.ts;
        long double x = state.A;
        long double v = state.x_dot;
        long double omega = state.omega;

        long double omega_active = omega + generate_phase_salt(t, omega);

        long double k1_x = f1(t, x, v);
        long double k1_v = f2(t, x, v, omega_active);

        long double k2_x = f1(t + 0.5L * dt, x + 0.5L * dt * k1_x, v + 0.5L * dt * k1_v);
        long double k2_v = f2(t + 0.5L * dt, x + 0.5L * dt * k1_x, v + 0.5L * dt * k1_v, omega_active);

        long double k3_x = f1(t + 0.5L * dt, x + 0.5L * dt * k2_x, v + 0.5L * dt * k2_v);
        long double k3_v = f2(t + 0.5L * dt, x + 0.5L * dt * k2_x, v + 0.5L * dt * k2_v, omega_active);

        long double k4_x = f1(t + dt, x + dt * k3_x, v + dt * k3_v);
        long double k4_v = f2(t + dt, x + dt * k3_x, v + dt * k3_v, omega_active);

        long double update_x = (dt / 6.0L) * (k1_x + 2.0L * k2_x + 2.0L * k3_x + k4_x);
        long double update_v = (dt / 6.0L) * (k1_v + 2.0L * k2_v + 2.0L * k3_v + k4_v);

        // Kahan summation for A
        long double y_A = update_x - c_A_;
        long double t_A = state.A + y_A;
        c_A_ = (t_A - state.A) - y_A;
        state.A = t_A;

        // Kahan summation for x_dot
        long double y_v = update_v - c_v_;
        long double t_v = state.x_dot + y_v;
        c_v_ = (t_v - state.x_dot) - y_v;
        state.x_dot = t_v;

        state.ts = t + dt;

        // Approximate phase, simplistic update based on angular velocity.
        // In a true system, phase might be extracted via Hilbert transform,
        // but for now we maintain it as a simple accumulator.
        state.theta = std::fmod(state.theta + omega_active * dt, 2.0 * M_PI);
    }

private:
    long double delta_;
    long double alpha_;
    long double beta_;
    long double gamma_;
    long double original_gamma_;
    bool ambient_mode_ = false;
    long double c_A_ = 0.0L;
    long double c_v_ = 0.0L;
};

} // namespace core
} // namespace wave_native
