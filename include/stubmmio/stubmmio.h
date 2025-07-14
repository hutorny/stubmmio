/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * stubmmio.h - main header for the stubmmio lib
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#pragma once
#include <initializer_list>
#include <functional>
#include <map>
#include <source_location>
#include <stdexcept>
#include <stubmmio/types.h>
#include <stubmmio/operators.h>

namespace stubmmio {
enum class onfail { returns, throws, logs };
class arena {
public:
    static constexpr std::uintptr_t max_size = 0x100000000UL;
    arena() = delete;

    static void size(std::uintptr_t requested_size, onfail on_fail = onfail::throws) {
        if (check_boundary(requested_size, on_fail))
            size_ = requested_size;
    }
    static std::uintptr_t size() noexcept { return size_; }
    static bool check_boundary(std::uintptr_t size = max_size, onfail on_fail = onfail::throws);
    static bool check_pagesize(int pagesize, onfail on_fail = onfail::throws);
private:
    inline static std::uintptr_t size_ = max_size;
};

namespace exceptions {
struct duplicate_address final : std::logic_error {
    using std::logic_error::logic_error;
};
struct overlapping_elements final : std::logic_error {
    using std::logic_error::logic_error;
};
struct page_size_mismatch final : std::logic_error {
    using std::logic_error::logic_error;
};
struct region_reversed final : std::logic_error {
    using std::logic_error::logic_error;
};
struct conflicting_allocation final : std::logic_error {
    using std::logic_error::logic_error;
};
struct page_is_not_allocated final : std::logic_error {
    using std::logic_error::logic_error;
};
struct arena_is_not_fully_available final : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct access_to_unallocated_address final : std::runtime_error {
    using std::runtime_error::runtime_error;
};
}

namespace detail {
using file_name_type = decltype(std::source_location::current().file_name());
using line_numb_type = decltype(std::source_location::current().line());
} // namespace detail

class region {
public:
    using address_type = std::uintptr_t;
    using size_type = std::size_t;
    /// region of (un)specified size, staring at address
    constexpr region(address_type address, size_type size_in_bytes) noexcept
      : address_ { address }, size_{size_in_bytes} {}
    /// region of specified size, staring at address
    constexpr region(address address, size_type size_in_bytes) noexcept
      : address_ { static_cast<address_type>(address) }, size_{size_in_bytes} {}
    /// region of specified size, staring at pointer
    constexpr region(void* pointer, size_type size_in_bytes) noexcept
      : pointer_ {pointer}, size_{size_in_bytes} {}
    /// region of specified size, staring at volatile pointer
    constexpr region(volatile void* pointer, size_type size_in_bytes) noexcept
      : volatile_pointer_ {pointer}, size_{size_in_bytes} {}
    /// region of predefined native size, staring at pointer
    template<std::unsigned_integral T>
    constexpr region(T* pointer) noexcept
      : pointer_ {pointer}, size_{sizeof(T)} {}
    /// region of predefined native size, staring at volatile pointer
      template<std::unsigned_integral T>
    constexpr region(volatile T* pointer) noexcept
      : volatile_pointer_ {pointer}, size_{sizeof(T)} {}
    /// region, staring at begin and ending before end
    template<trivial_data T>
    region(T* begin, T* end, std::source_location location = std::source_location::current())
      : pointer_{begin}, size_ { static_cast<size_type>(end - begin) * sizeof(T) } {
          detail::ensure_region_is_not_reversed({begin, end}, location);
      }
    /// volatile region, staring at begin and ending before end
    template<trivial_data T>
    region(volatile T* begin, volatile T* end, std::source_location location = std::source_location::current())
      : volatile_pointer_{begin}, size_ { static_cast<size_type>(end - begin) * sizeof(T) } {
          detail::ensure_region_is_not_reversed({begin, end}, location);
      }
    constexpr region(const region&) = default;
    constexpr region(region&&) = default;
    region& operator=(const region&) = default;
    region& operator=(region&&) = default;
    ~region() = default;
    constexpr auto addr() const noexcept { return address_; }
    constexpr auto size() const noexcept { return size_; }
    constexpr auto operator==(const region& that) const noexcept {
        return addr() == that.addr() && size() == that.size();
    }
    template<typename T = void>
    auto begin() const noexcept { return reinterpret_cast<T*>(address_); }
    template<typename T = void>
    auto end() const noexcept { return reinterpret_cast<T*>(address_ + size_); }
private:
    union {
        address_type address_;
        void* pointer_;
        volatile void* volatile_pointer_;
    };
    size_type size_;
};

inline constexpr auto operator<=>(const region& lhs, const region& rhs) noexcept {
    return lhs.addr() == rhs.addr() ? lhs.size() <=> rhs.size() : lhs.addr() <=> rhs.addr();
}

inline constexpr auto overlapping(const region& a, const region& b) noexcept {
    return (a.addr() <= b.addr() && b.addr() < (a.addr() + a.size())) ||
           (b.addr() <= a.addr() && a.addr() < (b.addr() + b.size()));
}

template<anoperator Operator>
class element {
public:
    using operator_type = typename Operator::operator_type;
    /// primary constructor
    element(region region, operator_type value, std::source_location location = std::source_location::current())
      : region_{region}, operator_{std::move(value)}, location_ {location} {}
    /// uninitialized element
    element(region region, std::source_location location = std::source_location::current())
      requires requires { Operator::none(); }
      : element(region, Operator::none(), location) {}
    template<trivial_data DataType>
    element(address addr, const DataType& data, std::source_location location = std::source_location::current())
      : element(region{addr, sizeof(DataType)}, Operator::one(data, location), location) {}
    /// data element at given pointer
    template<trivial_data DataType>
    element(DataType* addr, const DataType& data, std::source_location location = std::source_location::current())
      : element(region{addr, addr + 1}, Operator::one(data, location), location) {}
    template<trivial_data DataType>
    element(volatile DataType* addr, const DataType& data, std::source_location location = std::source_location::current())
      : element(region{addr, addr + 1}, Operator::one(data, location), location) {}
    /// data element at given span
    template<trivial_data DataType, std::size_t N>
    element(std::span<DataType,N> sp, const DataType& data, std::source_location location = std::source_location::current())
      : element{region{sp.data(), sp.size_bytes() }, Operator::all(data, location), location} {}
    /// data element at given volatile span
    template<trivial_data DataType, std::size_t N>
    element(std::span<volatile DataType,N> sp, const DataType& data, std::source_location location = std::source_location::current())
      : element{region{sp.data(), sp.size_bytes() }, Operator::all(data, location), location} {}

