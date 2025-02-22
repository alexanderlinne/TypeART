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

#include "support/System.hpp"

#include <algorithm>
#include <cstdio>
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <filesystem>
#include <memory>
#include <sstream>
#include <sys/resource.h>

namespace typeart {
namespace system {

Process::Process() {
  self_exe = std::filesystem::canonical("/proc/self/exe");
}

const std::string& Process::exe() const {
  return self_exe;
}

const Process& Process::get() {
  static Process self;
  return self;
}

long Process::getMaxRSS() {
  rusage proc;
  getrusage(RUSAGE_SELF, &proc);

  return proc.ru_maxrss;
}

class CommandPipe {
  using UniqueFile = std::unique_ptr<FILE, int (*)(FILE*)>;
  UniqueFile command;

  explicit CommandPipe(UniqueFile command);

 public:
  static std::optional<CommandPipe> create(std::string_view command);

  [[nodiscard]] std::string nextLine() const;
};

CommandPipe::CommandPipe(CommandPipe::UniqueFile command) : command(std::move(command)) {
}

std::optional<CommandPipe> CommandPipe::create(std::string_view command) {
  UniqueFile comm(popen(command.data(), "r"), &pclose);

  if (!comm) {
    return {};
  }

  return CommandPipe{std::move(comm)};
}

std::string CommandPipe::nextLine() const {
  char* buffer{nullptr};
  size_t buffer_length{0};
  std::string result;

  if (getline(&buffer, &buffer_length, command.get()) != -1) {
    result = buffer;
  }

  if (buffer != nullptr) {
    free(buffer);
  }

  if (!result.empty()) {
    result.resize(result.size() - 1);  // remove \n
  }

  return result;
}

bool test_command(std::string_view command, std::string_view test_arg) {
  const auto available = [](const std::string_view command) -> bool {
    constexpr int command_not_found{127};
    auto* proc        = popen(command.data(), "r");
    const int ret_val = pclose(proc);
    return WEXITSTATUS(ret_val) != command_not_found;
  };

  std::ostringstream os;
  os << command << " " << test_arg;

  const bool avail = available(os.str());
  return avail;
}

class SourceLocHelper {
  bool has_addr2line{false};
  bool has_llvmsymbolizer{false};

  SourceLocHelper() {
    has_addr2line      = test_command("addr2line");
    has_llvmsymbolizer = test_command("llvm-symbolizer");
  }

 public:
  static const SourceLocHelper& get() {
    static SourceLocHelper helper;
    return helper;
  }

  [[nodiscard]] bool hasAddr2line() const {
    return has_addr2line;
  }

  [[nodiscard]] bool hasLLVMSymbolizer() const {
    return has_llvmsymbolizer;
  }
};

}  // namespace system

std::optional<BinaryLocation> BinaryLocation::create(const void* addr) {
  if (addr == nullptr) {
    return {};
  }

  const auto demangle = [](auto symbol_name) {
    using demangle_result = std::unique_ptr<char, decltype(&std::free)>;
    int status{0};
    demangle_result result{abi::__cxa_demangle(symbol_name, NULL, NULL, &status), &std::free};

    return result ? std::string{result.get()} : std::string{symbol_name};
  };

  Dl_info info;
  if (dladdr(addr, &info) != 0) {
    auto symbol_name = info.dli_sname != nullptr ? std::optional{demangle(info.dli_sname)} : std::nullopt;
    return {BinaryLocation{std::string{info.dli_fname}, info.dli_fbase, symbol_name, info.dli_saddr}};
  }

  return {};
}

std::optional<SourceLocation> SourceLocation::create(const void* addr) {
  if (addr == nullptr) {
    return {};
  }

  const auto pipe = [](const void* addr) -> std::optional<system::CommandPipe> {
    using namespace system;
    const auto& sloc_helper = SourceLocHelper::get();
    const auto& proc        = system::Process::get();

    if (sloc_helper.hasLLVMSymbolizer()) {
      std::ostringstream command;
      command << "unset LD_PRELOAD && llvm-symbolizer --demangle --output-style=GNU -f -e " << proc.exe() << " "
              << addr;
      auto llvm_symbolizer = system::CommandPipe::create(command.str());
      if (llvm_symbolizer) {
        return llvm_symbolizer;
      }
    }

    if (sloc_helper.hasAddr2line()) {
      std::ostringstream command;
      command << "unset LD_PRELOAD && addr2line --demangle=auto -f -e " << proc.exe() << " " << addr;
      auto addr2line = system::CommandPipe::create(command.str());
      if (addr2line) {
        return addr2line;
      }
    }

    return {};
  }(addr);

  if (!pipe) {
    return {};
  }

  SourceLocation loc;

  loc.function             = pipe->nextLine();
  const auto file_and_line = pipe->nextLine();
  const auto delimiter     = file_and_line.find(':');
  loc.line                 = file_and_line.substr(delimiter + 1);
  loc.file                 = file_and_line.substr(0, delimiter);

  return loc;
}

StacktraceEntry StacktraceEntry::create(void* addr) {
  return StacktraceEntry{addr, BinaryLocation::create(addr), SourceLocation::create(addr)};
}

std::ostream& operator<<(std::ostream& os, const StacktraceEntry& entry) {
  if (entry.binary.has_value()) {
    const auto& binary = entry.binary.value();

    os << binary.file << " (";

    if (binary.function.has_value()) {
      os << binary.function.value() << "+"
         << (static_cast<char*>(entry.addr) - static_cast<char*>(binary.function_addr));
    } else {
      if (entry.source.has_value()) {
        os << entry.source->function;
      }
    }
  } else {
    os << "?? (";
    if (entry.source.has_value()) {
      os << entry.source->function;
    }
  }

  os << ") at ";

  if (entry.source.has_value()) {
    const auto& source = entry.source.value();
    os << source.file << ":" << source.line;
  } else {
    os << "??:0";
  }

  return os;
}

Stacktrace::Stacktrace(std::vector<StacktraceEntry> entries) : entries(std::move(entries)) {
}

Stacktrace Stacktrace::current() {
  // TODO Document dladdr needs -rdynamic as linker option.
  void* buffer[MAX_STACKTRACE_SIZE];
  const auto size = backtrace(buffer, MAX_STACKTRACE_SIZE);

  std::vector<StacktraceEntry> entries;
  std::transform(buffer, buffer + size, std::back_inserter(entries), &StacktraceEntry::create);

  return {entries};
}

}  // namespace typeart
