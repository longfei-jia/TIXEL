cmake_minimum_required(VERSION 3.16)
project(lumen_preview CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

file(GLOB LUMEN_SRC ${CMAKE_CURRENT_LIST_DIR}/../../components/lumen/src/*.cpp)
file(GLOB DEMO_SRC ${CMAKE_CURRENT_LIST_DIR}/../../components/lumen_demo/src/*.cpp)

add_executable(preview preview.cpp ${LUMEN_SRC} ${DEMO_SRC})
target_include_directories(preview PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../components/lumen/include
  ${CMAKE_CURRENT_LIST_DIR}/../../components/lumen_demo/include)
target_compile_options(preview PRIVATE -Wall -Wextra -Wno-unused-parameter)
