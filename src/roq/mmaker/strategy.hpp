// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/config/manager.hpp"
#include "roq/core/handler.hpp"
#include "roq/core/manager.hpp"
#include "roq/client.hpp"
#include "roq/core/basic_handler.hpp"
#include <absl/container/flat_hash_map.h>
#include <roq/cache/gateway.hpp>
#include <roq/client/config.hpp>
#include <roq/client/dispatcher.hpp>
#include <roq/layer.hpp>
#include <roq/top_of_book.hpp>
#include <type_traits>

//#include "roq/mmaker/mbp_depth_array.hpp"
#include "roq/oms/manager.hpp"
//#include "roq/mmaker/publisher.hpp"
//#include "umm/core/context.hpp"
//#include "umm/core/type.hpp"
//#include "umm/core/type/depth_level.hpp"
//#include "umm/core/event.hpp"
//#include "umm/core/model_api.hpp"
//#include "umm/core/model.hpp"
//#include "application.hpp"
//#include "./context.hpp"
#include "roq/core/markets.hpp"
#include "roq/dag/pricer.hpp"

namespace roq {
namespace mmaker {

struct Application;


struct Strategy final
: core::BasicHandler < Strategy
, client::Handler
, oms::Handler
, config::Handler
> {
    using Base = core::BasicHandler < Strategy
    , client::Handler
    , oms::Handler
    , config::Handler
    >;
    using Self = Strategy;

    using Base::dispatch, Base::self;

    Strategy(Strategy const&) = delete;
    Strategy(Strategy&&) = delete;

    Strategy&operator=(Strategy const&) = delete;
    Strategy&operator=(Strategy&&) = delete;

    Strategy(client::Dispatcher& dispatcher, Application& context);

    virtual std::unique_ptr<core::Handler> make_pricer();

    template<class Config, class Node>
    void configure(Config& config, Node node) {
      core.configure(config, node);
      oms.configure(config, node["oms"]); 
    }

    ~Strategy();

    /// client::Handler

    void operator()(const Event<core::ExposureUpdate>& event);

    // route events
    template<class T>
    void dispatch(const roq::Event<T> &event) {
        Base::dispatch(event);
        if constexpr(std::is_invocable_v<decltype(oms), decltype(event)>) {
          oms(event);
        }
        if constexpr(std::is_invocable_v<decltype(core), decltype(event)>) {
          core(event);
        }
        if constexpr(std::is_invocable_v<decltype(pricer), decltype(event)>) {
          pricer(event);        
        }
    }

    void operator()(const Event<TopOfBook> &event);
    void operator()(const Event<MarketByPriceUpdate>& event);

    /// IQuoter::Handler
    //void dispatch(const umm::Event<umm::QuotesUpdate> &) override;

private:
    //core::Hash<core::MarketIdent, bool> umm_mbp_snapshot_sent_;
private:
    client::Dispatcher& dispatcher_;
    std::string strategy_name;
    std::vector<Layer> layers_;
    config::Manager& config;
    // components
    core::Manager core;
    oms::Manager oms;
    
    std::unique_ptr<core::Handler> pricer;
    
    // pricer::Manager pricer;
    // liqs::Manager pricer;

};



} // mmaker
} // roq