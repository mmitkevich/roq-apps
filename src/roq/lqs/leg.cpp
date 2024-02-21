#include "roq/lqs/leg.hpp"
#include "roq/core/exposure.hpp"
#include <roq/parameter.hpp>
#include <magic_enum.hpp>
#include "roq/lqs/pricer.hpp"
#include "roq/lqs/underlying.hpp"
#include "roq/core/string_utils.hpp"

namespace roq::lqs {

using namespace std::literals;

void Leg::operator()(const roq::Parameter & p, lqs::Strategy& s) {
    auto [prefix, label] = core::split_prefix(p.label, ':');    
    if(label=="delta_by_volume"sv) {
        delta_by_volume = core::Double::parse(p.value);
    } else if(label=="buy_volume"sv) {
        buy_volume = core::Double::parse(p.value);
    } else if(label=="sell_volume"sv) {
        sell_volume = core::Double::parse(p.value);
    } else if(label=="passive_mode"sv) {
        passive_mode = magic_enum::enum_cast<lqs::PassiveMode>(p.value).value_or(lqs::PassiveMode::UNDEFINED);
    } else if(label=="account"sv) {
        account = std::string_view {p.value};
    }
}

void Leg::operator()(const core::Exposure& e, lqs::Strategy& s) {
    this->position.buy.volume = e.position_buy;   
    this->position.sell.volume = e.position_sell;
    log::info("lqs market.{} {}@{} position {} buy {} sell {}", market.market, market.symbol, market.exchange, 
        position.buy.volume-position.sell.volume,
        position.buy.volume, position.sell.volume);
}

bool Leg::check_market_quotes(core::BestQuotes& m) {
    if(m.buy.empty() || m.sell.empty()) {
        return false;
    }
    core::Double bid_ask_spread = ( m.sell.price/m.buy.price - 1.0 ) * 1E4;
    if(!slippage.empty() && bid_ask_spread > slippage) {
        return false;
    }
    return true;
}

void Leg::compute(lqs::Underlying const& u, lqs::Strategy const & portfolio) {
    core::BestQuotes& q = this->exec_quotes;
    core::BestQuotes& m = this->market_quotes;            
    core::BestQuotes& p = this->position;

    if(!check_market_quotes(m)) {
        exec_quotes.clear();
        return;
    }

    // exposure = signed leg position
    core::Volume current_position = p.buy.volume - p.sell.volume;

    core::Volume exposure =  current_position - target_position;

    // trade towards zero exposure with buy_volume/sell_volume steps at most
    q.buy.volume = buy_volume.min((-exposure).max(0));
    q.sell.volume = sell_volume.min(exposure.max(0));

    // avoid delta leaving delta_range
    core::Double delta_plus = (u.delta_max-u.delta).max(0); // how much we can add to the delta
    core::Double delta_minus = (u.delta-u.delta_min).max(0); // how much we can subtract from the delta
    
    q.buy.volume = q.buy.volume.min( delta_plus / delta_by_volume );
    q.sell.volume = q.sell.volume.min( delta_minus /delta_by_volume );

    if(!portfolio.enabled) {
        q.buy.volume = q.sell.volume = 0;
    }
    
    // take market agressively
    if(q.buy.volume>0)
        q.buy.price = m.sell.price;
    if(q.sell.volume>0)
        q.sell.price = m.buy.price;
}

} // roq::lqs