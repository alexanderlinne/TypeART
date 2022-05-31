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

#include "Util.h"

#include <cstdint>
#include <cstdio>

namespace typeart {

const char* error_message_for(Status status) {
  switch (status) {
    case Status::OK:
      return "No errors";
    case Status::UNKNOWN_ADDRESS:
      return "Buffer not registered";
    case Status::BAD_ALIGNMENT:
      return "Buffer access is not aligned correctly";
    case Status::OFFSET_OUT_OF_RANGE:
      return "Error in offset computation";
    case Status::BAD_OFFSET:
      return "Error in offset computation";
    case Status::WRONG_KIND:
      return "Wrong type kind";
    case Status::INVALID_ALLOC_ID:
      return "Invalid allocation ID";
    case Status::INVALID_META_ID:
      return "Invalid metadata ID";
    case Status::UNSUPPORTED_TYPE:
      return "Type is not supported by TypeART";
    default:
      return "Invalid error code";
  }
}

const char* combiner_name_for(int combiner) {
  switch (combiner) {
    case MPI_COMBINER_NAMED:
      return "MPI predefined datatype";
    case MPI_COMBINER_DUP:
      return "MPI_Type_dup";
    case MPI_COMBINER_CONTIGUOUS:
      return "MPI_Type_contiguous";
    case MPI_COMBINER_VECTOR:
      return "MPI_Type_vector";
    case MPI_COMBINER_HVECTOR:
      return "MPI_Type_hvector";
    case MPI_COMBINER_INDEXED:
      return "MPI_Type_indexed";
    case MPI_COMBINER_HINDEXED:
      return "MPI_Type_hindexed";
    case MPI_COMBINER_INDEXED_BLOCK:
      return "MPI_Type_create_indexed_block";
    case MPI_COMBINER_STRUCT:
      return "MPI_Type_struct";
    case MPI_COMBINER_SUBARRAY:
      return "MPI_Type_create_subarray";
    case MPI_COMBINER_DARRAY:
      return "MPI_Type_create_darray";
    case MPI_COMBINER_F90_REAL:
      return "MPI_Type_create_f90_real";
    case MPI_COMBINER_F90_COMPLEX:
      return "MPI_Type_create_f90_complex";
    case MPI_COMBINER_F90_INTEGER:
      return "MPI_Type_create_f90_integer";
    case MPI_COMBINER_RESIZED:
      return "MPI_Type_create_resized";
    default:
      return "invalid combiner id";
  }
}

}  // namespace typeart
