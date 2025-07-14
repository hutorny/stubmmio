/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/global.cxx - unit tests for stubmmio
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <stubmmio/stubmmio.h>
#include <stubmmio/unit.h>
#include <array>

using namespace stubmmio;
using namespace stubmmio::detail;

namespace {
using namespace boost::ut;
using namespace boost::ut::bdd;
using namespace boost::ut::literals;

suite<"global"> global_suite = [] {
    "set_page_fill has effect"_test = [] {
        static constexpr uint64_t fill = 0x5A69788796A5B4C3UL;
        set_page_fill(fill);
        stub sut {{{0x10000, 32}}};
        sut();
        expect(eq(*reinterpret_cast<const std::uint64_t*>(0x10010), fill));
    };
    "set_page_nofill has effect"_test = [] {
        static constexpr uint64_t fill = 0x2D5A69788796A5B4UL;
        set_page_fill(fill);
        set_page_nofill();
        stub sut {{{0x20000, 32}}};
        sut();
        expect(neq(*reinterpret_cast<const std::uint64_t*>(0x20010), fill));
    };
    "stub, change and verify"_test = [] {
        static constexpr std::array<uint32_t, 2> init = { 0x1E2D3C4B, 0x5A697887 };
        static constexpr std::array<uint32_t, 2> expected = { 0x2D3C4B, 0x5A697887 };
        stub sut {{ address(0x40000000), init }};
        sut();
        *reinterpret_cast<std::uint32_t*>(0x40000000) = expected[0];
        verify vut {{address(0x40000000), expected[0]}};
        expect(vut());
    };
    scenario("BDD given, when, then") = [] {
      verify::expect = [](bool c, std::source_location l) { expect(c, l); return verify::control::run; };
      given("initial zero") = [] {
          stub sut{{ address(0x40000000), test::native_type(0U) }};
          sut();
          when("set bit") = [] { *reinterpret_cast<std::uint32_t*>(0x40000000) |= 1; };
          then("value is 1") = verify {{address(0x40000000), 1U}};
      };
      verify::expect = verify::default_expect;
    };
};

} // namespace

