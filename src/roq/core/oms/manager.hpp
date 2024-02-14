// (c) copyright 2023 Mikhail Mitkevich

#pragma once
#include "roq/core/oms/book.hpp"

#include <roq/client.hpp>
#include <roq/client/dispatcher.hpp>
#include <roq/client/handler.hpp>
#include <roq/create_order.hpp>
#include <roq/download_end.hpp>
#include <roq/error.hpp>
#include <roq/execution_instruction.hpp>
#include <roq/gateway_status.hpp>
#include <roq/rate_limit_trigger.hpp>
#include <roq/stream_status.hpp>
#include <roq/string_types.hpp>
#include <string>
//#include "roq/core/dispatcher.hpp"
#include "roq/core/handler.hpp"
#include "roq/core/manager.hpp"
//#include "roq/core/market/manager.hpp"
#include "roq/core/clock.hpp"

#include "roq/core/order.hpp"
#include "roq/core/types.hpp"
#include "roq/core/quote.hpp"
#include "roq/core/quotes.hpp"
#include "roq/core/basic_handler.hpp"
#include "roq/core/position_source.hpp"
#include "roq/oms/order.hpp"
#include "roq/core/exposure_update.hpp"

#include "roq/core/dispatcher.hpp"
#include "roq/core/oms/handler.hpp"
#include "roq/core/oms/market.hpp"
#include "roq/core/basic_handler.hpp"
#include "roq/core/config/toml_file.hpp"

//#include "roq/mmaker/profit_loss.hpp"


