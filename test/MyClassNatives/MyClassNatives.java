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

import dalvik.annotation.optimization.CriticalNative;
import dalvik.annotation.optimization.FastNative;

/*
 * AUTOMATICALLY GENERATED FROM art/tools/mako-source-generator/...../MyClassNatives.java.mako
 *
 * The tool is however not available, so the changes now need to be done by hand.
 */
class MyClassNatives {

    // Normal native
    native void throwException();
    // Normal native
    synchronized native void synchronizedThrowException();
    // Normal native
    native void foo();
    // Normal native
    native int bar(int count);
    // Normal native
    static native int sbar(int count);
    // Normal native
    native int fooI(int x);
    // Normal native
    native int fooL(Object x);
    // Normal native
    native void fooI_V(int x);
    // Normal native
    native byte fooI_B(int x);
    // Normal native
    native char fooI_C(int x);
    // Normal native
    native short fooI_S(int x);
    // Normal native
    native boolean fooI_Z(int x);
    // Normal native
    native long fooI_J(int x);
    // Normal native
    native float fooI_F(int x);
    // Normal native
    native double fooI_D(int x);
    // Normal native
    native Object fooI_L(int x);
    // Normal native
    static native int sfooI(int x);
    // Normal native
    static native int sfooB(byte x);
    // Normal native
    static native int sfooC(char x);
    // Normal native
    static native int sfooS(short x);
    // Normal native
    static native int sfooZ(boolean x);
    // Normal native
    static native int sfooL(Object x);
    // Normal native
    native int fooII(int x, int y);
    // Normal native
    native long fooJJ(long x, long y);
    // Normal native
    native Object fooO(Object x);
    // Normal native
    native double fooDD(double x, double y);
    // Normal native
    synchronized native long fooJJ_synchronized(long x, long y);
    // Normal native
    native Object fooIOO(int x, Object y, Object z);
    // Normal native
    static native Object fooSIOO(int x, Object y, Object z);
    // Normal native
    static native int fooSII(int x, int y);
    // Normal native
    static native double fooSDD(double x, double y);
    // Normal native
    static synchronized native Object fooSSIOO(int x, Object y, Object z);
    // Normal native
    static native void arraycopy(Object src, int src_pos, Object dst, int dst_pos, int length);
    // Normal native
    native boolean compareAndSwapInt(Object obj, long offset, int expected, int newval);
    // Normal native
    static native int getText(long val1, Object obj1, long val2, Object obj2);
    // Normal native
    synchronized native Object[] getSinkPropertiesNative(String path);

    // Normal native
    native Class<?> instanceMethodThatShouldReturnClass();
    // Normal native
    static native Class<?> staticMethodThatShouldReturnClass();

    // Normal native
    native void instanceMethodThatShouldTakeClass(int i, Class<?> c);
    // Normal native
    static native void staticMethodThatShouldTakeClass(int i, Class<?> c);

    // TODO: These 3 seem like they could work for @CriticalNative as well if they were static.
    // Normal native
    native float checkFloats(float f1, float f2);
    // Normal native
    native void forceStackParameters(int i1, int i2, int i3, int i4, int i5, int i6, int i8, int i9,
                                     float f1, float f2, float f3, float f4, float f5, float f6,
                                     float f7, float f8, float f9);
    // Normal native
    native void checkParameterAlign(int i1, long l1);

