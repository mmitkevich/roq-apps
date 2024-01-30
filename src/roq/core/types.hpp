// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include <cstdint>
#include <span>
#include "roq/core/fmt.hpp"
#include "roq/core/empty_value.hpp"
#include "roq/core/double.hpp"
#include "roq/clock.hpp"
namespace roq::core {

using Price = Double;
using Volume = Double;

using Ident = uint32_t;
using MarketIdent = core::Ident;
using PortfolioIdent = core::Ident;

struct PortfolioKey {
    core::PortfolioIdent portfolio;
    std::string_view portfolio_name;
};

using Integer = int64_t;
using Bool = bool;
using String = std::string;

using OrderIdent = uint64_t;
using OrderVersionIdent = uint32_t;


template<class T>
struct Range {
  T min;
  T max;
};

using TimePoint = std::chrono::nanoseconds;
using Duration = std::chrono::nanoseconds;

struct Manager;

} // roq::core
