#include "context.hpp"
namespace roq { 
namespace mmaker {


/// Config::Handler
inline void Context::dispatch(roq::client::Config::Handler &handler) const {
    using namespace std::literals;
    //log::info<1>("Config::dispatch"sv);
    markets_.get_markets([&](const auto& item) {
        log::info<1>("symbol={}, exchange={}, market {}"sv, item.symbol, item.exchange, this->markets(item.market));
        handler(client::Symbol {
            .regex = item.symbol,
            .exchange = item.exchange
        });
    });

    for(auto& [exchange, account]: accounts_) {
        log::info<1>("account {} exchange {}"sv, account, exchange);
        handler(client::Account {
            .regex = account
        });
    };
}


} // mmaker
} // roq