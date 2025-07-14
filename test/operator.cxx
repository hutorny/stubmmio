/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/operator.cxx - unit tests for operators
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */


#include <stubmmio/stubmmio.h>
#include <stubmmio/unit.h>
#include <cstring>

namespace {
struct trivial {
    std::uint32_t a;
    std::uint32_t b;
    std::uint32_t  c;
};
static_assert(std::has_unique_object_representations_v<trivial>);
struct padded {
    std::uint32_t a;
    std::uint32_t  c;
    std::uint8_t b;
};

using namespace stubmmio;
using namespace boost::ut;
using namespace boost::ut::bdd;

suite<"operator"> operator_suite = [] {
    "generator::one writes single uint8_t value"_test = [] {
        static constexpr std::uint8_t i = 15;
        static constexpr std::uint8_t v = 240;
        static constexpr uint8_t expected[8] { i, i, i, v, i, i, i, i};
        uint8_t arena[8] { i, i, i, i, i, i, i, i };
        generator::one(v, {})(&arena[3], &arena[8]);
        static_assert(std::end(expected) - std::begin(expected) == 8);
        expect(std::equal(std::begin(expected), std::end(expected), arena));
    };
    "generator::one writes single uint16_t value"_test = [] {
        static constexpr std::uint16_t i = 0x0F0F;
        static constexpr std::uint16_t v = 0xF0F0;
        static constexpr uint16_t expected[4] { i, v, i, i};
        uint16_t arena[4] { i, i, i, i };
        generator::one(v, {})(&arena[1], &arena[4]);
        static_assert(std::end(expected) - std::begin(expected) == 4);
        expect(std::equal(std::begin(expected), std::end(expected), arena));
    };
    "generator::one writes single uint32_t value"_test = [] {
        static constexpr std::uint32_t i = 0x0F0F0F0F;
        static constexpr std::uint32_t v = 0xF0F0F0F0;
        static constexpr uint32_t expected[4] { i, i, v, i};
        uint32_t arena[4] { i, i, i, i };
        generator::one(v, {})(&arena[2], &arena[3]);
        static_assert(std::end(expected) - std::begin(expected) == 4);
        expect(std::equal(std::begin(expected), std::end(expected), arena));
    };
    "generator::one writes single struct"_test = [] {
        static constexpr trivial i { 0xAAAAAAAA, 0xBBBB, 0xCC };
        static constexpr trivial v { 0xCCCCCCCC, 0xAAAA, 0xBB };
        static constexpr trivial expected[3] { i, v, i};
        trivial arena[3] { i, i, i };
        generator::one(v, {})(&arena[1], &arena[3]);
        static_assert(std::end(expected) - std::begin(expected) == 3);
        expect(std::memcmp(expected, arena, sizeof(arena)) == 0);
    };

    "generator::all writes multiple uint8_t values"_test = [] {
        static constexpr std::uint8_t i = 15;
        static constexpr std::uint8_t v = 240;
        static constexpr uint8_t expected[8] { i, i, i, v, v, v, v, i};
        uint8_t arena[8] { i, i, i, i, i, i, i, i };
        generator::all(v, {})(&arena[3], &arena[7]);
        static_assert(std::end(expected) - std::begin(expected) == 8);
        expect(std::equal(std::begin(expected), std::end(expected), arena));
    };
    "generator::all writes multiple uint16_t values"_test = [] {
        static constexpr std::uint16_t i = 0x0F0F;
        static constexpr std::uint16_t v = 0xF0F0;
        static constexpr uint16_t expected[4] { i, v, v, i};
        uint16_t arena[4] { i, i, i, i };
        generator::all(v, {})(&arena[1], &arena[3]);
        static_assert(std::end(expected) - std::begin(expected) == 4);
        expect(std::equal(std::begin(expected), std::end(expected), arena));
    };
    "generator::all writes multiple uint32_t values"_test = [] {
        static constexpr std::uint32_t i = 0x0F0F0F0F;
        static constexpr std::uint32_t v = 0xF0F0F0F0;
        static constexpr uint32_t expected[4] { i, v, v, i};
        uint32_t arena[4] { i, i, i, i };
        generator::all(v, {})(&arena[1], &arena[3]);
        static_assert(std::end(expected) - std::begin(expected) == 4);
        expect(std::equal(std::begin(expected), std::end(expected), arena));
    };
    "generator::all writes multiple structs"_test = [] {
        static constexpr trivial i { 0xAAAAAAAA, 0xBBBB, 0xCC };
        static constexpr trivial v { 0xCCCCCCCC, 0xAAAA, 0xBB };
        static constexpr trivial expected[4] { i, v, v, i };
        trivial arena[4] { i, i, i, i };
        generator::all(v, {})(&arena[1], &arena[3]);
        static_assert(std::end(expected) - std::begin(expected) == 4);
        expect(std::memcmp(expected, arena, sizeof(arena)) == 0);
    };

    "comparator::one compares single uint8_t value"_test = [] {
        static constexpr std::uint8_t i = 15;
        static constexpr std::uint8_t v = 240;
        uint8_t arena[8] { i, i, i, v, i, i, i, i };
        expect(comparator::one(v, {})(&arena[3], &arena[8]));
    };
    "generator::one compares single uint16_t value"_test = [] {
        static constexpr std::uint16_t i = 0x0F0F;
        static constexpr std::uint16_t v = 0xF0F0;
        static constexpr uint16_t expected[4] { i, v, i, i};
        uint16_t arena[4] { i, v, i, i };
        expect(comparator::one(v, {})(&arena[1], &arena[4]));
        static_assert(std::end(expected) - std::begin(expected) == 4);
    };
    "comparator::one compares single uint32_t value"_test = [] {
        static constexpr std::uint32_t i = 0x0F0F0F0F;
        static constexpr std::uint32_t v = 0xF0F0F0F0;
        uint32_t arena[4] { i, i, v, i };
        expect(comparator::one(v, {})(&arena[2], &arena[3]));
    };
    "comparator::one compares single struct"_test = [] {
        static constexpr trivial i { 0xAAAAAAAA, 0xBBBB, 0xCC };
        static constexpr trivial v { 0xCCCCCCCC, 0xAAAA, 0xBB };
        trivial arena[3] { i, v, i };
        expect(comparator::one(v, {})(&arena[1], &arena[3]));
        expect(comparator::one(i, {})(&arena[0], &arena[2]));
    };
    "comparator::all compares multiple uint8_t values"_test = [] {
        static constexpr std::uint8_t i = 15;
        static constexpr std::uint8_t v = 240;
        uint8_t arena[8] { i, i, i, v, v, v, v, i };
        expect(comparator::all(v, {})(&arena[3], &arena[7]));
    };
    "comparator::all compares multiple uint16_t values"_test = [] {
        static constexpr std::uint16_t i = 0x0F0F;
        static constexpr std::uint16_t v = 0xF0F0;
        uint16_t arena[4] { i, v, v, i };
        expect(comparator::all(v, {})(&arena[1], &arena[3]));
    };
    "comparator::all compares multiple uint32_t values"_test = [] {
        static constexpr std::uint32_t i = 0x0F0F0F0F;
        static constexpr std::uint32_t v = 0xF0F0F0F0;
        uint32_t arena[4] { i, v, v, i };
        expect(comparator::all(v, {})(&arena[1], &arena[3]));
    };
    "comparator::all detects differences in uint32_t values"_test = [] {
        static constexpr std::uint32_t i = 0x0F0F0F0F;
        static constexpr std::uint32_t v = 0xF0F0F0F0;
        uint32_t arena[4] { i, v, i, i };
        expect(!comparator::all(v, {})(&arena[1], &arena[3]));
    };
    "comparator::all compares multiple structs"_test = [] {
        static constexpr trivial i { 0xAAAAAAAA, 0xBBBB, 0xCC };
        static constexpr trivial v { 0xCCCCCCCC, 0xAAAA, 0xBB };
        trivial arena[4] { i, v, v, i };
        expect(comparator::all(v, {})(&arena[1], &arena[3]));
    };

};
}

