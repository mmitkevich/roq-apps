#include "roq/core/manager.hpp"
#include "roq/core/best_quotes.hpp"
#include "roq/core/quotes.hpp"
#include "roq/lqs/pricer.hpp"
#include "roq/logging.hpp"

namespace roq::lqs {

Pricer::Pricer(core::Dispatcher &dispatcher, core::Manager &core)
: dispatcher(dispatcher)
, core(core)
{}


bool Pricer::compute(core::MarketIdent market, std::invocable<core::Quotes const&> auto fn) {
    bool result = true;
    result &= core.best_quotes.get_quotes(market, [&] (core::BestQuotes const& market_quotes) {
        // take market BBO prices
        core::BestQuotes exec_quotes = market_quotes;
        // spread them little bit
        exec_quotes.buy.price /= 1.1;
        exec_quotes.sell.price *= 1.1;
        // send out TargetQuotes
        fn(core::to_quotes(exec_quotes, market));
    });
    return result;
}

void Pricer::operator()(const roq::Event<core::Quotes> &e) {
    compute(e.value.market, [&](core::Quotes const& exec_quotes) {
        log::debug<2>("lqs::Quotes market_quotes {} exec_quotes {}", e.value, exec_quotes);
        roq::MessageInfo info{};
        roq::Event event{info, exec_quotes};
        dispatcher(event); // send to OMS
    });
}

} // namespace roq::lqs
