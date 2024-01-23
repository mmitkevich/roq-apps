#pragma once

#include "roq/spreader/spread.hpp"
#include <roq/parameters_update.hpp>

namespace roq::spreader {

struct Bait {
  template<class T>
  void operator()(roq::Event<T> const& e) { }
  void operator()(roq::Event<roq::ParametersUpdate> const& e);
  bool operator()(spreader::Spread& spread, std::invocable<spreader::Leg const&> auto fn);
};

} // lqs