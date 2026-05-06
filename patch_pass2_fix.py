with open('src/interceptor/phy_listener.cpp', 'r') as f:
    content = f.read()

content = content.replace('void PhyListener::flush_delay_queue() {', 'namespace wave_native {\nnamespace interceptor {\n\nvoid PhyListener::flush_delay_queue() {')
content = content + '\n} // namespace interceptor\n} // namespace wave_native\n'

with open('src/interceptor/phy_listener.cpp', 'w') as f:
    f.write(content)
