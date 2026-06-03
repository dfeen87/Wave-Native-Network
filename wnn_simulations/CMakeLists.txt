cmake_minimum_required(VERSION 3.10)
project(wnn_simulations)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

get_filename_component(WNN_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

add_executable(wnn_sim_runner
    wnn_sim_runner.cpp
    "${WNN_ROOT_DIR}/src/pll_controller.cpp"
    "${WNN_ROOT_DIR}/src/ambient_verifier.cpp"
    "${WNN_ROOT_DIR}/mesh_legacy/mesh_discovery.cpp"
    "${WNN_ROOT_DIR}/mesh_legacy/peer_routing.cpp"
    "${WNN_ROOT_DIR}/mesh_legacy/zk_verifier.cpp"
)

target_include_directories(wnn_sim_runner PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${WNN_ROOT_DIR}"
    "${WNN_ROOT_DIR}/src"
    "${WNN_ROOT_DIR}/mesh_legacy"
)

target_compile_definitions(wnn_sim_runner PRIVATE
    WNN_SIM_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(wnn_sim_runner PRIVATE
    m
    atomic
    Threads::Threads
)