    // Normal native
    native void maxParamNumber(Object o0, Object o1, Object o2, Object o3, Object o4, Object o5, Object o6, Object o7,
        Object o8, Object o9, Object o10, Object o11, Object o12, Object o13, Object o14, Object o15,
        Object o16, Object o17, Object o18, Object o19, Object o20, Object o21, Object o22, Object o23,
        Object o24, Object o25, Object o26, Object o27, Object o28, Object o29, Object o30, Object o31,
        Object o32, Object o33, Object o34, Object o35, Object o36, Object o37, Object o38, Object o39,
        Object o40, Object o41, Object o42, Object o43, Object o44, Object o45, Object o46, Object o47,
        Object o48, Object o49, Object o50, Object o51, Object o52, Object o53, Object o54, Object o55,
        Object o56, Object o57, Object o58, Object o59, Object o60, Object o61, Object o62, Object o63,
        Object o64, Object o65, Object o66, Object o67, Object o68, Object o69, Object o70, Object o71,
        Object o72, Object o73, Object o74, Object o75, Object o76, Object o77, Object o78, Object o79,
        Object o80, Object o81, Object o82, Object o83, Object o84, Object o85, Object o86, Object o87,
        Object o88, Object o89, Object o90, Object o91, Object o92, Object o93, Object o94, Object o95,
        Object o96, Object o97, Object o98, Object o99, Object o100, Object o101, Object o102, Object o103,
        Object o104, Object o105, Object o106, Object o107, Object o108, Object o109, Object o110, Object o111,
        Object o112, Object o113, Object o114, Object o115, Object o116, Object o117, Object o118, Object o119,
        Object o120, Object o121, Object o122, Object o123, Object o124, Object o125, Object o126, Object o127,
        Object o128, Object o129, Object o130, Object o131, Object o132, Object o133, Object o134, Object o135,
        Object o136, Object o137, Object o138, Object o139, Object o140, Object o141, Object o142, Object o143,
        Object o144, Object o145, Object o146, Object o147, Object o148, Object o149, Object o150, Object o151,
        Object o152, Object o153, Object o154, Object o155, Object o156, Object o157, Object o158, Object o159,
        Object o160, Object o161, Object o162, Object o163, Object o164, Object o165, Object o166, Object o167,
        Object o168, Object o169, Object o170, Object o171, Object o172, Object o173, Object o174, Object o175,
        Object o176, Object o177, Object o178, Object o179, Object o180, Object o181, Object o182, Object o183,
        Object o184, Object o185, Object o186, Object o187, Object o188, Object o189, Object o190, Object o191,
        Object o192, Object o193, Object o194, Object o195, Object o196, Object o197, Object o198, Object o199,
        Object o200, Object o201, Object o202, Object o203, Object o204, Object o205, Object o206, Object o207,
        Object o208, Object o209, Object o210, Object o211, Object o212, Object o213, Object o214, Object o215,
        Object o216, Object o217, Object o218, Object o219, Object o220, Object o221, Object o222, Object o223,
        Object o224, Object o225, Object o226, Object o227, Object o228, Object o229, Object o230, Object o231,
        Object o232, Object o233, Object o234, Object o235, Object o236, Object o237, Object o238, Object o239,
        Object o240, Object o241, Object o242, Object o243, Object o244, Object o245, Object o246, Object o247,
        Object o248, Object o249, Object o250, Object o251, Object o252, Object o253);

    // Normal native
    native void withoutImplementation();
    // Normal native
    native Object withoutImplementationRefReturn();
    // Normal native
    native static void staticWithoutImplementation();

    // Normal native
    native static void stackArgsIntsFirst(int i1, int i2, int i3, int i4, int i5, int i6, int i7,
        int i8, int i9, int i10, float f1, float f2, float f3, float f4, float f5, float f6,
        float f7, float f8, float f9, float f10);

    // Normal native
    native static void stackArgsFloatsFirst(float f1, float f2, float f3, float f4, float f5,
        float f6, float f7, float f8, float f9, float f10, int i1, int i2, int i3, int i4, int i5,
        int i6, int i7, int i8, int i9, int i10);

    // Normal native
    native static void stackArgsMixed(int i1, float f1, int i2, float f2, int i3, float f3, int i4,
        float f4, int i5, float f5, int i6, float f6, int i7, float f7, int i8, float f8, int i9,
        float f9, int i10, float f10);

    // Normal native
    static native double logD(double d);
    // Normal native
    static native float logF(float f);
    // Normal native
    static native boolean returnTrue();
    // Normal native
    static native boolean returnFalse();
    // Normal native
    static native int returnInt();
    // Normal native
    static native double returnDouble();
    // Normal native
    static native long returnLong();

