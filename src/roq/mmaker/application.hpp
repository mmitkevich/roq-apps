/* Copyright (c) 2021 Mikhail Mitkevich */

#pragma once

#include <string_view>
#include <span>
#include <roq/service.hpp>
#include "umm/quoter.hpp"
#include "context.hpp"
#include <roq/client.hpp>

namespace roq {
namespace mmaker {

struct Application final :  Service  {
    Application(int argc, char**argv);

    int main(std::span<std::string_view> args);

    int main(int argc, char **argv) override;

    mmaker::Context context;
};

}  // namespace mmaker
}  // namespace roq
