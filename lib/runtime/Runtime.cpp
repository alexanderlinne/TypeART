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

#include "runtime/Runtime.hpp"

#include "AccessCountPrinter.h"
#include "db/Database.hpp"
#include "runtime/AccessCounter.hpp"
#include "runtime/tracker/Tracker.hpp"
#include "support/Logger.hpp"
#include "support/System.hpp"

#ifdef TYPEART_USE_ALLOCATOR
#include "allocator/Allocator.hpp"
#endif

#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace typeart {

std::ostream& operator<<(std::ostream& os, const Status& status) {
  switch (status) {
    case Status::OK:
      os << "OK";
      break;
    case Status::UNKNOWN_ADDRESS:
      os << "UNKNOWN_ADDRESS";
      break;
    case Status::BAD_ALIGNMENT:
      os << "BAD_ALIGNMENT";
      break;
    case Status::OFFSET_OUT_OF_RANGE:
      os << "OFFSET_OUT_OF_RANGE";
      break;
    case Status::BAD_OFFSET:
      os << "BAD_OFFSET";
      break;
    case Status::WRONG_KIND:
      os << "WRONG_KIND";
      break;
    case Status::INVALID_ALLOC_ID:
      os << "INVALID_ALLOC_ID";
      break;
    case Status::INVALID_META_ID:
      os << "INVALID_META_ID";
      break;
    case Status::UNSUPPORTED_TYPE:
      os << "UNSUPPORTED_TYPE";
      break;
    case Status::ERROR:
      os << "ERROR";
      break;
  }
  return os;
}

}  // namespace typeart

namespace typeart::runtime {

byte_offset byte_offset::zero = {0};

static constexpr const char* defaultTypeFileName = "types.yaml";

class Runtime {
  // scope must be set to true before all other members are initialized.
  // This is achieved by adding this struct as the first member.
  struct Initializer {
    Initializer() {
      scope = 1;
    }

    void reset() const {
      scope = initial_scope;
    }

   private:
    size_t initial_scope = scope;
  };

  Initializer init;
  Database db{};
  Recorder recorder{};

 public:
  static thread_local size_t scope;

  static Runtime& get() {
    // As opposed to a global variable, a singleton + instantiation during
    // the first callback/query avoids some problems when
    // preloading (especially with MUST).
    static Runtime instance;
    return instance;
  }

  static Recorder& getRecorder() {
    return Runtime::get().recorder;
  }

  static Database& getDatabase() {
    return Runtime::get().db;
  }

 public:
  Runtime() : init() {
    LOG_TRACE("TypeART Runtime Trace");
    LOG_TRACE("*********************");

    auto loadTypes = [this](const std::string& file, std::error_code& ec) -> bool {
      auto database = Database::load(file);
      if (database.has_value()) {
        db = std::move(database).value();
        return true;
      }
      return false;
    };

    std::error_code error;
    // Try to load types from specified file first.
    // Then look at default location.
    const char* type_file = std::getenv("TYPEART_TYPE_FILE");
    if (type_file == nullptr || strcmp(type_file, "") == 0) {
      // FIXME Deprecated name
      type_file = std::getenv("TA_TYPE_FILE");
      if (type_file != nullptr) {
        LOG_WARNING("Use of deprecated env var TA_TYPE_FILE.");
      }
    }
    if (type_file != nullptr && strcmp(type_file, "") != 0) {
      if (!loadTypes(type_file, error)) {
        LOG_FATAL("Failed to load recorded types from TYPEART_TYPE_FILE={}. Reason: {}", type_file, error.message());
        std::exit(EXIT_FAILURE);  // TODO: Error handling
      }
    } else {
      if (!loadTypes(defaultTypeFileName, error)) {
        LOG_WARNING(
            "No type file with default name \"{}\" in current directory. Using default built-in types only. To specify "
            "a different file, edit TYPEART_TYPE_FILE environment variable. Reason: {}",
            defaultTypeFileName, error.message());
      }
    }

    init.reset();
  }

