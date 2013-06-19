lockfree-cpp
============

Some lock-free datastructures and tools in C++ that do not require C++11.

Included are
- A lock-free exchanger
- An atomic tagged pointer type
- An intrusive lock-free stack (LIFO) that uses elimination during backoff
- An intrusive lock-free MPSC queue (FIFO)

The datastructures themselves are header-only; you'll need pthreads and the
STL to compile the unit tests.

Thanks to [Phil Nash](http://www.levelofindirection.com/about-me/) for his 
[Catch](https://github.com/philsquared/Catch) unit test framework.
