/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * types.h - common type definitions
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#pragma once
#include <cstdint>
#include <span>

namespace stubmmio {

enum class address : std::uintptr_t {};

namespace detail {

using volatile_span = std::span<const volatile char>;

} // namespace detail
} // namespace stubmmio
