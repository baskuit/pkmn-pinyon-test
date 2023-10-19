cmake_minimum_required(VERSION 3.8.0)
project(pkmn-pinyon-test LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 20)

add_subdirectory(extern/pinyon)
target_include_directories(pinyon INTERFACE extern/pinyon/src)

include_directories(extern/engine/zig-out/include)

add_executable(main ./main.cc)
target_link_libraries(main pinyon)
target_link_libraries(main ${PROJECT_SOURCE_DIR}/extern/engine/zig-out/lib/libpkmn-showdown.a)

add_executable(example ./example.cc)
target_link_libraries(example ${PROJECT_SOURCE_DIR}/extern/engine/zig-out/lib/libpkmn-showdown.a)

add_executable(benchmark ./benchmark.cc)
target_link_libraries(benchmark pinyon)
target_link_libraries(benchmark ${PROJECT_SOURCE_DIR}/extern/engine/zig-out/lib/libpkmn-showdown.a)
