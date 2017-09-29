pyshm - Python share memory library
===================================

|Build Status| |PyPI version| |pyversions| |License|

pyshm is a python library that used to pass memory block between
multiple processes. it exports three modules:

1. ``shm``: mainly used to create share memory between multiple
   process(using ``mmap``)
2. ``mempool``: A memory pool is an allocator of fixed-size memory blocks.
   In python memory block is represented as a buffer-like object.
3. ``ring``: lockless FIFO queue in share memory. user can use ``Ring`` to
   manage the memory blocks allocated from Mempool

installation and usage
----------------------

you can install pyshm using setuptools or pip:

::

    pip install pyshm

or:

::

    python setup.py install

usage
-----

1. create share memory

   ::

       from pyshm import ShareMem

       test_shm = ShareMem(64*1024*1024) # 64M

   pls note: these code should execute before ``fork``

2. using ``mempool``

   ::

       from pyshm import mempool
       count = 4
       mp = mempool.Mempool(count, 4, shm=test_shm)

       pid = os.fork()
       if pid == 0:
           for i in range(count):
               buf = mp.get()
               struct.pack_into("=i", buf, 0, i)
               mp.put(buf)
       else:
           cid, status = os.wait()
           assert pid == cid
           for i in range(count):
               buf = mp.get()
               v, = struct.unpack_from("=i", buf)
               assert v == i
           mp.cleanup()

   ``get`` used to get memory block(a buffer-like object), ``put`` returns the
   memory block to the mempool

3. using ``ring``: ring can used to pass the memory block(get from mempool)
   to the other process. it proviod ``enqueue`` and ``dequeue``, one
   process enequeue and the other process ``dequeue``

.. |Build Status| image:: https://travis-ci.org/yuyang0/pyshm.svg?branch=master
   :target: https://travis-ci.org/yuyang0/pyshm
.. |PyPI version| image:: https://badge.fury.io/py/pyshm.svg
   :target: https://pypi.python.org/pypi/pyshm
.. |pyversions| image:: https://img.shields.io/badge/python-2.7,3.5,3.6,pypy-blue.svg
   :target: https://pypi.python.org/pypi/pyshm
.. |License| image:: https://img.shields.io/badge/License-BSD%202--Clause-orange.svg
   :target: https://opensource.org/licenses/BSD-2-Clause
