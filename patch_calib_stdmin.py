with open('src/calibration.cpp', 'r') as f:
    content = f.read()

content = content.replace('std::min(1.0, 1.0 / (1.0 + std::abs(state.x_dot)))', 'std::min(1.0, (double)(1.0 / (1.0 + std::abs(state.x_dot))))')
content = content.replace('std::min(1.0, 1.0 / (1.0 + std::abs(state.A)))', 'std::min(1.0, (double)(1.0 / (1.0 + std::abs(state.A))))')

with open('src/calibration.cpp', 'w') as f:
    f.write(content)
