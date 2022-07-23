#include "application.hpp"
#include "quoters/active.hpp"

template<class Context, class Setup>
std::unique_ptr<umm::Quoter> make_quoter(Context& ctx, Setup& setup) {
    return umm::DynamicQuoter { mixer(ctx) };
}

int main(int argc, char **argv) {
  auto app = roq::mmaker::Application(argc, argv);
  app.context.factory = make_quoter<umm::Context, umm::Setup>;
  return app.run();
}