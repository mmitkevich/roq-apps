#pragma once

#include "./basic_handler.hpp"
#include "./markets.hpp"
#include "umm/core/type.hpp"
#include "umm/printable.hpp"
//#include "absl/container/flat_hash_map.h"
#include <roq/cache/gateway.hpp>
#include <roq/cache/manager.hpp>
#include <roq/gateway_status.hpp>
#include <roq/market_by_price_update.hpp>
#include <roq/mask.hpp>
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

    /// NOTE: market is associated with source by receiveing ReferenceData here!
    void operator()(const Event<ReferenceData> &event) {
        auto market = self()->get_market_ident(event.value.symbol, event.value.exchange);
        const auto& source = source_by_market_[market] = event.message_info.source;
        log::info<2>("ReferenceData exchange {} symbol {} market {} source {}", event.value.exchange, event.value.symbol, self()->prn(market), source);
        Base::operator()(event);
    }

    template<class T>
    bool dispatch_to_gateway(const Event<T>& event) {
        auto gateway_iter = gateway_by_source_.find(event.message_info.source);
        if(gateway_iter==std::end(gateway_by_source_)) {
            log::info<2>("new_gateway {}", event.message_info.source);
        }
        auto& gateway = gateway_by_source_[event.message_info.source];
        return gateway(event);
    }

    bool is_ready(MarketIdent market) const {
        bool ready = true;
        auto source_iter = source_by_market_.find(market);
        if(source_iter==std::end(source_by_market_)) {
            ready = false;
            log::info<2>("is_ready {} market {} source not found", ready, self()->prn(market));
            return ready;
        }
        auto source = source_iter->second;
        auto gateway_iter = gateway_by_source_.find(source);
        if(gateway_iter == std::end(gateway_by_source_)) {
            ready = false;
            log::info<2>("is_ready {} market {} source {} gateway not found", ready, self()->prn(market), source);
            return ready;
        }
        auto& gateway = gateway_iter->second;
        auto expected = self()->get_expected_support_type(market);
        if(!gateway.ready(expected)) {
            ready = false;
            log::info<2>("is_ready {} market {} source {} expected {} available {} unavailable {}", ready, self()->prn(market), source, expected, gateway.state.status.available, gateway.state.status.unavailable);
            return ready;
        }
        return ready;
    }

    roq::Mask<SupportType> get_expected_support_type(MarketIdent market) const {
        return roq::Mask<SupportType> {roq::SupportType::REFERENCE_DATA};
    }

protected:
    //roq::Mask<SupportType> expected {SupportType::REFERENCE_DATA, SupportType::MARKET_BY_PRICE};
    absl::flat_hash_map<umm::MarketIdent, uint32_t> source_by_market_;
    absl::flat_hash_map<uint32_t, cache::Gateway> gateway_by_source_;
    cache::Manager cache_;
};

}  // namespace mmaker
}  // namespace roq