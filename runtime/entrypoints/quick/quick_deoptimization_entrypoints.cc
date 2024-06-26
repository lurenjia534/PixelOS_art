/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include "base/logging.h"  // For VLOG_IS_ON.
#include "base/mutex.h"
#include "callee_save_frame.h"
#include "interpreter/interpreter.h"
#include "obj_ptr-inl.h"  // TODO: Find the other include that isn't complete, and clean this up.
#include "quick_exception_handler.h"
#include "runtime.h"
#include "thread.h"

namespace art HIDDEN {

NO_RETURN static void artDeoptimizeImpl(Thread* self,
                                        DeoptimizationKind kind,
                                        bool single_frame,
                                        bool skip_method_exit_callbacks)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  Runtime::Current()->IncrementDeoptimizationCount(kind);
  if (VLOG_IS_ON(deopt)) {
    if (single_frame) {
      // Deopt logging will be in DeoptimizeSingleFrame. It is there to take advantage of the
      // specialized visitor that will show whether a method is Quick or Shadow.
    } else {
      LOG(INFO) << "Deopting:";
      self->Dump(LOG_STREAM(INFO));
    }
  }

  self->AssertHasDeoptimizationContext();
  QuickExceptionHandler exception_handler(self, true);
  if (single_frame) {
    exception_handler.DeoptimizeSingleFrame(kind);
  } else {
    exception_handler.DeoptimizeStack(skip_method_exit_callbacks);
  }
  if (exception_handler.IsFullFragmentDone()) {
    exception_handler.DoLongJump(true);
  } else {
    exception_handler.DeoptimizePartialFragmentFixup();
    // We cannot smash the caller-saves, as we need the ArtMethod in a parameter register that would
    // be caller-saved. This has the downside that we cannot track incorrect register usage down the
    // line.
    exception_handler.DoLongJump(false);
  }
}

extern "C" NO_RETURN void artDeoptimize(Thread* self, bool skip_method_exit_callbacks)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ScopedQuickEntrypointChecks sqec(self);
  artDeoptimizeImpl(self, DeoptimizationKind::kFullFrame, false, skip_method_exit_callbacks);
}

// This is called directly from compiled code by an HDeoptimize.
extern "C" NO_RETURN void artDeoptimizeFromCompiledCode(DeoptimizationKind kind, Thread* self)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ScopedQuickEntrypointChecks sqec(self);
  // Before deoptimizing to interpreter, we must push the deoptimization context.
  JValue return_value;
  return_value.SetJ(0);  // we never deoptimize from compiled code with an invoke result.
  self->PushDeoptimizationContext(return_value,
                                  /* is_reference= */ false,
                                  self->GetException(),
                                  /* from_code= */ true,
                                  DeoptimizationMethodType::kDefault);
  // Deopting from compiled code, so method exit haven't run yet. Don't skip method exit callbacks
  // if required.
  artDeoptimizeImpl(self, kind, true, /* skip_method_exit_callbacks= */ false);
}

}  // namespace art
