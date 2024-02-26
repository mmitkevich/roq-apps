#include "roq/core/config/client.hpp"
#include "roq/string_types.hpp"

namespace roq::core::config {
using namespace std::literals;

Client::Client(io::Context& io_context, std::string_view const &uri)
: uri_ {uri}
{
  URI parsed_uri {uri_};

  config = Client::Config {  
    .uris = std::span{&parsed_uri,1},
    .validate_certificate = false,                
    .connection = web::http::Connection::KEEP_ALIVE,  
    .user_agent = ROQ_PACKAGE_NAME,    
    .request_timeout = std::chrono::milliseconds{100},      
    .decode_buffer_size = 10 * 1024 * 1024,                                                                           
    .encode_buffer_size = 10 * 1024 * 1024,                 
    .allow_pipelining = false,                 
  };
  /*

  struct Config final {
    // connection
    std::string_view interface;
    std::span<io::web::URI const> uris;
    bool validate_certificate = {};
    // connection manager
    std::chrono::nanoseconds connection_timeout = {};
    std::chrono::nanoseconds disconnect_on_idle_timeout = {};
    http::Connection connection = {};
    // proxy
    io::web::URI proxy;
    // http
    std::string_view query;
    std::string_view user_agent;
    std::chrono::nanoseconds request_timeout = {};
    std::chrono::nanoseconds ping_frequency = {};
    std::string_view ping_path;
    // implementation
    size_t decode_buffer_size = {};
    size_t encode_buffer_size = {};
    bool allow_pipelining = false;
  };  
  */
  assert(&io_context);
  log::debug("URI {}", parsed_uri);
  rest_ = web::rest::Client::create(*this, io_context, config);
}

  //using Callback = std::function<void(std::string_view const &request_id, Response const &)>;

  //virtual void operator()(std::string_view const &request_id, Request const &, Callback &&) = 0;

void Client::operator()(roq::ParametersUpdate const& u) {
  roq::RequestId request_id;
  fmt::format_to(std::back_inserter(request_id), "put-parameters-{}", ++last_request_id);

  std::string body = "[";
  std::size_t i=0;
  for(auto const& p : u.parameters) {
    if(i++) {
      fmt::format_to(std::back_inserter(body), ",");
    }
    fmt::format_to(std::back_inserter(body), R"({{ 
        "label":"{}",
        "value":"{}",
        "account":"{}",
        "symbol":"{}",
        "exchange":"{}",
        "strategy_id":{}
      }})", p.label, p.value, p.account, p.symbol, p.exchange, p.strategy_id);
  }
  fmt::format_to(std::back_inserter(body), "]");
  
  web::rest::Request request {
      .method = web::http::Method::PUT,
      .path = "/api/parameters"sv,
      .query = ""sv,
      .content_type = roq::web::http::ContentType::APPLICATION_JSON,
      .body = body,
      //http::Accept accept;
      //http::ContentType content_type;
      //std::string_view headers;
      .quality_of_service = io::QualityOfService::IMMEDIATE,                                                            
  };
  log::debug("PUT {} {}", request.path, uri_);

  // I can't PUT here sicne connection is not ready. So I need to queue. Lot of boilerplate
  if((*rest_).can_try()) {
    (*rest_)(request_id, request, [&](auto&&...args) { (*this)(std::forward<decltype(args)>(args)...); });
    (*rest_).refresh({});
  }
}


void Client::operator()(roq::Event<roq::ParametersUpdate> const& event) {
  this->operator()(event.value);
}

void Client::operator()(std::string_view const &request_id, web::rest::Response const &response) {
  log::debug("request {} response {}",request_id, response);
}


void Client::operator()(Event<Timer> const& event) {
  if((*rest_).can_try()) {
    (*rest_).refresh({});
  }
}

void Client::start() {
  (*rest_).start();
}

void Client::stop() {
  (*rest_).stop();
}


} // roq::core::config