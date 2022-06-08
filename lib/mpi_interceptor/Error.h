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

#ifndef TYPEART_MPI_INTERCEPTOR_ERROR_H
#define TYPEART_MPI_INTERCEPTOR_ERROR_H

#include "Config.h"

#include <memory>
#include <mpi.h>
#include <optional>
#include <result.hpp>
#include <typeart/TypeART.hpp>
#include <typeart/support/System.hpp>
#include <variant>
#include <vector>

namespace typeart {

namespace detail {

template <class... Ts>
struct [[nodiscard]] VariantError {
 private:
  std::variant<Ts...> data;

 public:
  template <class... Param>
  VariantError(Param&&... param) : data(std::forward<Param>(param)...) {
  }

  template <class T>
  [[nodiscard]] bool is() const {
    return std::holds_alternative<T>(data);
  }

  template <class T>
  [[nodiscard]] const T& get() const& {
    return std::get<T>(data);
  }

  template <class T>
  [[nodiscard]] T get() && {
    return std::get<T>(std::move(data));
  }

  template <class Visitor>
  auto visit(Visitor&& visitor) const -> decltype(auto) {
    return std::visit(std::forward<Visitor>(visitor), data);
  }
};

}  // namespace detail

struct MPIError {
  std::string function_name;
  std::string message;
};

struct TypeARTError {
  std::string message;
};

struct InvalidArgument {
  std::string message;
};

struct UnsupportedCombiner {
  std::string combiner_name;
};

struct UnsupportedCombinerArgs {
  std::string message;
};

struct [[nodiscard]] InternalError : public detail::VariantError<MPIError, TypeARTError, InvalidArgument,
                                                                 UnsupportedCombiner, UnsupportedCombinerArgs> {};

struct TypeError;
struct InsufficientBufferSize {
  size_t actual;
  size_t required;
};

struct BuiltinTypeMismatch {
  PointerInfo pointer_info;
  MPI_Datatype mpi_type;
};

struct BufferNotOfStructType {
  PointerInfo pointer_info;
};

struct MemberCountMismatch {
  PointerInfo pointer_info;
  size_t struct_member_count;
  int mpi_member_count;
};

struct MemberOffsetMismatch {
  PointerInfo pointer_info;
  const meta::di::Member* member;
  byte_offset struct_offset;
  byte_offset mpi_offset;
};

struct MemberTypeMismatch {
  PointerInfo pointer_info;
  const meta::di::Member* member;
  std::unique_ptr<TypeError> error;
};

struct MemberElementCountMismatch {
  PointerInfo pointer_info;
  const meta::di::Member* member;
  size_t buffer_count;
  size_t mpi_count;
};

struct StructSubtypeMismatch {
  PointerInfo pointer_info;
  PointerInfo subtype_info;
  std::unique_ptr<TypeError> error;
};

struct StructSubtypeErrors {
  std::unique_ptr<TypeError> primary_error;
  std::vector<StructSubtypeMismatch> subtype_errors;
};

struct [[nodiscard]] TypeError
    : public detail::VariantError<StructSubtypeErrors, InsufficientBufferSize, BuiltinTypeMismatch,
                                  BufferNotOfStructType, MemberCountMismatch, MemberOffsetMismatch, MemberTypeMismatch,
                                  MemberElementCountMismatch> {};

struct [[nodiscard]] Error : public detail::VariantError<InternalError, TypeError> {
  std::optional<Stacktrace> stacktrace =
      Config::get().isWithBacktraces() ? std::optional{Stacktrace::current()} : std::optional<Stacktrace>{};
};

template <class T>
struct Result : public cpp::result<T, std::shared_ptr<Error>> {
  Result(T value) : cpp::result<T, std::shared_ptr<Error>>(std::move(value)){};

  Result(std::shared_ptr<Error> err) : cpp::result<T, std::shared_ptr<Error>>(cpp::fail(std::move(err))){};

  Result(cpp::result<T, std::shared_ptr<Error>> result) : cpp::result<T, std::shared_ptr<Error>>(std::move(result)){};
};

template <>
struct Result<void> : public cpp::result<void, std::shared_ptr<Error>> {
  Result() : cpp::result<void, std::shared_ptr<Error>>(){};

  Result(std::shared_ptr<Error> err) : cpp::result<void, std::shared_ptr<Error>>(cpp::fail(std::move(err))){};

  Result(cpp::result<void, std::shared_ptr<Error>> result)
      : cpp::result<void, std::shared_ptr<Error>>(std::move(result)){};
};

template <class Type, class... Param>
std::shared_ptr<Error> make_internal_error(Param... param) {
  return std::make_shared<Error>(Error{InternalError{Type{std::forward<Param>(param)...}}});
}

template <class Type, class... Param>
std::shared_ptr<Error> make_type_error(Param... param) {
  return std::make_shared<Error>(Error{TypeError{Type{std::forward<Param>(param)...}}});
}

}  // namespace typeart

#endif  // TYPEART_MPI_INTERCEPTOR_ERROR_H
