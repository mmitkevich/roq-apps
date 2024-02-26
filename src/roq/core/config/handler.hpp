#pragma once
#include "roq/parameters_update.hpp"

namespace roq::core::config {

 
struct Handler {
    virtual void operator()(Event<ParametersUpdate> const& event) = 0;
};
   
}