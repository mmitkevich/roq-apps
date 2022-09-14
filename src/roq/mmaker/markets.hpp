#pragma once
#include "string_view"
#include "umm/core/type.hpp"
#include "absl/container/flat_hash_map.h"
#include <roq/string_types.hpp>

namespace roq {
namespace mmaker {


struct Markets {
    struct Item {
        std::string_view symbol;
        std::string_view exchange;
        umm::MarketIdent market;
    };
    template<class Fn>
    void get_markets(Fn&& fn) const {
        for(auto& [exchange, symbol_to_market] : exchange_to_symbols_) {
            for(auto& [symbol, market] : symbol_to_market) {
                fn(Item{.symbol=symbol, .exchange=exchange, .market=market});
            }
        }
    }
    void emplace(const Item& item) {
        exchange_to_symbols_[item.exchange][item.symbol] = item.market;
    }
    void clear() { exchange_to_symbols_.clear(); }

    umm::MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) {
        auto iter_1 = exchange_to_symbols_.find(exchange);
        if(iter_1 == std::end(exchange_to_symbols_)) {
            return {};
        }
        auto& symbol_to_market = iter_1->second;
        auto iter_2 = symbol_to_market.find(symbol);
        if(iter_2==std::end(symbol_to_market)) {
            return {};
        }
        return iter_2->second;
    }
    absl::flat_hash_map<roq::Exchange, absl::flat_hash_map<roq::Symbol, umm::MarketIdent>> exchange_to_symbols_;
};

} // namespace mmaker
} // namesapce roq