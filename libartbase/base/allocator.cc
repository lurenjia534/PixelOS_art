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

#include "allocator.h"

#include <inttypes.h>
#include <stdlib.h>

#include <android-base/logging.h>

#include "atomic.h"

namespace art {

class CallocAllocator final : public Allocator {
 public:
  CallocAllocator() {}
  ~CallocAllocator() {}

  void* Alloc(size_t size) override {
    return calloc(sizeof(uint8_t), size);
  }

  void Free(void* p) override {
    free(p);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CallocAllocator);
};

CallocAllocator g_malloc_allocator;

class NoopAllocator final : public Allocator {
 public:
  NoopAllocator() {}
  ~NoopAllocator() {}

  void* Alloc([[maybe_unused]] size_t size) override {
    LOG(FATAL) << "NoopAllocator::Alloc should not be called";
    UNREACHABLE();
  }

  void Free([[maybe_unused]] void* p) override {
    // Noop.
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NoopAllocator);
};

NoopAllocator g_noop_allocator;

Allocator* Allocator::GetCallocAllocator() {
  return &g_malloc_allocator;
}

Allocator* Allocator::GetNoopAllocator() {
  return &g_noop_allocator;
}

namespace TrackedAllocators {

// These globals are safe since they don't have any non-trivial destructors.
Atomic<size_t> g_bytes_used[kAllocatorTagCount];
Atomic<size_t> g_max_bytes_used[kAllocatorTagCount];
Atomic<uint64_t> g_total_bytes_used[kAllocatorTagCount];

void Dump(std::ostream& os) {
  if (kEnableTrackingAllocator) {
    os << "Dumping native memory usage\n";
    for (size_t i = 0; i < kAllocatorTagCount; ++i) {
      uint64_t bytes_used = g_bytes_used[i].load(std::memory_order_relaxed);
      uint64_t max_bytes_used = g_max_bytes_used[i].load(std::memory_order_relaxed);
      uint64_t total_bytes_used = g_total_bytes_used[i].load(std::memory_order_relaxed);
      if (total_bytes_used != 0) {
        os << static_cast<AllocatorTag>(i) << " active=" << bytes_used << " max="
           << max_bytes_used << " total=" << total_bytes_used << "\n";
      }
    }
  }
}

}  // namespace TrackedAllocators

}  // namespace art
