#! cmake -P
cmake_minimum_required(VERSION 3.22)

set(LIB_PATHS /lib /lib/x86_64-linux-gnu /usr/lib /usr/local/lib)

find_library(boost_thread_lib NAMES boost_thread boost_thread-mt PATHS ${LIB_PATHS} REQUIRED)
message("${boost_thread_lib}")
find_library(log4cxx_lib log4cxx PATHS ${LIB_PATHS} REQUIRED)
message("${log4cxx_lib}")
