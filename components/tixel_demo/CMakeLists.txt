# TIXEL C++ demo pages (optional component).
#
# Boot splash, icon carousel, clock, music, monitor, settings menus and about.
# Depends on `tixel`; contains no platform code.
idf_component_register(
    SRCS
        "src/app_common.cpp"
        "src/app_boot.cpp"
        "src/app_home.cpp"
        "src/app_clock.cpp"
        "src/app_music.cpp"
        "src/app_monitor.cpp"
        "src/app_settings.cpp"
        "src/app_about.cpp"
    INCLUDE_DIRS "include"
    REQUIRES tixel
)

target_compile_options(${COMPONENT_LIB} PRIVATE -fno-exceptions -fno-rtti)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wno-unused-parameter)
