// (c) copyright 2023 Mikhail Mitkevich
#pragma once

#include <roq/core/types.hpp>
#include <roq/side.hpp>
#include <roq/core/exposure.hpp>

namespace roq::core {

enum class ExposureType {
    UNDEFINED
};

struct ExposureUpdate {
    ExposureType type = ExposureType::UNDEFINED;
    std::span<core::Exposure> exposure;
    core::PortfolioIdent portfolio {};    
    std::string_view portfolio_name;
    std::string_view account;
};

}