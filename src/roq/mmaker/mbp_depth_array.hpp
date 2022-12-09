#pragma once

#include <roq/cache/market_by_price.hpp>
#include "umm/core/type/depth_array.hpp"
#include "umm/printable.hpp"
#include "umm/core/type/depth_level.hpp"

namespace roq {
namespace mmaker {

/// view for roq::cache::MarketByPrice
struct MBPDepthArray : umm::BasicPrintable<MBPDepthArray> {
    std::vector<roq::MBPUpdate> bids_storage;
    std::vector<roq::MBPUpdate> asks_storage;
    std::span<roq::MBPUpdate> bids { bids_storage.data(), bids_storage.size() };
    std::span<roq::MBPUpdate> asks { asks_storage.data(), asks_storage.size() };
    umm::Double tick_size;                       // required when exclude_empty=false!
    std::size_t num_levels = 0;             // max depth levels
    bool exclude_empty {true};              // empty levels are excluded. indexing not possible
    bool dirty {true};                     // cum_vol not calculaed

    std::size_t size(umm::Side side) const;

    void update(const roq::cache::MarketByPrice &mbp,
                std::size_t num_levels = 0);

    umm::DepthLevel operator()(umm::Side side, std::size_t price_level) const;

    umm::BestPrice best_price() const {
        umm::BestPrice bp;
        if (!bids.empty()) {
            bp.bid_price = bids[0].price;
            bp.bid_volume = bids[0].quantity;
        }
        if (!asks.empty()) {
            bp.ask_price = asks[0].price;
            bp.ask_volume = asks[0].quantity;
        }
        return bp;
    }

    template<class Fn>
    void iterate(umm::Side side, Fn&& fn) const {
        for(std::size_t i=0; i < size(side); i++) {
            fn(this->operator()(side, i));
        }
    }
/*
    const umm::DepthLevel bid(std::size_t price_level) const { 
        assert(price_level< size(umm::Side::BUY)); 
        assert(price_level>=0);  
        const auto& bid = bids[price_level];
        return umm::DepthLevel {
            .price = bid.price,
            .volume = bid.quantity,
            //.create_time = ? TODO
        };
    }
    
    const umm::DepthLevel ask(std::size_t price_level) const { 
        assert(price_level< size(umm::Side::SELL)); 
        assert(price_level>=0);  
        const auto& ask = asks[price_level];
        return umm::DepthLevel {
            .price = ask.price,
            .volume = ask.quantity,
            // .create_time = ? TODO
        };
    }

    const umm::Price bid_price() const {
        if(bids_size())
            return bids[0].price;
        return {};
    }

    const umm::Price ask_price() const {
        if(asks_size())
            return asks[0].price;
        return {};
    }

    const umm::Volume bid_volume() const {
        if(bids_size())
            return bids[0].quantity;
        return {};
    }

    const umm::Volume ask_volume() const {
        if(asks_size())
            return asks[0].quantity;
        return {};
    }
*/
    template<class Format, class Context>
    decltype(auto) format(Format& fmt, const Context& ctx) const {
        fmt::format_to(fmt.out(),"(tick_size={} num_levels={}) *** bids *** [{}] {{", tick_size, num_levels, bids.size());
        std::size_t n;
        n=0;
        for(auto& bid: bids) {
            //if(bid.quantity>0)
                fmt::format_to(fmt.out(), " {{ px {} qty {} }}", bid.price, bid.quantity);
            if(++n>=umm::MAX_DEPTH_LOG) {
                fmt::format_to(fmt.out(), "... {} ommited", bids.size()-n);
                break;
            }
        }
        n=0;
        fmt::format_to(fmt.out(), " }} *** asks *** [{}] {{", asks.size());
        for(auto& ask: asks) {
            //if(ask.quantity>0)
                fmt::format_to(fmt.out(), " {{ px {} qty {} }}", ask.price, ask.quantity);
            if(++n>=umm::MAX_DEPTH_LOG) {
                fmt::format_to(fmt.out(), "... {} ommited", asks.size()-n);
                break;
            }
        }
        fmt::format_to(fmt.out(), " }}");
        return fmt.out();
    }
};

} // mmaker
} // roq