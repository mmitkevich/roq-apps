/* Copyright (c) 2021 Mikhail Mitkevich */
//#include "umm/prologue.hpp"
#include "roq/client.hpp"

#include "roq/flags/args.hpp"
#include "roq/logging/flags/settings.hpp"

//#include "roq/mmaker/publisher.hpp"
//#include "umm/core/model_api.hpp"
//#include "umm/model/provider.hpp"
//#include "./context.hpp"
#include "./application.hpp"
#include "./flags/flags.hpp"
#include "./strategy.hpp"
#include <cstdlib>
#include <memory>
#include <roq/client/config.hpp>
#include <roq/logging.hpp>
#include "roq/config/toml_file.hpp"

using namespace std::chrono_literals;
using namespace std::literals;

/*
namespace umm {
UMM_NOINLINE
LogLevel get_log_level_from_env() {
    const char* v = getenv("ROQ_v");
    if(v)
        return  (umm::LogLevel)atoi(v);
    return LogLevel::WARN;
}
}
*/

namespace roq {
namespace mmaker {

void Application::dispatch(roq::client::Config::Handler &handler) const {
    using namespace std::literals;
    //log::info<1>("Config::dispatch"sv);


    handler(client::Symbol{
        .regex = ".*",
        .exchange = ".*"
    });
    
    handler(client::Account { .regex = ".*" });

    handler(client::Settings {
        .order_cancel_policy = OrderCancelPolicy::BY_ACCOUNT
    });

    /*markets_.get_markets([&](const auto& data) {
        log::info<1>("symbol={}, exchange={}, market {}"sv, data.symbol, data.exchange, this->markets(data.market));
        handler(client::Symbol {
            .regex = data.symbol,
            .exchange = data.exchange
        });
    });*/


    /*for(auto& [exchange, account]: accounts_) {
        log::info<1>("account {} exchange {}"sv, account, exchange);
        handler(client::Account {
            .regex = account
        });
    };*/
}

void Application::operator()(roq::Event<roq::ParametersUpdate> const & event) {

}
    

int Application::main(args::Parser const &parser) {
  
  strategy_name = Flags::strategy();
  auto config_file = Flags::config_file();
  
  log::info<1>("using strategy={} config_file={}", strategy_name, config_file);

  config.load(config_file);

  client::flags::Settings settings {parser};
  
  client::Trader{settings,/*client::Config=*/ *this, /* argv */parser.params()}.template dispatch<Strategy>(/*context*/*this);
  return EXIT_SUCCESS;
}


}  // namespace mmaker
}  // namespace roq


namespace {
auto const INFO = roq::Service::Info{
    .description = ROQ_PACKAGE_NAME,
    .package_name = ROQ_PACKAGE_NAME,
    .build_version = ROQ_VERSION,
};
}  // namespace


int main(int argc, char **argv) {
  roq::flags::Args args{argc, argv, INFO.description, INFO.build_version};
  roq::logging::flags::Settings settings{args};
  return roq::mmaker::Application{args, settings, INFO}.run();
}
