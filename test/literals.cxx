/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/literals.cxx - tests for stubmmio::literals
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <stubmmio/literals.h>

using namespace stubmmio::literals;

static_assert(0_U8 == 0U);
static_assert(7_U8 == 7U);
static_assert(0x7_U8 == 0x7U);
static_assert(0xE1_U8 == 0xE1U);
static_assert(0xFF_U8 == 0xFFU);
static_assert(07_U8 == 07);
static_assert(0b10101_U8 == 0b10101);
static_assert(0b11111111_U8 == 0b11111111);

static_assert(0_U16 == 0U);
static_assert(7_U16 == 7U);
static_assert(0x77_U16 == 0x77);
static_assert(0xFFFF_U16 == 0xFFFFU);
static_assert(0xabcd_U16 == 0xABCDU);
static_assert(077777_U16 == 077777U);
static_assert(0b10101_U16 == 0b10101);
static_assert(0b1111111111111111_U16 == 0b1111111111111111U);

static_assert(0_U32 == 0U);
static_assert(7_U32 == 7U);
static_assert(0x77_U32 == 0x77);
static_assert(0xFFFFFFFF_U32 == 0xFFFFFFFFU);
static_assert(0x89abcdef_U32 == 0x89ABCDEFU);
static_assert(07777777777_U32 == 07777777777U);
static_assert(0b10101_U32 == 0b10101);
static_assert(0b1111111111111111_U32 == 0b1111111111111111U);
static_assert(0b11111111111111111111111111111111_U32 == 0b11111111111111111111111111111111U);
