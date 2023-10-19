#pragma once

#include <functional>
#include <cstdint>

namespace roq::pricer {

struct Node;
struct Manager;

struct Context {
    pricer::Node& node;
    pricer::Manager& pricer;
    uint32_t offset = 0;

    Context(pricer::Node& node, pricer::Manager& pricer) : node{node}, pricer{pricer} {}

#ifdef ROQ_CORE_NODE_STORAGE
    template<class T> 
    T& read();
#endif
};

using Compute = std::function<bool(Context& ctx)>;


static inline bool empty_compute(pricer::Node& node, pricer::Manager& pricer) { return false; }

#ifdef ROQ_CORE_NODE_STORAGE
template<class T> 
T& Context::read() { 
    assert(offset+sizeof(T)<=sizeof(node.storage)); 
    T* data = reinterpret_cast<T*>(node.storage+offset);
    offset += sizeof(T);
    return *data;
}
#endif



} // roq::pricer