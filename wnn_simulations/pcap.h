#pragma once

#include <cstdint>
#include <sys/time.h>

using u_char = unsigned char;
using bpf_u_int32 = std::uint32_t;

struct pcap;
using pcap_t = pcap;

struct pcap_pkthdr {
    timeval ts;
    bpf_u_int32 caplen;
    bpf_u_int32 len;
};

#ifndef PCAP_ERRBUF_SIZE
#define PCAP_ERRBUF_SIZE 256
#endif
