# TIXEL C++ - IDF component (library only).
#
# No dependencies beyond libm: fonts are decoded in-tree, so the component adds
# no third-party code to the build. Add the components/ directory to
# EXTRA_COMPONENT_DIRS, then `REQUIRES tixel`.
idf_component_register(
    SRCS
        "src/math.cpp"
        "src/motion.cpp"
        "src/font.cpp"
        "src/fonts_data.cpp"
        "src/canvas.cpp"
        "src/input.cpp"
        "src/settings.cpp"
        "src/ui.cpp"
        "src/overlay.cpp"
        "src/widgets.cpp"
        "src/menu.cpp"
        "src/icons.cpp"
    INCLUDE_DIRS "include"
)

# The UI never throws and never uses RTTI; keeping both off saves flash and
# keeps the component usable from -fno-exceptions projects.
target_compile_options(${COMPONENT_LIB} PRIVATE -fno-exceptions -fno-rtti)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wno-unused-parameter)
