"""
idf.py wrapper for MSys.

ESP-IDF v6 tools/idf.py detects MSYSTEM and bails out with a warning instead of
running main(), so nothing happens under Git Bash. MSYSTEM is injected by the
MSys runtime on exec and cannot be unset from the shell; pop it in-process and
run idf.py via runpy.

    source tools/idfenv.sh
    python tools/idfpy_wrapper.py -C examples/esp32_demo build
    python tools/idfpy_wrapper.py -C examples/esp32_demo -p COM7 flash monitor
"""
import os
import runpy
import sys

os.environ.pop("MSYSTEM", None)

idf_path = os.environ["IDF_PATH"]
idf_py = os.path.join(idf_path, "tools", "idf.py")
tools = os.path.join(idf_path, "tools")

# idf.py imports sibling modules (python_version_checker, ...) directly
if tools not in sys.path:
    sys.path.insert(0, tools)

sys.argv = [idf_py] + sys.argv[1:]
runpy.run_path(idf_py, run_name="__main__")
