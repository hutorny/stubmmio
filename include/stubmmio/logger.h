/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * logger.h - simple redirectable logger
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#pragma once
#include <string_view>
#include <logovod/logovod.h>

namespace stubmmio {
using priority = logovod::priority;

namespace logcategory {

using writer_type = logovod::sink::bundle::writer_type;

/// safe log category write file descriptor 2, as clog and cout may not yet/already be available
/// during static construction/destruction
struct safe : logovod::category {
    static constexpr logovod::priority level() noexcept { return logovod::priority::error; }
    static constexpr auto writer() noexcept { return logovod::sink::fd<2>; }
};


struct configuration {
    std::optional<writer_type> writer;
    std::optional<priority> level;
};

template<class Category, class Base>
struct configurable : Base {
  static priority level() noexcept { return config_.level.value_or(Base::level()); }
  static void level(priority value) noexcept { config_.level = value; }
  static void nolevel() noexcept { config_.level.reset(); }
  static writer_type writer() noexcept { return config_.writer.value_or(Base::writer()); }
  static void writer(writer_type value) noexcept { config_.writer = value; }
  static void nowriter() noexcept { config_.writer.reset(); }
  static auto config() noexcept { return config_; }
  static auto config(configuration cfg) noexcept { return config_ = cfg; }
private:
  static inline constinit configuration config_ {};
};
struct basic : configurable<basic, safe> {};
struct arena : configurable<arena, basic> {};
struct sigsegv : configurable<sigsegv, basic> {};
struct stimulus : configurable<stimulus, basic> {};
struct mock : configurable<sigsegv, basic> {};
struct verify : configurable<verify, basic> {};

template<typename ... Category>
void reset() {
    (Category::nowriter(), ...);
    (Category::nolevel(), ...);
}

} // namespace logcategory
namespace util {

template<typename Category>
requires requires { Category::writer(logovod::sink::clog); }
/// redirects loggers of Category and restores original configuration
class scoped_redirector {
public:
    using writer_type = logcategory::writer_type;
    /// scoped replacement of unexpected call logger
    scoped_redirector(writer_type func, priority value) : config_ { Category::config() } {
      Category::config({func, value});
    }
    scoped_redirector(writer_type func) : config_ { Category::config() } {
      Category::writer(func);
    }
    scoped_redirector() : config_ { Category::config() } {
      Category::config({&null, priority::emergency});
    }
    ~scoped_redirector() { Category::config(config_); }
    scoped_redirector(const scoped_redirector&) = delete;
    scoped_redirector(scoped_redirector&&) = delete;
    scoped_redirector& operator=(const scoped_redirector&) = delete;
    scoped_redirector& operator=(scoped_redirector&&) = delete;
    static void writer(writer_type func) noexcept { Category::writer(func); }
    static void level(priority value) noexcept { Category::level(value); }
private:
    static void null(std::string_view, std::string_view, logovod::attributes) noexcept {}
    logcategory::configuration config_ {};
};

struct redirect : scoped_redirector<logcategory::basic> {
    using base = scoped_redirector<logcategory::basic>;
    using base::base;
    ~redirect() {
        logcategory::reset<logcategory::basic, logcategory::arena, logcategory::mock, logcategory::stimulus,
                           logcategory::sigsegv, logcategory::verify>();
    }
};

template<auto Writer>
void simpler_writer(std::string_view message, std::string_view, logovod::attributes) noexcept {
    Writer(message);
}

} // namespace util
} // namespace stubmmio
