#include "roq/lqs/underlying.hpp"
#include "roq/lqs/pricer.hpp"
#include <algorithm>
#include "roq/core/string_utils.hpp"
#include "roq/core/contract_style.hpp"

namespace roq::lqs {

using namespace std::literals;

void Underlying::operator()(const roq::Parameter & p, lqs::Strategy& s, std::string_view label) {
    //auto [prefix, label] = core::split_prefix(p.label,':');
    //auto [prefx_2, label_2] = core::split_prefix(label, ':');
    if(label=="delta_min"sv) {
        delta_min = core::Double::parse(p.value);
    } else if(label=="delta_max"sv) {
        delta_max = core::Double::parse(p.value);
    }
}

core::Double Underlying::get_delta_by_volume(const lqs::Leg& leg, lqs::Strategy& s) {
    core::Price spot_price = (market_quotes.buy.price+market_quotes.sell.price)/2;
    switch(leg.contract_style) {
        case core::ContractStyle::LINEAR:return leg.delta_by_volume * spot_price;
        case core::ContractStyle::INVERSE: return leg.delta_by_volume;
        default: return leg.delta_by_volume;// FIXME: assert(false)
    }
    // market_uqotes object is filled with underluying market price
}

void Underlying::compute(lqs::Strategy& s) {
    delta = 0;
    s.get_legs(*this, [&](lqs::Leg const& leg) {
        delta += (leg.position.buy.volume - leg.position.sell.volume) * get_delta_by_volume(leg, s);
    });
}

void Underlying::remove_leg(core::Market const& leg) {
    auto iter = std::remove_if(legs.begin(), legs.end(), [&](auto l) { return l.first == leg.market && l.second == leg.account; });
}

void  Underlying::add_leg(core::Market const& key) {
    legs.push_back({key.market, key.account});
}

} // namespace roq::lqs