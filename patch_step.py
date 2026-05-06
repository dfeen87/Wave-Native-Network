import re

with open('src/wave_state.hpp', 'r') as f:
    content = f.read()

# Update step signature
content = content.replace('void step(WaveState& state, double dt) {', 'void step(WaveState& state, long double dt) {')

# Replace RK4 block with Kahan block
old_rk4 = """        double t = state.ts;
        double x = state.A;
        double v = state.x_dot;
        double omega = state.omega;

        double omega_active = omega + generate_phase_salt(t, omega);

        double k1_x = f1(t, x, v);
        double k1_v = f2(t, x, v, omega_active);

        double k2_x = f1(t + 0.5 * dt, x + 0.5 * dt * k1_x, v + 0.5 * dt * k1_v);
        double k2_v = f2(t + 0.5 * dt, x + 0.5 * dt * k1_x, v + 0.5 * dt * k1_v, omega_active);

        double k3_x = f1(t + 0.5 * dt, x + 0.5 * dt * k2_x, v + 0.5 * dt * k2_v);
        double k3_v = f2(t + 0.5 * dt, x + 0.5 * dt * k2_x, v + 0.5 * dt * k2_v, omega_active);

        double k4_x = f1(t + dt, x + dt * k3_x, v + dt * k3_v);
        double k4_v = f2(t + dt, x + dt * k3_x, v + dt * k3_v, omega_active);

        state.A = x + (dt / 6.0) * (k1_x + 2.0 * k2_x + 2.0 * k3_x + k4_x);
        state.x_dot = v + (dt / 6.0) * (k1_v + 2.0 * k2_v + 2.0 * k3_v + k4_v);
        state.ts = t + dt;"""

new_rk4 = """        long double t = state.ts;
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

        state.ts = t + dt;"""

content = content.replace(old_rk4, new_rk4)

with open('src/wave_state.hpp', 'w') as f:
    f.write(content)
