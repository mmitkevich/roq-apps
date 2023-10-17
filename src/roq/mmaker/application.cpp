/* Copyright (c) 2021 Mikhail Mitkevich */
//#include "umm/prologue.hpp"
#include "roq/client.hpp"

#include "roq/flags/args.hpp"
#include "roq/logging/flags/settings.hpp"

#include "roq/mmaker/publisher.hpp"
#include "umm/core/model_api.hpp"
#include "umm/model/provider.hpp"
#include "./context.hpp"
#include "./application.hpp"
#include "./flags/flags.hpp"
#include "./strategy.hpp"
#include <cstdlib>
#include <memory>
#include <roq/client/config.hpp>
#include <roq/logging.hpp>

using namespace std::chrono_literals;
using namespace std::literals;

namespace umm {
UMM_NOINLINE
LogLevel get_log_level_from_env() {
    const char* v = getenv("ROQ_v");
    if(v)
        return  (umm::LogLevel)atoi(v);
    return LogLevel::WARN;
}
}

namespace roq {
namespace mmaker {

int Application::main(args::Parser const &parser) {
  auto args = parser.params();
  auto config_file = roq::mmaker::Flags::config_file();
  log::info("config_file '{}'", config_file);
  umm::TomlConfig config { config_file };
  mmaker::Context context;

  // FIXME: use ROQ_v
  umm::LogLevel log_level = umm::get_log_level_from_env();
  //log_level = umm::LogLevel::TRACE;
  umm::set_log_level(log_level);

  config.get_market_ident = [&](std::string_view market) -> core::MarketIdent { return context.get_market_ident(market); };
  config.get_portfolio_ident = [&](std::string_view folio) -> core::PortfolioIdent { return context.get_portfolio_ident(folio); };

  auto strategy = Flags::strategy();
  if(strategy.empty()) {
    log::warn<0>("--strategy=STRATEGY option expected");
    return EXIT_FAILURE;
  }
  log::debug<2>("finding strategy {}", strategy);

  bool strategy_found = false;
  config.get_nodes("strategy", [&](auto strategy_node) {
    auto strategy_str = config.get_string(strategy_node, "strategy");
    log::debug<2>("finding strategy {} current {}", strategy, strategy_str);
    if(!strategy_found &&  strategy_str == strategy) {
      strategy_found = true;
      auto model_str = config.get_string(strategy_node, "model");
      auto quoter_factory = umm::Provider::create(static_cast<umm::Context&>(context), config, model_str);
      if(!quoter_factory) {
        log::warn<0>("error: strategy {} not supported", strategy);
        return;
      }
      std::unique_ptr<umm::IQuoter> quoter = quoter_factory();

      context.configure( config );

      // parameters
      config(*quoter);
      context.initialize(*quoter);      

      std::unique_ptr<mmaker::OrderManager> order_manager = std::make_unique<mmaker::OrderManager>(context);

      order_manager->configure(config, strategy_node);
      
      std::unique_ptr<mmaker::Publisher> publisher = std::make_unique<mmaker::Publisher>(context);
      client::flags::Settings settings {parser};
      client::Trader{settings, context, args}.template dispatch<Strategy>(context, std::move(quoter), std::move(order_manager), std::move(publisher));
    }
  });
  if(!strategy_found) {
    log::warn<0>("strategy {} not found", strategy);
    return EXIT_FAILURE;
  }
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
