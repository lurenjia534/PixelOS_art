/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ART_RUNTIME_MIRROR_METHOD_HANDLES_LOOKUP_H_
#define ART_RUNTIME_MIRROR_METHOD_HANDLES_LOOKUP_H_

#include "base/utils.h"
#include "base/macros.h"
#include "handle.h"
#include "obj_ptr.h"
#include "object.h"

namespace art HIDDEN {

struct MethodHandlesLookupOffsets;
class RootVisitor;

namespace mirror {

class MethodHandle;
class MethodType;

// C++ mirror of java.lang.invoke.MethodHandles.Lookup
class MANAGED MethodHandlesLookup : public Object {
 public:
  MIRROR_CLASS("Ljava/lang/invoke/MethodHandles$Lookup;");

  static ObjPtr<mirror::MethodHandlesLookup> Create(Thread* const self, Handle<Class> lookup_class)
      REQUIRES_SHARED(Locks::mutator_lock_) REQUIRES(!Roles::uninterruptible_);

  // Returns the result of java.lang.invoke.MethodHandles.lookup().
  static ObjPtr<mirror::MethodHandlesLookup> GetDefault(Thread* const self)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Find constructor using java.lang.invoke.MethodHandles$Lookup.findConstructor().
  ObjPtr<mirror::MethodHandle> FindConstructor(Thread* const self,
                                               Handle<Class> klass,
                                               Handle<MethodType> method_type)
      REQUIRES_SHARED(Locks::mutator_lock_);

 private:
  static MemberOffset AllowedModesOffset() {
    return MemberOffset(OFFSETOF_MEMBER(MethodHandlesLookup, allowed_modes_));
  }

  static MemberOffset LookupClassOffset() {
    return MemberOffset(OFFSETOF_MEMBER(MethodHandlesLookup, lookup_class_));
  }

  HeapReference<mirror::Class> lookup_class_;

  int32_t allowed_modes_;

  friend struct art::MethodHandlesLookupOffsets;  // for verifying offset information
  DISALLOW_IMPLICIT_CONSTRUCTORS(MethodHandlesLookup);
};

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_METHOD_HANDLES_LOOKUP_H_
