// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/portfolio/manager.hpp"
#include "roq/core/exposure_update.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/types.hpp"
#include "roq/logging.hpp"
#include <roq/string_types.hpp>
#include <roq/core/config/toml_file.hpp>

namespace roq::core::portfolio {

void Manager::configure(const config::TomlFile& config, config::TomlNode root) {
    config.get_nodes(root,"portfolio", [&](config::TomlNode node) {
        auto [portfolio, is_new] = emplace_portfolio({
            .portfolio_name = config.get_string(node, "portfolio"),
        });
        log::info("configure portfolio.{} {}", portfolio.portfolio, portfolio.portfolio_name);
    });
    config.get_nodes(root, "account", [&](auto node) {
        roq::Exchange exchange = config.get_string_or(node, "exchange", {});
        roq::Account account = config.get_string_or(node, "account", {});
        std::string portfolio_name = config.get_string_or(node, "portfolio", {});
        core::PortfolioIdent portfolio = get_portfolio_ident(portfolio_name);
        portfolio_by_account_[exchange][account] = portfolio;
        log::info("configure account {}@{} -> portfolio.{}", account, exchange, portfolio);
    });
}

void Manager::operator()(core::ExposureUpdate const& u) {
    for(auto const& exposure : u.exposure) {
        log::info<2>("Portfolios::ExposureUpdate exposure {}", exposure);
    }
}


void Manager::operator()(const roq::Event<roq::PositionUpdate>& event) {
    auto& u = event.value;
    auto [market, is_new_market] = core.markets.emplace_market(event);
    
    core::Exposure exposure {
        .position_buy = u.long_quantity,
        .position_sell = u.short_quantity,
        .market = market.market,
        .exchange = market.exchange,        
        .symbol = market.symbol,
//        .portfolio = portfolio.portfolio,
//        .portfolio_name = portfolio.portfolio_name,
    };
    
    get_portfolio_by_account(u.account, u.exchange,[&](core::Portfolio & portfolio){
        exposure.portfolio = portfolio.portfolio;
        exposure.portfolio_name = portfolio.portfolio_name;
        portfolio.set_position(market.market, exposure);
    });

    log::info<2>("portfolios:: PositionUpdate exposure {}", exposure);
    
    if(handler) {
        core::ExposureUpdate update {
            .exposure = std::span {&exposure, 1}
        };
        (*handler)(update, *this);
    }
}

std::pair<core::Portfolio &, bool> Manager::emplace_portfolio(core::PortfolioKey key) {
    if (key.portfolio != 0) {
        auto iter = portfolios_.find(key.portfolio);
        if (iter != std::end(portfolios_)) {
          return {iter->second, false};
        }
    }
    auto iter = portfolio_by_name_.find(key.portfolio_name);
    if (iter != std::end(portfolio_by_name_)) {
        core::PortfolioIdent id = iter->second;
        auto [iter_2, is_new] = portfolios_.try_emplace(id);
        return {iter_2->second, is_new};
    }
    auto [iter_2, is_new] = portfolios_.try_emplace(++last_portfolio_id);
    iter_2->second.portfolio = last_portfolio_id;
    portfolio_by_name_.try_emplace(key.portfolio_name, last_portfolio_id);
    auto& portfolio = iter_2->second;
    portfolio.portfolio_name = key.portfolio_name;
    return {iter_2->second, is_new};
}

core::PortfolioIdent Manager::get_portfolio_ident(std::string_view name) {
    auto iter = portfolio_by_name_.find(name);
    if (iter == std::end(portfolio_by_name_)) {
        return {};
    }
    return iter->second;
}

core::Volume Manager::get_net_exposure(core::PortfolioIdent portfolio, core::MarketIdent market, core::Volume exposure) {
    portfolios_[portfolio].get_position(market, [&] (core::Exposure const& p) {
        exposure =  p.position_buy -  p.position_sell;
    });
    return exposure;
}

} // namespace roq::core