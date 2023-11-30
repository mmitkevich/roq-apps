// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/types.hpp"
#include "roq/pricer/manager.hpp"
#include <fmt/format.h>
#include <roq/exceptions.hpp>
#include <roq/message_info.hpp>
#include <roq/parameters_update.hpp>
#include <roq/string_types.hpp>
#include "roq/pricer/factory.hpp"
#include "roq/pricer/node.hpp"
//#include "roq/core/dispatcher.hpp"
#include "roq/core/utils/string_utils.hpp"
#include "roq/mask.hpp"

namespace roq::pricer {

Manager::Manager(pricer::Handler& handler, core::Manager& core ) 
: handler(&handler) 
, core(core)
{}

void Manager::operator()(const Event<roq::Timer> &event) {

}

void Manager::operator()(const Event<core::ExposureUpdate> &event) {
    for(auto& e : event.value.exposure) {
        // TODO: find all nodes subscribed to e.portfolio and update their exposure
    }
}

void Manager::operator()(const roq::Event<roq::MarketStatus>&) {

}




void Manager::operator()(const roq::Event<roq::ParametersUpdate>& e) {
    using namespace std::literals;    
    
    log::debug("pricer: parameters_update {}"sv, e);


    for(const roq::Parameter& p: e.value.parameters) {
        std::string node_name = fmt::format("{}:{}", p.exchange, p.symbol);
        auto [node, is_new] = emplace_node({
            .name = node_name
        });
        log::debug("pricer param label {} exchange {} symbol {} value {}", p.label, p.exchange, p.symbol, p.value);
        // label = ref.weight mdata exec compute.parameter
        //for(const auto tok : std::views::split(std::string_view{p.label}, '.')) {
        //    auto token = std::string_view { tok.data(), tok.size() };
        auto[label, suffix] = core::utils::split_suffix(p.label, ':');
        if(label == "mdata"sv) {
            auto [exchange, symbol] = core::utils::split_prefix(p.value, ':');
            set_mdata(node, {
                .symbol = symbol,
                .exchange = exchange
            });
        } else if(label == "portfolio"sv) {
            set_portfolio(node, {.name = p.value});
        } else if(label == "ref"sv) {
            set_ref(node, p.value, suffix);
            // TOOD: add refs
        } else if(label == "pipeline"sv) {
            auto vec = core::utils::split_sep(p.value, ' ');
            node.set_pipeline(vec);
        } else {
            node.update(std::span {&p, 1});
        }   
        //}
        fail:;
    }
}

void Manager::operator()(const Event<core::Quotes> &event) {
    auto & quotes = event.value;
    //auto [node, is_new] = emplace_node({
   //     .node=quotes.market, .symbol=quotes.symbol, .exchange=quotes.exchange
    //});
    //node.update(quotes);
    
    // TODO: for all nodes with mdata linked to event.market update quotes

    //log::debug<2>("pricer::Quotes Node market={} symbol={} exchange={} bid={} ask={}",node.market, quotes.symbol, quotes.exchange, node.quotes.bid, node.quotes.ask);
    
//    target_quotes(node);
/*
    /// compute dependents and publish
    get_path(quotes.market, [&](core::MarketIdent item) {
        bool changed = false;
        Context context{node, *this};
        for(auto& compute: node.pipeline) {
            if(compute(context))
                changed = true;
        }
        if(node.market && changed) {
            target_quotes(node);
        }
    });
*/        
}

void Manager::target_quotes(Node& node) {
    assert(node.exec!=0);
    core.markets.get_market(node.exec, [&](auto& market) {
        core::TargetQuotes quotes {
            .market = market.market,
            .symbol = market.symbol,
            .exchange = market.exchange,
            .buy = std::span {&node.quotes.buy, 1}, 
            .sell = std::span {&node.quotes.sell, 1}
        };
        roq::MessageInfo info {};
        roq::Event event {info, quotes};
        (*handler)(event);
    });
}

Node *Manager::get_node(core::MarketIdent market) {
  auto iter = nodes.find(market);
  if (iter == std::end(nodes))
    return &iter->second;
  return nullptr;
}

std::pair<pricer::Node&, bool> Manager::emplace_node(pricer::NodeKey key) {
    auto node_id = key.node;
    if(node_id==0) {
        auto iter = node_by_name.find(key.name);
        if(iter!=std::end(node_by_name)) {
            node_id = iter->second;
        }
    }
    if(node_id==0) {
        node_id = ++last_node_id;
    }
    auto [iter, is_new_node] = nodes.try_emplace(node_id, *this);
    auto& node = iter->second;    
    if(is_new_node) {
        node.node = node_id;
        node.name = key.name;
        log::debug("pricer: emplace node.{} {}", node.node, node.name);
    }
    return {node, is_new_node};
}

bool Manager::set_mdata(pricer::Node& node, core::Market const& market_key) {
    if(node.flags.has(NodeFlags::INPUT)) {
        throw roq::RuntimeError("pricer: input for node already specified");
    }

    auto [market, is_new_market] = core.markets.emplace_market(market_key);
    node.mdata = market_key.market;
    node_by_market[node.mdata] = node.node;
    if(node.portfolio) {
        node_by_portfolio[node.portfolio][node.mdata] = node.node;
    }
    node.flags |= NodeFlags::INPUT;
    node.flags |= NodeFlags::PRICE;
    return true;
}


bool Manager::set_ref(pricer::Node& node, std::string_view ref_node_name, std::string_view flags_sv) {
    auto [ref_node, is_new] = emplace_node({.name = ref_node_name});
    std::string flags_str = core::utils::to_upper(flags_sv);
    //TODO: parse FLAG1|FLAG2|FLAG3 mask
    std::optional<pricer::NodeFlags> flags_opt = magic_enum::enum_cast<pricer::NodeFlags>(flags_str);
    if(flags_opt.has_value()) {
        ref_node.flags.set(flags_opt.value());
    } else if (!flags_sv.empty()){
        log::info("pricer: failed to parse node flags {}", flags_sv);
    }
    node.add_ref(ref_node.node, ref_node.flags);
    log::debug("pricer: add ref node.{} -> node.{} flags {}", node.node, ref_node.node, ref_node.flags);
    return true;
}

bool Manager::set_portfolio(pricer::Node& node, core::PortfolioKey const& portfolio_key) {
    if(node.flags.has(NodeFlags::INPUT)) {
        throw roq::RuntimeError("input for node already specified");
    }
    auto [portfolio, is_new_portfolio] = core.portfolios.emplace_portfolio(portfolio_key);
    node.portfolio = portfolio_key.portfolio;
    if(node.mdata) {
        node_by_portfolio[node.portfolio][node.mdata] = node.node;
    }
    node.flags |= NodeFlags::INPUT;    
    node.flags |= NodeFlags::EXPOSURE;
    return true;
}


} // namespace roq::pricer
