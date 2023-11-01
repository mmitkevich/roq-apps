// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/types.hpp"
#include "roq/pricer/manager.hpp"
#include <roq/exceptions.hpp>
#include <roq/message_info.hpp>
#include <roq/parameters_update.hpp>
#include <ranges>
//#include "roq/core/dispatcher.hpp"

namespace roq::pricer {

Manager::Manager(pricer::Handler& handler, core::Manager& core ) 
: handler(&handler) 
, core(core)
{}

void Manager::operator()(const Event<roq::Timer> &event) {

}

void Manager::operator()(const Event<core::ExposureUpdate> &event) {
    for(auto& e : event.value.exposure) {
        auto [node, is_new] = emplace_node({
            .market = e.market, 
            .symbol = e.symbol,
            .exchange = e.exchange
        });
        node.exposure = e.exposure;
    }
}

void Manager::operator()(const roq::Event<roq::MarketStatus>&) {

}

void Manager::operator()(const roq::Event<roq::ParametersUpdate>& e) {
    enum Flags {
        MDATA = 1,
        QUOTE = 2,
        REF = 4,
        SET = 5,
        APPLY = 6,
    };
    
    uint64_t flags = 0;

    get_nodes([&](auto& node) {
        for(const roq::Parameter& p: e.value.parameters) {
            pricer::Node* node = nullptr;
            using namespace std::literals;
            for(const auto tok : std::views::split(std::string_view{p.label}, " "sv)) {
                auto token = std::string_view{tok.data(), tok.size()};
                if(token == "mdata"sv) {
                    node = &emplace_node( {
                        .symbol = p.symbol,
                        .exchange = p.exchange
                    }).first;
                    flags |= MDATA;
                } else if (token == "quote"sv) {
                    roq::Symbol symbol;
                    fmt::format_to(std::back_inserter(symbol), "{}:{}", token, p.symbol);

                    node = &emplace_node( {
                        .symbol = symbol,       // quote:BTC-PERPETUAL instead of BTC-PERPETUAL
                        .exchange = p.exchange
                    }).first;
                    flags |= QUOTE;
                } else if (token == "to"sv) {
                    if(flags!=MDATA) {
                        log::info("unexpected token '{}'", token);
                        goto fail;
                    }
                    flags |= REF;
                } else if(token == "set"sv) {
                    if(!(flags==MDATA || flags==QUOTE)) {
                        log::info("unexpected token '{}'", token);
                        goto fail;
                    }
                    flags |= SET;
                } else if(token == "apply"sv) {
                    if(!(flags==QUOTE)) {
                        log::info("unexpected token '{}'", token);
                    }
                    flags |= APPLY;
                }
            }
            fail:;
        }
        node(e);
    });
}

void Manager::operator()(const Event<core::Quotes> &event) {
    auto & quotes = event.value;
    auto [node, is_new] = emplace_node({.market=quotes.market, .symbol=quotes.symbol, .exchange=quotes.exchange});
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
    assert(node.market.market!=0);
    core::TargetQuotes quotes {
        .market = node.market.market,
        .symbol = node.market.symbol,
        .exchange = node.market.exchange,
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

std::pair<pricer::Node&, bool> Manager::emplace_node(core::Market args) {
    auto market_id = args.market;
    auto [market, is_new_market] = core.markets.emplace_market(args);
    
    if(0==market_id)
        market_id = market.market;
    
    assert(market_id);
    
    auto [iter, is_new_node] = nodes.try_emplace(market_id);
    auto& node = iter->second;    
    if(is_new_node) {
        node.market = market;
    }
  return {node, is_new_node};
}

} // namespace roq::pricer
