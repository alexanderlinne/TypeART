#include "CommandLine.h"

#include "llvm/Support/CommandLine.h"

using namespace llvm;

static cl::OptionCategory typeart_category("TypeART instrumentation pass", "These control the instrumentation.");

static cl::opt<std::string> cl_typeart_mode(
    "typeart-mode", cl::desc("Hint for typeart whether it runs on optimized code or will be optimized."),
    cl::init("combined"), cl::cat(typeart_category));

static cl::opt<std::string> cl_typeart_type_file("typeart-types", cl::desc("Location of the generated type file."),
                                                 cl::init("types.yaml"), cl::cat(typeart_category));

static cl::opt<bool> cl_typeart_stats("typeart-stats", cl::desc("Show statistics for TypeArt type pass."), cl::Hidden,
                                      cl::init(false), cl::cat(typeart_category));

static cl::opt<bool> cl_typeart_instrument_heap("typeart-heap",
                                                cl::desc("Instrument heap allocation/free instructions."),
                                                cl::init(true), cl::cat(typeart_category));

static cl::opt<bool> cl_typeart_instrument_global("typeart-global", cl::desc("Instrument global allocations."),
                                                  cl::init(false), cl::cat(typeart_category));

static cl::opt<bool> cl_typeart_instrument_stack(
    "typeart-stack", cl::desc("Instrument stack (alloca) allocations. Turns on global instrumentation."),
    cl::init(false), cl::cat(typeart_category), cl::callback([](const bool& opt) {
      if (opt) {
        ::cl_typeart_instrument_global = true;
      }
    }));

static cl::opt<bool> cl_typeart_instrument_stack_lifetime(
    "typeart-stack-lifetime", cl::desc("Instrument lifetime.start intrinsic instead of alloca."), cl::init(true),
    cl::cat(typeart_category));

static cl::OptionCategory typeart_meminstfinder_category(
    "TypeART memory instruction finder", "These options control which memory instructions are collected/filtered.");

static cl::opt<bool> cl_typeart_filter_stack_non_array("typeart-stack-array-only",
                                                       cl::desc("Only find stack (alloca) instructions of arrays."),
                                                       cl::Hidden, cl::init(false),
                                                       cl::cat(typeart_meminstfinder_category));

static cl::opt<bool> cl_typeart_filter_heap_alloc(
    "typeart-malloc-store-filter", cl::desc("Filter alloca instructions that have a store from a heap allocation."),
    cl::Hidden, cl::init(false), cl::cat(typeart_meminstfinder_category));

static cl::opt<bool> cl_typeart_filter_global("typeart-filter-globals", cl::desc("Filter globals of a module."),
                                              cl::Hidden, cl::init(true), cl::cat(typeart_meminstfinder_category));

static cl::opt<bool> cl_typeart_call_filter(
    "typeart-call-filter",
    cl::desc("Filter (stack/global) alloca instructions that are passed to specific function calls."), cl::Hidden,
    cl::init(false), cl::cat(typeart_meminstfinder_category));

static cl::opt<typeart::analysis::FilterImplementation> cl_typeart_call_filter_implementation(
    "typeart-call-filter-impl", cl::desc("Select the call filter implementation."),
    cl::values(clEnumValN(typeart::analysis::FilterImplementation::none, "none", "No filter"),
               clEnumValN(typeart::analysis::FilterImplementation::standard, "std",
                          "Standard forward filter (default)"),
               clEnumValN(typeart::analysis::FilterImplementation::cg, "cg", "Call-graph-based filter")),
    cl::Hidden, cl::init(typeart::analysis::FilterImplementation::standard), cl::cat(typeart_meminstfinder_category));

static cl::opt<std::string> cl_typeart_call_filter_glob(
    "typeart-call-filter-str", cl::desc("Filter allocas based on the function name (glob) <string>."), cl::Hidden,
    cl::init("*MPI_*"), cl::cat(typeart_meminstfinder_category));

static cl::opt<std::string> cl_typeart_call_filter_glob_deep(
    "typeart-call-filter-deep-str",
    cl::desc("Filter allocas based on specific API, i.e., value passed as void* are correlated when string matched and "
             "possibly kept."),
    cl::Hidden, cl::init("MPI_*"), cl::cat(typeart_meminstfinder_category));

static cl::opt<std::string> cl_typeart_call_filter_cg_file("typeart-call-filter-cg-file",
                                                           cl::desc("Location of call-graph file to use."), cl::Hidden,
                                                           cl::init(""), cl::cat(typeart_meminstfinder_category));

static cl::opt<bool> cl_typeart_filter_pointer_alloca("typeart-filter-pointer-alloca",
                                                      cl::desc("Filter allocas of pointer types."), cl::Hidden,
                                                      cl::init(true), cl::cat(typeart_meminstfinder_category));

namespace typeart::cl {

analysis::MemInstFinderConfig getMemInstFinderConfig() {
  return analysis::MemInstFinderConfig{cl_typeart_instrument_heap,                                                   //
                                       cl_typeart_instrument_stack,                                                  //
                                       cl_typeart_instrument_global,                                                 //
                                       analysis::MemInstFinderConfig::Filter{cl_typeart_filter_stack_non_array,      //
                                                                             cl_typeart_filter_heap_alloc,           //
                                                                             cl_typeart_filter_global,               //
                                                                             cl_typeart_call_filter,                 //
                                                                             cl_typeart_filter_pointer_alloca,       //
                                                                             cl_typeart_call_filter_implementation,  //
                                                                             cl_typeart_call_filter_glob,            //
                                                                             cl_typeart_call_filter_glob_deep,       //
                                                                             cl_typeart_call_filter_cg_file}};
}

const std::string& getTypeFilepath() {
  assert(!cl_typeart_type_file.empty() && "Default type file not set");
  return cl_typeart_type_file.getValue();
}

InstrumentationMode getInstrumentationMode() {
  auto& value = cl_typeart_mode.getValue();
  if (value == "combined") {
    return InstrumentationMode::combined;
  } else if (value == "preopt") {
    return InstrumentationMode::before_optimization;
  } else if (value == "postopt") {
    return InstrumentationMode::after_optimization;
  }
  abort();
}

bool getInstrumentGlobal() {
  return cl_typeart_instrument_global.getValue();
}

bool getInstrumentStack() {
  return cl_typeart_instrument_stack.getValue();
}

bool getInstrumentStackLifetime() {
  return cl_typeart_instrument_stack_lifetime.getValue();
}

bool getInstrumentHeap() {
  return cl_typeart_instrument_heap.getValue();
}

bool getPrintStats() {
  return cl_typeart_stats.getValue();
}

}  // namespace typeart::cl
