/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/pagerange.cxx - unit tests for page range
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */


#include <stubmmio/stubmmio.h>
#include <stubmmio/unit.h>
#include <pagerange.h>

namespace {
using namespace stubmmio::detail;
using namespace stubmmio::test;
using namespace boost::ut;
using namespace boost::ut::bdd;
constexpr auto operator""_n(unsigned long long int v) {
    return v / page_size;
}

suite<"pagerange"> pagerane_suite = [] {
    "primary constructor single page"_test = [] {
        pagerange sut(0x10000_p,0x11000_p);
        expect(eq(sut.begin(), 0x10000_n));
        expect(eq(sut.size(), 1U));
    };
    "primary constructor two pages"_test = [] {
        pagerange sut(0x10000_p,0x12000_p);
        expect(eq(sut.begin(), 0x10000_n));
        expect(eq(sut.size(), 2U));
    };
    "primary constructor two pages almost"_test = [] {
        pagerange sut(0x10000_p,0x11FFF_p);
        expect(eq(sut.begin(), 0x10000_n));
        expect(eq(sut.size(), 2U));
    };
    "overlapping returns true for overlapping page ranges"_test = [] {
        pagerange sut1(0x10000_p,0x14000_p);
        pagerange sut2(0x12000_p,0x13000_p);
        pagerange sut3(0x13000_p,0x15000_p);
        pagerange sut4(0x8000_p, 0x11100_p);
        expect(sut1.overlapping(sut2));
        expect(sut1.overlapping(sut3));
        expect(sut1.overlapping(sut4));
        expect(sut2.overlapping(sut1));
        expect(sut3.overlapping(sut1));
        expect(sut4.overlapping(sut1));
    };
    "overlapping returns true for adjacent ranges"_test = [] {
        pagerange sut1(0x10000_p,0x11000_p);
        pagerange sut2(0x11000_p,0x12000_p);
        expect(sut1.overlapping(sut2));
        expect(sut2.overlapping(sut1));
    };
    "overlapping returns false non-overlapping ranges"_test = [] {
        pagerange sut1(0x10000_p,0x11000_p);
        pagerange sut2(0x13000_p,0x14000_p);
        expect(!sut1.overlapping(sut2));
        expect(!sut2.overlapping(sut1));
    };
    "join merges adjacent ranges"_test = [] {
        pagerange sut1(0x10000_p,0x11000_p);
        const pagerange sut2(0x11000_p,0x12000_p);
        pagerange sut3(0x0F000_p,0x10000_p);
        expect(sut1.join(sut2));
        expect(eq(sut1.begin(), 0x10000_n));
        expect(eq(sut1.size(), 2U));
        expect(sut1.join(sut3));
        expect(eq(sut1.begin(), 0x0F000_n));
        expect(eq(sut1.size(), 3U));
    };
    "join absorbs inner ranges"_test = [] {
        pagerange sut1(0x10000_p,0x13000_p);
        const pagerange sut2(0x11000_p,0x12000_p);
        expect(sut1.join(sut2));
        expect(eq(sut1.begin(), 0x10000_n));
        expect(eq(sut1.size(), 3U));
    };
    "join ignores non-overlapping ranges"_test = [] {
        pagerange sut1(0x10000_p,0x11000_p);
        pagerange sut2(0x14000_p,0x15000_p);
        expect(!sut1.join(sut2));
        expect(eq(sut1.begin(), 0x10000_n));
        expect(eq(sut1.size(), 1U));
        expect(!sut2.join(sut1));
        expect(eq(sut2.begin(), 0x14000_n));
        expect(eq(sut2.size(), 1U));
    };

};
} // namespace
