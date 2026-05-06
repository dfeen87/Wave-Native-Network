with open('src/interceptor/phy_listener.cpp', 'r') as f:
    content = f.read()

content = content.replace('listener_thread_ = std::jthread(&PhyListener::listener_loop, this);', 'listener_thread_ = std::jthread([this](std::stop_token st) { listener_loop(st); });')
content = content.replace('injector_thread_ = std::jthread(&PhyListener::injector_loop, this);', 'injector_thread_ = std::jthread([this](std::stop_token st) { injector_loop(st); });')

with open('src/interceptor/phy_listener.cpp', 'w') as f:
    f.write(content)

with open('src/calibration.cpp', 'r') as f:
    content = f.read()

content = content.replace('(double)(1.0 / (1.0 + std::abs(state.x_dot)))', 'static_cast<double>(1.0 / (1.0 + std::abs(state.x_dot)))')
content = content.replace('(double)(1.0 / (1.0 + std::abs(state.A)))', 'static_cast<double>(1.0 / (1.0 + std::abs(state.A)))')

with open('src/calibration.cpp', 'w') as f:
    f.write(content)
