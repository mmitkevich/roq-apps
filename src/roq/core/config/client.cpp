#include "roq/core/config/client.hpp"
#include "roq/string_types.hpp"

namespace roq::core::config {
using namespace std::literals;

Client::Client(io::Context& io_context, web::rest::Client::Config const config/*={}*/)
{
  rest_ = web::rest::Client::create(*this, io_context, config);
}

  //using Callback = std::function<void(std::string_view const &request_id, Response const &)>;

  //virtual void operator()(std::string_view const &request_id, Request const &, Callback &&) = 0;

void Client::operator()(roq::ParametersUpdate const& u) {
  roq::RequestId request_id;

  web::rest::Request request {
    .method = web::http::Method::POST,
    .path = "/parameters"sv,
    .query = ""sv,
    //http::Accept accept;
    //http::ContentType content_type;
    //std::string_view headers;
    //std::string_view body;
    //io::QualityOfService quality_of_service = {};
  };

  (*rest_)(request_id, request, [&](auto&&...args) { (*this)(std::forward<decltype(args)>(args)...); });
}

void Client::operator()(std::string_view const &request_id, web::rest::Response const &response) {

}

} // roq::core::config