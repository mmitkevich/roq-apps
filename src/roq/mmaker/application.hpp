/* Copyright (c) 2021 Mikhail Mitkevich */

#pragma once

#include <string_view>
#include <span>
#include <roq/service.hpp>
#include <roq/client.hpp>

namespace roq {
namespace mmaker {

struct Application final : roq::Service  {
    Application(int argc, char**argv);

    int main(std::span<std::string_view> args);

    int main(int argc, char **argv) override;
};

}  // namespace mmaker
}  // namespace roq
