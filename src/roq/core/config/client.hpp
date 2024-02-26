/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include "roq/web/rest/client.hpp"
#include "roq/core/config/handler.hpp"

namespace roq::web::rest {

template<class...B>
struct BasicClientHandler :  web::rest::Client::Handler, B... {
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

struct Client final : web::rest::BasicClientHandler < config::Handler  > {
  using super = web::rest::BasicClientHandler < config::Handler >;
  using Config = super::Client::Config;
  using URI = io::web::URI;

  Client(io::Context& io_context, std::string_view const &uri);  

  void operator()(roq::ParametersUpdate const& u);

  virtual void operator()(Event<ParametersUpdate> const& event);

  void operator()(std::string_view const &request_id, super::Response const &response);

  void operator()(Event<Timer> const& event);

  void start();
  void stop();
private:
  
  std::unique_ptr<super::Client> rest_;
  Config config;
  roq::String<4096> uri_;
  uint32_t last_request_id;
};

}  // namespace roq
