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

#include "jni/jni_internal.h"

#include <pthread.h>

#include "common_runtime_test.h"
#include "gc/heap.h"
#include "java_vm_ext.h"
#include "runtime.h"

namespace art HIDDEN {

class JavaVmExtTest : public CommonRuntimeTest {
 protected:
  JavaVmExtTest() {
    this->use_boot_image_ = true;  // Make the Runtime creation cheaper.
  }

  void SetUp() override {
    CommonRuntimeTest::SetUp();

    vm_ = Runtime::Current()->GetJavaVM();
  }


  void TearDown() override {
    CommonRuntimeTest::TearDown();
  }

  JavaVMExt* vm_;
};

TEST_F(JavaVmExtTest, JNI_GetDefaultJavaVMInitArgs) {
  jint err = JNI_GetDefaultJavaVMInitArgs(nullptr);
  EXPECT_EQ(JNI_ERR, err);
}

TEST_F(JavaVmExtTest, JNI_GetCreatedJavaVMs) {
  JavaVM* vms_buf[1];
  jsize num_vms;
  jint ok = JNI_GetCreatedJavaVMs(vms_buf, arraysize(vms_buf), &num_vms);
  EXPECT_EQ(JNI_OK, ok);
  EXPECT_EQ(1, num_vms);
  EXPECT_EQ(vms_buf[0], vm_);
}

static bool gSmallStack = false;
static bool gAsDaemon = false;

static void* attach_current_thread_callback([[maybe_unused]] void* arg) {
  JavaVM* vms_buf[1];
  jsize num_vms;
  JNIEnv* env;
  jint ok = JNI_GetCreatedJavaVMs(vms_buf, arraysize(vms_buf), &num_vms);
  EXPECT_EQ(JNI_OK, ok);
  if (ok == JNI_OK) {
    if (!gAsDaemon) {
      ok = vms_buf[0]->AttachCurrentThread(&env, nullptr);
    } else {
      ok = vms_buf[0]->AttachCurrentThreadAsDaemon(&env, nullptr);
    }
    // TODO: Find a way to test with exact SMALL_STACK value, for which we would bail. The pthreads
    //       spec says that the stack size argument is a lower bound, and bionic currently gives us
    //       a chunk more on arm64.
    if (!gSmallStack) {
      EXPECT_EQ(JNI_OK, ok);
    }
    if (ok == JNI_OK) {
      ok = vms_buf[0]->DetachCurrentThread();
      EXPECT_EQ(JNI_OK, ok);
    }
  }
  return nullptr;
}

TEST_F(JavaVmExtTest, AttachCurrentThread) {
  pthread_t pthread;
  const char* reason = __PRETTY_FUNCTION__;
  gSmallStack = false;
  gAsDaemon = false;
  CHECK_PTHREAD_CALL(pthread_create, (&pthread, nullptr, attach_current_thread_callback,
      nullptr), reason);
  void* ret_val;
  CHECK_PTHREAD_CALL(pthread_join, (pthread, &ret_val), reason);
  EXPECT_EQ(ret_val, nullptr);
}

TEST_F(JavaVmExtTest, AttachCurrentThreadAsDaemon) {
  pthread_t pthread;
  const char* reason = __PRETTY_FUNCTION__;
  gSmallStack = false;
  gAsDaemon = true;
  CHECK_PTHREAD_CALL(pthread_create, (&pthread, nullptr, attach_current_thread_callback,
      nullptr), reason);
  void* ret_val;
  CHECK_PTHREAD_CALL(pthread_join, (pthread, &ret_val), reason);
  EXPECT_EQ(ret_val, nullptr);
}

TEST_F(JavaVmExtTest, AttachCurrentThread_SmallStack) {
  TEST_DISABLED_FOR_MEMORY_TOOL();  // b/123500163
  pthread_t pthread;
  pthread_attr_t attr;
  const char* reason = __PRETTY_FUNCTION__;
  gSmallStack = true;
  gAsDaemon = false;
  CHECK_PTHREAD_CALL(pthread_attr_init, (&attr), reason);
  CHECK_PTHREAD_CALL(pthread_attr_setstacksize, (&attr, PTHREAD_STACK_MIN), reason);
  CHECK_PTHREAD_CALL(pthread_create, (&pthread, &attr, attach_current_thread_callback,
      nullptr), reason);
  CHECK_PTHREAD_CALL(pthread_attr_destroy, (&attr), reason);
  void* ret_val;
  CHECK_PTHREAD_CALL(pthread_join, (pthread, &ret_val), reason);
  EXPECT_EQ(ret_val, nullptr);
}

TEST_F(JavaVmExtTest, DetachCurrentThread) {
  JNIEnv* env;
  jint ok = vm_->AttachCurrentThread(&env, nullptr);
  ASSERT_EQ(JNI_OK, ok);
  ok = vm_->DetachCurrentThread();
  EXPECT_EQ(JNI_OK, ok);

  jint err = vm_->DetachCurrentThread();
  EXPECT_EQ(JNI_ERR, err);
}

class JavaVmExtStackTraceTest : public JavaVmExtTest {
 protected:
  void SetUpRuntimeOptions(RuntimeOptions* options) override {
    options->emplace_back("-XX:GlobalRefAllocStackTraceLimit=50000", nullptr);
  }
};

TEST_F(JavaVmExtStackTraceTest, TestEnableDisable) {
  ASSERT_FALSE(Runtime::Current()->GetHeap()->IsAllocTrackingEnabled());

  JNIEnv* env;
  jint ok = vm_->AttachCurrentThread(&env, nullptr);
  ASSERT_EQ(JNI_OK, ok);

  std::vector<jobject> global_refs_;
  jobject local_ref = env->NewStringUTF("Hello");
  for (size_t i = 0; i < 2000; ++i) {
    global_refs_.push_back(env->NewGlobalRef(local_ref));
  }

  EXPECT_TRUE(Runtime::Current()->GetHeap()->IsAllocTrackingEnabled());

  for (jobject global_ref : global_refs_) {
    env->DeleteGlobalRef(global_ref);
  }

  EXPECT_FALSE(Runtime::Current()->GetHeap()->IsAllocTrackingEnabled());

  global_refs_.clear();
  for (size_t i = 0; i < 2000; ++i) {
    global_refs_.push_back(env->NewGlobalRef(local_ref));
  }

  EXPECT_TRUE(Runtime::Current()->GetHeap()->IsAllocTrackingEnabled());

  for (jobject global_ref : global_refs_) {
    env->DeleteGlobalRef(global_ref);
  }

  EXPECT_FALSE(Runtime::Current()->GetHeap()->IsAllocTrackingEnabled());

  ok = vm_->DetachCurrentThread();
  EXPECT_EQ(JNI_OK, ok);
}

}  // namespace art
