/* Copyright (c) 2021 Mikhail Mitkevich */

#include "roq/client.hpp"

#include "roq/umm/application.hpp"
#include "roq/umm/flags/flags.h"
#include "roq/umm/strategy.hpp"

using namespace std::chrono_literals;
using namespace std::literals;

namespace roq {
namespace umm {

Application::Application(int argc, char**argv)
: Service(argc, argv, Info {}) {}


void Application::ClientConfig::dispatch(client::Config::Handler &handler) const {

}

int Application::main(std::span<std::string_view> args) {
  toml_config_.parse_file(Flags::config_file());
  std::vector<std::string_view> connections;
  ClientConfig client_config_ {toml_config_};
  client::Trader(client_config_, std::span(connections)).dispatch<Strategy>(*this);
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

}  // namespace shared
}  // namespace roq
