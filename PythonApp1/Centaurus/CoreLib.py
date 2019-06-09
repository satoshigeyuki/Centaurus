#!/usr/bin/env python3

import os
import ctypes
import platform

def load_dll():
    dl_path_env = os.getenv("CENTAURUS_DL_PATH", "")
    if platform.uname()[0] == "Windows":
        dl_path = os.path.join(dl_path_env, "libpycentaurus.dll")
    elif platform.uname()[0] == "Linux":
        dl_path = os.path.join(dl_path_env, "libpycentaurus.so")
    return ctypes.CDLL(dl_path, mode=ctypes.RTLD_GLOBAL)

CoreLib = load_dll()
