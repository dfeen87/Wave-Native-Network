cmake_minimum_required(VERSION 3.10)
project(WaveNativeProxy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto")

find_package(Threads REQUIRED)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/mesh_legacy
)

# ---------------------------------------------------------
# Fetch AILEE Trust Layer
# ---------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    ailee_trust_layer
    GIT_REPOSITORY https://github.com/dfeen87/AILEE-Trust-Layer.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ailee_trust_layer)

# ---------------------------------------------------------
# Fetch nlohmann_json
# ---------------------------------------------------------
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------
# Simulations subdirectory
# ---------------------------------------------------------
add_subdirectory(simulations)

# ---------------------------------------------------------
# Wave Native Network Core (WNN v2.0.0)
# ---------------------------------------------------------
file(GLOB_RECURSE WNN_CORE_SOURCES
    "${CMAKE_SOURCE_DIR}/src/space_segment/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/infrastructure_segment/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/sync_membrane/*.cpp"
)

add_library(wnn_core ${WNN_CORE_SOURCES})

target_include_directories(wnn_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(wnn_core
    PRIVATE
        nlohmann_json::nlohmann_json
        m
        atomic
        Threads::Threads
)

# ---------------------------------------------------------
# WNN Vehicle Demo
# ---------------------------------------------------------
add_executable(wnn_vehicle_demo
    examples/wnn_vehicle_demo.cpp
)

target_include_directories(wnn_vehicle_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(wnn_vehicle_demo
    PRIVATE
        wnn_core
        nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# Wavefront Daemon (legacy mesh + new WNN integration)
# ---------------------------------------------------------
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
    PRIVATE
        pcap
        m
        atomic
        Threads::Threads
)
