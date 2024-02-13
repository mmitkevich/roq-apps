#include "roq/core/portfolio.hpp"

namespace roq::core {

std::pair<core::ExposureValue&,bool> Portfolio::emplace_position(core::ExposureKey const& key) {
    auto [iter, is_new] = positions_[key.account].try_emplace(key.market);
    return {iter->second, is_new};
}

std::pair<core::ExposureValue&,bool> Portfolio::operator()(core::Trade const& trade) {
    assert(trade.market);
    auto [exposure,is_new] = emplace_position({.market = trade.market, .account = trade.account});
    switch(trade.side) {
        case roq::Side::BUY: 
            exposure.position_buy += trade.quantity; 
        break;
        case roq::Side::SELL:
            exposure.position_sell += trade.quantity;
        break;
        default: assert(false);
    }
    return {exposure, is_new};
}

void Portfolio::set_position(core::ExposureKey const& key, core::ExposureValue const& exposure) {
    positions_[key.account][key.market] = exposure;
}

}