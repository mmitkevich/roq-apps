#include "context.hpp"
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

    for(auto& [exchange, account]: accounts_) {
        log::info<1>("account {} exchange {}"sv, account, exchange);
        handler(client::Account {
            .regex = account
        });
    };
}


} // mmaker
} // roq