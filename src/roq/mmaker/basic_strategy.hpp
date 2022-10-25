#pragma once

#include "./basic_handler.hpp"
#include "./markets.hpp"
#include "umm/printable.hpp"
//#include "absl/container/flat_hash_map.h"
#include <roq/cache/gateway.hpp>
#include <roq/cache/manager.hpp>
#include <roq/gateway_status.hpp>
#include <roq/market_by_price_update.hpp>
#include <roq/reference_data.hpp>
#include <roq/support_type.hpp>

namespace roq {
namespace mmaker {

template<class Self, class Handler = client::Handler>
struct BasicStrategy : BasicHandler<Self, Handler> {
    Self* self() { return static_cast<Self*>(this); }
    const Self* self() const { return static_cast<const Self*>(this); }

    using Base = BasicHandler<Self, Handler>;
    using Base::dispatch, Base::operator();

    struct State  {
        roq::Mas
    };

    BasicStrategy()
    : cache_(client::MarketByPriceFactory::create)
    {}

    virtual ~BasicStrategy() = default;

    template<class T>
    const auto& prn(const T& val) const {
        return val;
    }
    
    void operator()(const Event<DownloadBegin> &event) {
        dispatch_to_gateway(event);
        Base::operator()(event);
    }

    void operator()(const Event<DownloadEnd> &event) {
        dispatch_to_gateway(event);
        Base::operator()(event);
    }

    void operator()(const Event<Connected> &event) {
        dispatch_to_gateway(event);
        Base::operator()(event);
    }

    void operator()(const Event<Disconnected> &event) {
        dispatch_to_gateway(event);
        Base::operator()(event);
    }

    void operator()(const Event<GatewayStatus> &event) {
        dispatch_to_gateway(event);
        Base::operator()(event);
    }

    void operator()(const Event<GatewaySettings> &event) {
        dispatch_to_gateway(event);
        Base::operator()(event);
    }

    void operator()(const Event<ReferenceData> &event) {
        auto market = self()->get_market_ident(event.value.symbol, event.value.exchange);
        auto& state = markets_[market];
        state(event);
        log::info<2>("ReferenceData exchange {} symbol {} market {}", event.value.exchange, event.value.symbol, self()->prn(market));
        Base::operator()(event);
    }
    
    void operator()(const Event<MarketByPriceUpdate> &event) {
        auto market = self()->get_market_ident(event.value.symbol, event.value.exchange);
        auto& state = markets_[market];
        state(event);
        log::info<2>("MarketByPriceUpdate exchange {} symbol {} market {}", event.value.exchange, event.value.symbol, self()->prn(market));
        Base::operator()(event);
    }

    template<class T>
    bool dispatch_to_gateway(const Event<T>& event) {
        auto gateway_iter = gateways_.find(event.message_info.source);
        if(gateway_iter==std::end(gateways_)) {
            log::info<2>("new_gateway {}", event.message_info.source);
        }
        auto& gateway = gateways_[event.message_info.source];
        return gateway(event);
    }

    bool is_ready(MarketIdent market) const {
        bool ready = true;
        auto market_iter = markets_.find(market);
        if(market_iter == std::end(markets_)) {
            ready = false;
        } else {
            auto& market_state = market_iter->second;
            if(!market_state.ready(expected)) {
                ready = false;
                log::info<2>("is_ready {} market {} snapshots {} expected {}", ready, self()->prn(market), market_state.snapshots_received, expected);
            } else {
                auto gateway_iter = gateways_.find(market_state.source);
                if(gateway_iter == std::end(gateways_)) {
                    ready = false;
                    log::info<2>("is_ready {} market {} source {} not found", ready, self()->prn(market), market_state.source);
                } else {
                    auto& gateway = gateway_iter->second;
                    if(!gateway.ready(expected)) {
                        ready = false;
                        log::info<2>("is_ready {} market {} source {} expected {}", ready, self()->prn(market), market_state.source, expected);
                    } else {
                        ready = true;
                    }
                }
            }
        }
        return ready;
    }

protected:
    Mask<SupportType> expected {SupportType::REFERENCE_DATA, SupportType::MARKET_BY_PRICE};
    absl::flat_hash_map<MarketIdent, State> markets_;
    absl::flat_hash_map<uint32_t, cache::Gateway> gateways_;
    cache::Manager cache_;
};

}  // namespace mmaker
}  // namespace roq