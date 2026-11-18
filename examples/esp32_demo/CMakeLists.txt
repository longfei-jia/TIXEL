cmake_minimum_required(VERSION 3.16)

# The two UI components live at ../../components. A board port (panel driver,
# key driver) is the only extra piece an application needs.
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../../components)

# Build only what this demo needs: the full IDF component set (mbedTLS, Wi-Fi,
# BT, ...) is irrelevant here and dominates build time.
set(COMPONENTS main lumen lumen_demo)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lumen_esp32_demo)
