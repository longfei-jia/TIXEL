cmake_minimum_required(VERSION 3.16)
project(tixel_preview CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

file(GLOB TIXEL_SRC ${CMAKE_CURRENT_LIST_DIR}/../../components/tixel/src/*.cpp)
file(GLOB APPS_SRC ${CMAKE_CURRENT_LIST_DIR}/../../examples/apps/src/*.cpp)

add_executable(preview preview.cpp ${TIXEL_SRC} ${APPS_SRC})
target_include_directories(preview PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../components/tixel/include
  ${CMAKE_CURRENT_LIST_DIR}/../../examples/apps/include)
target_compile_options(preview PRIVATE -Wall -Wextra -Wno-unused-parameter)
