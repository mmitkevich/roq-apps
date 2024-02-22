/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include "roq/web/rest/client.hpp"

namespace roq::web::rest {

  struct BasicClientHandler :  web::rest::Client::Handler {
      using Client = web::rest::Client;
      using Connected  = typename Client::Connected;
      using Disconnected = typename Client::Disconnected;
      using Header = typename Client::Header;
      using Latency = typename Client::Latency;
      using Response = web::rest::Response;

      virtual void operator()(Trace<Connected> const &) override {}
      virtual void operator()(Trace<Disconnected> const &) override {};
    // note! headers may convey some global state, e.g. rate-limit usage
      virtual void operator()(Trace<Header> const &) override {}
      virtual void operator()(Trace<Latency> const &) override {};
  };
}

namespace roq::core::config {

struct Client final : web::rest::BasicClientHandler {
  using super = web::rest::BasicClientHandler;

  Client(io::Context& io_context, super::Client::Config const config = {});  

  void operator()(roq::ParametersUpdate const& u);

  void operator()(std::string_view const &request_id, super::Response const &response);

private:
  std::unique_ptr<super::Client> rest_;
};

}  // namespace roq
