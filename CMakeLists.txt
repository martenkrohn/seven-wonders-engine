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
# Build the common core once; every executable runs the same implementation.
add_library(engine_core STATIC src/card.cpp src/decision.cpp src/player.cpp src/wonder.cpp src/filer.cpp)
add_executable(7Wonders main.cpp)
target_link_libraries(7Wonders engine_core)
enable_testing()
foreach(name card_tables card_effects decision_resolution choice_simple choice_once_per_turn final_status joint_action_resources science_scoring discard_safety)
    add_executable(test_${name} tests/test_${name}.cpp)
    target_link_libraries(test_${name} engine_core)
    add_test(NAME ${name} COMMAND test_${name})
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${name}_runtime/io" "${CMAKE_BINARY_DIR}/${name}_runtime/logs")
    set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${name}_runtime" TIMEOUT 30)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_${name} PRIVATE -UNDEBUG)
    endif()
endforeach()
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/io" "${CMAKE_BINARY_DIR}/logs")

# Optional standalone client: standard library only, no RL dependencies.
find_package(Python3 3.10 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME generated_card_data COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_card_data.py" --check)
    add_test(NAME compiled_card_data COMMAND "${CMAKE_COMMAND}" -E env
        "SEVENWONDERS_CARD_TEST=$<TARGET_FILE:test_card_tables>" "${Python3_EXECUTABLE}" -m unittest discover
        -s "${CMAKE_CURRENT_SOURCE_DIR}/tests" -p test_bga_card_data.py -v)
    set_tests_properties(generated_card_data compiled_card_data PROPERTIES TIMEOUT 30)
    add_test(NAME protocol_smoke COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/smoke_protocol.py"
        --engine "$<TARGET_FILE:7Wonders>")
    set_tests_properties(protocol_smoke PROPERTIES TIMEOUT 60)
    add_test(NAME decision_protocol COMMAND "${CMAKE_COMMAND}" -E env
        "SEVENWONDERS_TEST_ENGINE=$<TARGET_FILE:7Wonders>" "${Python3_EXECUTABLE}" -m unittest discover
        -s "${CMAKE_CURRENT_SOURCE_DIR}/tests" -p test_decision_protocol.py -v)
    set_tests_properties(decision_protocol PROPERTIES TIMEOUT 60)
endif()
