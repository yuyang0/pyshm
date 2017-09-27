#!/usr/bin/env python3
# -*- coding:utf-8 -*-

"""

"""
from __future__ import print_function, division, absolute_import
import os

from ._shm import ffi, lib


class ShareMemError(Exception):
    def __init__(self, ss):
        self.ss = ss

    def __str__(self):
        return self.ss


class ShareMem(object):
    _INSTANCE = None

    def __init__(self, size):
        self.c_shm = lib.shm_create(size)
        if self.c_shm == ffi.NULL:
            raise ShareMemError("ShareMem: %s" % os.strerror(ffi.errno))

    @classmethod
    def instance(cls, size=None):
        if cls._INSTANCE is None:
            if size is None:
                raise ShareMemError("size can't be None")
            cls._INSTANCE = cls(size)
        else:
            if size is not None:
                raise ShareMemError("can't initialize global instance multiple times")

        return cls._INSTANCE

    def statis(self):
        pass

    def cleanup(self):
        lib.shm_destroy(self.c_shm)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.cleanup()


def get_shm():
    return ShareMem.instance()


def init_shm(size):
    return ShareMem.instance(size)
