import re

with open('src/wave_state.hpp', 'r') as f:
    content = f.read()

# Change double to long double in WaveState
content = re.sub(r'struct WaveState \{(.*?)\};',
    lambda m: 'struct WaveState {' + m.group(1).replace('double A;', 'long double A;').replace('double theta;', 'long double theta;').replace('double omega;', 'long double omega;').replace('double x_dot;', 'long double x_dot;').replace('double ts;', 'long double ts;') + '};',
    content, flags=re.DOTALL)

# update constructor
content = content.replace('WaveState() : A(0.0), theta(0.0), omega(1.2), x_dot(0.0), ts(0.0) {}', 'WaveState() : A(0.0L), theta(0.0L), omega(1.2L), x_dot(0.0L), ts(0.0L) {}')
content = content.replace('WaveState(double a, double th, double om, double xd, double t)', 'WaveState(long double a, long double th, long double om, long double xd, long double t)')

# DuffingOscillator parameters
content = content.replace('DuffingOscillator(double damping = 0.3, double alpha = -1.0, double beta = 1.0, double gamma = 0.5)', 'DuffingOscillator(long double damping = 0.3L, long double alpha = -1.0L, long double beta = 1.0L, long double gamma = 0.5L)')

content = content.replace('double delta_;', 'long double delta_;')
content = content.replace('double alpha_;', 'long double alpha_;')
content = content.replace('double beta_;', 'long double beta_;')
content = content.replace('double gamma_;', 'long double gamma_;')
content = content.replace('double original_gamma_;', 'long double original_gamma_;')

# Kahan summation variables
content = content.replace('bool ambient_mode_ = false;', 'bool ambient_mode_ = false;\n    long double c_A_ = 0.0L;\n    long double c_v_ = 0.0L;')

# Update f1 and f2
content = content.replace('double f1(double /*t*/, double /*x*/, double v) const', 'long double f1(long double /*t*/, long double /*x*/, long double v) const')
content = content.replace('double f2(double t, double x, double v, double omega) const', 'long double f2(long double t, long double x, long double v, long double omega) const')

# inside f2 math
content = content.replace('return -delta_ * v - alpha_ * x - beta_ * x * x * x + gamma_ * std::cos(omega * t);', 'return -delta_ * v - alpha_ * x - beta_ * x * x * x + gamma_ * std::cos((double)(omega * t));')

with open('src/wave_state.hpp', 'w') as f:
    f.write(content)
