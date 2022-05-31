#include "db/Database.hpp"
#include "support/Logger.hpp"
#include "support/System.hpp"

#include <fmt/core.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/YAMLParser.h>
#include <llvm/Support/YAMLTraits.h>
#include <sstream>

using namespace llvm::yaml;

namespace compat {
auto open_flag() {
#if LLVM_VERSION_MAJOR < 13
  return llvm::sys::fs::OpenFlags::F_Text;
#else
  return llvm::sys::fs::OpenFlags::OF_Text;
#endif
}
}  // namespace compat

template <>
struct ScalarTraits<std::optional<size_t>> {
  static void output(const std::optional<size_t>& value, void* p, llvm::raw_ostream& out) {
    if (value.has_value()) {
      ScalarTraits<size_t>::output(value.value(), p, out);
    } else {
      out << "None";
    }
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, std::optional<size_t>& value) {
    if (scalar != "None") {
      size_t actual_value;
      auto result = ScalarTraits<size_t>::input(scalar, p, actual_value);
      value       = {actual_value};
      return result;
    }
    return {};
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return QuotingType::None;
  }
};

template <>
struct ScalarTraits<typeart::alloc_id_t> {
  static void output(const typeart::alloc_id_t& value, void* p, llvm::raw_ostream& out) {
    ScalarTraits<typeart::alloc_id_t::value_type>::output(value.value(), p, out);
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, typeart::alloc_id_t& value) {
    typeart::alloc_id_t::value_type actual_value;
    auto result = ScalarTraits<typeart::alloc_id_t::value_type>::input(scalar, p, actual_value);
    value       = actual_value;
    return result;
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return ScalarTraits<typeart::alloc_id_t::value_type>::mustQuote(scalar);
  }
};

template <>
struct ScalarTraits<typeart::meta_id_t> {
  static void output(const typeart::meta_id_t& value, void* p, llvm::raw_ostream& out) {
    ScalarTraits<typeart::meta_id_t::value_type>::output(value.value(), p, out);
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, typeart::meta_id_t& value) {
    typeart::meta_id_t::value_type actual_value;
    auto result = ScalarTraits<typeart::meta_id_t::value_type>::input(scalar, p, actual_value);
    value       = actual_value;
    return result;
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return ScalarTraits<typeart::meta_id_t::value_type>::mustQuote(scalar);
  }
};

template <>
struct ScalarTraits<typeart::meta::Kind> {
  using Kind = typeart::meta::Kind;
  static void output(const Kind& value, void* p, llvm::raw_ostream& out) {
    std::ostringstream os;
    os << value;
    out << os.str();
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, Kind& value) {
    std::optional<Kind> input;
    std::istringstream is(scalar.str());
    is >> input;
    if (!input.has_value()) {
      return fmt::format("Unknown kind \"{}\"!\n", std::string{scalar});
    }
    value = input.value();
    return {};
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return QuotingType::None;
  }
};

template <class T>
struct ScalarTraits<typeart::meta::Ref<T>> {
  static void output(const typeart::meta::Ref<T>& ref, void* p, llvm::raw_ostream& out) {
    ScalarTraits<typeart::meta::Kind>::output(ref.get_kind(), p, out);
    out << '#';
    ScalarTraits<typeart::meta_id_t>::output(ref.get_id(), p, out);
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, typeart::meta::Ref<T>& value) {
    auto [kind_str, id_str] = scalar.split('#');
    typeart::meta::Kind kind;
    auto result = ScalarTraits<typeart::meta::Kind>::input(kind_str, p, kind);
    if (result != StringRef{}) {
      return result;
    }
    typeart::meta_id_t id;
    result = ScalarTraits<typeart::meta_id_t>::input(id_str, p, id);
    value  = typeart::meta::Ref<T>{id, kind};
    return result;
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return scalar.endswith("?") ? QuotingType::Single : QuotingType::None;
  }
};

struct TypeFile {
  std::vector<typeart::AllocationInfo>& allocation_info;
  std::vector<std::unique_ptr<typeart::meta::Meta>>& meta;
};

