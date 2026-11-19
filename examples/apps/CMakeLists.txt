# TIXEL sample app set (component `apps`).
#
# The seven pages that demo the framework, ported from the upstream reference
# sample: boot splash, icon carousel, clock, music, system monitor, settings
# menus and about page. No platform code lives here - a port supplies time,
# keys and a display sink.
#
# Lives under examples/ because it is sample material, not part of the library:
# add this directory to EXTRA_COMPONENT_DIRS only when you want the pages.
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
        "src/app_anim.cpp"
    INCLUDE_DIRS "include"
    REQUIRES tixel
)

target_compile_options(${COMPONENT_LIB} PRIVATE -fno-exceptions -fno-rtti)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wno-unused-parameter)
