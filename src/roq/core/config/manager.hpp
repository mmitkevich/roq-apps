#pragma once

#include "roq/core/config/toml_file.hpp"
#include <roq/parameter.hpp>
#include <roq/parameters_update.hpp>
#include <roq/string_types.hpp>
#include <roq/client/config.hpp>
//#include "roq/core/config/client.hpp"
#include "roq/io/context.hpp"
#include "roq/core/config/handler.hpp"
#include "roq/timer.hpp"

namespace roq::core::config {

struct Query {
    std::string_view label;    
    std::string_view symbol;
    std::string_view exchange;
    std::string_view account;
    core::StrategyIdent strategy_id;

    bool operator()(const roq::Parameter& item) const;
};

enum class ConfigType {
    UNDEFINED=0,
    TOML=1
};

struct Manager : client::Config {
    ConfigType config_type;

    TomlFile toml;
    std::string url;
    bool is_downloading = false;
    roq::User user;
    std::chrono::nanoseconds start_time_ {};

    std::vector<roq::Parameter> parameters;

    //std::unique_ptr<config::Client> client_;

    Manager() = default;
    Manager(io::Context& io_context);
    ~Manager();

    void operator()(Query const& query, std::invocable<roq::Parameter const&> auto&& fn) const {
        for(auto& item : parameters) {
          if(query(item))
            fn(item);
        }
    }
    
    template<class Strategy>
    void configure(Strategy& strategy) {
          strategy.configure(toml, toml.get_root());
          // FIXME: 
          this->dispatch(strategy);
    }

    void dispatch(client::Config::Handler& handler) const override;

    // cache only
    void load(std::string_view url);

    void dispatch(config::Handler& handler);

    // from strategy to override
    void operator()(Event<ParametersUpdate> const& event);

    void operator()(Event<roq::Timer> const& event);
};



} // roq::config