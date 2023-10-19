#include "publisher.hpp"
#include "umm/core/type/best_price.hpp"
#include "flags/flags.hpp"

namespace roq {
namespace mmaker {

Publisher::Publisher( mmaker::Context& context)
: context(context) { }


void Publisher::dispatch(const umm::Event<umm::BestPriceUpdate> & event) {
    if(Flags::publisher_id()==0) {
        return;
    }
    context.get_market(event->market, [&](const auto& market) {
        if(market.pub_price_source == core::BestPriceSource::TOP_OF_BOOK) {
            umm::BestPrice& best_price = context.best_price[event->market];
            publish(market.to_market_info(event->market), best_price);
        }
    });
}

bool Publisher::publish(const MarketInfo& data, const umm::BestPrice& best_price) {
    if(Flags::publisher_id()==0) {
        return false;
    }
    auto source_id = Flags::publisher_id()-1;
    log::info<2>("publish {} {} to gateway {} ", context.prn(data.market), context.prn(best_price), source_id);
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
    return true;
}

} // namespace mmaker
} // namespace roq

