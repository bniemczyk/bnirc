#!/usr/bin/env python

from distutils.core import setup, Extension
from os import getenv

ext = Extension("bnirc", [])
ext.extra_objects = ["python.o"]
ext.extra_link_args = ["-L../../src", "-L../../src/.libs", "-L../../src/libs", "-lbnirc"]
ver = getenv("VERSION")

setup(name="bnirc", version=ver,  ext_modules=[ext])
