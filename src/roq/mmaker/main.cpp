#include "application.hpp"
#include "umm/quoter.hpp"
#include "umm/context.hpp"
#include "umm/config.hpp"
#include "quoters/quoters.hpp"

int main(int argc, char **argv) {
  auto app = roq::mmaker::Application(argc, argv);
  app.context.factory = umm::quoters::factory;
  return app.run();
}