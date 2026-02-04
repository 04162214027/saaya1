cmake_minimum_required(VERSION 3.22.1)

project("llama-android")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Android libraries
find_library(log-lib log)
find_library(android-lib android)

# Add llama.cpp source directory
# You need to place llama.cpp source in src/main/cpp/llama.cpp/
set(LLAMA_DIR ${CMAKE_SOURCE_DIR}/llama.cpp)

# llama.cpp sources
set(LLAMA_SOURCES
    ${LLAMA_DIR}/ggml.c
    ${LLAMA_DIR}/llama.cpp
    ${LLAMA_DIR}/common/common.cpp
    ${LLAMA_DIR}/common/sampling.cpp
)

# Build llama.cpp as static library
add_library(llama STATIC ${LLAMA_SOURCES})

target_include_directories(llama PUBLIC
    ${LLAMA_DIR}
    ${LLAMA_DIR}/common
)

target_compile_definitions(llama PRIVATE
    GGML_USE_CPU
)

# Build JNI wrapper
add_library(${CMAKE_PROJECT_NAME} SHARED
    llama-android.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${LLAMA_DIR}
    ${LLAMA_DIR}/common
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    llama
    ${log-lib}
    ${android-lib}
)
