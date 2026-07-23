// Copyright (c) 2026 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMI_COMPARE_H_
#define TMI_COMPARE_H_

#include <concepts>
#include <compare>

namespace tmi::detail
{

template <class T>
concept boolean_testable_impl = std::convertible_to<T, bool>;

template <class T>
concept boolean_testable = boolean_testable_impl<T> && requires(T&& t) {
    { !std::forward<T>(t) } -> boolean_testable_impl;
};

inline constexpr auto synth_three_way = []<class T, class U>(const T& t, const U& u)
requires requires {
    { t < u } -> boolean_testable;
    { u < t } -> boolean_testable;
}
{
    if constexpr (std::three_way_comparable_with<T, U>) {
        return t <=> u;
    } else {
        if (t < u)
            return std::weak_ordering::less;
        if (u < t)
            return std::weak_ordering::greater;
        return std::weak_ordering::equivalent;
    }
};

template <class T, class U = T>
using synth_three_way_result = decltype(synth_three_way(std::declval<T&>(), std::declval<U&>()));

}

#endif // TMI_COMPARE_H_
