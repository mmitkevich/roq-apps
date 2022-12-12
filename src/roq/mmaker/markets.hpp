#pragma once
#include "roq/mmaker/best_price_source.hpp"
#include "string_view"
#include "umm/core/type.hpp"
#include "absl/container/flat_hash_map.h"
#include <absl/container/node_hash_map.h>
#include <roq/string_types.hpp>
#include <roq/logging.hpp>

namespace roq {
namespace mmaker {

using MarketIdent = umm::MarketIdent;

struct MarketInfo {
    umm::MarketIdent market;
    std::string_view symbol;
    std::string_view exchange;
};

struct SymbolExchange {
    roq::Exchange exchange;        
    roq::Symbol symbol;

    template <typename H>
    friend H AbslHashValue(H h, const SymbolExchange& self) {
        return H::combine(std::move(h), self.exchange, self.symbol);
    }
    bool operator==(const SymbolExchange& that) const {
        return exchange==that.exchange && symbol==that.symbol;
    }
};

struct Markets {
    struct Data {
        roq::Exchange exchange;        
        roq::Symbol symbol;
        mmaker::BestPriceSource pub_price_source;
        mmaker::BestPriceSource best_price_source = mmaker::BestPriceSource::MARKET_BY_PRICE;
        umm::Volume lot_size = 1;
        MarketInfo to_market_info(umm::MarketIdent market) const {
            return MarketInfo {
                .market = market,
                .symbol = symbol,
                .exchange = exchange
            };
        }
    };

    template<class Fn>
    void get_markets(Fn&& fn) const {
        for(auto& [exchange, symbol_to_market] : symbol_to_market_) {
            for(auto& [symbol, market] : symbol_to_market) {
                fn(MarketInfo {
                    .market = market, 
                    .symbol = std::string_view{symbol},
                    .exchange =  std::string_view{exchange}
                });
            }
        }
    }
    
    Data& emplace(MarketIdent market, std::string_view symbol, std::string_view exchange) {
        auto& data = market_data_[market] = Data { 
            .exchange = exchange, 
            .symbol = symbol
        };
        symbol_to_market_[data.exchange][data.symbol] = market;
        return data;
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
            Data& data = emplace(market, symbol, exchange);
            data.pub_price_source = config.get_value_or(market_node, "pub_price_source", mmaker::BestPriceSource::UNDEFINED);
            data.best_price_source = config.get_value_or(market_node, "best_price_source", mmaker::BestPriceSource::MARKET_BY_PRICE);
            data.lot_size = config.get_value_or(market_node, "lot_size", umm::Volume{1.0});
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
        auto iter = market_data_.find(market);
        if(iter != std::end(market_data_)) {
            const auto& data = iter->second;
            fn(data);
            return true;
        }
        return false;
    }
private:
    absl::flat_hash_map<roq::Exchange, absl::flat_hash_map<roq::Symbol, umm::MarketIdent> > symbol_to_market_;
    absl::node_hash_map<umm::MarketIdent, Data> market_data_;
};

} // namespace mmaker
} // namesapce roq
