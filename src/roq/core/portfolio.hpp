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

public:
    core::PortfolioIdent portfolio;
    roq::Account account;   // associated account if any
private:
    core::Hash<core::MarketIdent, core::Volume> positions_;
};


} // roq::core