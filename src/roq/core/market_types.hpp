// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include <roq/core/types.hpp>
#include <roq/string_types.hpp>
#include <roq/

namespace roq::core {


struct SymbolExchange {
    roq::Exchange exchange;        
    roq::Symbol symbol;

    template <typename H>
    friend H AbslHashValue(H h, const SymbolExchange& self) {
        return H::combine(std::move(h), self.exchange, self.symbol);
    }
    bool operator==(const SymbolExchange& that) const {
        return exchange==that.exchange && symbol==that.symbol;
    }
};

}