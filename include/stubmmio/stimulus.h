/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * stimulus.h - stubmmio stimulus
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#pragma once
#include <type_traits>
#include <concepts>
#include <cstdint>
#include <source_location>
#include <limits>
#include <vector>
#include <stubmmio/types.h>

namespace stubmmio {
namespace detail {

template<std::integral Type> using watch_type = std::add_volatile_t<std::add_const_t<Type>>;
template<std::integral Type> using watch_pointer = std::add_pointer_t<watch_type<Type>>;
template<std::integral Type> using watch_lref = std::add_lvalue_reference_t<watch_type<Type>>;
template<std::integral Type> using modify_type = std::add_volatile_t<std::remove_const_t<Type>>;
template<std::integral Type> using modify_pointer = std::add_pointer_t<modify_type<Type>>;
template<std::integral Type> using modify_lref = std::add_lvalue_reference_t<modify_type<Type>>;
template<typename Pointer>
requires std::is_pointer_v<Pointer>
static constexpr volatile void* cast(Pointer ptr) noexcept {
    using type = std::add_pointer_t<std::remove_const_t<std::remove_pointer_t<Pointer>>>;
    return const_cast<type>(ptr);
}

template<typename Pointer>
requires std::is_pointer_v<Pointer>
static constexpr volatile_span make_span(Pointer ptr) noexcept {
    using type = std::remove_pointer_t<Pointer>;
    return { static_cast<const volatile char*>(static_cast<const volatile void*>(ptr)), sizeof(type) };
}

}

template<class Function, typename Type>
concept condition = std::invocable<Function, detail::watch_lref<Type>>;
template<class Function, typename Type>
concept action = std::invocable<Function, detail::modify_lref<Type>>;

static inline constexpr struct inactive_type {} inactive {};

class istimulus {
public:
    enum class identity_type : std::uint64_t {};
    enum class status_type { idle, active, running, done };
    using spans_type = std::vector<detail::volatile_span>;
    virtual ~istimulus() = default;
    /// Activate or reactivate the stimulus
    void operator()() { activate(*this); }
    /// Returns stimulus status
    auto status() const noexcept { return status_; }
    /// Returns count of active stimuli
    static std::size_t count();
    /// Terminates all stimuli
    static void terminate();
protected:
    constexpr istimulus(std::source_location location = std::source_location::current())
      : location_ {location} {}
    /// activates the stimulus
    static void activate(istimulus&);
    /// deactivates the stimulus
    static bool deactivate(istimulus&);
private:
    /// returns associated memory spans
    virtual spans_type spans() = 0;
    /// runs the stimulus logic
    virtual status_type run() = 0;
    void active() { status_ = status_type::active; }
    void inactive() { status_ = status_ == status_type::done ? status_ : status_type::idle; }
    status_type running() {
        status_ = status_type::running;
        auto status = run();
        if (status == status_type::done)
            status_ = status_type::done;
        return status;
    }
    std::source_location location_;
    status_type status_ {};
    friend class stimulator;
};

template<std::integral Watch, std::integral Modify, condition<Watch> Condition, action<Modify> Action>
class stimulus : public istimulus {
public:
    // constructs and activates the stimulus upon pointers
    stimulus(Watch* watch, Condition cond, Modify* modify, Action act,
             std::source_location location = std::source_location::current())
      : istimulus { location }, condition_ { cond }, action_{act},
        watch_ { watch }, modify_ { const_cast<detail::modify_pointer<Modify>>(modify) } {
            activate(*this);
    }
    // constructs and activates the stimulus upon addresses
    stimulus(address watch, Condition cond, address modify, Action act,
             std::source_location location = std::source_location::current())
      : istimulus { location }, condition_ { cond }, action_{act},
        watch_addr_ { static_cast<std::uintptr_t>(watch) },
        modify_addr_ { static_cast<std::uintptr_t>(modify) } {
            activate(*this);
    }
    /// constructs inactive stimulus, to be cloned or activated later
    constexpr stimulus(inactive_type, Watch* watch, Condition cond, Modify* modify, Action act,
                       std::source_location location = std::source_location::current())
      : istimulus { location }, condition_ {cond }, action_{act}, watch_ { watch },
        modify_ { const_cast<detail::modify_pointer<Modify>>(modify) } { }
    /// constructs inactive stimulus from address, to be cloned or activated later
    constexpr stimulus(inactive_type, address watch, Condition cond, address modify, Action act,
                       std::source_location location = std::source_location::current())
      : istimulus { location }, condition_ {cond }, action_{act},
        watch_addr_ { static_cast<std::uintptr_t>(watch) },
        modify_addr_ { static_cast<std::uintptr_t>(modify) } { }
    /// creates a copy of stimulus and activates it
    stimulus(const stimulus& that)
      : istimulus { that }, condition_ { that.condition_ }, action_ { that.action_ }, watch_ { that.watch_ },
        modify_ { that.modify_ } {
        activate(*this);
    }
    /// creates an inactive copy of stimulus
    constexpr stimulus(inactive_type, const stimulus& that)
      : istimulus { that }, condition_ { that.condition_ }, action_ { that.action_ }, watch_ { that.watch_ },
        modify_ { that.modify_ } {
    }
    /// moves stimulus and preserves its activation status
    stimulus(stimulus&& that)
    : istimulus { that }, watch_ { that.watch_ }, modify_ { that.modify_ } {
        const bool was_active = deactivate(that);
        condition_ = std::move(that.condition_);
        action_ = std::move (that.action_);
        if (was_active)
            activate(*this);
    }
    stimulus& operator=(const stimulus&) = delete;
    stimulus& operator=(stimulus&&) = delete;
    constexpr ~stimulus() {
        if ! consteval {
            deactivate(*this);
        }
    }
private:
    spans_type spans() override {
        return {
            detail::make_span(watch_),
            detail::make_span(modify_),
        };
    }
    status_type run() override {
        if(condition_(*watch_)) {
            action_(*modify_);
            return status_type::done;
        } else
            return status_type::idle;
    }
    Condition condition_ {};
    Action action_ {};
    union {
      detail::watch_pointer<Watch> watch_;
      std::uintptr_t watch_addr_;
    };
    union {
        detail::modify_pointer<Modify> modify_;
        std::uintptr_t modify_addr_;
    };
};

template<std::integral Type>
using simple_condition = bool(*)(detail::watch_lref<Type>);
template<std::integral Type>
using simple_action = void(*)(detail::modify_lref<Type>);

stimulus(inactive_type, address, simple_condition<std::uint8_t>, address, simple_action<std::uint8_t>)
  ->stimulus<std::uint8_t, std::uint8_t, simple_condition<std::uint8_t>, simple_action<std::uint8_t>>;

stimulus(inactive_type, address, simple_condition<std::uint16_t>, address, simple_action<std::uint16_t>)
  ->stimulus<std::uint16_t, std::uint16_t, simple_condition<std::uint16_t>, simple_action<std::uint16_t>>;

stimulus(inactive_type, address, simple_condition<std::uint32_t>, address, simple_action<std::uint32_t>)
  ->stimulus<std::uint32_t, std::uint32_t, simple_condition<std::uint32_t>, simple_action<std::uint32_t>>;

stimulus(address, simple_condition<std::uint8_t>, address, simple_action<std::uint8_t>)
  ->stimulus<std::uint8_t, std::uint8_t, simple_condition<std::uint8_t>, simple_action<std::uint8_t>>;

stimulus(address, simple_condition<std::uint16_t>, address, simple_action<std::uint16_t>)
  ->stimulus<std::uint16_t, std::uint16_t, simple_condition<std::uint16_t>, simple_action<std::uint16_t>>;

stimulus(address, simple_condition<std::uint32_t>, address, simple_action<std::uint32_t>)
  ->stimulus<std::uint32_t, std::uint32_t, simple_condition<std::uint32_t>, simple_action<std::uint32_t>>;


} // namespace stubmmio



 // namespace 
