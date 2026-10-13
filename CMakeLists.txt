cmake_minimum_required(VERSION 3.25)
project(125A_Rifferator VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()
# Windows VST3 shipping target: exactly one guitar MIDI output bus.
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(vst3sdk GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
 GIT_TAG v3.8.1_build_84)
FetchContent_MakeAvailable(vst3sdk)
smtg_add_vst3plugin(Rifferator src/RifferatorProcessor.cpp
 src/RifferatorProcessor.hpp src/RifferatorFactory.cpp)
target_link_libraries(Rifferator PRIVATE sdk vstgui_support)
smtg_target_add_plugin_resources(Rifferator RESOURCES resource/rifferator.uidesc)
if(WIN32)
 target_sources(Rifferator PRIVATE resource/rifferator.rc
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp)
endif()
smtg_target_configure_version_file(Rifferator)

function(add_rifferator_test short source)
  add_executable(rifferator_${short}_tests tests/${source})
  target_include_directories(rifferator_${short}_tests PRIVATE src/core src)
  if(MSVC)
   target_compile_options(rifferator_${short}_tests PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/tests/enable_asserts.h")
  else()
   target_compile_options(rifferator_${short}_tests PRIVATE -UNDEBUG)
  endif()
  add_test(NAME rifferator_${short} COMMAND rifferator_${short}_tests)
endfunction()
add_rifferator_test(core riff_engine_tests.cpp)
add_rifferator_test(random riff_random_tests.cpp)
add_rifferator_test(fill riff_fill_tests.cpp)
add_rifferator_test(arrangement riff_arrangement_tests.cpp)
add_rifferator_test(generation riff_generation_tests.cpp)
add_rifferator_test(steps guitar_step_controls_tests.cpp)
add_rifferator_test(scheduler guitar_scheduler_regression_tests.cpp)
add_rifferator_test(numeric numeric_input_tests.cpp)
add_rifferator_test(gui gui_layout_contract_tests.cpp)
set_tests_properties(rifferator_gui PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(rifferator_processor_state_tests
 tests/processor_state_roundtrip_tests.cpp src/RifferatorProcessor.cpp)
target_sources(rifferator_processor_state_tests PRIVATE
 ${vst3sdk_SOURCE_DIR}/public.sdk/source/common/memorystream.cpp
 ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/parameterchanges.cpp
 ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/eventlist.cpp)
target_link_libraries(rifferator_processor_state_tests PRIVATE sdk)
if(MSVC)
 target_compile_options(rifferator_processor_state_tests PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/tests/enable_asserts.h")
else()
 target_compile_options(rifferator_processor_state_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME rifferator_processor_state COMMAND rifferator_processor_state_tests)

# Hostless controller parameter/state regression, not a DAW lifecycle PASS.
add_executable(rifferator_controller_state_tests
 tests/controller_component_state_tests.cpp src/RifferatorProcessor.cpp)
target_sources(rifferator_controller_state_tests PRIVATE
 ${vst3sdk_SOURCE_DIR}/public.sdk/source/common/memorystream.cpp)
target_link_libraries(rifferator_controller_state_tests PRIVATE sdk vstgui_support)
if(MSVC)
 target_compile_options(rifferator_controller_state_tests PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/tests/enable_asserts.h")
else()
 target_compile_options(rifferator_controller_state_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME rifferator_controller_state COMMAND rifferator_controller_state_tests)
