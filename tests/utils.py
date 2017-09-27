#!/usr/bin/env python3
# -*- coding:utf-8 -*-

"""

"""
from __future__ import print_function, division, absolute_import
import pytest
import pyshm
from pyshm import ShareMem


@pytest.fixture(scope="module")
def test_shm(request):
    with ShareMem(64*1024*1024) as shm:   # 64M
        yield shm
