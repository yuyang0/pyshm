#!/usr/bin/env python3
# -*- coding:utf-8 -*-

"""

"""
from __future__ import print_function, division, absolute_import
import os

from .shm import get_shm
from ._shm import ffi, lib


MEMPOOL_F_SP_PUT = lib.MEMPOOL_F_SP_PUT
MEMPOOL_F_SC_GET = lib.MEMPOOL_F_SC_GET


class MempoolError(Exception):
    pass


class EmptyError(MempoolError):
    pass


class FullError(MempoolError):
    pass


class MempoolIOError(MempoolError):
    def __init__(self, prefix):
        self.prefix = prefix
        self.errno = ffi.errno

    def __str__(self):
        return "%s %s" % (self.prefix, os.strerror(self.errno))


class Mempool(object):
    def __init__(self, count, elt_size, flags=0, shm=None):
        if shm is None:
            c_shm = get_shm().c_shm
        else:
            c_shm = shm.c_shm

        self.count = count
        self.elt_size = elt_size
        self.flags = flags
        self.c_mempool = lib.mempool_create(c_shm, count, elt_size, flags)
        if self.c_mempool == ffi.NULL:
            raise MempoolIOError("mempool: ")

    def get(self, flags=None):
        obj_p = ffi.new("void**")
        if flags is None:
            res = lib.mempool_get(self.c_mempool, obj_p)
        else:
            if flags & MEMPOOL_F_SC_GET:
                res = lib.mempool_sc_get(self.c_mempool, obj_p)
            else:
                res = lib.mempool_mc_get(self.c_mempool, obj_p)
        if res != 0:
            raise MempoolIOError("get: ")
        # return ffi.cast("char[%d]" % self.elt_size, obj_p[0])
        return ffi.buffer(obj_p[0], self.elt_size)

    def put(self, buf, flags=None):
        cbuf = ffi.from_buffer(buf)
        if flags is None:
            res = lib.mempool_put(self.c_mempool, cbuf)
        else:
            if flags & MEMPOOL_F_SC_PUT:
                res = lib.mempool_sp_put(self.c_mempool, cbuf)
            else:
                res = lib.mempool_mp_put(self.c_mempool, cbuf)
        if res != 0:
            raise MempoolIOError("put: ")

    def cleanup(self):
        lib.mempool_destroy(self.c_mempool)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.cleanup()
