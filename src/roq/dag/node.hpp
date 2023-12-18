#pragma once

#include "roq/core/best_quotes.hpp"
#include "roq/core/fmt.hpp"
#include "roq/core/market.hpp"
#include "roq/core/quote.hpp"
#include "roq/core/types.hpp"
#include "roq/dag/aggr_type.hpp"
#include <roq/parameters_update.hpp>
#include <roq/string_types.hpp>
#include "roq/dag/compute.hpp"

namespace roq::core {
    using NodeIdent = core::Ident;
}

namespace roq::dag {

struct Pricer;

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
    roq::Mask<dag::NodeFlags> flags;
};

struct Context {
    Pricer & manager;
    Node& node;

    ptrdiff_t offset {0};    // of parameters in the node

    // result (will be state of the virtual instrument after computation)
    core::BestQuotes quotes;
    core::Double exposure;

    // fetch element of type T from the storage and increment key
    template<class T>
    T& get_parameters();

    template<class Fn>
    void get_refs(Fn&& fn) const;

    void clear() {
        quotes = {};
    }

    // key to parameter in the node
    //ptrdiff_t key = 0;
};


struct Node {
    
    dag::Pricer& manager;

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

    // update from computed values, returns if changed
    bool update(dag::Context const &rhs);
    bool update(core::Quotes const &rhs);
    bool update(std::span<const roq::Parameter> const update);
    // update using pipeline, return if changed
    bool compute();

    // cache-friendly refs storage
    static constexpr uint32_t MAX_REFS_SIZE = 16;
    uint32_t refs_size {0};
    //std::array<NodeRef, MAX_REFS_SIZE> refs;
    NodeRef refs[MAX_REFS_SIZE];

    // enumerate refs
    void get_refs(auto&& fn) const {
        for(std::size_t i = 0; i < refs_size; i++) {
            fn(refs[i]);
        }
    }

    dag::NodeRef& add_ref(core::NodeIdent ref_node, roq::Mask<NodeFlags> flags = {}) {
        NodeRef& ref = refs[refs_size];
        ref = NodeRef {
            .node = ref_node,
            .flags = roq::Mask{flags}
        };
        refs_size++;
        return ref;
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

    void clear();

    // cache friendly parameters storage
    std::byte parameters[256];
};


//bool empty_compute(dag::Context& context, dag::Node const& node, dag::Manager & manager);

template<class Parameters>
Parameters& Context::get_parameters() {
    assert(offset+sizeof(Parameters)<=sizeof(node.parameters)); 
    Parameters* data = reinterpret_cast<Parameters*>(node.parameters + this->offset);
    return *data;
}

} // roq::dag

ROQ_CORE_FMT_DECL(roq::dag::NodeFlags, "", magic_enum::enum_name(_))