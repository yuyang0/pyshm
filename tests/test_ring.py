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
from pyshm import ring, mempool


def test_multiple_ring(test_shm):
    count = 4
    r1 = ring.Ring(64, 0, shm=test_shm)
    r2 = ring.Ring(64, 0, shm=test_shm)
    mp = mempool.Mempool(count, 4, shm=test_shm)

    pid = os.fork()
    if pid == 0:
        for i in range(count):
            buf = mp.get()
            struct.pack_into("=i", buf, 0, i)
            r1.enqueue(buf)
    else:
        cid, status = os.wait()
        assert pid == cid
        assert r1.used_count == count

        for i in range(count):
            buf = r1.dequeue(4)
            assert len(buf) == 4
            v, = struct.unpack_from("=i", buf)
            assert v == i
        mp.cleanup()
        r1.cleanup()
        r2.cleanup()
