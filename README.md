lockfree-cpp
============

Some lock-free datastructures and tools in C++ that do not require C++11.

Included are
- A lock-free exchanger based on the JDK's java.util.concurrent.Exchanger
- An atomic tagged pointer type based on the JDK's
  java.util.concurrent.atomic.AtomicReference
- An intrusive lock-free stack (LIFO) that uses operation elimination as
  described in [A scalable lock-free stack algorithm](http://citeseer.ist.psu.edu/viewdoc/summary?doi=10.1.1.156.8728)
- An intrusive lock-free MPSC queue (FIFO) cribbed directly from the work of
  [Dmitry Vyukov](http://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue)

The datastructures themselves are header-only; you'll need pthreads and the
STL to compile the unit tests.

Thanks to [Phil Nash](http://www.levelofindirection.com/about-me/) for his 
[Catch](https://github.com/philsquared/Catch) unit test framework.
