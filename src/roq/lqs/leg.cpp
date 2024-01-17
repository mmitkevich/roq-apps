#include "roq/lqs/leg.hpp"

namespace roq::lqs {


void Leg::operator()(core::Exposure const& u) {
    exposure = u.exposure;
    compute();
}

void Leg::compute() {
    compute_delta();
    compute_fills();
}

void Leg::compute_delta() {
    delta_buy = std::max(exposure, core::Double {0}) * delta_by_volume;
    delta_sell = std::max(-exposure, core::Double {0}) * delta_by_volume;
}

void Leg::compute_fills() {
    core::Price G_B = position.buy.price; 
    core::Volume N_B = position.buy.volume;

    core::Price G_B_0 = position_0.buy.price;
    core::Volume N_B_0 = position_0.buy.volume;
    
    core::Price G_S = position.sell.price;
    core::Volume N_S = position.sell.volume;

    core::Price G_S_0 = position_0.sell.price;    
    core::Volume N_S_0 = position_0.sell.volume;

    core::Volume U_B = N_B - N_B_0;
    core::Volume U_S = N_S - N_S_0;

    core::Price F_B = (G_B * N_B - G_B_0 * N_B_0) / U_B;
    core::Price F_S = (G_S * N_S - G_S_0 * N_S_0) / U_S;

    fills = {
        .buy = {
            .price = F_B,
            .volume = U_B - std::min(U_B, U_S)
        },
        .sell = {
            .price = F_S,
            .volume = U_S - std::min(U_B, U_S)
        }
    };
}


} // roq::lqs