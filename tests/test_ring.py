#!/usr/bin/env python3
# -*- coding:utf-8 -*-

"""

"""
from __future__ import print_function, division, absolute_import

import sys
from os.path import dirname, abspath

sys.path.insert(0, dirname(abspath(__file__)))

from utils import test_shm
from pyshm import ring


def test_ring(test_shm):
    assert 1
