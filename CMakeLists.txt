cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_C_FLAGS_DEBUG "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O3")

project(freedv-tnc)

add_executable(${PROJECT_NAME})

add_subdirectory(src)

target_link_libraries(${PROJECT_NAME} PRIVATE codec2 pulse pulse-simple gpiod m)

#target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fsanitize=undefined -fno-sanitize-recover)
#target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -static-libasan -static-libubsan)

#set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-save-temps")
#target_compile_options(${PROJECT_NAME} PRIVATE -g -pg)
#target_link_options(${PROJECT_NAME} PRIVATE -g -pg)
