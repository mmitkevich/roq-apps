#pragma once

#include <memory>
#include <roq/string_types.hpp>
#include "roq/core/types.hpp"
#include "roq/core/hash.hpp"


namespace roq::core {

struct Portfolio {
    template<class...Args>
    Portfolio(Args&&...args) : positions_ {std::forward<Args>(args)...} {}

    void set_position(core::MarketIdent ident, core::Volume volume) {
        positions_[ident] = volume;
    }

    core::Volume get_position(core::MarketIdent ident) const {
        return positions_(ident, {});
    }

public:
    core::PortfolioIdent portfolio_id;
    roq::Account account;   // associated account if any
private:
    core::Hash<core::MarketIdent, core::Volume> positions_;
};


} // roq::core