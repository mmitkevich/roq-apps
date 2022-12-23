#pragma once
#include "roq/mmaker/best_price_source.hpp"
#include "string_view"
#include "umm/core/type.hpp"
#include "absl/container/flat_hash_map.h"
#include <absl/container/node_hash_map.h>
#include <roq/gateway_status.hpp>
#include <roq/string_types.hpp>
#include <roq/logging.hpp>
#include "roq/mmaker/basic_handler.hpp"

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

struct Markets : roq::BasicDispatch<Markets> {
    using Base = roq::BasicDispatch<Markets>;
    
    struct Item {
        roq::Exchange exchange;        
        roq::Symbol symbol;
        mmaker::BestPriceSource pub_price_source;
        mmaker::BestPriceSource best_price_source = mmaker::BestPriceSource::MARKET_BY_PRICE;
        umm::Volume lot_size = 1;
        roq::Source mdata_gateway;
        uint32_t mdata_gateway_id = -1;
        roq::Source trade_gateway;
        uint32_t trade_gateway_id = -1;
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
        for(auto& [exchange, market_by_symbol] : market_by_symbol_by_exchange_) {
            for(auto& [symbol, market] : market_by_symbol) {
                fn(MarketInfo {
                    .market = market, 
                    .symbol = std::string_view{symbol},
                    .exchange =  std::string_view{exchange}
                });
            }
        }
    }
    
    Markets::Item& emplace(MarketIdent market, std::string_view symbol, std::string_view exchange) {
        auto& market_config = item_by_market_[market] = Markets::Item { 
            .exchange = exchange, 
            .symbol = symbol
        };
        market_by_symbol_by_exchange_[market_config.exchange][market_config.symbol] = market;
        return market_config;
    }
    
    using Base::operator();

    void operator()(const Event<GatewayStatus> & event) {
        for(auto& [market, item] : item_by_market_) {
            if(event.message_info.source_name == item.mdata_gateway) {
                item.mdata_gateway_id = event.message_info.source;
                log::info<1>("Markets: resolved mdata_gateway_id {} mdata_gateway {}", item.mdata_gateway_id, item.mdata_gateway);
            }
            if(event.message_info.source_name == item.trade_gateway) {
                item.trade_gateway_id = event.message_info.source;
                log::info<1>("Markets: resolved trade_gateway_id {} trade_gateway {}", item.trade_gateway_id, item.trade_gateway);
            }            
        }
    }

    template<class Context, class Config>
    void configure(Context& context, const Config& config) {
        clear();
        config.get_nodes("market",[&](auto market_node) {
            auto symbol = config.get_string(market_node, "symbol");
            auto exchange = config.get_string(market_node, "exchange");
            auto market_str = config.get_string(market_node, "market");
            auto mdata_gateway_str = config.get_string_or(market_node, "mdata_gateway", "");
            auto trade_gateway_str = config.get_string_or(market_node, "trade_gateway", "");
            umm::MarketIdent market = context.get_market_ident(market_str);
            log::info<1>("symbol {}, exchange {}, market {} {} mdata_gateway '{}' trade_gateway '{}'", symbol, exchange, market.value, context.prn(market), mdata_gateway_str, trade_gateway_str);
            Item& item = emplace(market, symbol, exchange);
            item.pub_price_source = config.get_value_or(market_node, "pub_price_source", mmaker::BestPriceSource::UNDEFINED);
            item.best_price_source = config.get_value_or(market_node, "best_price_source", mmaker::BestPriceSource::MARKET_BY_PRICE);
            item.lot_size = config.get_value_or(market_node, "lot_size", umm::Volume{1.0});
            item.mdata_gateway = mdata_gateway_str;
            item.trade_gateway = trade_gateway_str;
        });    
    }
    
    void clear() { 
        market_by_symbol_by_exchange_.clear(); 
        item_by_market_.clear();
    }

    umm::MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) const {
        auto iter_1 = market_by_symbol_by_exchange_.find(exchange);
        if(iter_1 == std::end(market_by_symbol_by_exchange_)) {
            return {};
        }
        auto& market_by_symbol = iter_1->second;
        auto iter_2 = market_by_symbol.find(symbol);
        if(iter_2==std::end(market_by_symbol)) {
            return {};
        }
        return iter_2->second;
    }

    template<class Fn>
    bool get_market(umm::MarketIdent market, Fn&& fn) const {
        auto iter = item_by_market_.find(market);
        if(iter != std::end(item_by_market_)) {
            const auto& item = iter->second;
            fn(item);
            return true;
        }
        return false;
    }
private:
    absl::flat_hash_map<roq::Exchange, absl::flat_hash_map<roq::Symbol, umm::MarketIdent> > market_by_symbol_by_exchange_;
    absl::node_hash_map<umm::MarketIdent, Markets::Item> item_by_market_;
};

} // namespace mmaker
} // namesapce roq
