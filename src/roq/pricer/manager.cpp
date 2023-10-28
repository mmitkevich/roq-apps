// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/types.hpp"
#include "roq/pricer/manager.hpp"
#include <roq/message_info.hpp>
#include <roq/parameters_update.hpp>
//#include "roq/core/dispatcher.hpp"

namespace roq::pricer {

Manager::Manager(pricer::Handler& handler, core::Manager& core ) 
: handler(&handler) 
, core(core)
{}

void Manager::operator()(const Event<roq::Timer> &event) {

}

void Manager::operator()(const Event<core::ExposureUpdate> &event) {
    for(auto& exposure : event.value.exposure) {
        auto [node, is_new] = emplace_node(exposure.market, exposure.symbol, exposure.exchange);
        node.exposure = exposure.exposure;
    }
}

void Manager::operator()(const roq::Event<roq::MarketStatus>&) {

}

void Manager::operator()(const roq::Event<roq::ParametersUpdate>& e) {
    get_nodes([&](auto& node) {
        node(e);
    });
}

void Manager::operator()(const Event<core::Quotes> &event) {
    auto & quotes = event.value;
    auto [node, is_new] = emplace_node(quotes.market, quotes.symbol, quotes.exchange);
    node.update(quotes);

    log::debug<2>("pricer::Quotes Node market={} symbol={} exchange={} bid={} ask={}",node.market, quotes.symbol, quotes.exchange, node.quotes.bid, node.quotes.ask);
    
    target_quotes(node);
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
    assert(node.market!=0);
    core::TargetQuotes quotes {
        .market = node.market,
        .symbol = node.symbol,
        .exchange = node.exchange,
        .bids = std::span {&node.quotes.bid, 1}, 
        .asks = std::span {&node.quotes.ask, 1}
    };
    roq::MessageInfo info {};
    roq::Event event {info, quotes};
    log::debug<2>("pricer::handler={}",(void*)handler);
    (*handler)(event);
}

Node *Manager::get_node(core::MarketIdent market) {
  auto iter = nodes.find(market);
  if (iter == std::end(nodes))
    return &iter->second;
  return nullptr;
}

std::pair<Node&, bool> Manager::emplace_node(core::MarketIdent market_id, std::string_view symbol, std::string_view exchange) {
  auto [iter, is_new] = nodes.try_emplace(market_id);
  if(is_new) {
    auto& r = iter->second;
    r.market = market_id;
    r.symbol = symbol;
    r.exchange = exchange;
  }
  return {iter->second, is_new};
}

} // namespace roq::pricer
