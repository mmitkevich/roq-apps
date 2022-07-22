/* Copyright (c) 2021 Mikhail Mitkevich */

#pragma once

#include <string_view>
#include <span>
#include <roq/service.hpp>
#include <roq/umm/config.hpp>

#include "umm/config/toml++.hpp"

namespace roq {
namespace umm {


struct Application final :  Service  {

    struct ClientConfig : client::Config {
        toml::Config& config_;
        ClientConfig(toml::Config& config): config_(config) {}
        void dispatch(Handler &) const override;
    };

    Application(int argc, char**argv);

    int main(std::span<std::string_view> args);

    int main(int argc, char **argv) override;


private:
    toml::Config toml_config_;
};

}  // namespace mmaker
}  // namespace roq
