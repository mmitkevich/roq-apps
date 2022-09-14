#include "context.hpp"
namespace roq { 
namespace mmaker {


/// Config::Handler
inline void Context::dispatch(roq::client::Config::Handler &handler) const {
    using namespace std::literals;
    //log::info<1>("Config::dispatch"sv);
    markets_map_.get_markets([&](const auto& item) {
        log::info<1>("symbol={}, exchange={}, market {}"sv, item.symbol, item.exchange, this->markets(item.market));
        handler(client::Symbol {
            .regex = item.symbol,
            .exchange = item.exchange
        });
    });

    for(auto& account_str: accounts_) {
        log::info<1>("account={}"sv, account_str);
        handler(client::Account {
            .regex = account_str
        });
    };
}


} // mmaker
} // roq