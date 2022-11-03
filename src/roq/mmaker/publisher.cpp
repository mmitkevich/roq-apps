#include "publisher.hpp"
#include "umm/core/type/best_price.hpp"

namespace roq {
namespace mmaker {

Publisher::Publisher( mmaker::Context& context)
: context(context) { }

void Publisher::dispatch(const umm::Event<umm::QuotesUpdate> & event) {
    context.get_market(event->market, [&](const auto& market) {
        if(market.pub_price_source == BestPriceSource::TOP_OF_BOOK) {
            umm::BestPrice& best_price = context.best_price[event->market];
            publish(market.to_market_info(event->market), best_price);
        }
    });
}

void Publisher::publish(const MarketInfo& data, const umm::BestPrice& best_price) {
    log::info<2>("publish {} {}", context.prn(data.market), context.prn(best_price));
    using namespace std::literals;
    items_.resize(4);
    std::size_t i = 0;
    items_[i++] = roq::Measurement {
        .name = "bp"sv,
        .value = best_price.bid_price
    };
    items_[i++] = roq::Measurement {
        .name = "ap"sv,
        .value = best_price.ask_price
    };
    items_[i++] = roq::Measurement {
        .name = "bq"sv,
        .value = best_price.bid_volume
    };
    items_[i++] = roq::Measurement {
        .name = "aq"sv,
        .value = best_price.ask_volume
    };
    assert(dispatcher_);
    dispatcher_->send( roq::CustomMetrics {
        .label = "best_price"sv,
        .account = {},
        .exchange = data.exchange,
        .symbol = data.symbol,
        .measurements = items_,
        .update_type = UpdateType::INCREMENTAL,
//        .update_type = UpdateType::SNAPSHOT,
    }, source_id);
}

} // namespace mmaker
} // namespace roq

