#include "db/Database.hpp"
#include "support/Logger.hpp"
#include "support/System.hpp"

#include <fmt/core.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/YAMLParser.h>
#include <llvm/Support/YAMLTraits.h>

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
struct ScalarTraits<typeart::type_id_t> {
  static void output(const typeart::type_id_t& value, void* p, llvm::raw_ostream& out) {
    ScalarTraits<typeart::type_id_t::value_type>::output(value.value(), p, out);
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, typeart::type_id_t& value) {
    typeart::type_id_t::value_type actual_value;
    auto result = ScalarTraits<typeart::type_id_t::value_type>::input(scalar, p, actual_value);
    value       = actual_value;
    return result;
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return ScalarTraits<typeart::type_id_t::value_type>::mustQuote(scalar);
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
struct ScalarTraits<typeart::meta::Kind> {
  using Kind = typeart::meta::Kind;
  static void output(const Kind& value, void* p, llvm::raw_ostream& out) {
    switch (value) {
      case Kind::Unknown:
        out << "Unknown";
        break;
      case Kind::CompileUnit:
        out << "CompileUnit";
        break;
      case Kind::File:
        out << "File";
        break;
      case Kind::Subprogram:
        out << "Subprogram";
        break;
      case Kind::Namespace:
        out << "Namespace";
        break;
      case Kind::BasicType:
        out << "BasicType";
        break;
      case Kind::CompositeType:
        out << "CompositeType";
        break;
      case Kind::DerivedType:
        out << "DerivedType";
        break;
      case Kind::SubroutineType:
        out << "SubroutineType";
        break;
      case Kind::Location:
        out << "Location";
        break;
      case Kind::LocalVariable:
        out << "LocalVariable";
        break;
      case Kind::GlobalVariable:
        out << "GlobalVariable";
        break;
      case Kind::HeapAllocation:
        out << "HeapAllocation";
        break;
      case Kind::StackAllocation:
        out << "StackAllocation";
        break;
      case Kind::GlobalAllocation:
        out << "GlobalAllocation";
        break;
      case Kind::Subrange:
        out << "Subrange";
        break;
      case Kind::LexicalBlock:
        out << "LexicalBlock";
        break;
      case Kind::LexicalBlockFile:
        out << "LexicalBlockFile";
        break;
      case Kind::Enumerator:
        out << "Enumerator";
        break;
      case Kind::String:
        out << "String";
        break;
      case Kind::Tuple:
        out << "Tuple";
        break;
      default:
        fmt::print(stderr, "Unknown Kind {}!\n", (int)value);
    }
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, Kind& value) {
    if (scalar == "Unknown") {
      value = Kind::Unknown;
    } else if (scalar == "CompileUnit") {
      value = Kind::CompileUnit;
    } else if (scalar == "File") {
      value = Kind::File;
    } else if (scalar == "Subprogram") {
      value = Kind::Subprogram;
    } else if (scalar == "Namespace") {
      value = Kind::Namespace;
    } else if (scalar == "BasicType") {
      value = Kind::BasicType;
    } else if (scalar == "CompositeType") {
      value = Kind::CompositeType;
    } else if (scalar == "DerivedType") {
      value = Kind::DerivedType;
    } else if (scalar == "SubroutineType") {
      value = Kind::SubroutineType;
    } else if (scalar == "Location") {
      value = Kind::Location;
    } else if (scalar == "LocalVariable") {
      value = Kind::LocalVariable;
    } else if (scalar == "GlobalVariable") {
      value = Kind::GlobalVariable;
    } else if (scalar == "HeapAllocation") {
      value = Kind::HeapAllocation;
    } else if (scalar == "StackAllocation") {
      value = Kind::StackAllocation;
    } else if (scalar == "GlobalAllocation") {
      value = Kind::GlobalAllocation;
    } else if (scalar == "Subrange") {
      value = Kind::Subrange;
    } else if (scalar == "LexicalBlock") {
      value = Kind::LexicalBlock;
    } else if (scalar == "LexicalBlockFile") {
      value = Kind::LexicalBlockFile;
    } else if (scalar == "Enumerator") {
      value = Kind::Enumerator;
    } else if (scalar == "String") {
      value = Kind::String;
    } else if (scalar == "Tuple") {
      value = Kind::Tuple;
    } else {
      return fmt::format("Unknown kind \"{}\"!\n", std::string{scalar});
    }
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
    if (ref.is_weak()) {
      out << '?';
    }
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, typeart::meta::Ref<T>& value) {
    bool is_weak = false;
    if (scalar.endswith("?")) {
      is_weak = true;
      scalar  = scalar.substr(0, scalar.size() - 1);
    }
    auto [kind_str, id_str] = scalar.split('#');
    typeart::meta::Kind kind;
    auto result = ScalarTraits<typeart::meta::Kind>::input(kind_str, p, kind);
    if (result != StringRef{}) {
      return result;
    }
    typeart::meta_id_t id;
    result = ScalarTraits<typeart::meta_id_t>::input(id_str, p, id);
    value  = typeart::meta::Ref<T>{id, kind};
    value.set_weak(is_weak);
    return result;
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return scalar.endswith("?") ? QuotingType::Single : QuotingType::None;
  }
};

template <>
struct ScalarTraits<typeart::meta::di::Encoding> {
  static void output(const typeart::meta::di::Encoding& value, void* p, llvm::raw_ostream& out) {
    switch (value) {
      case typeart::meta::di::Encoding::Unspecified:
        out << "Unspecified";
        break;
      case typeart::meta::di::Encoding::Address:
        out << "Address";
        break;
      case typeart::meta::di::Encoding::Boolean:
        out << "Boolean";
        break;
      case typeart::meta::di::Encoding::Float:
        out << "Float";
        break;
      case typeart::meta::di::Encoding::Signed:
        out << "Signed";
        break;
      case typeart::meta::di::Encoding::Unsigned:
        out << "Unsigned";
        break;
      case typeart::meta::di::Encoding::SignedChar:
        out << "SignedChar";
        break;
      case typeart::meta::di::Encoding::UnsignedChar:
        out << "UnsignedChar";
        break;
      default:
        fmt::print(stderr, "Unknown Encoding {}!\n", (int)value);
    }
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, typeart::meta::di::Encoding& value) {
    if (scalar == "Unspecified") {
      value = typeart::meta::di::Encoding::Unspecified;
    } else if (scalar == "Address") {
      value = typeart::meta::di::Encoding::Address;
    } else if (scalar == "Boolean") {
      value = typeart::meta::di::Encoding::Boolean;
    } else if (scalar == "Float") {
      value = typeart::meta::di::Encoding::Float;
    } else if (scalar == "Signed") {
      value = typeart::meta::di::Encoding::Signed;
    } else if (scalar == "Unsigned") {
      value = typeart::meta::di::Encoding::Unsigned;
    } else if (scalar == "SignedChar") {
      value = typeart::meta::di::Encoding::SignedChar;
    } else if (scalar == "UnsignedChar") {
      value = typeart::meta::di::Encoding::UnsignedChar;
    } else {
      return fmt::format("Unknown encoding \"{}\"!\n", std::string{scalar});
    }
    return {};
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return QuotingType::None;
  }
};

template <>
struct ScalarTraits<typeart::meta::di::CompositeKind> {
  static void output(const typeart::meta::di::CompositeKind& value, void* p, llvm::raw_ostream& out) {
    switch (value) {
      case typeart::meta::di::CompositeKind::Structure:
        out << "Structure";
        break;
      case typeart::meta::di::CompositeKind::Class:
        out << "Class";
        break;
      case typeart::meta::di::CompositeKind::Union:
        out << "Union";
        break;
      default:
        fmt::print(stderr, "Unknown CompositeKind {}!\n", (int)value);
    }
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, typeart::meta::di::CompositeKind& value) {
    if (scalar == "Structure") {
      value = typeart::meta::di::CompositeKind::Structure;
    } else if (scalar == "Class") {
      value = typeart::meta::di::CompositeKind::Class;
    } else if (scalar == "Union") {
      value = typeart::meta::di::CompositeKind::Union;
    } else {
      return fmt::format("Unknown composite kind \"{}\"!\n", std::string{scalar});
    }
    return {};
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return QuotingType::None;
  }
};

template <>
struct ScalarTraits<typeart::meta::di::DerivedKind> {
  static void output(const typeart::meta::di::DerivedKind& value, void* p, llvm::raw_ostream& out) {
    switch (value) {
      case typeart::meta::di::DerivedKind::Member:
        out << "Member";
        break;
      case typeart::meta::di::DerivedKind::Typedef:
        out << "Typedef";
        break;
      case typeart::meta::di::DerivedKind::Pointer:
        out << "Pointer";
        break;
      case typeart::meta::di::DerivedKind::Reference:
        out << "Reference";
        break;
      case typeart::meta::di::DerivedKind::RvalueReference:
        out << "RvalueReference";
        break;
      case typeart::meta::di::DerivedKind::Const:
        out << "Const";
        break;
      case typeart::meta::di::DerivedKind::Inheritance:
        out << "Inheritance";
        break;
      case typeart::meta::di::DerivedKind::Restrict:
        out << "Restrict";
        break;
      case typeart::meta::di::DerivedKind::Volatile:
        out << "Volatile";
        break;
      case typeart::meta::di::DerivedKind::PtrToMemberType:
        out << "PtrToMemberType";
        break;
      default:
        fmt::print(stderr, "Unknown DerivedKind {}!\n", (int)value);
    }
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, typeart::meta::di::DerivedKind& value) {
    if (scalar == "Member") {
      value = typeart::meta::di::DerivedKind::Member;
    } else if (scalar == "Typedef") {
      value = typeart::meta::di::DerivedKind::Typedef;
    } else if (scalar == "Pointer") {
      value = typeart::meta::di::DerivedKind::Pointer;
    } else if (scalar == "Reference") {
      value = typeart::meta::di::DerivedKind::Reference;
    } else if (scalar == "RvalueReference") {
      value = typeart::meta::di::DerivedKind::RvalueReference;
    } else if (scalar == "Const") {
      value = typeart::meta::di::DerivedKind::Const;
    } else if (scalar == "Inheritance") {
      value = typeart::meta::di::DerivedKind::Inheritance;
    } else if (scalar == "Restrict") {
      value = typeart::meta::di::DerivedKind::Restrict;
    } else if (scalar == "Volatile") {
      value = typeart::meta::di::DerivedKind::Volatile;
    } else if (scalar == "PtrToMemberType") {
      value = typeart::meta::di::DerivedKind::PtrToMemberType;
    } else {
      return fmt::format("Unknown derived kind \"{}\"!\n", std::string{scalar});
    }
    return {};
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return QuotingType::None;
  }
};

template <>
struct ScalarTraits<typeart::meta::di::Language> {
  static void output(const typeart::meta::di::Language& value, void* p, llvm::raw_ostream& out) {
    switch (value) {
      case typeart::meta::di::Language::C:
        out << "C";
        break;
      case typeart::meta::di::Language::C89:
        out << "C89";
        break;
      case typeart::meta::di::Language::C99:
        out << "C99";
        break;
      case typeart::meta::di::Language::C11:
        out << "C11";
        break;
      case typeart::meta::di::Language::Cpp:
        out << "Cpp";
        break;
      case typeart::meta::di::Language::Cpp03:
        out << "Cpp03";
        break;
      case typeart::meta::di::Language::Cpp11:
        out << "Cpp11";
        break;
      case typeart::meta::di::Language::Cpp14:
        out << "Cpp14";
        break;
      default:
        fmt::print(stderr, "Unknown Language {}!\n", (int)value);
    }
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, typeart::meta::di::Language& value) {
    if (scalar == "C") {
      value = typeart::meta::di::Language::C;
    } else if (scalar == "C89") {
      value = typeart::meta::di::Language::C89;
    } else if (scalar == "C99") {
      value = typeart::meta::di::Language::C99;
    } else if (scalar == "C11") {
      value = typeart::meta::di::Language::C11;
    } else if (scalar == "Cpp") {
      value = typeart::meta::di::Language::Cpp;
    } else if (scalar == "Cpp03") {
      value = typeart::meta::di::Language::Cpp03;
    } else if (scalar == "Cpp11") {
      value = typeart::meta::di::Language::Cpp11;
    } else if (scalar == "Cpp14") {
      value = typeart::meta::di::Language::Cpp14;
    } else {
      return fmt::format("Unknown language \"{}\"!\n", std::string{scalar});
    }
    return {};
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return QuotingType::None;
  }
};

struct TypeFile {
  std::vector<typeart::AllocationInfo> allocation_info;
  std::vector<typeart::StructType> struct_info;

  std::vector<typeart::meta::di::CompileUnit> di_compile_units;
  std::vector<typeart::meta::di::File> di_files;
  std::vector<typeart::meta::di::Subprogram> di_subprograms;
  std::vector<typeart::meta::di::Namespace> di_namespaces;
  std::vector<typeart::meta::di::BasicType> di_basic_types;
  std::vector<typeart::meta::di::CompositeType> di_composite_types;
  std::vector<typeart::meta::di::DerivedType> di_derived_types;
  std::vector<typeart::meta::di::SubroutineType> di_subroutine_types;
  std::vector<typeart::meta::di::Location> di_locations;
  std::vector<typeart::meta::di::LocalVariable> di_local_variables;
  std::vector<typeart::meta::di::GlobalVariable> di_global_variables;
  std::vector<typeart::meta::HeapAllocation> heap_allocations;
  std::vector<typeart::meta::StackAllocation> stack_allocations;
  std::vector<typeart::meta::GlobalAllocation> global_allocations;
  std::vector<typeart::meta::di::Subrange> subranges;
  std::vector<typeart::meta::di::LexicalBlock> lexical_blocks;
  std::vector<typeart::meta::di::LexicalBlockFile> lexical_block_files;
  std::vector<typeart::meta::di::Enumerator> enumerators;
  std::vector<typeart::meta::String> strings;
  std::vector<typeart::meta::Tuple> tuples;
};

template <>
struct llvm::yaml::MappingTraits<TypeFile> {
  static void mapping(IO& io, TypeFile& info) {
    io.mapRequired("allocations", info.allocation_info);
    io.mapRequired("types", info.struct_info);

    io.mapRequired("compile_units", info.di_compile_units);
    io.mapRequired("files", info.di_files);
    io.mapRequired("subprograms", info.di_subprograms);
    io.mapRequired("namespaces", info.di_namespaces);
    io.mapRequired("basic_types", info.di_basic_types);
    io.mapRequired("composite_types", info.di_composite_types);
    io.mapRequired("derived_types", info.di_derived_types);
    io.mapRequired("subroutine_types", info.di_subroutine_types);
    io.mapRequired("locations", info.di_locations);
    io.mapRequired("local_variables", info.di_local_variables);
    io.mapRequired("global_variables", info.di_global_variables);
    io.mapRequired("heap_allocations", info.heap_allocations);
    io.mapRequired("stack_allocations", info.stack_allocations);
    io.mapRequired("global_allocations", info.global_allocations);
    io.mapRequired("subranges", info.subranges);
    io.mapRequired("lexical_blocks", info.lexical_blocks);
    io.mapRequired("lexical_block_files", info.lexical_block_files);
    io.mapRequired("enumerators", info.enumerators);
    io.mapRequired("strings", info.strings);
    io.mapRequired("tuples", info.tuples);
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::AllocationInfo> {
  static void mapping(IO& io, typeart::AllocationInfo& info) {
    io.mapRequired("alloc_id", info.alloc_id);
    io.mapRequired("type_id", info.type_id);
    io.mapRequired("meta_id", info.meta_id);
    io.mapOptional("count", info.count);
  }
};

template <>
struct llvm::yaml::ScalarTraits<typeart::StructTypeFlag> {
  static void output(const typeart::StructTypeFlag& value, void*, llvm::raw_ostream& out) {
    out << static_cast<int>(value);
  }

  static StringRef input(StringRef scalar, void*, typeart::StructTypeFlag& value) {
    int flag{0};
    const auto result = scalar.getAsInteger(0, flag);
    if (result) {
      // Error result, assume user_defined:
      value = typeart::StructTypeFlag::USER_DEFINED;
    } else {
      value = static_cast<typeart::StructTypeFlag>(flag);
    }
    return StringRef();
  }

  // Determine if this scalar needs quotes.
  static QuotingType mustQuote(StringRef) {
    return QuotingType::None;
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::StructType> {
  static void mapping(IO& io, typeart::StructType& info) {
    io.mapRequired("id", info.type_id);
    io.mapRequired("name", info.name);
    io.mapRequired("extent", info.extent);
    io.mapRequired("member_count", info.num_members);
    io.mapRequired("offsets", info.offsets);
    io.mapRequired("types", info.member_types);
    io.mapRequired("sizes", info.array_sizes);
    io.mapRequired("flags", info.flag);
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::CompileUnit> {
  static void mapping(IO& io, typeart::meta::di::CompileUnit& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("language", value.get_language());
    io.mapRequired("is_optimized", value.get_is_optimized());
    io.mapRequired("runtime_version", value.get_runtime_version());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::File> {
  static void mapping(IO& io, typeart::meta::di::File& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::Subprogram> {
  static void mapping(IO& io, typeart::meta::di::Subprogram& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("line", value.get_line());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::Namespace> {
  static void mapping(IO& io, typeart::meta::di::Namespace& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::BasicType> {
  static void mapping(IO& io, typeart::meta::di::BasicType& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("size_in_bits", value.get_size_in_bits());
    io.mapRequired("encoding", value.get_encoding());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::CompositeType> {
  static void mapping(IO& io, typeart::meta::di::CompositeType& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("size_in_bits", value.get_size_in_bits());
    io.mapRequired("line", value.get_line());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::DerivedType> {
  static void mapping(IO& io, typeart::meta::di::DerivedType& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("size_in_bits", value.get_size_in_bits());
    io.mapRequired("tag", value.get_tag());
    io.mapRequired("line", value.get_line());
    io.mapRequired("offset_in_bits", value.get_offset_in_bits());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::SubroutineType> {
  static void mapping(IO& io, typeart::meta::di::SubroutineType& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::Location> {
  static void mapping(IO& io, typeart::meta::di::Location& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("line", value.get_line());
    io.mapRequired("column", value.get_column());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::LocalVariable> {
  static void mapping(IO& io, typeart::meta::di::LocalVariable& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("line", value.get_line());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::GlobalVariable> {
  static void mapping(IO& io, typeart::meta::di::GlobalVariable& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("line", value.get_line());
    io.mapRequired("is_local", value.get_is_local());
    io.mapRequired("is_definition", value.get_is_definition());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::HeapAllocation> {
  static void mapping(IO& io, typeart::meta::HeapAllocation& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::StackAllocation> {
  static void mapping(IO& io, typeart::meta::StackAllocation& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::GlobalAllocation> {
  static void mapping(IO& io, typeart::meta::GlobalAllocation& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::Subrange> {
  static void mapping(IO& io, typeart::meta::di::Subrange& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("count", value.get_count());
    io.mapRequired("lower_bound", value.get_lower_bound());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::LexicalBlock> {
  static void mapping(IO& io, typeart::meta::di::LexicalBlock& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("line", value.get_line());
    io.mapRequired("column", value.get_column());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::LexicalBlockFile> {
  static void mapping(IO& io, typeart::meta::di::LexicalBlockFile& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("discriminator", value.get_discriminator());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::di::Enumerator> {
  static void mapping(IO& io, typeart::meta::di::Enumerator& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("value", value.get_value());
    io.mapRequired("is_unsigned", value.get_is_unsigned());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::String> {
  static void mapping(IO& io, typeart::meta::String& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
    io.mapRequired("data", value.get_data());
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::meta::Tuple> {
  static void mapping(IO& io, typeart::meta::Tuple& value) {
    io.mapRequired("meta_id", value.get_id());
    io.mapRequired("refs", value.get_refs());
  }
};

LLVM_YAML_IS_FLOW_SEQUENCE_VECTOR(typeart::type_id_t)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::AllocationInfo)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::StructType)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::CompileUnit)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::File)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::Subprogram)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::Namespace)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::BasicType)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::CompositeType)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::DerivedType)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::SubroutineType)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::Location)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::LocalVariable)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::GlobalVariable)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::HeapAllocation)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::StackAllocation)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::GlobalAllocation)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::Subrange)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::LexicalBlock)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::LexicalBlockFile)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::di::Enumerator)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::String)
LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::meta::Tuple)
LLVM_YAML_IS_FLOW_SEQUENCE_VECTOR(typeart::meta::Ref<typeart::meta::Meta>)

namespace typeart {

void registerData(Database& db, TypeFile& type_file) {
  for (const auto& typeInfo : type_file.struct_info) {
    db.registerStruct(typeInfo);
  }
  db.registerAllocations(std::move(type_file.allocation_info));

  auto meta = std::vector<std::unique_ptr<typeart::meta::Meta>>{};
  std::transform(type_file.di_compile_units.begin(), type_file.di_compile_units.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::CompileUnit>(std::move(e)); });
  std::transform(type_file.di_files.begin(), type_file.di_files.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::File>(std::move(e)); });
  std::transform(type_file.di_subprograms.begin(), type_file.di_subprograms.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::Subprogram>(std::move(e)); });
  std::transform(type_file.di_namespaces.begin(), type_file.di_namespaces.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::Namespace>(std::move(e)); });
  std::transform(type_file.di_basic_types.begin(), type_file.di_basic_types.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::BasicType>(std::move(e)); });
  std::transform(type_file.di_composite_types.begin(), type_file.di_composite_types.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::CompositeType>(std::move(e)); });
  std::transform(type_file.di_derived_types.begin(), type_file.di_derived_types.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::DerivedType>(std::move(e)); });
  std::transform(type_file.di_subroutine_types.begin(), type_file.di_subroutine_types.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::SubroutineType>(std::move(e)); });
  std::transform(type_file.di_locations.begin(), type_file.di_locations.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::Location>(std::move(e)); });
  std::transform(type_file.di_local_variables.begin(), type_file.di_local_variables.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::LocalVariable>(std::move(e)); });
  std::transform(type_file.di_global_variables.begin(), type_file.di_global_variables.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::GlobalVariable>(std::move(e)); });
  std::transform(type_file.heap_allocations.begin(), type_file.heap_allocations.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::HeapAllocation>(std::move(e)); });
  std::transform(type_file.stack_allocations.begin(), type_file.stack_allocations.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::StackAllocation>(std::move(e)); });
  std::transform(type_file.global_allocations.begin(), type_file.global_allocations.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::GlobalAllocation>(std::move(e)); });
  std::transform(type_file.subranges.begin(), type_file.subranges.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::Subrange>(std::move(e)); });
  std::transform(type_file.lexical_blocks.begin(), type_file.lexical_blocks.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::LexicalBlock>(std::move(e)); });
  std::transform(type_file.lexical_block_files.begin(), type_file.lexical_block_files.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::LexicalBlockFile>(std::move(e)); });
  std::transform(type_file.enumerators.begin(), type_file.enumerators.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::di::Enumerator>(std::move(e)); });
  std::transform(type_file.strings.begin(), type_file.strings.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::String>(std::move(e)); });
  std::transform(type_file.tuples.begin(), type_file.tuples.end(), std::back_inserter(meta),
                 [](auto&& e) { return std::make_unique<typeart::meta::Tuple>(std::move(e)); });
  if (!db.registerMeta(std::move(meta))) {
    fmt::print(stderr, "Couln't register meta information!\n");
    abort();
  }
}

std::optional<Database> Database::load(const std::string& file) {
  using namespace llvm;
  ErrorOr<std::unique_ptr<MemoryBuffer>> memBuffer = MemoryBuffer::getFile(file);

  if (std::error_code error = memBuffer.getError(); error) {
    // TODO meaningful error handling/message
    LOG_WARNING("Warning while loading type file to " << file << ". Reason: " << error.message());
    return {};
  }

  yaml::Input in(memBuffer.get()->getMemBufferRef());
  TypeFile type_file;
  in >> type_file;

  Database db;
  registerData(db, type_file);
  return db;
}

void sortMetaInfoInto(const std::vector<std::unique_ptr<typeart::meta::Meta>>& meta_info, TypeFile& type_file) {
  for (auto&& meta : meta_info) {
    if (!meta) {
      continue;
    }
    switch (meta->get_kind()) {
      case typeart::meta::Kind::CompileUnit:
        type_file.di_compile_units.push_back(*(typeart::meta::di::CompileUnit*)meta.get());
        break;
      case typeart::meta::Kind::File:
        type_file.di_files.push_back(*(typeart::meta::di::File*)meta.get());
        break;
      case typeart::meta::Kind::Subprogram:
        type_file.di_subprograms.push_back(*(typeart::meta::di::Subprogram*)meta.get());
        break;
      case typeart::meta::Kind::Namespace:
        type_file.di_namespaces.push_back(*(typeart::meta::di::Namespace*)meta.get());
        break;
      case typeart::meta::Kind::BasicType:
        type_file.di_basic_types.push_back(*(typeart::meta::di::BasicType*)meta.get());
        break;
      case typeart::meta::Kind::CompositeType:
        type_file.di_composite_types.push_back(*(typeart::meta::di::CompositeType*)meta.get());
        break;
      case typeart::meta::Kind::DerivedType:
        type_file.di_derived_types.push_back(*(typeart::meta::di::DerivedType*)meta.get());
        break;
      case typeart::meta::Kind::SubroutineType:
        type_file.di_subroutine_types.push_back(*(typeart::meta::di::SubroutineType*)meta.get());
        break;
      case typeart::meta::Kind::Location:
        type_file.di_locations.push_back(*(typeart::meta::di::Location*)meta.get());
        break;
      case typeart::meta::Kind::LocalVariable:
        type_file.di_local_variables.push_back(*(typeart::meta::di::LocalVariable*)meta.get());
        break;
      case typeart::meta::Kind::GlobalVariable:
        type_file.di_global_variables.push_back(*(typeart::meta::di::GlobalVariable*)meta.get());
        break;
      case typeart::meta::Kind::HeapAllocation:
        type_file.heap_allocations.push_back(*(typeart::meta::HeapAllocation*)meta.get());
        break;
      case typeart::meta::Kind::StackAllocation:
        type_file.stack_allocations.push_back(*(typeart::meta::StackAllocation*)meta.get());
        break;
      case typeart::meta::Kind::GlobalAllocation:
        type_file.global_allocations.push_back(*(typeart::meta::GlobalAllocation*)meta.get());
        break;
      case typeart::meta::Kind::Subrange:
        type_file.subranges.push_back(*(typeart::meta::di::Subrange*)meta.get());
        break;
      case typeart::meta::Kind::LexicalBlock:
        type_file.lexical_blocks.push_back(*(typeart::meta::di::LexicalBlock*)meta.get());
        break;
      case typeart::meta::Kind::LexicalBlockFile:
        type_file.lexical_block_files.push_back(*(typeart::meta::di::LexicalBlockFile*)meta.get());
        break;
      case typeart::meta::Kind::Enumerator:
        type_file.enumerators.push_back(*(typeart::meta::di::Enumerator*)meta.get());
        break;
      case typeart::meta::Kind::String:
        type_file.strings.push_back(*(typeart::meta::String*)meta.get());
        break;
      case typeart::meta::Kind::Tuple:
        type_file.tuples.push_back(*(typeart::meta::Tuple*)meta.get());
        break;
      default:
        fmt::print(stderr, "Unknown meta kind {}!\n", meta->get_kind());
        abort();
    }
  }
}

bool Database::store(const std::string& file) const {
  using namespace llvm;

  std::error_code error;
  raw_fd_ostream oss(StringRef(file), error, compat::open_flag());

  if (oss.has_error()) {
    LOG_WARNING("Warning while storing type file to " << file << ". Reason: " << error.message());
    return false;
  }

  auto type_file = TypeFile{};
  type_file.struct_info.reserve(struct_types.size());
  std::copy_if(struct_types.begin(), struct_types.end(), std::back_inserter(type_file.struct_info),
               [](auto& info) { return info.isValid(); });
  type_file.allocation_info = allocation_info;
  sortMetaInfoInto(meta_info, type_file);

  yaml::Output out(oss);
  out << type_file;
  return true;
}

}  // namespace typeart
