#pragma once
#include "umm/context.hpp"
#include "umm/type.hpp"
#include "umm/quoter.hpp"
#include <roq/cache/market.hpp>
#include <roq/client/config.hpp>

namespace roq {
namespace mmaker {

inline constexpr umm::MarketIdent get_umm_id(roq::cache::Market const & market) {
    return umm::get_market_ident(market.context.symbol, market.context.exchange);
}

struct Context : client::Config {

    toml::table config;
    umm::Context umm {config};
    umm::Quoter::Factory factory;

    operator umm::Context&() { return umm;}
    
    /// client::Config
    void dispatch(Handler &) const override;

    void operator()(const Event<MarketByPriceUpdate>& event, cache::Market& market);
    void operator()(const Event<ReferenceData> &event, cache::Market& market);
};


} // mmaker
} // roq