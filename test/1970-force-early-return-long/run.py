#!/bin/bash
#
# Copyright 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


def run(ctx, args):
  # On RI we need to turn class-load tests off since those events are buggy around
  # pop-frame (see b/116003018).
  test_args = ["DISABLE_CLASS_LOAD_TESTS"] if args.jvm else []

  ctx.default_run(args, jvmti=True, test_args=test_args)

  # The RI has changed how the Thread toString conversion is done.
  if args.jvm:
    ctx.expected_stdout = ctx.expected_stdout.with_suffix(".jvm.txt")
