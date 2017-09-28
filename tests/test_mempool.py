#!/usr/bin/env python3
# -*- coding:utf-8 -*-

"""

"""
from __future__ import print_function, division, absolute_import
import sys
import os
from os.path import dirname, abspath
import struct

import pytest

sys.path.insert(0, dirname(abspath(__file__)))

from utils import test_shm
from pyshm import mempool


def test_single_process(test_shm):
    with mempool.Mempool(1, 4, shm=test_shm) as mp:
        v = 1000
        buf = mp.get()
        with pytest.raises(mempool.EmptyError):
            mp.get()
        assert len(buf) == 4
        struct.pack_into("=i", buf, 0, v)
        mp.put(buf)
        buf = mp.get()
        vv, = struct.unpack_from("=i", buf)
        assert v == vv


def test_multiple_processes(test_shm):
    count = 4
    mp = mempool.Mempool(count, 4, shm=test_shm)
    assert mp.used_count == 0
    assert mp.free_count == 4

    pid = os.fork()
    if pid == 0:
        for i in range(count):
            buf = mp.get()
            struct.pack_into("=i", buf, 0, i)
            mp.put(buf)
    else:
        cid, status = os.wait()
        assert pid == cid
        with mp:
            for i in range(count):
                buf = mp.get()
                assert len(buf) == 4
                v, = struct.unpack_from("=i", buf)
                assert v == i