template <>
struct llvm::yaml::MappingTraits<TypeFile> {
  static void mapping(IO& io, TypeFile& info) {
    io.mapRequired("allocations", info.allocation_info);
    io.mapRequired("meta", info.meta);
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::AllocationInfo> {
  static void mapping(IO& io, typeart::AllocationInfo& info) {
    io.mapRequired("alloc_id", info.alloc_id);
    io.mapRequired("meta_id", info.meta_id);
    io.mapOptional("count", info.count);
  }
};

template <>
struct llvm::yaml::MappingTraits<std::unique_ptr<typeart::meta::Meta>> {
  static void mapping(IO& io, std::unique_ptr<typeart::meta::Meta>& value) {
    using namespace typeart;
    auto meta_id = value != nullptr ? value->get_id() : meta_id_t::invalid;
    auto kind    = value != nullptr ? value->get_kind() : meta::Kind::Unknown;
    io.mapRequired("meta_id", meta_id);
    io.mapRequired("kind", kind);
    if (kind == typeart::meta::Kind::Unknown) {
      return;
    }
    if (io.outputting()) {
      assert(value != nullptr);
      io.mapRequired("refs", value->get_refs());
    } else {
      assert(value == nullptr);
      std::vector<meta::Ref<meta::Meta>> refs;
      io.mapRequired("refs", refs);
      value = meta::make_meta(kind, std::move(refs));
      value->set_id(meta_id);
    }
    if (auto integer = meta::dyn_cast<meta::Integer>(value.get())) {
      auto data = integer->get_data();
      io.mapRequired("data", data);
      integer->set_data(data);
    }
    if (auto string = meta::dyn_cast<meta::String>(value.get())) {
      auto data = string->get_data();
      io.mapRequired("data", data);
      string->set_data(std::move(data));
    }
  }
};

template <>
struct SequenceTraits<std::vector<std::unique_ptr<typeart::meta::Meta>>> {
  static size_t size(IO& io, std::vector<std::unique_ptr<typeart::meta::Meta>>& list) {
    return std::count_if(list.begin(), list.end(), [](auto& e) { return !!e; });
  }
  static std::unique_ptr<typeart::meta::Meta>& element(IO& io, std::vector<std::unique_ptr<typeart::meta::Meta>>& list,
                                                       size_t index) {
    if (io.outputting()) {
      return *std::find_if(list.begin(), list.end(), [&index](auto& e) { return (index--) == 0; });
    } else {
      if (index >= list.size()) {
        list.resize(index + 1);
      }
      return list[index];
    }
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::AllocationInfo)
LLVM_YAML_IS_FLOW_SEQUENCE_VECTOR(typeart::meta::Ref<typeart::meta::Meta>)

namespace typeart {

std::optional<Database> Database::load(const std::string& file) {
  using namespace llvm;
  ErrorOr<std::unique_ptr<MemoryBuffer>> memBuffer = MemoryBuffer::getFile(file);

  if (std::error_code error = memBuffer.getError(); error) {
    // TODO meaningful error handling/message
    LOG_WARNING("Warning while loading type file to {}. Reason: {}", file, error.message());
    return {};
  }

  yaml::Input in(memBuffer.get()->getMemBufferRef());
  std::vector<AllocationInfo> allocation_info;
  std::vector<std::unique_ptr<typeart::meta::Meta>> meta_info;
  TypeFile type_file{allocation_info, meta_info};
  in >> type_file;

  Database db;
  db.registerAllocations(std::move(allocation_info));
  if (!db.registerMeta(std::move(meta_info))) {
    fmt::print(stderr, "Couln't register meta information!\n");
    abort();
  }
  return std::optional<Database>{std::move(db)};
}

bool Database::store(const std::string& file) {
  using namespace llvm;

  std::error_code error;
  raw_fd_ostream oss(StringRef(file), error, compat::open_flag());

  if (oss.has_error()) {
    LOG_WARNING("Warning while storing type file to {}. Reason: {}", file, error.message());
    return false;
  }

  auto type_file = TypeFile{allocation_info, meta_info};
  yaml::Output out(oss);
  out << type_file;
  return true;
}

}  // namespace typeart
