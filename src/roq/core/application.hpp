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
    
    std::string strategy_name;
    std::unique_ptr<io::Context> context;
    std::unique_ptr<config::Manager> config;

    Application(args::Parser const &, logging::Settings const &, Info const &);

    void operator()(roq::Event<roq::ParametersUpdate> const & event);

    int main(args::Parser const &) override;

    static int main(int argc, char**argv);
};

}  // namespace roq::core
