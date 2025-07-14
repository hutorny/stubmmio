/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * test/main.cxx - main test runner
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <stubmmio/stubmmio.h>
#include <stubmmio/logger.h>
#include <stubmmio/unit.h>
#include <iostream>

using namespace stubmmio;
int main(int argc, char *argv[]) {
    logcategory::basic::level(priority::warning); // warning level for all log categories
    util::redirect logging(logovod::sink::clog);  // using clog for logging for the scope of main
    arena::check_boundary();
    arena::check_pagesize(getpagesize());
    util::handle_sigsegv();
    auto result = boost::ut::cfg<>.run(boost::ut::run_cfg{.argc = argc, .argv = const_cast<const char**>(argv)});
    return result;
}
