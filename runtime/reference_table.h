/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ART_RUNTIME_REFERENCE_TABLE_H_
#define ART_RUNTIME_REFERENCE_TABLE_H_

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "base/allocator.h"
#include "base/locks.h"
#include "base/macros.h"
#include "gc_root.h"
#include "obj_ptr.h"

namespace art HIDDEN {
namespace jni {
class LocalReferenceTable;
}  // namespace jni
namespace mirror {
class Object;
}  // namespace mirror

// Maintain a table of references.  Used for JNI monitor references and
// JNI pinned array references.
//
// None of the functions are synchronized.
class ReferenceTable {
 public:
  ReferenceTable(const char* name, size_t initial_size, size_t max_size);
  ~ReferenceTable();

  void Add(ObjPtr<mirror::Object> obj) REQUIRES_SHARED(Locks::mutator_lock_);

  void Remove(ObjPtr<mirror::Object> obj) REQUIRES_SHARED(Locks::mutator_lock_);

  size_t Size() const;

  void Dump(std::ostream& os)
      REQUIRES_SHARED(Locks::mutator_lock_)
      REQUIRES(!Locks::alloc_tracker_lock_);

  EXPORT void VisitRoots(RootVisitor* visitor, const RootInfo& root_info)
      REQUIRES_SHARED(Locks::mutator_lock_);

 private:
  using Table = std::vector<GcRoot<mirror::Object>,
                            TrackingAllocator<GcRoot<mirror::Object>, kAllocatorTagReferenceTable>>;
  static void Dump(std::ostream& os, Table& entries)
      REQUIRES_SHARED(Locks::mutator_lock_)
      REQUIRES(!Locks::alloc_tracker_lock_);
  friend class IndirectReferenceTable;  // For Dump.
  friend class jni::LocalReferenceTable;  // For Dump.

  std::string name_;
  Table entries_;
  size_t max_size_;
};

}  // namespace art

#endif  // ART_RUNTIME_REFERENCE_TABLE_H_
