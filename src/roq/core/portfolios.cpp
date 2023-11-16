// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/portfolios.hpp"
#include "roq/logging.hpp"

namespace roq::core {

void Portfolios::operator()(const roq::Event<core::ExposureUpdate>& event) {
    for(auto&u : event.value.exposure) {
        log::info<2>("Portfolios::ExposureUpdate exposure {} portfolio {} market {}", u.exposure, u.portfolio, u.market);
    }
}

std::pair<core::Portfolio &, bool> Portfolios::emplace_portfolio(core::PortfolioKey key) {
    if (key.portfolio != 0) {
        auto iter = portfolios_.find(key.portfolio);
        if (iter != std::end(portfolios_)) {
          return {iter->second, false};
        }
    }
    auto iter = portfolio_index_.find(key.name);
    if (iter != std::end(portfolio_index_)) {
        core::PortfolioIdent id = iter->second;
        auto [iter_2, is_new] = portfolios_.try_emplace(id);
        return {iter_2->second, is_new};
    }
    auto [iter_2, is_new] = portfolios_.try_emplace(++last_portfolio_id);
    iter_2->second.portfolio = last_portfolio_id;
    portfolio_index_.try_emplace(key.name, last_portfolio_id);
    return {iter_2->second, is_new};
}
} // namespace roq::core