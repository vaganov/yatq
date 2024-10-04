cmake_minimum_required(VERSION 3.22)
project(yatq)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost CONFIG REQUIRED COMPONENTS thread)
find_package(log4cxx CONFIG REQUIRED)
find_package(bdl)

include_directories("include")

add_executable(yatq_example
        examples/yatq_example.cpp
)

target_link_libraries(yatq_example Boost::thread log4cxx)

add_executable(test_precision
        tests/precision/test_precision.cpp
)

add_executable(test_wait_until
        tests/precision/test_wait_until.cpp
)

add_executable(test_load
        tests/profiling/test_load.cpp
)

add_executable(test_bde
        tests/profiling/test_bde.cpp
)
target_link_libraries(test_bde bdl)

include(CTest OPTIONAL)
enable_testing()
add_test(NAME test_precision COMMAND test_precision)
add_test(NAME test_wait_until COMMAND test_wait_until)
add_test(NAME test_load COMMAND test_load)
add_test(NAME test_bde COMMAND test_bde)

install(DIRECTORY include/yatq TYPE INCLUDE)