    // Normal native
    static native int sfoo7FI(float f1, float f2, float f3, float f4, float f5, float f6,
        float f7, int i1);
    // Normal native
    static native int sfoo3F5DI(float f1, float f2, float f3, double d1, double d2, double d3,
        double d4, double d5, int i1);
    // Normal native
    static native int sfoo3F6DI(float f1, float f2, float f3, double d1, double d2, double d3,
        double d4, double d5, double d6, int i1);
    // Normal native
    native int fooL4I(Object o1, int i1, int i2, int i3, int i4);
    // Normal native
    native int fooL5I(Object o1, int i1, int i2, int i3, int i4, int i5);
    // Normal native
    native int fooL3IJC(Object o1, int i1, int i2, int i3, long l1, char c1);
    // Normal native
    native int fooL3IJCS(Object o1, int i1, int i2, int i3, long l1, char c1, short s1);
    // Normal native
    native int foo9F(float f1, float f2, float f3, float f4, float f5, float f6, float f7,
        float f8, float f9);
    // Normal native
    native int foo7FDF(float f1, float f2, float f3, float f4, float f5, float f6, float f7,
        double d1, float f8);
    // Normal native
    native int foo7FIFF(float f1, float f2, float f3, float f4, float f5, float f6, float f7,
        int i1, float f8, float f9);
    // Normal native
    native int foo7I(int i1, int i2, int i3, int i4, int i5, int i6, int i7);
    // Normal native
    native int foo5IJI(int i1, int i2, int i3, int i4, int i5, long l1, int i6);
    // Normal native
    native int foo5IFII(int i1, int i2, int i3, int i4, int i5, float f1, int i6, int i7);
    // Normal native
    native int foo2FL(float f1, float f2, Object o1);
    // Normal native
    native int fooFDL(float f1, double f2, Object o1);
    // Normal native
    native int foo3FL(float f1, float f2, float f3, Object o1);
    // Normal native
    native int foo2FIL(float f1, float f2, int i1, Object o1);
    // Normal native
    native int foo2IFL(int i1, int i2, float f1, Object o1);
    // Normal native
    native int fooICFL(int i1, char c1, float f1, Object o1);
    // Normal native
    native int fooICIL(int i1, char c1, int i2, Object o1);


    @FastNative
    native void throwException_Fast();
    @FastNative
    native void foo_Fast();
    @FastNative
    native int bar_Fast(int count);
    @FastNative
    static native int sbar_Fast(int count);
    @FastNative
    native int fooI_Fast(int x);
    @FastNative
    native int fooII_Fast(int x, int y);
    @FastNative
    native long fooJJ_Fast(long x, long y);
    @FastNative
    native Object fooO_Fast(Object x);
    @FastNative
    native double fooDD_Fast(double x, double y);
    @FastNative
    native Object fooIOO_Fast(int x, Object y, Object z);
    @FastNative
    static native Object fooSIOO_Fast(int x, Object y, Object z);
    @FastNative
    static native int fooSII_Fast(int x, int y);
    @FastNative
    static native double fooSDD_Fast(double x, double y);
    @FastNative
    static native void arraycopy_Fast(Object src, int src_pos, Object dst, int dst_pos, int length);
    @FastNative
    native boolean compareAndSwapInt_Fast(Object obj, long offset, int expected, int newval);
    @FastNative
    static native int getText_Fast(long val1, Object obj1, long val2, Object obj2);
    @FastNative
    native Object[] getSinkPropertiesNative_Fast(String path);

    @FastNative
    native Class<?> instanceMethodThatShouldReturnClass_Fast();
    @FastNative
    static native Class<?> staticMethodThatShouldReturnClass_Fast();

    @FastNative
    native void instanceMethodThatShouldTakeClass_Fast(int i, Class<?> c);
    @FastNative
    static native void staticMethodThatShouldTakeClass_Fast(int i, Class<?> c);

    // TODO: These 3 seem like they could work for @CriticalNative as well if they were static.
    @FastNative
    native float checkFloats_Fast(float f1, float f2);
    @FastNative
    native void forceStackParameters_Fast(int i1, int i2, int i3, int i4, int i5, int i6, int i8, int i9,
                                     float f1, float f2, float f3, float f4, float f5, float f6,
                                     float f7, float f8, float f9);
    @FastNative
    native void checkParameterAlign_Fast(int i1, long l1);

