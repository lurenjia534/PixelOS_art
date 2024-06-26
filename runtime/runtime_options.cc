/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "runtime_options.h"

#include <memory>

#include "base/fast_exit.h"
#include "base/sdk_version.h"
#include "base/utils.h"
#include "debugger.h"
#include "gc/heap.h"
#include "monitor.h"
#include "runtime.h"
#include "thread_list.h"
#include "trace.h"

namespace art HIDDEN {

// Specify storage for the RuntimeOptions keys.

#define RUNTIME_OPTIONS_KEY(Type, Name, ...) const RuntimeArgumentMap::Key<Type> RuntimeArgumentMap::Name {__VA_ARGS__};
#include "runtime_options.def"

}  // namespace art
