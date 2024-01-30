#include "roq/core/strategy.hpp"

#define USE_LQS
//#define USE_DAG
//#define USE_SPREADER

#ifdef USE_LQS
#include "roq/lqs/pricer.hpp"
#endif

#ifdef USE_DAG
#include "roq/dag/factory.hpp"
#include "roq/dag/pricer.hpp"
#endif

#ifdef USE_SPREADER
#include "roq/spreader/pricer.hpp"
#endif

namespace roq::core {
using namespace std::literals;

std::unique_ptr<core::Handler> factory(core::Strategy& s) {
  #ifdef USE_LQS
  if(s.strategy_name == "lqs"sv) {
    return std::make_unique<lqs::Pricer>(s.oms, s.core);
  } 
  #endif
  #ifdef USE_DAG
  if(strategy_name == "dag"sv) {
    dag::Factory::initialize_all();
    return std::make_unique<dag::Pricer>(oms, core);
  }
  #endif
  throw roq::RuntimeError("pricer '{}' was not found"sv, s.strategy_name);
  return {};
}

} // roq::mmaker
