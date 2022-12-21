#pragma once
#include "umm/prologue.hpp"
#include <cmath>

namespace roq {
namespace mmaker {

struct ProfitLoss {
    enum class Metric {
        UNDEFINED=0,
        RPNL=1,
        PNL=2,
        BUY_QTY=3,
        SELL_QTY=4,
        BUY_AMT=5,
        SELL_AMT=6,
        BUY_CNT=7,
        SELL_CNT=8,
    };

    double buy_qty {0};
    double sell_qty {0};
    double buy_amt {0};
    double sell_amt {0};
    std::size_t buy_cnt {0};
    std::size_t sell_cnt {0};

    void reset() {
        buy_qty = sell_qty = 0;
        buy_amt = sell_amt = 0;
        buy_cnt = sell_cnt = 0;
    }

    void update(double price, double qty) {
        if(qty>0) {
            buy_qty += qty;
            buy_amt += qty*price;
            buy_cnt++;
        } else {
            sell_qty += qty;
            sell_amt += -qty*price;
            sell_cnt++;
        }
    }
    
    double pnl(double price) const {
        double pnl = sell_amt - buy_amt + (buy_qty-sell_qty) * price;
        return pnl;
    }

    double rpnl() const {
        double avg_sell = sell_amt/sell_qty;
        double avg_buy = buy_amt/buy_qty;
        return (avg_sell-avg_buy)*std::min(buy_qty, sell_qty);
    }

    double operator()(Metric what, double price=NAN) const {
        switch(what) {
            case Metric::PNL: return pnl(price);
            case Metric::RPNL: return rpnl();
            case Metric::BUY_AMT: return buy_amt;
            case Metric::SELL_AMT: return sell_amt;
            case Metric::BUY_QTY: return buy_qty;
            case Metric::SELL_QTY: return sell_qty;
            default: return NAN;
        }
    }

};

} // mmaker
} // roq