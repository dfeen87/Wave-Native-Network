#pragma once

#include <cmath>

namespace wave_native {
namespace core {

struct WaveState {
    double A;   // Amplitude (x)
    double theta; // Phase (not strictly part of standard duffing state, but kept for signature)
    double omega; // Forcing frequency
    double x_dot; // Velocity
    double ts;  // Time

    WaveState() : A(0.0), theta(0.0), omega(1.2), x_dot(0.0), ts(0.0) {}
    WaveState(double a, double th, double om, double xd, double t)
        : A(a), theta(th), omega(om), x_dot(xd), ts(t) {}
};

class DuffingOscillator {
public:
    DuffingOscillator(double damping = 0.3, double alpha = -1.0, double beta = 1.0, double gamma = 0.5)
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

    double f1(double /*t*/, double /*x*/, double v) const {
        return v;
    }

    double f2(double t, double x, double v, double omega) const {
        return -delta_ * v - alpha_ * x - beta_ * x * x * x + gamma_ * std::cos(omega * t);
    }

    void step(WaveState& state, double dt) {
        double t = state.ts;
        double x = state.A;
        double v = state.x_dot;
        double omega = state.omega;

        double k1_x = f1(t, x, v);
        double k1_v = f2(t, x, v, omega);

        double k2_x = f1(t + 0.5 * dt, x + 0.5 * dt * k1_x, v + 0.5 * dt * k1_v);
        double k2_v = f2(t + 0.5 * dt, x + 0.5 * dt * k1_x, v + 0.5 * dt * k1_v, omega);

        double k3_x = f1(t + 0.5 * dt, x + 0.5 * dt * k2_x, v + 0.5 * dt * k2_v);
        double k3_v = f2(t + 0.5 * dt, x + 0.5 * dt * k2_x, v + 0.5 * dt * k2_v, omega);

        double k4_x = f1(t + dt, x + dt * k3_x, v + dt * k3_v);
        double k4_v = f2(t + dt, x + dt * k3_x, v + dt * k3_v, omega);

        state.A = x + (dt / 6.0) * (k1_x + 2.0 * k2_x + 2.0 * k3_x + k4_x);
        state.x_dot = v + (dt / 6.0) * (k1_v + 2.0 * k2_v + 2.0 * k3_v + k4_v);
        state.ts = t + dt;

        // Approximate phase, simplistic update based on angular velocity.
        // In a true system, phase might be extracted via Hilbert transform,
        // but for now we maintain it as a simple accumulator.
        state.theta = std::fmod(state.theta + omega * dt, 2.0 * M_PI);
    }

private:
    double delta_;
    double alpha_;
    double beta_;
    double gamma_;
    double original_gamma_;
    bool ambient_mode_ = false;
};

} // namespace core
} // namespace wave_native
