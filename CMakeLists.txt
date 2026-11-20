cmake_minimum_required(VERSION 3.22)
project(faraziq_native_core VERSION 0.1.0 LANGUAGES CXX)

add_library(faraziq_core STATIC
    native/src/cpu/cpu_resource_manager.cpp
    native/src/gpu/gpu_resource_manager.cpp
    native/src/memory/memory_pool.cpp
    native/src/scheduler/task_scheduler.cpp
)

target_include_directories(faraziq_core PUBLIC native/include)
target_compile_features(faraziq_core PUBLIC cxx_std_23)
set_target_properties(faraziq_core PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(faraziq_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(faraziq_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
if(ANDROID)
    target_compile_definitions(faraziq_core PUBLIC FARAZIQ_ANDROID=1)
endif()
