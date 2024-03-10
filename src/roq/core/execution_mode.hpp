#pragma once
namespace roq::core {

enum class ExecutionMode {
  UNDEFINED,
  CROSS,
  JOIN,
  JOIN_PLUS,     // join our side plus 1 tick to be top in the queue
  CROSS_MINUS,  // passive cross
};

}