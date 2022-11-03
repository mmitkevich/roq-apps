/* Copyright (c) 2021 Mikhail Mitkevich */
#include "roq/client.hpp"
#include "roq/mmaker/publisher.hpp"
#include "umm/core/model_api.hpp"
#include "umm/model/provider.hpp"
#include "./context.hpp"
#include "./application.hpp"
#include "./flags/flags.hpp"
#include "./strategy.hpp"
#include <cstdlib>
#include <roq/logging.hpp>

using namespace std::chrono_literals;
using namespace std::literals;

namespace umm {
inline LogLevel get_log_level_from_env() {
    const char* v = getenv("ROQ_v");
    if(v)
        return  (umm::LogLevel)atoi(v);
    return LogLevel::WARN;
}
}

namespace roq {
namespace mmaker {

Application::Application(int argc, char**argv)
: Service(argc, argv, Info {})
{}

int Application::main(std::span<std::string_view> args) {
  auto config_file = Flags::config_file();
  log::info("config_file '{}'", config_file);
  umm::TomlConfig config { config_file };
  mmaker::Context context;
  
  // FIXME: use ROQ_v
  umm::set_log_level(umm::get_log_level_from_env());

  config.get_market_ident = [&](std::string_view market) -> umm::MarketIdent { return context.get_market_ident(market); };
  config.get_portfolio_ident = [&](std::string_view folio) -> umm::PortfolioIdent { return context.get_portfolio_ident(folio); };

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
      auto quoter_factory = umm::Provider::create(context, config, model_str);
      if(!quoter_factory) {
        log::warn<0>("error: strategy {} not supported", strategy);
        return;
      }
      std::unique_ptr<umm::IQuoter> quoter = quoter_factory();

      /// pass parameters to the quoter
      config(*quoter);
      
      /// pass positions to the quoter
      context.configure( *quoter, config );

      std::unique_ptr<mmaker::OrderManager> order_manager = std::make_unique<mmaker::OrderManager>(context);
      std::unique_ptr<mmaker::Publisher> publisher = std::make_unique<mmaker::Publisher>(context);

      client::Trader(context, args).template dispatch<Strategy>(context, std::move(quoter), std::move(order_manager), std::move(publisher));
    }
  });
  if(!strategy_found) {
    log::warn<0>("strategy {} not found", strategy);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int Application::main(int argc, char **argv) {
  std::vector<std::string_view> args;
  args.reserve(argc);
  for (int i = 0; i < argc; ++i)
    args.emplace_back(argv[i]);
  assert(!args.empty());
 // if (args.size() == 1u)
    //throw RuntimeErrorException("Expected arguments"sv);
  //if (args.size() != 2u)
    //throw RuntimeErrorException("Expected exactly one argument"sv);
  return main(std::span(args).subspan(1));
}

}  // namespace mmaker
}  // namespace roq



int main(int argc, char **argv) {
  auto app = roq::mmaker::Application(argc, argv);
  return app.run();
}