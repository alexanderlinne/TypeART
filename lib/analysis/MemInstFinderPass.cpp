/*
 * MemInstFinderPass.cpp
 *
 *  Created on: Jun 3, 2018
 *      Author: ahueck
 */

#include "MemInstFinderPass.h"

#include "MemOpVisitor.h"
#include "support/Logger.h"
#include "support/TypeUtil.h"
#include "support/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Format.h"

using namespace llvm;

#define DEBUG_TYPE "meminstanalysis"

namespace {
static RegisterPass<typeart::MemInstFinderPass> X("mem-inst-finder",                                // pass option
                                                  "Find heap and stack allocations in a program.",  // pass description
                                                  true,  // does not modify the CFG
                                                  true   // and it's an analysis
);
}  // namespace

static cl::opt<bool> ClFilterNonArrayAlloca("alloca-array-only", cl::desc("Only use alloca instructions of arrays."),
                                            cl::Hidden, cl::init(true));

static cl::opt<bool> ClFilterMallocAllocPair("malloc-store-filter",
                                             cl::desc("Filter allocs that get a store from a heap alloc."), cl::Hidden,
                                             cl::init(false));

static cl::opt<bool> ClCallFilter("call-filter",
                                  cl::desc("Filter alloca instructions that are passed to specific calls."), cl::Hidden,
                                  cl::init(false));

static cl::opt<bool> ClCallFilterHeap("call-filter-heap",
                                      cl::desc("Filter alloca instructions that are passed to specific calls."),
                                      cl::Hidden, cl::init(false));

static cl::opt<const char*> ClCallFilterGlob("call-filter-str", cl::desc("Filter alloca instructions based on string."),
                                             cl::Hidden, cl::init("MPI_*"));

STATISTIC(NumDetectedAllocs, "Number of detected allocs");
STATISTIC(NumCallFilteredAllocs, "Number of call filtered allocs");
STATISTIC(NumFilteredMallocAllocs, "Number of  filtered  malloc-related allocs");
STATISTIC(NumFilteredNonArrayAllocs, "Number of call filtered allocs");

namespace typeart {

namespace filter {

class CallFilter::FilterImpl {
  const std::string call_regex;
  bool malloc_mode{false};

 public:
  explicit FilterImpl(const std::string& glob) : call_regex(util::glob2regex(glob)) {
  }

  void setMode(bool search_malloc) {
    malloc_mode = search_malloc;
  }

  bool filter(Value* in) const {
    if (in == nullptr) {
      LOG_DEBUG("Called with nullptr");
      return false;
    }

    llvm::SmallPtrSet<Value*, 16> visited_set;
    llvm::SmallVector<Value*, 16> working_set;
    llvm::SmallVector<CallSite, 8> working_set_calls;

    const auto addToWork = [&visited_set, &working_set](auto vals) {
      for (auto v : vals) {
        if (visited_set.find(v) == visited_set.end()) {
          working_set.push_back(v);
          visited_set.insert(v);
        }
      }
    };

    const auto peek = [&working_set]() -> Value* {
      auto user_iter = working_set.end() - 1;
      working_set.erase(user_iter);
      return *user_iter;
    };

    // Seed working set with users of value (e.g., our AllocaInst)
    addToWork(in->users());

    // Search through all users of users of .... (e.g., our AllocaInst)
    while (!working_set.empty()) {
      auto val = peek();

      // If we encounter a callsite, we want to analyze later, or quit in case we have a regex match
      CallSite c(val);
      if (c.isCall()) {
        const auto callee = c.getCalledFunction();
        const bool indirect_call = callee == nullptr;

        if (indirect_call || callee->isDeclaration()) {
          LOG_DEBUG("Found an indirect call/only declaration, not filtering alloca. Call: "
                    << util::dump(*c.getInstruction()));
          return false;  // Indirect calls might contain critical function calls.
        }

        const auto name = FilterImpl::getName(callee);

        LOG_DEBUG("Found a call. Call: (" << util::dump(*c.getInstruction()) << ") Name: " << name);
        if (util::regex_matches(call_regex, name)) {
          LOG_DEBUG("Keeping alloca based on call name filter match");
          return false;
        }

        working_set_calls.push_back(c);
        // Caveat: below at the end of the loop, we add users of the function call to the search even though it might be
        // a simple "sink" for the alloca we analyse
      } else if (StoreInst* store = llvm::dyn_cast<StoreInst>(val)) {
        // If we encounter a store, we follow the store target pointer.
        // More inclusive than strictly necessary in some cases.
        LOG_DEBUG("Store found: " << util::dump(*store)
                                  << " Store target has users: " << util::dump(store->getPointerOperand()->users()));
        auto store_target = store->getPointerOperand();
        // FIXME here we check store operand, if target is another alloca, we already track that?:
        // Note: if we apply this to malloc filtering, this might become problematic?
        if (!malloc_mode && llvm::isa<AllocaInst>(store_target)) {
          LOG_DEBUG("Target is alloca, skipping!");
        } else {
          addToWork(store_target->users());
        }
        continue;
      }
      // cont. our search
      addToWork(val->users());
    }

    return std::all_of(working_set_calls.begin(), working_set_calls.end(), [&](CallSite c) { return filter(c, in); });
  }

