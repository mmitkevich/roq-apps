#pragma once
#include "roq/lqs/leg.hpp"
#include "roq/lqs/underlying.hpp"
#include "roq/core/hash.hpp"
#include "roq/reference_data.hpp"
#include "roq/download_begin.hpp"
#include "roq/download_end.hpp"
#include "roq/string_types.hpp"

namespace roq::lqs {

struct Pricer;

struct Strategy {

  Strategy(lqs::Pricer& pricer) 
  : pricer(pricer) {}

  Strategy(Strategy const&) = default;
  Strategy(Strategy&&) = default;

  bool operator()(roq::Parameter const & p, std::string_view label);
  bool operator()(core::Quotes const& u);
  bool operator()(core::Exposure const& e);
  bool operator()(roq::ReferenceData const& u);
  bool operator()(roq::DownloadBegin const& u);
  bool operator()(roq::DownloadEnd const& u);
  std::pair<lqs::Underlying&, bool> emplace_underlying(core::Market const& key);
  std::pair<lqs::Leg&, bool> emplace_leg(core::Market const& key);
  
  bool get_underlying(lqs::Leg& leg, std::invocable<lqs::Underlying &> auto fn);
  bool get_underlying(core::Market const& key, std::invocable<lqs::Underlying &> auto fn);
  bool get_leg(core::Market const& market, std::invocable<lqs::Leg &> auto fn);
  void get_legs(lqs::Underlying& underlying, std::invocable<Leg&> auto fn);
  void get_legs(std::invocable<Leg&> auto fn);

  void get_accounts(std::invocable<std::string_view> auto fn);
  bool compute(lqs::Leg& this_leg);
public:
  lqs::Pricer& pricer;
  bool enabled = false;
  //bool auto_legs = true;  // legs would be created upon receiving exposure
  core::StrategyIdent strategy;
  core::PortfolioIdent portfolio; // NOTE: for now portfolio == strategy always
  core::Hash<core::MarketIdent, lqs::Underlying> underlyings;
  core::Hash<roq::Account, core::Hash<core::MarketIdent, lqs::Leg> > leg_by_market_by_account;
};

inline void Strategy::get_accounts(std::invocable<std::string_view> auto fn) {
  for(auto& [account, _] : leg_by_market_by_account) {
    fn(account);
  }
}

inline bool Strategy::get_leg(core::Market const& key, std::invocable<lqs::Leg &> auto fn) {
    auto& leg_by_market = leg_by_market_by_account[key.account];
    auto iter = leg_by_market.find(key.market);
    if(iter == std::end(leg_by_market))
        return false;
    fn(iter->second);
    return true;
}

inline bool Strategy::get_underlying(lqs::Leg& leg, std::invocable<lqs::Underlying &> auto fn) {
    if(!leg.underlying)
      return false;
    core::Market key {
      .market = leg.underlying,
    };
    return get_underlying(key, fn);
}

inline bool Strategy::get_underlying(core::Market const& key, std::invocable<lqs::Underlying &> auto fn) {
  core::MarketIdent underlying = key.market;
  assert(underlying);
  auto iter = underlyings.find(underlying);
    if(iter==std::end(underlyings))
      return false;
    fn(iter->second);
    return true;
}

inline void Strategy::get_legs(lqs::Underlying& underlying, std::invocable<Leg&> auto fn) {
  for(auto item: underlying.legs) {
    core::Market key {
      .market = item.first,
      .account = item.second,
    };
    get_leg(key, fn);
  }
}

inline void Strategy::get_legs(std::invocable<Leg&> auto fn) {
  for(auto& [account, leg_by_market] : leg_by_market_by_account)
    for(auto& [market, leg]: leg_by_market) {
      fn(leg);
    }
}

} // roq::lqs