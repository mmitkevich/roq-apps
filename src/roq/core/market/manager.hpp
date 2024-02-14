// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include "roq/core/type_list.hpp"
#include "string_view"
#include "roq/core/best_quotes_source.hpp"
#include <absl/container/node_hash_map.h>
#include <roq/gateway_status.hpp>
#include <roq/reference_data.hpp>
#include <roq/string_types.hpp>
#include <roq/logging.hpp>
#include "roq/core/basic_handler.hpp"
#include "roq/core/types.hpp"
#include "roq/core/market.hpp"
#include "roq/core/market/info.hpp"
#include "roq/core/config/toml_file.hpp"
#include "roq/core/hash.hpp"
#include "roq/core/oms/market.hpp"

namespace roq::core::market {

struct Manager : core::BasicDispatch<market::Manager> {
    using Base = core::BasicDispatch<market::Manager>;

    void get_markets(std::invocable<core::market::Info const&> auto && fn) const {
        for(auto& [market_id, market] : market_by_id_) {
            fn(market);
        }
    }

    std::pair<core::market::Info&, bool> emplace_market(core::Market const&);

    // helper
    template<class T>
    std::pair<core::market::Info&, bool> emplace_market(roq::Event<T> const& e) {
        auto & u = e.value;
        auto [market, is_new] = emplace_market({.symbol=u.symbol, .exchange=u.exchange});
        //if(is_new) {
        //    market.mdata_gateway_id = market.trade_gateway_id = e.message_info.source;
        //    market.mdata_gateway_name = market.trade_gateway_name = e.message_info.source_name;
        //}
        return {market, is_new};
    }

    using Base::operator();

    void operator()(const Event<GatewaySettings> &event);
    void operator()(const Event<ReferenceData> &event);

    void configure(const config::TomlFile& config, config::TomlNode root);
    
    void clear();

    core::MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) const;

    template<class T>
    core::MarketIdent get_market_ident(roq::Event<T> const& event) const {
        return get_market_ident(event.value.symbol, event.value.exchange);
    }
    
    bool get_market(core::Market const& market, std::invocable<core::market::Info const&> auto&& fn) const {
        if(market.market)
            return get_market(market.market, fn);
        auto market_id = get_market_ident(market.symbol, market.exchange);
        return get_market(market_id, fn);
    }

    bool get_market(core::MarketIdent market_id, std::invocable<core::market::Info const&> auto&& fn) const {
        auto iter = market_by_id_.find(market_id);
        if(iter != std::end(market_by_id_)) {
            const auto& item = iter->second;
            fn(item);
            return true;
        }
        return false;
    }
public:
    core::MarketIdent last_market_id = 0;
private:
    absl::flat_hash_map<std::string_view/*roq::Exchange*/, absl::flat_hash_map<std::string_view/*roq::Symbol*/, core::MarketIdent> > market_by_symbol_by_exchange_;
    absl::node_hash_map<core::MarketIdent, core::market::Info> market_by_id_;
};

} // roq::core::market

