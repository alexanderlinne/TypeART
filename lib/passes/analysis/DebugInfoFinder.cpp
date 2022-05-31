#include "DebugInfoFinder.hpp"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Transforms/Utils/Local.h>

namespace typeart::analysis {

std::pair<llvm::DILocalVariable*, llvm::DILocation*> findDbgInfoFor(llvm::AllocaInst* alloca) {
  if (alloca == nullptr) {
    return {nullptr, nullptr};
  }
  llvm::SmallVector<llvm::DbgVariableIntrinsic*, 4> dbg_users;
  llvm::findDbgUsers(dbg_users, alloca);
  for (const auto& user : alloca->users()) {
    if (auto bit_cast = llvm::dyn_cast<llvm::BitCastInst>(user)) {
      llvm::findDbgUsers(dbg_users, bit_cast);
    }
  }
  for (const auto& user : dbg_users) {
    if (auto inst = llvm::dyn_cast<llvm::DbgDeclareInst>(user)) {
      return {inst->getVariable(), inst->getDebugLoc()};
    }
    if (auto inst = llvm::dyn_cast<llvm::DbgValueInst>(user)) {
      auto expr = inst->getExpression();
      if (expr->getNumElements() == 1 && expr->getElement(0) == llvm::dwarf::DW_OP_deref) {
        return {inst->getVariable(), inst->getDebugLoc()};
      }
    }
  }
  return {nullptr, nullptr};
}

llvm::DIType* removeIrrelevantDerivedTypes(llvm::DIType* type) {
  while (type->getTag() == llvm::dwarf::DW_TAG_typedef || type->getTag() == llvm::dwarf::DW_TAG_member) {
    type = llvm::dyn_cast<llvm::DIDerivedType>(type)->getBaseType();
    assert(type != nullptr);
  }
  return type;
}

llvm::DIType* backtrackDIType(llvm::Value* value) {
  if (value == nullptr) {
    return nullptr;
  }

  if (auto inst = llvm::dyn_cast<llvm::Instruction>(value)) {
    switch (inst->getOpcode()) {
      case llvm::Instruction::Store:
        return backtrackDIType(llvm::cast<llvm::StoreInst>(inst)->getPointerOperand());
      case llvm::Instruction::Alloca: {
        auto [local_variable, _] = findDbgInfoFor(llvm::cast<llvm::AllocaInst>(inst));
        return local_variable != nullptr ? local_variable->getType() : nullptr;
      }
      case llvm::Instruction::Load: {
        // A load means that a pointer or reference is dereferenced. We therefore
        // determine the type of the pointer operand and remove one "pointer level"
        // from the type.
        auto type = backtrackDIType(llvm::cast<llvm::LoadInst>(inst)->getPointerOperand());
        if (type == nullptr) {
          return nullptr;
        }
        type              = removeIrrelevantDerivedTypes(type);
        auto derived_type = llvm::dyn_cast<llvm::DIDerivedType>(type);
        assert(derived_type != nullptr);
        assert(derived_type->getTag() == llvm::dwarf::DW_TAG_pointer_type ||
               derived_type->getTag() == llvm::dwarf::DW_TAG_reference_type);
        return derived_type->getBaseType();
      }
      case llvm::Instruction::GetElementPtr: {
        auto gep_inst = llvm::cast<llvm::GetElementPtrInst>(inst);
        auto type     = backtrackDIType(gep_inst->getPointerOperand());
        if (type == nullptr) {
          return nullptr;
        }
        type = removeIrrelevantDerivedTypes(type);
        if (gep_inst->getSourceElementType()->isPointerTy()) {
          return type;
        } else if (gep_inst->getSourceElementType()->isArrayTy()) {
          auto composite_type = llvm::dyn_cast<llvm::DICompositeType>(type);
          assert(composite_type != nullptr);
          return composite_type->getBaseType();
        } else {
          auto composite_type = llvm::dyn_cast<llvm::DICompositeType>(type);
          assert(composite_type != nullptr);
          gep_inst->print(llvm::errs());
          llvm::errs() << "\n";
          assert(gep_inst->getSourceElementType()->isStructTy());
          assert(gep_inst->getNumIndices() == 1 || gep_inst->getNumIndices() == 2);
          auto index_it    = gep_inst->indices().begin();
          auto first_const = llvm::dyn_cast<llvm::ConstantInt>(index_it->get());
          if (gep_inst->getNumIndices() == 1 || first_const == nullptr || first_const->getZExtValue() != 0) {
            return composite_type;
          }
          auto index_const = llvm::dyn_cast<llvm::ConstantInt>((index_it + 1)->get());
          assert(index_const != nullptr);
          auto struct_type = gep_inst->getSourceElementType();
          auto dl          = gep_inst->getModule()->getDataLayout();
          auto offset      = dl.getIndexedOffsetInType(struct_type, {*index_it, index_const}) * 8;
          for (const auto& element : composite_type->getElements()) {
            if (auto derived_type = llvm::dyn_cast<llvm::DIDerivedType>(element);
                derived_type != nullptr && derived_type->getTag() == llvm::dwarf::DW_TAG_member &&
                static_cast<int64_t>(derived_type->getOffsetInBits()) == offset) {
              return derived_type;
            }
          }
          llvm::errs() << "Couln't determine gep type!\n";
          abort();
        }
      }
      case llvm::Instruction::BitCast:
        return backtrackDIType(llvm::cast<llvm::BitCastInst>(inst)->getOperand(0));
      default:
        llvm::errs() << "backtrackDIType: Unknown instruction type " << value->getValueID() << "!\n";
        value->print(llvm::errs());
        llvm::errs() << "\n";
        abort();
    }
  } else {
    if (auto global_variable = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
      auto global_expression =
          llvm::cast_or_null<llvm::DIGlobalVariableExpression>(global_variable->getMetadata("dbg"));
      return global_expression != nullptr ? global_expression->getVariable()->getType() : nullptr;
    } else if (auto argument = llvm::dyn_cast<llvm::Argument>(value)) {
      if (auto subprogram = argument->getParent()->getSubprogram(); subprogram != nullptr) {
        return subprogram->getType()->getTypeArray()[argument->getArgNo() + 1];
      }
      return nullptr;
    } else if (auto const_expr = llvm::dyn_cast<llvm::ConstantExpr>(value)) {
      auto converted_inst = const_expr->getAsInstruction();
      auto result         = backtrackDIType(converted_inst);
      converted_inst->deleteValue();
      return result;
    } else {
      llvm::errs() << "backtrackDIType: Unknown value type " << value->getValueID() << "!\n";
      value->print(llvm::errs());
      llvm::errs() << "\n";
      abort();
    }
  }
}

// When seraching forward for revelant instructions, this function determines
// wether a user should be added to the worklist.
bool shouldFollowUser(llvm::Value* operand, llvm::User* user) {
  if (auto inst = llvm::dyn_cast<llvm::Instruction>(user)) {
    switch (inst->getOpcode()) {
        // Only follow store instructions if the allocated pointer is stored,
        // not if a value is stored into it.
      case llvm::Instruction::Store:
        return operand == llvm::cast<llvm::StoreInst>(inst)->getValueOperand();

        // Only follow a gep instruction if the allocated pointer is offset,
        // not if it is used as an offset value.
      case llvm::Instruction::GetElementPtr:
        return operand == llvm::cast<llvm::GetElementPtrInst>(inst)->getPointerOperand();

      case llvm::Instruction::BitCast:
      case llvm::Instruction::PHI:
      case llvm::Instruction::Call:
      case llvm::Instruction::CallBr:
      case llvm::Instruction::Invoke:
      case llvm::Instruction::Ret:
        return true;

      case llvm::Instruction::ICmp:
      case llvm::Instruction::PtrToInt:
        return false;

      default:
        llvm::errs() << "should_follow_user: Unknown instruction type!\n";
        user->print(llvm::errs());
        llvm::errs() << "\n";
        abort();
    }
  } else {
    llvm::errs() << "should_follow_isre: Unknown user type!\n";
    user->print(llvm::errs());
    llvm::errs() << "\n";
    abort();
  }
}

llvm::SmallVector<llvm::DIType*, 4> findDITypes(llvm::CallBase& call) {
  auto result   = llvm::SmallVector<llvm::DIType*, 4>{};
  auto visited  = llvm::SmallPtrSet<llvm::User*, 4>{};
  auto worklist = llvm::SmallVector<llvm::Use*, 4>{};

  auto add_work = [&](llvm::Value* operand, llvm::Use* use) {
    if (shouldFollowUser(operand, use->getUser())) {
      if (visited.insert(use->getUser()).second) {
        worklist.push_back(use);
      }
    }
  };
  auto add_result = [&](llvm::DIType* type) {
    if (type != nullptr) {
      result.push_back(type);
    }
  };

  for (auto& use : call.uses()) {
    add_work(&call, &use);
  }
  while (!worklist.empty()) {
    auto use = worklist.pop_back_val();
    assert(visited.count(use->getUser()));
    if (auto inst = llvm::dyn_cast<llvm::Instruction>(use->getUser())) {
      switch (inst->getOpcode()) {
        case llvm::Instruction::BitCast:
        case llvm::Instruction::GetElementPtr:
        case llvm::Instruction::PHI:
          for (auto& _use : inst->uses()) {
            add_work(inst, &_use);
          }
          break;
        case llvm::Instruction::Store:
          add_result(backtrackDIType(inst));
          break;
        case llvm::Instruction::Ret: {
          if (auto subprogram = inst->getFunction()->getSubprogram(); subprogram != nullptr) {
            add_result(*subprogram->getType()->getTypeArray().begin());
          }
          break;
        }
        case llvm::Instruction::Call:
        case llvm::Instruction::CallBr:
        case llvm::Instruction::Invoke: {
          auto call_inst = llvm::cast<llvm::CallBase>(use->getUser());
          if (auto subprogram = call_inst->getCalledFunction()->getSubprogram(); subprogram != nullptr) {
            add_result(subprogram->getType()->getTypeArray()[use->getOperandNo() + 1]);
          }
          break;
        }
        default:
          llvm::errs() << "findDITypes: Unknown instruction type!\n";
          use->getUser()->print(llvm::errs());
          llvm::errs() << "\n";
          abort();
      }
    } else {
      llvm::errs() << "findDITypes: Unknown value type!\n";
      use->getUser()->print(llvm::errs());
      llvm::errs() << "\n";
      abort();
    }
  }
  return result;
}

}  // namespace typeart::analysis
