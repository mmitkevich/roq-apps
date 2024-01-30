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
    ExposureType type;
    std::span<core::Exposure> exposure;
};

}