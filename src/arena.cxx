/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * src/arena.cxx - arena methods
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <stubmmio/stubmmio.h>
#include <stubmmio/logger.h>
#include <format>
#include <iostream>
#include "pagerange.h"

extern "C" const uint64_t __executable_start;

namespace stubmmio {
using namespace stubmmio::exceptions;
namespace {
using log = logovod::logger<logcategory::arena>;

template<typename Exception>
bool failed(onfail on_fail, std::string&& message) {
    switch(on_fail) {
    case onfail::throws:
        throw Exception(std::move(message));
    case onfail::logs:
        log::critical{message};
        return false;
    case onfail::returns:
    default:
        return false;
    }
}
} // namespace

bool arena::check_pagesize(int actual, onfail on_fail) {
    using stubmmio::detail::page_size;
    if(static_cast<std::size_t>(actual) != page_size) {
        return failed<page_size_mismatch>(on_fail, std::format(
                "Actual page size {} is not equal the page size used in compile time {}", actual, page_size));

    }
    return true;
}

bool arena::check_boundary(std::uintptr_t arena_boundary, onfail on_fail) {
    if (reinterpret_cast<uintptr_t>(&__executable_start) < arena_boundary) {
        return failed<arena_is_not_fully_available>(on_fail, std::format(
              "Expected arena size {} is not available, only {} bytes are. Check PIE build options",
              arena_boundary, reinterpret_cast<uintptr_t>(&__executable_start)));
    }
    return true;
}

} // namespace stubmmio
