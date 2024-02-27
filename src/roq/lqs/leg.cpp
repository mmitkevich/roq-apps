#include "roq/lqs/leg.hpp"
#include "roq/core/exposure.hpp"
#include <roq/parameter.hpp>
#include <magic_enum.hpp>
#include "roq/lqs/pricer.hpp"
#include "roq/lqs/underlying.hpp"
#include "roq/core/string_utils.hpp"
#include "roq/core/contract_style.hpp"

namespace roq::lqs {

using namespace std::literals;

void Leg::operator()(const roq::Parameter & p, lqs::Strategy& s, std::string_view label) {
    //auto [prefix, label] = core::split_prefix(p.label, ':');

    if(label=="delta_by_volume"sv) {
        delta_by_volume = core::Double::parse(p.value);
    } else if(label=="buy_volume"sv) {
        buy_volume = core::Double::parse(p.value);
    } else if(label=="sell_volume"sv) {
        sell_volume = core::Double::parse(p.value);
    } else if(label=="passive_mode"sv) {
        passive_mode = magic_enum::enum_cast<core::PassiveMode>(p.value).value_or(core::PassiveMode::UNDEFINED);
    } else if(label=="account"sv) {
        account = std::string_view {p.value};
        assert(!account.empty());
    } else if(label=="slippage"sv) {
        slippage = core::Double::parse(p.value);
    } else if(label=="contract_style"sv) {
        contract_style = magic_enum::enum_cast<core::ContractStyle>(p.value).value_or(core::ContractStyle::UNDEFINED);
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

void Leg::compute(lqs::Strategy const & strategy, lqs::Underlying const* underlying_opt/*=nullptr*/) {
    core::BestQuotes& q = this->exec_quotes;
    core::BestQuotes& m = this->market_quotes;            
    core::BestQuotes& p = this->position;

    if(!check_market_quotes(m)) {
        exec_quotes.clear();
        return;
    }

    if(!strategy.enabled) {
        exec_quotes.clear();
        return;
    }

    // exposure = signed leg position
    core::Volume current_position = p.buy.volume - p.sell.volume;

    core::Volume exposure =  current_position - target_position;

    // trade towards zero exposure with buy_volume/sell_volume steps at most
    q.buy.volume = buy_volume.min((-exposure).max(0));
    q.sell.volume = sell_volume.min(exposure.max(0));

    if(underlying_opt) {
        lqs::Underlying const& u = *underlying_opt;
        // avoid delta leaving delta_range
        core::Double delta_plus = (u.delta_max-u.delta).max(0); // how much we can add to the delta
        core::Double delta_minus = (u.delta-u.delta_min).max(0); // how much we can subtract from the delta
        q.buy.volume = q.buy.volume.min( delta_plus / delta_by_volume );
        q.sell.volume = q.sell.volume.min( delta_minus /delta_by_volume );
    }

    switch(passive_mode) {
        case core::PassiveMode::CROSS:
            // take market agressively
            if(q.buy.volume>0)
                q.buy.price = m.sell.price;
            if(q.sell.volume>0)
                q.sell.price = m.buy.price;
            break;
        case core::PassiveMode::JOIN:
            // take market agressively
            if(q.buy.volume>0)
                q.buy.price = m.buy.price;
            if(q.sell.volume>0)
                q.sell.price = m.sell.price;
            break;
        default: assert(false);
    }
}

} // roq::lqs