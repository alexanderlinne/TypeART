// TypeART library
//
// Copyright (c) 2017-2022 TypeART Authors
// Distributed under the BSD 3-Clause license.
// (See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/BSD-3-Clause)
//
// Project home: https://github.com/tudasc/TypeART
//
// SPDX-License-Identifier: BSD-3-Clause
//

#ifndef TYPEART_RUNTIME_H
#define TYPEART_RUNTIME_H

#include "AccessCounter.h"
#include "TypeDB.h"
#include "TypeResolution.h"
#include "tracker/Tracker.h"

#include <cstddef>
#include <string>

namespace typeart::runtime {

struct PointerInfo;

class Runtime {
  static thread_local size_t scope;

  // scope must be set to true before all other members are initialized.
  // This is achieved by adding this struct as the first member.
  struct Initializer {
    Initializer() {
      scope = 1;
    }

    void reset() const {
      scope = initial_scope;
    }

   private:
    size_t initial_scope = scope;
  };

  Initializer init;
  TypeDB typeDB{};
  Recorder recorder{};
  TypeResolution typeResolution;
  tracker::Tracker tracker;

 public:
  struct ScopeGuard final {
    ScopeGuard() {
      scope += 1;
    }

    ~ScopeGuard() {
      scope -= 1;
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&)      = delete;

    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

    bool shouldTrack() const {
      return scope <= 1;
    }
  };

  static Runtime& get() {
    // As opposed to a global variable, a singleton + instantiation during
    // the first callback/query avoids some problems when
    // preloading (especially with MUST).
    static Runtime instance;
    return instance;
  }

  static tracker::Tracker& getTracker() {
    return get().tracker;
  }

  static const TypeResolution& getTypeResolution() {
    return get().typeResolution;
  }

  static Recorder& getRecorder() {
    return get().recorder;
  }

  static ScopeGuard scopeGuard() {
    return {};
  }

  static const AllocationInfo* getAllocationInfo(allocation_id_t allocation_id) {
    auto result = (const AllocationInfo*)nullptr;
    get().typeResolution.getAllocationInfo(allocation_id, &result);
    return result;
  }

  static std::string toString(const void* memAddr, int typeId, size_t count, size_t typeSize, const void* calledFrom);
  static std::string toString(const void* memAddr, int typeId, size_t count, const void* calledFrom);
  static std::string toString(const void* addr, const PointerInfo& info);

 private:
  Runtime();
  ~Runtime();
};

}  // namespace typeart::runtime

#endif  // TYPEART_RUNTIME_H
