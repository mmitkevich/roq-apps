#include "application.hpp"
#include "umm/quoter.hpp"
#include "umm/context.hpp"
#include "quoters/quoters.hpp"

int main(int argc, char **argv) {
  auto app = roq::mmaker::Application(argc, argv);
  return app.run();
}