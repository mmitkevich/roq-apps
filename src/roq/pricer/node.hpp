#pragma once

#include "roq/core/best_quotes.hpp"
#include "roq/core/quote.hpp"
#include "roq/core/types.hpp"
#include "roq/pricer/aggr_type.hpp"
#include <roq/parameters_update.hpp>
#include <roq/string_types.hpp>

namespace roq::pricer {

struct Manager;

struct NodeRef {
    core::MarketIdent market;
    core::Double weight;
};

struct Node;

// pipeline step result
struct Context {
    // result (will be state of the virtual instrument after computation)
    core::BestQuotes quotes;
    core::Double exposure;

    void clear();

    // key to parameter in the node
    ptrdiff_t key = 0;
};

// context is both input and output 
struct Compute {
    virtual ~Compute()=default;
    virtual bool operator()(pricer::Context& context, pricer::Node const& node, pricer::Manager & manager) const= 0;
    virtual bool operator()(pricer::Context& context, roq::ParametersUpdate const& ) { return false; }
};


struct Node {
    core::MarketIdent market; // receive from or publish into
    roq::Symbol symbol;
    roq::Exchange exchange;

    core::BestQuotes quotes;
    core::Double exposure;

    core::Double multiplier {1.0};

    void clear();
    // update from computed values, returns if changed
    bool update(Context const &rhs);
    bool update(core::Quotes const &rhs);
    // update using pipeline, return if changed
    bool compute(pricer::Manager& manager);

    bool operator()(roq::Event<roq::ParametersUpdate> const& e);

    // cache-friendly refs storage
    static constexpr uint32_t MAX_REFS_SIZE = 16;
    uint32_t refs_size {0};
    std::array<NodeRef, MAX_REFS_SIZE> refs;

    // enumerate refs
    void get_refs(auto&& fn) const {
        for(std::size_t i = 0; i < refs_size; i++) {
            fn(refs[i]);
        }
    }

    // cache-friendly pipeline pointers storage 
    static constexpr uint32_t MAX_PIPELINE_SIZE = 16;
    uint32_t pipeline_size {0};
    std::array<Compute*, MAX_PIPELINE_SIZE> pipeline;

    // enumerate pipeline
    void get_pipeline(auto&& fn) {
        for(std::size_t i = 0; i < pipeline_size; i++) {
            fn(*pipeline[i]);
        }
    }

    // cache friendly parameters storage
    std::byte parameters[256];

    // fetch element of type T from the storage and increment key
    template<class T>
    const T& fetch_parameter(ptrdiff_t& key) const;
};


bool empty_compute(pricer::Context& context, pricer::Node const& node, pricer::Manager & manager);

template<class T>
const T& Node::fetch_parameter(ptrdiff_t& offset) const {
    assert(offset+sizeof(T)<=sizeof(parameters)); 
    const T* data = reinterpret_cast<const T*>(parameters+offset);
    offset += sizeof(T);
    return *data;
}


} // roq::pricer