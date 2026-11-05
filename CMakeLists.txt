# Downstream standalone build; see NOTICE and docs/SIMULATOR_CHANGES.md.
cmake_minimum_required(VERSION 3.5)

project(7Wonders LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JSON_BuildTests OFF CACHE INTERNAL "")

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(SOURCES
    main.cpp
    src/card.cpp
    src/decision.cpp
	src/player.cpp
    src/wonder.cpp
	src/filer.cpp
	)

set(HEADERS
    include/card.h
	include/player.h
	include/resources.h
    include/wonder.h
	include/filer.h
	include/nlohmann/json.hpp
	)

execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE ENGINE_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git diff --quiet WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE ENGINE_DIRTY)
if(NOT ENGINE_COMMIT)
    set(ENGINE_COMMIT "unknown-archive")
endif()
if(ENGINE_DIRTY EQUAL 0)
    set(ENGINE_DIRTY_VALUE false)
else()
    set(ENGINE_DIRTY_VALUE true)
endif()
add_definitions(-DENGINE_SOURCE_COMMIT="${ENGINE_COMMIT}" -DENGINE_SOURCE_DIRTY=${ENGINE_DIRTY_VALUE})
add_executable(7Wonders ${SOURCES} ${HEADERS})

add_executable(test_decision_resolution tests/test_decision_resolution.cpp
    src/card.cpp src/decision.cpp src/player.cpp src/wonder.cpp src/filer.cpp)
add_test(NAME decision_resolution COMMAND test_decision_resolution)
set_tests_properties(decision_resolution PROPERTIES TIMEOUT 30)

# Test executable for choice-once-per-turn verification (simple, no card loading)
add_executable(test_choice_simple
    tests/test_choice_simple.cpp
    src/player.cpp
    src/wonder.cpp
    src/card.cpp
    src/decision.cpp
    src/filer.cpp
)

# Test executable for choice-once-per-turn verification (full integration)
add_executable(test_choice_once_per_turn
    tests/test_choice_once_per_turn.cpp
    src/card.cpp
    src/decision.cpp
    src/player.cpp
    src/wonder.cpp
    src/filer.cpp
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/io" "${CMAKE_CURRENT_BINARY_DIR}/logs")

# Register the existing regression executables for local checks and CI.
enable_testing()
add_test(NAME choice_simple COMMAND test_choice_simple)
add_test(NAME choice_once_per_turn COMMAND test_choice_once_per_turn)

add_executable(test_final_status tests/test_final_status.cpp
    src/card.cpp src/decision.cpp src/player.cpp src/wonder.cpp src/filer.cpp)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/final_status_runtime")
add_test(NAME final_status COMMAND test_final_status)
set_tests_properties(final_status PROPERTIES
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/final_status_runtime" TIMEOUT 10)

add_executable(test_joint_action_resources tests/test_joint_action_resources.cpp
    src/card.cpp src/decision.cpp src/player.cpp src/wonder.cpp src/filer.cpp)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/joint_action_resources_runtime")
add_test(NAME joint_action_resources COMMAND test_joint_action_resources)
set_tests_properties(joint_action_resources PROPERTIES
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/joint_action_resources_runtime" TIMEOUT 10)

add_executable(test_science_scoring tests/test_science_scoring.cpp
    src/card.cpp src/decision.cpp src/player.cpp src/wonder.cpp src/filer.cpp)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/science_scoring_runtime")
add_test(NAME science_scoring COMMAND test_science_scoring)
set_tests_properties(science_scoring PROPERTIES
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/science_scoring_runtime" TIMEOUT 10)

add_executable(test_discard_safety tests/test_discard_safety.cpp
    src/card.cpp src/decision.cpp src/player.cpp src/wonder.cpp src/filer.cpp)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/discard_safety_runtime")
add_test(NAME discard_safety COMMAND test_discard_safety)
set_tests_properties(discard_safety PROPERTIES
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/discard_safety_runtime" TIMEOUT 15)

# The regression programs use assert(), including when the engine is built Release.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_choice_simple PRIVATE -UNDEBUG)
    target_compile_options(test_choice_once_per_turn PRIVATE -UNDEBUG)
endif()

# Optional standalone client: standard library only, no RL dependencies.
find_package(Python3 3.10 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME protocol_smoke COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/smoke_protocol.py"
        --engine "$<TARGET_FILE:7Wonders>")
    set_tests_properties(protocol_smoke PROPERTIES TIMEOUT 60)
    add_test(NAME decision_protocol COMMAND "${CMAKE_COMMAND}" -E env
        "SEVENWONDERS_TEST_ENGINE=$<TARGET_FILE:7Wonders>" "${Python3_EXECUTABLE}" -m unittest discover
        -s "${CMAKE_CURRENT_SOURCE_DIR}/tests" -p test_decision_protocol.py -v)
    set_tests_properties(decision_protocol PROPERTIES TIMEOUT 60)
endif()
