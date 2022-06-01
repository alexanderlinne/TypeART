#pragma once

#include "analysis/MemInstFinder.h"

namespace typeart::cl {

analysis::MemInstFinderConfig getMemInstFinderConfig();

const std::string& getTypeFilepath();
bool getInstrumentGlobal();
bool getInstrumentStack();
bool getInstrumentStackLifetime();
bool getInstrumentHeap();
bool getPrintStats();

}  // namespace typeart::cl
