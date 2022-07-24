#pragma once

#define UMM_LOG_LOC(x, file, line) { \
    std::stringstream ss;\
    ss << x;\
    roq::log::info<1>("UMM {}",ss.str());\
}
#include <roq/logging.hpp>
#include "umm/context.hpp"
#include "umm/type.hpp"
#include "umm/quoter.hpp"
#include <absl/container/flat_hash_map.h>
#include <roq/cache/market.hpp>
#include <roq/client/config.hpp>

namespace roq {
namespace mmaker {

enum class BestPriceSource {
    UNDEFINED,
    TOP_OF_BOOK,
    MARKET_BY_PRICE,
    VWAP
};

struct Context : umm::Context, client::Config {

    toml::table toml;
    umm::Quoter::Factory factory;
    BestPriceSource best_price_source {BestPriceSource::MARKET_BY_PRICE};
    umm::Volume vwap_volume;

    Context();
    virtual ~Context() = default;
    
    void configure(toml::table&& table);

    inline umm::MarketIdent get_market_ident(roq::cache::Market const & market) {
        return get_market_ident(market.context.symbol, market.context.exchange);
    }

    template<class T>
    inline umm::MarketIdent get_market_ident(const Event<T> &event) {
        return get_market_ident(event.value.symbol, event.value.exchange);
    }

    inline umm::MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) {
        auto iter_1 = exchange_symbol_to_umm_id_.find(exchange);
        if(iter_1 == std::end(exchange_symbol_to_umm_id_)) {
            return {};
        }
        auto& symbol_to_umm_id = iter_1->second;
        auto iter_2 = symbol_to_umm_id.find(symbol);
        if(iter_2==std::end(symbol_to_umm_id)) {
            return {};
        }
        return iter_2->second;
    }

    /// client::Config
    void dispatch(Handler &) const override;

    bool operator()(const Event<MarketByPriceUpdate>& event, cache::Market& market);
    bool operator()(const Event<ReferenceData> &event, cache::Market& market);
private:
    umm::BestPrice get_best_price_from_market_by_price(const cache::Market& market);
    absl::flat_hash_map<Exchange, absl::flat_hash_map<Symbol, umm::MarketIdent>> exchange_symbol_to_umm_id_;
};


} // mmaker
} // roq