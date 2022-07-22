#include "roq/umm/application.hpp"

int main(int argc, char **argv) {
  using namespace std::literals;
  return roq::umm::Application(argc, argv).run();
}
