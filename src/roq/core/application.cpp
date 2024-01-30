// (c) copyright 2023 Mikhail Mitkevich
//#include "umm/prologue.hpp"
#include "roq/client.hpp"

#include "roq/flags/args.hpp"
#include "roq/logging/flags/settings.hpp"

//#include "roq/mmaker/publisher.hpp"
//#include "umm/core/model_api.hpp"
//#include "umm/model/provider.hpp"
//#include "./context.hpp"
#include "roq/core/application.hpp"
#include "roq/core/flags/flags.hpp"
#include "roq/core/strategy.hpp"
#include <cstdlib>
#include <memory>
#include <roq/client/config.hpp>
#include <roq/logging.hpp>
#include "roq/core/config/toml_file.hpp"


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
namespace core {

std::unique_ptr<core::Handler> factory(core::Strategy& s);

int Application::main(args::Parser const &parser) {
  
  strategy_name = Flags::strategy();
  auto config_file = Flags::config_file();
  
  log::info<1>("using strategy={} config_file={}", strategy_name, config_file);

  config.load(config_file);

  client::flags::Settings settings {parser};

  client::Trader{settings, config, /* argv */parser.params()}.template dispatch<core::Strategy>(config, factory, strategy_name);
  return EXIT_SUCCESS;
}


}  // namespace core
}  // namespace roq




namespace {
auto const INFO = roq::Service::Info{
    .description = ROQ_PACKAGE_NAME,
    .package_name = ROQ_PACKAGE_NAME,
    .build_version = ROQ_VERSION,
};
}  // namespace


int roq::core::Application::main(int argc, char **argv) {
  roq::flags::Args args{argc, argv, INFO.description, INFO.build_version};
  roq::logging::flags::Settings settings{args};
  return roq::core::Application{args, settings, INFO}.run();
}