#pragma once

#include <fmt/format.h>
#include <fmt/compile.h>

#define ROQ_CORE_FMT_DECL(T, fmt_str, ...)\
template <>\
struct fmt::formatter<T> {\
  template <typename Context>\
  constexpr auto parse(Context &context) {\
    return std::begin(context);\
  }\
  template <typename Context>\
  auto format(const T &_, Context &context) const {\
    using namespace std::literals;\
    return fmt::format_to(context.out(), fmt_str, __VA_ARGS__);\
  }\
};

#define ROQ_CORE_FMT_DOUBLE "{:.10g}"

/// Example:  
/// ROQ_CORE_DECLARE_FORMATTER(Double, ROQ_CORE_DOUBLE_FMT, std::roundl(value/1E-6)*1E-6)


