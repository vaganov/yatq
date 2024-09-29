#! cmake -P
cmake_minimum_required(VERSION 3.22)

set(INCLUDE_PATHS /usr/include /usr/local/include)

find_path(boost_include_dir boost PATHS ${INCLUDE_PATHS} REQUIRED)
message("${boost_include_dir}")
find_path(log4cxx_include_dir log4cxx PATHS ${INCLUDE_PATHS} REQUIRED)
message("${log4cxx_include_dir}")
