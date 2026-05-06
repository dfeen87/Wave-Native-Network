with open('src/interceptor/phy_listener.hpp', 'r') as f:
    content = f.read()

content = content.replace('void listener_loop();', 'void listener_loop(std::stop_token stoken);')
content = content.replace('void injector_loop();', 'void injector_loop(std::stop_token stoken);')

with open('src/interceptor/phy_listener.hpp', 'w') as f:
    f.write(content)

with open('src/interceptor/phy_listener.cpp', 'r') as f:
    content = f.read()

content = content.replace('listener_thread_ = std::thread(', 'listener_thread_ = std::jthread(')
content = content.replace('injector_thread_ = std::thread(', 'injector_thread_ = std::jthread(')

with open('src/interceptor/phy_listener.cpp', 'w') as f:
    f.write(content)
