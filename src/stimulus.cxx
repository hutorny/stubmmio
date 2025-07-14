/*
 * Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * src/stimulus.cxx -stimulus implementation
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <stubmmio/stubmmio.h>
#include <stubmmio/stimulus.h>
#include <stubmmio/logger.h>

#include "mmio.h"

#include <algorithm>
#include <condition_variable>
#include <iostream>
#include <format>
#include <mutex>
#include <thread>
#include <vector>

namespace stubmmio {

class stimulator : detail::mmio::listener {
public:
    stimulator() : thread_ { [this]() noexcept { run(); } } {
        detail::mmio::arena().subscribe(this);
    }
    // instance to be created on first use
    static stimulator& instance() {
        static stimulator inst {};
        return inst;
    }
    void activate(istimulus&);
    bool deactivate(istimulus&);
    void terminate() noexcept {
        terminate_ = true;
    }
    virtual ~stimulator() {
        log_stalls();
        terminate();
        detail::mmio::arena().unsubscribe(this);
    }
    auto count() {
        std::lock_guard lock { mutex_ };
        return stimuli_.size();
    }
private:
    void run() noexcept;
    void unmapping(detail::volatile_span, std::source_location) override;
    using pos_type = std::make_signed_t<std::size_t>;
    void adjust_index(auto);
    std::jthread thread_;
    std::vector<istimulus*> stimuli_ {};
    std::mutex mutex_ {};
    std::size_t current_index_ {};
    volatile bool terminate_ {};
    volatile bool ready_ {};
    static void check_pages(const auto& list, std::source_location location);
    void log_stalls();
    using log = logovod::logger<logcategory::stimulus>;
};

static bool contains(detail::volatile_span range, detail::volatile_span addresses) {
    return (range.begin() <= addresses.begin() && addresses.begin() <= range.end()) ||
           (range.begin() <= addresses.end() && addresses.end() <= range.end());
}

void stimulator::unmapping(detail::volatile_span range, std::source_location location) {
    std::lock_guard lock {mutex_};
    if (stimuli_.empty()) return; // nothing to do if no stimuli
    std::size_t putpos = 0;
    for(std::size_t i = 0; i < stimuli_.size(); ++i) {
        istimulus* stimul = stimuli_[i];
        const auto spans { stimul->spans() };
        const auto found = std::find_if(spans.begin(), spans.end(), [range](auto sp){ return contains(range, sp); });
        if (found == spans.end()) {
            stimuli_[putpos++] = stimul;
        } else {
            adjust_index(i);
            log::error{}.format("Removing stimulus because it uses stub page being deallocated\n"
                "Stimulus defined at {}:{}:\nStub defined at {}:{}\n",
                stimul->location_.file_name(), stimul->location_.line(), location.file_name(), location.line());
        }
    }
    if (putpos != stimuli_.size())
        stimuli_.erase(stimuli_.begin() + static_cast<pos_type>(putpos), stimuli_.end());
}


void stimulator::check_pages(const auto& list, std::source_location location) {
    for(const auto& el : list) {
        if(reinterpret_cast<std::uintptr_t>(&el.front()) < arena::size() && ! detail::mmio::arena().contains(el)) {
            throw exceptions::page_is_not_allocated{std::format(
                "page is not allocated for stimulus declared at {}:{}",
                location.file_name(), location.line())};
        }
    }
}

void stimulator::activate(istimulus& stimul) {
    check_pages(stimul.spans(), stimul.location_);
    std::lock_guard lock {mutex_};
    auto found = std::find(stimuli_.begin(), stimuli_.end(), &stimul);
    if (found != stimuli_.end())
        return;
    stimuli_.push_back(&stimul);
    stimul.active();
    ready_ = true;
}

void stimulator::adjust_index(auto index_removed) {
  if (! stimuli_.empty() && static_cast<std::size_t>(index_removed) < (current_index_ % stimuli_.size())) {
         --current_index_;
    }
}

bool stimulator::deactivate(istimulus& stimul) {
    std::lock_guard lock {mutex_};
    const auto found = std::find(stimuli_.begin(), stimuli_.end(), &stimul);
    if (found == stimuli_.end())
        return false;
    adjust_index(found - stimuli_.begin());
    std::erase(stimuli_, &stimul);
    stimul.inactive();
    ready_ = ! stimuli_.empty();
    return true;
}

void stimulator::run() noexcept {
    try {
        for(; !terminate_; std::this_thread::yield()) {
            std::unique_lock lock {mutex_};
            while(!(ready_ || terminate_)) {
                lock.unlock();
                std::this_thread::yield();
                lock.lock();
            }
            if (terminate_ || stimuli_.empty()) continue;
            auto stimul { stimuli_[current_index_ % stimuli_.size()] };
            try {
                const auto status = stimul->running();
                if (status == istimulus::status_type::done) {
                    std::erase(stimuli_, stimul);
                } else {
                    ++current_index_;
                }
            } catch(const std::exception& error) {
                log::error{}.format("Exception caught when running stimulus defined at {}:{}:\n{}",
                        stimul->location_.file_name(), stimul->location_.line(), error.what());
                std::erase(stimuli_, stimul);
            }
        }
    } catch(...) {
        log::alert("Stimulator thread terminated with unknown exception");
    }
}

void stimulator::log_stalls() {
    std::lock_guard lock {mutex_};
    if(!stimuli_.empty()) {
        log::error{}.format("{} stall stimuli has not finished:\n", stimuli_.size());
        for(auto stimul : stimuli_) {
            log::error{}.format("Stimulus defined at {}:{}:\n", stimul->location_.file_name(), stimul->location_.line());
        }
    }
}

void istimulus::activate(istimulus& stimul) {
    stimulator::instance().activate(stimul);
}

bool istimulus::deactivate(istimulus& stimul) {
    return stimulator::instance().deactivate(stimul);
}

std::size_t istimulus::count() {
    return stimulator::instance().count();
}

void istimulus::terminate() {
    return stimulator::instance().terminate();
}


} // namespace stubmmio


