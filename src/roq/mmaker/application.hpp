/* Copyright (c) 2021 Mikhail Mitkevich */

#pragma once

#include <string_view>
#include <span>
#include <roq/service.hpp>
#include <roq/client.hpp>

namespace roq {
namespace mmaker {

struct Application final : roq::Service  {
    using roq::Service::Service;

    int main(args::Parser const &) override;
};

}  // namespace mmaker
}  // namespace roq
