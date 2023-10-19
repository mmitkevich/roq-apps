/* Copyright (c) 2021 Mikhail Mitkevich */

#pragma once

#include <string_view>
#include <span>
#include <roq/service.hpp>
#include <roq/client.hpp>
#include "roq/config/toml_config.hpp"

namespace roq::mmaker {

struct Application final : roq::Service, client::Config  {
    using roq::Service::Service;
    
    config::TomlConfig config;
    std::string strategy;

    void get_markets(auto&& fn) {
        config.get_nodes("market", [&](auto node) {
            auto market_name = get_string(node, "market");
            //auto market = context.get_market_ident(market_str);
            fn(market_name, node);
        });
    }

    void dispatch(roq::client::Config::Handler &handler) const override;

    int main(args::Parser const &) override;
};

}  // namespace roq
