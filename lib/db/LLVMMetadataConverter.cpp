#include "db/LLVMMetadataConverter.hpp"

#include "db/Database.hpp"
#include "support/Logger.hpp"

#include <fmt/core.h>
#include <llvm/IR/DebugInfo.h>

namespace typeart::meta {

template <class MetaClass>
MetaClass* LLVMMetadataConverter::lookup_meta(const llvm::Metadata& llvm_meta) {
  if (auto it = llvm_to_meta.find(&llvm_meta); it != llvm_to_meta.end()) {
    auto meta = dyn_cast<MetaClass>(it->second);
    if (meta == nullptr) {
      LOG_FATAL("Unexpected Meta type! actual: {} (id: {}) expected: {}, llvm actual: {}, llvm expected: {}\n",
                (int)it->second->get_kind(), it->second->get_id().value(), (int)MetaClass::META_KIND,
                it->first->getMetadataID(), llvm_meta.getMetadataID());
      abort();
    }
    return meta;
  }
  return {};
}

template <class MetaClass, class InitializerFn>
Ref<MetaClass> LLVMMetadataConverter::make_meta(const llvm::Metadata& llvm_meta, InitializerFn&& initializer_fn) {
  if (auto meta = lookup_meta<MetaClass>(llvm_meta); meta != nullptr) {
    auto ref = Ref{*meta};
    return std::find(parents.begin(), parents.end(), &llvm_meta) != parents.end() ? ref.as_weak_ref() : ref;
  }
  auto meta = std::make_unique<MetaClass>();
  parents.emplace_back(&llvm_meta);
  llvm_to_meta[&llvm_meta] = meta.get();
  initializer_fn(meta);
  parents.pop_back();
  auto result              = db->addMeta(std::move(meta));
  llvm_to_meta[&llvm_meta] = result;
  return {*dyn_cast<MetaClass>(result)};
}

template <class MetaClass, class InitializerFn>
Ref<MetaClass> LLVMMetadataConverter::make_meta(InitializerFn&& initializer_fn) {
  auto meta = std::make_unique<MetaClass>();
  initializer_fn(meta);
  return {*dyn_cast<MetaClass>(db->addMeta(std::move(meta)))};
}

[[nodiscard]] Ref<StackAllocation> LLVMMetadataConverter::createStackAllocation(const llvm::DILocalVariable& di_local,
                                                                                const llvm::DILocation& di_location) {
  return make_meta<StackAllocation>([this, &di_local, &di_location](auto& result) {
    result->set_local_variable_raw(convertDILocalVariable(di_local));
    result->set_location_raw(convertDILocation(di_location));
    result->set_retained(true);
  });
}

[[nodiscard]] Ref<HeapAllocation> LLVMMetadataConverter::createHeapAllocation(const llvm::DIType& di_type,
                                                                              const llvm::DILocation& di_location) {
  return make_meta<HeapAllocation>([this, &di_type, &di_location](auto& result) {
    result->set_type_raw(convertDIType(&di_type));
    result->set_location_raw(convertDILocation(di_location));
    result->set_retained(true);
  });
}

[[nodiscard]] Ref<GlobalAllocation> LLVMMetadataConverter::createGlobalAllocation(
    const llvm::DIGlobalVariable& di_global) {
  return make_meta<GlobalAllocation>([this, &di_global](auto& result) {
    result->set_global_variable_raw(convertDIGlobalVariable(di_global));
    result->set_retained(true);
  });
}

[[nodiscard]] Ref<di::LocalVariable> LLVMMetadataConverter::convertDILocalVariable(
    const llvm::DILocalVariable& di_local) {
  return make_meta<di::LocalVariable>(di_local, [this, &di_local](auto& result) {
    auto name = convertString(di_local.getName());
    result->set_name_raw(name);
    result->set_linkage_name_raw(name);
    result->set_scope_raw(convertDIScope(di_local.getScope()));
    result->set_file_raw(convertDIFile(di_local.getFile()));
    result->set_line_raw(convertInteger(di_local.getLine()));
    result->set_type_raw(convertDIType(di_local.getType()));
  });
}

[[nodiscard]] Ref<di::GlobalVariable> LLVMMetadataConverter::convertDIGlobalVariable(
    const llvm::DIGlobalVariable& di_global) {
  return make_meta<di::GlobalVariable>(di_global, [this, &di_global](auto& result) {
    result->set_name_raw(convertString(di_global.getName()));
    result->set_linkage_name_raw(convertString(di_global.getLinkageName()));
    result->set_scope_raw(convertDIScope(di_global.getScope()));
    result->set_file_raw(convertDIFile(di_global.getFile()));
    result->set_line_raw(convertInteger(di_global.getLine()));
    result->set_type_raw(convertDIType(di_global.getType()));
    result->set_is_local_raw(convertInteger(di_global.isLocalToUnit()));
    result->set_is_definition_raw(convertInteger(di_global.isDefinition()));
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
      return convertDIScope((const llvm::DIScope*)&di_node);
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
    return make_meta<di::Subrange>(di_subrange, [this, &di_subrange](auto& result) {
      const auto& count         = di_subrange.getCount();
      const auto& integer_count = count.get<llvm::ConstantInt*>()->getZExtValue();
      result->set_count_raw(convertInteger(integer_count));
      result->set_lower_bound_raw(convertInteger(di_subrange.getLowerBound()));
    });
  } else {
    fmt::print(stderr, "DISubranges with non-const count are currently not supported!\n");
    return make_meta<di::Subrange>(di_subrange, [this, &di_subrange](auto& result) {
      result->set_lower_bound_raw(convertInteger(di_subrange.getLowerBound()));
    });
  }
}

[[nodiscard]] Ref<di::Enumerator> LLVMMetadataConverter::convertDIEnumerator(const llvm::DIEnumerator& di_enum) {
  return make_meta<di::Enumerator>(di_enum, [this, &di_enum](auto& result) {
    result->set_name_raw(convertString(di_enum.getName()));
    result->set_value_raw(convertInteger(di_enum.getValue()));
    result->set_is_unsigned_raw(convertInteger(di_enum.isUnsigned()));
  });
}

[[nodiscard]] Ref<di::Scope> LLVMMetadataConverter::convertDIScope(const llvm::DIScope* di_scope) {
  if (di_scope == nullptr) {
    return make_meta<di::GlobalOrBuiltin>([](auto&) {});
  }
  switch (di_scope->getMetadataID()) {
    case llvm::Metadata::DIBasicTypeKind:
    case llvm::Metadata::DICompositeTypeKind:
    case llvm::Metadata::DIDerivedTypeKind:
    case llvm::Metadata::DISubroutineTypeKind:
      return convertDIType((const llvm::DIType*)di_scope);
    case llvm::Metadata::DISubprogramKind:
      return convertDISubprogram(*(const llvm::DISubprogram*)di_scope);
    case llvm::Metadata::DIFileKind:
      return convertDIFile((const llvm::DIFile*)di_scope);
    case llvm::Metadata::DICompileUnitKind:
      return convertDICompileUnit(*(const llvm::DICompileUnit*)di_scope);
    case llvm::Metadata::DINamespaceKind:
      return convertDINamespace(*(const llvm::DINamespace*)di_scope);
    case llvm::Metadata::DILexicalBlockKind:
      return convertDILexicalBlock(*(const llvm::DILexicalBlock*)di_scope);
    case llvm::Metadata::DILexicalBlockFileKind:
      return convertDILexicalBlockFile(*(const llvm::DILexicalBlockFile*)di_scope);
    default:
      fmt::print(stderr, "Unsupported metadata scope kind {}\n", di_scope->getMetadataID());
      abort();
  }
}

[[nodiscard]] Ref<di::File> LLVMMetadataConverter::convertDIFile(const llvm::DIFile* di_file) {
  if (di_file == nullptr) {
    return make_meta<di::File>([this](auto& result) {
      auto str = convertString("?");
      result->set_filename_raw(str);
      result->set_directory_raw(str);
    });
  }
  return make_meta<di::File>(*di_file, [this, &di_file](auto& result) {
    result->set_filename_raw(convertString(di_file->getFilename()));
    result->set_directory_raw(convertString(di_file->getDirectory()));
  });
}

[[nodiscard]] Ref<di::Type> LLVMMetadataConverter::convertDIType(const llvm::DIType* di_type) {
  if (di_type == nullptr) {
    return make_meta<di::VoidType>([](auto&) {});
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
  return make_meta<di::BasicType>(di_type, [this, &di_type](auto& result) {
    result->set_name_raw(convertString(di_type.getName()));
    result->set_size_in_bits_raw(convertInteger(di_type.getSizeInBits()));
    result->set_encoding_raw(convertInteger((int)fromLLVMEncoding(di_type.getEncoding())));
  });
}

[[nodiscard]] Ref<di::Type> LLVMMetadataConverter::convertDICompositeType(const llvm::DICompositeType& di_type) {
  auto build_result = [this, &di_type](auto& result) {
    result->set_name_raw(convertString(di_type.getName()));
    result->set_size_in_bits_raw(convertInteger(di_type.getSizeInBits()));
    result->set_identifier_raw(convertString(di_type.getIdentifier()));
    result->set_file_raw(convertDIFile(di_type.getFile()));
    result->set_scope_raw(convertDIScope(di_type.getScope()));
    result->set_line_raw(convertInteger(di_type.getLine()));
    auto refs = std::vector<Ref<Meta>>{};
    if (di_type.getRawElements() != nullptr) {
      const auto& di_elements = di_type.getElements();
      refs.reserve(di_elements.size());
      for (const auto& di_elem : di_elements) {
        refs.emplace_back(convertDINode(*di_elem));
      }
    }
    result->set_elements_raw(convertTuple(std::move(refs)));
  };
  if (di_type.getTag() == llvm::dwarf::DW_TAG_class_type || di_type.getTag() == llvm::dwarf::DW_TAG_structure_type) {
    return convertStructureOrClassType(di_type);
  } else if (di_type.getTag() == llvm::dwarf::DW_TAG_union_type) {
    return make_meta<di::UnionType>(di_type, build_result);
  } else if (di_type.getTag() == llvm::dwarf::DW_TAG_enumeration_type) {
    return make_meta<di::EnumerationType>(di_type, build_result);
  } else if (di_type.getTag() == llvm::dwarf::DW_TAG_array_type) {
    return convertArrayType(di_type);
  } else {
    LOG_FATAL("Unknown dwarf tag {}", di_type.getTag());
    abort();
  }
}

[[nodiscard]] Ref<di::StructureType> LLVMMetadataConverter::convertStructureOrClassType(
    const llvm::DICompositeType& di_type) {
  return make_meta<di::StructureType>(di_type, [this, &di_type](auto& result) {
    result->set_name_raw(convertString(di_type.getName()));
    result->set_size_in_bits_raw(convertInteger(di_type.getSizeInBits()));
    result->set_identifier_raw(convertString(di_type.getIdentifier()));
    result->set_file_raw(convertDIFile(di_type.getFile()));
    result->set_scope_raw(convertDIScope(di_type.getScope()));
    result->set_line_raw(convertInteger(di_type.getLine()));
    auto inheritance_refs = std::vector<Ref<Meta>>{};
    auto method_refs      = std::vector<Ref<Meta>>{};
    auto member_refs      = std::vector<Ref<Meta>>{};
    if (di_type.getRawElements() != nullptr) {
      for (const auto di_elem : di_type.getElements()) {
        if (const auto derived_type = llvm::dyn_cast<llvm::DIDerivedType>(di_elem)) {
          if (derived_type->getTag() == llvm::dwarf::DW_TAG_member) {
            member_refs.emplace_back(convertDIDerivedTypeMember(*derived_type));
          } else if (derived_type->getTag() == llvm::dwarf::DW_TAG_inheritance) {
            inheritance_refs.emplace_back(convertDIDerivedTypeInheritance(*derived_type));
          } else {
            LOG_FATAL("Unexpected derived type tag {} in structure or class type", derived_type->getTag());
            abort();
          }
        } else if (const auto subprogram = llvm::dyn_cast<llvm::DISubprogram>(di_elem)) {
          method_refs.emplace_back(convertDISubprogram(*subprogram));
        } else {
          LOG_FATAL("Unexpected llvm meta subclass of kind {} as element of structure or type",
                    di_elem->getMetadataID());
          abort();
        }
      }
    }
    result->set_base_classes_raw(convertTuple(std::move(inheritance_refs)));
    result->set_methods_raw(convertTuple(std::move(method_refs)));
    result->set_members_raw(convertTuple(std::move(member_refs)));
  });
}

[[nodiscard]] Ref<di::ArrayType> LLVMMetadataConverter::convertArrayType(const llvm::DICompositeType& di_type) {
  auto subrange_to_int = [](const llvm::DISubrange* di_subrange) {
    const auto& count = di_subrange->getCount();
    if (count.is<llvm::ConstantInt*>()) {
      return count.get<llvm::ConstantInt*>()->getZExtValue();
    } else {
      LOG_FATAL("VLAs are currently unsupported!");
      abort();
    }
  };
  return make_meta<di::ArrayType>(di_type, [this, &di_type, &subrange_to_int](auto& result) {
    result->set_base_type_raw(convertDIType(di_type.getBaseType()));
    result->set_size_in_bits_raw(convertInteger(di_type.getSizeInBits()));
    auto refs = std::vector<Ref<Meta>>{};
    if (di_type.getRawElements() != nullptr) {
      const auto& di_elements = di_type.getElements();
      refs.reserve(di_elements.size());
      for (const auto& di_elem : di_elements) {
        auto di_subrange = llvm::dyn_cast<llvm::DISubrange>(di_elem);
        assert(di_subrange != nullptr);
        refs.emplace_back(convertInteger(subrange_to_int(di_subrange)));
      }
    }
    result->set_counts_raw(convertTuple(std::move(refs)));
  });
}

di::DerivedKind fromLLVMDerivedTypeTag(unsigned int tag) {
  switch (tag) {
    case llvm::dwarf::DW_TAG_pointer_type:
      return di::DerivedKind::Pointer;
    case llvm::dwarf::DW_TAG_reference_type:
      return di::DerivedKind::Reference;
    case llvm::dwarf::DW_TAG_typedef:
      return di::DerivedKind::Typedef;
    case llvm::dwarf::DW_TAG_const_type:
      return di::DerivedKind::Const;
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

[[nodiscard]] Ref<di::Type> LLVMMetadataConverter::convertDIDerivedType(const llvm::DIDerivedType& di_type) {
  if (di_type.getTag() == llvm::dwarf::DW_TAG_member) {
    LOG_FATAL("LLVMMetadataConverter::convertDIDerivedTypeMember should be used for members");
    abort();
  } else if (di_type.getTag() == llvm::dwarf::DW_TAG_inheritance) {
    LOG_FATAL("LLVMMetadataConverter::convertDIDerivedTypeInheritance should be used for inheritance");
    abort();
  } else {
    return make_meta<di::DerivedType>(di_type, [this, &di_type](auto& result) {
      result->set_name_raw(convertString(di_type.getName()));
      result->set_size_in_bits_raw(convertInteger(di_type.getSizeInBits()));
      result->set_tag_raw(convertInteger((int)fromLLVMDerivedTypeTag(di_type.getTag())));
      result->set_file_raw(convertDIFile(di_type.getFile()));
      result->set_scope_raw(convertDIScope(di_type.getScope()));
      result->set_line_raw(convertInteger(di_type.getLine()));
      result->set_base_type_raw(convertDIType(di_type.getBaseType()));
      result->set_offset_in_bits_raw(convertInteger(di_type.getOffsetInBits()));
    });
  }
}

[[nodiscard]] Ref<di::Inheritance> LLVMMetadataConverter::convertDIDerivedTypeInheritance(
    const llvm::DIDerivedType& di_type) {
  if (di_type.getTag() == llvm::dwarf::DW_TAG_inheritance) {
    return make_meta<di::Inheritance>(di_type, [this, &di_type](auto& result) {
      result->set_scope_raw(convertDIScope(di_type.getScope()));
      auto base = convertDIType(di_type.getBaseType());
      if (base->get_canonical_type().get_kind() != Kind::StructureType) {
        LOG_FATAL("Expected the canonical base of an inheritance to be a structure type, but it has kind {}",
                  base->get_canonical_type().get_kind());
        abort();
      }
      result->set_base_raw(base);
      result->set_offset_in_bits_raw(convertInteger(di_type.getOffsetInBits()));
    });
  } else {
    LOG_FATAL(
        "Called LLVMMetadataConverter::convertDIDerivedTypeInheritance called with llvm::DIDerivedType which is not an "
        "inheritance");
    abort();
  }
}

[[nodiscard]] Ref<di::Member> LLVMMetadataConverter::convertDIDerivedTypeMember(const llvm::DIDerivedType& di_type) {
  if (di_type.getTag() == llvm::dwarf::DW_TAG_member) {
    return make_meta<di::Member>(di_type, [this, &di_type](auto& result) {
      result->set_name_raw(convertString(di_type.getName()));
      result->set_size_in_bits_raw(convertInteger(di_type.getSizeInBits()));
      result->set_file_raw(convertDIFile(di_type.getFile()));
      result->set_scope_raw(convertDIScope(di_type.getScope()));
      result->set_line_raw(convertInteger(di_type.getLine()));
      result->set_type_raw(convertDIType(di_type.getBaseType()));
      result->set_offset_in_bits_raw(convertInteger(di_type.getOffsetInBits()));
    });
  } else {
    LOG_FATAL(
        "Called LLVMMetadataConverter::convertDIDerivedTypeMember called with llvm::DIDerivedType which is not a "
        "member");
    abort();
  }
}

[[nodiscard]] Ref<di::SubroutineType> LLVMMetadataConverter::convertDISubroutineType(
    const llvm::DISubroutineType& di_type) {
  return make_meta<di::SubroutineType>(di_type, [this, &di_type](auto& result) {
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
  return make_meta<di::LexicalBlock>(di_block, [this, &di_block](auto& result) {
    result->set_file_raw(convertDIFile(di_block.getFile()));
    result->set_scope_raw(convertDIScope(di_block.getScope()));
    result->set_line_raw(convertInteger(di_block.getLine()));
    result->set_column_raw(convertInteger(di_block.getColumn()));
  });
}

[[nodiscard]] Ref<di::LexicalBlockFile> LLVMMetadataConverter::convertDILexicalBlockFile(
    const llvm::DILexicalBlockFile& di_block) {
  return make_meta<di::LexicalBlockFile>(di_block, [this, &di_block](auto& result) {
    result->set_file_raw(convertDIFile(di_block.getFile()));
    result->set_scope_raw(convertDIScope(di_block.getScope()));
    result->set_discriminator_raw(convertInteger(di_block.getDiscriminator()));
  });
}

[[nodiscard]] Ref<di::Subprogram> LLVMMetadataConverter::convertDISubprogram(const llvm::DISubprogram& di_subprogram) {
  return make_meta<di::Subprogram>(di_subprogram, [this, &di_subprogram](auto& result) {
    result->set_name_raw(convertString(di_subprogram.getName()));
    result->set_linkage_name_raw(convertString(di_subprogram.getLinkageName()));
    result->set_file_raw(convertDIFile(di_subprogram.getFile()));
    result->set_scope_raw(convertDIScope(di_subprogram.getScope()));
    result->set_line_raw(convertInteger(di_subprogram.getLine()));
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
  return make_meta<di::CompileUnit>(di_unit, [this, &di_unit](auto& result) {
    result->set_language_raw(convertInteger((int)fromLLVMDILanguage(di_unit.getSourceLanguage())));
    result->set_file_raw(convertDIFile(di_unit.getFile()));
    result->set_producer_raw(convertString(di_unit.getProducer()));
    result->set_is_optimized_raw(convertInteger(di_unit.isOptimized()));
    result->set_runtime_version_raw(convertInteger(di_unit.getRuntimeVersion()));
  });
}

[[nodiscard]] Ref<di::Namespace> LLVMMetadataConverter::convertDINamespace(const llvm::DINamespace& di_namespace) {
  return make_meta<di::Namespace>(di_namespace, [this, &di_namespace](auto& result) {
    result->set_name_raw(convertString(di_namespace.getName()));
    result->set_scope_raw(convertDIScope(di_namespace.getScope()));
  });
}

[[nodiscard]] Ref<di::Location> LLVMMetadataConverter::convertDILocation(const llvm::DILocation& di_location) {
  return make_meta<di::Location>(di_location, [this, &di_location](auto& result) {
    result->set_line_raw(convertInteger(di_location.getLine()));
    result->set_column_raw(convertInteger(di_location.getColumn()));
    auto scope = dyn_cast<di::LocalScope>(convertDIScope(di_location.getScope()));
    assert(scope.get() != nullptr);
    result->set_scope_raw(scope);
  });
}

[[nodiscard]] Ref<Integer> LLVMMetadataConverter::convertInteger(int64_t value) {
  return make_meta<Integer>([&value](auto& result) { result->set_data(value); });
}

[[nodiscard]] Ref<String> LLVMMetadataConverter::convertString(std::string str) {
  return make_meta<String>([str = std::move(str)](auto& result) { result->set_data(std::move(str)); });
}

[[nodiscard]] Ref<Tuple> LLVMMetadataConverter::convertTuple(std::vector<Ref<Meta>> refs) {
  return make_meta<Tuple>([refs = std::move(refs)](auto& result) { result->get_refs() = std::move(refs); });
}

}  // namespace typeart::meta
