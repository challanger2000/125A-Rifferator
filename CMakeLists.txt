cmake_minimum_required(VERSION 3.25)
project(125A_Rifferator VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()
add_executable(rifferator_core_tests tests/riff_engine_tests.cpp)
target_include_directories(rifferator_core_tests PRIVATE src/core)
add_test(NAME rifferator_core COMMAND rifferator_core_tests)
add_executable(rifferator_random_tests tests/riff_random_tests.cpp)
add_test(NAME rifferator_random COMMAND rifferator_random_tests)


add_executable(rifferator_fill_tests tests/riff_fill_tests.cpp)
add_test(NAME rifferator_fill COMMAND rifferator_fill_tests)

add_executable(rifferator_scheduler_tests tests/block_scheduler_tests.cpp)
add_test(NAME rifferator_scheduler COMMAND rifferator_scheduler_tests)

# SDK version is pinned to the same Steinberg release already validated in
# 125A-Phraseator-Final; do not silently float to SDK main.
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(vst3sdk
 GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
 GIT_TAG v3.8.1_build_84
)
FetchContent_MakeAvailable(vst3sdk)
smtg_add_vst3plugin(Rifferator
 src/RifferatorProcessor.cpp
 src/RifferatorProcessor.hpp
 src/RifferatorFactory.cpp
)
target_link_libraries(Rifferator PRIVATE sdk vstgui_support)
smtg_target_add_plugin_resources(Rifferator RESOURCES resource/rifferator.uidesc)
if(WIN32)
 target_sources(Rifferator PRIVATE
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp)
endif()
smtg_target_configure_version_file(Rifferator)

add_executable(rifferator_arrangement_tests tests/riff_arrangement_tests.cpp)
add_test(NAME rifferator_arrangement COMMAND rifferator_arrangement_tests)

add_executable(rifferator_transport_tests tests/transport_regression_tests.cpp)
add_test(NAME rifferator_transport COMMAND rifferator_transport_tests)

add_executable(rifferator_drum_tests tests/drum_companion_tests.cpp)
add_test(NAME rifferator_drums COMMAND rifferator_drum_tests)

add_executable(rifferator_generation_tests tests/riff_generation_tests.cpp)
add_test(NAME rifferator_generation COMMAND rifferator_generation_tests)

add_executable(rifferator_guitar_steps_tests tests/guitar_step_controls_tests.cpp)
add_test(NAME rifferator_guitar_steps COMMAND rifferator_guitar_steps_tests)

add_executable(rifferator_ebm_engine_tests tests/ebm_bass_engine_tests.cpp)
add_test(NAME rifferator_ebm_engine COMMAND rifferator_ebm_engine_tests)

add_executable(rifferator_ebm_arrangement_tests tests/ebm_bass_arrangement_tests.cpp)
add_test(NAME rifferator_ebm_arrangement COMMAND rifferator_ebm_arrangement_tests)

add_executable(rifferator_ebm_scheduler_tests tests/ebm_bass_scheduler_tests.cpp)
add_test(NAME rifferator_ebm_scheduler COMMAND rifferator_ebm_scheduler_tests)

add_executable(rifferator_ebm_end_to_end_tests tests/ebm_bass_end_to_end_tests.cpp)
add_test(NAME rifferator_ebm_end_to_end COMMAND rifferator_ebm_end_to_end_tests)

add_executable(rifferator_ebm_midi_audit_tests tests/ebm_bass_midi_audit_tests.cpp)
add_test(NAME rifferator_ebm_midi_audit COMMAND rifferator_ebm_midi_audit_tests)

add_executable(rifferator_ebm_realtime_tests tests/ebm_bass_realtime_tests.cpp)
add_test(NAME rifferator_ebm_realtime COMMAND rifferator_ebm_realtime_tests)

add_executable(rifferator_ebm_shared_routing_tests tests/ebm_shared_routing_tests.cpp)
add_test(NAME rifferator_ebm_shared_routing COMMAND rifferator_ebm_shared_routing_tests)

add_executable(rifferator_ebm_live_controls_tests tests/ebm_live_controls_tests.cpp)
add_test(NAME rifferator_ebm_live_controls COMMAND rifferator_ebm_live_controls_tests)

add_executable(rifferator_chord_tests tests/chord_companion_tests.cpp)
add_test(NAME rifferator_chords COMMAND rifferator_chord_tests)
add_executable(rifferator_hybrid_harmony_tests tests/ebm_hybrid_harmony_tests.cpp)
add_test(NAME rifferator_hybrid_harmony COMMAND rifferator_hybrid_harmony_tests)

add_executable(rifferator_ebm_guitar_coordination_tests tests/ebm_guitar_coordination_tests.cpp)
add_test(NAME rifferator_ebm_guitar_coordination COMMAND rifferator_ebm_guitar_coordination_tests)

add_executable(rifferator_industrial_guitar_tests tests/industrial_guitar_groove_tests.cpp)
add_test(NAME rifferator_industrial_guitar COMMAND rifferator_industrial_guitar_tests)

add_executable(rifferator_guitar_activation_tests tests/guitar_step_activation_regression_tests.cpp)
add_test(NAME rifferator_guitar_activation COMMAND rifferator_guitar_activation_tests)

add_executable(rifferator_midi_overflow_tests tests/midi_overflow_assertions_tests.cpp)
add_test(NAME rifferator_midi_overflow COMMAND rifferator_midi_overflow_tests)

add_executable(rifferator_guitar_fill_routing_tests tests/guitar_fill_live_routing_tests.cpp)
add_test(NAME rifferator_guitar_fill_routing COMMAND rifferator_guitar_fill_routing_tests)

add_executable(rifferator_processor_state_tests tests/processor_state_roundtrip_tests.cpp src/RifferatorProcessor.cpp)
target_link_libraries(rifferator_processor_state_tests PRIVATE sdk)
add_test(NAME rifferator_processor_state COMMAND rifferator_processor_state_tests)

# ASSERTION AUDIT: CI Release must not compile out regression checks.
# /UNDEBUG is MSVC /U<macro>; -UNDEBUG is GCC/Clang equivalent.
get_property(rifferator_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(test_target IN LISTS rifferator_targets)
  if(test_target MATCHES "^rifferator_.*tests$")
    if(MSVC)
      target_compile_options(${test_target} PRIVATE /UNDEBUG)
    else()
      target_compile_options(${test_target} PRIVATE -UNDEBUG)
    endif()
  endif()
endforeach()
