#pragma once

#include "roq/pricer/node.hpp"

#include <roq/exceptions.hpp>

#include "roq/pricer/aggr/aggr_type.hpp"
#include "roq/pricer/aggr/product.hpp"
#include "roq/pricer/aggr/sum.hpp"
namespace roq::pricer {

struct Manager;

struct Context {
    pricer::Node& node;
    pricer::Manager& manager;
    uint32_t offset = 0;

    Context(pricer::Node& node, pricer::Manager& manager)
    : node{node}, manager{manager} {}

    bool get_aggr(aggr::AggrType aggr, auto&& fn);

    template<class T>
    T& get_arg(ptrdiff_t& offset);

    std::size_t get_refs(std::invocable<pricer::NodeRef const&, pricer::Node const&> auto && fn);


};

template<class T> 
T& Context::get_arg(ptrdiff_t& offset){ 
    assert(offset+sizeof(T)<=sizeof(node.storage)); 
    T* data = reinterpret_cast<T*>(node.storage+offset);
    offset += sizeof(T);
    return *data;
}


bool Context::get_aggr(aggr::AggrType aggr, auto&& fn) {
    switch(aggr) {
        case aggr::AggrType::SUM: fn(aggr::Sum{});
        case aggr::AggrType::PRODUCT: fn(aggr::Product{});
        default: 
            throw roq::RuntimeError("UNEXPECTED");
            return false;
    }
    return true;
}


template<class Compute>
bool Context::compute() {
    pricer::Quotes aggregated_quotes;
    
    get_aggr(node.aggr_type, [&](auto&& aggregate) {
        aggregate(aggregated_quotes, [&](std::invocable<pricer::NodeRef const&, pricer::Node const&> auto && next_item) { 
            return this->get_refs(std::forward<decltype(next_item)>(next_item));
        });
    });

    Compute& compute = get_arg<Compute>(offset);
    compute(node, aggregated_quotes);
}

} // roq::pricer