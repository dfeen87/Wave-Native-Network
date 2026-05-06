with open('CMakeLists.txt', 'r') as f:
    content = f.read()

content = content.replace('set(CMAKE_CXX_STANDARD 17)', 'set(CMAKE_CXX_STANDARD 20)')
content = content.replace('set(CMAKE_CXX_STANDARD_REQUIRED ON)', 'set(CMAKE_CXX_STANDARD_REQUIRED ON)\nset(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native -flto")')

with open('CMakeLists.txt', 'w') as f:
    f.write(content)

with open('src/wavefrontd.cpp', 'r') as f:
    content = f.read()

content = content.replace('std::thread pruning_thread([&peer_table, &shared_ts]() {', 'std::jthread pruning_thread([&peer_table, &shared_ts](std::stop_token stoken) {')
content = content.replace('while (global_running) {\n            peer_table.prune_decayed_peers(shared_ts.load());', 'while (!stoken.stop_requested() && global_running) {\n            peer_table.prune_decayed_peers(shared_ts.load());')
content = content.replace('std::thread spectral_thread([&]() {', 'std::jthread spectral_thread([&](std::stop_token stoken) {')
content = content.replace('while (global_running) {\n            std::vector<double> local_stream;', 'while (!stoken.stop_requested() && global_running) {\n            std::vector<double> local_stream;')
content = content.replace('stream_cv.wait_for(lock, std::chrono::milliseconds(100), [&]{ return new_stream_ready || !global_running; });', 'stream_cv.wait_for(lock, std::chrono::milliseconds(100), [&]{ return new_stream_ready || !global_running || stoken.stop_requested(); });')
content = content.replace('if (!global_running) break;', 'if (!global_running || stoken.stop_requested()) break;')

content = content.replace('if (pruning_thread.joinable()) {\n        pruning_thread.join();\n    }', '')
content = content.replace('if (spectral_thread.joinable()) {\n        spectral_thread.join();\n    }', '')

with open('src/wavefrontd.cpp', 'w') as f:
    f.write(content)

with open('src/interceptor/phy_listener.hpp', 'r') as f:
    content = f.read()

content = content.replace('std::thread listener_thread_;', 'std::jthread listener_thread_;')
content = content.replace('std::thread injector_thread_;', 'std::jthread injector_thread_;')

with open('src/interceptor/phy_listener.hpp', 'w') as f:
    f.write(content)

with open('src/interceptor/phy_listener.cpp', 'r') as f:
    content = f.read()

content = content.replace('void PhyListener::listener_loop() {', 'void PhyListener::listener_loop(std::stop_token stoken) {')
content = content.replace('void PhyListener::injector_loop() {', 'void PhyListener::injector_loop(std::stop_token stoken) {')
content = content.replace('if (listener_thread_.joinable()) {\n        listener_thread_.join();\n    }', '')
content = content.replace('if (injector_thread_.joinable()) {\n        injector_thread_.join();\n    }', '')
content = content.replace('while (running_) {', 'while (running_ && !stoken.stop_requested()) {')
content = content.replace('injector_cv_.wait(lock, [this]() { return !delay_queue_.empty() || !running_; });', 'injector_cv_.wait(lock, [this, &stoken]() { return !delay_queue_.empty() || !running_ || stoken.stop_requested(); });')
content = content.replace('if (!running_) break;', 'if (!running_ || stoken.stop_requested()) break;')

with open('src/interceptor/phy_listener.cpp', 'w') as f:
    f.write(content)