    @FastNative
    native void maxParamNumber_Fast(Object o0, Object o1, Object o2, Object o3, Object o4, Object o5, Object o6, Object o7,
        Object o8, Object o9, Object o10, Object o11, Object o12, Object o13, Object o14, Object o15,
        Object o16, Object o17, Object o18, Object o19, Object o20, Object o21, Object o22, Object o23,
        Object o24, Object o25, Object o26, Object o27, Object o28, Object o29, Object o30, Object o31,
        Object o32, Object o33, Object o34, Object o35, Object o36, Object o37, Object o38, Object o39,
        Object o40, Object o41, Object o42, Object o43, Object o44, Object o45, Object o46, Object o47,
        Object o48, Object o49, Object o50, Object o51, Object o52, Object o53, Object o54, Object o55,
        Object o56, Object o57, Object o58, Object o59, Object o60, Object o61, Object o62, Object o63,
        Object o64, Object o65, Object o66, Object o67, Object o68, Object o69, Object o70, Object o71,
        Object o72, Object o73, Object o74, Object o75, Object o76, Object o77, Object o78, Object o79,
        Object o80, Object o81, Object o82, Object o83, Object o84, Object o85, Object o86, Object o87,
        Object o88, Object o89, Object o90, Object o91, Object o92, Object o93, Object o94, Object o95,
        Object o96, Object o97, Object o98, Object o99, Object o100, Object o101, Object o102, Object o103,
        Object o104, Object o105, Object o106, Object o107, Object o108, Object o109, Object o110, Object o111,
        Object o112, Object o113, Object o114, Object o115, Object o116, Object o117, Object o118, Object o119,
        Object o120, Object o121, Object o122, Object o123, Object o124, Object o125, Object o126, Object o127,
        Object o128, Object o129, Object o130, Object o131, Object o132, Object o133, Object o134, Object o135,
        Object o136, Object o137, Object o138, Object o139, Object o140, Object o141, Object o142, Object o143,
        Object o144, Object o145, Object o146, Object o147, Object o148, Object o149, Object o150, Object o151,
        Object o152, Object o153, Object o154, Object o155, Object o156, Object o157, Object o158, Object o159,
        Object o160, Object o161, Object o162, Object o163, Object o164, Object o165, Object o166, Object o167,
        Object o168, Object o169, Object o170, Object o171, Object o172, Object o173, Object o174, Object o175,
        Object o176, Object o177, Object o178, Object o179, Object o180, Object o181, Object o182, Object o183,
        Object o184, Object o185, Object o186, Object o187, Object o188, Object o189, Object o190, Object o191,
        Object o192, Object o193, Object o194, Object o195, Object o196, Object o197, Object o198, Object o199,
        Object o200, Object o201, Object o202, Object o203, Object o204, Object o205, Object o206, Object o207,
        Object o208, Object o209, Object o210, Object o211, Object o212, Object o213, Object o214, Object o215,
        Object o216, Object o217, Object o218, Object o219, Object o220, Object o221, Object o222, Object o223,
        Object o224, Object o225, Object o226, Object o227, Object o228, Object o229, Object o230, Object o231,
        Object o232, Object o233, Object o234, Object o235, Object o236, Object o237, Object o238, Object o239,
        Object o240, Object o241, Object o242, Object o243, Object o244, Object o245, Object o246, Object o247,
        Object o248, Object o249, Object o250, Object o251, Object o252, Object o253);

    @FastNative
    native void withoutImplementation_Fast();
    @FastNative
    native Object withoutImplementationRefReturn_Fast();
    @FastNative
    native static void staticWithoutImplementation_Fast();

    @FastNative
    native static void stackArgsIntsFirst_Fast(int i1, int i2, int i3, int i4, int i5, int i6, int i7,
        int i8, int i9, int i10, float f1, float f2, float f3, float f4, float f5, float f6,
        float f7, float f8, float f9, float f10);

    @FastNative
    native static void stackArgsFloatsFirst_Fast(float f1, float f2, float f3, float f4, float f5,
        float f6, float f7, float f8, float f9, float f10, int i1, int i2, int i3, int i4, int i5,
        int i6, int i7, int i8, int i9, int i10);

    @FastNative
    native static void stackArgsMixed_Fast(int i1, float f1, int i2, float f2, int i3, float f3, int i4,
        float f4, int i5, float f5, int i6, float f6, int i7, float f7, int i8, float f8, int i9,
        float f9, int i10, float f10);

