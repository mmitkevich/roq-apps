// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include "roq/core/config/manager.hpp"
#include "roq/core/config/toml_file.hpp"
#include "roq/core/exposure_update.hpp"
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
#include "roq/core/oms/manager.hpp"
#include "roq/core/market/manager.hpp"
#include "roq/core/portfolio/manager.hpp"

namespace roq {
namespace core {

struct Application;
struct Strategy;

using Factory = std::function<std::unique_ptr<core::Handler>(core::Strategy&)>;

struct Strategy final
: core::BasicHandler < Strategy
, client::Handler
, core::oms::Handler
, core::config::Handler
, core::portfolio::Handler
> {
    using Base = core::BasicHandler < Strategy
    , client::Handler
    , core::oms::Handler
    , core::config::Handler
    , core::portfolio::Handler
    >;
    using Self = Strategy;

    using Base::dispatch, Base::self;

    Strategy(Strategy const&) = delete;
    Strategy(Strategy&&) = delete;

    Strategy&operator=(Strategy const&) = delete;
    Strategy&operator=(Strategy&&) = delete;

    Strategy(client::Dispatcher& dispatcher, core::config::Manager& config, core::Factory factory, std::string strategy_name);

    template<class Config, class Node>
    void configure(Config& config, Node node) {
      core.configure(config, node);
      oms.configure(config, node["oms"]); 
    }

    ~Strategy();

    /// client::Handler

    void operator()(core::ExposureUpdate const& u, core::oms::Manager& oms) override;
    void operator()(core::ExposureUpdate const& u,  core::portfolio::Manager& portfolios) override;

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
        if constexpr(std::is_invocable_v<decltype(*pricer), decltype(event)>) {
          (*pricer)(event);        
        }
    }

    void operator()(const Event<TopOfBook> &event);
    void operator()(const Event<MarketByPriceUpdate>& event);
    void operator()(const Event<core::ExposureUpdate>& event);

    void operator()(const Event<core::Quotes>& event);

public:
    client::Dispatcher& dispatcher_;
    std::string strategy_name;
    core::config::Manager& config;
    // components
    core::Manager core;
    core::oms::Manager oms;
    std::unique_ptr<core::Handler> pricer;
    core::Factory & factory;
private:
    std::vector<Layer> layers_;
};



} // mmaker
} // roq