 private:
  bool filter(CallSite& csite, Value* in) const {
    const auto analyse_arg = [&](auto& csite, auto argNum) -> bool {
      Argument& the_arg = *(csite.getCalledFunction()->arg_begin() + argNum);
      LOG_DEBUG("Calling filter with inst of argument: " << util::dump(the_arg));
      const bool filter_arg = filter(&the_arg);
      LOG_DEBUG("Should filter? : " << filter_arg);
      return filter_arg;
    };

    LOG_DEBUG("Analyzing function call " << csite.getCalledFunction()->getName());

    // this only works if we can correlate alloca with argument:
    const auto pos = std::find_if(csite.arg_begin(), csite.arg_end(),
                                  [&in](const Use& arg_use) -> bool { return arg_use.get() == in; });
    // auto pos = csite.arg_end();
    if (pos != csite.arg_end()) {
      const auto argNum = std::distance(csite.arg_begin(), pos);
      LOG_DEBUG("Found exact position: " << argNum);
      return analyse_arg(csite, argNum);
    } else {
      LOG_DEBUG("Analyze all args, cannot correlate alloca with arg.");
      return std::all_of(csite.arg_begin(), csite.arg_end(), [&csite, &analyse_arg](const Use& arg_use) {
        auto argNum = csite.getArgumentNo(&arg_use);
        return analyse_arg(csite, argNum);
      });
    }

    return true;
  }

  bool filter(Argument* arg) const {
    for (auto* user : arg->users()) {
      LOG_DEBUG("Looking at arg user " << util::dump(*user));
      // This code is for non mem2reg code (i.e., where the argument is stored to a local alloca):
      if (StoreInst* store = llvm::dyn_cast<StoreInst>(user)) {
        // if (auto* alloca = llvm::dyn_cast<AllocaInst>(store->getPointerOperand())) {
        //  LOG_DEBUG("Argument is a store inst and the operand is alloca");
        return filter(store->getPointerOperand());
        // }
      }
    }
    return filter(llvm::dyn_cast<Value>(arg));
  }

