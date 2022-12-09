#include "mbp_depth_array.hpp"
namespace roq {
namespace mmaker {

umm::DepthLevel MBPDepthArray::operator()(umm::Side side,
                                          std::size_t price_level) const {
  if (side == umm::Side::BUY) {
    if (price_level < size(side)) {
      return umm::DepthLevel{.price = bids[price_level].price,
                             .volume = bids[price_level].quantity,
                             .create_time = {}};
    } else
      return {};
  } else if (side == umm::Side::SELL) {
    if (price_level < size(side)) {
      return umm::DepthLevel{.price = asks[price_level].price,
                             .volume = asks[price_level].quantity,
                             .create_time = {}};
    } else
      return {};
  } else {
    assert(false);
    return {};
  }
}
void MBPDepthArray::update(const roq::cache::MarketByPrice &mbp,
                           std::size_t num_levels) {
  this->num_levels = num_levels;
  auto [bids_size, asks_size] = mbp.size();
  this->bids_storage.resize(bids_size);
  this->asks_storage.resize(asks_size);
  std::tie(bids, asks) = mbp.extract(this->bids_storage, this->asks_storage,
                                     /*.allow_truncate=*/false);
}
std::size_t MBPDepthArray::size(umm::Side side) const {
  switch (side) {
  case umm::Side::BUY:
    return num_levels ? std::min(this->bids.size(), num_levels)
                      : this->bids.size();
  case umm::Side::SELL:
    return num_levels ? std::min(this->asks.size(), num_levels)
                      : this->asks.size();
  default:
    assert(false);
    return 0;
  }
}
} // namespace mmaker
} // roq