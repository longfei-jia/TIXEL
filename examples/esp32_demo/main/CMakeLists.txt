idf_component_register(
    SRCS "app_main.cpp" "board.cpp"
    INCLUDE_DIRS "."
    REQUIRES lumen lumen_demo esp_timer esp_driver_gpio
)

target_compile_options(${COMPONENT_LIB} PRIVATE -fno-exceptions -fno-rtti)
