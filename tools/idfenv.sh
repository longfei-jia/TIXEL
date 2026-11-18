#!/usr/bin/env bash
# ESP-IDF v6.1 build environment for Git Bash / MSys.
#
#   source tools/idfenv.sh
#   python tools/idfpy_wrapper.py -C examples/esp32_demo build
#
# idf.py refuses to run under MSys (it only prints a warning), so the wrapper
# strips MSYSTEM before running it. Paths given to python/cmake use E:/ form,
# PATH entries use /e/ form.
_ORIG_PATH="$PATH"
export PATH="/usr/bin:/bin:$PATH"

export IDF_PATH="E:/Programs/ESP-IDF/v6.1/esp-idf"
export IDF_TOOLS_PATH="E:/Programs/ESP-IDF/v6.1/tools"
export IDF_PYTHON_ENV_PATH="E:/Programs/ESP-IDF/v6.1/tools/python/v6.1/venv"
export ESP_IDF_VERSION="6.1.0"
export IDF_CCACHE_ENABLE=1
export ESP_ROM_ELF_DIR="E:/Programs/ESP-IDF/v6.1/tools/esp-rom-elfs/20241011"

R="/e/Programs/ESP-IDF/v6.1/tools"
export PATH="$R/python/v6.1/venv/Scripts:$R/ccache/4.12.1/ccache-4.12.1-windows-x86_64:$R/cmake/4.0.3/bin:$R/ninja/1.12.1:$R/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/bin:$IDF_PATH/tools:/usr/bin:/bin:$_ORIG_PATH"
