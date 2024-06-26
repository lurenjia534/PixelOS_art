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

#include "common_runtime_test.h"

#include "base/memory_tool.h"
#include "class_linker-inl.h"
#include "class_root-inl.h"
#include "handle_scope-inl.h"
#include "mirror/object-inl.h"
#include "mirror/object_array-alloc-inl.h"
#include "mirror/object_array-inl.h"
#include "mirror/string.h"
#include "runtime.h"
#include "scoped_thread_state_change-inl.h"
#include "verification-inl.h"

namespace art HIDDEN {
namespace gc {

class VerificationTest : public CommonRuntimeTest {
 protected:
  VerificationTest() {
    use_boot_image_ = true;  // Make the Runtime creation cheaper.
  }

  template <class T>
  ObjPtr<mirror::ObjectArray<T>> AllocObjectArray(Thread* self, size_t length)
      REQUIRES_SHARED(Locks::mutator_lock_) {
    return mirror::ObjectArray<T>::Alloc(
        self,
        GetClassRoot<mirror::ObjectArray<mirror::Object>>(),
        length);
  }
};

TEST_F(VerificationTest, IsValidHeapObjectAddress) {
  ScopedObjectAccess soa(Thread::Current());
  const Verification* const v = Runtime::Current()->GetHeap()->GetVerification();
  EXPECT_FALSE(v->IsValidHeapObjectAddress(reinterpret_cast<const void*>(1)));
  EXPECT_FALSE(v->IsValidHeapObjectAddress(reinterpret_cast<const void*>(4)));
  EXPECT_FALSE(v->IsValidHeapObjectAddress(nullptr));
  VariableSizedHandleScope hs(soa.Self());
  Handle<mirror::String> string(
      hs.NewHandle(mirror::String::AllocFromModifiedUtf8(soa.Self(), "test")));
  EXPECT_TRUE(v->IsValidHeapObjectAddress(string.Get()));
  // Address in the heap that isn't aligned.
  const void* unaligned_address =
      reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(string.Get()) + 1);
  EXPECT_TRUE(v->IsAddressInHeapSpace(unaligned_address));
  EXPECT_FALSE(v->IsValidHeapObjectAddress(unaligned_address));
  EXPECT_TRUE(v->IsValidHeapObjectAddress(string->GetClass()));
  const uintptr_t uint_klass = reinterpret_cast<uintptr_t>(string->GetClass());
  // Not actually a valid object but the verification can't know that. Guaranteed to be inside a
  // heap space.
  EXPECT_TRUE(v->IsValidHeapObjectAddress(
      reinterpret_cast<const void*>(uint_klass + kObjectAlignment)));
  EXPECT_FALSE(v->IsValidHeapObjectAddress(
      reinterpret_cast<const void*>(&uint_klass)));
}

TEST_F(VerificationTest, IsValidClassOrNotInHeap) {
  ScopedObjectAccess soa(Thread::Current());
  VariableSizedHandleScope hs(soa.Self());
  Handle<mirror::String> string(
      hs.NewHandle(mirror::String::AllocFromModifiedUtf8(soa.Self(), "test")));
  const Verification* const v = Runtime::Current()->GetHeap()->GetVerification();
  EXPECT_FALSE(v->IsValidClass(reinterpret_cast<mirror::Class*>(1)));
  EXPECT_FALSE(v->IsValidClass(reinterpret_cast<mirror::Class*>(4)));
  EXPECT_FALSE(v->IsValidClass(nullptr));
  EXPECT_TRUE(v->IsValidClass(string->GetClass()));
  EXPECT_FALSE(v->IsValidClass(reinterpret_cast<mirror::Class*>(string.Get())));
}

TEST_F(VerificationTest, IsValidClassInHeap) {
  // Now that the String class is allocated in the non-moving space when the
  // runtime is running without a boot image (which is the case in this gtest),
  // and we run with AddressSanizer, it is possible that the (presumably
  // invalid) memory location `uint_klass - kObjectAlignment` tested below is
  // poisoned when running with AddressSanizer. Disable this test in that case.
  TEST_DISABLED_FOR_MEMORY_TOOL();
  ScopedObjectAccess soa(Thread::Current());
  VariableSizedHandleScope hs(soa.Self());
  Handle<mirror::String> string(
      hs.NewHandle(mirror::String::AllocFromModifiedUtf8(soa.Self(), "test")));
  const Verification* const v = Runtime::Current()->GetHeap()->GetVerification();
  uintptr_t uint_klass = reinterpret_cast<uintptr_t>(string->GetClass());
  EXPECT_FALSE(v->IsValidClass(reinterpret_cast<mirror::Class*>(uint_klass - kObjectAlignment)));
  EXPECT_FALSE(v->IsValidClass(reinterpret_cast<mirror::Class*>(&uint_klass)));
}

TEST_F(VerificationTest, DumpInvalidObjectInfo) {
  ScopedLogSeverity sls(LogSeverity::INFO);
  ScopedObjectAccess soa(Thread::Current());
  Runtime* const runtime = Runtime::Current();
  VariableSizedHandleScope hs(soa.Self());
  const Verification* const v = runtime->GetHeap()->GetVerification();
  LOG(INFO) << v->DumpObjectInfo(reinterpret_cast<const void*>(1), "obj");
  LOG(INFO) << v->DumpObjectInfo(reinterpret_cast<const void*>(4), "obj");
  LOG(INFO) << v->DumpObjectInfo(nullptr, "obj");
}

TEST_F(VerificationTest, DumpValidObjectInfo) {
  // Now that the String class is allocated in the non-moving space when the
  // runtime is running without a boot image (which is the case in this gtest),
  // and we run with AddressSanizer, it is possible that the calls to
  // Verification::DumpObjectInfo below involving the String class object
  // (`string->GetClass()`, `uint_klass`, etc.) access poisoned memory when they
  // call Verification::DumpRAMAroundAddress. Disable this test in that case.
  TEST_DISABLED_FOR_MEMORY_TOOL();
  ScopedLogSeverity sls(LogSeverity::INFO);
  ScopedObjectAccess soa(Thread::Current());
  Runtime* const runtime = Runtime::Current();
  VariableSizedHandleScope hs(soa.Self());
  Handle<mirror::String> string(
      hs.NewHandle(mirror::String::AllocFromModifiedUtf8(soa.Self(), "obj")));
  Handle<mirror::ObjectArray<mirror::Object>> arr(
      hs.NewHandle(AllocObjectArray<mirror::Object>(soa.Self(), 256)));
  const Verification* const v = runtime->GetHeap()->GetVerification();
  LOG(INFO) << v->DumpObjectInfo(string.Get(), "test");
  LOG(INFO) << v->DumpObjectInfo(string->GetClass(), "obj");
  const uintptr_t uint_klass = reinterpret_cast<uintptr_t>(string->GetClass());
  LOG(INFO) << v->DumpObjectInfo(reinterpret_cast<const void*>(uint_klass - kObjectAlignment),
                                 "obj");
  LOG(INFO) << v->DumpObjectInfo(reinterpret_cast<const void*>(&uint_klass), "obj");
  LOG(INFO) << v->DumpObjectInfo(arr.Get(), "arr");
}

TEST_F(VerificationTest, LogHeapCorruption) {
  // Now that the String class is allocated in the non-moving space when the
  // runtime is running without a boot image (which is the case in this gtest),
  // and we run with AddressSanizer, it is possible that the call to
  // Verification::LogHeapCorruption below involving the String class object
  // (`string->GetClass()`) accesses poisoned memory when it calls
  // Verification::DumpRAMAroundAddress. Disable this test in that case.
  TEST_DISABLED_FOR_MEMORY_TOOL();
  ScopedLogSeverity sls(LogSeverity::INFO);
  ScopedObjectAccess soa(Thread::Current());
  Runtime* const runtime = Runtime::Current();
  VariableSizedHandleScope hs(soa.Self());
  Handle<mirror::String> string(
      hs.NewHandle(mirror::String::AllocFromModifiedUtf8(soa.Self(), "obj")));
  using ObjArray = mirror::ObjectArray<mirror::Object>;
  Handle<ObjArray> arr(
      hs.NewHandle(AllocObjectArray<mirror::Object>(soa.Self(), 256)));
  const Verification* const v = runtime->GetHeap()->GetVerification();
  arr->Set(0, string.Get());
  // Test normal cases.
  v->LogHeapCorruption(arr.Get(), ObjArray::DataOffset(kHeapReferenceSize), string.Get(), false);
  v->LogHeapCorruption(string.Get(), mirror::Object::ClassOffset(), string->GetClass(), false);
  // Test null holder cases.
  v->LogHeapCorruption(nullptr, MemberOffset(0), string.Get(), false);
  v->LogHeapCorruption(nullptr, MemberOffset(0), arr.Get(), false);
}

TEST_F(VerificationTest, FindPathFromRootSet) {
  ScopedLogSeverity sls(LogSeverity::INFO);
  ScopedObjectAccess soa(Thread::Current());
  Runtime* const runtime = Runtime::Current();
  VariableSizedHandleScope hs(soa.Self());
  Handle<mirror::ObjectArray<mirror::Object>> arr(
      hs.NewHandle(AllocObjectArray<mirror::Object>(soa.Self(), 256)));
  ObjPtr<mirror::String> str = mirror::String::AllocFromModifiedUtf8(soa.Self(), "obj");
  arr->Set(0, str);
  const Verification* const v = runtime->GetHeap()->GetVerification();
  std::string path = v->FirstPathFromRootSet(str);
  EXPECT_GT(path.length(), 0u);
  std::ostringstream oss;
  oss << arr.Get();
  EXPECT_NE(path.find(oss.str()), std::string::npos);
  LOG(INFO) << path;
}

}  // namespace gc
}  // namespace art
