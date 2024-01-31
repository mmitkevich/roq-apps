#include "roq/spreader/spread.hpp"
#include "roq/core/utils/string_utils.hpp"
#include "roq/spreader/pricer.hpp"

namespace roq::spreader {

using namespace std::literals;

void Spread::compute_delta() {
    delta = 0;
    for (auto const &leg : legs) {
        delta += leg.position.buy.volume  * leg.volume_multiplier;
        delta -= leg.position.sell.volume * leg.volume_multiplier;
    }
}

void Spread::compute() {
    compute_delta();
}

void Spread::operator()(const roq::Event<roq::ParametersUpdate> & event, spreader::Pricer& ctx) {
    for(const roq::Parameter& p: event.value.parameters) {
        // FIXME:
        core::Market market {
            .symbol = p.symbol,
            .exchange = p.exchange
        };
        auto [label, index] = core::utils::split_suffix_uint32(p.value,':');
        if(label=="leg"sv) {
            market.symbol = p.value;
        }
        if(label=="exchange"sv) {
            market.symbol = p.value;
        }

    }
}

} // roq::spreader