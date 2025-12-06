project(CUDATestProgram LANGUAGES CXX CUDA)

set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

enable_language(CUDA)
include(CheckLanguage)
check_language(CUDA)

find_package(CUDAToolkit REQUIRED)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CUDA_ARCHITECTURES 89)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Wno-deprecated-gpu-targets")

file(GLOB_RECURSE Sources
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cu"
)

file(GLOB_RECURSE Headers
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_executable(CUDATestProgram ${Sources} ${Headers})
set_target_properties(CUDATestProgram PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_compile_features(CUDATestProgram PRIVATE cuda_std_20)

target_link_libraries(CUDATestProgram PRIVATE
        Bee::Bee
        CUDA::cudart
        #        CUDA::cublas
)
