cmake_minimum_required(VERSION 3.16)
project(tixel_parity LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(REPO ${CMAKE_CURRENT_LIST_DIR}/..)
# The reference C sources are NOT vendored. Point REF_DIR at a checkout of the
# upstream implementation (the default is the res/LUMEN folder of this repo):
#   cmake -S tests -B build/tests -DREF_DIR=/path/to/checkout
set(REF_DIR "${REPO}/res/LUMEN" CACHE PATH "upstream reference implementation (tests only)")
if(NOT EXISTS "${REF_DIR}/core/lm_gfx.c")
  message(FATAL_ERROR
    "Upstream reference not found at ${REF_DIR}.\n"
    "Clone it (git clone https://github.com/Xulijun180/LUMEN) and pass -DREF_DIR=<path>.")
endif()

file(GLOB TIXEL_SRC ${REPO}/components/tixel/src/*.cpp)
file(GLOB U8G2_SRC ${REF_DIR}/third_party/u8g2/csrc/*.c)

add_executable(parity
  parity.cpp
  ${TIXEL_SRC}
  ${U8G2_SRC}
  ${REF_DIR}/core/lm_gfx.c
  ${REF_DIR}/core/lm_math.c
  ${REF_DIR}/fonts/u8g2_fonts_subset.c)

# The port runs at the reference geometry here, with the upstream font set, so
# every draw call can be compared against lm_gfx.c on the same buffer.
target_compile_definitions(parity PRIVATE TIXEL_PANEL_WIDTH=128 TIXEL_PANEL_HEIGHT=64)
target_include_directories(parity PRIVATE
  ${REPO}/components/tixel/include
  ${REF_DIR}/core
  ${REF_DIR}/third_party/u8g2/csrc)
target_compile_options(parity PRIVATE -O2 -ffp-contract=off -w)
target_link_libraries(parity PRIVATE m)

add_executable(menu_parity
  menu_parity.cpp
  ${TIXEL_SRC}
  ${U8G2_SRC}
  ${REF_DIR}/core/lm_gfx.c
  ${REF_DIR}/core/lm_math.c
  ${REF_DIR}/core/lm_anim.c
  ${REF_DIR}/core/lm_settings.c
  ${REF_DIR}/widget/lm_widgets.c
  ${REF_DIR}/widget/lm_menu.c
  ${REF_DIR}/fonts/u8g2_fonts_subset.c)
target_compile_definitions(menu_parity PRIVATE TIXEL_PANEL_WIDTH=128 TIXEL_PANEL_HEIGHT=64)
target_include_directories(menu_parity PRIVATE
  ${REPO}/components/tixel/include
  ${REF_DIR}/core
  ${REF_DIR}/widget
  ${REF_DIR}/fonts
  ${REF_DIR}/third_party/u8g2/csrc)
target_compile_options(menu_parity PRIVATE -O2 -ffp-contract=off -w)
target_link_libraries(menu_parity PRIVATE m)
