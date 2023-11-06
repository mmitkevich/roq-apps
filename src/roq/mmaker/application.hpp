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
    
    void operator()(roq::Event<roq::ParametersUpdate> const & event);

    int main(args::Parser const &) override;
};

}  // namespace roq
