#include "compute.hpp"

namespace roq::pricer {

void Context::clear() {
    quotes = {};
    exposure = {};
}

}