/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * src/util.cxx -
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <stubmmio/stubmmio.h>
#include <stubmmio/logger.h>
#include <format>
#include <signal.h>
#include <iostream>
#include <errno.h>
#include <cstring>

namespace stubmmio::util {
using namespace stubmmio::exceptions;
using log = logovod::logger<logcategory::sigsegv>;

static void sigsegv_action(int, siginfo_t * si, void*) {
    auto msg = std::format("Access to unallocated address {}\n", si->si_addr);
    log::error{}(msg);
    throw access_to_unallocated_address(msg);
}

void handle_sigsegv() {
    struct sigaction act {};
    struct sigaction old {};
    act.sa_sigaction = &sigsegv_action;
    act.sa_flags = SA_SIGINFO;
    if(sigaction(SIGSEGV, &act, &old) != 0) {
        log::critical{}.format("sigaction error {}: {}", errno, strerror(errno));
    }
}

} // namespace stubmmio::util
