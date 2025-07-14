/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * src/mmio.h - Memory Mapped IO implementation
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#pragma once

#include <stubmmio/stubmmio.h>
#include <stubmmio/logger.h>
#include "pagerange.h"
#include <sys/mman.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <format>
#include <optional>
#include <span>
#include <source_location>
#include <vector>

namespace stubmmio::detail {
class mmio {
public:
    ~mmio();
    mmio(const mmio&) = delete;
    mmio(mmio&&) = delete;
    mmio& operator=(const mmio&) = delete;
    mmio& operator=(mmio&&) = delete;
    static mmio& arena();
    void allocate(pagerange, const stub& owner);
    void deallocate(const stub& owner);
    void claim(const stub& looser, const stub& claimer);
    std::size_t allocation_size() const;
    bool contains(pagerange) const;
    bool contains(volatile_span) const;
    void set_fill(std::uint64_t value) noexcept {
        fill_ = value;
    }
    void set_nofill() noexcept {
        fill_.reset();
    }
    struct listener {
        virtual ~listener() {}
        virtual void unmapping(volatile_span, std::source_location) = 0;
    };
    void subscribe(listener*);
    void unsubscribe(listener*);
private:
    void validate(pagerange, const stub& owner) const;
    struct allocation {
        pagerange range;
        stub::identity_type owner;
        std::source_location location;
    };
    void map(pagerange);
    void notify(pagerange, std::source_location);
    mmio() = default;
    std::map<pageid_type, allocation> allocations_{};
    std::vector<listener*> listeners_ {};
    std::optional<std::uint64_t> fill_ {};
};

using log = logovod::logger<logcategory::arena>;

inline mmio& mmio::arena() {
    static mmio instance{};
    return instance;
}

inline void unmap_range(pagerange pr) {
    munmap(pr.pointer(), pr.size_bytes());
}

inline mmio::~mmio() {
    for(const auto& i : allocations_) {
        notify(i.second.range, i.second.location);
    }
    for(const auto& i : allocations_) {
        unmap_range(i.second.range);
    }
}

inline void mmio::subscribe(listener* l) {
    listeners_.push_back(l);
}

inline void mmio::unsubscribe(listener* l) {
    std::erase(listeners_, l);
}

inline void mmio::notify(pagerange pages, std::source_location location) {
    const volatile_span addresses { static_cast<volatile_span::element_type*>(pages.pointer()), pages.size_bytes() };
    for(auto l : listeners_) l->unmapping(addresses, location);
}

inline void report_conflicting_allocation(pagerange requested, pagerange previous, std::source_location location) {
    throw exceptions::conflicting_allocation{
        std::format("Requested allocation {}[{}] conflicts with previous {}[{}] of the same owner @ {}:{}",
                requested.pointer(), requested.size_bytes(), previous.pointer(), previous.size_bytes(),
                location.file_name(), location.line())};
}

inline void report_conflicting_allocation(pagerange requested, pagerange previous, std::source_location requestor, std::source_location owner) {
    throw exceptions::conflicting_allocation{
        std::format("Page range {}[{}] requested by stub @ {}:{} conflicts with previous {}[{}] by another stub @ {}:{}",
                requested.pointer(), requested.size_bytes(), requestor.file_name(), requestor.line(),
                previous.pointer(), previous.size_bytes(), owner.file_name(), owner.line())};
}

inline void mmio::validate(pagerange requested, const stub& owner) const {
    if(allocations_.contains(requested.begin())) {
        const auto& previous = allocations_.at(requested.begin());
        if (previous.owner == owner.identity()) {
            if(previous.range == requested)
                return; // exact page already allocated
            report_conflicting_allocation(requested, previous.range, owner.location());
        } else {
            report_conflicting_allocation(requested, previous.range, owner.location(), previous.location);
        }
    }
    auto found = std::find_if(allocations_.begin(), allocations_.end(), [requested](const auto& item) {
        return requested.overlapping(item.second.range);
    });
    if (found != allocations_.end() && found->second.owner != owner.identity()) {
        report_conflicting_allocation(requested, found->second.range, owner.location(), found->second.location);
    }
}

inline std::span<std::uint64_t> map_range(pagerange pr) {
    static constexpr int prot = PROT_READ | PROT_WRITE;
    static constexpr int flags =  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED;
    auto ptr = mmap(pr.pointer(), pr.size_bytes(), prot, flags, -1, 0);
    if (ptr == MAP_FAILED) {
        auto err = errno;
        auto msg = std::format("mmap({}, {}) has failed: {} - {}", pr.pointer(), pr.size_bytes(), err, strerror(err));
        log::critical{}(msg);
        throw std::system_error{{err, std::system_category()}, msg};
    }
    return { static_cast<std::uint64_t*>(ptr), pr.size_bytes() / sizeof(std::uint64_t) };
}

inline void mmio::allocate(pagerange requested, const stub& owner) {
    validate(requested, owner);
    auto page = map_range(requested);
    allocations_.emplace(requested.begin(), allocation{requested, owner.identity(), owner.location()});
    if (fill_.has_value()) {
        std::fill(page.begin(), page.end(), *fill_);
    }
}

inline void mmio::deallocate(const stub& owner) {
    const auto identity = owner.identity();
    std::erase_if(allocations_, [this, identity](const auto& i) -> bool {
       if(i.second.owner == identity) {
           notify(i.second.range, i.second.location);
           unmap_range(i.second.range);
           return true;
       } else {
           return false;
       }
    });
}

inline void mmio::claim(const stub& looser, const stub& claimer) {
    const auto looserid = looser.identity();
    const auto claimerid = claimer.identity();
    for(auto& i : allocations_) {
        if(i.second.owner == looserid) {
            i.second.owner = claimerid;
        }
    }
}

inline std::size_t mmio::allocation_size() const {
    std::size_t result {};
    for(const auto& i : allocations_) {
        result += i.second.range.size();
    }
    result *= page_size;
    return result;
}

inline bool mmio::contains(pagerange requested) const {
    if(allocations_.contains(requested.begin())) {
        const auto& found = allocations_.at(requested.begin());
        return found.range.contains(requested);
    } else {
        auto found = std::find_if(allocations_.begin(), allocations_.end(), [requested](const auto& item) {
            return requested.overlapping(item.second.range);
        });
        if (found == allocations_.end())
            return false;
        return found->second.range.contains(requested);
    }
}

inline bool mmio::contains(volatile_span requested) const {
    return contains(detail::pagerange{requested});
}


} // namespace stubmmio::detail