  ~Runtime() {
    scope = 1;
    std::ostringstream stream;
    softcounter::serialize(recorder, stream);
    if (!stream.str().empty()) {
      // llvm::errs/LOG will crash with virtual call error
      std::cerr << stream.str();
    }
  }
};

thread_local size_t Runtime::scope = 0;

PointerInfo::PointerInfo(pointer base_addr, const meta::Allocation& allocation, const meta::di::Type& type,
                         size_t count)
    : base_addr(base_addr), allocation(&allocation), type(&type), count(count) {
}

cpp::result<PointerInfo, Status> PointerInfo::get(pointer addr) {
  LOG_TRACE("PointerInfo::get with address {}", addr);
  auto guard = ScopeGuard{};
  getRecorder().incUsedInRequest(addr);
#ifdef TYPEART_USE_ALLOCATOR
  auto pointer_info = allocator::getPointerInfo(addr);
#else
  auto pointer_info_opt = tracker::Tracker::get().getPointerInfo(addr);
#endif
  if (!pointer_info_opt.has_value()) {
    LOG_TRACE("Address {} is unknown", addr);
    getRecorder().incAddrMissing(addr);
    return cpp::fail(Status::UNKNOWN_ADDRESS);
  }
  const auto pointer_info = std::move(pointer_info_opt).value();
  LOG_TRACE("Found pointer info {}", pointer_info);
  if (addr == pointer_info.base_addr) {
    return pointer_info;
  }
  return pointer_info.resolveSubtype(addr);
}

bool PointerInfo::contains(pointer p) const {
  return p >= base_addr && (base_addr + byte_size::from_bits(count * type->get_size_in_bits())) > p;
}

// Returns this pointer info with it's type canonicalized.
PointerInfo PointerInfo::getCanonicalized() const& {
  return PointerInfo{base_addr, *allocation, type->get_canonical_type(), count};
}

// Returns this pointer info with it's type canonicalized.
PointerInfo PointerInfo::getCanonicalized() && {
  type = &type->get_canonical_type();
  return std::move(*this);
}

cpp::result<PointerInfo, Status> PointerInfo::resolveStructureOrArrayType() const {
  if (const auto structure_type = meta::dyn_cast<meta::di::StructureType>(type)) {
    const auto first_member = structure_type->find_member(0);
    if (auto array_type = meta::dyn_cast<meta::di::ArrayType>(&first_member->get_type())) {
      return PointerInfo{base_addr, *allocation, first_member->get_type(), array_type->get_flattened_count()};
    } else {
      return PointerInfo{base_addr, *allocation, first_member->get_type(), 1};
    }
  } else if (const auto array_type = meta::dyn_cast<meta::di::ArrayType>(type)) {
    return PointerInfo{base_addr, *allocation, array_type->get_base_type(), array_type->get_flattened_count()};
  } else {
    return cpp::fail(Status::WRONG_KIND);
  }
}

cpp::result<PointerInfo::Subrange, Status> PointerInfo::getSubrange(pointer addr) const {
  // Check for exact match -> no further checks and offsets calculations needed
  if (base_addr == addr) {
    return Subrange{addr, 0, byte_offset::zero, count};
  }

  // Ensure that the given address is in bounds
  const auto type_size = byte_size::from_bits(type->get_size_in_bits());
  if (!contains(addr)) {
    const auto offset2base = addr - base_addr;
    const auto oob_index   = (offset2base.value() / type_size.value()) - count + 1;
    LOG_WARNING("Lookup for addr {} with pointer info {} was {} elements out of bounds!", addr, *this, oob_index);
    return cpp::fail(Status::UNKNOWN_ADDRESS);
  }

  assert(addr >= base_addr && "Error in base address computation");
  const auto offset = addr - base_addr;

  // Offset of the pointer w.r.t. the first subrange element
  const auto internal_offset = offset % type_size;

  // First index of the array
  const auto lower_bound    = static_cast<size_t>(offset / type_size);
  const auto subrange_count = count - lower_bound;

  // Retrieve and return type information
  return Subrange{addr - internal_offset, lower_bound, internal_offset, subrange_count};
}

cpp::result<PointerInfo, Status> PointerInfo::resolveSubtype(pointer addr) const {
  LOG_TRACE("Resolving subtype of kind {} on pointer info {}", type->get_kind(), *this);
  auto subrange_result = getSubrange(addr);
  if (subrange_result.has_error()) {
    LOG_TRACE("Subrange construction failed with error {}", subrange_result.error());
    return cpp::fail(subrange_result.error());
  }
  const auto subrange = std::move(subrange_result).value();
  if (subrange.offset == byte_offset::zero) {
    return PointerInfo{subrange.base_addr, *allocation, *type, subrange.count};
  }
  const auto& canonical_type = type->get_canonical_type();
  assert(type != nullptr);
  return meta::di::visit_type(
      canonical_type,
      make_lambda_visitor<cpp::result<PointerInfo, Status>>(
          [](const meta::di::VoidType&) { return cpp::fail(Status::UNSUPPORTED_TYPE); },
          [](const meta::di::BasicType&) {
            LOG_TRACE("BasicType not properly aligned!");
            return cpp::fail(Status::BAD_ALIGNMENT);
          },
          [](const meta::di::DerivedType&) { return cpp::fail(Status::BAD_ALIGNMENT); },
          [](const meta::di::EnumerationType&) { return cpp::fail(Status::BAD_ALIGNMENT); },
          [&](const meta::di::StructureType& structure_type) -> cpp::result<PointerInfo, Status> {
            auto member_info_result = StructMemberInfo::get(subrange.base_addr, subrange.offset, structure_type);
            if (member_info_result.has_error()) {
              const auto error = member_info_result.error();
              if (error == Status::OFFSET_OUT_OF_RANGE) {
                LOG_FATAL("Offset in the {} in pointer info {} was out of bounds for struct type {} unexpectedly",
                          subrange, *this, structure_type.get_pretty_name());
                abort();
              } else if (error == Status::BAD_OFFSET) {
                // A pointer to padding bytes within the struct is a pointer with a bad alignment to the struct
                return cpp::fail(Status::BAD_ALIGNMENT);
              } else {
                LOG_FATAL("StructMemberInfo::get returned unexpected error {}", error);
              }
            }
            const auto member_info = std::move(member_info_result.value());
            return member_info.intoPointerInfo(*this).value().resolveSubtype(addr);
          },
          [&](const meta::di::UnionType&) {
            LOG_FATAL("Unions cannot currently be type-checked correctly!");
            return cpp::fail(Status::UNSUPPORTED_TYPE);
          },
          [&](const meta::di::ArrayType& array_type) -> cpp::result<PointerInfo, Status> {
            auto array_info_result = ArrayElementInfo::get(subrange.base_addr, subrange.offset, array_type);
            if (array_info_result.has_error()) {
              return cpp::fail(array_info_result.error());
            }
            const auto array_info = std::move(array_info_result).value();
            return array_info.intoPointerInfo(*this).value().resolveSubtype(addr);
          },
          [](const meta::di::SubroutineType&) {
            LOG_FATAL("Unexpected SubroutineType while resolving address!");
            return cpp::fail(Status::UNSUPPORTED_TYPE);
          }));
}

std::ostream& operator<<(std::ostream& os, const PointerInfo& pointer_info) {
  auto const& alloc = pointer_info.getAllocation();
  auto const& file  = alloc.get_file();
  os << fmt::format("{} of type [{} x {}] allocated at {}/{}:{}", pointer_info.getBaseAddr(), pointer_info.getCount(),
                    pointer_info.getType().get_pretty_name(), file.get_directory(), file.get_filename(),
                    alloc.get_line());
  return os;
}

std::ostream& operator<<(std::ostream& os, const PointerInfo::Subrange& subrange) {
  os << fmt::format("subrange at {} (index {}) with offset {} and {} elements", subrange.base_addr, subrange.base_idx,
                    subrange.offset, subrange.count);
  return os;
}

cpp::result<StructMemberInfo, Status> StructMemberInfo::get(pointer base_addr, byte_offset offset,
                                                            const meta::di::StructureType& type) {
  LOG_TRACE("StructInfoMember::get for type {} with offset {}", type.get_pretty_name(), offset);
  if (offset >= byte_offset::from_bits(type.get_size_in_bits())) {
    LOG_TRACE("StructInfoMember::get offset larger than size of the struct");
    return cpp::fail(Status::OFFSET_OUT_OF_RANGE);
  }

  // Get index of the struct member at the address
  const auto member = type.find_member(offset.as_bits());
  if (member == nullptr) {
    LOG_TRACE("StructInfoMember::get offset points to padding bytes");
    return cpp::fail(Status::BAD_OFFSET);
  }
  LOG_TRACE("Found member {} with type {}", (const void*)member, member->get_type().get_pretty_name());
  const auto member_offset = byte_offset::from_bits(member->get_offset_in_bits());

  return StructMemberInfo{base_addr + member_offset, member, offset - member_offset};
}

std::optional<PointerInfo> StructMemberInfo::intoPointerInfo(const PointerInfo& original) const {
  if (!original.contains(base_addr)) {
    return {};
  }
  return PointerInfo{base_addr, original.getAllocation(), member->get_type(), 1};
}

cpp::result<ArrayElementInfo, Status> ArrayElementInfo::get(pointer base_addr, byte_offset offset,
                                                            const meta::di::ArrayType& type) {
  if (offset >= byte_offset::from_bits(type.get_size_in_bits())) {
    return cpp::fail(Status::BAD_OFFSET);
  }
  const auto type_size      = byte_size::from_bits(type.get_base_type().get_size_in_bits());
  const auto index          = static_cast<size_t>(offset / type_size);
  const auto type_offset    = offset % type_size;
  const auto element_offset = offset - type_offset;
  return ArrayElementInfo{base_addr + element_offset, &type, index, type_offset};
}

std::optional<PointerInfo> ArrayElementInfo::intoPointerInfo(const PointerInfo& original) const {
  if (!original.contains(base_addr)) {
    return {};
  }
  return PointerInfo{base_addr, original.getAllocation(), type->get_base_type(), type->get_flattened_count() - index};
}

ScopeGuard::ScopeGuard() {
  Runtime::scope += 1;
}

ScopeGuard::~ScopeGuard() {
  Runtime::scope -= 1;
}

bool ScopeGuard::shouldTrack() const {
  return Runtime::scope <= 1;
}

Recorder& getRecorder() {
  return Runtime::getRecorder();
}

const meta::Meta* getMeta(meta_id_t meta_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().getMetaInfo(meta_id);
}

const AllocationInfo* getAllocationInfo(alloc_id_t alloc_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().getAllocationInfo(alloc_id);
}

}  // namespace typeart::runtime
