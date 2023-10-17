#pragma once
#include <cmath>

#include "roq/core/fmt.hpp"

namespace roq::core {

struct Double  {
    double value {NAN};
        
    constexpr Double() = default;
    constexpr Double(double val) : value(val) {}
    void clear() { value = NAN; }
    constexpr operator double() const { /*assert(!empty());*/ return value; }
    constexpr operator double&() { return value; }
    constexpr bool empty() const  { return std::isnan(value); }

    #define ROQ_CORE_ARITHMETIC_OPS_DECL(Class) \
    template<class RHS> \
    constexpr Class operator+(const RHS& rhs) const { double val = rhs; return value+val; } \
    template<class RHS> \
    constexpr Class operator-(const RHS& rhs) const { double val = rhs; return value-val; } \
    template<class RHS> \
    constexpr Class operator*(const RHS& rhs) const { double val = rhs; return value*val; } \
    template<class RHS> \
    constexpr Class operator/(const RHS& rhs) const { double val = rhs; return value/val; } \
    constexpr Class operator-() const { return -value; }

    ROQ_CORE_ARITHMETIC_OPS_DECL(Double)

    constexpr static inline double epsilon = 1e-6;

    int compare(const Double& rhs) const {
        if(!empty() && !rhs.empty()) {
            auto diff = value-rhs.value;
            if(diff > epsilon )
                return 1;
            if(diff < -epsilon)
                return -1;
            return 0;
        }
        if(empty() && rhs.empty())
            return 0;
        if(empty())
            return -1;
        // if(rhs.empty())
        return 1;
    }

    #define ROQ_CORE_COMPARE_OPS_DECL(OP) \
    template<class RHS> \
    constexpr bool operator OP (const RHS& rhs) const { \
        auto cmp =  compare(Double{rhs}); \
        return cmp OP 0; \
    }

    ROQ_CORE_COMPARE_OPS_DECL(==)
    ROQ_CORE_COMPARE_OPS_DECL(!=)
    ROQ_CORE_COMPARE_OPS_DECL(<)
    ROQ_CORE_COMPARE_OPS_DECL(>)
    ROQ_CORE_COMPARE_OPS_DECL(<=)
    ROQ_CORE_COMPARE_OPS_DECL(>=)
};

} // roq::core


ROQ_CORE_FMT_DECL(roq::core::Double, ROQ_CORE_FMT_DOUBLE, _.value)
    