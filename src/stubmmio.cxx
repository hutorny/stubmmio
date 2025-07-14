/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * src/stubmmio.cxx - stubmmio implementation
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <stubmmio/stubmmio.h>
#include <stubmmio/logger.h>
#include <format>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include "mmio.h"

namespace stubmmio {
namespace  detail {
using namespace stubmmio::exceptions;

using file_name_type = decltype(std::source_location::current().file_name());
using line_numb_type = decltype(std::source_location::current().line());

void ensure_size_match(void_range, std::size_t, std::source_location) {
    // TODO
}
void ensure_size_multiplyof(void_range, std::size_t, std::source_location) {
    // TODO
}

void ensure_region_is_not_reversed(void_range r, std::source_location l) {
    if(static_cast<const volatile std::byte*>(r.end) - static_cast<const volatile std::byte*>(r.begin) < 0) {
        auto message = std::format("Reversed region [{}..{}] at {}::{}",
                const_cast<const void*>(r.begin), const_cast<const void*>(r.end), l.file_name(), l.line());
        throw region_reversed{std::move(message)};
    }
}

template<typename Element>
void report_duplicate(const Element& duplicate, const Element& original, std::source_location location) {
    static constexpr std::format_string<region::address_type, file_name_type, line_numb_type,
        file_name_type, line_numb_type, file_name_type, line_numb_type>
        message_template =
            "Duplicate address {:X} in the element declared at '{}:{}'\n"
            "    used in stub declared at '{}:{}'\n"
            "    original element declared at '{}:{}'\n";
    auto message = std::format(message_template, duplicate.addr(), duplicate.location().file_name(), duplicate.location().line(),
            location.file_name(), location.line(), original.location().file_name(), original.location().line());
    throw duplicate_address{std::move(message)};
}

template <typename T>
concept is_pair = requires(T p) {
    p.first;
    p.second;
};

template<typename ElementsType, typename Iterator>
static void append(ElementsType& dst, Iterator begin, Iterator end, std::source_location location) {
    for(auto i = begin; i!= end; ++i) {
        if constexpr(is_pair<decltype(*i)>) {
            if (auto success = dst.try_emplace(i->first, i->second); ! success.second) {
                report_duplicate(i->second, success.first->second, location);
            }
        } else {
            if(auto success = dst.try_emplace(i->addr(), *i); ! success.second) {
                report_duplicate(*i, success.first->second, location);
            }
        }
    }
}

static void check_overlapping(const stub::elements_type& elements, std::source_location location) {
    static constexpr std::format_string<file_name_type, line_numb_type,
        region::address_type, region::size_type, file_name_type, line_numb_type,
        region::address_type, region::size_type, file_name_type, line_numb_type>
        message_template =
           "Stub declared at {}:{} has overlappings:\n"
           "Element   0x{:X}[{}] declared at {}:{}\n"
           "Overlaps  0x{:X}[{}] declared at {}:{}\n";

    auto overlaps = std::adjacent_find(elements.begin(), elements.end(),
         [](const auto& lhs, const auto& rhs) {return overlapping(lhs.second, rhs.second);});
    if (overlaps != elements.end()) {
        auto overlape = overlaps;
        ++overlape;
        auto message = std::format(message_template, location.file_name(), location.line(),
           overlaps->second.addr(), overlaps->second.size(), overlaps->second.location().file_name(), overlaps->second.location().line(),
           overlape->second.addr(), overlape->second.size(), overlape->second.location().file_name(), overlape->second.location().line());
        throw exceptions::overlapping_elements(std::move(message));
    }
}

} // namespace stubmmio::detail


stub::stub(initializer_list elements, std::source_location location)
  : location_(location) {
    detail::append(elements_, elements.begin(), elements.end(), location);
    detail::check_overlapping(elements_, location_);
}
stub::stub(std::initializer_list<initializer_list> lists, std::source_location location)
 : location_(location) {
    for(const auto& elements :  lists) {
        detail::append(elements_, elements.begin(), elements.end(), location);
    }
    detail::check_overlapping(elements_, location_);
}


stub::stub(stub&& that)
 : elements_{std::move(that.elements_)}, location_{std::move(that.location_)} {
     detail::mmio::arena().claim(that, *this);
}

stub::~stub() {
    detail::mmio::arena().deallocate(*this);
}

stub& stub::operator|=(const stub& that) {
    detail::append(elements_, that.elements_.begin(), that.elements_.end(), location_);
    detail::check_overlapping(elements_, location_);
    return *this;
}

stub& stub::operator=(stub&& that) {
    elements_ = std::move(that.elements_);
    location_ = std::move(that.location_);
    detail::mmio::arena().claim(that, *this);
    return *this;
}

stub& stub::operator|=(stub&& that) {
    detail::append(elements_, std::make_move_iterator(that.elements_.begin()), std::make_move_iterator(that.elements_.end()), location_);
    detail::check_overlapping(elements_, location_);
    that.elements_.clear();
    detail::mmio::arena().claim(that, *this);
    return *this;
}

void stub::apply() const {
    std::vector<detail::pagerange> pages{};
    for(const auto& el : elements_) {
        if(el.first >= arena::size()) break;
        detail::pagerange page {el.second.begin(), el.second.end()};
        if(pages.empty() || ! pages.front().join(page) ) {
            pages.push_back(page);
        }
    }
    for(const auto& page : pages) {
        detail::mmio::arena().allocate(page, *this);
    }
    for(const auto& el : elements_) el.second();
}

verify::verify(initializer_list elements, std::source_location location)
: location_(location) {
  detail::append(elements_, elements.begin(), elements.end(), location);
}

verify::verify(std::initializer_list<initializer_list> lists, std::source_location location)
: location_(location) {
    for(const auto& elements :  lists) {
        detail::append(elements_, elements.begin(), elements.end(), location);
    }
}

verify& verify::operator|=(const verify& that) {
    detail::append(elements_, that.elements_.begin(), that.elements_.end(), location_);
    return *this;
}

verify& verify::operator|=(verify&& that) {
    detail::append(elements_, std::make_move_iterator(that.elements_.begin()), std::make_move_iterator(that.elements_.end()), location_);
    that.elements_.clear();
    return *this;
}


verify::control verify::default_expect(bool success, std::source_location loc) {
    using log = logovod::logger<logcategory::verify>;
    if (! success) {
        log::error{}.format("verify condition failed for element declared at {}:{}\n", loc.file_name(), loc.line());
    }
    return control::run;
}
constinit verify::expect_signature verify::expect = &verify::default_expect;

bool verify::apply() const {
    for(const auto& el : elements_) {
        if(el.first >= arena::size()) break;
        detail::pagerange page {el.second.begin(), el.second.end()};
        if(! detail::mmio::arena().contains(page)) {
            throw exceptions::page_is_not_allocated{std::format(
                "page is not allocated for element declared at {}:{}",
                el.second.location().file_name(), el.second.location().line())};
        }
    }
    bool fail = false;
    for(const auto& el : elements_) {
        const bool success = el.second();
        fail |= !success;
        if( expect(success, el.second.location()) == control::stop )
            break;
    }
    return !fail;
}

void set_page_fill(std::uint64_t value) noexcept {
    detail::mmio::arena().set_fill(value);
}

void set_page_nofill() noexcept {
    detail::mmio::arena().set_nofill();
}

} // namespace stubmmio
