#pragma once

#include "analysis/MemInstFinder.h"

namespace typeart::cl {

analysis::MemInstFinderConfig getMemInstFinderConfig();

const std::string& getTypeFilepath();
enum class InstrumentationMode {
  combined,
  before_optimization,
  after_optimization,
};
InstrumentationMode getInstrumentationMode();
bool getInstrumentGlobal();
bool getInstrumentStack();
bool getInstrumentStackLifetime();
bool getInstrumentHeap();
bool getPrintStats();

}  // namespace typeart::cl
