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

#ifndef ART_RUNTIME_BASE_TIMING_LOGGER_H_
#define ART_RUNTIME_BASE_TIMING_LOGGER_H_

#include "base/locks.h"
#include "base/macros.h"
#include "base/time_utils.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace art HIDDEN {
class TimingLogger;

class CumulativeLogger {
 public:
  explicit CumulativeLogger(const std::string& name);
  ~CumulativeLogger();
  void Start();
  void End() REQUIRES(!GetLock());
  void Reset() REQUIRES(!GetLock());
  void Dump(std::ostream& os) const REQUIRES(!GetLock());
  uint64_t GetTotalNs() const {
    return GetTotalTime() * kAdjust;
  }
  // Allow the name to be modified, particularly when the cumulative logger is a field within a
  // parent class that is unable to determine the "name" of a sub-class.
  void SetName(const std::string& name) REQUIRES(!GetLock());
  void AddLogger(const TimingLogger& logger) REQUIRES(!GetLock());
  size_t GetIterations() const REQUIRES(!GetLock());

 private:
  class CumulativeTime {
   public:
    CumulativeTime(const char* name, uint64_t time) : name_(name), time_(time) {}
    void Add(uint64_t time) { time_ += time; }
    const char* Name() const { return name_; }
    uint64_t Sum() const { return time_; }
    // Compare addresses of names for sorting.
    bool operator< (const CumulativeTime& ct) const {
      return std::less<const char*>()(name_, ct.name_);
    }

   private:
    const char* name_;
    uint64_t time_;
  };

  void DumpAverages(std::ostream &os) const REQUIRES(GetLock());
  void AddPair(const char* label, uint64_t delta_time) REQUIRES(GetLock());
  uint64_t GetTotalTime() const {
    return total_time_;
  }

  Mutex* GetLock() const {
    return lock_.get();
  }

  static constexpr uint64_t kAdjust = 1000;
  // Use a vector to keep dirty memory to minimal number of pages. Using a
  // hashtable would be performant, but could lead to more dirty pages. Also, we
  // don't expect this vector to be too big.
  std::vector<CumulativeTime> cumulative_timers_ GUARDED_BY(GetLock());
  std::string name_;
  const std::string lock_name_;
  mutable std::unique_ptr<Mutex> lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  size_t iterations_ GUARDED_BY(GetLock());
  uint64_t total_time_;

  DISALLOW_COPY_AND_ASSIGN(CumulativeLogger);
};

// A timing logger that knows when a split starts for the purposes of logging tools, like systrace.
class TimingLogger {
 public:
  static constexpr size_t kIndexNotFound = static_cast<size_t>(-1);

  // Kind of timing we are going to do. We collect time at the nano second.
  enum class TimingKind {
    kMonotonic,
    kThreadCpu,
  };

  class Timing {
   public:
    Timing(TimingKind kind, const char* name) : name_(name) {
       switch (kind) {
        case TimingKind::kMonotonic:
          time_ = NanoTime();
          break;
        case TimingKind::kThreadCpu:
          time_ = ThreadCpuNanoTime();
          break;
       }
    }
    bool IsStartTiming() const {
      return !IsEndTiming();
    }
    bool IsEndTiming() const {
      return name_ == nullptr;
    }
    uint64_t GetTime() const {
      return time_;
    }
    const char* GetName() const {
      return name_;
    }

   private:
    uint64_t time_;
    const char* name_;
  };

  // Extra data that is only calculated when you call dump to prevent excess allocation.
  class TimingData {
   public:
    TimingData() = default;
    TimingData(TimingData&& other) noexcept = default;
    TimingData& operator=(TimingData&& other) noexcept = default;
    uint64_t GetTotalTime(size_t idx) {
      return data_[idx].total_time;
    }
    uint64_t GetExclusiveTime(size_t idx) {
      return data_[idx].exclusive_time;
    }

   private:
    // Each begin split has a total time and exclusive time. Exclusive time is total time - total
    // time of children nodes.
    struct CalculatedDataPoint {
      CalculatedDataPoint() : total_time(0), exclusive_time(0) {}
      uint64_t total_time;
      uint64_t exclusive_time;
    };
    std::vector<CalculatedDataPoint> data_;
    friend class TimingLogger;
  };

  EXPORT TimingLogger(const char* name,
                      bool precise,
                      bool verbose,
                      TimingKind kind = TimingKind::kMonotonic);
  EXPORT ~TimingLogger();
  // Verify that all open timings have related closed timings.
  void Verify();
  // Clears current timings and labels.
  void Reset();
  // Starts a timing.
  EXPORT void StartTiming(const char* new_split_label);
  // Ends the current timing.
  EXPORT void EndTiming();
  // End the current timing and start a new timing. Usage not recommended.
  void NewTiming(const char* new_split_label) {
    EndTiming();
    StartTiming(new_split_label);
  }
  // Returns the total duration of the timings (sum of total times).
  uint64_t GetTotalNs() const;
  // Find the index of a timing by name.
  size_t FindTimingIndex(const char* name, size_t start_idx) const;
  EXPORT void Dump(std::ostream& os, const char* indent_string = "  ") const;

  // Scoped timing splits that can be nested and composed with the explicit split
  // starts and ends.
  class ScopedTiming {
   public:
    ScopedTiming(const char* label, TimingLogger* logger) : logger_(logger) {
      logger_->StartTiming(label);
    }
    ~ScopedTiming() {
      logger_->EndTiming();
    }
    // Closes the current timing and opens a new timing.
    void NewTiming(const char* label) {
      logger_->NewTiming(label);
    }

   private:
    TimingLogger* const logger_;  // The timing logger which the scoped timing is associated with.
    DISALLOW_COPY_AND_ASSIGN(ScopedTiming);
  };

  // Return the time points of when each start / end timings start and finish.
  const std::vector<Timing>& GetTimings() const {
    return timings_;
  }

  TimingData CalculateTimingData() const;

 protected:
  // The name of the timing logger.
  const char* const name_;
  // Do we want to print the exactly recorded split (true) or round down to the time unit being
  // used (false).
  const bool precise_;
  // Verbose logging.
  const bool verbose_;
  // The kind of timing we want.
  const TimingKind kind_;
  // Timing points that are either start or end points. For each starting point ret[i] = location
  // of end split associated with i. If it is and end split ret[i] = i.
  std::vector<Timing> timings_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TimingLogger);
};

}  // namespace art

#endif  // ART_RUNTIME_BASE_TIMING_LOGGER_H_
