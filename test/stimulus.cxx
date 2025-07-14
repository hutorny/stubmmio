/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/stimulus.cxx - unit tests for stimulus
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */
#include <stubmmio/stimulus.h>
#include <stubmmio/stubmmio.h>
#include <stubmmio/literals.h>
#include <stubmmio/unit.h>
#include <stubmmio/logger.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#pragma GCC diagnostic ignored "-Warray-bounds"

namespace {
using namespace std::chrono_literals;
using namespace boost::ut;
using namespace boost::ut::bdd;
using namespace boost::ut::literals;
using namespace stubmmio;
using namespace stubmmio::literals;

template<typename T>
[[gnu::always_inline]] inline volatile T* test_addr(std::uintptr_t addr) {
    return reinterpret_cast<volatile T*>(addr);
}

void test_workflow(auto& sut, auto volatile &var, auto value) {
    static constexpr auto timeout = 100ms;
    auto finish_by = std::chrono::steady_clock::now() + timeout;
    while(sut.status() == istimulus::status_type::idle) {
        std::this_thread::yield();
        if (std::chrono::steady_clock::now() > finish_by)
            throw std::logic_error("stimulus activation timed out");
    }
    var = value;
    finish_by = std::chrono::steady_clock::now() + timeout;
    while(sut.status() != istimulus::status_type::done) {
        std::this_thread::yield();
        if (std::chrono::steady_clock::now() > finish_by)
            throw std::logic_error("stimulus completion timed out");
    }
}

template<std::uint32_t BaseAddr>
static const stub::initializer_list test_mmio = {{address(BaseAddr), 0_U32}, {address(BaseAddr+sizeof(uint32_t)), 0_U32}};

template<std::uint32_t BaseAddr>
static constexpr auto active_stimulus() {
    return stimulus {
      test_addr<uint32_t>(BaseAddr), [](volatile const uint32_t& var) { return var & 1; },
      test_addr<uint32_t>(BaseAddr+sizeof(uint32_t)), [](volatile uint32_t& var) { var |= 2; },
    };
}

template<std::uint32_t BaseAddr>
static constexpr auto inactive_stimulus() {
    return stimulus { inactive,
      test_addr<uint32_t>(BaseAddr), [](volatile const uint32_t& var) { return var & 1; },
      test_addr<uint32_t>(BaseAddr+sizeof(uint32_t)), [](volatile uint32_t& var) { var |= 2; },
    };
}

bool test_condition(volatile const uint32_t& var) { return var & 1; }
void test_action(volatile uint32_t& var) { var |= 2; }

constinit stimulus constinit_stimulus  { inactive,
    address(2000), &test_condition,
    address(2004), &test_action,
};

constexpr stimulus constexpr_stimulus  { inactive,
    address(4000), &test_condition,
    address(4004), &test_action,
};

suite<"stimulus"> stimulus_suite = [] {
    "primary constructor"_test = [] {
        volatile bool watch_bool {};
        volatile bool modify_bool {};
        stimulus sut {
            &watch_bool, [](volatile const bool& var) { return var; },
            &modify_bool, [](volatile bool& var) {  var = true; }
        };
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, watch_bool, true);
        expect(modify_bool);
        expect(eq(istimulus::count(), 0U));
    };
    "constructor of inactive stimulus"_test = [] {
        volatile bool watch_bool {};
        volatile bool modify_bool {};
        stimulus sut { inactive,
            &watch_bool, [](volatile const bool& var) { return var; },
            &modify_bool, [](volatile bool& var) {  var = true; }
        };
        expect(eq(istimulus::count(), 0U));
        sut();
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, watch_bool, true);
        expect(modify_bool);
        expect(eq(istimulus::count(), 0U));
    };
    "deduced constructor of active stimulus"_test = [] {
        stub setup { test_mmio<2000> };
        setup();
        stimulus sut {
            address(2000), [](volatile const uint16_t& var) { return var != 0; },
            address(2004), [](volatile uint16_t& var) {  var = 1_U16; }
        };
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, *test_addr<uint16_t>(2000), 1_U16);
        expect(*test_addr<uint16_t>(2000) == 1_U16);
        expect(eq(istimulus::count(), 0U));
    };
    "deduced constructor of inactive stimulus"_test = [] {
        stub setup { test_mmio<2000> };
        setup();
        stimulus sut { inactive,
            address(2000), [](volatile const uint16_t& var) { return var != 0; },
            address(2004), [](volatile uint16_t& var) {  var = 1_U16; }
        };
        expect(eq(istimulus::count(), 0U));
        sut();
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, *test_addr<uint16_t>(2000), 1_U16);
        expect(*test_addr<uint16_t>(2000) == 1_U16);
        expect(eq(istimulus::count(), 0U));
    };
    "stimulus on MMIO arena"_test = [] {
        stub setup { test_mmio<2000> };
        setup();
        stimulus sut { active_stimulus<2000>() };
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, *test_addr<uint32_t>(2000), 1U);
        expect(*test_addr<uint32_t>(2004) == 2);
        expect(eq(istimulus::count(), 0U));
    };
    "stimulus throws on invalid page"_test = [] {
        stimulus sut { inactive_stimulus<2000>() };
        expect(eq(istimulus::count(), 0U));
        expect(throws([&sut] () { sut(); }));
        expect(eq(istimulus::count(), 0U));
    };
    "page being deallocated"_test = [] {
        util::scoped_redirector<logcategory::stimulus> ignore {};
        stub setup { test_mmio<0x4000> };
        setup();
        stimulus kept { active_stimulus<0x4000>() };
        auto sut = [] {
            stub local { test_mmio<2000> };
            local();
            return active_stimulus<2000>();
        }();
        expect(eq(istimulus::count(), 1U));
        test_workflow(kept, *test_addr<uint32_t>(0x4000), 1U);
        expect(*test_addr<uint32_t>(0x4004) == 2);
        expect(eq(istimulus::count(), 0U));
    };
    "copy constructor, active"_test = [] {
        stub setup { test_mmio<0x5000> };
        setup();
        stimulus sut { active_stimulus<0x5000>() };
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, *test_addr<uint32_t>(0x5000), 1U);
        expect(*test_addr<uint32_t>(0x5004) == 2);
        expect(eq(istimulus::count(), 0U));
    };
    "copy constructor, inactive"_test = [] {
        stub setup { test_mmio<2000> };
        setup();
        stimulus sut { constinit_stimulus };
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, *test_addr<uint32_t>(2000), 1U);
        expect(*test_addr<uint32_t>(2004) == 2);
        expect(eq(istimulus::count(), 0U));
    };
    "copy constructor, from inactive constexpr"_test = [] {
        stub setup { test_mmio<4000> };
        setup();
        stimulus sut { constexpr_stimulus };
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, *test_addr<uint32_t>(4000), 1U);
        expect(*test_addr<uint32_t>(4004) == 2);
        expect(eq(istimulus::count(), 0U));
    };
    "copy inactive constructor"_test = [] {
        stub setup { test_mmio<4000> };
        setup();
        stimulus sut { inactive, constexpr_stimulus };
        expect(eq(istimulus::count(), 0U));
        sut();
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, *test_addr<uint32_t>(4000), 1U);
        expect(*test_addr<uint32_t>(4004) == 2);
        expect(eq(istimulus::count(), 0U));
    };
    "move constructor, active"_test = [] {
        stub setup { test_mmio<2000> };
        setup();
        stimulus moving {
            address(2000), [](volatile const uint32_t& var) { return var != 0; },
            address(2004), [](volatile uint32_t& var) {  var = 1U; }
        };
        expect(eq(istimulus::count(), 1U));
        stimulus sut { std::move(moving) };
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, *test_addr<uint32_t>(2000), 1U);
        expect(*test_addr<uint32_t>(2004) == 1U);
        expect(eq(istimulus::count(), 0U));
    };
    "move constructor, inactive"_test = [] {
        stub setup { test_mmio<2000> };
        setup();
        stimulus moving { inactive,
            address(2000), [](volatile const uint32_t& var) { return var != 0; },
            address(2004), [](volatile uint32_t& var) {  var = 1U; }
        };
        expect(eq(istimulus::count(), 0U));
        stimulus sut { std::move(moving) };
        expect(eq(istimulus::count(), 0U));
        sut();
        expect(eq(istimulus::count(), 1U));
        test_workflow(sut, *test_addr<uint32_t>(2000), 1U);
        expect(*test_addr<uint32_t>(2004) == 1U);
        expect(eq(istimulus::count(), 0U));
    };
};

}