namespace roq::core::oms 
{

struct Manager final 
: core::BasicHandler<Manager, client::Handler>
, core::Dispatcher
{
    using Self = Manager;
    using Base = BasicHandler<Manager, client::Handler>;
    using Handler = oms::Handler;

    using Base::dispatch;
    
public:
    Manager(Manager const&) = delete;
    Manager(Manager &&) = delete;

    Manager(oms::Handler& handler, client::Dispatcher& dispatcher, core::Manager& core);

    std::chrono::nanoseconds now() const { return now_; }

//    bool is_ready(uint32_t source, std::string_view account, roq::Mask<roq::SupportType> mask) const;
//    bool is_downloading(uint32_t source) const;
    
    void configure(const config::TomlFile& config, config::TomlNode node);

//    double get_position(std::string_view account, std::string_view symbol, std::string_view exchange);

    std::pair<oms::Book&, bool> emplace_book(oms::Market const& market);
    bool get_book(oms::Market const& market, std::invocable<oms::Book&, market::Info const&> auto fn);
    void get_books(std::invocable<oms::Book &, market::Info const &> auto fn);

    template<class T, std::invocable<oms::Book&, market::Info const &> Fn>
    bool get_book(roq::Event<T> const& event, Fn&&fn);

    // roq::client::Handler
    void operator()(roq::Event<Timer> const& event) override;
    void operator()(roq::Event<OrderUpdate> const& event) override;
    void operator()(roq::Event<OrderAck> const& event) override;
    void operator()(roq::Event<GatewayStatus> const& event) override;
//    void operator()(roq::Event<Disconnected> const& event);
//    void operator()(roq::Event<Connected> const& event);
//    void operator()(roq::Event<StreamStatus> const& event);
    void operator()(Event<PositionUpdate> const & event) override;
    void operator()(Event<FundsUpdate> const & event) override;
    void operator()(Event<DownloadBegin> const& event) override;
    void operator()(Event<DownloadEnd> const& event) override;
    void operator()(Event<RateLimitTrigger> const& event) override;
    
    // roq::pricer::Handler
    void operator()(core::TargetQuotes const& target_quotes) override;    
public:
    //core::PortfolioIdent portfolio;

/// IMPLEMENTATION
private:
    bool erase_order(oms::Book& market, uint64_t order_id);
    void erase_all_orders(oms::Book& market);

    void order_create_reject(oms::Book& market, oms::Order& order, const roq::OrderAck& u);
    void order_modify_reject(oms::Book& market, oms::Order& order, const roq::OrderAck& u);
    void order_cancel_reject(oms::Book& market, oms::Order& order, const roq::OrderAck& u);
    void order_fwd(oms::Book& market, oms::Order& order, const roq::OrderAck& u);
    void order_accept(oms::Book& market, oms::Order& order, const roq::OrderAck& u);
    void order_confirm(oms::Book& market, oms::Order& order, const roq::OrderUpdate& u);
    void order_complete(oms::Book& market, oms::Order& order, const roq::OrderUpdate& u);
    void order_canceled(oms::Book& market, oms::Order& order, const roq::OrderUpdate& u);

    void order_fills(oms::Book& market, roq::Side side, double price, double fill_size);

    bool is_throttled(oms::Book& market, roq::RequestType req);
    bool can_create(oms::Book& market, market::Info const& info, const core::TargetOrder & target_order);
    bool can_cancel(oms::Book& market, market::Info const& info, oms::Order& order);
    bool can_modify(oms::Book& market, market::Info const& info, oms::Order& order);

    oms::Order& create_order(oms::Book& market, const core::TargetOrder& target);
    void modify_order(oms::Book& market, oms::Order& order, const core::TargetOrder& target);
    void cancel_order(oms::Book& market, oms::Order& order);
    void process(oms::Book& market, market::Info const& info);
    
    bool reconcile_positions(oms::Book&);

    bool resolve_trade_gateway(oms::Book& book);
private:
    std::chrono::nanoseconds reject_timeout_ = std::chrono::seconds {2};
    uint64_t max_order_id = 0;
    std::array<char, 32> routing_id;
    core::Hash<uint64_t, oms::Market> market_by_order_;
    core::Hash<core::StrategyIdent, core::Hash<roq::Account, core::Hash<core::MarketIdent, oms::Book> > > books_;
    //absl::flat_hash_map<roq::Exchange, roq::Account> account_by_exchange_;
    //int source_id = 0;
    std::chrono::nanoseconds now_{};
    std::chrono::nanoseconds last_process_{};
    
    client::Dispatcher & dispatcher;
    core::Manager& core;
    oms::Handler& handler_;
    //client::Dispatcher& dispatcher_;
};

template<class T, std::invocable<oms::Book&, market::Info const &> Fn>
bool core::oms::Manager::get_book(roq::Event<T> const& event,  Fn&&fn) {
    bool found = false;
    T const& u = event.value;
    //        .market = core.get_market_ident(u.symbol. u.exchange),
    core.markets.get_market({
        .symbol = u.symbol,
        .exchange = u.exchange
    }, [&](market::Info const & info) {
        found = this->get_book(oms::Market{}.merge(info), fn);
    });
    return found;
}

bool core::oms::Manager::get_book(oms::Market const& key, std::invocable<oms::Book&, market::Info const &> auto fn) {
    auto iter = books_.find(key.strategy);
    if(iter == std::end(books_)) {
        return false;
    }
    auto& by_account = iter->second;
    auto iter_1 = by_account.find(key.account);
    if(iter_1 == std::end(by_account)) {
        return false;
    }
    auto& by_market = iter_1->second;
    core::MarketIdent market = key.market;
    if(!market) {
        market = core.get_market_ident(key.symbol, key.exchange);
    }
    if(!market) {
        return false;
    }
    auto iter_2 = by_market.find(market);
    if(iter_2 == std::end(by_market)) {
        return false;
    }
    oms::Book& market_2 = iter_2->second;
    return core.markets.get_market(market_2.market, [&](market::Info const & info) {
        fn(market_2, info);
    });
}

void oms::Manager::get_books(std::invocable<oms::Book &, market::Info const &> auto fn) {
    for(auto& [strategy, by_account] : books_) {
        for(auto& [account, by_market] : by_account) {
            for(auto& [market_id, market] : by_market) {
                core.markets.get_market(market_id, [&](market::Info const & info) {
                    fn(market, info);
                });
            }
        }
    }
}




} // roq::oms

/*
template <>
struct fmt::formatter<roq::mmaker::TargetQuotes> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(const roq::mmaker::TargetQuotes &value, Context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        "market {} bid_price {} ask_price {} bid_volume {} ask_volume {}"sv,
        value.market,
        value.bids.size()>0 ? value.bids[0].price.value : NAN,
        value.asks.size()>0 ? value.asks[0].price.value : NAN,
        value.bids.size()>0 ? value.bids[0].volume.value : NAN,
        value.asks.size()>0 ? value.asks[0].volume.value : NAN);
  }
};*/