// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include "roq/core/basic_handler.hpp"
#include "roq/mmaker/mbp_depth_array.hpp"
#include "umm/prologue.hpp"
#include "umm/core/context.hpp"
#include "umm/core/type.hpp"
#include <roq/cache/manager.hpp>
#include <roq/gateway_settings.hpp>
#include <roq/gateway_status.hpp>
#include <roq/position_update.hpp>
#include <roq/string_types.hpp>
#include <roq/support_type.hpp>
#include <sstream>
#include "roq/logging.hpp"
#include "roq/client.hpp"
#include "./markets.hpp"
#include "./gateways.hpp"

namespace roq {
namespace mmaker {

struct Context : BasicDispatch<Context, umm::Context, client::Config> {
    using Base = BasicDispatch<Context, umm::Context, client::Config>;

    Context() = default;
    
    template<class Config>
    void configure(const Config& config);

    using umm::Context::get_market_ident;


    core::MarketIdent get_market_ident(std::string_view symbol, std::string_view exchange) {
        return markets_.get_market_ident(symbol, exchange);
    }

    core::MarketIdent get_market_ident(roq::cache::Market const & market) {
        return this->get_market_ident(market.context.symbol, market.context.exchange);
    }

    template<class T>
    core::MarketIdent get_market_ident(const Event<T> &event) {
        return this->get_market_ident(event.value.symbol, event.value.exchange);
    }

    template<class Fn>
    bool get_account(std::string_view exchange, Fn&& fn) const {
        auto it = accounts_.find(exchange);
        if(it==std::end(accounts_))
            return false;
        fn(it->second);
        return true;
    }

    template<class Fn>
    bool get_market(core::MarketIdent market, Fn&& fn) const {
        return markets_.get_market(market, [&](const auto &data) {
            fn(data);
        });
    }

    template<class T>
    void operator()(const roq::Event<T>& event) {
        this->dispatch(event);
    }

    template<class T>
    void dispatch(const roq::Event<T>& event) {
        set_now(event.message_info.receive_time_utc);
        this->gateways(event);
        this->markets_(event);
    }
    
    using Base::operator();

    void operator()(const Event<ReferenceData> &event);
    void operator()(const Event<MarketByPriceUpdate> &event);
    void operator()(const Event<TopOfBook> &event);

    /*template<class Fn>
    bool get_mdata_gateway(core::MarketIdent market, Fn&& fn) const {
        bool found = true;
        if(!get_market(market, [&](auto& info) {
            if(info.mdata_gateway_id==-1) {
                found = false;
            } else {
                if(!gateways.get_gateway(info.mdata_gateway_id, std::forward<Fn>(fn)))
                    found = false;
            }
        }))
            found = false;
        return found;
    }*/

    /*
    template<class Fn>
    bool get_trade_gateway(core::MarketIdent market, Fn&& fn) const {
        bool found = true;
        if(!get_market(market, [&](auto& info) {
            if(info.trade_gateway_id==-1) {
                found = false;
            } else {
                if(!gateways.get_gateway(info.trade_gateway_id, std::forward<Fn>(fn)))
                    found = false;
            }
        }))
            found = false;
        return found;
    }
    */
    
    //void initialize(umm::IModel& model);

    /// client::Config
    void dispatch(roq::client::Config::Handler &) const;

    bool is_ready(core::MarketIdent market) const;

    core::BestPriceSource get_best_price_source(core::MarketIdent market) const;

public:
    Gateways gateways;
private:
    cache::Manager cache_ {client::MarketByPriceFactory::create};
    MBPDepthArray mbp_depth_;
    absl::flat_hash_map<roq::Exchange, roq::Account> accounts_;
    Markets markets_;
    absl::flat_hash_map<core::MarketIdent, uint32_t> mbp_num_levels_;
    static roq::Mask<roq::SupportType> expected_md_support;

};

template<class ConfigT>
void Context::configure(const ConfigT& config) {
    markets_.configure(*this, config);
    
    portfolios.clear();
    config.get_nodes("position", [&]( auto position_node) {
        auto folio = umm::PortfolioIdent {config.get_string(position_node, "portfolio") };
        config.template get_pairs(umm::type_c<umm::Volume>{}, position_node, "market", "position", [&](auto market_str, auto position) {
            auto market = this->get_market_ident(market_str);
            UMM_INFO("market {} position {}", this->markets(market), position);
            portfolios[folio][market] = position;
//            umm::Event<umm::PositionUpdate> event;
//            quoter.dispatch(event);
        });
    });

    accounts_.clear();
    config.get_nodes("account", [&](auto account_node) {
        auto account_str = config.get_string(account_node, "account");
        auto exchange_str = config.get_string(account_node, "exchange");
        log::info<1>("account {} exchange {}", account_str, exchange_str);
        accounts_.emplace(exchange_str, account_str);
    });
}

} // mmaker
} // roq