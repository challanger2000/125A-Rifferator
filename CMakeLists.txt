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
if(WIN32)
 target_sources(Rifferator PRIVATE
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp)
endif()
smtg_target_configure_version_file(Rifferator)

add_executable(rifferator_arrangement_tests tests/riff_arrangement_tests.cpp)
add_test(NAME rifferator_arrangement COMMAND rifferator_arrangement_tests)

add_executable(rifferator_transport_tests tests/transport_regression_tests.cpp)
add_test(NAME rifferator_transport COMMAND rifferator_transport_tests)
