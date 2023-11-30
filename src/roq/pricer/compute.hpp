#pragma once

#include "roq/core/types.hpp"
#include "roq/core/best_quotes.hpp"
#include <roq/parameters_update.hpp>

namespace roq::pricer {

struct Node;
struct Manager;

// computation context, used to exchange parameters and results with the Node
struct Context;

// algorithm 
struct Compute {
    uint32_t size {0};      // of state struct    

    Compute(uint32_t size = 0)
    : size{size} {}

    virtual ~Compute() = default;

    /// node contains prices, manager contains all nodes, context is filled with result (and used to resolve parameters)
    virtual bool operator()(pricer::Context& context) const = 0;

    /// configure algorithm
    virtual bool operator()(pricer::Context& context, std::span<const roq::Parameter> )  { return true; }
};

} // roq::pricer