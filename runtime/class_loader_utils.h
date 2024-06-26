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

#ifndef ART_RUNTIME_CLASS_LOADER_UTILS_H_
#define ART_RUNTIME_CLASS_LOADER_UTILS_H_

#include "art_field-inl.h"
#include "base/locks.h"
#include "base/macros.h"
#include "handle_scope.h"
#include "jni/jni_internal.h"
#include "mirror/class_loader.h"
#include "mirror/object-inl.h"
#include "mirror/object.h"
#include "native/dalvik_system_DexFile.h"
#include "scoped_thread_state_change-inl.h"
#include "well_known_classes-inl.h"

namespace art HIDDEN {

// Returns true if the given class loader derives from BaseDexClassLoader.
inline bool IsInstanceOfBaseDexClassLoader(Handle<mirror::ClassLoader> class_loader)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  return class_loader->InstanceOf(WellKnownClasses::dalvik_system_BaseDexClassLoader.Get());
}

// Returns true if the given class loader is either a PathClassLoader or a DexClassLoader.
// (they both have the same behaviour with respect to class lookup order)
inline bool IsPathOrDexClassLoader(Handle<mirror::ClassLoader> class_loader)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ObjPtr<mirror::Class> class_loader_class = class_loader->GetClass();
  return (class_loader_class == WellKnownClasses::dalvik_system_PathClassLoader) ||
         (class_loader_class == WellKnownClasses::dalvik_system_DexClassLoader);
}

// Returns true if the given class loader is an InMemoryDexClassLoader.
inline bool IsInMemoryDexClassLoader(Handle<mirror::ClassLoader> class_loader)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ObjPtr<mirror::Class> class_loader_class = class_loader->GetClass();
  return (class_loader_class == WellKnownClasses::dalvik_system_InMemoryDexClassLoader);
}

inline bool IsDelegateLastClassLoader(Handle<mirror::ClassLoader> class_loader)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ObjPtr<mirror::Class> class_loader_class = class_loader->GetClass();
  return class_loader_class == WellKnownClasses::dalvik_system_DelegateLastClassLoader;
}

// Visit the DexPathList$Element instances in the given classloader with the given visitor.
// Constraints on the visitor:
//   * The visitor should return true to continue visiting more Elements.
//   * The last argument of the visitor is an out argument of RetType. It will be returned
//     when the visitor ends the visit (by returning false).
// This function assumes that the given classloader is a subclass of BaseDexClassLoader!
template <typename Visitor, typename RetType>
inline RetType VisitClassLoaderDexElements(Thread* self,
                                           Handle<mirror::ClassLoader> class_loader,
                                           Visitor fn,
                                           RetType defaultReturn)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ObjPtr<mirror::Object> dex_path_list =
      WellKnownClasses::dalvik_system_BaseDexClassLoader_pathList->GetObject(class_loader.Get());
  if (dex_path_list != nullptr) {
    // DexPathList has an array dexElements of Elements[] which each contain a dex file.
    ObjPtr<mirror::Object> dex_elements_obj =
        WellKnownClasses::dalvik_system_DexPathList_dexElements->GetObject(dex_path_list);
    // Loop through each dalvik.system.DexPathList$Element's dalvik.system.DexFile and look
    // at the mCookie which is a DexFile vector.
    if (dex_elements_obj != nullptr) {
      StackHandleScope<1> hs(self);
      Handle<mirror::ObjectArray<mirror::Object>> dex_elements =
          hs.NewHandle(dex_elements_obj->AsObjectArray<mirror::Object>());
      for (auto element : dex_elements.Iterate<mirror::Object>()) {
        if (element == nullptr) {
          // Should never happen, fail.
          break;
        }
        RetType ret_value;
        if (!fn(element, &ret_value)) {
          return ret_value;
        }
      }
    }
  }
  return defaultReturn;
}

// Visit the DexFiles in the given classloader with the given visitor.
// Constraints on the visitor:
//   * The visitor should return true to continue visiting more DexFiles.
//   * The last argument of the visitor is an out argument of RetType. It will be returned
//     when the visitor ends the visit (by returning false).
// This function assumes that the given classloader is a subclass of BaseDexClassLoader!
template <typename Visitor, typename RetType>
inline RetType VisitClassLoaderDexFiles(Thread* self,
                                        Handle<mirror::ClassLoader> class_loader,
                                        Visitor fn,
                                        RetType defaultReturn)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ArtField* const cookie_field = WellKnownClasses::dalvik_system_DexFile_cookie;
  ArtField* const dex_file_field = WellKnownClasses::dalvik_system_DexPathList__Element_dexFile;
  if (dex_file_field == nullptr || cookie_field == nullptr) {
    return defaultReturn;
  }
  auto visit_dex_files = [&](ObjPtr<mirror::Object> element, RetType* ret)
      REQUIRES_SHARED(Locks::mutator_lock_) {
    ObjPtr<mirror::Object> dex_file = dex_file_field->GetObject(element);
    if (dex_file != nullptr) {
      StackHandleScope<1> hs(self);
      Handle<mirror::LongArray> long_array =
          hs.NewHandle(cookie_field->GetObject(dex_file)->AsLongArray());
      if (long_array == nullptr) {
        // This should never happen so log a warning.
        LOG(WARNING) << "Null DexFile::mCookie";
        *ret = defaultReturn;
        return true;
      }
      int32_t long_array_size = long_array->GetLength();
      // First element is the oat file.
      for (int32_t j = kDexFileIndexStart; j < long_array_size; ++j) {
        const DexFile* cp_dex_file = reinterpret_cast<const DexFile*>(static_cast<uintptr_t>(
            long_array->GetWithoutChecks(j)));
        RetType ret_value;
        if (!fn(cp_dex_file, /* out */ &ret_value)) {
          *ret = ret_value;
          return false;
        }
      }
    }
    return true;
  };

  return VisitClassLoaderDexElements(self, class_loader, visit_dex_files, defaultReturn);
}

// Simplified version of the above, w/o out argument.
template <typename Visitor>
inline void VisitClassLoaderDexFiles(Thread* self,
                                     Handle<mirror::ClassLoader> class_loader,
                                     Visitor fn)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  auto helper = [&fn](const art::DexFile* dex_file, void** ret)
      REQUIRES_SHARED(Locks::mutator_lock_) {
#ifdef __clang_analyzer__
    *ret = nullptr;
#else
    UNUSED(ret);
#endif

    return fn(dex_file);
  };
  VisitClassLoaderDexFiles<decltype(helper), void*>(self,
                                                    class_loader,
                                                    helper,
                                                    /* defaultReturn= */ nullptr);
}

}  // namespace art

#endif  // ART_RUNTIME_CLASS_LOADER_UTILS_H_
