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

#define SPDLOG_ACTIVE_LEVEL 0

#pragma once

#include <fmt/ostream.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>

#ifndef TYPEART_LOG_LEVEL
/*
 * Usually set at compile time: -DTYPEART_LOG_LEVEL=<N>, N in [0, 5] for output
 * 5 being most verbose
 */
#define TYPEART_LOG_LEVEL 5
#endif

#ifndef TYPEART_MPI_LOGGER
#define TYPEART_MPI_LOGGER 0
#endif

namespace llvm {

inline std::ostream& operator<<(std::ostream& os, const llvm::Value& value) {
  std::string buffer;
  llvm::raw_string_ostream ss(buffer);
  ss << value;
  os << ss.str();
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const llvm::StringRef& str) {
  os << str.str();
  return os;
}

}  // namespace llvm

namespace typeart {

std::shared_ptr<spdlog::logger>& logger();

}  // namespace typeart

#if TYPEART_LOG_LEVEL >= 5
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(typeart::logger(), __VA_ARGS__)
#else
#define LOG_TRACE(...) (void)0
#endif

#if TYPEART_LOG_LEVEL >= 4
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(typeart::logger(), __VA_ARGS__)
#else
#define LOG_DEBUG(...) (void)0
#endif

#if TYPEART_LOG_LEVEL >= 3
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(typeart::logger(), __VA_ARGS__)
#else
#define LOG_INFO(...) (void)0
#endif

#if TYPEART_LOG_LEVEL >= 2
#define LOG_WARNING(...) SPDLOG_LOGGER_WARN(typeart::logger(), __VA_ARGS__)
#else
#define LOG_WARNING(...) (void)0
#endif

#if TYPEART_LOG_LEVEL >= 1
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(typeart::logger(), __VA_ARGS__)
#else
#define LOG_ERROR(...) (void)0
#endif

#if TYPEART_LOG_LEVEL >= 0
#define LOG_FATAL(...) SPDLOG_LOGGER_CRITICAL(typeart::logger(), __VA_ARGS__)
#else
#define LOG_FATAL(...) (void)0
#endif
