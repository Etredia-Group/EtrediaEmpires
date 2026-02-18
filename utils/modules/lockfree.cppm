module;

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>

export module utils:lockfree;

export using boost::lockfree::capacity;

export using boost::lockfree::queue;
export using boost::lockfree::spsc_queue;