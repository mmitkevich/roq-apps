#include "roq/api.hpp"
#include "strategy.hpp"

namespace roq {
namespace umm {


void Strategy::operator()(const Event<Timer> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<Connected> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<Disconnected> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<DownloadBegin> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<DownloadEnd> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<GatewayStatus> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<ReferenceData> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<MarketStatus> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<MarketByPriceUpdate> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<OrderAck> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<OrderUpdate> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<TradeUpdate> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<PositionUpdate> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<FundsUpdate> &event) {
  dispatch(event);
}
void Strategy::operator()(const Event<RateLimitTrigger> &event) {
  dispatch(event);
}


} // umm
} // roq