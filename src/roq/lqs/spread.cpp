#include "roq/lqs/spread.hpp"

namespace roq::lqs {

void Spread::compute_delta() {
    delta = 0;
    for (auto const &leg : legs) {
        delta += leg.position.buy.volume  * leg.delta_by_volume;
        delta -= leg.position.sell.volume * leg.delta_by_volume;
    }
}

void Spread::compute() {
    compute_delta();
}

} // roq::lqs