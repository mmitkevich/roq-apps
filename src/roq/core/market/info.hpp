#include "roq/core/types.hpp"
#include "roq/string_types.hpp"
#include "roq/core/market.hpp"

namespace roq::core::market {

struct Info {
    core::MarketIdent market;
    roq::Symbol symbol;
    roq::Exchange exchange;
    roq::Account account;

    operator core::Market() const {
        return core::Market {
            .market = market,
            .symbol = symbol,
            .exchange = exchange,
        };
    }

    // preferred
    uint32_t mdata_gateway_id = -1;
    roq::Source mdata_gateway_name;
    
    // preferred
    //uint32_t trade_gateway_id = -1;
    //roq::Source trade_gateway_name;

    core::BestQuotesSource pub_price_source;
    core::BestQuotesSource best_quotes_source = core::BestQuotesSource::MARKET_BY_PRICE;
    
    core::Volume lot_size = 1;
    
    core::Double tick_size;
    core::Double min_trade_vol;

    uint32_t depth_num_levels = 0;
};

}