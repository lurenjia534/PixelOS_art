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

#include "oatdump_test.h"

namespace art {

// Disable tests on arm and arm64 as they are taking too long to run. b/27824283.
#define TEST_DISABLED_FOR_ARM_AND_ARM64() \
  TEST_DISABLED_FOR_ARM();                \
  TEST_DISABLED_FOR_ARM64();

TEST_P(OatDumpTest, TestDumpImage) {
  TEST_DISABLED_FOR_RISCV64();
  TEST_DISABLED_FOR_ARM_AND_ARM64();
  // TODO(b/334200225): Temporarily disable this test case for x86 and x86_64
  // till the disassembler issue is fixed.
  TEST_DISABLED_FOR_X86();
  TEST_DISABLED_FOR_X86_64();
  std::string error_msg;
  ASSERT_TRUE(
      Exec(GetParam(), kArgImage | kArgBcp | kArgIsa, {}, kExpectImage | kExpectOat | kExpectCode));
}

TEST_P(OatDumpTest, TestDumpOatBcp) {
  TEST_DISABLED_FOR_RISCV64();
  TEST_DISABLED_FOR_ARM_AND_ARM64();
  std::string error_msg;
  ASSERT_TRUE(Exec(GetParam(), kArgOatBcp | kArgDexBcp, {}, kExpectOat | kExpectCode));
}

}  // namespace art
