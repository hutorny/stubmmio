/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * opmakers.h - stubmmio operator makers
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <source_location>
#include <type_traits>
#include <utility>

namespace stubmmio {

class region;
template<typename Type>
concept trivial_data
   = std::is_trivially_copyable_v<Type>
  && std::is_trivially_destructible_v<Type>
  && std::has_unique_object_representations_v<Type>
  && !std::is_same_v<Type, region>;

template<typename Operator>
concept anoperator = requires(unsigned v, void* p) {
    typename Operator::operator_type;
    Operator::one(v, std::source_location::current())(p, p);
    Operator::all(v, std::source_location::current())(p, p);
};

namespace detail {
struct void_range {
    const volatile void* begin;
    const volatile void* end;
};
void ensure_size_match(void_range, std::size_t, std::source_location);
void ensure_size_multiplyof(void_range, std::size_t, std::source_location);
void ensure_region_is_not_reversed(void_range, std::source_location);
} // namespace detail


constexpr auto empty() noexcept { return [](void*, void*) noexcept {}; }

struct generator {
    using siganture = void(void*, void*);
    using operator_type = std::function<siganture>;

    static auto none() noexcept { return [](void*, void*) noexcept {}; }

    static constexpr auto one(trivial_data auto v, std::source_location loc = std::source_location::current()) {
        using value_type = std::remove_cvref_t<decltype(v)>;
        return [v = std::move(v), loc](void* b, void* e) {
            detail::ensure_size_match({b, e}, sizeof(value_type), loc);
            *static_cast<value_type*>(b) = v;
        };
    }

    static  constexpr auto all(trivial_data auto v, std::source_location loc = std::source_location::current()) {
        using value_type = std::remove_cvref_t<decltype(v)>;
        return [v = std::move(v), loc](void* b, void* e) {
            detail::ensure_size_multiplyof({b, e}, sizeof(value_type), loc);
            std::fill(static_cast<value_type*>(b), static_cast<value_type*>(e), v);
        };
    }
};
static_assert(anoperator<generator>);

struct comparator {
    using siganture = bool(const void*, const void*);
    using operator_type = std::function<siganture>;

    static constexpr auto one(trivial_data auto v, std::source_location loc = std::source_location::current()) {
        using value_type = std::remove_cvref_t<decltype(v)>;
        return [v = std::move(v), loc](const void* b, const void* e) -> bool {
            detail::ensure_size_match({b, e}, sizeof(value_type), loc);
            return std::memcmp(b, &v, sizeof(value_type)) == 0;
        };
    }

    static constexpr auto all(/*trivial_data*/ auto v, std::source_location loc = std::source_location::current()) {
        using value_type = std::remove_cvref_t<decltype(v)>;
        return [v = std::move(v), loc](const void* b, const void* e) {
            detail::ensure_size_multiplyof({b, e}, sizeof(value_type), loc);
            for(auto i = static_cast<const value_type*>(b); i != static_cast<const  value_type*>(e); ++i) {
                if (std::memcmp(i, &v, sizeof(value_type)) != 0)
                    return false;
            }
            return true;
        };
    }
};
static_assert(anoperator<comparator>);
} // namespace stubmmio
