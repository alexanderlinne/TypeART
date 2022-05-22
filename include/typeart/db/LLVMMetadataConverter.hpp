#pragma once

#include "db/Types.hpp"

#include <map>
#include <set>

namespace llvm {
class Metadata;
class DILocation;
class DICompileUnit;
class DILocalVariable;
class DIType;
class DIGlobalVariable;
class DINode;
class DISubrange;
class DIScope;
class DIEnumerator;
class DIFile;
class DIBasicType;
class DIDerivedType;
class DICompositeType;
class DISubroutineType;
class DILexicalBlock;
class DILexicalBlockFile;
class DISubprogram;
class DINamespace;
};  // namespace llvm

namespace typeart::meta {

class LLVMMetadataConverter {
  Database* db;
  std::vector<const llvm::Metadata*> parents;
  std::map<const llvm::Metadata*, Meta*> llvm_to_meta;

 public:
  inline LLVMMetadataConverter(Database& db) : db(&db) {
  }

  [[nodiscard]] Ref<StackAllocation> createStackAllocation(const llvm::DILocalVariable& di_local,
                                                           const llvm::DILocation& di_location);
  [[nodiscard]] Ref<HeapAllocation> createHeapAllocation(const llvm::DIType& di_type,
                                                         const llvm::DILocation& di_location);
  [[nodiscard]] Ref<GlobalAllocation> createGlobalAllocation(const llvm::DIGlobalVariable& di_global);

 private:
  template <class MetaClass>
  MetaClass* lookup_meta(const llvm::Metadata& llvm_meta);

  template <class MetaClass, class InitializerFn>
  Ref<MetaClass> make_meta(const llvm::Metadata& llvm_meta, InitializerFn&& initializer_fn);

  template <class MetaClass, class InitializerFn>
  Ref<MetaClass> make_meta(InitializerFn&& initializer_fn);

  [[nodiscard]] Ref<di::LocalVariable> convertDILocalVariable(const llvm::DILocalVariable& di_local);
  [[nodiscard]] Ref<di::GlobalVariable> convertDIGlobalVariable(const llvm::DIGlobalVariable& di_global);
  [[nodiscard]] Ref<di::Node> convertDINode(const llvm::DINode& di_node);
  [[nodiscard]] Ref<di::Subrange> convertDISubrange(const llvm::DISubrange& di_subrange);
  [[nodiscard]] Ref<di::Enumerator> convertDIEnumerator(const llvm::DIEnumerator& di_enum);
  [[nodiscard]] Ref<di::Scope> convertDIScope(const llvm::DIScope* di_scope);
  [[nodiscard]] Ref<di::File> convertDIFile(const llvm::DIFile* di_file);
  [[nodiscard]] Ref<di::Type> convertDIType(const llvm::DIType* di_type);
  [[nodiscard]] Ref<di::BasicType> convertDIBasicType(const llvm::DIBasicType& di_type);
  [[nodiscard]] Ref<di::Type> convertDICompositeType(const llvm::DICompositeType& di_type);
  [[nodiscard]] Ref<di::DerivedType> convertDIDerivedType(const llvm::DIDerivedType& di_type);
  [[nodiscard]] Ref<di::SubroutineType> convertDISubroutineType(const llvm::DISubroutineType& di_type);
  [[nodiscard]] Ref<di::LexicalBlock> convertDILexicalBlock(const llvm::DILexicalBlock& di_block);
  [[nodiscard]] Ref<di::LexicalBlockFile> convertDILexicalBlockFile(const llvm::DILexicalBlockFile& di_block);
  [[nodiscard]] Ref<di::Subprogram> convertDISubprogram(const llvm::DISubprogram& di_subprogram);
  [[nodiscard]] Ref<di::CompileUnit> convertDICompileUnit(const llvm::DICompileUnit& di_unit);
  [[nodiscard]] Ref<di::Namespace> convertDINamespace(const llvm::DINamespace& di_namespace);
  [[nodiscard]] Ref<di::Location> convertDILocation(const llvm::DILocation& di_location);
  [[nodiscard]] Ref<Integer> convertInteger(int64_t value);
  [[nodiscard]] Ref<String> convertString(std::string str);
  [[nodiscard]] Ref<Tuple> convertTuple(std::vector<Ref<Meta>> refs);
};

}  // namespace typeart::meta