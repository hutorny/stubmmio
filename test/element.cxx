/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/element.cxx - unit tests for element
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <stubmmio/stubmmio.h>
#include <stubmmio/unit.h>
#include <array>
#include <span>

namespace {
template<unsigned N>
constexpr auto generate_bytes() {
    std::array<std::byte, N> result;
    unsigned state {0};
    for(auto& i : result) i = static_cast<std::byte>(++state);
    return result;
}

using namespace stubmmio;
using namespace boost::ut;
using namespace boost::ut::bdd;

using stub_element = stubmmio::element<generator>;
using verify_element = stubmmio::element<comparator>;
using address_type = stubmmio::region::address_type;
struct trivial_struct {
    test::native_type a;
    test::native_type b;
    test::native_type c;
    test::native_type d;
};

constexpr bool operator==(const trivial_struct& l, const trivial_struct& r) noexcept {
    return l.a == r.a && l.b == r.b && l.c == r.c && l.d == r.d;
}

struct testgen  {
    using siganture = int(void*, void*);
    using operator_type = std::function<siganture>;
    static constexpr auto magic_number = 0x55FF33;

    static auto none() noexcept { return [](void*, void*) noexcept { return magic_number; }; }

    static constexpr auto one(auto, std::source_location = std::source_location::current()) noexcept {
        return [](void*, void*) -> int { return magic_number; };
    }

    static  constexpr auto all(auto, std::source_location = std::source_location::current()) noexcept {
        return [](void*, void*) -> int { return magic_number; };
    }
};

suite<"stub element"> stub_element_suite = [] {
    "primary constructor"_test = [] {
        element<testgen> sut{{0x1000,4}, testgen::none(), {}};
        expect(eq(sut.addr(), 0x1000UL));
        expect(eq(sut.size(), 4UL));
        expect(eq(sut.location().line(), 0U));
        expect(eq(sut(), testgen::magic_number));
    };
    "copy constructor copies all fields"_test = [] {
        test::native_type variable = 0;
        static constexpr test::native_type value = 0x55FF33U;
        const stub_element src {&variable, value};
        stub_element sut {src};
        expect(eq(sut.addr(), src.addr()));
        expect(eq(sut.size(), src.size()));
        expect(eq(sut.location().file_name(), src.location().file_name()));
        expect(eq(sut.location().line(), src.location().line()));
        sut();
        expect(eq(variable, value));
    };
    "move constructor copies all fields"_test = [] {
        test::native_type variable = 0;
        static constexpr test::native_type value = 0x55FF33U;
        stub_element src {&variable, value};
        stub_element sut {std::move(src)};
        expect(eq(sut.addr(), src.addr()));
        expect(eq(sut.size(), src.size()));
        expect(eq(sut.location().file_name(), src.location().file_name()));
        expect(eq(sut.location().line(), src.location().line()));
        sut();
        expect(eq(variable, value));
    };
    "uninitialized element by address"_test = [] {
        stub_element sut {{0x1000, 4UL}};
        expect(eq(sut.addr(), 0x1000UL));
        expect(eq(sut.size(), 4UL));
    };
    "uninitialized element by pointer"_test = [] {
        std::byte array[32];
        stub_element sut {{&array[0], &array[32]}};
        expect(eq(sut.addr(), reinterpret_cast<address_type>(&array[0])));
        expect(eq(sut.size(), sizeof(array)));
    };
    "native element by pointer"_test = [] {
        static constexpr test::native_type magic_number= 0xFEEDBEEF;
        test::native_type var{};
        stub_element sut{&var, magic_number};
        expect(eq(sut.addr(), reinterpret_cast<address_type>(&var)));
        expect(eq(sut.size(), sizeof(var)));
        sut();
        expect(eq(var, magic_number));
    };
    "native element by volatile pointer"_test = [] {
        static constexpr test::native_type magic_number= 0xFEEDBEEF;
        volatile test::native_type var{};
        stub_element sut{&var, magic_number};
        expect(eq(sut.addr(), reinterpret_cast<address_type>(&var)));
        expect(eq(sut.size(), sizeof(var)));
        sut();
        expect(var == magic_number);
    };
    "trivial data by address"_test = [] {
        static constexpr auto data = generate_bytes<16>();
        decltype(data) array {};
        stub_element sut{address(reinterpret_cast<address_type>(&array)), data};
        expect(eq(sut.addr(), reinterpret_cast<address_type>(&array)));
        expect(eq(sut.size(), array.size()));
        sut();
        expect(array == data);
    };
    "trivial data by pointer"_test = [] {
        static constexpr trivial_struct data = { 1, 2, 3, 4 };
        trivial_struct var {};
        stub_element sut{&var, data};
        expect(eq(sut.addr(), reinterpret_cast<address_type>(&var)));
        expect(eq(sut.size(), sizeof(var)));
        sut();
        expect(var == data);
    };
    "trivial data array"_test = [] {
        static constexpr test::native_type data = 0xC0;
        test::native_type array[16];
        stub_element sut{std::span{array}, data};
        expect(eq(sut.addr(), reinterpret_cast<address_type>(&array[0])));
        expect(eq(sut.size(), sizeof(array)));
        sut();
    };
    "overlapping return true for overlapping regions"_test = [] {
        stub_element sut1{{0x1000, 16}};
        stub_element sut2{{0x1008,  4}};
        stub_element sut3{{0x100C,  4}};
        stub_element sut4{{0x100E,  4}};
        expect(overlapping(sut1, sut2));
        expect(overlapping(sut1, sut3));
        expect(overlapping(sut1, sut4));
    };
    "overlapping return false for non overlapping regions"_test = [] {
        stub_element sut1{{0x1000, 16}};
        stub_element sut2{{0x1016,  4}};
        expect(!overlapping(sut1, sut2));
    };
    //todo tests for invalid value size
};

suite<"verify element"> verify_element_suite = [] {
    "native element by pointer"_test = [] {
        static constexpr test::native_type magic_number= 0xFEEDBEEF;
        test::native_type var{magic_number};
        verify_element sut{&var, magic_number};
        expect(eq(sut.addr(), reinterpret_cast<address_type>(&var)));
        expect(eq(sut.size(), sizeof(var)));
        expect(sut());
    };
    "trivial data by address"_test = [] {
        static constexpr auto data = generate_bytes<16>();
        auto array {data};
        verify_element sut{{reinterpret_cast<address_type>(&array), sizeof(data)}, comparator::one(data)};
        expect(eq(sut.addr(), reinterpret_cast<address_type>(&array)));
        expect(eq(sut.size(), array.size()));
        expect(sut());
    };
    "native element by address and size"_test = [] {
        verify_element sut {region{0x1010, 16}, comparator::all(test::native_type{0xA5A5A5A5U})};
    };
    "native array"_test = [] {
        static constexpr test::native_type c = 0xC0C0;
        test::native_type array[16] { c, c, c, c, c, c, c, c, c, c, c, c, c, c, c, c };
        verify_element sut{std::span{array}, c};
        expect(eq(sut.addr(), reinterpret_cast<address_type>(&array[0])));
        expect(eq(sut.size(), sizeof(array)));
        expect(sut());
    };
};

}


