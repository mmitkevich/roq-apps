#include "roq/lqs/underlying.hpp"
#include "roq/lqs/pricer.hpp"
#include <algorithm>
#include "roq/core/string_utils.hpp"

namespace roq::lqs {

using namespace std::literals;

void Underlying::operator()(const roq::Parameter & p, lqs::Strategy& s) {
    auto [prefix, label] = core::split_prefix(p.label,':');
    if(label=="delta_min"sv) {
        delta_min = core::Double::parse(p.value);
    } else if(label=="delta_max"sv) {
        delta_max = core::Double::parse(p.value);
    }
}


void Underlying::compute(lqs::Strategy& s) {
    delta = 0;
    s.get_legs(*this, [&](lqs::Leg const& leg) {
        delta += (leg.position.buy.volume - leg.position.sell.volume) * leg.delta_by_volume;
    });
}

void Underlying::remove_leg(core::MarketIdent leg) {
    std::remove_if(legs.begin(), legs.end(), [&](auto l) { return l == leg; });
}

} // namespace roq::lqs