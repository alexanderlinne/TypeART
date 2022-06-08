// TypeART library
//
// Copyright (c) 2017-2021 TypeART Authors
// Distributed under the BSD 3-Clause license.
// (See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/BSD-3-Clause)
//
// Project home: https://github.com/tudasc/TypeART
//
// SPDX-License-Identifier: BSD-3-Clause
//

// We need to include this before we include spdlog headers
// clang-format off
#include <typeart/support/Logger.hpp>
// clang-format on

#include "Logger.h"

#include <fmt/ostream.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace typeart {

Logger::Logger() {
}

Logger::~Logger(){};

std::string name_for(MPI_Datatype datatype) {
  int len;
  char mpi_type_name[MPI_MAX_OBJECT_NAME];
  int mpierr = MPI_Type_get_name(datatype, &mpi_type_name[0], &len);
  return mpi_type_name;
}

struct InternalErrorVisitor {
  std::string operator()(const MPIError& err) {
    return fmt::format("{} failed: {}", err.function_name, err.message);
  }
  std::string operator()(const TypeARTError& err) {
    return fmt::format("{}", err.message);
  }
  std::string operator()(const InvalidArgument& err) {
    return fmt::format("{}", err.message);
  }
  std::string operator()(const UnsupportedCombiner& err) {
    return fmt::format("the MPI type combiner {} is currently not supported", err.combiner_name);
  }
  std::string operator()(const UnsupportedCombinerArgs& err) {
    return fmt::format("{}", err.message);
  }
};

struct TypeErrorVisitor {
  std::string operator()(const InsufficientBufferSize& err) {
    return fmt::format("buffer too small ({} elements, {} required)", err.actual, err.required);
  }
  std::string operator()(const BuiltinTypeMismatch& err) {
    auto type_name     = err.pointer_info.getType().get_pretty_name();
    auto mpi_type_name = name_for(err.mpi_type);
    return fmt::format(R"(expected a type matching MPI type "{}", but found type "{}")", mpi_type_name, type_name);
  }
  std::string operator()(const BufferNotOfStructType& err) {
    auto type_name = err.pointer_info.getType().get_pretty_name();
    return fmt::format("expected a struct type, but found type \"{}\"", type_name);
  }
  std::string operator()(const MemberCountMismatch& err) {
    auto type_name = err.pointer_info.getType().get_pretty_name();
    return fmt::format("expected {} members, but the type \"{}\" has {} members", err.mpi_member_count, type_name,
                       err.struct_member_count);
  }
  std::string operator()(const MemberOffsetMismatch& err) {
    auto type_name = err.pointer_info.getType().get_pretty_name();
    return fmt::format("expected a byte offset of {} for member \"{}\", but the type \"{}\" has an offset of {}",
                       err.mpi_offset, err.member->get_pretty_name(), type_name, err.struct_offset);
  }
  std::string operator()(const MemberTypeMismatch& err) {
    return fmt::format("the typecheck for member \"{}\" failed ({})", err.member->get_pretty_name(),
                       (*err.error).visit(*this));
  }
  std::string operator()(const MemberElementCountMismatch& err) {
    auto member_name      = err.member->get_pretty_name();
    auto member_type_name = err.member->get_type().get_pretty_name();
    return fmt::format("expected member \"{}\" to have {} element{}, but the type \"{}\" of \"{}\" has {} element{}",
                       member_name, err.mpi_count, err.mpi_count != 1 ? "s" : "", member_type_name, member_name,
                       err.buffer_count, err.buffer_count != 1 ? "s" : "");
  }
  std::string operator()(const StructSubtypeErrors& err) {
    std::vector<std::string> subtype_errors;
    std::transform(err.subtype_errors.begin(), err.subtype_errors.end(), std::back_inserter(subtype_errors),
                   [this](auto&& suberr) {
                     auto struct_type_name = suberr.pointer_info.getType().get_pretty_name();
                     auto type_name        = suberr.subtype_info.getType().get_pretty_name();
                     return fmt::format("Tried the first member [{} x {}] of struct type \"{}\" with error: {}",
                                        suberr.subtype_info.getCount(), type_name, struct_type_name,
                                        suberr.error->visit(*this));
                   });
    return fmt::format("{}. {}", err.primary_error->visit(*this), fmt::join(subtype_errors, ". "));
  }
};

