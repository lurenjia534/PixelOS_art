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

public class Main {
  public static void main(String[] args) throws Exception {
    System.loadLibrary(args[0]);
    if (isAotCompiled(Main.class, "hasJit")) {
      throw new Error("This test must be run with --no-prebuild!");
    }
    if (!hasJit()) {
      return;
    }

    // Deoptimize callThrough() to ensure it can be JITed afterwards.
    deoptimizeNativeMethod(Main.class, "callThrough");
    testCompilationUseAndCollection();
    testMixedFramesOnStack();
  }

  public static void testCompilationUseAndCollection() {
    // Test that callThrough() can be JIT-compiled.
    assertFalse(hasJitCompiledEntrypoint(Main.class, "callThrough"));
    assertFalse(hasJitCompiledCode(Main.class, "callThrough"));
    ensureCompiledCallThroughEntrypoint(/* call */ true);
    assertTrue(hasJitCompiledEntrypoint(Main.class, "callThrough"));
    assertTrue(hasJitCompiledCode(Main.class, "callThrough"));

    // Use callThrough() once again now that the method has a JIT-compiled stub.
    callThrough(Main.class, "doNothing");

    // Test that GC with the JIT-compiled stub on the stack does not collect it.
    // Also tests stack walk over the JIT-compiled stub.
    callThrough(Main.class, "testGcWithCallThroughStubOnStack");

    // Test that the JNI compiled stub can actually be collected.
    testStubCanBeCollected();
  }

  public static void testGcWithCallThroughStubOnStack() {
    // Check that this method was called via JIT-compiled callThrough() stub.
    assertTrue(hasJitCompiledEntrypoint(Main.class, "callThrough"));
    // This assertion also exercises stack walk over the JIT-compiled callThrough() stub.
    assertTrue(new Throwable().getStackTrace()[1].getMethodName().equals("callThrough"));

    doJitGcsUntilFullJitGcIsScheduled();
    // The callThrough() on the stack above this method is using the compiled stub,
    // so the JIT GC should not remove the compiled code.
    jitGc();
    assertTrue(hasJitCompiledCode(Main.class, "callThrough"));
  }

  public static void testMixedFramesOnStack() {
    // Starts without a compiled JNI stub for callThrough().
    assertFalse(hasJitCompiledEntrypoint(Main.class, "callThrough"));
    assertFalse(hasJitCompiledCode(Main.class, "callThrough"));
    callThrough(Main.class, "testMixedFramesOnStackStage2");
    // Though the callThrough() is on the stack, that frame is using the GenericJNI
    // and does not prevent the collection of the JNI stub.
    testStubCanBeCollected();
  }

  public static void testMixedFramesOnStackStage2() {
    // We cannot assert that callThrough() has no JIT compiled stub as that check
    // may race against the compilation task. Just check the caller.
    assertTrue(new Throwable().getStackTrace()[1].getMethodName().equals("callThrough"));
    // Now ensure that the JNI stub is compiled and used.
    ensureCompiledCallThroughEntrypoint(/* call */ true);
    callThrough(Main.class, "testMixedFramesOnStackStage3");
  }

  public static void testMixedFramesOnStackStage3() {
    // Check that this method was called via JIT-compiled callThrough() stub.
    assertTrue(hasJitCompiledEntrypoint(Main.class, "callThrough"));
    // This assertion also exercises stack walk over the JIT-compiled callThrough() stub.
    assertTrue(new Throwable().getStackTrace()[1].getMethodName().equals("callThrough"));
    // For a good measure, try a JIT GC.
    jitGc();
  }

  public static void testStubCanBeCollected() {
    jitGc();  // JIT GC without callThrough() on the stack should collect the callThrough() stub.
    assertFalse(hasJitCompiledEntrypoint(Main.class, "callThrough"));
    assertFalse(hasJitCompiledCode(Main.class, "callThrough"));
  }

  public static void doJitGcsUntilFullJitGcIsScheduled() {
    // We enter with a compiled stub for callThrough() but we also need the entrypoint to be set.
    assertTrue(hasJitCompiledCode(Main.class, "callThrough"));
    ensureCompiledCallThroughEntrypoint(/* call */ true);
    // Perform JIT GC until the next GC is marked to do full collection.
    do {
      assertTrue(hasJitCompiledEntrypoint(Main.class, "callThrough"));
      callThrough(Main.class, "jitGc");  // JIT GC with callThrough() safely on the stack.
    } while (!isNextJitGcFull());
    // The JIT GC before the full collection resets entrypoints and waits to see
    // if the methods are still in use.
    assertFalse(hasJitCompiledEntrypoint(Main.class, "callThrough"));
    assertTrue(hasJitCompiledCode(Main.class, "callThrough"));
  }

  public static void ensureCompiledCallThroughEntrypoint(boolean call) {
    int count = 0;
    while (!hasJitCompiledEntrypoint(Main.class, "callThrough")) {
      // If `call` is true, also exercise the `callThrough()` method to increase hotness.
      // Ramp-up the number of calls we do up to 1 << 12.
      final int rampUpCutOff = 12;
      int limit = call ? 1 << Math.min(count, rampUpCutOff) : 0;
      for (int i = 0; i < limit; ++i) {
        callThrough(Main.class, "doNothing");
      }
      try {
        // Sleep to give a chance for the JIT to compile `callThrough` stub.
        // After the ramp-up phase, give the JIT even more time to compile.
        Thread.sleep(count >= rampUpCutOff ? 200 : 100);
      } catch (Exception e) {
        // Ignore
      }
      if (++count == 50) {
        throw new Error("TIMEOUT");
      }
    }
  }

  public static void assertTrue(boolean value) {
    if (!value) {
      throw new AssertionError("Expected true!");
    }
  }

  public static void assertFalse(boolean value) {
    if (value) {
      throw new AssertionError("Expected false!");
    }
  }

  public static void doNothing() { }
  public static void throwError() { throw new Error(); }

  // Note that the callThrough()'s shorty differs from shorties of the other native methods used
  // in this test (except deoptimizeNativeMethod) because of the return type `void.`
  public native static void callThrough(Class<?> cls, String methodName);
  public native static void deoptimizeNativeMethod(Class<?> cls, String methodName);

  public native static void jitGc();
  public native static boolean isNextJitGcFull();

  public native static boolean isAotCompiled(Class<?> cls, String methodName);
  public native static boolean hasJitCompiledEntrypoint(Class<?> cls, String methodName);
  public native static boolean hasJitCompiledCode(Class<?> cls, String methodName);
  private native static boolean hasJit();
}