    element(const element&) = default;
    element(element&&) = default;
    ~element() = default;
    element& operator=(const element&) = default;
    element& operator=(element&&) = default;
    auto operator()() const {
        return operator_(region_.begin(), region_.end());
    }
    constexpr auto addr() const noexcept { return region_.addr(); }
    constexpr auto size() const noexcept { return region_.size(); }
    constexpr auto& location() const noexcept { return location_; }
    template<typename T = void>
    auto begin() const noexcept { return region_.begin(); }
    template<typename T = void>
    auto end() const noexcept { return region_.end(); }
    friend constexpr auto overlapping(const element& a, const element& b) noexcept {
        return overlapping(a.region_, b.region_);
    }
private:
    region region_;
    operator_type operator_; // std::function here makes element non-constexpr
    std::source_location location_;
};

/// stub - allocates and initializes regions of MMIO memory
class stub {
public:
    using operator_type =  generator;
    using element_type = element<generator>;
    using elements_type = std::map<region::address_type, element_type>;
    using initializer_list = std::initializer_list<element_type>;
    enum class identity_type : std::uint64_t {};
    explicit stub(std::source_location location = std::source_location::current()) : location_ { location } {}
    /// constructs stub from a list of elements
    stub(initializer_list elements, std::source_location location = std::source_location::current());
    /// constructs stub from multiple lists of elements
    stub(std::initializer_list<initializer_list> lists, std::source_location location = std::source_location::current());
    stub(const stub&) = default;
    stub(stub&&);
    stub& operator=(const stub&) = default;
    stub& operator=(stub&&);
    ~stub();
    auto identity() const noexcept {
        return static_cast<identity_type>(reinterpret_cast<std::uintptr_t>(this));
    }
    /// applies elements to MMIO arena
    void operator()() const {
        apply();
    }
    /// appends elements copied from another stub
    stub& operator|=(const stub& that);
    /// appends elements taken from another stub
    stub& operator|=(stub&&);
    /// returns source location of this stub
    auto& location() const noexcept { return location_; }
    auto element_count() const noexcept { return elements_.size(); }
private:
    void apply() const;
    elements_type elements_ {};
    std::source_location location_;
};

/// combines two stubs
inline stub operator|(const stub& lhs, const stub& rhs) {
    stub result {lhs};
    result |= rhs;
    return result;
}

/// verify - verifies data in regions of MMIO memory
class verify {
public:
    using operator_type =  comparator;
    using element_type = element<comparator>;
    using elements_type = std::map<region::address_type, element_type>;
    using initializer_list = std::initializer_list<element_type>;
    enum class control { stop, run };
    /// construct empty verify
    explicit verify(std::source_location location = std::source_location::current()) : location_ { location } {}
    /// construct verify from list of elements
    verify(initializer_list elements, std::source_location location = std::source_location::current());
    /// construct verify from list of lists
    verify(std::initializer_list<initializer_list> lists, std::source_location location = std::source_location::current());
    verify(const verify&) = default;
    verify(verify&&) = default;
    verify& operator=(const verify&) = default;
    verify& operator=(verify&&) = default;
    ~verify() = default;
    /// runs MMIO data verification
    bool operator()() const {
        return apply();
    }
    /// appends elements copied from another verify
    verify& operator|=(const verify& that);
    /// appends elements taken from another verify
    verify& operator|=(verify&&);
    /// return source location of this object
    auto& location() const noexcept { return location_; }
    auto element_count() const noexcept { return elements_.size(); }
    using expect_signature = control(*)(bool, std::source_location);
    static control default_expect(bool, std::source_location);
    static constinit expect_signature expect;
private:
    bool apply() const;
    elements_type elements_ {};
    std::source_location location_;
};

/// combines two verify
inline verify operator|(const verify& lhs, const verify& rhs) {
    verify result {lhs};
    result |= rhs;
    return result;
}

/// sets fill value for newly allocated pages
void set_page_fill(std::uint64_t value) noexcept;
/// resets fill value
void set_page_nofill() noexcept;

namespace util {
void handle_sigsegv();
}

} // namespace stubmmio
