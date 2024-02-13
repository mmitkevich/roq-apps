#include "roq/lqs/underlying.hpp"
#include "roq/lqs/pricer.hpp"
#include <algorithm>

namespace roq::lqs {

using namespace std::literals;

void Underlying::operator()(const roq::Parameter & p, lqs::Portfolio& portfolio) {
    if(p.label=="delta_min"sv) {
        delta_min = core::Double::parse(p.value);
    } else if(p.label=="delta_max"sv) {
        delta_max = core::Double::parse(p.value);
    }
}


void Underlying::compute(lqs::Portfolio& portfolio) {
    delta = 0;
    portfolio.get_legs(*this, [&](lqs::Leg const& leg) {
        delta += (leg.position.buy.volume - leg.position.sell.volume) * leg.delta_by_volume;
    });
}

void Underlying::remove_leg(core::MarketIdent leg) {
    std::remove_if(legs.begin(), legs.end(), [&](auto l) { return l == leg; });
}

} // namespace roq::lqs