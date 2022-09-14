#pragma once
#include "./markets.hpp"
#include "umm/core/context.hpp"
#include <roq/cache/manager.hpp>
#include "roq/logging.hpp"
#include "roq/client.hpp"

namespace roq {
namespace mmaker {

struct Context : umm::Context, client::Config {
    Context() = default;
    
    template<class Config>
    void configure(const Config& config);
    using umm::Context::get_market_ident;
    inline umm::MarketIdent get_market_ident(roq::cache::Market const & market) {
        return markets_map_.get_market_ident(market.context.symbol, market.context.exchange);
    }

    template<class T>
    inline umm::MarketIdent get_market_ident(const Event<T> &event) {
        return markets_map_.get_market_ident(event.value.symbol, event.value.exchange);
    }

    /// client::Config
    void dispatch(roq::client::Config::Handler &) const;

    std::unordered_set<std::string> accounts_;
    Markets markets_map_;
};

template<class ConfigT>
void Context::configure(const ConfigT& config) {
    markets_map_.clear();
    config.get_nodes("market",[&](auto market_node) {
        auto symbol = config.get_string(market_node, "symbol");
        auto exchange = config.get_string(market_node, "exchange");
        auto market_str = config.get_string(market_node, "market");
        umm::MarketIdent market = get_market_ident(market_str);
        log::info<1>("symbol {}, exchange {}, market {} {}", symbol, exchange, market.value, markets(market));
        markets_map_.emplace({.symbol=symbol, .exchange=exchange, .market=market});
    });

    portfolios.clear();
    config.get_nodes("position", [&]( auto position_node) {
        auto folio = umm::PortfolioIdent {config.get_string(position_node, "portfolio") };
        config.template get_pairs(umm::type_c<umm::Volume>{}, position_node, "market", "position", [&](auto market_str, auto position) {
            auto market = get_market_ident(market_str);
            log::info<1>("market {} position {}", markets(market), position);
            portfolios[folio][market] = position;
        });
    });
    
    accounts_.clear();
    config.get_nodes("account", [&](auto account_node) {
        auto account_str = config.get_string(account_node, "account");
        //log::info<1>("account={}", account_str);
        accounts_.emplace(account_str);
    });
}

} // mmaker
} // roq