/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/stub.cxx - unit tests for stub
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <stubmmio/stubmmio.h>
#include <stubmmio/unit.h>
#include <mmio.h>
#include <cstring>

using namespace stubmmio;
using namespace stubmmio::detail;

namespace {
struct trivial {
    std::uint32_t a;
    std::uint32_t b;
    std::uint8_t  c;
};

using namespace boost::ut;
using namespace boost::ut::bdd;
using namespace boost::ut::literals;

constexpr test::native_type fill = 0xA5A5A5A5U;
const stub::initializer_list shared = { {{0x1000, 8}, generator::one(fill)}, {{0x1008, 8}, generator::one(fill)}};

suite<"stub"> stub_suite = [] {
    "constructor with initializer list"_test = [] {
         expect(nothrow([](){
             stub {
                 {{0x1000, 8}, generator::one(fill)},
                 {{0x1010, 16}, generator::all(fill)},
                 {{0x1020, 32}}
             };
        }));
    };
    "constructor with two initializer lists"_test = [] {
         expect(nothrow([](){
             stub {{
                 {{0x1000, 8}, generator::one(fill)},
             },{
                 {{0x1010, 16}, generator::all(fill)},
                 {{0x1020, 32}}}
             };
         }));
    };
    "constructor with a local and a shared initializer lists"_test = [] {
        expect(nothrow([](){
             const stub::initializer_list local {{{0x1010, 16}},{{0x1020, 32}}};
             stub {shared, local};
        }));
    };
    "constructor fails with duplicated"_test = [] {
        expect(throws([](){
             stub {{{0x1004, 32}}, {{0x1004, 16}}};
        }));
    };
    "constructor with shared fails with duplicated"_test = [] {
        expect(throws([](){
             stub {shared, {{{0x1000, 32}}}};
        }));
    };
    "constructor fails with overlapping"_test = [] {
        expect(throws([](){
             stub { {{0x1000, 16}}, {{0x1004, 32}}};
        }));
    };
    "constructor with shared fails with overlapping"_test = [] {
        expect(throws([](){
             stub {shared, {{{0x1004, 16}}}};
        }));
    };
    "copy constructor copies location and elements"_test = [] {
         const stub src {{{0x1004, 16}}};
         const stub sut { src };
         expect(eq(src.element_count(), 1U));
         expect(eq(sut.location().file_name(), src.location().file_name()));
         expect(eq(sut.location().line(), src.location().line()));
    };
    "move constructor moves elements and location"_test = [] {
         stub src {{{0x1004, 16}}};
         stub sut { std::move(src) };
         expect(eq(src.element_count(), 0U));
         expect(eq(sut.element_count(), 1U));
         expect(eq(sut.location().file_name(), src.location().file_name()));
         expect(eq(sut.location().line(), src.location().line()));
    };
    "move constructor moves allocated pages"_test = [] {
         stub src {{{0x10004, 16}}};
         expect(nothrow([&src](){ src(); }));
         stub sut { std::move(src) };
         expect(nothrow([&sut](){ sut(); }));
         expect(eq(mmio::arena().allocation_size(), page_size));
    };
    "apply uninitialized element"_test = [] {
        stub sut {{{0x20000, 4}}};
        expect(nothrow([&sut](){ sut(); }));
    };
    "apply initialized element"_test = [] {
        stub sut {{address(0x20000), 4U}};
        expect(nothrow([&sut](){ sut(); }));
        expect(eq(mmio::arena().allocation_size(), page_size));
    };
    "unary concatenation not throws"_test = [] {
        expect(nothrow([]{
            stub sut {{{0x20000, 4}}};
            sut |= {{{0x20008, 4}}};
        }));
    };
    "unary concatenation throws on overlapping"_test = [] {
        expect(throws([]{
            stub sut {{{0x20000, 8}}};
            sut |= {{{0x20004, 4}}};
        }));
    };
    "binary concatenation throws on overlapping"_test = [] {
        expect(throws([]{
            const stub src {{{0x1000, 8}}};
            auto sut = shared | src;
        }));
    };
    "binary concatenation copies location of first element"_test = [] {
        const stub src {{{0x20000, 4}}};
        const stub src2 {{{0x20004, 4}}};
        stub sut = src | src2;
        expect(eq(sut.location().file_name(), src.location().file_name()));
        expect(eq(sut.location().line(), src.location().line()));
    };
    "memory outside of test area not causing allocations "_test = [] {
        test::native_type variable = 0;
        stub sut {{&variable, fill}};
        expect(nothrow([&sut]{ sut(); }));
        expect(eq(variable, fill));
        expect(eq(mmio::arena().allocation_size(), 0U));
    };
    "move assignment moves elements and location"_test = [] {
         stub src {{{0x1004, 16}}};
         stub sut {};
         sut = std::move(src);
         expect(eq(src.element_count(), 0U));
         expect(eq(sut.element_count(), 1U));
         expect(eq(sut.location().file_name(), src.location().file_name()));
         expect(eq(sut.location().line(), src.location().line()));
    };
    "move assignment moves allocated pages"_test = [] {
         stub src {{{0x10004, 16}}};
         expect(nothrow([&src](){ src(); }));
         stub sut { };
         sut = std::move(src);
         expect(nothrow([&sut](){ sut(); }));
         expect(eq(mmio::arena().allocation_size(), page_size));
    };
    "move concatenation moves elements and preserves location"_test = [] {
         stub src {{{0x1004, 16}}};
         stub sut {};
         sut |= std::move(src);
         expect(eq(src.element_count(), 0U));
         expect(eq(sut.element_count(), 1U));
         expect(neq(sut.location().line(), src.location().line()));
    };
    "move concatenation moves allocated pages"_test = [] {
         stub src {{{0x10004, 16}}};
         expect(nothrow([&src](){ src(); }));
         stub sut { };
         sut |= std::move(src);
         expect(nothrow([&sut](){ sut(); }));
         expect(eq(mmio::arena().allocation_size(), page_size));
    };
};

}


