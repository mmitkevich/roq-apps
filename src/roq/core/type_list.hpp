#pragma once
#include <utility>
// https://github.com/veselink1/refl-cpp/blob/master/include/refl.hpp

namespace roq {

template <typename... Ts>
struct type_list
{
	/** The number of types in this type_list */
	static constexpr std::size_t size = sizeof...(Ts);
};

template <typename T>
struct type_list<T>
{
	typedef T type;
	static constexpr std::size_t size = 1;
};

template <typename T>
using type_c = type_list<T>;

namespace detail {
template <typename F>
constexpr void eval_in_order(type_list<>, std::index_sequence<>, [[maybe_unused]]F&& f)
{
}

template <typename F, typename T>
constexpr auto invoke_optional_index(F&& f, type_c<T> t, std::size_t idx, int) -> decltype(f(idx, std::forward<decltype(t)>(t)))
{
	return f(idx, std::forward<decltype(t)>(t));
}

// This workaround is needed since C++ does not specify
// the order in which function arguments are evaluated and this leads
// to incorrect order of evaluation (noticeable when using indexes).
template <typename F, typename T, typename... Ts, std::size_t I, std::size_t... Idx>
constexpr void eval_in_order(type_list<T, Ts...>, std::index_sequence<I, Idx...>, F&& f)
{
	static_assert(std::is_trivial_v<T>, "Argument is a non-trivial type!");

	invoke_optional_index(f, type_c<T>{}, I, 0);
	return eval_in_order(
		type_list<Ts...>{},
		std::index_sequence<Idx...>{},
		std::forward<F>(f)
	);
}
} // detail 

template <typename F, typename... Ts>
constexpr void for_each(type_list<Ts...> list, F&& f)
{
	detail::eval_in_order(list, std::make_index_sequence<sizeof...(Ts)>{}, std::forward<F>(f));
}


/// container for meta-function
template<template<class...> class Fn>
struct fn_c {
    template<class...Args>
    using fn = Fn<Args...>;
};

/// apply meta-function with args
template<class Fn, class...Args>
using fn_apply_t = typename Fn::template fn<Args...>;

template<auto arg>
struct Const {
	using value_type = decltype(arg);
    constexpr static inline value_type value = arg;
	constexpr Const() = default;
	constexpr Const(value_type) {}
};


} // roq::core
