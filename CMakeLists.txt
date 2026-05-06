cmake_minimum_required(VERSION 3.10)
project(WaveNativeProxy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native -flto")

find_package(Threads REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/mesh_legacy
)

add_executable(wavefrontd
    src/wavefrontd.cpp
    src/interceptor/phy_listener.cpp
    src/ambient_verifier.cpp
    src/calibration.cpp
    src/pll_controller.cpp
    src/routing_engine.cpp
    src/safety_guardrails.cpp
    mesh_legacy/mesh_discovery.cpp
    mesh_legacy/peer_routing.cpp
    mesh_legacy/zk_verifier.cpp
)

target_link_libraries(wavefrontd
    pcap
    m
    Threads::Threads
)
