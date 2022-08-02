#pragma once
#ifdef UMM_LOG_LOC
//#warning UMM_LOG_LOC redefined
#undef UMM_LOG_LOC
#endif
#define UMM_LOG_LOC(x, file, line) { \
    std::stringstream ss;\
    ss << x;\
    roq::log::info<1>("UMM {}",ss.str());\
}
#include <roq/logging.hpp>
#include "umm/config.hpp"
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

struct MarketsMap {
    struct Item {
        std::string_view symbol;
        std::string_view exchange;
        umm::MarketIdent market;
    };
    template<class Fn>
    void get_markets(Fn&& fn) const {
        for(auto& [exchange, symbol_to_market] : exchange_to_symbols_) {
            for(auto& [symbol, market] : symbol_to_market) {
                fn(Item{.symbol=symbol, .exchange=exchange, .market=market});
            }
        }
    }
    void emplace(const Item& item) {
        exchange_to_symbols_[item.exchange][item.symbol] = item.market;
    }
    void clear() { exchange_to_symbols_.clear(); }

    umm::MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) {
        auto iter_1 = exchange_to_symbols_.find(exchange);
        if(iter_1 == std::end(exchange_to_symbols_)) {
            return {};
        }
        auto& symbol_to_market = iter_1->second;
        auto iter_2 = symbol_to_market.find(symbol);
        if(iter_2==std::end(symbol_to_market)) {
            return {};
        }
        return iter_2->second;
    }

    absl::flat_hash_map<Exchange, absl::flat_hash_map<Symbol, umm::MarketIdent>> exchange_to_symbols_;
};


struct Context : umm::Context, client::Config {  
    umm::Context context;
    BestPriceSource best_price_source {BestPriceSource::MARKET_BY_PRICE};
    umm::Volume vwap_volume;

    Context();
    virtual ~Context() = default;

    using umm::Context::get_market_ident;
    inline umm::MarketIdent get_market_ident(roq::cache::Market const & market) {
        return markets_map_.get_market_ident(market.context.symbol, market.context.exchange);
    }

    template<class T>
    inline umm::MarketIdent get_market_ident(const Event<T> &event) {
        return markets_map_.get_market_ident(event.value.symbol, event.value.exchange);
    }


    template<class Config>
    void configure(const Config& config);

    /// client::Config
    void dispatch(Handler &) const override;

    bool operator()(const Event<MarketByPriceUpdate>& event, cache::Market& market);
    bool operator()(const Event<ReferenceData> &event, cache::Market& market);

private:
    umm::BestPrice get_best_price_from_market_by_price(const cache::Market& market);
    MarketsMap markets_map_;
    std::unordered_set<std::string> accounts_;
};

template<class C>
void Context::configure(const C& config) {
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
        config.template get_pairs<umm::Volume>(position_node, "market", "position", [&](auto market_str, auto position) {
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