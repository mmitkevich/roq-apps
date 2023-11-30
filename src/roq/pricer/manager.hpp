// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/basic_handler.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/exposure.hpp"
#include "roq/core/market.hpp"
#include "roq/core/portfolio.hpp"
#include "roq/core/quotes.hpp"
#include <roq/market_status.hpp>
#include <roq/string_types.hpp>
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

#include "roq/core/manager.hpp"

#include "roq/pricer/handler.hpp"

#include "roq/pricer/node.hpp"

namespace roq::pricer {

struct Node;

struct NodeKey {
    core::NodeIdent node;
    std::string_view name;
};

struct Manager : core::Handler {
    Manager(pricer::Handler &handler, core::Manager& core);

    pricer::Node *get_node(core::MarketIdent market);
    const pricer::Node *get_node(core::MarketIdent market) const { return const_cast<Manager*>(this)->get_node(market); }

    std::pair<pricer::Node&, bool> emplace_node(pricer::NodeKey key);

    void operator()(const roq::Event<roq::MarketStatus>&);
    void operator()(const roq::Event<roq::Timer> &);

        // roq::core::Handler
    void operator()(const roq::Event<roq::core::Quotes> &) override;
    void operator()(const roq::Event<core::ExposureUpdate> &) override;

    void operator()(const roq::Event<roq::ParametersUpdate>& e) override;
public:
    void set_pipeline(pricer::Node&node, const std::vector<std::string_view> & pipeline);
    bool set_mdata(pricer::Node& node, core::Market const& market);
    bool set_portfolio(pricer::Node& node, core::PortfolioKey const& portfolio);
    bool set_ref(pricer::Node& node, std::string_view ref_node, std::string_view flags);

    bool get_path(core::MarketIdent market,  std::invocable<core::MarketIdent> auto && fn) {
        auto iter = paths.find(market);
        if(iter == std::end(paths))
            return false;
        for(auto& item: iter->second) {
            fn(item);
        }
        return true;
    }
    void get_refs(pricer::Node const& node, auto&& fn) const {
        node.get_refs([&](pricer::NodeRef const& r) {
            pricer::Node const* n = get_node(r.node);
            if(!n)
                throw roq::RuntimeError("UNEXPECTED");
            fn(r, *n);
        });
    }
    void get_nodes(auto&& fn) {
        for(auto& [id, node] : nodes) {
            fn(node);
        }
    }
    void target_quotes(pricer::Node& node);
  public:
    core::NodeIdent last_node_id = 0;
    core::Manager& core; 
    pricer::Handler* handler {};
    core::Hash<core::NodeIdent, pricer::Node> nodes;
    core::Hash<core::PortfolioIdent, core::Hash<core::MarketIdent, core::NodeIdent>> node_by_portfolio;
    core::Hash<core::MarketIdent, core::NodeIdent> node_by_market;
    core::Hash<std::string, core::NodeIdent> node_by_name; // deribit:BTC-P:A1 -> node identifier
    core::Hash<core::NodeIdent, std::vector<core::NodeIdent> > paths;
};



template<class Fn>
void Context::get_refs(Fn&& fn) const {
    node.get_refs([&](pricer::NodeRef const& r) {
        pricer::Node const* n = manager.get_node(r.node);
        if(!n)
            throw roq::RuntimeError("UNEXPECTED");
        fn(r, *n);
    });
}

} // roq::pricer
