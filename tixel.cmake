# TIXEL - drop-in helper for git submodule users.
#
#   git submodule add https://gitee.com/longfei-jia/TIXEL.git third_party/TIXEL
#
# In your project's root CMakeLists.txt, BEFORE project():
#
#   include(third_party/TIXEL/tixel.cmake)
#
# That is the whole integration: this file appends the component directories
# to EXTRA_COMPONENT_DIRS. Then depend on it from main:
#
#   idf_component_register(... REQUIRES tixel)        # library only
#   idf_component_register(... REQUIRES tixel apps)   # with the sample pages
#
# Options (set before the include):
#
#   set(TIXEL_WITH_APPS OFF)   # skip examples/apps, library only
#
# The include must come before project() because IDF resolves components at
# project() time; adding to EXTRA_COMPONENT_DIRS later has no effect.

set(TIXEL_DIR "${CMAKE_CURRENT_LIST_DIR}")

if(NOT EXISTS "${TIXEL_DIR}/components/tixel/CMakeLists.txt")
    message(FATAL_ERROR
        "TIXEL: components/tixel/CMakeLists.txt not found under ${TIXEL_DIR}. "
        "Is the submodule initialised? Run: git submodule update --init --recursive")
endif()

list(APPEND EXTRA_COMPONENT_DIRS "${TIXEL_DIR}/components")

if(NOT DEFINED TIXEL_WITH_APPS)
    set(TIXEL_WITH_APPS ON CACHE BOOL "Add TIXEL sample pages (component apps)")
endif()

if(TIXEL_WITH_APPS)
    list(APPEND EXTRA_COMPONENT_DIRS "${TIXEL_DIR}/examples")
endif()

message(STATUS "TIXEL: ${TIXEL_DIR} (sample apps: ${TIXEL_WITH_APPS})")
