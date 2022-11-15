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

#include "TypeCheck.h"

#include "Util.h"

#include <algorithm>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <functional>
#include <memory>
#include <numeric>
#include <utility>

namespace typeart {

std::string error_message_for(int mpierr) {
  int str_length;
  std::string mpierr_str;

  mpierr_str.resize(MPI_MAX_ERROR_STRING);
  MPI_Error_string(mpierr, mpierr_str.data(), &str_length);
  mpierr_str.resize(str_length);

  return mpierr_str;
}

Result<MPICombiner> MPICombiner::create(MPI_Datatype type) {
  MPICombiner result;
  int num_integers;
  int num_addresses;
  int num_datatypes;
  int combiner;
  auto mpierr = MPI_Type_get_envelope(type, &num_integers, &num_addresses, &num_datatypes, &combiner);

  if (mpierr != MPI_SUCCESS) {
    return make_internal_error<MPIError>("MPI_Type_get_envelope", error_message_for(mpierr));
  }

  result.id = combiner;

  if (combiner != MPI_COMBINER_NAMED) {
    result.integer_args.resize(num_integers);
    result.address_args.resize(num_addresses);
    std::vector<MPI_Datatype> type_args(num_datatypes);
    mpierr = MPI_Type_get_contents(type, num_integers, num_addresses, num_datatypes, result.integer_args.data(),
                                   result.address_args.data(), type_args.data());

    if (mpierr != MPI_SUCCESS) {
      return make_internal_error<MPIError>("MPI_Type_get_contents", error_message_for(mpierr));
    }

    result.type_args.reserve(num_datatypes);

    for (auto i = size_t{0}; i < num_datatypes; ++i) {
      auto type_arg = MPIType::create(type_args[i]);

      if (!type_arg) {
        return std::move(type_arg).error();
      }

      result.type_args.push_back(std::move(type_arg).value());
    }
  }

  return {result};
}

Result<MPIType> MPIType::create(MPI_Datatype type) {
  auto combiner = MPICombiner::create(type);
  if (combiner.has_error()) {
    return std::move(combiner).error();
  }
  return MPIType{type, *combiner};
}

struct Multipliers {
  size_t type;
  size_t buffer;
};

Result<void> check_type_and_count(const PointerInfo& pointer_info, const MPIType& type, int count);
Result<Multipliers> check_type(const PointerInfo& pointer_info, const MPIType& type);
Result<Multipliers> check_combiner_named(const PointerInfo& pointer_info, const MPIType& type);
Result<Multipliers> check_combiner_contiguous(const PointerInfo& pointer_info, const MPIType& type);
Result<Multipliers> check_combiner_vector(const PointerInfo& pointer_info, const MPIType& type);
Result<Multipliers> check_combiner_indexed_block(const PointerInfo& pointer_info, const MPIType& type);
Result<Multipliers> check_combiner_struct(const PointerInfo& pointer_info, const MPIType& type);
Result<Multipliers> check_combiner_subarray(const PointerInfo& pointer_info, const MPIType& type);

// For a given Buffer checks that the type of the buffer fits the MPI type
// `args.type` of this MPICall instance and that the buffer is large enough to
// hold `args.count` elements of the MPI type.
Result<void> check_buffer(const PointerInfo& pointer_info, const MPIType& type, int count) {
  auto stripped_pointer_info = pointer_info.resolveAllArrayTypes();
  auto result                = check_type_and_count(stripped_pointer_info, type, count);

  if (result.has_value() || result.error()->is<InternalError>()) {
    return result;
  }

  // If the type is a struct type and has a member with offset 0,
  // recursively check against the type of the first member.
  auto resolve_result = stripped_pointer_info.resolveStructureOrArrayType();
  if (resolve_result.has_error()) {
    return result;
  }
  std::vector<StructSubtypeMismatch> subtype_errors;
  auto outer_info = stripped_pointer_info;
  while (resolve_result.has_value()) {
    auto first_member_info = resolve_result.value().stripTypedefsAndQualifiers();
    auto subtype_result    = check_type_and_count(first_member_info, type, count);
    if (subtype_result.has_value() || subtype_result.error()->is<InternalError>()) {
      return subtype_result;
    }
    subtype_errors.push_back(
        StructSubtypeMismatch{outer_info, first_member_info,
                              std::make_unique<TypeError>(std::move(*subtype_result.error()).get<TypeError>())});
    outer_info     = first_member_info;
    resolve_result = first_member_info.resolveAllArrayTypes().resolveStructureOrArrayType();
  }

  auto primary_error = std::make_unique<TypeError>(std::move(*result.error()).get<TypeError>());
  return make_type_error<StructSubtypeErrors>(std::move(primary_error), std::move(subtype_errors));
}

Result<void> check_type_and_count(const PointerInfo& pointer_info, const MPIType& type, int count) {
  auto result = check_type(pointer_info, type);

  if (result.has_error()) {
    return std::move(result).error();
  }

  auto multipliers  = std::move(result).value();
  auto type_count   = static_cast<size_t>(count * multipliers.type);
  auto buffer_count = pointer_info.resolveAllArrayTypes().getCount() * multipliers.buffer;

  if (type_count > buffer_count) {
    return make_type_error<InsufficientBufferSize>(buffer_count, type_count);
  }

  return {};
}

// For a given Buffer and MPIType, checks that the buffer's type matches the
// MPI type.
// The resulting integer `type_count_multiplier` is the number of elements of
// the buffer's type required to represent one element of the MPI type
// (e.g. an MPI_Type_contiguous with a `count` of 4 and an `oldtype` of
// MPI_DOUBLE would require 4 double elements for each element of that type.)
// Similarly, `buffer_count_multiplier` is the number of elements of the MPI
// type needed to represent one element of the buffer's type. This is used to
// correctly handle MPI_BYTE, where for each given type T, sizeof(T) elements
// of MPI_BYTE are needed to represent one instance of T.
Result<Multipliers> check_type(const PointerInfo& pointer_info, const MPIType& type) {
  switch (type.combiner.id) {
    case MPI_COMBINER_NAMED:
      return check_combiner_named(pointer_info, type);
    case MPI_COMBINER_DUP:
      // MPI_Type_dup creates an exact duplicate of the type argument to the type
      // combiner, so we can delegate to a check against that type.
      return check_type(pointer_info, type.combiner.type_args[0]);
    case MPI_COMBINER_CONTIGUOUS:
      return check_combiner_contiguous(pointer_info, type);
    case MPI_COMBINER_VECTOR:
      return check_combiner_vector(pointer_info, type);
    case MPI_COMBINER_INDEXED_BLOCK:
      return check_combiner_indexed_block(pointer_info, type);
    case MPI_COMBINER_STRUCT:
      return check_combiner_struct(pointer_info, type);
    case MPI_COMBINER_SUBARRAY:
      return check_combiner_subarray(pointer_info, type);
    default:
      return make_internal_error<UnsupportedCombiner>(combiner_name_for(type.combiner.id));
  }
}

// See MPICall::check_type(const Buffer&, const MPIType&)
Result<Multipliers> check_combiner_named(const PointerInfo& pointer_info, const MPIType& type) {
  const auto mpi_type        = type.mpi_type;
  const auto basic_type_info = pointer_info.resolveAllArrayTypes();
  const auto type_size       = basic_type_info.getType().get_size_in_bits() / 8;
  // We assume MPI_BYTE to be the MPI equivalent of void*.
  if (type.mpi_type == MPI_BYTE) {
    return Multipliers{1, type_size};
  }
  const auto basic_type = meta::dyn_cast<meta::di::BasicType>(&basic_type_info.getType());
  if (basic_type == nullptr) {
    return make_type_error<BuiltinTypeMismatch>(pointer_info, type.mpi_type);
  }
  MPI_Count mpi_type_size;
  MPI_Type_size_x(mpi_type, &mpi_type_size);
  if (type_size != mpi_type_size) {
    return make_type_error<BuiltinTypeMismatch>(pointer_info, type.mpi_type);
  }
  const auto expected_encoding = std::unordered_map<MPI_Datatype, meta::di::Encoding>{
      {MPI_CHAR, std::is_signed_v<char> ? meta::di::Encoding::SignedChar : meta::di::Encoding::UnsignedChar},
      {MPI_SIGNED_CHAR, meta::di::Encoding::SignedChar},
      {MPI_UNSIGNED_CHAR, meta::di::Encoding::UnsignedChar},
      {MPI_SHORT, meta::di::Encoding::Signed},
      {MPI_UNSIGNED_SHORT, meta::di::Encoding::Unsigned},
      {MPI_INT, meta::di::Encoding::Signed},
      {MPI_UNSIGNED, meta::di::Encoding::Unsigned},
      {MPI_LONG, meta::di::Encoding::Signed},
      {MPI_UNSIGNED_LONG, meta::di::Encoding::Unsigned},
      {MPI_LONG_LONG, meta::di::Encoding::Signed},
      {MPI_UNSIGNED_LONG_LONG, meta::di::Encoding::Unsigned},
      {MPI_FLOAT, meta::di::Encoding::Float},
      {MPI_DOUBLE, meta::di::Encoding::Float},
      {MPI_LONG_DOUBLE, meta::di::Encoding::Float},
  };
  if (expected_encoding.at(mpi_type) == basic_type->get_encoding()) {
    return Multipliers{1, 1};
  } else {
    return make_type_error<BuiltinTypeMismatch>(pointer_info, type.mpi_type);
  }
}

// Type check for the type combiner:
// int MPI_Type_contiguous(int count, MPI_Datatype oldtype,
//     MPI_Datatype *newtype)
//
// See MPICall::check_type(const Buffer&, const MPIType&) for an explanation of
// the arguments and the return type.
Result<Multipliers> check_combiner_contiguous(const PointerInfo& pointer_info, const MPIType& type) {
  // MPI_Type_contiguous has one type argument and a count which denotes the
  // number of consecutive elements of the old type forming one element of the
  // conntiguous type. Therefore, we check that the old type matches the
  // buffer's type and multiply the count required for on element by the first
  // the first integer argument of the type combiner.
  auto count = type.combiner.integer_args[0];
  return check_type(pointer_info, type.combiner.type_args[0]).map([&](auto multipliers) {
    return Multipliers{multipliers.type * count, multipliers.buffer};
  });
}

// Type check for the type combiner:
// int MPI_Type_vector(int count, int blocklength, int stride,
//     MPI_Datatype oldtype, MPI_Datatype *newtype)
//
// See MPICall::check_type(const Buffer&, const MPIType&) for an explanation of
// the arguments and the return type.
Result<Multipliers> check_combiner_vector(const PointerInfo& pointer_info, const MPIType& type) {
  const auto count       = type.combiner.integer_args[0];
  const auto blocklength = type.combiner.integer_args[1];
  const auto stride      = type.combiner.integer_args[2];

  if (stride < 0) {
    return make_internal_error<UnsupportedCombinerArgs>(
        "negative strides for MPI_Type_vector are currently not supported");
  }

  // MPI_Type_vector forms a number of `count` blocks of `oldtype` where the
  // start of each consecutive block is `stride` elements of `oldtype` apart
  // and each block consists of `blocklength` elements of oldtype.
  // We therefore check the buffer's type against `oldtype` and multiply the
  // resulting count by `(count - 1) * stride + blocklength`.
  return check_type(pointer_info, type.combiner.type_args[0]).map([&](auto multipliers) {
    return Multipliers{multipliers.type * ((count - 1) * stride + blocklength), multipliers.buffer};
  });
}

// Type check for the type combiner:
// int MPI_Type_create_indexed_block(int count, int blocklength, const int
//     array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype)
//
// See MPICall::check_type(const Buffer&, const MPIType&) for an explanation of
// the arguments and the return type.
Result<Multipliers> check_combiner_indexed_block(const PointerInfo& pointer_info, const MPIType& type) {
  const auto count                  = type.combiner.integer_args[0];
  const auto blocklength            = type.combiner.integer_args[1];
  const auto array_of_displacements = type.combiner.integer_args.begin() + 2;
  const auto [min_displacement, max_displacement] =
      std::minmax_element(array_of_displacements, array_of_displacements + count);

  if (*min_displacement < 0) {
    return make_internal_error<UnsupportedCombinerArgs>(
        "negative displacements for MPI_Type_create_indexed_block are currently not supported");
  }

  // Similer to MPI_Type_vector but with a separate displacement specified for
  // each block.
  // We therefore check the buffer's type against `oldtype` and multiply the
  // resulting count by `max(array_of_displacements) + blocklength`.
  return check_type(pointer_info, type.combiner.type_args[0])
      .map([&, max_displacement = *max_displacement](auto multipliers) {
        return Multipliers{multipliers.type * (max_displacement + blocklength), multipliers.buffer};
      });
}

void collect_members(const meta::di::StructureType& current_struct, size_t inherited_offset,
                     std::vector<std::pair<const meta::di::Member*, size_t>>& members) {
  for (const auto& inheritance : current_struct.get_base_classes()) {
    collect_members(inheritance.get_base_structure_type(), inheritance.get_offset_in_bits() + inherited_offset,
                    members);
  }
  for (const auto& member : current_struct.get_direct_members()) {
    members.push_back(std::pair{&member, member.get_offset_in_bits() + inherited_offset});
  }
};

// Type check for the type combiner:
// int MPI_Type_create_struct(int count, int array_of_blocklengths[],
//     const MPI_Aint array_of_displacements[], const MPI_Datatype array_of_types[],
//     MPI_Datatype *newtype)
//
// See MPICall::check_type(const Buffer&, const MPIType&) for an explanation of
// the arguments and the return type.
Result<Multipliers> check_combiner_struct(const PointerInfo& pointer_info, const MPIType& type) {
  const auto count                 = type.combiner.integer_args[0];
  const auto array_of_blocklenghts = type.combiner.integer_args.begin() + 1;

  // First, check that the buffer's type is a struct type...
  const auto struct_type = meta::dyn_cast<meta::di::StructureType>(&pointer_info.getType());
  if (struct_type == nullptr) {
    return make_type_error<BufferNotOfStructType>(pointer_info);
  }

  std::vector<std::pair<const meta::di::Member*, size_t>> members;
  collect_members(*struct_type, 0, members);

  // ... and that the number of members of the struct matches the argument
  // `count` of the type combiner.
  if (members.size() != count) {
    return make_type_error<MemberCountMismatch>(pointer_info, members.size(), count);
  }

  // Then, for each member check that...
  for (size_t i = 0; i < members.size(); ++i) {
    // ... the byte offset of the member matches the respective element in
    // the `array_of_displacements` type combiner argument.
    if (members[i].second != 8 * type.combiner.address_args[i]) {
      return make_type_error<MemberOffsetMismatch>(pointer_info, members[i].first,
                                                   byte_offset::from_bits(members[i].second),
                                                   byte_offset::from_bytes(type.combiner.address_args[i]));
    }
  }

  for (size_t i = 0; i < members.size(); ++i) {
    auto member_info = pointer_info.findMember(byte_offset::from_bits(members[i].second)).value();

    // ... the type of the member matches the respective MPI type in the
    // `array_of_types` type combiner argument.
    auto result = check_type(member_info, type.combiner.type_args[i]);

    if (result.has_error()) {
      auto error = std::move(result).error();

      if (error->is<InternalError>()) {
        return std::move(error);
      }
      return make_type_error<MemberTypeMismatch>(pointer_info, members[i].first,
                                                 std::make_unique<TypeError>(std::move(*error).get<TypeError>()));
    }

    // ... the count of elements in the buffer of the member matches the count
    // required to represent `blocklength` elements of the MPI type.
    const auto multipliers  = std::move(result).value();
    const auto type_count   = static_cast<size_t>(array_of_blocklenghts[i]) * multipliers.type;
    const auto buffer_count = member_info.resolveAllArrayTypes().getCount() * multipliers.buffer;

    if (type_count != buffer_count) {
      return make_type_error<MemberElementCountMismatch>(pointer_info, members[i].first, buffer_count, type_count);
    }
  }

  return Multipliers{1, 1};
}

// Type check for the type combiner:
// int MPI_Type_create_subarray(int ndims, const int array_of_sizes[], const
//     int array_of_subsizes[], const int array_of_starts[], int order, MPI_Datatype
//     oldtype, MPI_Datatype *newtype)
//
// See MPICall::check_type(const Buffer&, const MPIType&) for an explanation of
// the arguments and the return type.
Result<Multipliers> check_combiner_subarray(const PointerInfo& pointer_info, const MPIType& type) {
  const auto ndims               = type.combiner.integer_args[0];
  const auto array_of_sizes      = type.combiner.integer_args.begin() + 1;
  const auto array_element_count = std::accumulate(array_of_sizes, array_of_sizes + ndims, 1, std::multiplies{});
  // As this type combiner specifies a subarray of a larger array, the buffer
  // must be large enough to hold that larger array. We therefore check the
  // buffer's type against `oldtype` and multiply the resulting count with
  // the product of all elements of the `array_of_sizes` (i.e. the element
  // count of the large n-dimensional array).
  return check_type(pointer_info, type.combiner.type_args[0]).map([&](auto multipliers) {
    return Multipliers{multipliers.type * array_element_count, multipliers.buffer};
  });
}

}  // namespace typeart
