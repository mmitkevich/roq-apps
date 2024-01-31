// (c) copyright 2023 Mikhail Mitkevich

#pragma once

#include <roq/parameters_update.hpp>
#include <string_view>
#include <span>
#include <roq/service.hpp>
#include <roq/client.hpp>
#include "roq/core/config/manager.hpp"

namespace roq::core {

struct Application final : roq::Service {
    using roq::Service::Service;
    
    core::config::Manager config;
    std::string strategy_name;
    
    void operator()(roq::Event<roq::ParametersUpdate> const & event);

    int main(args::Parser const &) override;

    static int main(int argc, char**argv);
};

}  // namespace roq::core
