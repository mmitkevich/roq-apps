// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include <string_view>
#include "roq/core/basic_handler.hpp"
#include "roq/core/hash.hpp"
#include "roq/string_types.hpp"

namespace roq::core {

struct Accounts : core::BasicDispatch<Accounts> {
    template<class Fn>
    bool get_account(std::string_view exchange, Fn&& fn) const {
        return accounts_.get_value(exchange, std::forward<Fn&&>(fn));
    }
private:
    core::Hash<roq::Exchange, roq::Account> accounts_;
};


}