#include "roq/core/portfolios.hpp"
#include "roq/logging.hpp"

namespace roq::core {

void Portfolios::operator()(const roq::Event<core::ExposureUpdate>& event) {
    for(auto&u : event.value.exposure) {
        log::info<2>("Portfolios::ExposureUpdate exposure {} portfolio {} market {}", u.exposure, u.portfolio, u.market);
    }
}
    
}