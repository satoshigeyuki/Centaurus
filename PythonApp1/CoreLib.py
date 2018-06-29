#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import ctypes
import platform

def get_instance():
    if "_instance" not in get_instance.__dict__:
        dl_path_env = os.getenv("CENTAURUS_DL_PATH", "")
        if platform.uname()[0] == "Windows":
            dl_path = os.path.join(dl_path_env, "libpycentaurus.dll")
        elif platform.uname()[0] == "Linux":
            dl_path = os.path.join(dl_path_env, "libpycentaurus.so")
        get_instance._instance = ctypes.CDLL(dl_path)
    return get_instance._instance