#pragma once

#include <memory>
#include <roq/string_types.hpp>
#include "roq/core/types.hpp"
// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/hash.hpp"


namespace roq::core {

struct PortfolioKey {
    core::PortfolioIdent portfolio;
    std::string_view name;
};

struct Portfolio {
    template<class...Args>
    Portfolio(Args&&...args) : positions_ {std::forward<Args>(args)...} {}

    void set_position(core::MarketIdent ident, core::Volume volume) {
        positions_[ident] = volume;
    }

    core::Volume get_position(core::MarketIdent ident) const {
        return positions_(ident, {});
    }

    void get_positions(std::invocable<core::MarketIdent, core::Volume> auto callback) {
        for(auto& [market_id, position] : positions_) {
            callback(market_id, position);
        }
    }
public:
    core::PortfolioIdent portfolio;
    roq::Account name;   // associated account if any
private:
    core::Hash<core::MarketIdent, core::Volume> positions_;
};


} // roq::core