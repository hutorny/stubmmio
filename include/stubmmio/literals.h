/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * literals.h - stubmmio literals _U, _U8, _U16, _U32
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#pragma once

#include <cstdint>
#include <limits>

namespace stubmmio {
namespace detail {
inline consteval auto pow(unsigned long long base, unsigned exponent) noexcept {
    auto result = base;
    while(--exponent != 0) result *= base;
    return result;
}
template <char Char>
inline constexpr auto hex_digit = Char >= 'A' && Char <= 'F'
  ? (static_cast<unsigned long long>(Char - 'A') + 10ULL)
  : Char >= 'a' && Char <= 'f' ? (static_cast<unsigned long long>(Char - 'a') + 10ULL)
  : static_cast<unsigned long long>(Char - '0');

template<unsigned long long Radix, char First, char ... List>
consteval unsigned long long simple_literal_value() noexcept {
    if constexpr(sizeof...(List) == 0) {
        return First - '0';
    } else {
        constexpr auto scale = pow(Radix, sizeof...(List));
        return ((First - '0') * scale) + simple_literal_value<Radix, List...>();
    }
}

template<unsigned long long Radix>
consteval unsigned long long simple_literal_value() noexcept {
    return 0ULL;
}


template<char First, char ... List>
consteval unsigned long long hexadecimal_literal_value() noexcept {
    if constexpr(sizeof...(List) == 0) {
        return hex_digit<First>;
    } else {
        constexpr auto scale = pow(16ULL, sizeof...(List));
        return (hex_digit<First> * scale) + hexadecimal_literal_value<List...>();
    }
}
template<char ... List>
struct literal_impl {
    static constexpr auto value = detail::simple_literal_value<10ULL, List...>();
};

template<char ... List>
struct literal_impl<'0', 'b', List...> {
    static constexpr auto value = simple_literal_value<2ULL, List...>();
};

template<char ... List>
struct literal_impl<'0', 'B', List...> {
    static constexpr auto value = simple_literal_value<2ULL, List...>();
};

template<char ... List>
struct literal_impl<'0', 'x', List...> {
    static constexpr auto value = hexadecimal_literal_value<List...>();
};

template<char ... List>
struct literal_impl<'0', 'X', List...> {
    static constexpr auto value = hexadecimal_literal_value<List...>();
};

template<char ... List>
struct literal_impl<'0', List...> {
    static constexpr auto value = simple_literal_value<8ULL, List...>();
};

}
namespace literals {

template<char ... List>
consteval unsigned operator ""_U() {
    constexpr auto result = detail::literal_impl<List...>::value;
    static_assert(result <= static_cast<decltype(result)>(std::numeric_limits<unsigned>::max()), "literal exceeds limit");
    return static_cast<unsigned>(result);
}
template<char ... List>
consteval std::uint8_t operator ""_U8() {
    constexpr auto result = detail::literal_impl<List...>::value;
    static_assert(result <= static_cast<decltype(result)>(std::numeric_limits<std::uint8_t>::max()), "literal exceeds limit");
    return static_cast<uint8_t>(result);
}
template<char ... List>
consteval std::uint16_t operator ""_U16() {
    constexpr auto result = detail::literal_impl<List...>::value;
    static_assert(result <= static_cast<decltype(result)>(std::numeric_limits<std::uint16_t>::max()), "literal exceeds limit");
    return static_cast<uint16_t>(result);
}
template<char ... List>
consteval std::uint32_t operator ""_U32() {
    constexpr auto result = detail::literal_impl<List...>::value;
    static_assert(result <= static_cast<decltype(result)>(std::numeric_limits<std::uint32_t>::max()), "literal exceeds limit");
    return static_cast<uint32_t>(result);
}

} // namespace literals
} // namespace stubmmio
