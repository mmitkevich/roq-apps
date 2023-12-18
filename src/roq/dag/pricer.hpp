// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/basic_handler.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/exposure.hpp"
#include "roq/core/market.hpp"
#include "roq/core/portfolio.hpp"
#include "roq/core/quotes.hpp"
#include <concepts>
#include <roq/market_status.hpp>
#include <roq/string_types.hpp>
#include <roq/timer.hpp>
#include <roq/top_of_book.hpp>
#include <roq/market_by_price_update.hpp>
#include "roq/core/hash.hpp"
#include "roq/core/types.hpp"
#include <cassert>

// to oms
#include "roq/core/dispatcher.hpp"

//#include "roq/core/basic_pricer.hpp"

// from cache
#include "roq/core/handler.hpp"
#include "roq/core/dispatcher.hpp"
#include "roq/core/manager.hpp"

#include "roq/dag/node.hpp"

namespace roq::dag {

struct Node;

struct NodeKey {
    core::NodeIdent node;
    std::string_view name;
};

struct Pricer : core::Handler {
    Pricer(core::Dispatcher &dispatcher, core::Manager& core)
    : dispatcher(dispatcher) 
    , core(core)
    {}

    dag::Node *get_node(core::NodeIdent node_id);
    const dag::Node *get_node(core::MarketIdent market) const { return const_cast<dag::Pricer*>(this)->get_node(market); }

    bool get_node(core::NodeIdent node_id, std::invocable<const dag::Node&> auto && fn) const {
        const Node* node = get_node(node_id);
        if(node) {
            fn(*node);
            return true;
        }
        return false;
    }
    
    bool get_node(core::NodeIdent node_id, std::invocable<dag::Node&> auto && fn)  {
        Node* node = get_node(node_id);
        if(node) {
            fn(*node);
            return true;
        }
        return false;
    }


    bool get_node_by_market(core::MarketIdent market, std::invocable<dag::Node&> auto && fn) {
        auto iter = node_by_market.find(market);
        if(iter == std::end(node_by_market)) {
            return false;
        }
        return get_node(iter->second, fn);
    }

    std::pair<dag::Node&, bool> emplace_node(dag::NodeKey key);

    void operator()(const roq::Event<roq::MarketStatus>&);
    void operator()(const roq::Event<roq::Timer> &);

        // roq::core::Handler
    void operator()(const roq::Event<roq::core::Quotes> &) override;
    void operator()(const roq::Event<core::ExposureUpdate> &) override;

    void operator()(const roq::Event<roq::ParametersUpdate>& e) override;
public:
    void set_pipeline(core::NodeIdent node_id, const std::vector<std::string_view> & pipeline);
    bool set_mdata(core::NodeIdent node_id, core::Market const& market);
    bool set_portfolio(core::NodeIdent node_id, core::PortfolioKey const& portfolio);
    bool set_ref(core::NodeIdent node_id, std::string_view ref_node, std::string_view flags);

    bool get_path(core::MarketIdent market,  std::invocable<core::NodeIdent> auto && fn) {
        auto iter = paths.find(market);
        if(iter == std::end(paths))
            return false;
        for(auto& item: iter->second) {
            fn(item);
        }
        return true;
    }
    void get_refs(dag::Node const& node, auto&& fn) const {
        node.get_refs([&](dag::NodeRef const& r) {
            dag::Node const* n = get_node(r.node);
            if(!n)
                throw roq::RuntimeError("UNEXPECTED");
            fn(r, *n);
        });
    }
    void get_nodes(std::invocable<dag::Node&> auto&& fn) {
        for(auto& [id, node] : nodes) {
            fn(node);
        }
    }
    void send_target_quotes(core::NodeIdent node);
    void rebuild_paths();
  public:
    core::Dispatcher& dispatcher;
    core::Manager& core; 
    core::NodeIdent last_node_id = 0;
    core::Hash<core::NodeIdent, dag::Node> nodes;
    core::Hash<core::PortfolioIdent, core::Hash<core::MarketIdent, core::NodeIdent>> node_by_portfolio;
    core::Hash<core::MarketIdent, core::NodeIdent> node_by_market;
    core::Hash<std::string, core::NodeIdent> node_by_name; // deribit:BTC-P:A1 -> node identifier
    core::Hash<core::NodeIdent, std::vector<core::NodeIdent> > paths;
};



template<class Fn>
void Context::get_refs(Fn&& fn) const {
    node.get_refs([&](dag::NodeRef const& r) {
        dag::Node const* n = manager.get_node(r.node);
        if(!n)
            throw roq::RuntimeError("UNEXPECTED");
        fn(r, *n);
    });
}

} // roq::dag
