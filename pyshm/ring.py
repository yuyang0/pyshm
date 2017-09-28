#!/usr/bin/env python3
# -*- coding:utf-8 -*-

"""

"""
from __future__ import print_function, division, absolute_import
import os

from ._shm import ffi, lib
from .shm import get_shm

RING_F_SP_ENQ = lib.RING_F_SP_ENQ
RING_F_SC_DEQ = lib.RING_F_SC_DEQ


class RingError(Exception):
    def __init__(self, prefix):
        self.prefix = prefix
        self.errno = ffi.errno

    def __str__(self):
        return "%s %s" % (self.prefix, os.strerror(self.errno))


class EmptyError(Exception):
    pass


class FullError(Exception):
    pass


class Ring(object):
    def __init__(self, count, flags, shm=None):
        if shm is None:
            c_shm = get_shm().c_shm
        else:
            c_shm = shm.c_shm

        self.count = count
        self.flags = flags
        self.c_ring = lib.ring_create(c_shm, count, flags)
        if self.c_ring == ffi.NULL:
            raise RingError("Ring: ")

    def enqueue(self, buf, flags=None):
        cbuf = ffi.from_buffer(buf)
        if flags is None:
            res = lib.ring_enqueue(self.c_ring, cbuf)
        else:
            if flags & RING_F_SP_ENQ:
                res = lib.ring_sp_enqueue(self.c_ring, cbuf)
            else:
                res = lib.ring_mp_enqueue(self.c_ring, cbuf)
        if res == 0:
            return res
        else:
            if self.free_count == 0:
                raise FullError()
            raise RingError("enqueue: ")

    def dequeue(self, size, flags=None):
        obj_p = ffi.new("void**")
        if flags is None:
            res = lib.ring_dequeue(self.c_ring, obj_p)
        else:
            if flags & RING_F_SC_DEQ:
                res = lib.ring_sc_dequeue(self.c_ring, obj_p)
            else:
                res = lib.ring_mc_dequeue(self.c_ring, obj_p)
        if res == 0:
            return ffi.buffer(obj_p[0], size)
        else:
            if self.used_count == 0:
                raise EmptyError()
            raise RingError("dequeue: ")

    @property
    def used_count(self):
        return lib.ring_count(self.c_ring)

    @property
    def free_count(self):
        return lib.ring_free_count(self.c_ring)

    def cleanup(self):
        lib.ring_destroy(self.c_ring)
        self.c_ring = ffi.NULL

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.cleanup()
