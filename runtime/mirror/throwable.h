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

#ifndef ART_RUNTIME_MIRROR_THROWABLE_H_
#define ART_RUNTIME_MIRROR_THROWABLE_H_

#include "object.h"

namespace art HIDDEN {

class RootVisitor;
struct ThrowableOffsets;

namespace mirror {

class String;

// C++ mirror of java.lang.Throwable
class MANAGED Throwable : public Object {
 public:
  MIRROR_CLASS("Ljava/lang/Throwable;");

  void SetDetailMessage(ObjPtr<String> new_detail_message) REQUIRES_SHARED(Locks::mutator_lock_);

  ObjPtr<String> GetDetailMessage() REQUIRES_SHARED(Locks::mutator_lock_);

  EXPORT std::string Dump() REQUIRES_SHARED(Locks::mutator_lock_);

  // This is a runtime version of initCause, you shouldn't use it if initCause may have been
  // overridden. Also it asserts rather than throwing exceptions. Currently this is only used
  // in cases like the verifier where the checks cannot fail and initCause isn't overridden.
  void SetCause(ObjPtr<Throwable> cause) REQUIRES_SHARED(Locks::mutator_lock_);
  ObjPtr<Throwable> GetCause() REQUIRES_SHARED(Locks::mutator_lock_);
  void SetStackState(ObjPtr<Object> state) REQUIRES_SHARED(Locks::mutator_lock_);
  bool IsCheckedException() REQUIRES_SHARED(Locks::mutator_lock_);
  bool IsError() REQUIRES_SHARED(Locks::mutator_lock_);

  int32_t GetStackDepth() REQUIRES_SHARED(Locks::mutator_lock_);

 private:
  ObjPtr<Object> GetStackState() REQUIRES_SHARED(Locks::mutator_lock_);
  ObjPtr<Object> GetStackTrace() REQUIRES_SHARED(Locks::mutator_lock_);

  // Field order required by test "ValidateFieldOrderOfJavaCppUnionClasses".
  HeapReference<Object> backtrace_;
  HeapReference<Throwable> cause_;
  HeapReference<String> detail_message_;
  HeapReference<Object> stack_trace_;
  HeapReference<Object> suppressed_exceptions_;

  friend struct art::ThrowableOffsets;  // for verifying offset information
  DISALLOW_IMPLICIT_CONSTRUCTORS(Throwable);
};

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_THROWABLE_H_
