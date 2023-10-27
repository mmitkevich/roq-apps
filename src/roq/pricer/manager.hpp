// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/basic_handler.hpp"
#include "roq/core/exposure.hpp"
#include "roq/core/quotes.hpp"
#include <roq/market_status.hpp>
#include <roq/timer.hpp>
#include <roq/top_of_book.hpp>
#include <roq/market_by_price_update.hpp>
#include "roq/core/hash.hpp"
#include "roq/core/types.hpp"
#include <cassert>

// to oms
//#include "roq/core/dispatcher.hpp"

// from cache
#include "roq/core/handler.hpp"

#include "roq/pricer/node.hpp"

#include "roq/core/manager.hpp"

#include "roq/pricer/handler.hpp"

namespace roq::pricer {

struct Node;

struct Manager : core::Handler {
    Manager(pricer::Handler &handler, core::Manager& core);

    pricer::Node *get_node(core::MarketIdent market);

    std::pair<pricer::Node&, bool> emplace_node(core::MarketIdent market, std::string_view symbol, std::string_view exchange);
    
    void operator()(const roq::Event<roq::MarketStatus>&);
    void operator()(const roq::Event<roq::Timer> &);

        // roq::core::Handler
    void operator()(const roq::Event<roq::core::Quotes> &) override;
    void operator()(const roq::Event<core::ExposureUpdate> &) override;
public:
    bool get_path(core::MarketIdent market,  std::invocable<core::MarketIdent> auto && fn) {
        auto iter = paths.find(market);
        if(iter == std::end(paths))
            return false;
        for(auto& item: iter->second) {
            fn(item);
        }
        return true;
    }
    void target_quotes(Node& node);
  public:
    core::Manager& core; 
    pricer::Handler* handler;
    core::Hash<core::MarketIdent, pricer::Node> nodes;
    core::Hash<core::MarketIdent, std::vector<core::MarketIdent> > paths;
};


} // roq::pricer