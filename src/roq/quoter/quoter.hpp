#pragma once

#include "roq/core/exposure.hpp"
#include "roq/core/quotes.hpp"
#include <roq/market_status.hpp>
#include <roq/timer.hpp>
#include <roq/top_of_book.hpp>
#include <roq/market_by_price_update.hpp>
#include "roq/core/hash.hpp"
#include "roq/core/types.hpp"
#include <cassert>

namespace roq::quoter {

struct Quoter;
struct Node;

struct Context {
    Node& node;
    Quoter& quoter;
    uint32_t offset = 0;

    Context(Node& node, Quoter& quoter) : node{node}, quoter{quoter} {}

//    template<class T> 
//    T& read();
};

using Compute = std::function<bool(Context& ctx)>;

struct Node {
    struct Ref {
        core::MarketIdent market;
        core::Double weight;
    };

    core::Quote bid;
    core::Quote ask;
    core::Double exposure;
    std::vector<Ref> refs;
    core::Double multiplier {1.0};
    core::MarketIdent market; // publish into

    std::vector<Compute> pipeline;
     
//    char storage[256];   // inplace storage
};

/*
template<class T> 
T& NodeData::read() { 
    assert(offset+sizeof(T)<=sizeof(node.storage)); 
    T* data = reinterpret_cast<T*>(node.storage+offset);
    offset += sizeof(T);
    return *data;
}
*/


static inline bool empty_compute(Node& node, Quoter& quoter) { return false; }

struct Quoter {
    struct Dispatcher {
        virtual void send(core::Quotes const&) = 0;
    };

    Quoter(Dispatcher &dispatcher);

    Node *get_node(core::MarketIdent market);

    std::pair<Node&, bool> emplace_node(core::MarketIdent market, Node&& node = {});


    virtual void operator()(const roq::Timer &);
    virtual void operator()(const roq::core::Quotes &);
    virtual void operator()(const core::Exposure &);    

  public:
    Dispatcher& dispatcher;
    core::Hash<core::MarketIdent, Node> nodes;
    core::Hash<core::MarketIdent, std::vector<core::MarketIdent> > paths;

    template<class Fn>
    bool get_path(core::MarketIdent market,  Fn&& fn) {
        auto iter = paths.find(market);
        if(iter == std::end(paths))
            return false;
        for(auto& item: iter->second) {
            fn(item);
        }
        return true;
    }
};


} // roq::mmaker