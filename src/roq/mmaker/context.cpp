#include "context.hpp"
#include <roq/client/config.hpp>
namespace roq { 
namespace mmaker {


/// Config::Handler
inline void Context::dispatch(roq::client::Config::Handler &handler) const {
    using namespace std::literals;
    //log::info<1>("Config::dispatch"sv);
    markets_.get_markets([&](const auto& data) {
        log::info<1>("symbol={}, exchange={}, market {}"sv, data.symbol, data.exchange, this->markets(data.market));
        handler(client::Symbol {
            .regex = data.symbol,
            .exchange = data.exchange
        });
    });
    handler(client::Settings {
        .order_cancel_policy = OrderCancelPolicy::BY_ACCOUNT
    });
    for(auto& [exchange, account]: accounts_) {
        log::info<1>("account {} exchange {}"sv, account, exchange);
        handler(client::Account {
            .regex = account
        });
    };
}

void Context::initialize(umm::IModel& model) {

    for(auto & [folio, portfolio]: portfolios) {
        for(auto &[market, position]: portfolio) {
            umm::Event<umm::PositionUpdate> event;
            event->market = market;  
            event->portfolio = folio;
            event.header.receive_time_utc = self()->now();
            log::info<1>("portfolio_snapshot folio {}  market {}  position {}", self()->prn(folio), self()->prn(market), position);
            model.dispatch(event);
        }
    }
}

} // mmaker
} // roq