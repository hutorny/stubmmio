/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/include/stubmmio/unit.h - common unit tests definitions
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <boost/ut.hpp>
#pragma GCC diagnostic pop

namespace stubmmio::test {
inline auto operator""_p(unsigned long long int v) {
    return reinterpret_cast<const void *>(v);
}
using native_type = std::uint32_t;
} // namespace stubmmio::test
