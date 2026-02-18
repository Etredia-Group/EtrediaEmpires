module;

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>

export module utils:lockfree;

export namespace boost
{
    using lockfree::capacity;

    using lockfree::queue;
    using lockfree::spsc_queue;
}