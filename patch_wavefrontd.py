with open('src/wavefrontd.cpp', 'r') as f:
    content = f.read()

# Update wavefrontd.cpp variables to long double
content = content.replace('double omega = 1.2;', 'long double omega = 1.2L;')
content = content.replace('double alpha = -1.0;', 'long double alpha = -1.0L;')
content = content.replace('double beta = 1.0;', 'long double beta = 1.0L;')
content = content.replace('double delta = 0.3;', 'long double delta = 0.3L;')
content = content.replace('const double dt = 0.01;', 'const long double dt = 0.01L;')

# Update integration loop strictly deriving state.ts from monotonic clock
old_loop_setup = """    auto last_tick = std::chrono::steady_clock::now();
    uint64_t tick_count = 0;

    std::atomic<double> shared_ts{0.0};"""

new_loop_setup = """    auto start_time = std::chrono::steady_clock::now();
    auto last_tick = start_time;
    uint64_t tick_count = 0;

    std::atomic<double> shared_ts{0.0};"""

content = content.replace(old_loop_setup, new_loop_setup)

old_loop_check = """        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - last_tick;

        if (elapsed.count() >= dt) {"""

new_loop_check = """        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<long double> elapsed = now - last_tick;

        if (elapsed.count() >= dt) {"""

content = content.replace(old_loop_check, new_loop_check)

old_time_update = """            last_tick = now;
            tick_count++;"""

new_time_update = """            tick_count++;
            last_tick = start_time + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<long double>(tick_count * dt));
            state.ts = tick_count * dt; // Strict temporal hardening derived from monotonic clock without jitter"""

content = content.replace(old_time_update, new_time_update)

# Also fix the `double phys_amp = 0.0;` if needed, keep it double is fine but let's change perturbation to long double
content = content.replace('double phys_amp = 0.0;', 'long double phys_amp = 0.0L;')

with open('src/wavefrontd.cpp', 'w') as f:
    f.write(content)
