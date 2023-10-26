// (c) copyright 2023 Mikhail Mitkevich
#pragma once
#include <cmath>

namespace roq::core {

template<class T>
constexpr T empty_value() { return {}; }

//template<class T>
//constexpr bool is_empty_value(T val) { return false; }

template<class T>
constexpr bool is_empty_value(const T& val) { return val.empty(); }

template<>
constexpr bool is_empty_value(const double& val) {  return std::isnan(val); }


} // roq::core