/* Copyright (c) 2021 Mikhail Mitkevich */
#include "roq/client.hpp"
#include "umm/core/model_api.hpp"
#include "umm/model/provider.hpp"
#include "./context.hpp"
#include "./application.hpp"
#include "./flags/flags.hpp"
#include "./strategy.hpp"

using namespace std::chrono_literals;
using namespace std::literals;

namespace roq {
namespace mmaker {

Application::Application(int argc, char**argv)
: Service(argc, argv, Info {})
{}

int Application::main(std::span<std::string_view> args) {
  auto config_file = Flags::config_file();
  log::info("config_file '{}'", config_file);
  toml::table toml = toml::parse_file(config_file);
  umm::TomlConfig config { toml };
  mmaker::Context context;
  config.get_market_ident = [&](std::string_view market) -> umm::MarketIdent { return context.get_market_ident(market); };
  config.get_portfolio_ident = [&](std::string_view folio) -> umm::PortfolioIdent { return context.get_portfolio_ident(folio); };

  auto factory = umm::Provider::create(context, config, config["app"]);
  std::unique_ptr<umm::IQuoter> quoter = factory();

  /// pass parameters to the quoter
  config(*quoter);
  
  /// pass positions to the quoter
  context.configure( *quoter, config );

  std::unique_ptr<mmaker::OrderManager> order_manager = std::make_unique<mmaker::OrderManager>(context);
  client::Trader(context, args).dispatch<Strategy>(context, *quoter, *order_manager);
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