#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

from distutils.core import setup, Extension

centaurus_module = Extension('_centaurus', sources = ['module.cpp'])

setup(name = 'Centaurus',
	  version = '0.0.1',
	  description = 'Python binding for Centaurus',
	  author = 'Hiroka Ihara',
	  author_email = 'ihara@eidos.ic.i.u-tokyo.ac.jp',
	  packages = ['centaurus'],
	  ext_modules = [centaurus_module])
