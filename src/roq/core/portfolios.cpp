// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/portfolios.hpp"
#include "roq/core/manager.hpp"
#include "roq/logging.hpp"
#include <roq/string_types.hpp>

namespace roq::core {

void Portfolios::operator()(const roq::Event<core::ExposureUpdate>& event) {
    for(auto const& exposure : event.value.exposure) {
        log::info<2>("Portfolios::ExposureUpdate exposure {}", exposure);
    }
}

void Portfolios::operator()(const roq::Event<roq::PositionUpdate>& event) {
    auto& u = event.value;
    auto [market, is_new_market] = core.markets.emplace_market(event);
    
    std::array<char, roq::detail::MAX_LENGTH_ACCOUNT+roq::detail::MAX_LENGTH_EXCHANGE+16> portfolio_name;
    auto result = fmt::format_to_n(portfolio_name.data(), portfolio_name.size(), "{}:{}", u.exchange, u.account);
    auto [portfolio,is_new] = emplace_portfolio({
        .portfolio_name = std::string_view{ portfolio_name.data(), result.size }
    });
    core::Exposure exposure {
        .position_buy = u.long_quantity,
        .position_sell = u.short_quantity,
        .market = market.market,
        .exchange = market.exchange,        
        .symbol = market.symbol,
        .portfolio = portfolio.portfolio,
        .portfolio_name = portfolio.portfolio_name,
    };

    log::info<2>("Portfolios:: PositionUpdate exposure {} portfolio {} {} market {} symbol {} exchange {}", 
        exposure, 
        portfolio.portfolio, 
        portfolio.portfolio_name, 
        market.market, 
        u.symbol, 
        u.exchange);
    portfolio.set_position(market.market, exposure);
}

std::pair<core::Portfolio &, bool> Portfolios::emplace_portfolio(core::PortfolioKey key) {
    if (key.portfolio != 0) {
        auto iter = portfolios_.find(key.portfolio);
        if (iter != std::end(portfolios_)) {
          return {iter->second, false};
        }
    }
    auto iter = portfolio_index_.find(key.portfolio_name);
    if (iter != std::end(portfolio_index_)) {
        core::PortfolioIdent id = iter->second;
        auto [iter_2, is_new] = portfolios_.try_emplace(id);
        return {iter_2->second, is_new};
    }
    auto [iter_2, is_new] = portfolios_.try_emplace(++last_portfolio_id);
    iter_2->second.portfolio = last_portfolio_id;
    portfolio_index_.try_emplace(key.portfolio_name, last_portfolio_id);
    auto& portfolio = iter_2->second;
    portfolio.portfolio_name = key.portfolio_name;
    return {iter_2->second, is_new};
}

core::PortfolioIdent Portfolios::get_portfolio_ident(std::string_view name) {
    auto iter = portfolio_index_.find(name);
    if (iter == std::end(portfolio_index_)) {
        return {};
    }
    return iter->second;
}

core::Volume Portfolios::get_net_exposure(core::PortfolioIdent portfolio, core::MarketIdent market, core::Volume exposure) {
    portfolios_[portfolio].get_position(market, [&] (core::Exposure const& p) {
        exposure =  p.position_buy -  p.position_sell;
    });
    return exposure;
}

} // namespace roq::core