struct ErrorVisitor {
  std::string operator()(const InternalError& err) {
    return err.visit(InternalErrorVisitor{});
  }
  std::string operator()(const TypeError& err) {
    return err.visit(TypeErrorVisitor{});
  }
};

bool use_source_location_for(spdlog::level::level_enum level) {
  const auto source_location_config = Config::get().getSourceLocation();
  return source_location_config == Config::SourceLocation::None ||
         (source_location_config == Config::SourceLocation::Error && level < spdlog::level::err);
}

std::string format_source_location(spdlog::level::level_enum level, const void* addr) {
  auto source_location =
      use_source_location_for(level) ? std::optional<SourceLocation>{} : SourceLocation::create(addr);

  if (source_location.has_value()) {
    return fmt::format("{}[{}] at {}:{}: ", source_location->function, addr, source_location->file,
                       source_location->line);
  }
  return fmt::format("at {}: ", addr);
}

void Logger::log(const void* called_from, const std::string& prefix, const Error& error) {
  auto source_location = error.stacktrace.has_value() ? "" : format_source_location(spdlog::level::err, called_from);
  LOG_ERROR("{}{}{}", source_location, prefix, error.visit(ErrorVisitor{}));

  if (error.stacktrace.has_value()) {
    for (const auto& entry : error.stacktrace.value()) {
      LOG_ERROR("\tin {}", entry);
    }
  }
}

void Logger::log(const char* name, const void* called_from, bool is_send, const runtime::PointerInfo& pointer_info,
                 const MPIType& type, int count, const Result<void>& result) {
  if (result.has_value()) {
    LOG_INFO("{}{}: successfully checked {}-buffer {} of type [{} x {}] against {} {} of MPI type \"{}\"",
             format_source_location(spdlog::level::info, called_from), name, is_send ? "send" : "recv",
             pointer_info.getBaseAddr(), pointer_info.getCount(), pointer_info.getType().get_pretty_name(), count,
             count == 1 ? "element" : "elements", name_for(type.mpi_type));
  } else {
    auto error                 = result.error();
    auto internal_error_prefix = error->is<TypeError>() ? "type error " : "internal error ";
    log(called_from,
        fmt::format("{}: {}while checking {}-buffer {} of type [{} x {}] against {} {} of MPI type \"{}\": ", name,
                    internal_error_prefix, is_send ? "send" : "recv", pointer_info.getBaseAddr(),
                    pointer_info.getCount(), pointer_info.getType().get_pretty_name(), count,
                    count == 1 ? "element" : "elements", name_for(type.mpi_type)),
        *error);
  }
}

void Logger::log(const char* name, const void* called_from, bool is_send, const void* ptr, const Error& error) {
  auto error_prefix = error.is<TypeError>() ? "error " : "internal error ";
  log(called_from,
      fmt::format("{}while checking the {}-buffer {} in a call to {}: ", error_prefix, is_send ? "send" : "recv", ptr,
                  name),
      error);
}

void Logger::log(const CallCounter& call_counter, long ru_maxrss) {
  LOG_INFO("CCounter {{ Send: {} Recv: {} Send_Recv: {} Unsupported: {} MAX RSS[KBytes]: {} }}", call_counter.send,
           call_counter.recv, call_counter.send_recv, call_counter.unsupported, ru_maxrss);
}

void Logger::log(const MPICounter& mpi_counter) {
  LOG_INFO("MCounter {{ Error: {} Null_Buf: {} Null_Count: {} Type_Error: {} }}", mpi_counter.error,
           mpi_counter.null_buff, mpi_counter.null_count, mpi_counter.type_error);
}

void Logger::log_zero_count(const char* function_name, const void* called_from, bool is_send, const void* ptr) {
  LOG_DEBUG("{}{}: attempted to {} 0 elements of buffer {}", format_source_location(spdlog::level::warn, called_from),
            function_name, is_send ? "send" : "receive", ptr);
}

void Logger::log_null_buffer(const char* function_name, const void* called_from, bool is_send) {
  LOG_DEBUG("{}{}: {}-buffer is NULL", format_source_location(spdlog::level::warn, called_from), function_name,
            is_send ? "send" : "recv");
}

void Logger::log_unsupported(const char* name) {
  LOG_ERROR("The MPI function {} is currently not checked by TypeArt", name);
}

}  // namespace typeart
