cmake_minimum_required(VERSION 3.16)
project(tixel_parity LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(REPO ${CMAKE_CURRENT_LIST_DIR}/..)
# Upstream reference sources are NOT vendored; point this at your LUMEN clone:
#   cmake -S tests -B build/tests -DLUMEN_REF_DIR=/path/to/LUMEN
set(LUMEN_REF_DIR "${REPO}/res/LUMEN" CACHE PATH "upstream LUMEN checkout (reference only)")
if(NOT EXISTS "${LUMEN_REF_DIR}/core/lm_gfx.c")
  message(FATAL_ERROR
    "Upstream LUMEN not found at ${LUMEN_REF_DIR}.\n"
    "Clone it (git clone https://github.com/Xulijun180/LUMEN) and pass -DLUMEN_REF_DIR=<path>.")
endif()

file(GLOB TIXEL_SRC ${REPO}/components/tixel/src/*.cpp)
file(GLOB U8G2_SRC ${LUMEN_REF_DIR}/third_party/u8g2/csrc/*.c)

add_executable(parity
  parity.cpp
  ${TIXEL_SRC}
  ${U8G2_SRC}
  ${LUMEN_REF_DIR}/core/lm_gfx.c
  ${LUMEN_REF_DIR}/core/lm_math.c
  ${LUMEN_REF_DIR}/fonts/u8g2_fonts_subset.c)

# The port runs at the reference geometry here, with the upstream font set, so
# every draw call can be compared against lm_gfx.c on the same buffer.
target_compile_definitions(parity PRIVATE TIXEL_PANEL_WIDTH=128 TIXEL_PANEL_HEIGHT=64)
target_include_directories(parity PRIVATE
  ${REPO}/components/tixel/include
  ${LUMEN_REF_DIR}/core
  ${LUMEN_REF_DIR}/third_party/u8g2/csrc)
target_compile_options(parity PRIVATE -O2 -ffp-contract=off -w)
target_link_libraries(parity PRIVATE m)

add_executable(menu_parity
  menu_parity.cpp
  ${TIXEL_SRC}
  ${U8G2_SRC}
  ${LUMEN_REF_DIR}/core/lm_gfx.c
  ${LUMEN_REF_DIR}/core/lm_math.c
  ${LUMEN_REF_DIR}/core/lm_anim.c
  ${LUMEN_REF_DIR}/core/lm_settings.c
  ${LUMEN_REF_DIR}/widget/lm_widgets.c
  ${LUMEN_REF_DIR}/widget/lm_menu.c
  ${LUMEN_REF_DIR}/fonts/u8g2_fonts_subset.c)
target_compile_definitions(menu_parity PRIVATE TIXEL_PANEL_WIDTH=128 TIXEL_PANEL_HEIGHT=64)
target_include_directories(menu_parity PRIVATE
  ${REPO}/components/tixel/include
  ${LUMEN_REF_DIR}/core
  ${LUMEN_REF_DIR}/widget
  ${LUMEN_REF_DIR}/fonts
  ${LUMEN_REF_DIR}/third_party/u8g2/csrc)
target_compile_options(menu_parity PRIVATE -O2 -ffp-contract=off -w)
target_link_libraries(menu_parity PRIVATE m)
