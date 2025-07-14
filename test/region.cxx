/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/region.cxx - unit tests for region
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <stubmmio/stubmmio.h>
#include <stubmmio/unit.h>
#include <array>

namespace {
using namespace stubmmio;

constexpr region region_from_addr_size { 0x1000, 16 };

std::uint32_t variable_uint32;
constexpr region region_from_pointer_and_size { static_cast<void*>(&variable_uint32), sizeof(variable_uint32) };

std::byte bytes[100];

const region region_from_range { &bytes[0], &bytes[64] };

using namespace boost::ut;
using namespace boost::ut::bdd;

suite<"region"> region_suite = [] {
    "region_from_addr_size has expected pointer"_test = [] {
        static_assert(region_from_addr_size.addr() == 0x1000);
        static_assert(region_from_addr_size.size() == 16);
        expect(region_from_addr_size.begin() == reinterpret_cast<void*>(0x1000));
    };
    "region_from_pointer_and_size has expected address"_test = [] {
        expect(region_from_pointer_and_size.addr() == reinterpret_cast<region::address_type>(& variable_uint32));
    };
    "region_from_range has expected address"_test = [] {
        expect(eq(region_from_range.size(), 64U));
        expect(region_from_range.addr() == reinterpret_cast<region::address_type>(&bytes[0]));
    };
    "region with reversed addresses throws"_test = [] {
        expect(throws([]{ region reversed { &bytes[8], &bytes[0] }; }));
    };
    "copy_from_addr_size has expected address, pointer and size"_test = [] {
        static constinit region copy_from_addr_size {region_from_addr_size};
        expect(eq(copy_from_addr_size.addr(), 0x1000U));
        expect(eq(copy_from_addr_size.size(), 16U));
        expect(copy_from_addr_size.begin() == reinterpret_cast<void*>(0x1000));
    };
    "equal regions are equal"_test = [] {
        static constexpr region another_region_from_addr_size { 0x1000, 16 };
        region another_region_from_pointer_and_size { static_cast<void*>(&variable_uint32), sizeof(variable_uint32) };
        static_assert(another_region_from_addr_size == region_from_addr_size);
        expect(another_region_from_pointer_and_size == region_from_pointer_and_size);

    };
    "regions with different address are not equal"_test = [] {
        static constexpr region another_region_from_addr_size { 0x1010, 16 };
        region another_region_from_pointer_and_size { static_cast<void*>(&variable_uint32), sizeof(variable_uint32) };

        static_assert(another_region_from_addr_size != region_from_addr_size);
        expect(another_region_from_pointer_and_size != region_from_addr_size);
    };
    "regions with different size are not equal"_test = [] {
        static constexpr region another_region_from_addr_size { 0x1000, 8 };
        region another_region_from_pointer_and_size { static_cast<void*>(&variable_uint32), 2*sizeof(variable_uint32) };

        static_assert(another_region_from_addr_size != region_from_addr_size);
        expect(another_region_from_pointer_and_size != region_from_pointer_and_size);
    };
    "regions with same address and smaller size are lesser"_test = [] {
        static constexpr region another_region_from_addr_size { 0x1000, 8 };
        region another_region_from_pointer_and_size { static_cast<void*>(&variable_uint32), 2*sizeof(variable_uint32) };

        static_assert(another_region_from_addr_size < region_from_addr_size);
        expect(region_from_pointer_and_size < another_region_from_pointer_and_size);
    };
    "regions with same address and larger size are greater"_test = [] {
        static constexpr region another_region_from_addr_size { 0x1000, 8 };
        region another_region_from_pointer_and_size { static_cast<void*>(&variable_uint32), 2*sizeof(variable_uint32) };

        static_assert(region_from_addr_size > another_region_from_addr_size);
        expect(another_region_from_pointer_and_size > region_from_pointer_and_size);
    };
    "regions with lesser address are lesser"_test = [] {
        static constexpr region another_region_from_addr_size { 0x1008, 16 };
        [[maybe_unused]]
        region another_region_from_pointer_and_size { static_cast<void*>(&variable_uint32), 2*sizeof(variable_uint32) };
        region another_region_from_range { &bytes[4], &bytes[64] };

        static_assert(region_from_addr_size < another_region_from_addr_size);
        expect(region_from_range < another_region_from_range);
    };
    "regions with greater address are greater"_test = [] {
        static constexpr region another_region_from_addr_size { 0x1008, 16 };
        [[maybe_unused]]
        region another_region_from_pointer_and_size { static_cast<void*>(&variable_uint32), 2*sizeof(variable_uint32) };
        region another_region_from_range { &bytes[4], &bytes[64] };

        static_assert(another_region_from_addr_size > region_from_addr_size);
        expect(another_region_from_range > region_from_range);
    };
    "overlapping return true for overlapping regions"_test = [] {
        const region fully_internal { &bytes[8], &bytes[14] };
        const region overlapping_region_from_addr_size { 0x1004, 4 };
        region partially_overlapping { &bytes[12], &bytes[68] };

        expect(overlapping(overlapping_region_from_addr_size, region_from_addr_size));
        expect(overlapping(region_from_addr_size, overlapping_region_from_addr_size));
        expect(overlapping(fully_internal, region_from_range));
        expect(overlapping(region_from_range, fully_internal));
        expect(overlapping(partially_overlapping, region_from_range));
        expect(overlapping(partially_overlapping, fully_internal));
        expect(overlapping(fully_internal, partially_overlapping));
    };
    "overlapping return false for adjacent regions"_test = [] {
        const region adjacent_region { &bytes[64], &bytes[66] };
        static constexpr region adjacent_region_from_addr_size { 0x1010, 4 };
        static_assert(!overlapping(adjacent_region_from_addr_size, region_from_addr_size));
        static_assert(!overlapping(region_from_addr_size, adjacent_region_from_addr_size));
        expect(!overlapping(adjacent_region, region_from_range));
        expect(!overlapping(region_from_range, adjacent_region));
    };
    "overlapping return false for disjoint regions"_test = [] {
        const region another_region { &bytes[16], &bytes[32] };
        const region disjoint_region { &bytes[64], &bytes[80] };
        static constexpr region disjoint_region_from_addr_size { 0x1020, 16 };
        static_assert(!overlapping(disjoint_region_from_addr_size, region_from_addr_size));
        static_assert(!overlapping(region_from_addr_size, disjoint_region_from_addr_size));
        expect(!overlapping(disjoint_region, another_region));
        expect(!overlapping(another_region, disjoint_region));
    };
};

}
