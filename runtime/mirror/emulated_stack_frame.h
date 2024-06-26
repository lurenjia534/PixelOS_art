/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ART_RUNTIME_MIRROR_EMULATED_STACK_FRAME_H_
#define ART_RUNTIME_MIRROR_EMULATED_STACK_FRAME_H_

#include "base/utils.h"
#include "base/macros.h"
#include "dex/dex_instruction.h"
#include "handle.h"
#include "object.h"
#include "stack.h"

namespace art HIDDEN {

struct EmulatedStackFrameOffsets;

namespace mirror {

class MethodType;

// C++ mirror of dalvik.system.EmulatedStackFrame
class MANAGED EmulatedStackFrame : public Object {
 public:
  MIRROR_CLASS("Ldalvik/system/EmulatedStackFrame;");

  // Creates an emulated stack frame whose type is |frame_type| from
  // a shadow frame.
  static ObjPtr<mirror::EmulatedStackFrame> CreateFromShadowFrameAndArgs(
      Thread* self,
      Handle<mirror::MethodType> args_type,
      Handle<mirror::MethodType> frame_type,
      const ShadowFrame& caller_frame,
      const InstructionOperands* const operands) REQUIRES_SHARED(Locks::mutator_lock_);

  // Writes the contents of this emulated stack frame to the |callee_frame|
  // whose type is |callee_type|, starting at |first_dest_reg|.
  void WriteToShadowFrame(
      Thread* self,
      Handle<mirror::MethodType> callee_type,
      const uint32_t first_dest_reg,
      ShadowFrame* callee_frame) REQUIRES_SHARED(Locks::mutator_lock_);

  // Sets |value| to the return value written to this emulated stack frame (if any).
  void GetReturnValue(Thread* self, JValue* value) REQUIRES_SHARED(Locks::mutator_lock_);

  // Sets the return value slot of this emulated stack frame to |value|.
  void SetReturnValue(Thread* self, const JValue& value) REQUIRES_SHARED(Locks::mutator_lock_);

  ObjPtr<mirror::MethodType> GetType() REQUIRES_SHARED(Locks::mutator_lock_);

  ObjPtr<mirror::Object> GetReceiver() REQUIRES_SHARED(Locks::mutator_lock_);

 private:
  ObjPtr<mirror::ObjectArray<mirror::Object>> GetReferences() REQUIRES_SHARED(Locks::mutator_lock_);

  ObjPtr<mirror::ByteArray> GetStackFrame() REQUIRES_SHARED(Locks::mutator_lock_);

  static MemberOffset TypeOffset() {
    return MemberOffset(OFFSETOF_MEMBER(EmulatedStackFrame, type_));
  }

  static MemberOffset ReferencesOffset() {
    return MemberOffset(OFFSETOF_MEMBER(EmulatedStackFrame, references_));
  }

  static MemberOffset StackFrameOffset() {
    return MemberOffset(OFFSETOF_MEMBER(EmulatedStackFrame, stack_frame_));
  }

  HeapReference<mirror::ObjectArray<mirror::Object>> references_;
  HeapReference<mirror::ByteArray> stack_frame_;
  HeapReference<mirror::MethodType> type_;

  friend struct art::EmulatedStackFrameOffsets;  // for verifying offset information
  DISALLOW_IMPLICIT_CONSTRUCTORS(EmulatedStackFrame);
};

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_EMULATED_STACK_FRAME_H_
