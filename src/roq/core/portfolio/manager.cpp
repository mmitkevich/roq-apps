// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/portfolio/manager.hpp"
#include "roq/core/exposure_update.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/types.hpp"
#include "roq/logging.hpp"
#include <roq/string_types.hpp>
#include <roq/core/config/toml_file.hpp>

namespace roq::core::portfolio {

using namespace std::literals;

void Manager::configure(const config::TomlFile& config, config::TomlNode root) {
    this->position_source = config.get_value_or(root["oms"], "position_source", core::PositionSource::ORDERS);
    
    //config.get_nodes(root,"portfolio", [&](config::TomlNode node) {
    //    auto [portfolio, is_new] = emplace_portfolio({
    //        .portfolio_name = config.get_string(node, "portfolio"),
    //    });
    //    log::info("configure portfolio.{} {}", portfolio.portfolio, portfolio.portfolio_name);
    //});

    config.get_nodes(root, "account", [&](auto node) {
        roq::Exchange exchange = config.get_string_or(node, "exchange", {});
        roq::Account account = config.get_string_or(node, "account", {});
        std::string portfolio_name = config.get_string_or(node, "portfolio", {});
        auto [portfolio, is_new] = emplace_portfolio({
            .portfolio_name = portfolio_name
        });
        portfolio_by_account_[exchange][account] = portfolio.portfolio;
        log::info("configure account {}@{} -> portfolio.{} {} is_new {}", account, exchange, portfolio.portfolio, portfolio.portfolio_name, is_new);
    });
}

void Manager::operator()(roq::Event<roq::ParametersUpdate> const& event) {
   for(auto& p: event.value.parameters) {
       if(p.label == "portfolio"sv) {
           auto [portfolio, is_new] = emplace_portfolio({
               .portfolio_name = p.value
           });
           assert(p.strategy_id);
           portfolio_by_strategy_[p.strategy_id] = portfolio.portfolio;
           log::info("configure strategy {} -> portfolio.{} {} is_new {}", p.strategy_id, portfolio.portfolio, portfolio.portfolio_name, is_new);
       }
   }
}
    

// from OMS
void Manager::operator()(core::Trade const& u) {
    if(position_source!=core::PositionSource::ORDERS)
        return;
    get_portfolio_by_account(u.account, u.exchange, [&](core::Portfolio & portfolio) {
        auto [v, is_new] = portfolio(u);

        core::Exposure exposure {
            .position_buy = v.position_buy,
            .position_sell = v.position_sell,
            .market = u.market,
            .symbol = u.symbol,        
            .exchange = u.exchange,        
    //        .portfolio = portfolio.portfolio,
    //        .portfolio_name = portfolio.portfolio_name,
        };
        core::ExposureUpdate update {
            .exposure = std::span {&exposure, 1},
            .portfolio = portfolio.portfolio,
            .portfolio_name = portfolio.portfolio_name,
        };
        
        log::info<2>("portfolios trade exposure {}", exposure);
        
        if(handler) {
            (*handler)(update);
        }
    });
}

/// from the gateway
void Manager::operator()(const roq::Event<roq::PositionUpdate>& event) {
    auto& u = event.value;
    auto [market, is_new_market] = core.markets.emplace_market(event);
    
    auto gateway_id = event.message_info.source;
    bool is_downloading = core.gateways.is_downloading(gateway_id, u.account);   

    switch(position_source) {
        case core::PositionSource::ORDERS:      
            // only position snapshot should be used
            if(!is_downloading) 
                return; 
        break;
        case core::PositionSource::PORTFOLIO:   // before download_end take snapshot from PORTFOLIO

        break;
    }

    core::Exposure exposure {
        .position_buy = u.long_quantity,
        .position_sell = u.short_quantity,
        .market = market.market,
        .symbol = market.symbol,        
        .exchange = market.exchange,        
//        .portfolio = portfolio.portfolio,
//        .portfolio_name = portfolio.portfolio_name,
    };

    core::ExposureUpdate update {
        .exposure = std::span {&exposure, 1},
    };
    
    get_portfolio_by_account(u.account, u.exchange, [&](core::Portfolio & portfolio) {
        //exposure.portfolio = portfolio.portfolio;
        //exposure.portfolio_name = portfolio.portfolio_name;
        ExposureKey key = {
            .market = market.market,
            .account = u.account
        };
        portfolio.set_position(key, exposure);
        update.portfolio_name = portfolio.portfolio_name;
        update.portfolio = portfolio.portfolio;
    });

    log::info<2>("portfolios position_update exposure {}", exposure);
    
    if(handler) {
        (*handler)(update);
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
/*
core::Volume Manager::get_net_exposure(core::PortfolioIdent portfolio, core::MarketIdent market, core::Volume exposure) {
    portfolios_[portfolio].get_position(market, [&] (core::Exposure const& p) {
        exposure =  p.position_buy -  p.position_sell;
    });
    return exposure;
}*/

} // namespace roq::core