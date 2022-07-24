#pragma once
#include "umm/context.hpp"
#include "umm/quoter.hpp"
#include "umm/type_list.hpp"

namespace umm {
namespace quoters {

template<class...Quoters>
struct Factory {
  using QuotersTL = umm::type_list<Quoters...>;

  std::unique_ptr<umm::Quoter> operator()(umm::Context& ctx) {
    const auto& cfg = ctx.config;
    auto root = cfg.root();
    auto quoter_name = cfg.get_string("app.model");
    return create(ctx, quoter_name);
  }

  std::unique_ptr<umm::Quoter> create(umm::Context& ctx, std::string_view quoter_name) {
    std::unique_ptr<Quoter> result;
    umm::for_each(QuotersTL{}, [&](std::size_t idx, auto tl) {
      using QuoterT = typename decltype(tl)::type;
      if (QuoterT::name() == quoter_name) {
        result =  to_dynamic(ctx % QuoterT{});
      }
    });
    return result;
  }
};

} // quoters
} // umm