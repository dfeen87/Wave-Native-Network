with open('src/interceptor/phy_listener.cpp', 'r') as f:
    content = f.read()

content = content.replace('''    if (raw_pcap == nullptr) {
        std::cerr << "Warning: Could not open device " << interface_name_ << " for live capture: " << errbuf << std::endl;
        std::cerr << "Running in mock/silent mode." << std::endl;
    pcap_handle_.reset(raw_pcap);
    } else {
        // Set non-blocking mode if desired, but here we run in a dedicated thread
        // using pcap_loop or pcap_dispatch
    }''', '''    if (raw_pcap == nullptr) {
        std::cerr << "Warning: Could not open device " << interface_name_ << " for live capture: " << errbuf << std::endl;
        std::cerr << "Running in mock/silent mode." << std::endl;
    } else {
        pcap_handle_.reset(raw_pcap);
        // Set non-blocking mode if desired, but here we run in a dedicated thread
        // using pcap_loop or pcap_dispatch
    }''')

with open('src/interceptor/phy_listener.cpp', 'w') as f:
    f.write(content)
