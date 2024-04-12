# (c) copyright 2023 Mikhail Mitkevich
cmake_minimum_required(VERSION 3.12)

project(roqa)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_VERBOSE_MAKEFILE ON)

# conda

if(DEFINED ENV{CONDA_PREFIX} AND NOT DEFINED ENV{CONDA_BUILD})
  message("CMAKE_INSTALL_PREFIX $ENV{CONDA_PREFIX}")
  set(CMAKE_INSTALL_PREFIX $ENV{CONDA_PREFIX})
  set(CMAKE_FIND_ROOT_PATH $ENV{CONDA_PREFIX})
endif()

option(SANITIZE_BUILD "SANITIZE BUILD" ON)

add_compile_options(-ftemplate-backtrace-limit=0)

if(SANITIZE_BUILD)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
  add_compile_options(-fno-omit-frame-pointer -fsanitize=address)
  add_compile_options(-fno-omit-frame-pointer -fno-inline -fno-visibility-inlines-hidden)
  add_compile_options( -fsanitize=address )
  link_libraries("-fsanitize=address")
  message("SANITIZE BUILD = ON")
endif (SANITIZE_BUILD)

# rpath

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# c++

# filesystem

include(GNUInstallDirs)

message(STATUS "CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE} CMAKE_CXX_FLAGS_DEBUG:${CMAKE_CXX_FLAGS_DEBUG}")

option(USE_LQS "USE_LQS" ON)
option(USE_DAG "USE_DAG" OFF)
option(USE_SPREADER "USE_SPREADER" OFF)

# dependencies

find_package(fmt REQUIRED)
find_package(absl REQUIRED)
find_package(magic_enum REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(roq-api REQUIRED)
find_package(roq-client REQUIRED)
find_package(roq-logging REQUIRED)
find_package(roq-flags REQUIRED)
find_package(roq-cache REQUIRED)
find_package(roq-io REQUIRED)
find_package(roq-web REQUIRED)

link_libraries(${CMAKE_DL_LIBS})

# include

include_directories("${CMAKE_SOURCE_DIR}/src")

# sub-projects

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/roq/core")

if(USE_DAG)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/roq/dag")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/roq/dag/ops")
endif()
if(USE_SPREADER)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/roq/spreader")
endif()
if(USE_LQS)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/roq/lqs")
endif()

#add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/roq/app")


install(
  TARGETS roqa
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)