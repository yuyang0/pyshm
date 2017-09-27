#!/usr/bin/env python3
# -*- coding:utf-8 -*-

"""

"""
from __future__ import print_function, division, absolute_import
from os import path
from cffi import FFI
CUR_DIR = path.dirname(path.realpath(__file__))

with open(path.join(CUR_DIR, "pyshm_source.c")) as f:
    _SOURCE = f.read()
with open(path.join(CUR_DIR, "pyshm_cdef.h")) as f:
    _CDEF = f.read()

_LIBS = ' '

__SOURCE_NAMES = ['buddy2.c', 'allocator.c', 'ring.c', 'mempool.c']
EXTRA_SOURCES = [path.join(CUR_DIR, name) for name in __SOURCE_NAMES]

COMPILE_ARGS = "-std=gnu99 -Wall -W -O3 -g -ggdb -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C -DRTE_MACHINE_CPUFLAG_AVX2 "

ffibuilder = FFI()

ffibuilder.set_source("pyshm._shm",
                      _SOURCE,
                      include_dirs=[
                          CUR_DIR,
                      ],
                      library_dirs=[
                          CUR_DIR,
                      ],
                      sources=EXTRA_SOURCES,
                      extra_compile_args=COMPILE_ARGS.split(),
                      # extra_link_args=_LIBS.split()
)

ffibuilder.cdef(_CDEF)

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
