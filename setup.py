#!/usr/bin/env python
# -*- coding:utf-8 -*-
import sys
import os
import re
from os import path
from setuptools import setup, find_packages


CUR_DIR = path.dirname(path.realpath(__file__))

needs_pytest = {'pytest', 'test', 'ptr'}.intersection(sys.argv)
pytest_runner = ['pytest-runner'] if needs_pytest else []


def find_version(*paths):
    fname = os.path.join(*paths)
    with open(fname) as fhandler:
        version_file = fhandler.read()
        version_match = re.search(r"^__VERSION__ = ['\"]([^'\"]*)['\"]",
                                  version_file, re.M)

    if not version_match:
        raise RuntimeError("Unable to find version string in %s" % (fname,))

    version = version_match.group(1)
    return version


version = find_version('pyshm', '__init__.py')

setup(
    name="pyshm",
    version=version,
    description="python share memory library",
    long_description=open('README.rst').read(),
    url="https://github.com/yuyang0/pyshm",
    author="Yu Yang",
    author_email="yyangplus@gmail.com",
    classifiers=[
        "Development Status :: 4 - Beta",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: Implementation :: PyPy",
        "License :: OSI Approved :: BSD License",
    ],
    packages=find_packages(),
    install_requires=[
        "cffi>=1.0.0",
    ],
    setup_requires=["cffi>=1.0.0"] + pytest_runner,
    tests_require=[
        'pytest-cov',
        'pytest-randomly',
        'pytest-mock',
        'pytest'
    ],
    cffi_modules=[
        "pyshm_ffi/pyshm_build.py:ffibuilder",
    ],
)
