// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/quote.hpp"
#include "roq/pricer/compute.hpp"
#include <roq/string_types.hpp>

namespace roq::pricer {


struct Node {
    struct Ref {
        core::MarketIdent market;
        core::Double weight;
        void *ctx; // user context
    };

    core::MarketIdent market; // receive from or publish into
    roq::Symbol symbol;
    roq::Exchange exchange;

    core::Quote bid;
    core::Quote ask;
    core::Double exposure;
    core::Double multiplier {1.0};

    static constexpr uint32_t MAX_REFS_SIZE = 16;
    uint32_t refs_size {0};
    std::array<Ref, MAX_REFS_SIZE> refs;

    static constexpr uint32_t MAX_PIPELINE_SIZE = 16;
    uint32_t pipeline_size {0};
    std::array<Compute, MAX_PIPELINE_SIZE> pipeline;

    void* ctx;  // user context
#ifdef ROQ_CORE_NODE_STORAGE
    char storage[256];   // inplace storage, see utils.hpp
#endif
};


} // roq::pricer