cmake_minimum_required(VERSION 3.23)

set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_C_FLAGS_DEBUG "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O3")

project(freedv-tnc)

add_executable(${PROJECT_NAME})

add_subdirectory(src)

target_link_libraries(${PROJECT_NAME} PRIVATE codec2)
