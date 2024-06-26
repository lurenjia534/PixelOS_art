/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include "arena_bit_vector.h"

#include "allocator.h"
#include "arena_allocator.h"
#include "bit_vector-inl.h"

namespace art {

template <bool kCount>
class ArenaBitVectorAllocatorKindImpl;

template <>
class ArenaBitVectorAllocatorKindImpl<false> {
 public:
  // Not tracking allocations, ignore the supplied kind and arbitrarily provide kArenaAllocSTL.
  explicit ArenaBitVectorAllocatorKindImpl([[maybe_unused]] ArenaAllocKind kind) {}
  ArenaBitVectorAllocatorKindImpl(const ArenaBitVectorAllocatorKindImpl&) = default;
  ArenaBitVectorAllocatorKindImpl& operator=(const ArenaBitVectorAllocatorKindImpl&) = default;
  ArenaAllocKind Kind() { return kArenaAllocGrowableBitMap; }
};

template <bool kCount>
class ArenaBitVectorAllocatorKindImpl {
 public:
  explicit ArenaBitVectorAllocatorKindImpl(ArenaAllocKind kind) : kind_(kind) { }
  ArenaBitVectorAllocatorKindImpl(const ArenaBitVectorAllocatorKindImpl&) = default;
  ArenaBitVectorAllocatorKindImpl& operator=(const ArenaBitVectorAllocatorKindImpl&) = default;
  ArenaAllocKind Kind() { return kind_; }

 private:
  ArenaAllocKind kind_;
};

using ArenaBitVectorAllocatorKind =
    ArenaBitVectorAllocatorKindImpl<kArenaAllocatorCountAllocations>;

template <typename ArenaAlloc>
class ArenaBitVectorAllocator final : public Allocator, private ArenaBitVectorAllocatorKind {
 public:
  static ArenaBitVectorAllocator* Create(ArenaAlloc* allocator, ArenaAllocKind kind) {
    void* storage = allocator->template Alloc<ArenaBitVectorAllocator>(kind);
    return new (storage) ArenaBitVectorAllocator(allocator, kind);
  }

  ~ArenaBitVectorAllocator() {
    LOG(FATAL) << "UNREACHABLE";
    UNREACHABLE();
  }

  void* Alloc(size_t size) override {
    return allocator_->Alloc(size, this->Kind());
  }

  void Free(void*) override {}  // Nop.

 private:
  ArenaBitVectorAllocator(ArenaAlloc* allocator, ArenaAllocKind kind)
      : ArenaBitVectorAllocatorKind(kind), allocator_(allocator) { }

  ArenaAlloc* const allocator_;

  DISALLOW_COPY_AND_ASSIGN(ArenaBitVectorAllocator);
};

ArenaBitVector::ArenaBitVector(ArenaAllocator* allocator,
                               unsigned int start_bits,
                               bool expandable,
                               ArenaAllocKind kind)
  :  BitVector(start_bits,
               expandable,
               ArenaBitVectorAllocator<ArenaAllocator>::Create(allocator, kind)) {
  DCHECK_EQ(GetHighestBitSet(), -1) << "The arena bit vector should start empty";
}

ArenaBitVector::ArenaBitVector(ScopedArenaAllocator* allocator,
                               unsigned int start_bits,
                               bool expandable,
                               ArenaAllocKind kind)
  :  BitVector(start_bits,
               expandable,
               ArenaBitVectorAllocator<ScopedArenaAllocator>::Create(allocator, kind)) {
  ClearAllBits();
}

}  // namespace art
