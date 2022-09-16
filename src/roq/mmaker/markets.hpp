#pragma once
#include "string_view"
#include "umm/core/type.hpp"
#include "absl/container/flat_hash_map.h"
#include <absl/container/node_hash_map.h>
#include <roq/string_types.hpp>
#include <roq/logging.hpp>

namespace roq {
namespace mmaker {

using MarketIdent = umm::MarketIdent;

struct Markets {
    struct ExchangeSymbol {
        roq::Exchange exchange;        
        roq::Symbol symbol;
    };
    struct Item {
        std::string_view symbol;
        std::string_view exchange;
        MarketIdent market;
    };
    template<class Fn>
    void get_markets(Fn&& fn) const {
        for(auto& [exchange, symbol_to_market] : symbol_to_market_) {
            for(auto& [symbol, market] : symbol_to_market) {
                fn(Item{.symbol=symbol, .exchange=exchange, .market=market});
            }
        }
    }
    void emplace(const Item& item) {
        auto& sym = market_to_symbol_[item.market] = ExchangeSymbol{.exchange=item.exchange, .symbol=item.symbol};
        symbol_to_market_[sym.exchange][sym.symbol] = item.market;
    }
    
    template<class Context, class Config>
    void configure(Context& context, const Config& config) {
        clear();
        config.get_nodes("market",[&](auto market_node) {
            auto symbol = config.get_string(market_node, "symbol");
            auto exchange = config.get_string(market_node, "exchange");
            auto market_str = config.get_string(market_node, "market");
            umm::MarketIdent market = context.get_market_ident(market_str);
            log::info<1>("symbol {}, exchange {}, market {} {}", symbol, exchange, market.value, context.prn(market));
            emplace({.symbol=symbol, .exchange=exchange, .market=market});
        });    
    }

    void clear() { symbol_to_market_.clear(); }

    umm::MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) const {
        auto iter_1 = symbol_to_market_.find(exchange);
        if(iter_1 == std::end(symbol_to_market_)) {
            return {};
        }
        auto& symbol_to_market = iter_1->second;
        auto iter_2 = symbol_to_market.find(symbol);
        if(iter_2==std::end(symbol_to_market)) {
            return {};
        }
        return iter_2->second;
    }
    template<class Fn>
    bool get_market(umm::MarketIdent market, Fn&& fn) const {
        auto iter = market_to_symbol_.find(market);
        if(iter != std::end(market_to_symbol_)) {
            fn(Item{.symbol = iter->second.symbol, .exchange = iter->second.exchange, .market=market, });
            return true;
        }
        return false;
    }
private:
    absl::flat_hash_map<roq::Exchange, absl::flat_hash_map<roq::Symbol, umm::MarketIdent> > symbol_to_market_;
    absl::node_hash_map<umm::MarketIdent, ExchangeSymbol> market_to_symbol_;
};

} // namespace mmaker
} // namesapce roq