    @FastNative
    static native double logD_Fast(double d);
    @FastNative
    static native float logF_Fast(float f);
    @FastNative
    static native boolean returnTrue_Fast();
    @FastNative
    static native boolean returnFalse_Fast();
    @FastNative
    static native int returnInt_Fast();
    @FastNative
    static native double returnDouble_Fast();
    @FastNative
    static native long returnLong_Fast();

    @FastNative
    native boolean fooI_Z_Fast(int i);
    @FastNative
    native long fooI_J_Fast(int i);
    @FastNative
    native int fooICFL_Fast(int i1, char c1, float f1, Object o1);
    @FastNative
    native int foo2IFL_Fast(int i1, int i2, float f1, Object o1);
    @FastNative
    native int fooICIL_Fast(int i1, char c1, int i2, Object o1);
    @FastNative
    native int fooFDL_Fast(float f1, double d1, Object o1);
    @FastNative
    native int foo2FL_Fast(float f1, float f2, Object o1);
    @FastNative
    native int foo3FL_Fast(float f1, float f2, float f3, Object o1);
    @FastNative
    native int foo2FIL_Fast(float f1, float f2, int i1, Object o1);
    @FastNative
    native int foo7F_Fast(float f1, float f2, float f3, float f4, float f5, float f6, float f7);
    @FastNative
    native int foo3F5D_Fast(float f1, float f2, float f3, double d1, double d2, double d3,
        double d4, double d5);
    @FastNative
    native int foo3F6D_Fast(float f1, float f2, float f3, double d1, double d2, double d3,
        double d4, double d5, double d6);
    @FastNative
    native int fooL5I_Fast(Object o1, int i1, int i2, int i3, int i4, int i5);
    @FastNative
    native int fooL3IJC_Fast(Object o1, int i1, int i2, int i3, long l1, char c1);
    @FastNative
    native int fooL3IJCS_Fast(Object o1, int i1, int i2, int i3, long l1, char c1, short s1);


    @CriticalNative
    static native int sbar_Critical(int count);
    @CriticalNative
    static native int fooSII_Critical(int x, int y);
    @CriticalNative
    static native double fooSDD_Critical(double x, double y);

    @CriticalNative
    native static void staticWithoutImplementation_Critical();

    @CriticalNative
    native static void stackArgsIntsFirst_Critical(int i1, int i2, int i3, int i4, int i5, int i6, int i7,
        int i8, int i9, int i10, float f1, float f2, float f3, float f4, float f5, float f6,
        float f7, float f8, float f9, float f10);

    @CriticalNative
    native static void stackArgsFloatsFirst_Critical(float f1, float f2, float f3, float f4, float f5,
        float f6, float f7, float f8, float f9, float f10, int i1, int i2, int i3, int i4, int i5,
        int i6, int i7, int i8, int i9, int i10);

    @CriticalNative
    native static void stackArgsMixed_Critical(int i1, float f1, int i2, float f2, int i3, float f3, int i4,
        float f4, int i5, float f5, int i6, float f6, int i7, float f7, int i8, float f8, int i9,
        float f9, int i10, float f10);

    @CriticalNative
    static native double logD_Critical(double d);
    @CriticalNative
    static native float logF_Critical(float f);
    @CriticalNative
    static native boolean returnTrue_Critical();
    @CriticalNative
    static native boolean returnFalse_Critical();
    @CriticalNative
    static native int returnInt_Critical();
    @CriticalNative
    static native double returnDouble_Critical();
    @CriticalNative
    static native long returnLong_Critical();

    @CriticalNative
    static native int foo7F_Critical(float f1, float f2, float f3, float f4, float f5, float f6,
        float f7);
    @CriticalNative
    static native int foo3F5D_Critical(float f1, float f2, float f3, double d1, double d2,
        double d3, double d4, double d5);
    @CriticalNative
    static native int foo3F6D_Critical(float f1, float f2, float f3, double d1, double d2,
        double d3, double d4, double d5, double d6);


    // Check for @FastNative/@CriticalNative annotation presence [or lack of presence].
    public static native void normalNative();
    @FastNative
    public static native void fastNative();
    @CriticalNative
    public static native void criticalNative();
}
