#pragma once

#include <memory>
#include <roq/string_types.hpp>
#include "roq/core/best_quotes.hpp"
#include "roq/core/exposure.hpp"
#include "roq/core/types.hpp"
// (c) copyright 2023 Mikhail Mitkevich
#include "roq/core/hash.hpp"


namespace roq::core {

struct Portfolio {
    template<class...Args>
    Portfolio(Args&&...args) : positions_ {std::forward<Args>(args)...} {}

    void set_position(core::MarketIdent ident, core::Exposure const& exposure) {
        positions_[ident] = exposure;
    }

    bool get_position(core::MarketIdent ident, std::invocable<core::Exposure const&> auto fn) const {
        auto iter = positions_.find(ident);
        if(iter!=std::end(positions_)) {
            fn(iter->second);
            return true;
        }
        return false;
    }

    void get_positions(std::invocable<core::Exposure const&> auto fn) {
        for(auto& [market_id, position] : positions_) {
            fn(position);
        }
    }

public:
    core::PortfolioIdent portfolio;
    roq::Account portfolio_name;      // arbitrary name (could be account name)
private:
    core::Hash<core::MarketIdent, core::Exposure> positions_;
};


} // roq::core