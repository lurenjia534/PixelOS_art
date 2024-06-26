/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_LIBARTBASE_BASE_ALLOCATOR_H_
#define ART_LIBARTBASE_BASE_ALLOCATOR_H_

#include <type_traits>

#include "atomic.h"
#include "macros.h"

namespace art {

static constexpr bool kEnableTrackingAllocator = false;

class Allocator {
 public:
  static Allocator* GetCallocAllocator();
  static Allocator* GetNoopAllocator();

  Allocator() {}
  virtual ~Allocator() {}

  virtual void* Alloc(size_t) = 0;
  virtual void Free(void*) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Allocator);
};

// Used by TrackedAllocators.
enum AllocatorTag {
  kAllocatorTagHeap,
  kAllocatorTagMonitorList,
  kAllocatorTagClassTable,
  kAllocatorTagInternTable,
  kAllocatorTagMaps,
  kAllocatorTagLOS,
  kAllocatorTagSafeMap,
  kAllocatorTagLOSMaps,
  kAllocatorTagReferenceTable,
  kAllocatorTagHeapBitmap,
  kAllocatorTagHeapBitmapLOS,
  kAllocatorTagMonitorPool,
  kAllocatorTagLOSFreeList,
  kAllocatorTagVerifier,
  kAllocatorTagRememberedSet,
  kAllocatorTagModUnionCardSet,
  kAllocatorTagModUnionReferenceArray,
  kAllocatorTagJNILibraries,
  kAllocatorTagCompileTimeClassPath,
  kAllocatorTagOatFile,
  kAllocatorTagDexFileVerifier,
  kAllocatorTagRosAlloc,
  kAllocatorTagCount,  // Must always be last element.
};
std::ostream& operator<<(std::ostream& os, AllocatorTag tag);

namespace TrackedAllocators {

// We use memory_order_relaxed updates of the following counters. Values are treated as approximate
// wherever concurrent updates are possible.
// Running count of number of bytes used for this kind of allocation. Increased by allocations,
// decreased by deallocations.
extern Atomic<size_t> g_bytes_used[kAllocatorTagCount];

// Largest value of bytes used seen.
extern Atomic<size_t> g_max_bytes_used[kAllocatorTagCount];

// Total number of bytes allocated of this kind.
extern Atomic<uint64_t> g_total_bytes_used[kAllocatorTagCount];

void Dump(std::ostream& os);

inline void RegisterAllocation(AllocatorTag tag, size_t bytes) {
  g_total_bytes_used[tag].fetch_add(bytes, std::memory_order_relaxed);
  size_t new_bytes = g_bytes_used[tag].fetch_add(bytes, std::memory_order_relaxed) + bytes;
  size_t max_bytes = g_max_bytes_used[tag].load(std::memory_order_relaxed);
  while (max_bytes < new_bytes
    && !g_max_bytes_used[tag].compare_exchange_weak(max_bytes /* updated */, new_bytes,
                                                    std::memory_order_relaxed)) {
  }
}

inline void RegisterFree(AllocatorTag tag, size_t bytes) {
  g_bytes_used[tag].fetch_sub(bytes, std::memory_order_relaxed);
}

}  // namespace TrackedAllocators

// Tracking allocator for use with STL types, tracks how much memory is used.
template <class T, AllocatorTag kTag>
class TrackingAllocatorImpl {
  static_assert(kTag < kAllocatorTagCount, "kTag must be less than kAllocatorTagCount");

 public:
  using value_type = T;

  // Used internally by STL data structures. This copy constructor needs to be implicit. Don't wrap
  // the line because that would break cpplint's detection of the implicit constructor.
  template <class U>
  TrackingAllocatorImpl([[maybe_unused]] const TrackingAllocatorImpl<U, kTag>& alloc) noexcept {}  // NOLINT [runtime/explicit]
  // Used internally by STL data structures.
  TrackingAllocatorImpl() noexcept {}

  // Enables an allocator for objects of one type to allocate storage for objects of another type.
  // Used internally by STL data structures.
  template <class U>
  struct rebind {
    using other = TrackingAllocatorImpl<U, kTag>;
  };

  T* allocate(size_t n) {
    const size_t size = n * sizeof(T);
    TrackedAllocators::RegisterAllocation(GetTag(), size);
    return reinterpret_cast<T*>(malloc(size));
  }

  void deallocate(T* p, size_t n) {
    const size_t size = n * sizeof(T);
    TrackedAllocators::RegisterFree(GetTag(), size);
    free(p);
  }

  static constexpr AllocatorTag GetTag() {
    return kTag;
  }
};

template<class T, AllocatorTag kTag>
using TrackingAllocator = std::conditional_t<kEnableTrackingAllocator,
                                             TrackingAllocatorImpl<T, kTag>,
                                             std::allocator<T>>;

}  // namespace art

#endif  // ART_LIBARTBASE_BASE_ALLOCATOR_H_
