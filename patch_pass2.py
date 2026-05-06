with open('src/interceptor/phy_listener.hpp', 'r') as f:
    content = f.read()

content = content.replace('pcap_t* pcap_handle_;', 'std::unique_ptr<pcap_t, void(*)(pcap_t*)> pcap_handle_;')
content = content.replace('PhyListener(const std::string& interface_name);', 'PhyListener(const std::string& interface_name);\n    void flush_delay_queue();')

with open('src/interceptor/phy_listener.hpp', 'w') as f:
    f.write(content)

with open('src/interceptor/phy_listener.cpp', 'r') as f:
    content = f.read()

content = content.replace('pcap_handle_(nullptr)', 'pcap_handle_(nullptr, [](pcap_t* p){ if (p) pcap_close(p); })')

content = content.replace('pcap_handle_ = pcap_open_live(', 'pcap_t* raw_pcap = pcap_open_live(')
content = content.replace('if (pcap_handle_ == nullptr) {', 'if (raw_pcap == nullptr) {')
content = content.replace('pcap_handle_ = nullptr;', '')
content = content.replace('} else {\n        // Set non-blocking mode if desired', 'pcap_handle_.reset(raw_pcap);\n    } else {\n        // Set non-blocking mode if desired')

content = content.replace('if (pcap_handle_) {', 'if (pcap_handle_) {') # Keep matching existing usage, it works with unique_ptr
content = content.replace('pcap_breakloop(pcap_handle_);', 'pcap_breakloop(pcap_handle_.get());')
content = content.replace('pcap_close(pcap_handle_);', '') # Unique_ptr handles this
content = content.replace('pcap_dispatch(pcap_handle_', 'pcap_dispatch(pcap_handle_.get()')
content = content.replace('pcap_inject(pcap_handle_', 'pcap_inject(pcap_handle_.get()')

content += """
void PhyListener::flush_delay_queue() {
    std::lock_guard<std::mutex> lock(injector_mutex_);
    delay_queue_.clear();
}
"""

with open('src/interceptor/phy_listener.cpp', 'w') as f:
    f.write(content)

with open('src/routing_engine.hpp', 'r') as f:
    content = f.read()

content = content.replace('void set_transduction_allowed(bool allowed) { transduction_allowed_ = allowed; }', 'void set_transduction_allowed(bool allowed) { transduction_allowed_ = allowed; }\n    void flush_transduction_queue() {\n        if (phy_listener_) phy_listener_->flush_delay_queue();\n    }')

with open('src/routing_engine.hpp', 'w') as f:
    f.write(content)

with open('src/safety_guardrails.cpp', 'r') as f:
    content = f.read()

content = content.replace('routing_engine_->set_transduction_allowed(false);', 'routing_engine_->set_transduction_allowed(false);\n        routing_engine_->flush_transduction_queue();')

with open('src/safety_guardrails.cpp', 'w') as f:
    f.write(content)
