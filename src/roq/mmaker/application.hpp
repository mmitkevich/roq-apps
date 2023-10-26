// (c) copyright 2023 Mikhail Mitkevich

#pragma once

#include <roq/parameters_update.hpp>
#include <string_view>
#include <span>
#include <roq/service.hpp>
#include <roq/client.hpp>
#include "roq/config/manager.hpp"

namespace roq::mmaker {

struct Application final : roq::Service {
    using roq::Service::Service;
    
    config::Manager config;
    std::string strategy_name;

    void get_markets(auto&& fn) {
        auto& toml = config.toml;
        toml.get_nodes("market", [&](auto node) {
            auto market_name = toml.get_string(node, "market");
            //auto market = context.get_market_ident(market_str);
            fn(market_name, node);
        });
    }

    void operator()(roq::Event<roq::ParametersUpdate> const & event);

    int main(args::Parser const &) override;
};

}  // namespace roq
