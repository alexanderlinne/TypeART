#include "db/LLVMMetadataConverter.hpp"

#include "db/Database.hpp"

#include <fmt/core.h>
#include <llvm/IR/DebugInfo.h>

namespace typeart::meta {

template <class MetaClass>
Ref<MetaClass> LLVMMetadataConverter::lookup_meta(const llvm::Metadata* llvm_meta) {
  if (llvm_meta == nullptr) {
    return {};
  }
  if (auto it = llvm_to_meta.find(llvm_meta); it != llvm_to_meta.end()) {
    auto ref = dyn_cast<MetaClass>(it->second);
    if (ref == nullptr) {
      fmt::print(stderr, "Unexpected Meta type! actual: {} (id: {}) expected: {}, llvm actual: {}, llvm expected: {}\n",
                 (int)it->second->get_kind(), it->second->get_id().value(), (int)MetaClass::META_KIND,
                 it->first->getMetadataID(), llvm_meta->getMetadataID());
      abort();
    }
    return ref;
  }
  return {};
}

template <class MetaClass, class InitializerFn>
Ref<MetaClass> LLVMMetadataConverter::make_meta(const llvm::Metadata* llvm_meta, InitializerFn&& initializer_fn) {
  if (auto ref = lookup_meta<MetaClass>(llvm_meta); ref.is_valid()) {
    ref.set_weak(std::find(parents.begin(), parents.end(), llvm_meta) != parents.end());
    return ref;
  }
  auto meta = std::make_unique<MetaClass>();
  if (llvm_meta != nullptr) {
    parents.emplace_back(llvm_meta);
    llvm_to_meta[llvm_meta] = meta.get();
  }
  initializer_fn(meta);
  if (llvm_meta != nullptr) {
    parents.pop_back();
  }
  if (auto existing_meta = db->lookupMeta(meta.get()); existing_meta != nullptr) {
    auto casted = dyn_cast<MetaClass>(existing_meta);
    assert(casted != nullptr);
    if (llvm_meta != nullptr) {
      llvm_to_meta[llvm_meta] = casted;
    }
    return Ref{casted, casted->get_kind()};
  }
  return dyn_cast<MetaClass>(db->addMeta(std::move(meta)));
}

[[nodiscard]] Ref<StackAllocation> LLVMMetadataConverter::createStackAllocation(const llvm::DILocalVariable* di_local,
                                                                                const llvm::DILocation* di_location) {
  return make_meta<StackAllocation>(nullptr, [this, &di_local, &di_location](auto& result) {
    result->set_local_variable_raw(di_local != nullptr ? convertDILocalVariable(*di_local) : Ref<di::LocalVariable>{});
    result->set_location_raw(di_location != nullptr ? convertDILocation(*di_location) : Ref<di::Location>{});
  });
}

[[nodiscard]] Ref<HeapAllocation> LLVMMetadataConverter::createHeapAllocation(const llvm::DIType* di_type,
                                                                              const llvm::DILocation* di_location) {
  return make_meta<HeapAllocation>(nullptr, [this, &di_type, &di_location](auto& result) {
    result->set_type_raw(di_type != nullptr ? convertDIType(di_type) : Ref<di::Type>{});
    result->set_location_raw(di_location != nullptr ? convertDILocation(*di_location) : Ref<di::Location>{});
  });
}

[[nodiscard]] Ref<GlobalAllocation> LLVMMetadataConverter::createGlobalAllocation(
    const llvm::DIGlobalVariable* di_global) {
  return make_meta<GlobalAllocation>(nullptr, [this, &di_global](auto& result) {
    result->set_global_variable_raw(di_global != nullptr ? convertDIGlobalVariable(*di_global)
                                                         : Ref<di::GlobalVariable>{});
  });
}

[[nodiscard]] Ref<di::LocalVariable> LLVMMetadataConverter::convertDILocalVariable(
    const llvm::DILocalVariable& di_local) {
  return make_meta<di::LocalVariable>(&di_local, [this, &di_local](auto& result) {
    assert(di_local.getScope() != nullptr);
    assert(di_local.getType() != nullptr);
    auto name = convertString(di_local.getName());
    result->set_name_raw(name);
    result->set_linkage_name_raw(name);
    result->set_scope_raw(convertDIScope(*di_local.getScope()));
    result->set_file_raw(di_local.getFile() != nullptr ? convertDIFile(*di_local.getFile()) : nullptr);
    result->set_line(di_local.getLine());
    result->set_type_raw(convertDIType(di_local.getType()));
  });
}

[[nodiscard]] Ref<di::GlobalVariable> LLVMMetadataConverter::convertDIGlobalVariable(
    const llvm::DIGlobalVariable& di_global) {
  return make_meta<di::GlobalVariable>(&di_global, [this, &di_global](auto& result) {
    assert(di_global.getScope() != nullptr);
    assert(di_global.getFile() != nullptr);
    assert(di_global.getType() != nullptr);
    result->set_name_raw(convertString(di_global.getName()));
    result->set_linkage_name_raw(convertString(di_global.getLinkageName()));
    result->set_scope_raw(convertDIScope(*di_global.getScope()));
    result->set_file_raw(convertDIFile(*di_global.getFile()));
    result->set_line(di_global.getLine());
    result->set_type_raw(convertDIType(di_global.getType()));
    result->set_is_local(di_global.isLocalToUnit());
    result->set_is_definition(di_global.isDefinition());
  });
}

[[nodiscard]] Ref<di::Node> LLVMMetadataConverter::convertDINode(const llvm::DINode& di_node) {
  switch (di_node.getMetadataID()) {
    case llvm::Metadata::DIBasicTypeKind:
    case llvm::Metadata::DICompositeTypeKind:
    case llvm::Metadata::DIDerivedTypeKind:
    case llvm::Metadata::DISubroutineTypeKind:
    case llvm::Metadata::DISubprogramKind:
    case llvm::Metadata::DIFileKind:
    case llvm::Metadata::DICompileUnitKind:
    case llvm::Metadata::DINamespaceKind:
    case llvm::Metadata::DILexicalBlockKind:
    case llvm::Metadata::DILexicalBlockFileKind:
      return convertDIScope(*(const llvm::DIScope*)&di_node);
    case llvm::Metadata::DISubrangeKind:
      return convertDISubrange(*(const llvm::DISubrange*)&di_node);
    case llvm::Metadata::DIEnumeratorKind:
      return convertDIEnumerator(*(const llvm::DIEnumerator*)&di_node);
    default:
      fmt::print(stderr, "Unsupported di node kind {}\n", di_node.getMetadataID());
      abort();
  }
}

[[nodiscard]] Ref<di::Subrange> LLVMMetadataConverter::convertDISubrange(const llvm::DISubrange& di_subrange) {
  const auto& count = di_subrange.getCount();
  if (count.is<llvm::ConstantInt*>()) {
    return make_meta<di::Subrange>(&di_subrange, [&di_subrange](auto& result) {
      const auto& count         = di_subrange.getCount();
      const auto& integer_count = count.get<llvm::ConstantInt*>()->getZExtValue();
      result->set_count(integer_count);
      result->set_lower_bound(di_subrange.getLowerBound());
    });
  } else {
    fmt::print(stderr, "DISubranges with non-const count are currently not supported!\n");
    return make_meta<di::Subrange>(
        &di_subrange, [&di_subrange](auto& result) { result->set_lower_bound(di_subrange.getLowerBound()); });
  }
}

[[nodiscard]] Ref<di::Enumerator> LLVMMetadataConverter::convertDIEnumerator(const llvm::DIEnumerator& di_enum) {
  return make_meta<di::Enumerator>(&di_enum, [this, &di_enum](auto& result) {
    result->set_name_raw(convertString(di_enum.getName()));
    result->set_value(di_enum.getValue());
    result->set_is_unsigned(di_enum.isUnsigned());
  });
}

[[nodiscard]] Ref<di::Scope> LLVMMetadataConverter::convertDIScope(const llvm::DIScope& di_scope) {
  switch (di_scope.getMetadataID()) {
    case llvm::Metadata::DIBasicTypeKind:
    case llvm::Metadata::DICompositeTypeKind:
    case llvm::Metadata::DIDerivedTypeKind:
    case llvm::Metadata::DISubroutineTypeKind:
      return convertDIType((const llvm::DIType*)&di_scope);
    case llvm::Metadata::DISubprogramKind:
      return convertDISubprogram(*(const llvm::DISubprogram*)&di_scope);
    case llvm::Metadata::DIFileKind:
      return convertDIFile(*(const llvm::DIFile*)&di_scope);
    case llvm::Metadata::DICompileUnitKind:
      return convertDICompileUnit(*(const llvm::DICompileUnit*)&di_scope);
    case llvm::Metadata::DINamespaceKind:
      return convertDINamespace(*(const llvm::DINamespace*)&di_scope);
    case llvm::Metadata::DILexicalBlockKind:
      return convertDILexicalBlock(*(const llvm::DILexicalBlock*)&di_scope);
    case llvm::Metadata::DILexicalBlockFileKind:
      return convertDILexicalBlockFile(*(const llvm::DILexicalBlockFile*)&di_scope);
    default:
      fmt::print(stderr, "Unsupported metadata scope kind {}\n", di_scope.getMetadataID());
      abort();
  }
}

[[nodiscard]] Ref<di::File> LLVMMetadataConverter::convertDIFile(const llvm::DIFile& di_file) {
  return make_meta<di::File>(&di_file, [this, &di_file](auto& result) {
    result->set_filename_raw(convertString(di_file.getFilename()));
    result->set_directory_raw(convertString(di_file.getDirectory()));
  });
}

[[nodiscard]] Ref<di::Type> LLVMMetadataConverter::convertDIType(const llvm::DIType* di_type) {
  if (di_type == nullptr) {
    return {};
  }
  switch (di_type->getMetadataID()) {
    case llvm::Metadata::DIBasicTypeKind:
      return convertDIBasicType(*(const llvm::DIBasicType*)di_type);
    case llvm::Metadata::DICompositeTypeKind:
      return convertDICompositeType(*(const llvm::DICompositeType*)di_type);
    case llvm::Metadata::DIDerivedTypeKind:
      return convertDIDerivedType(*(const llvm::DIDerivedType*)di_type);
    case llvm::Metadata::DISubroutineTypeKind:
      return convertDISubroutineType(*(const llvm::DISubroutineType*)di_type);
    default:
      di_type->print(llvm::errs());
      llvm::errs() << "\n";
      fmt::print(stderr, "Unsupported metadata type kind {}\n", di_type->getMetadataID());
      abort();
  }
}

di::Encoding fromLLVMEncoding(unsigned int encoding) {
  switch (encoding) {
    case 0:
      return di::Encoding::Unspecified;
    case llvm::dwarf::DW_ATE_signed_char:
      return di::Encoding::SignedChar;
    case llvm::dwarf::DW_ATE_unsigned_char:
      return di::Encoding::UnsignedChar;
    case llvm::dwarf::DW_ATE_address:
      return di::Encoding::Address;
    case llvm::dwarf::DW_ATE_boolean:
      return di::Encoding::Boolean;
    case llvm::dwarf::DW_ATE_float:
      return di::Encoding::Float;
    case llvm::dwarf::DW_ATE_signed:
      return di::Encoding::Signed;
    case llvm::dwarf::DW_ATE_unsigned:
      return di::Encoding::Unsigned;
    default:
      fmt::print(stderr, "Unknown LLVM encoding value {}!\n", encoding);
      abort();
  }
}

[[nodiscard]] Ref<di::BasicType> LLVMMetadataConverter::convertDIBasicType(const llvm::DIBasicType& di_type) {
  return make_meta<di::BasicType>(&di_type, [this, &di_type](auto& result) {
    result->set_name_raw(convertString(di_type.getName()));
    result->set_size_in_bits(di_type.getSizeInBits());
    result->set_encoding(fromLLVMEncoding(di_type.getEncoding()));
  });
}

[[nodiscard]] Ref<di::CompositeType> LLVMMetadataConverter::convertDICompositeType(
    const llvm::DICompositeType& di_type) {
  return make_meta<di::CompositeType>(&di_type, [this, &di_type](auto& result) {
    result->set_name_raw(convertString(di_type.getName()));
    result->set_size_in_bits(di_type.getSizeInBits());
    result->set_identifier_raw(convertString(di_type.getIdentifier()));
    result->set_file_raw(di_type.getFile() != nullptr ? convertDIFile(*di_type.getFile()) : Ref<di::File>{});
    result->set_scope_raw(di_type.getScope() != nullptr ? convertDIScope(*di_type.getScope()) : Ref<di::Scope>{});
    result->set_line(di_type.getLine());
    auto refs = std::vector<Ref<Meta>>{};
    if (di_type.getRawElements() != nullptr) {
      const auto& di_elements = di_type.getElements();
      refs.reserve(di_elements.size());
      for (const auto& di_elem : di_elements) {
        refs.emplace_back(convertDINode(*di_elem));
      }
    }
    result->set_elements_raw(convertTuple(std::move(refs)));
  });
}

di::DerivedKind fromLLVMDerivedTypeTag(unsigned int tag) {
  switch (tag) {
    case llvm::dwarf::DW_TAG_pointer_type:
      return di::DerivedKind::Pointer;
    case llvm::dwarf::DW_TAG_member:
      return di::DerivedKind::Member;
    case llvm::dwarf::DW_TAG_reference_type:
      return di::DerivedKind::Reference;
    case llvm::dwarf::DW_TAG_typedef:
      return di::DerivedKind::Typedef;
    case llvm::dwarf::DW_TAG_const_type:
      return di::DerivedKind::Const;
    case llvm::dwarf::DW_TAG_inheritance:
      return di::DerivedKind::Inheritance;
    case llvm::dwarf::DW_TAG_restrict_type:
      return di::DerivedKind::Restrict;
    case llvm::dwarf::DW_TAG_rvalue_reference_type:
      return di::DerivedKind::RvalueReference;
    case llvm::dwarf::DW_TAG_volatile_type:
      return di::DerivedKind::Volatile;
    case llvm::dwarf::DW_TAG_ptr_to_member_type:
      return di::DerivedKind::PtrToMemberType;
    default:
      fmt::print(stderr, "Unknown LLVM derived type tag value {}!\n", tag);
      abort();
  }
}

[[nodiscard]] Ref<di::DerivedType> LLVMMetadataConverter::convertDIDerivedType(const llvm::DIDerivedType& di_type) {
  return make_meta<di::DerivedType>(&di_type, [this, &di_type](auto& result) {
    result->set_name_raw(convertString(di_type.getName()));
    result->set_size_in_bits(di_type.getSizeInBits());
    result->set_tag(fromLLVMDerivedTypeTag(di_type.getTag()));
    result->set_file_raw(di_type.getFile() != nullptr ? convertDIFile(*di_type.getFile()) : Ref<di::File>{});
    result->set_scope_raw(di_type.getScope() != nullptr ? convertDIScope(*di_type.getScope()) : Ref<di::Scope>{});
    result->set_line(di_type.getLine());
    result->set_base_type_raw(convertDIType(di_type.getBaseType()));
    result->set_offset_in_bits(di_type.getOffsetInBits());
  });
}

[[nodiscard]] Ref<di::SubroutineType> LLVMMetadataConverter::convertDISubroutineType(
    const llvm::DISubroutineType& di_type) {
  return make_meta<di::SubroutineType>(&di_type, [this, &di_type](auto& result) {
    const auto& di_types = di_type.getTypeArray();
    result->set_return_type_raw(convertDIType(*di_types.begin()));
    auto refs = std::vector<Ref<Meta>>{};
    refs.reserve(di_types.size());
    for (auto it = di_types.begin(); it != di_types.end(); ++it) {
      if (it == di_types.begin()) {
        continue;
      }
      refs.emplace_back(convertDIType(*it));
    }
    result->set_argument_types_raw(convertTuple(std::move(refs)));
  });
}

[[nodiscard]] Ref<di::LexicalBlock> LLVMMetadataConverter::convertDILexicalBlock(const llvm::DILexicalBlock& di_block) {
  return make_meta<di::LexicalBlock>(&di_block, [this, &di_block](auto& result) {
    assert(di_block.getFile() != nullptr);
    assert(di_block.getScope() != nullptr);
    result->set_file_raw(convertDIFile(*di_block.getFile()));
    result->set_scope_raw(convertDIScope(*di_block.getScope()));
    result->set_line(di_block.getLine());
    result->set_column(di_block.getColumn());
  });
}

[[nodiscard]] Ref<di::LexicalBlockFile> LLVMMetadataConverter::convertDILexicalBlockFile(
    const llvm::DILexicalBlockFile& di_block) {
  return make_meta<di::LexicalBlockFile>(&di_block, [this, &di_block](auto& result) {
    assert(di_block.getFile() != nullptr);
    assert(di_block.getScope() != nullptr);
    result->set_file_raw(convertDIFile(*di_block.getFile()));
    result->set_scope_raw(convertDIScope(*di_block.getScope()));
    result->set_discriminator(di_block.getDiscriminator());
  });
}

[[nodiscard]] Ref<di::Subprogram> LLVMMetadataConverter::convertDISubprogram(const llvm::DISubprogram& di_subprogram) {
  return make_meta<di::Subprogram>(&di_subprogram, [this, &di_subprogram](auto& result) {
    assert(di_subprogram.getFile() != nullptr);
    assert(di_subprogram.getScope() != nullptr);
    assert(di_subprogram.getType() != nullptr);
    result->set_name_raw(convertString(di_subprogram.getName()));
    result->set_linkage_name_raw(convertString(di_subprogram.getLinkageName()));
    result->set_file_raw(convertDIFile(*di_subprogram.getFile()));
    result->set_scope_raw(convertDIScope(*di_subprogram.getScope()));
    result->set_line(di_subprogram.getLine());
    result->set_type_raw(convertDISubroutineType(*di_subprogram.getType()));
  });
}

di::Language fromLLVMDILanguage(unsigned int language) {
  switch (language) {
    case llvm::dwarf::DW_LANG_C:
      return di::Language::C;
    case llvm::dwarf::DW_LANG_C89:
      return di::Language::C89;
    case llvm::dwarf::DW_LANG_C99:
      return di::Language::C99;
    case llvm::dwarf::DW_LANG_C11:
      return di::Language::C11;
    case llvm::dwarf::DW_LANG_C_plus_plus:
      return di::Language::Cpp;
    case llvm::dwarf::DW_LANG_C_plus_plus_03:
      return di::Language::Cpp03;
    case llvm::dwarf::DW_LANG_C_plus_plus_11:
      return di::Language::Cpp11;
    case llvm::dwarf::DW_LANG_C_plus_plus_14:
      return di::Language::Cpp14;
    default:
      fmt::print(stderr, "Unknown LLVM language value {}!\n", language);
      abort();
  }
}

[[nodiscard]] Ref<di::CompileUnit> LLVMMetadataConverter::convertDICompileUnit(const llvm::DICompileUnit& di_unit) {
  return make_meta<di::CompileUnit>(&di_unit, [this, &di_unit](auto& result) {
    assert(di_unit.getFile() != nullptr);
    result->set_language(fromLLVMDILanguage(di_unit.getSourceLanguage()));
    result->set_file_raw(convertDIFile(*di_unit.getFile()));
    result->set_producer_raw(convertString(di_unit.getProducer()));
    result->set_is_optimized(di_unit.isOptimized());
    result->set_runtime_version(di_unit.getRuntimeVersion());
  });
}

[[nodiscard]] Ref<di::Namespace> LLVMMetadataConverter::convertDINamespace(const llvm::DINamespace& di_namespace) {
  return make_meta<di::Namespace>(&di_namespace, [this, &di_namespace](auto& result) {
    result->set_name_raw(convertString(di_namespace.getName()));
    result->set_scope_raw(di_namespace.getScope() != nullptr ? convertDIScope(*di_namespace.getScope())
                                                             : Ref<di::Scope>{});
  });
}

[[nodiscard]] Ref<di::Location> LLVMMetadataConverter::convertDILocation(const llvm::DILocation& di_location) {
  return make_meta<di::Location>(&di_location, [this, &di_location](auto& result) {
    assert(di_location.getScope() != nullptr);
    result->set_line(di_location.getLine());
    result->set_column(di_location.getColumn());
    result->set_scope_raw(dyn_cast<di::LocalScope>(convertDIScope(*di_location.getScope())));
    assert(result->get_scope() != nullptr);
  });
}

[[nodiscard]] Ref<String> LLVMMetadataConverter::convertString(std::string str) {
  return make_meta<String>(nullptr, [str = std::move(str)](auto& result) { result->set_data(std::move(str)); });
}

[[nodiscard]] Ref<Tuple> LLVMMetadataConverter::convertTuple(std::vector<Ref<Meta>> refs) {
  return make_meta<Tuple>(nullptr, [refs = std::move(refs)](auto& result) { result->get_refs() = std::move(refs); });
}

}  // namespace typeart::meta
