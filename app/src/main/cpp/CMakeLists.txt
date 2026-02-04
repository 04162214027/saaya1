cmake_minimum_required(VERSION 3.22.1)

project(llama-android)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags for ARM
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")

# Enable NEON for ARM
if(ANDROID_ABI MATCHES "^armeabi-v7a")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
endif()

# llama.cpp source directory (will be cloned here)
set(LLAMA_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp)
set(LLAMA_SRC_DIR ${LLAMA_CPP_DIR}/src)
set(GGML_DIR ${LLAMA_CPP_DIR}/ggml)

# Add llama.cpp sources (new structure as of 2024)
set(LLAMA_SOURCES
    ${GGML_DIR}/src/ggml.c
    ${GGML_DIR}/src/ggml-alloc.c
    ${GGML_DIR}/src/ggml-backend.c
    ${GGML_DIR}/src/ggml-quants.c
    ${LLAMA_SRC_DIR}/llama.cpp
    ${LLAMA_SRC_DIR}/llama-vocab.cpp
    ${LLAMA_SRC_DIR}/llama-grammar.cpp
    ${LLAMA_SRC_DIR}/llama-sampling.cpp
)

# Include directories
include_directories(
    ${LLAMA_CPP_DIR}/include
    ${GGML_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Build llama.cpp as static library
add_library(llama STATIC ${LLAMA_SOURCES})

target_compile_definitions(llama PRIVATE
    GGML_USE_K_QUANTS
    GGML_MULTIPLATFORM
)

# JNI wrapper library
add_library(llama-android SHARED
    llama-android.cpp
)

# Link libraries
target_link_libraries(llama-android
    llama
    android
    log
)
