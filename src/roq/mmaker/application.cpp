/* Copyright (c) 2021 Mikhail Mitkevich */

#include "roq/client.hpp"

#include "application.hpp"
#include "flags/flags.h"
#include "strategy.hpp"

using namespace std::chrono_literals;
using namespace std::literals;

namespace roq {
namespace mmaker {

Application::Application(int argc, char**argv)
: Service(argc, argv, Info {})
{}

int Application::main(std::span<std::string_view> args) {
  context.config = toml::parse_file(Flags::config_file());
  client::Trader(context, args).dispatch<Strategy>(context);
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

