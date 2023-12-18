#pragma once

#include "roq/core/dispatcher.hpp"
#include "roq/core/manager.hpp"
#include "roq/core/handler.hpp"

//#include "roq/core/basic_pricer.hpp"

namespace roq::lqs {

struct Pricer : core::Handler {
  Pricer(core::Dispatcher &dispatcher, core::Manager &core);

  bool compute(core::MarketIdent market, std::invocable<core::Quotes const&> auto fn);

  void operator()(const roq::Event<roq::core::Quotes> &e) override;

  core::Dispatcher &dispatcher;
  core::Manager &core;
};


}

