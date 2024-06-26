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

#ifndef ART_RUNTIME_JNI_JNI_ENV_EXT_INL_H_
#define ART_RUNTIME_JNI_JNI_ENV_EXT_INL_H_

#include "jni_env_ext.h"

#include "local_reference_table-inl.h"
#include "mirror/object.h"

namespace art HIDDEN {

template<typename T>
inline T JNIEnvExt::AddLocalReference(ObjPtr<mirror::Object> obj) {
  std::string error_msg;
  jobject ref = reinterpret_cast<jobject>(locals_.Add(obj, &error_msg));
  if (UNLIKELY(ref == nullptr)) {
    // This is really unexpected if we allow resizing LRTs...
    LOG(FATAL) << error_msg;
    UNREACHABLE();
  }

  // TODO: fix this to understand PushLocalFrame, so we can turn it on.
  if (false) {
    if (check_jni_) {
      size_t entry_count = locals_.Capacity();
      if (entry_count > 16) {
        locals_.Dump(LOG_STREAM(WARNING) << "Warning: more than 16 JNI local references: "
                                        << entry_count << " (most recent was a "
                                        << mirror::Object::PrettyTypeOf(obj) << ")\n");
      // TODO: LOG(FATAL) in a later release?
      }
    }
  }

  return reinterpret_cast<T>(ref);
}

inline void JNIEnvExt::UpdateLocal(IndirectRef iref, ObjPtr<mirror::Object> obj) {
  locals_.Update(iref, obj);
}

}  // namespace art

#endif  // ART_RUNTIME_JNI_JNI_ENV_EXT_INL_H_
