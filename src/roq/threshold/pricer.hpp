#pragma once

#include "roq/core/handler.hpp"

namespace roq::threshold {



struct Pricer : core::Handler, core::Dispatcher {

    Pricer(core::Handler& next, core::Dispatcher &dispatcher, core::Manager &core);

    void operator()(const roq::Event<roq::ParametersUpdate> & e) override;
    void operator()(const roq::Event<core::Quotes> &e) override;
    void operator()(const roq::Event<core::ExposureUpdate> &) override;
    
    std::pair<lqs::Portfolio&,bool> emplace_portfolio(std::string_view portfolio_name);
    bool get_portfolio(core::PortfolioIdent portfolio, std::invocable<lqs::Portfolio&> auto fn);
    void get_portfolios(std::invocable<lqs::Portfolio&> auto fn);


    core::Handler &next;
    core::Dispatcher &dispatcher;
    core::Manager &core;
};

}