#pragma once

#include "meta/Meta.hpp"

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

namespace meta {

class LLVMMetadataConverter {
  Database* db;
  std::vector<const llvm::Metadata*> parents;
  std::map<const llvm::Metadata*, Meta*> llvm_to_meta;

 public:
  inline LLVMMetadataConverter(Database& db) : db(&db) {
  }

  [[nodiscard]] StackAllocation* createStackAllocation(const llvm::DILocalVariable& di_local,
                                                       const llvm::DILocation& di_location,
                                                       std::optional<size_t> count);
  [[nodiscard]] HeapAllocation* createHeapAllocation(const llvm::DIType& di_type, const llvm::DILocation& di_location);
  [[nodiscard]] GlobalAllocation* createGlobalAllocation(const llvm::DIGlobalVariable& di_global);

 private:
  template <class MetaClass>
  MetaClass* lookup_meta(const llvm::Metadata& llvm_meta);

  template <class MetaClass, class InitializerFn>
  MetaClass* make_meta(const llvm::Metadata& llvm_meta, InitializerFn&& initializer_fn);

  template <class MetaClass, class InitializerFn>
  MetaClass* make_meta(InitializerFn&& initializer_fn);

  [[nodiscard]] di::LocalVariable* convertDILocalVariable(const llvm::DILocalVariable& di_local);
  [[nodiscard]] di::GlobalVariable* convertDIGlobalVariable(const llvm::DIGlobalVariable& di_global);
  [[nodiscard]] di::Node* convertDINode(const llvm::DINode& di_node);
  [[nodiscard]] di::Subrange* convertDISubrange(const llvm::DISubrange& di_subrange);
  [[nodiscard]] di::Enumerator* convertDIEnumerator(const llvm::DIEnumerator& di_enum);
  [[nodiscard]] di::Scope* convertDIScope(const llvm::DIScope* di_scope);
  [[nodiscard]] di::File* convertDIFile(const llvm::DIFile* di_file);
  [[nodiscard]] di::Type* convertDIType(const llvm::DIType* di_type);
  [[nodiscard]] di::BasicType* convertDIBasicType(const llvm::DIBasicType& di_type);
  [[nodiscard]] di::Type* convertDICompositeType(const llvm::DICompositeType& di_type);
  [[nodiscard]] di::StructureType* convertStructureOrClassType(const llvm::DICompositeType& di_type);
  [[nodiscard]] di::UnionType* convertUnionType(const llvm::DICompositeType& di_type);
  [[nodiscard]] di::ArrayType* convertArrayType(const llvm::DICompositeType& di_type);
  [[nodiscard]] di::Type* convertDIDerivedType(const llvm::DIDerivedType& di_type);
  [[nodiscard]] di::Inheritance* convertDIDerivedTypeInheritance(const llvm::DIDerivedType& di_type);
  [[nodiscard]] di::Member* convertDIDerivedTypeMember(const llvm::DIDerivedType& di_type);
  [[nodiscard]] di::SubroutineType* convertDISubroutineType(const llvm::DISubroutineType& di_type);
  [[nodiscard]] di::LexicalBlock* convertDILexicalBlock(const llvm::DILexicalBlock& di_block);
  [[nodiscard]] di::LexicalBlockFile* convertDILexicalBlockFile(const llvm::DILexicalBlockFile& di_block);
  [[nodiscard]] di::Subprogram* convertDISubprogram(const llvm::DISubprogram& di_subprogram);
  [[nodiscard]] di::CompileUnit* convertDICompileUnit(const llvm::DICompileUnit& di_unit);
  [[nodiscard]] di::Namespace* convertDINamespace(const llvm::DINamespace& di_namespace);
  [[nodiscard]] di::Location* convertDILocation(const llvm::DILocation& di_location);
  [[nodiscard]] Integer* convertInteger(int64_t value);
  [[nodiscard]] String* convertString(std::string str);
  [[nodiscard]] Tuple* convertTuple(std::vector<Meta*> refs);
  [[nodiscard]] Optional* convertOptional(std::optional<Meta*> value);
};

}  // namespace typeart::meta