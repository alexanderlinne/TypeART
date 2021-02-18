//
// Created by ahueck on 12.10.20.
//

#ifndef TYPEART_ACCESSCOUNTER_H
#define TYPEART_ACCESSCOUNTER_H

#include "RuntimeData.h"
#include "RuntimeInterface.h"
//#include "safe_ptr.h"

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace typeart {

namespace softcounter {

using Counter       = long long int;
using AtomicCounter = std::atomic<Counter>;

/**
 * Updates an atomic maximum value.
 * Based on https://stackoverflow.com/questions/16190078/how-to-atomically-update-a-maximum-value.
 * @param maxVal The maximum value.
 * @param newVal The new value.
 */
template <typename T>
inline void updateMax(std::atomic<T>& maxVal, T newVal) noexcept {
  T prevMax = maxVal;
  while (prevMax < newVal && !maxVal.compare_exchange_weak(prevMax, newVal)) {
  };
}

struct CounterStats {
  static CounterStats create(const std::vector<Counter>& vals) {
    unsigned n   = vals.size();
    double sum   = std::accumulate(vals.begin(), vals.end(), 0.0);
    double mean  = sum / n;
    double sqSum = std::inner_product(vals.begin(), vals.end(), vals.begin(), 0.0);
    double std   = std::sqrt(sqSum / n - mean * mean);
    Counter min  = *std::min_element(vals.begin(), vals.end());
    Counter max  = *std::max_element(vals.begin(), vals.end());
    return CounterStats(sum, min, max, mean, std);
  }

  CounterStats(double sum, double min, double max, double mean, double std)
      : sum(sum), minVal(min), maxVal(max), meanVal(mean), stdVal(std) {
  }

  const double sum{0};
  const double minVal{0};
  const double maxVal{0};
  const double meanVal{0};
  const double stdVal{0};
};

class AccessRecorder {
 public:
  //  template <typename T>
  //  using ContFreeObj =
  //      sf::safe_obj<T, sf::contention_free_shared_mutex<>, std::unique_lock<sf::contention_free_shared_mutex<>>,
  //                   std::shared_lock<sf::contention_free_shared_mutex<>>>;
  using TypeCountMap = std::unordered_map<int, Counter>;
  // using TypeCountMapSafe = sf::contfree_safe_ptr<TypeCountMap>;
  //  using TypeCountMapSafe = ContFreeObj<TypeCountMap>;
  using AddressSet = std::unordered_set<MemAddr>;
  // using AddressSetSafe = sf::contfree_safe_ptr<AddressSet>;
  //  using AddressSetSafe = ContFreeObj<AddressSet>;

  // using MutexT = sf::contention_free_shared_mutex<>;
  using MutexT = std::shared_mutex;

  class ThreadRecorder {
   public:
    inline void incHeapAlloc(size_t count) {
      ++heapAllocs;
      if (count > 1) {
        heapArray++;
      }
    }

    inline void incHeapFree(size_t count) {
      ++heapAllocsFree;
      if (count > 1) {
        ++heapArrayFree;
      }
    }

    inline void incStackAlloc(size_t count) {
      ++curStackAllocs;
      ++stackAllocs;
      if (count > 1) {
        ++stackArray;
      }
    }

    inline void incStackFree(size_t count) {
      ++stackAllocsFree;
      if (count > 1) {
        ++stackArrayFree;
      }
    }

    inline void decStackAlloc(size_t amount) {
      updateMax(maxStackAllocs, curStackAllocs.load());
      curStackAllocs -= amount;
    }

    Counter getHeapAllocs() const {
      return heapAllocs;
    }
    Counter getHeapArray() const {
      return heapArray;
    }
    Counter getHeapAllocsFree() const {
      return heapAllocsFree;
    }
    Counter getHeapArrayFree() const {
      return heapArrayFree;
    }
    Counter getStackAllocs() const {
      return stackAllocs;
    }
    Counter getMaxStackAllocs() const {
      return maxStackAllocs;
    }
    Counter getCurStackAllocs() const {
      return curStackAllocs;
    }
    Counter getStackArray() const {
      return stackArray;
    }
    Counter getStackAllocsFree() const {
      return stackAllocsFree;
    }
    Counter getStackArrayFree() const {
      return stackArrayFree;
    }

    std::unordered_map<std::string, Counter> getValuesAsMap() const {
      std::unordered_map<std::string, Counter> vals;
      vals["heapAllocs"]      = heapAllocs;
      vals["heapArray"]       = heapArray;
      vals["heapAllocsFree"]  = heapAllocsFree;
      vals["heapArrayFree"]   = heapArrayFree;
      vals["stackAllocs"]     = stackAllocs;
      vals["curStackAllocs"]  = curStackAllocs;
      vals["maxStackAllocs"]  = maxStackAllocs;
      vals["stackArray"]      = stackArray;
      vals["stackAllocsFree"] = stackAllocsFree;
      vals["stackArrayFree"]  = stackArrayFree;
      return vals;
    }

   private:
    AtomicCounter heapAllocs     = 0;
    AtomicCounter heapArray      = 0;
    AtomicCounter heapAllocsFree = 0;
    AtomicCounter heapArrayFree  = 0;

    AtomicCounter stackAllocs     = 0;
    AtomicCounter curStackAllocs  = 0;
    AtomicCounter maxStackAllocs  = 0;
    AtomicCounter stackArray      = 0;
    AtomicCounter stackAllocsFree = 0;
    AtomicCounter stackArrayFree  = 0;
  };

  using ThreadRecorderMap     = std::unordered_map<std::thread::id, ThreadRecorder>;
  using ThreadRecorderMapSafe = sf::contfree_safe_ptr<ThreadRecorderMap>;

  ~AccessRecorder() = default;

  inline void incHeapAlloc(int typeId, size_t count) {
    ++curHeapAllocs;

    // Always check here for max
    // A program without free would otherwise never update maxHeap (see test 20_softcounter_max)
    updateMax(maxHeapAllocs, curHeapAllocs.load());

    ++heapAllocs;
    if (count > 1) {
      ++heapArray;
    }

    {
      std::lock_guard threadRecorderGuard(threadRecorderMutex);
      getCurrentThreadRecorder().incHeapAlloc(count);
    }

    std::lock_guard lock(heapAllocMutex);
    ++heapAlloc[typeId];
  }

  inline void incStackAlloc(int typeId, size_t count) {
    {
      std::lock_guard threadRecorderGuard(threadRecorderMutex);
      getCurrentThreadRecorder().incStackAlloc(count);
    }

    std::lock_guard lock(stackAllocMutex);
    ++stackAlloc[typeId];
  }

  inline void incGlobalAlloc(int typeId, size_t count) {
    ++globalAllocs;
    if (count > 1) {
      ++globalArray;
    }

    std::lock_guard lock(globalAllocMutex);
    ++globalAlloc[typeId];
  }

  inline void incStackFree(int typeId, size_t count) {
    {
      std::lock_guard threadRecorderGuard(threadRecorderMutex);
      getCurrentThreadRecorder().incStackFree(count);
    }

    std::lock_guard lock(stackFreeMutex);
    ++stackFree[typeId];
  }

  inline void incHeapFree(int typeId, size_t count) {
    ++heapAllocsFree;
    if (count > 1) {
      ++heapArrayFree;
    }

    {
      std::lock_guard threadRecorderGuard(threadRecorderMutex);
      getCurrentThreadRecorder().incHeapFree(count);
    }

    std::lock_guard lock(heapFreeMutex);
    ++heapFree[typeId];
  }

  inline void decHeapAlloc() {
    // Removed, since we already increment maxHeapAllocs just in time:
    //    if (curHeapAllocs > maxHeapAllocs) {
    //      maxHeapAllocs = curHeapAllocs;
    //    }
    --curHeapAllocs;
  }

  inline void decStackAlloc(size_t amount) {
    {
      std::lock_guard threadRecorderGuard(threadRecorderMutex);
      getCurrentThreadRecorder().decStackAlloc(amount);
    }
  }

  inline void incUsedInRequest(MemAddr addr) {
    ++addrChecked;

    std::lock_guard lock(seenMutex);
    seen.insert(addr);
  }

  inline void incAddrReuse() {
    ++addrReuses;
  }

  inline void incAddrMissing(MemAddr addr) {
    ++addrMissing;

    std::lock_guard lock(missingMutex);
    missing.insert(addr);
  }

  inline void incNullAddr() {
    ++nullAlloc;
  }

  inline void incZeroLengthAddr() {
    ++zeroAlloc;
  }

  inline void incZeroLengthAndNullAddr() {
    ++nullAndZeroAlloc;
  }

  inline void incUDefTypes(size_t count) {
    numUDefTypes += count;
  }

  inline void incOmpContextStack() {
    ++omp_stack;
  }

  inline void incOmpContextHeap() {
    ++omp_heap;
  }

  inline void incOmpContextFree() {
    ++omp_heap_free;
  }

  Counter getHeapAllocs() const {
    return heapAllocs;
  }
  Counter getStackAllocs() const {
    return getStackAllocsThreadStats().sum;
  }
  Counter getGlobalAllocs() const {
    return globalAllocs;
  }
  Counter getMaxHeapAllocs() const {
    return maxHeapAllocs;
  }
  Counter getMaxStackAllocs() const {
    return getMaxStackAllocsThreadStats().maxVal;
  }
  Counter getCurHeapAllocs() const {
    return curHeapAllocs;
  }
  //  Counter getCurStackAllocs() const {
  //    return curStackAllocs;
  //  }
  Counter getAddrReuses() const {
    return addrReuses;
  }
  Counter getAddrMissing() const {
    return addrMissing;
  }
  Counter getAddrChecked() const {
    return addrChecked;
  }
  Counter getStackArray() const {
    return getStackArrayThreadStats().sum;
  }
  Counter getHeapArray() const {
    return heapArray;
  }
  Counter getGlobalArray() const {
    return globalArray;
  }
  Counter getStackAllocsFree() const {
    return getStackAllocsFreeThreadStats().sum;
  }
  Counter getStackArrayFree() const {
    return getStackArrayFreeThreadStats().sum;
  }
  Counter getHeapAllocsFree() const {
    return heapAllocsFree;
  }
  Counter getHeapArrayFree() const {
    return heapArrayFree;
  }
  Counter getNullAlloc() const {
    return nullAlloc;
  }
  Counter getZeroAlloc() const {
    return zeroAlloc;
  }
  Counter getNullAndZeroAlloc() const {
    return nullAndZeroAlloc;
  }
  Counter getNumUDefTypes() const {
    return numUDefTypes;
  }
  Counter getOmpHeapCalls() const {
    return omp_heap;
  }
  Counter getOmpFreeCalls() const {
    return omp_heap_free;
  }
  Counter getOmpStackCalls() const {
    return omp_stack;
  }
  Counter getCurrentThreadStackAllocs() {
    std::lock_guard slock(threadRecorderMutex);
    return getCurrentThreadRecorder().getCurStackAllocs();
  }

#define THREAD_VALS_GETTER_FN(COUNTER_NAME)                    \
  std::vector<Counter> get##COUNTER_NAME##ThreadData() const { \
    std::shared_lock guard(threadRecorderMutex);               \
    std::vector<Counter> vals;                                 \
    vals.reserve(threadRecorders.size());                      \
    for (auto& [id, r] : threadRecorders) {                    \
      vals.push_back(r.get##COUNTER_NAME());                   \
    }                                                          \
    return vals;                                               \
  }

  THREAD_VALS_GETTER_FN(HeapAllocs)
  THREAD_VALS_GETTER_FN(HeapArray)
  THREAD_VALS_GETTER_FN(HeapAllocsFree)
  THREAD_VALS_GETTER_FN(HeapArrayFree)
  THREAD_VALS_GETTER_FN(StackAllocs)
  THREAD_VALS_GETTER_FN(MaxStackAllocs)
  THREAD_VALS_GETTER_FN(StackArray)
  THREAD_VALS_GETTER_FN(StackAllocsFree)
  THREAD_VALS_GETTER_FN(StackArrayFree)

#undef THREAD_STATS_GETTER_FN

#define THREAD_STATS_GETTER_FN(COUNTER_NAME)                      \
  CounterStats get##COUNTER_NAME##ThreadStats() const {           \
    return CounterStats::create(get##COUNTER_NAME##ThreadData()); \
  }

  THREAD_STATS_GETTER_FN(HeapAllocs)
  THREAD_STATS_GETTER_FN(HeapArray)
  THREAD_STATS_GETTER_FN(HeapAllocsFree)
  THREAD_STATS_GETTER_FN(HeapArrayFree)
  THREAD_STATS_GETTER_FN(StackAllocs)
  THREAD_STATS_GETTER_FN(MaxStackAllocs)
  THREAD_STATS_GETTER_FN(StackArray)
  THREAD_STATS_GETTER_FN(StackAllocsFree)
  THREAD_STATS_GETTER_FN(StackArrayFree)

#undef THREADS_STATS_GETTER_FN

  AddressSet getMissing() const {
    std::shared_lock slock(missingMutex);
    return missing;
  }
  AddressSet getSeen() const {
    std::shared_lock slock(seenMutex);
    return seen;
  }
  TypeCountMap getStackAlloc() const {
    std::shared_lock slock(stackAllocMutex);
    return stackAlloc;
  }
  TypeCountMap getHeapAlloc() const {
    std::shared_lock slock(heapAllocMutex);
    return heapAlloc;
  }
  TypeCountMap getGlobalAlloc() const {
    std::shared_lock slock(globalAllocMutex);
    return globalAlloc;
  }
  TypeCountMap getStackFree() const {
    std::shared_lock slock(stackFreeMutex);
    return stackFree;
  }
  TypeCountMap getHeapFree() const {
    std::shared_lock slock(heapFreeMutex);
    return heapFree;
  }

  /**
   * Must be locked by the caller.
   * @return
   */
  inline ThreadRecorder& getCurrentThreadRecorder() {
    auto tid = std::this_thread::get_id();
    //    if (threadRecorders.find(tid) == threadRecorders.end()) {
    //      std::cout << "New thread registered: " << tid << std::endl;
    //    }
    return threadRecorders[tid];
  }

  std::vector<std::thread::id> getThreadIds() const {
    std::vector<std::thread::id> ids;
    std::shared_lock slock(threadRecorderMutex);
    std::transform(threadRecorders.begin(), threadRecorders.end(), std::back_inserter(ids),
                   [](const auto& pair) { return pair.first; });
    return ids;
  }

  std::unordered_map<std::string, Counter> getThreadDataAsMap(std::thread::id threadId) const {
    std::shared_lock slock(threadRecorderMutex);
    if (auto it = threadRecorders.find(threadId); it != threadRecorders.end()) {
      return it->second.getValuesAsMap();
    }
    return {};
  }

  size_t getNumThreads() const {
    std::shared_lock guard(threadRecorderMutex);
    return threadRecorders.size();
  }

 private:
  AtomicCounter heapAllocs = 0;
  //  AtomicCounter stackAllocs      = 0;
  AtomicCounter globalAllocs  = 0;
  AtomicCounter maxHeapAllocs = 0;
  //  AtomicCounter maxStackAllocs   = 0;
  AtomicCounter curHeapAllocs = 0;
  //  AtomicCounter curStackAllocs   = 0;
  AtomicCounter addrReuses  = 0;
  AtomicCounter addrMissing = 0;
  AtomicCounter addrChecked = 0;
  //  AtomicCounter stackArray       = 0;
  AtomicCounter heapArray   = 0;
  AtomicCounter globalArray = 0;
  //  AtomicCounter stackAllocsFree  = 0;
  //  AtomicCounter stackArrayFree   = 0;
  AtomicCounter heapAllocsFree   = 0;
  AtomicCounter heapArrayFree    = 0;
  AtomicCounter nullAlloc        = 0;
  AtomicCounter zeroAlloc        = 0;
  AtomicCounter nullAndZeroAlloc = 0;
  AtomicCounter numUDefTypes     = 0;
  AtomicCounter omp_stack        = 0;
  AtomicCounter omp_heap         = 0;
  AtomicCounter omp_heap_free    = 0;

  //  ThreadRecorderMapSafe threadRecorders;
  mutable MutexT threadRecorderMutex;
  ThreadRecorderMap threadRecorders;

  AddressSet missing;
  mutable MutexT missingMutex;

  AddressSet seen;
  mutable MutexT seenMutex;

  TypeCountMap stackAlloc;
  mutable MutexT stackAllocMutex;

  TypeCountMap heapAlloc;
  mutable MutexT heapAllocMutex;

  TypeCountMap globalAlloc;
  mutable MutexT globalAllocMutex;

  TypeCountMap stackFree;
  mutable MutexT stackFreeMutex;

  TypeCountMap heapFree;
  mutable MutexT heapFreeMutex;

  //  AddressSetSafe missing;
  //  AddressSetSafe seen;
  //  TypeCountMapSafe stackAlloc;
  //  TypeCountMapSafe heapAlloc;
  //  TypeCountMapSafe globalAlloc;
  //  TypeCountMapSafe stackFree;
  //  TypeCountMapSafe heapFree;
};

/**
 * Used for no-operations in counter methods when not using softcounters.
 */
class NoneRecorder {
 public:
  [[maybe_unused]] inline void incHeapAlloc(int, size_t) {
  }
  [[maybe_unused]] inline void incStackAlloc(int, size_t) {
  }
  [[maybe_unused]] inline void incGlobalAlloc(int, size_t) {
  }
  [[maybe_unused]] inline void incUsedInRequest(MemAddr) {
  }
  [[maybe_unused]] inline void decHeapAlloc() {
  }
  [[maybe_unused]] inline void decStackAlloc(size_t) {
  }
  [[maybe_unused]] inline void incAddrReuse() {
  }
  [[maybe_unused]] inline void incAddrMissing(MemAddr) {
  }
  [[maybe_unused]] inline void incStackFree(int, size_t) {
  }
  [[maybe_unused]] inline void incHeapFree(int, size_t) {
  }
  [[maybe_unused]] inline void incNullAddr() {
  }
  [[maybe_unused]] inline void incZeroLengthAddr() {
  }
  [[maybe_unused]] inline void incZeroLengthAndNullAddr() {
  }
  [[maybe_unused]] inline void incUDefTypes(size_t count) {
  }
  [[maybe_unused]] inline void incOmpContextStack() {
  }
  [[maybe_unused]] inline void incOmpContextHeap() {
  }
  [[maybe_unused]] inline void incOmpContextFree() {
  }
};

}  // namespace softcounter

#if ENABLE_SOFTCOUNTER == 1
using Recorder = softcounter::AccessRecorder;
#else
using Recorder = softcounter::NoneRecorder;
#endif

}  // namespace typeart

#endif  // TYPEART_ACCESSCOUNTER_H
