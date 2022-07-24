#include "context.hpp"
#include "umm/empty_value.hpp"
#include "umm/type.hpp"
#include "umm/type/best_price.hpp"
#include <roq/cache/market_by_price.hpp>
#include <roq/client/config.hpp>
#include <roq/order_cancel_policy.hpp>
#include <roq/order_management.hpp>
#include <roq/logging.hpp>

namespace roq { 
namespace mmaker {

Context::Context() {
    config.set_root(toml);
}

void Context::configure(toml::table&& table) {
    log::info<1>("Context::configure(toml::table)");
    toml = std::move(table);
    config.for_each("market",[&](auto i, auto market_node) {
        auto symbol = config.get_string(market_node, "symbol");
        auto exchange = config.get_string(market_node, "exchange");
        auto market_str = config.get_string(market_node, "market");
        umm::MarketIdent umm_id = config.get_market(market_str);
        log::info<1>("symbol {}, exchange {}, umm {} {}", symbol, exchange, umm_id.value, markets(umm_id));
        exchange_symbol_to_umm_id_[exchange][symbol] = umm_id;
    });
    config.for_each("position", [&](auto i, auto position_node) {
        auto upf = umm::PortfolioIdent {config.get_string(position_node, "portfolio") };
        config.get_markets(position_node, "market", [&](auto i, auto umm) {
            this->portfolios[upf][umm] = config.get_value<umm::Volume>(config.get_param_node(position_node, "position", i));
        });
    });
}
/// Config::Handler
void Context::dispatch(Handler &handler) const {
    log::info<1>("Config::dispatch");
    config.for_each("market", [&](auto i, auto market_node) {
        using namespace std::literals;
        auto symbol = config.get_string(market_node, "symbol");
        auto exchange = config.get_string(market_node, "exchange");
        auto market_str = config.get_string(market_node, "market");
        umm::MarketIdent market = config.get_market(market_str);
        log::info<1>("[{}] symbol={}, exchange={}, umm {} {}"sv, i, symbol, exchange, market.value, market_str);
        handler(client::Symbol {
            .regex = symbol,
            .exchange = exchange
        });
    });

    config.for_each("account", [&](auto i, auto account_node) {
        using namespace std::literals;
        auto account_str = config.get_string(account_node, "account");
        log::info<1>("[{}] account={}"sv, i, account_str);
        handler(client::Account {
            .regex = account_str
        });
    });
}

umm::BestPrice Context::get_best_price_from_market_by_price(const cache::Market& market) {
    auto bids = market.market_by_price->bids();
    auto asks = market.market_by_price->asks();
    auto& mbp = *market.market_by_price;
    umm::BestPrice best_price;
    if(!bids.empty()) {
        best_price.bid_price = mbp.internal_to_price(bids[0].first);
        best_price.bid_volume = mbp.internal_to_price(bids[0].second);
    }
    if(!asks.empty()) {
        best_price.ask_price = mbp.internal_to_price(asks[0].first);
        best_price.ask_volume = mbp.internal_to_price(asks[0].second);
    }
    return best_price;
}

bool Context::operator()(const Event<MarketByPriceUpdate>& event, cache::Market& market) {
    auto umm_id = get_market_ident(market);
    if(best_price_source==BestPriceSource::MARKET_BY_PRICE) {
        this->best_price[umm_id] = get_best_price_from_market_by_price(market);

    } else if(best_price_source==BestPriceSource::VWAP) {
        // TODO
    } else {
        return false;
    }
    return true;
}

bool Context::operator()(const Event<ReferenceData> &event, roq::cache::Market& market) {
    auto & src = market.reference_data;
    auto umm_id = get_market_ident(market);
    this->tick_rules.min_trade_vol[umm_id] =  src.min_trade_vol;
    this->tick_rules.tick_size[umm_id] =  src.tick_size;
    return true;
}

} // mmaker
} // roq