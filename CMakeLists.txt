cmake_minimum_required(VERSION 3.10)

option(USE_VCPKG "Use vcpkg to manage dependencies" OFF)

if(USE_VCPKG)
  message(STATUS "Using vcpkg")
	include(cmake/Vcpkg.cmake)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(MLPACK mlpack::mlpack)
  if (NOT MLPACK)
    message(FATAL_ERROR "Please install mlpack and its dependencies. https://github.com/mlpack/mlpack/blob/master/doc/user/install.md")
  endif()
endif()

project(mons C CXX)

# Find dependencies
find_package(Armadillo CONFIG REQUIRED)
find_path(MLPACK_INCLUDE_DIRS "mlpack.hpp")
find_package(asio CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)

# Tests
add_executable(mons-test "${PROJECT_SOURCE_DIR}/tests/test.cpp")
add_executable(message-tests "${PROJECT_SOURCE_DIR}/tests/message_serialization_test.cpp")

# Link dependencies
target_link_libraries(mons-test PRIVATE armadillo)
target_include_directories(mons-test PRIVATE ${MLPACK_INCLUDE_DIRS})
target_link_libraries(mons-test PRIVATE asio::asio)
target_link_libraries(mons-test PRIVATE cereal::cereal)

target_link_libraries(message-tests PRIVATE armadillo)
target_include_directories(message-tests PRIVATE ${MLPACK_INCLUDE_DIRS})
target_link_libraries(message-tests PRIVATE asio::asio)
target_link_libraries(message-tests PRIVATE cereal::cereal)
