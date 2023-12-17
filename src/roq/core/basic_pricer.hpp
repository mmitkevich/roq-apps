#pragma once
#include "roq/core/types.hpp"
#include "roq/core/handler.hpp"
#include "roq/core/dispatcher.hpp"

namespace roq::core {

struct Manager;
struct Dispatcher;

template<class D, class Iface=core::Handler>
struct BasicPricer : Iface {

    virtual ~BasicPricer() = default;

    BasicPricer(core::Dispatcher &dispatcher, core::Manager& core)
    : dispatcher(&dispatcher) 
    , core(core)
    {}


    void operator()(const roq::Event<roq::core::Quotes> &e) override {
        roq::MessageInfo info {};
        auto const& market_quotes = e.value;
        core::Quote buy = market_quotes.get_best_buy();
        core::Quote sell = market_quotes.get_best_sell();
        static_cast<D*>(this)->compute_quotes(buy, sell);
        core::Quotes exec_quotes { 
            .buy = {&buy, 1},
            .sell = {&sell, 1}
        };
        roq::Event event {info, exec_quotes};
        dispatcher(event);  // send to OMS
    }

    void compute_quotes(core::Quote& buy, core::Quote& sell) { throw roq::RuntimeError("NOT IMPLEMENTD"); }

    core::Dispatcher& dispatcher;
    core::Manager& core; 
};

} // roq::core