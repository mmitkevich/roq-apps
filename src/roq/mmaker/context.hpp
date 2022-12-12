#pragma once
#include "umm/prologue.hpp"
#include "umm/core/context.hpp"
#include "umm/core/type.hpp"
#include <roq/cache/manager.hpp>
#include <roq/position_update.hpp>
#include <roq/string_types.hpp>
#include <sstream>
#include "roq/logging.hpp"
#include "roq/client.hpp"
#include "./markets.hpp"

namespace roq {
namespace mmaker {

struct Context : umm::Context, client::Config {
    using Base = umm::Context;

    Context() = default;
    
    template<class Config>
    void configure(const Config& config);

    using umm::Context::get_market_ident;


    umm::MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) {
        return markets_.get_market_ident(symbol, exchange);
    }

    umm::MarketIdent get_market_ident(roq::cache::Market const & market) {
        return this->get_market_ident(market.context.symbol, market.context.exchange);
    }

    template<class T>
    umm::MarketIdent get_market_ident(const Event<T> &event) {
        return this->get_market_ident(event.value.symbol, event.value.exchange);
    }

    template<class Fn>
    bool get_account(std::string_view exchange, Fn&& fn) const {
        auto it = accounts_.find(exchange);
        if(it==std::end(accounts_))
            return false;
        fn(it->second);
        return true;
    }

    template<class Fn>
    bool get_market(MarketIdent market, Fn&& fn) const {
        return markets_.get_market(market, [&](const auto &data) {
            fn(data);
        });
    }
    
    void initialize(umm::IModel& model);

    /// client::Config
    void dispatch(roq::client::Config::Handler &) const;
private:
    absl::flat_hash_map<roq::Exchange, roq::Account> accounts_;
    Markets markets_;
};

template<class ConfigT>
void Context::configure(const ConfigT& config) {
    markets_.configure(*this, config);
    
    portfolios.clear();
    config.get_nodes("position", [&]( auto position_node) {
        auto folio = umm::PortfolioIdent {config.get_string(position_node, "portfolio") };
        config.template get_pairs(umm::type_c<umm::Volume>{}, position_node, "market", "position", [&](auto market_str, auto position) {
            auto market = this->get_market_ident(market_str);
            UMM_INFO("market {} position {}", this->markets(market), position);
            portfolios[folio][market] = position;
//            umm::Event<umm::PositionUpdate> event;
//            quoter.dispatch(event);
        });
    });

    accounts_.clear();
    config.get_nodes("account", [&](auto account_node) {
        auto account_str = config.get_string(account_node, "account");
        auto exchange_str = config.get_string(account_node, "exchange");
        log::info<1>("account {} exchange {}", account_str, exchange_str);
        accounts_.emplace(exchange_str, account_str);
    });
}

} // mmaker
} // roq