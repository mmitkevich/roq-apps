#pragma once
#include "umm/prologue.hpp"
#include "umm/core/type.hpp"
#include "umm/core/event.hpp"
#include <roq/client/dispatcher.hpp>
#include "roq/mmaker/best_price_source.hpp"
#include "roq/mmaker/context.hpp"

namespace roq {
namespace mmaker {

struct Strategy;

struct Publisher {
    Publisher(mmaker::Context& context);

    void set_dispatcher(client::Dispatcher& dispatcher) { dispatcher_ = &dispatcher; }

    void dispatch(const umm::Event<umm::QuotesUpdate> & event);

    void publish(const MarketInfo& market, const umm::BestPrice& best_price);


    roq::client::Dispatcher* dispatcher_ {};
    mmaker::Context& context;
    std::vector<roq::Measurement> items_;
    uint8_t source_id = 0;
};

} // namespace mmaker
} // namespace roq