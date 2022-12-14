#pragma once
#include <roq/client.hpp>
#include <roq/logging.hpp>

#include <roq/cache/gateway.hpp>
#include <absl/container/flat_hash_map.h>

namespace roq {
namespace mmaker {


struct Context;

struct Gateways {
    Gateways(const mmaker::Context& context)
    : context(context) {}

    template<class T>
    bool operator()(const Event<T>& event) {
        bool result = false;
        if constexpr(std::is_invocable_v<cache::Gateway, const Event<T>&>) {
            uint32_t source = event.message_info.source;
            auto [gateway, is_new] = get_gateway_or_create(source);
            if(is_new) {
                log::info<2>("Gateways::NewGateway source={}", source);
            }
            result = gateway(event);
        }
        return result;
    }

    bool is_ready(roq::Mask<SupportType> mask, uint32_t source) const {
        auto gateway_iter = gateway_by_source_.find(source);
        if (gateway_iter == std::end(gateway_by_source_)) {
            return false;
        }
        auto &gateway = gateway_iter->second;
        return gateway.ready(mask);
    }
    
    bool is_ready(roq::Mask<SupportType> mask, uint32_t source, std::string_view account) const {
        auto gateway_iter = gateway_by_source_.find(source);
        if (gateway_iter == std::end(gateway_by_source_)) {
            return false;
        }
        auto &gateway = gateway_iter->second;
        bool ready =  gateway.ready(mask, account);
        if(!ready) {
            auto iter = gateway.state_by_account.find(account);
            if(iter!=std::end(gateway.state_by_account)) {
                auto& state = iter->second;
                log::info<2>("Gateways: source {} account {} not ready downloading {} available {} unavailable {} expected {}", source, account, state.downloading, state.status.available, state.status.unavailable, mask);
            } else {
                log::info<2>("Gateways: source {} account {} not ready expected {}", source, account, mask);
            }
            
        }
        return ready;
    }

    bool is_downloading(uint32_t source) const {
        auto gateway_iter = gateway_by_source_.find(source);
        if (gateway_iter == std::end(gateway_by_source_)) {
            return false;
        }
        auto &gateway = gateway_iter->second;
        return gateway.state.downloading;
    }

    template<class Fn>
    bool get_gateway(uint32_t source, Fn&& fn) const {
        auto iter = gateway_by_source_.find(source);
        if(iter!=std::end(gateway_by_source_)) {
            fn(iter->second);
            return true;
        }
        return false;
    }

    std::pair<cache::Gateway&, bool> get_gateway_or_create(uint32_t source) {
        auto iter = gateway_by_source_.find(source);
        if(iter==std::end(gateway_by_source_)) {
            return std::pair<cache::Gateway&, bool>{gateway_by_source_[source], true};
        }
        return std::pair<cache::Gateway&, bool>{iter->second, false};
    }
    
private:
    const mmaker::Context& context;
    absl::flat_hash_map<uint32_t, cache::Gateway> gateway_by_source_;
    absl::flat_hash_map<roq::Exchange, uint32_t> source_by_exchange_;
};

}
}