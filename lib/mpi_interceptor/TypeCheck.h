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

#ifndef TYPEART_MPI_INTERCEPTOR_TYPE_CHECK_H
#define TYPEART_MPI_INTERCEPTOR_TYPE_CHECK_H

#include "Error.h"
#include "System.h"
#include "Util.h"
#include "runtime/RuntimeInterface.h"

#include <atomic>
#include <cstddef>
#include <cstdio>
#include <mpi.h>
#include <optional>
#include <string>
#include <vector>

namespace typeart {

struct MPIType;

struct Buffer {
  ptrdiff_t offset;
  const void* ptr;
  size_t count;
  type_id_t::value_type type_id;

 public:
  static Result<Buffer> create(const void* ptr);
  static Buffer create(ptrdiff_t offset, const void* ptr, size_t count, type_id_t::value_type type_id);
};

struct MPICombiner {
  int id;
  std::vector<int> integer_args;
  std::vector<MPI_Aint> address_args;
  std::vector<MPIType> type_args;

 public:
  static Result<MPICombiner> create(MPI_Datatype type);
};

struct MPIType {
  MPI_Datatype mpi_type;
  type_id_t::value_type type_id;
  MPICombiner combiner;

 public:
  static Result<MPIType> create(MPI_Datatype type);
};

Result<void> check_buffer(const Buffer& buffer, const MPIType& type, int count);

}  // namespace typeart

#endif  // TYPEART_MPI_INTERCEPTOR_TYPE_CHECK_H
