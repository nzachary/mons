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

# Link dependencies

link_libraries(armadillo)
include_directories(${MLPACK_INCLUDE_DIRS})
link_libraries(asio::asio)
link_libraries(cereal::cereal)

# Tests
add_executable(message-tests "${PROJECT_SOURCE_DIR}/tests/message_serialization_test.cpp")
add_executable(network-tests "${PROJECT_SOURCE_DIR}/tests/local_network_test.cpp")
add_executable(equality-tests "${PROJECT_SOURCE_DIR}/tests/equality_test.cpp")

# Examples
add_executable(ffn-server "${PROJECT_SOURCE_DIR}/examples/FFN/server.cpp")
add_executable(ffn-client "${PROJECT_SOURCE_DIR}/examples/FFN/client.cpp")
