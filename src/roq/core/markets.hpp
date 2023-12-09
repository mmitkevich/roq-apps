// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include "string_view"
//#include "umm/core/type.hpp"
#include "roq/core/best_price_source.hpp"
#include "absl/container/flat_hash_map.h"
#include <absl/container/node_hash_map.h>
#include <roq/gateway_status.hpp>
#include <roq/string_types.hpp>
#include <roq/logging.hpp>
#include "roq/core/basic_handler.hpp"
#include "roq/core/types.hpp"
#include "roq/core/market.hpp"
namespace roq {
namespace core {


struct Markets : core::BasicDispatch<Markets> {
    using Base = core::BasicDispatch<Markets>;

    void get_markets(std::invocable<core::MarketInfo const&> auto && fn) const {
        for(auto& [market_id, market] : market_by_id_) {
            fn(market);
        }
    }

    std::pair<core::MarketInfo&, bool> emplace_market(core::Market const&);

    // helper
    template<class T>
    std::pair<core::MarketInfo&, bool> emplace_market(roq::Event<T> const& e) {
        auto & u = e.value;
        auto [market, is_new] = emplace_market({.symbol=u.symbol, .exchange=u.exchange});
        if(is_new) {
            market.mdata_gateway_id = market.trade_gateway_id = e.message_info.source;
            market.mdata_gateway_name = market.trade_gateway_name = e.message_info.source_name;
        }
        return {market, is_new};
    }

    using Base::operator();

    void operator()(const Event<GatewayStatus> &event);

    template<class Config, class Node>
    void configure(const Config& config, Node root) {
        clear();
        config.get_nodes(root, "market",[&](auto node) {
            auto symbol = config.get_string(node, "symbol");
            auto exchange = config.get_string(node, "exchange");
            //auto market_str = config.get_string(node, "market");
            auto mdata_gateway_name = config.get_string_or(node, "mdata_gateway", "");
            auto trade_gateway_name = config.get_string_or(node, "trade_gateway", "");
            core::MarketIdent market = this->get_market_ident(symbol, exchange);
            log::info<1>("symbol {}, exchange {}, market {} mdata_gateway '{}' trade_gateway '{}'", symbol, exchange, market, mdata_gateway_name, trade_gateway_name);
            auto [item, is_new] = emplace_market({.market=market, .symbol=symbol, .exchange=exchange});
            // FIXME:
            //item.market = market_str;
            item.pub_price_source = config.get_value_or(node, "pub_price_source", core::BestPriceSource::UNDEFINED);
            item.best_price_source = config.get_value_or(node, "best_price_source", core::BestPriceSource::MARKET_BY_PRICE);
            item.lot_size = config.get_value_or(node, "lot_size", core::Volume{1.0});
            item.mdata_gateway_name = mdata_gateway_name;
            item.trade_gateway_name = trade_gateway_name;
            item.depth_num_levels = config.get_value_or(node, "depth_num_levels", core::Integer{0});
        });    
    }

    void clear();

    core::MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) const;

    template<class T>
    core::MarketIdent get_market_ident(roq::Event<T> const& event) const {
        return get_market_ident(event.value.symbol, event.value.exchange);
    }
    
    bool get_market(core::MarketIdent market_id, std::invocable<core::MarketInfo const&> auto&& fn) const {
        auto iter = market_by_id_.find(market_id);
        if(iter != std::end(market_by_id_)) {
            const auto& item = iter->second;
            fn(item);
            return true;
        }
        return false;
    }
/*
    template<class Fn, class V>
    auto get_market_prop(core::MarketIdent market_id, Fn&& fn, V default_value) const {
        auto iter = market_by_id_.find(market_id);
        if(iter != std::end(market_by_id_)) {
            const auto& item = iter->second;
            return fn(item);
        }
        return default_value;
    }
*/
public:
    core::MarketIdent last_market_id = 0;
private:
    absl::flat_hash_map<std::string_view/*roq::Exchange*/, absl::flat_hash_map<std::string_view/*roq::Symbol*/, core::MarketIdent> > market_by_symbol_by_exchange_;
    absl::node_hash_map<core::MarketIdent, core::MarketInfo> market_by_id_;
};

} // namespace mmaker
} // namesapce roq
