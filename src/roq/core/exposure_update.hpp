// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include <roq/core/types.hpp>
#include <roq/side.hpp>
#include <roq/core/exposure.hpp>

namespace roq::core {

//enum class ExposureType {
//    UNDEFINED
//};

struct ExposureUpdate {
    //ExposureType update_type = ExposureType::UNDEFINED;
    std::span<const core::Exposure> exposure;
    core::PortfolioIdent portfolio {};    
    std::string_view portfolio_name;
//    core::StrategyIdent strategy_id {};
};

}

template <>
struct fmt::formatter<roq::core::ExposureUpdate> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::core::ExposureUpdate const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(portfolio="{}", )"
        R"(portolio_name={}, )"
        //R"(strategy_id="{}", )"
        R"(exposure=[{}], )"
        //R"(update_type={}, )"
        R"(}})"sv,
        value.portfolio,
        value.portfolio_name,
        //value.strategy_id,
        fmt::join(value.exposure, ", "sv));
        //value.update_type);
  }
};