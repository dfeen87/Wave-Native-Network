set(SIMULATION_RUNNERS
    run_phase_locking
    run_jitter_robustness
    run_handoff
    run_replay_attack
    run_ablation
    run_scalability
)

foreach(runner IN LISTS SIMULATION_RUNNERS)
    add_executable(${runner} ${runner}.cpp)
    target_include_directories(${runner} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/mesh_legacy
        ${CMAKE_SOURCE_DIR}/simulations
    )
    target_compile_features(${runner} PRIVATE cxx_std_20)
    target_compile_definitions(${runner} PRIVATE WNN_PROJECT_ROOT=\"${CMAKE_SOURCE_DIR}\")
endforeach()
