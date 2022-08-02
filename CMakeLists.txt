cmake_minimum_required(VERSION 3.12)

project(roq-mmaker)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# conda

if(DEFINED ENV{CONDA_PREFIX} AND NOT DEFINED ENV{CONDA_BUILD})
  message("CMAKE_INSTALL_PREFIX $ENV{CONDA_PREFIX}")
  set(CMAKE_INSTALL_PREFIX $ENV{CONDA_PREFIX})
  set(CMAKE_FIND_ROOT_PATH $ENV{CONDA_PREFIX})
endif()

add_compile_options(-ftemplate-backtrace-limit=0)

# rpath

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# c++

# filesystem

include(GNUInstallDirs)

message(STATUS "CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE} CMAKE_CXX_FLAGS_DEBUG:${CMAKE_CXX_FLAGS_DEBUG}")

option(UMM_USE_TOML "UMM_USE_TOML" ON)

# dependencies

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(roq-api REQUIRED)
find_package(roq-client REQUIRED)
find_package(roq-logging REQUIRED)
find_package(roq-logging REQUIRED)
find_package(tomlplusplus REQUIRED)

# include

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/src/umm/src")

# sub-projects

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/umm/src")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/roq/mmaker")
