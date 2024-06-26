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

#ifndef ART_RUNTIME_MIRROR_OBJECT_ARRAY_INL_H_
#define ART_RUNTIME_MIRROR_OBJECT_ARRAY_INL_H_

#include "base/globals.h"
#include "object_array.h"

#include <string>

#include "android-base/stringprintf.h"

#include "array-inl.h"
#include "class.h"
#include "obj_ptr-inl.h"
#include "object-inl.h"
#include "read_barrier-inl.h"
#include "runtime.h"
#include "thread-current-inl.h"
#include "write_barrier-inl.h"

namespace art HIDDEN {
namespace mirror {

template<class T> template<VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline ObjPtr<T> ObjectArray<T>::Get(int32_t i) {
  if (!CheckIsValidIndex<kVerifyFlags>(i)) {
    DCHECK(Thread::Current()->IsExceptionPending());
    return nullptr;
  }
  return GetFieldObject<T, kVerifyFlags, kReadBarrierOption>(OffsetOfElement(i));
}

template<class T> template<VerifyObjectFlags kVerifyFlags>
inline bool ObjectArray<T>::CheckAssignable(ObjPtr<T> object) {
  if (object != nullptr) {
    ObjPtr<Class> element_class = GetClass<kVerifyFlags>()->GetComponentType();
    if (UNLIKELY(!object->InstanceOf(element_class))) {
      ThrowArrayStoreException(object);
      return false;
    }
  }
  return true;
}

template<class T>
inline void ObjectArray<T>::Set(int32_t i, ObjPtr<T> object) {
  if (Runtime::Current()->IsActiveTransaction()) {
    Set<true>(i, object);
  } else {
    Set<false>(i, object);
  }
}

template<class T>
template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline void ObjectArray<T>::Set(int32_t i, ObjPtr<T> object) {
  if (CheckIsValidIndex(i) && CheckAssignable<kVerifyFlags>(object)) {
    SetFieldObject<kTransactionActive, kCheckTransaction, kVerifyFlags>(OffsetOfElement(i), object);
  } else {
    DCHECK(Thread::Current()->IsExceptionPending());
  }
}

template<class T>
template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline void ObjectArray<T>::SetWithoutChecks(int32_t i, ObjPtr<T> object) {
  DCHECK(CheckIsValidIndex<kVerifyFlags>(i)) << i << " vs " << GetLength();
  DCHECK(CheckAssignable<static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis)>(object));
  SetFieldObject<kTransactionActive, kCheckTransaction, kVerifyFlags>(OffsetOfElement(i), object);
}

template<class T>
template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline void ObjectArray<T>::SetWithoutChecksAndWriteBarrier(int32_t i, ObjPtr<T> object) {
  DCHECK(CheckIsValidIndex<kVerifyFlags>(i)) << i << " vs " << GetLength();
  // TODO:  enable this check. It fails when writing the image in ImageWriter::FixupObjectArray.
  // DCHECK(CheckAssignable(object));
  SetFieldObjectWithoutWriteBarrier<kTransactionActive, kCheckTransaction, kVerifyFlags>(
      OffsetOfElement(i), object);
}

template<class T> template<VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline ObjPtr<T> ObjectArray<T>::GetWithoutChecks(int32_t i) {
  DCHECK(CheckIsValidIndex(i)) << i << " vs " << GetLength();
  return GetFieldObject<T, kVerifyFlags, kReadBarrierOption>(OffsetOfElement(i));
}

template<class T>
inline void ObjectArray<T>::AssignableMemmove(int32_t dst_pos,
                                              ObjPtr<ObjectArray<T>> src,
                                              int32_t src_pos,
                                              int32_t count) {
  if (kIsDebugBuild) {
    for (int i = 0; i < count; ++i) {
      // The get will perform the VerifyObject.
      src->GetWithoutChecks(src_pos + i);
    }
  }
  // Perform the memmove using int memmove then perform the write barrier.
  static_assert(sizeof(HeapReference<T>) == sizeof(uint32_t),
                "art::mirror::HeapReference<T> and uint32_t have different sizes.");
  // TODO: Optimize this later?
  // We can't use memmove since it does not handle read barriers and may do by per byte copying.
  // See b/32012820.
  const bool copy_forward = (src != this) || (dst_pos < src_pos) || (dst_pos - src_pos >= count);
  if (copy_forward) {
    // Forward copy.
    bool baker_non_gray_case = false;
    if (gUseReadBarrier && kUseBakerReadBarrier) {
      uintptr_t fake_address_dependency;
      if (!ReadBarrier::IsGray(src.Ptr(), &fake_address_dependency)) {
        baker_non_gray_case = true;
        DCHECK_EQ(fake_address_dependency, 0U);
        src.Assign(reinterpret_cast<ObjectArray<T>*>(
            reinterpret_cast<uintptr_t>(src.Ptr()) | fake_address_dependency));
        for (int i = 0; i < count; ++i) {
          // We can skip the RB here because 'src' isn't gray.
          ObjPtr<T> obj = src->template GetWithoutChecks<kDefaultVerifyFlags, kWithoutReadBarrier>(
              src_pos + i);
          SetWithoutChecksAndWriteBarrier<false>(dst_pos + i, obj);
        }
      }
    }
    if (!baker_non_gray_case) {
      for (int i = 0; i < count; ++i) {
        // We need a RB here. ObjectArray::GetWithoutChecks() contains a RB.
        ObjPtr<T> obj = src->GetWithoutChecks(src_pos + i);
        SetWithoutChecksAndWriteBarrier<false>(dst_pos + i, obj);
      }
    }
  } else {
    // Backward copy.
    bool baker_non_gray_case = false;
    if (gUseReadBarrier && kUseBakerReadBarrier) {
      uintptr_t fake_address_dependency;
      if (!ReadBarrier::IsGray(src.Ptr(), &fake_address_dependency)) {
        baker_non_gray_case = true;
        DCHECK_EQ(fake_address_dependency, 0U);
        src.Assign(reinterpret_cast<ObjectArray<T>*>(
            reinterpret_cast<uintptr_t>(src.Ptr()) | fake_address_dependency));
        for (int i = count - 1; i >= 0; --i) {
          // We can skip the RB here because 'src' isn't gray.
          ObjPtr<T> obj = src->template GetWithoutChecks<kDefaultVerifyFlags, kWithoutReadBarrier>(
              src_pos + i);
          SetWithoutChecksAndWriteBarrier<false>(dst_pos + i, obj);
        }
      }
    }
    if (!baker_non_gray_case) {
      for (int i = count - 1; i >= 0; --i) {
        // We need a RB here. ObjectArray::GetWithoutChecks() contains a RB.
        ObjPtr<T> obj = src->GetWithoutChecks(src_pos + i);
        SetWithoutChecksAndWriteBarrier<false>(dst_pos + i, obj);
      }
    }
  }
  WriteBarrier::ForArrayWrite(this, dst_pos, count);
  if (kIsDebugBuild) {
    for (int i = 0; i < count; ++i) {
      // The get will perform the VerifyObject.
      GetWithoutChecks(dst_pos + i);
    }
  }
}

template<class T>
inline void ObjectArray<T>::AssignableMemcpy(int32_t dst_pos,
                                             ObjPtr<ObjectArray<T>> src,
                                             int32_t src_pos,
                                             int32_t count) {
  if (kIsDebugBuild) {
    for (int i = 0; i < count; ++i) {
      // The get will perform the VerifyObject.
      src->GetWithoutChecks(src_pos + i);
    }
  }
  // Perform the memmove using int memcpy then perform the write barrier.
  static_assert(sizeof(HeapReference<T>) == sizeof(uint32_t),
                "art::mirror::HeapReference<T> and uint32_t have different sizes.");
  // TODO: Optimize this later?
  // We can't use memmove since it does not handle read barriers and may do by per byte copying.
  // See b/32012820.
  bool baker_non_gray_case = false;
  if (gUseReadBarrier && kUseBakerReadBarrier) {
    uintptr_t fake_address_dependency;
    if (!ReadBarrier::IsGray(src.Ptr(), &fake_address_dependency)) {
      baker_non_gray_case = true;
      DCHECK_EQ(fake_address_dependency, 0U);
      src.Assign(reinterpret_cast<ObjectArray<T>*>(
          reinterpret_cast<uintptr_t>(src.Ptr()) | fake_address_dependency));
      for (int i = 0; i < count; ++i) {
        // We can skip the RB here because 'src' isn't gray.
        ObjPtr<Object> obj =
            src->template GetWithoutChecks<kDefaultVerifyFlags, kWithoutReadBarrier>(src_pos + i);
        SetWithoutChecksAndWriteBarrier<false>(dst_pos + i, obj);
      }
    }
  }
  if (!baker_non_gray_case) {
    for (int i = 0; i < count; ++i) {
      // We need a RB here. ObjectArray::GetWithoutChecks() contains a RB.
      ObjPtr<T> obj = src->GetWithoutChecks(src_pos + i);
      SetWithoutChecksAndWriteBarrier<false>(dst_pos + i, obj);
    }
  }
  WriteBarrier::ForArrayWrite(this, dst_pos, count);
  if (kIsDebugBuild) {
    for (int i = 0; i < count; ++i) {
      // The get will perform the VerifyObject.
      GetWithoutChecks(dst_pos + i);
    }
  }
}

template<class T>
template<bool kTransactionActive>
inline void ObjectArray<T>::AssignableCheckingMemcpy(int32_t dst_pos,
                                                     ObjPtr<ObjectArray<T>> src,
                                                     int32_t src_pos,
                                                     int32_t count,
                                                     bool throw_exception) {
  DCHECK_NE(this, src)
      << "This case should be handled with memmove that handles overlaps correctly";
  // We want to avoid redundant IsAssignableFrom checks where possible, so we cache a class that
  // we know is assignable to the destination array's component type.
  ObjPtr<Class> dst_class = GetClass()->GetComponentType();
  ObjPtr<Class> lastAssignableElementClass = dst_class;

  ObjPtr<T> o = nullptr;
  int i = 0;
  bool baker_non_gray_case = false;
  if (gUseReadBarrier && kUseBakerReadBarrier) {
    uintptr_t fake_address_dependency;
    if (!ReadBarrier::IsGray(src.Ptr(), &fake_address_dependency)) {
      baker_non_gray_case = true;
      DCHECK_EQ(fake_address_dependency, 0U);
      src.Assign(reinterpret_cast<ObjectArray<T>*>(
          reinterpret_cast<uintptr_t>(src.Ptr()) | fake_address_dependency));
      for (; i < count; ++i) {
        // The follow get operations force the objects to be verified.
        // We can skip the RB here because 'src' isn't gray.
        o = src->template GetWithoutChecks<kDefaultVerifyFlags, kWithoutReadBarrier>(
            src_pos + i);
        if (o == nullptr) {
          // Null is always assignable.
          SetWithoutChecks<kTransactionActive>(dst_pos + i, nullptr);
        } else {
          // TODO: use the underlying class reference to avoid uncompression when not necessary.
          ObjPtr<Class> o_class = o->GetClass();
          if (LIKELY(lastAssignableElementClass == o_class)) {
            SetWithoutChecks<kTransactionActive>(dst_pos + i, o);
          } else if (LIKELY(dst_class->IsAssignableFrom(o_class))) {
            lastAssignableElementClass = o_class;
            SetWithoutChecks<kTransactionActive>(dst_pos + i, o);
          } else {
            // Can't put this element into the array, break to perform write-barrier and throw
            // exception.
            break;
          }
        }
      }
    }
  }
  if (!baker_non_gray_case) {
    for (; i < count; ++i) {
      // The follow get operations force the objects to be verified.
      // We need a RB here. ObjectArray::GetWithoutChecks() contains a RB.
      o = src->GetWithoutChecks(src_pos + i);
      if (o == nullptr) {
        // Null is always assignable.
        SetWithoutChecks<kTransactionActive>(dst_pos + i, nullptr);
      } else {
        // TODO: use the underlying class reference to avoid uncompression when not necessary.
        ObjPtr<Class> o_class = o->GetClass();
        if (LIKELY(lastAssignableElementClass == o_class)) {
          SetWithoutChecks<kTransactionActive>(dst_pos + i, o);
        } else if (LIKELY(dst_class->IsAssignableFrom(o_class))) {
          lastAssignableElementClass = o_class;
          SetWithoutChecks<kTransactionActive>(dst_pos + i, o);
        } else {
          // Can't put this element into the array, break to perform write-barrier and throw
          // exception.
          break;
        }
      }
    }
  }
  WriteBarrier::ForArrayWrite(this, dst_pos, count);
  if (UNLIKELY(i != count)) {
    std::string actualSrcType(mirror::Object::PrettyTypeOf(o));
    std::string dstType(PrettyTypeOf());
    Thread* self = Thread::Current();
    std::string msg = android::base::StringPrintf(
        "source[%d] of type %s cannot be stored in destination array of type %s",
        src_pos + i,
        actualSrcType.c_str(),
        dstType.c_str());
    if (throw_exception) {
      self->ThrowNewException("Ljava/lang/ArrayStoreException;", msg.c_str());
    } else {
      LOG(FATAL) << msg;
    }
  }
}

template<class T>
inline MemberOffset ObjectArray<T>::OffsetOfElement(int32_t i) {
  return MemberOffset(DataOffset(kHeapReferenceSize).Int32Value() + (i * kHeapReferenceSize));
}

template<class T> template<typename Visitor>
inline void ObjectArray<T>::VisitReferences(const Visitor& visitor) {
  const size_t length = static_cast<size_t>(GetLength());
  for (size_t i = 0; i < length; ++i) {
    visitor(this, OffsetOfElement(i), /* is_static= */ false);
  }
}

template<class T> template<typename Visitor>
inline void ObjectArray<T>::VisitReferences(const Visitor& visitor,
                                            MemberOffset begin,
                                            MemberOffset end) {
  const size_t length = static_cast<size_t>(GetLength());
  begin = std::max(begin, OffsetOfElement(0));
  end = std::min(end, OffsetOfElement(length));
  while (begin < end) {
    visitor(this, begin, /* is_static= */ false, /*is_obj_array*/ true);
    begin += kHeapReferenceSize;
  }
}

template <class T>
inline ConstObjPtrArrayIter<T> ObjectArray<T>::cbegin() const {
  return ConstObjPtrArrayIter<T>(this, 0);
}
template <class T>
inline ConstObjPtrArrayIter<T> ObjectArray<T>::cend() const {
  return ConstObjPtrArrayIter<T>(this, GetLength());
}
template <class T>
inline ConstHandleArrayIter<T> ObjectArray<T>::cbegin(const Handle<ObjectArray<T>>& h_this) {
  return ConstHandleArrayIter<T>(h_this, 0);
}
template <class T>
inline ConstHandleArrayIter<T> ObjectArray<T>::cend(const Handle<ObjectArray<T>>& h_this) {
  return ConstHandleArrayIter<T>(h_this, h_this->GetLength());
}

template <class T>
inline ObjPtrArrayIter<T> ObjectArray<T>::begin() {
  return ObjPtrArrayIter<T>(this, 0);
}
template <class T>
inline ObjPtrArrayIter<T> ObjectArray<T>::end() {
  return ObjPtrArrayIter<T>(this, GetLength());
}
template <class T>
inline HandleArrayIter<T> ObjectArray<T>::begin(Handle<ObjectArray<T>>& h_this) {
  return HandleArrayIter<T>(h_this, 0);
}
template <class T>
inline HandleArrayIter<T> ObjectArray<T>::end(Handle<ObjectArray<T>>& h_this) {
  return HandleArrayIter<T>(h_this, h_this->GetLength());
}

template<typename T, typename C>
inline void ArrayIter<T, C>::CheckIdx() const {
  if (kIsDebugBuild) {
    Locks::mutator_lock_->AssertSharedHeld(Thread::Current());
  }
  DCHECK_LE(0, idx_);
  DCHECK_LE(idx_, array_->GetLength());
}

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_OBJECT_ARRAY_INL_H_
