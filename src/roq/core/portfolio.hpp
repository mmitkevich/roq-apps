#pragma once

#include <memory>
#include <roq/string_types.hpp>
#include "roq/core/best_quotes.hpp"
#include "roq/core/exposure.hpp"
#include "roq/core/exposure_update.hpp"
#include "roq/core/types.hpp"
// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/hash.hpp"
#include "roq/core/trade.hpp"

namespace roq::core {

struct Portfolio {
    template<class...Args>
    Portfolio(Args&&...args) : positions_ {std::forward<Args>(args)...} {}

    std::pair<core::ExposureValue&,bool> operator()(core::Trade const& trade);

    std::pair<core::ExposureValue&,bool> emplace_position(core::ExposureKey const& key);
    
    void set_position(core::ExposureKey const& key, core::ExposureValue const& exposure);

    bool get_position(core::ExposureKey const& key, std::invocable<core::ExposureValue const&> auto fn) const;

    void get_positions(std::invocable<core::Exposure const&> auto fn) const;
public:
    core::PortfolioIdent portfolio;
    roq::Account portfolio_name;      // arbitrary name (could be account name)
private:
    core::Hash<roq::Account, core::Hash<core::MarketIdent, core::ExposureValue>> positions_;
};


inline bool Portfolio::get_position(core::ExposureKey const& key, std::invocable<core::ExposureValue const&> auto fn) const {
    auto iter_1 = positions_.find(key.account);
    if(iter_1 == std::end(positions_)) {
        return false;
    }
    auto& by_market = iter_1->second;
    auto iter_2 = by_market.find(key.market);
    if(iter_2 == std::end(by_market)) {
        return false;
    }
    
    auto& v = iter_2->second;

    core::Exposure exposure {
        .position_buy = v.position_buy,
        .position_sell = v.position_sell,
        .avg_price_buy = v.avg_price_buy,
        .avg_price_sell = v.avg_price_sell,
        .market = key.market,
        .account = key.account,
    };

    core::ExposureUpdate update {
        .exposure = std::span {&exposure, 1},
        .portfolio = portfolio,
        .portfolio_name = portfolio_name,
    };
    fn(update);
    return true;
}

inline void Portfolio::get_positions(std::invocable<core::Exposure const&> auto fn) const {
    for(auto& [account, by_market] : positions_) {
        for(auto& [market, v] : by_market) {
            core::Exposure exposure {
                .position_buy = v.position_buy,
                .position_sell = v.position_sell,
                .avg_price_buy = v.avg_price_buy,
                .avg_price_sell = v.avg_price_sell,
                .market = market,
                .account = account,
            };
            
            fn(exposure);
        }
    }
}


} // roq::core
