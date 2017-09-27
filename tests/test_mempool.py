#!/usr/bin/env python3
# -*- coding:utf-8 -*-

"""

"""
from __future__ import print_function, division, absolute_import
import sys
from os.path import dirname, abspath

sys.path.insert(0, dirname(abspath(__file__)))

from utils import test_shm
from pyshm import mempool


def test_mempool(test_shm):
    with mempool.Mempool(2, 4, shm=test_shm) as mp:
        buf = mp.get()
        assert len(buf) == 4
        buf[:4] = b"abcd"
        mp.put(buf)
        buf = mp.get()
        assert buf[:4] == b'abcd'
