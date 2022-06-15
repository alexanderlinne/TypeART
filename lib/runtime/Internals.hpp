#pragma once

#include <meta/Database.hpp>
#include <runtime/AccessCounter.hpp>

namespace typeart {

struct ScopeGuard final {
  ScopeGuard();
  ~ScopeGuard();

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard(ScopeGuard&&)      = delete;

  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

  bool shouldTrack() const;
};

Recorder& getRecorder();
meta::Database& getDatabase();

}  // namespace typeart