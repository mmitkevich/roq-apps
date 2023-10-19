#pragma once

#include "roq/core/gateways.hpp"
#include "roq/core/markets.hpp"
#include "roq/core/portfolios.hpp"
//#include "roq/core/account_cache.hpp"
#include "roq/core/types.hpp"
#include "roq/mask.hpp"
#include "roq/support_type.hpp"
#include <roq/cache/manager.hpp>
#include <roq/core/basic_handler.hpp>

namespace roq::core {

struct Manager : core::BasicDispatch<Manager> {

    operator cache::Manager&() { return cache; }
    operator Gateways&() { return gateways; }
    operator Markets&() { return markets; }
    operator Portfolios&() { return portfolios; }
    //operator Accounts&() { return accounts; }

    template<class T>
    core::MarketIdent get_market_ident (const Event<T>& event) { return markets.get_market_ident(event); }

    core::MarketIdent get_market_ident (std::string_view symbol ,std::string_view exchange) { return markets.get_market_ident(symbol, exchange); }

    std::string_view get_exchange(core::MarketIdent market) const { 
        std::string_view result = {};
        markets.get_market(market, [&](auto& item) { result = item.exchange; });
        return result;
    }

    std::string_view get_symbol(core::MarketIdent market) const { 
        std::string_view result = {};
        markets.get_market(market, [&](auto& item) { result = item.symbol; });
        return result;
    }

    bool is_ready(core::MarketIdent market) const;
public:
    static roq::Mask<roq::SupportType> expected_md_support;
public:
    cache::Manager cache {client::MarketByPriceFactory::create};
    core::Gateways gateways;
    core::Markets markets;
    core::Portfolios portfolios; // all the positions sitting here
    //core::Accounts accounts;
    
};

} // roq::cache