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

#include "scoped_thread_state_change.h"

#include <type_traits>

#include "base/aborting.h"
#include "base/casts.h"
#include "jni/java_vm_ext.h"
#include "mirror/object-inl.h"
#include "obj_ptr-inl.h"
#include "runtime-inl.h"

namespace art HIDDEN {

// See ScopedObjectAccessAlreadyRunnable::ScopedObjectAccessAlreadyRunnable(JavaVM*).
static_assert(std::is_base_of<JavaVM, JavaVMExt>::value, "JavaVMExt does not extend JavaVM");

void ScopedObjectAccessAlreadyRunnable::DCheckObjIsNotClearedJniWeakGlobal(
    ObjPtr<mirror::Object> obj) {
  DCHECK_NE(obj, Runtime::Current()->GetClearedJniWeakGlobal());
}

bool ScopedObjectAccessAlreadyRunnable::ForceCopy() const {
  return vm_->ForceCopy();
}

void ScopedThreadStateChange::ScopedThreadChangeDestructorCheck() {
  if (!expected_has_no_thread_) {
    Runtime* runtime = Runtime::Current();
    bool shutting_down = (runtime == nullptr) || runtime->IsShuttingDown(nullptr) || gAborting > 0;
    CHECK(shutting_down);
  }
}

}  // namespace art
