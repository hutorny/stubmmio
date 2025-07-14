/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * src/pagerange.h - page range implementation
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#pragma once
#include <source_location>
#include <string_view>
#include <stubmmio/types.h>

namespace stubmmio::detail {
inline constexpr std::size_t page_size = 4096;
using pageid_type = std::uint32_t;

class pagerange {
public:
    using pointer_type = const volatile void*;
    using element_type = pageid_type;
    pagerange(pointer_type from, pointer_type to) noexcept
    : begin_{ptr2page(from)}, size_{ptrs2size(from, to)} { }
    pagerange(volatile_span sp) noexcept
    : begin_{ptr2page(&*sp.begin())}, size_{ptrs2size(&*sp.begin(), &*sp.end())} { }
    pagerange(const pagerange&) = default;
    pagerange(pagerange&&) = default;
    ~pagerange() = default;
    pagerange& operator=(const pagerange&) = default;
    pagerange& operator=(pagerange&&) = default;

    constexpr auto begin() const noexcept { return begin_; }
    constexpr auto end() const noexcept { return begin_ + size_; }
    constexpr auto size() const noexcept { return size_; }
    constexpr auto empty() const noexcept { return size_ == 0U; }
    auto pointer() const noexcept { return reinterpret_cast<void*>(std::uintptr_t{begin_} * page_size); }
    constexpr auto size_bytes() const noexcept { return std::size_t{size_} * page_size; }
    bool join(pagerange that) noexcept {
        if(! overlapping(that)) return false;
        auto maxend = std::max(end(), that.end());
        begin_ = std::min(begin_, that.begin_);
        size_ = maxend - begin_;
        return true;
    }
    constexpr bool overlapping(const pagerange& r) const noexcept {
        return (begin() <= r.begin() && r.begin() <= end()) ||
               (r.begin() <= begin() && begin() <= r.end());
    }
    constexpr bool operator==(const pagerange& r) const noexcept {
        return begin() == r.begin() && size() == r.size();
    }
    constexpr bool contains(const pagerange& r) const noexcept {
        return (begin() <= r.begin() && r.end() <= end());
    }
private:
    enum class round { down, up };
    template<round rounding = round::down>
    static element_type ptr2page(const volatile void* ptr) {
        auto address = reinterpret_cast<std::uintptr_t>(ptr);
        if constexpr(rounding == round::up) {
            address += page_size - 1;
        }
        address /= page_size;
        return static_cast<element_type>(address);
    }
    static element_type ptrs2size(pointer_type from, pointer_type to) {
        return ptr2page<round::up>(to) - ptr2page<round::down>(from);
    }
    element_type begin_;
    element_type size_;
};

} // namespace stubmmio::detail
