#include "meta/Database.hpp"
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
struct ScalarTraits<meta::meta_id_t> {
  static void output(const meta::meta_id_t& value, void* p, llvm::raw_ostream& out) {
    ScalarTraits<meta::meta_id_t::value_type>::output(value.value(), p, out);
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, meta::meta_id_t& value) {
    meta::meta_id_t::value_type actual_value;
    auto result = ScalarTraits<meta::meta_id_t::value_type>::input(scalar, p, actual_value);
    value       = actual_value;
    return result;
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return ScalarTraits<meta::meta_id_t::value_type>::mustQuote(scalar);
  }
};

template <>
struct ScalarTraits<meta::Kind> {
  using Kind = meta::Kind;
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

struct MetaWrapper {
  meta::Meta* meta;
};

struct MetaInfoFile {
  size_t size;
  std::vector<std::unique_ptr<meta::Meta>>& meta;
};

template <>
struct ScalarTraits<MetaWrapper> {
  static void output(const MetaWrapper& wrapper, void* p, llvm::raw_ostream& out) {
    auto& meta = wrapper.meta;
    if (meta == nullptr) {
      out << "None";
    } else {
      ScalarTraits<meta::Kind>::output(meta->get_kind(), p, out);
      out << '#';
      ScalarTraits<meta::meta_id_t>::output(meta->get_id(), p, out);
    }
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, MetaWrapper& wrapper) {
    auto& meta = wrapper.meta;
    if (scalar == "None") {
      meta = nullptr;
      return {};
    }
    auto& ctx               = *(MetaInfoFile*)p;
    auto [kind_str, id_str] = scalar.split('#');
    meta::Kind kind;
    auto result = ScalarTraits<meta::Kind>::input(kind_str, p, kind);
    if (result != StringRef{}) {
      return result;
    }
    meta::meta_id_t id;
    result            = ScalarTraits<meta::meta_id_t>::input(id_str, p, id);
    auto& stored_meta = ctx.meta[id.value() - 1];
    if (stored_meta != nullptr) {
      meta = stored_meta.get();
    } else {
      auto new_meta = meta::make_meta(kind);
      new_meta->set_id(id);
      meta        = new_meta.get();
      stored_meta = std::move(new_meta);
    }
    return result;
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return scalar.endswith("?") ? QuotingType::Single : QuotingType::None;
  }
};

template <>
struct MappingTraits<std::unique_ptr<meta::Meta>> {
  static void mapping(IO& io, std::unique_ptr<meta::Meta>& value) {
    using namespace typeart;
    auto wrapper = MetaWrapper{value != nullptr ? value.get() : nullptr};
    io.mapRequired("self", wrapper);
    auto self = wrapper.meta;
    if (auto tuple = meta::dyn_cast<meta::TupleBase>(self)) {
      auto size = tuple->get_refs().size();
      io.mapRequired("size", size);
      tuple->get_refs().resize(size);
    }
    if (value != nullptr) {
      io.mapRequired("refs", self->get_refs());
    }
    if (auto integer = meta::dyn_cast<meta::Integer>(self)) {
      auto data = integer->get_data();
      io.mapRequired("data", data);
      integer->set_data(data);
    }
    if (auto string = meta::dyn_cast<meta::String>(self)) {
      auto data = string->get_data();
      io.mapRequired("data", data);
      string->set_data(std::move(data));
    }
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(std::unique_ptr<meta::Meta>)

template <>
struct SequenceTraits<std::vector<meta::Meta*>> {
  static size_t size(IO& io, std::vector<meta::Meta*>& list) {
    return list.size();
  }
  static MetaWrapper& element(IO& io, std::vector<meta::Meta*>& list, size_t index) {
    return *(MetaWrapper*)&list[index];
  }
  static const bool flow = true;
};

template <>
struct MappingTraits<MetaInfoFile> {
  static void mapping(IO& io, MetaInfoFile& value) {
    io.mapRequired("size", value.size);
    value.meta.resize(value.size);
    io.mapRequired("meta", value.meta);
  }
};

namespace meta {

std::optional<Database> Database::load(const std::string& file) {
  using namespace llvm;
  ErrorOr<std::unique_ptr<MemoryBuffer>> memBuffer = MemoryBuffer::getFile(file);

  if (std::error_code error = memBuffer.getError(); error) {
    // TODO meaningful error handling/message
    LOG_WARNING("Warning while loading type file to {}. Reason: {}", file, error.message());
    return {};
  }

  std::vector<std::unique_ptr<meta::Meta>> meta_info;
  auto meta_info_file = MetaInfoFile{0, meta_info};
  yaml::Input in(memBuffer.get()->getMemBufferRef(), &meta_info_file);
  in >> meta_info_file;

  Database db;
  if (!db.registerMeta(std::move(meta_info))) {
    fmt::print(stderr, "Couln't register meta information!\n");
    abort();
  }
  return db;
}

bool Database::store(const std::string& file) {
  using namespace llvm;

  std::error_code error;
  raw_fd_ostream oss(StringRef(file), error, compat::open_flag());

  if (oss.has_error()) {
    LOG_WARNING("Warning while storing type file to {}. Reason: {}", file, error.message());
    return false;
  }

  yaml::Output out(oss);
  auto meta_info_file = MetaInfoFile{meta_info.size(), meta_info};
  out << meta_info_file;
  return true;
}

}  // namespace meta