  static inline std::string getName(const Function* f) {
    auto name = f->getName();
    // FIXME figure out if we need to demangle, i.e., source is .c or .cpp
    const auto f_name = util::demangle(name);
    if (f_name != "") {
      name = f_name;
    }

    return name;
  }
};

CallFilter::CallFilter(const std::string& glob) : fImpl{std::make_unique<FilterImpl>(glob)} {
}

bool CallFilter::operator()(AllocaInst* in) {
  LOG_DEBUG("Analyzing value: " << util::dump(*in));
  fImpl->setMode(/*search mallocs = */ false);
  const auto filter_ = fImpl->filter(in);
  if (filter_) {
    LOG_DEBUG("Filtering value: " << util::dump(*in) << "\n");
  } else {
    LOG_DEBUG("Keeping value: " << util::dump(*in) << "\n");
  }
  return filter_;
}

bool CallFilter::operator()(CallSite c) {
  auto in = c.getInstruction();
  LOG_DEBUG("Analyzing call: " << util::dump(*in));
  fImpl->setMode(/*search mallocs = */ true);
  const auto filter_ = fImpl->filter(in);
  fImpl->setMode(false);
  if (filter_) {
    LOG_DEBUG("Filtering call: " << util::dump(*in) << "\n");
  } else {
    LOG_DEBUG("Keeping call: " << util::dump(*in) << "\n");
  }
  return filter_;
}

CallFilter& CallFilter::operator=(CallFilter&&) noexcept = default;

CallFilter::~CallFilter() = default;

}  // namespace filter

char MemInstFinderPass::ID = 0;

MemInstFinderPass::MemInstFinderPass() : llvm::FunctionPass(ID), mOpsCollector(), filter(ClCallFilterGlob.getValue()) {
}

void MemInstFinderPass::getAnalysisUsage(llvm::AnalysisUsage& info) const {
  info.setPreservesAll();
}

bool MemInstFinderPass::runOnFunction(llvm::Function& f) {
  LOG_DEBUG("Running on function: " << f.getName())
  const auto checkAmbigiousMalloc = [](const MallocData& mallocData) {
    auto primaryBitcast = mallocData.primary;
    if (primaryBitcast) {
      const auto& bitcasts = mallocData.bitcasts;
      std::for_each(bitcasts.begin(), bitcasts.end(), [&](auto bitcastInst) {
        if (bitcastInst != primaryBitcast && (!typeart::util::type::isVoidPtr(bitcastInst->getDestTy()) &&
                                              primaryBitcast->getDestTy() != bitcastInst->getDestTy())) {
          // Second non-void* bitcast detected - semantics unclear
          LOG_WARNING("Encountered ambiguous pointer type in allocation: " << util::dump(*(mallocData.call)));
          LOG_WARNING("  Primary cast: " << util::dump(*primaryBitcast));
          LOG_WARNING("  Secondary cast: " << util::dump(*bitcastInst));
        }
      });
    }
  };

  mOpsCollector.clear();
  mOpsCollector.visit(f);

  NumDetectedAllocs += mOpsCollector.listAlloca.size();

  if (ClFilterNonArrayAlloca) {
    auto& allocs = mOpsCollector.listAlloca;
    for (auto* alloc : allocs) {
      if (!alloc->getAllocatedType()->isArrayTy()) {
        allocs.erase(alloc);
        ++NumFilteredNonArrayAllocs;
      }
    }
  }

  if (ClFilterMallocAllocPair) {
    auto& alist = mOpsCollector.listAlloca;
    auto& mlist = mOpsCollector.listMalloc;

    const auto filterMallocAllocPairing = [&mlist](const auto alloc) {
      // Only look for the direct users of the alloc:
      // TODO is a deeper analysis required?
      for (auto inst : alloc->users()) {
        if (StoreInst* store = dyn_cast<StoreInst>(inst)) {
          const auto source = store->getValueOperand();
          if (isa<BitCastInst>(source)) {
            for (auto& mdata : mlist) {
              // is it a bitcast we already collected? if yes, we can filter the alloc
              return std::any_of(mdata.bitcasts.begin(), mdata.bitcasts.end(),
                                 [&source](const auto bcast) { return bcast == source; });
            }
          } else if (isa<CallInst>(source)) {
            return std::any_of(mlist.begin(), mlist.end(),
                               [&source](const auto& mdata) { return mdata.call == source; });
          }
        }
      }
      return false;
    };

    for (auto alloc : alist) {
      LOG_DEBUG("Filtering allocs (used to store a heap alloc pointer!) in function: " << f.getName());
      if (filterMallocAllocPairing(alloc)) {
        LOG_DEBUG("Filtering alloc: " << util::dump(*alloc));
        alist.erase(alloc);
        ++NumFilteredMallocAllocs;
      }
    }
  }

  if (ClCallFilter) {
    auto& allocs = mOpsCollector.listAlloca;
    for (auto* alloc : allocs) {
      if (filter(alloc)) {
        allocs.erase(alloc);
        ++NumCallFilteredAllocs;
      }
    }
    LOG_DEBUG("Allocas to instrument : " << util::dump(allocs));
  }

  auto& mallocs = mOpsCollector.listMalloc;

  if (ClCallFilterHeap) {
    mallocs.erase(mallocs.begin(),
                  std::remove_if(mallocs.begin(), mallocs.end(), [&](MallocData& data) { return filter(data.call); }));
  }

  for (const auto& mallocData : mallocs) {
    checkAmbigiousMalloc(mallocData);
  }

  return false;
}

bool MemInstFinderPass::doFinalization(llvm::Module&) {
  auto& out = llvm::outs();

  const unsigned max_string{28u};
  const unsigned max_val{5u};
  std::string line(42, '-');
  line += "\n";
  const auto make_format = [&](const char* desc, const auto val) {
    return format("%-*s: %*.1f\n", max_string, desc, max_val, val);
  };

  auto all_stack = double(NumDetectedAllocs.getValue());
  auto nonarray_stack = double(NumFilteredNonArrayAllocs.getValue());
  auto malloc_alloc_stack = double(NumFilteredMallocAllocs.getValue());
  auto call_filter_stack = double(NumCallFilteredAllocs.getValue());

  out << line;
  out << "   MemInstFinderPass\n";
  out << line;
  out << "Stack Memory\n";
  out << line;
  out << make_format("Alloca", all_stack);
  out << make_format("% non array filtered", (nonarray_stack / all_stack) * 100.0);
  out << make_format("% malloc-alloc filtered",
                     (malloc_alloc_stack / std::max(1.0, all_stack - nonarray_stack)) * 100.0);
  out << make_format("% call filtered",
                     (call_filter_stack / std::max(1.0, all_stack - nonarray_stack - malloc_alloc_stack)) * 100.0);
  out << line;
  out.flush();
  return false;
}

const llvm::SmallVector<MallocData, 8>& MemInstFinderPass::getFunctionMallocs() const {
  return mOpsCollector.listMalloc;
}

const llvm::SmallPtrSet<llvm::AllocaInst*, 8>& MemInstFinderPass::getFunctionAllocs() const {
  return mOpsCollector.listAlloca;
}

const llvm::SmallPtrSet<llvm::CallInst*, 8>& MemInstFinderPass::getFunctionFrees() const {
  return mOpsCollector.listFree;
}

}  // namespace typeart
