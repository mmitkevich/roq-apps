#pragma once

#include "roq/core/best_quotes.hpp"
#include "roq/core/market.hpp"
#include "roq/core/quote.hpp"
#include "roq/core/types.hpp"
#include "roq/pricer/aggr_type.hpp"
#include <roq/parameters_update.hpp>
#include <roq/string_types.hpp>
#include "roq/pricer/compute.hpp"

namespace roq::core {
    using NodeIdent = core::Ident;
}

namespace roq::pricer {

struct Manager;

enum class NodeFlags : uint64_t{
    UNDEFINED       = 0,
    INPUT           = 1,  // node is used as market data input
    PRICE           = 2,
    EXPOSURE        = 4,
};

struct NodeRef {
    core::NodeIdent node;
    core::Double weight;
    core::Double denominator = {1.0};     // denominator
    roq::Mask<pricer::NodeFlags> flags;
};


struct Node {
    
    pricer::Manager& manager;

    // node identification
    core::NodeIdent node;
    roq::Symbol name;

    core::MarketIdent mdata;        // mdata (real) market (input)
    core::MarketIdent exec;         // exec (real) market (output)
    roq::Account account;           // account (for exec)
    core::PortfolioIdent portfolio;  // associated portfolio (for exec)

    core::BestQuotes quotes;        // quotes, either market data, or calculated

    //core::Double exposure;          // exposure

    core::Double multiplier {1.0};
    
    roq::Mask<NodeFlags> flags {};

    Context make_context() { return {.manager = manager, .node = *this}; }

    void clear();
    // update from computed values, returns if changed
    bool update(pricer::Context const &rhs);
    bool update(core::Quotes const &rhs);
    bool update(std::span<const roq::Parameter> const update);
    // update using pipeline, return if changed
    bool compute();

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

    void set_pipeline(const std::span<std::string_view>  vec);

    // enumerate pipeline
    void get_pipeline(auto&& fn) {
        for(std::size_t i = 0; i < pipeline_size; i++) {
            fn(*pipeline[i]);
        }
    }

    // cache friendly parameters storage
    std::byte parameters[256];
};


//bool empty_compute(pricer::Context& context, pricer::Node const& node, pricer::Manager & manager);

template<class Parameters>
Parameters& Context::get_parameters() {
    assert(offset+sizeof(Parameters)<=sizeof(node.parameters)); 
    Parameters* data = reinterpret_cast<Parameters*>(node.parameters + this->offset);
    return *data;
}


} // roq::pricer