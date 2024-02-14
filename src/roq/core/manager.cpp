// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/manager.hpp"
#include <roq/market_by_price_update.hpp>
#include <roq/support_type.hpp>
//#include "roq/cache/market_cache.hpp"

namespace roq::core {

roq::Mask<roq::SupportType> Manager::expected_md_support = {
    roq::SupportType::REFERENCE_DATA, 
    roq::SupportType::TOP_OF_BOOK, 
    roq::SupportType::MARKET_BY_PRICE, 
    roq::SupportType::MARKET_STATUS
};


bool Manager::is_ready(core::MarketIdent market_id) const {
    bool ready = true;
    ready &= markets.get_market(market_id, [&](const market::Info &market) {
        ready &= gateways.get_gateway(market.mdata_gateway_id, [&](const core::Gateway& gateway) {
            ready &= gateways.is_ready(expected_md_support, gateway.gateway_id);
            bool is_downloading = gateways.is_downloading(gateway.gateway_id);
            ready &= !is_downloading;
            if(!ready) {
                const cache::Gateway & gateway_1 = gateway;
                const auto& status = gateway_1.state.status;
                log::info<2>("is_ready=false market {} exchange {} symbol {} mdata_gateway.{} {} expected {} available {} unavailable {} downloading {}", 
                    market_id, get_exchange(market.market), get_symbol(market.market), market.mdata_gateway_id, gateway.gateway_name, 
                    expected_md_support, status.available, status.unavailable, is_downloading);

            }
        });
    });
    return ready;
}

}