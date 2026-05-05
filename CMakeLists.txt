cmake_minimum_required(VERSION 3.10)
project(WaveNativeProxy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    mesh_legacy/mesh_discovery.cpp
    mesh_legacy/peer_routing.cpp
    mesh_legacy/zk_verifier.cpp
)

target_link_libraries(wavefrontd
    pcap
    Threads::Threads
)
