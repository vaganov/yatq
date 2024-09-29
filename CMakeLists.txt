cmake_minimum_required(VERSION 3.22)
project(yatq)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost CONFIG REQUIRED COMPONENTS thread)
find_package(log4cxx CONFIG REQUIRED)

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

include(CTest OPTIONAL)
enable_testing()
add_test(NAME test_precision COMMAND test_precision)
add_test(NAME test_wait_until COMMAND test_wait_until)

install(DIRECTORY include/yatq TYPE INCLUDE)
