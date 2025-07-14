/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/verify.cxx - unit tests for verify
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */


#include <stubmmio/stubmmio.h>
#include <stubmmio/unit.h>

using namespace stubmmio;
using namespace stubmmio::detail;

namespace {
using namespace boost::ut;
using namespace boost::ut::bdd;
using namespace boost::ut::literals;

constexpr test::native_type fill = 0xA5A5A5A5U;
constexpr uint64_t qword = 0x5A5A5A5A5A5A5A5AUL;
verify::initializer_list shared = { {{0x1000, 8}, comparator::one(fill)}, {{0x1008, 8}, comparator::one(fill)}};

suite<"verify"> verify_suite = [] {
    "constructor with initializer list"_test = [] {
         expect(nothrow([](){
             verify {
                 {{0x1000, 8}, comparator::one(fill)},
                 {{0x1010, 16}, comparator::all(fill)},
             };
        }));
    };
    "constructor with two initializer lists"_test = [] {
         expect(nothrow([](){
             verify {
                 {{{0x1000,  8}, comparator::one(fill)}},
                 {{{0x1010, 16}, comparator::all(fill)}}
             };
         }));
    };
    "constructor with a local and a shared initializer lists"_test = [] {
        expect(nothrow([](){
             verify {shared, {{address(0x1010), fill}, {address(0x1020), fill}}};
        }));
    };
    "constructor fails with duplicated"_test = [] {
        expect(throws([](){
             verify {{address(0x1004), fill}, {address(0x1004), fill}};
        }));
    };
    "constructor ignores overlapping"_test = [] {
        expect(nothrow([](){
             verify {shared, {{address(0x1010), qword}, {address(0x1012), qword}}};
        }));
    };
    "constructor with local memory and comparator"_test = [] {
        test::native_type variable = 0;
        verify sut{ {&variable, comparator::one(0)} };
        expect(sut());
    };
    "copy constructor copies location and elements"_test = [] {
        const verify src {{{address(0x1004), 16}}};
        const verify sut { src };
        expect(eq(src.element_count(), 1U));
        expect(eq(sut.location().file_name(), src.location().file_name()));
        expect(eq(sut.location().line(), src.location().line()));
    };
    "move constructor moves elements and location"_test = [] {
        verify src {{address(0x1004), fill}};
        verify sut { std::move(src) };
        expect(eq(src.element_count(), 0U));
        expect(eq(sut.element_count(), 1U));
        expect(eq(sut.location().file_name(), src.location().file_name()));
        expect(eq(sut.location().line(), src.location().line()));
    };
    "user-provided expect called once when returns stop"_test = [] {
        static unsigned count {};
        count = false;
        verify::expect = [](bool, std::source_location) -> verify::control {
            ++count;
            return verify::control::stop;
        };
        test::native_type v1 = 0, v2 = 0;
        verify sut{ {&v1, fill}, {&v2, fill} };
        expect(!sut());
        expect(eq(count, 1U));
        verify::expect = verify::default_expect;
    };
};
}
