/*
 * Copyright (C) 2024 The Android Open Source Project
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

package com.android.server.art;

import static com.android.server.art.model.ArtFlags.ScheduleStatus;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Context;
import android.os.Build;
import android.os.CancellationSignal;
import android.os.SystemProperties;
import android.provider.DeviceConfig;

import androidx.annotation.RequiresApi;

import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.server.art.model.ArtFlags;
import com.android.server.art.model.ArtServiceJobInterface;
import com.android.server.art.prereboot.PreRebootDriver;
import com.android.server.art.prereboot.PreRebootStatsReporter;

import java.time.Duration;
import java.util.Objects;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

/**
 * The Pre-reboot Dexopt job.
 *
 * During Pre-reboot Dexopt, the old version of this code is run.
 *
 * @hide
 */
@RequiresApi(Build.VERSION_CODES.VANILLA_ICE_CREAM)
public class PreRebootDexoptJob implements ArtServiceJobInterface {
    /**
     * "android" is the package name for a <service> declared in
     * frameworks/base/core/res/AndroidManifest.xml
     */
    private static final String JOB_PKG_NAME = Utils.PLATFORM_PACKAGE_NAME;
    /** An arbitrary number. Must be unique among all jobs owned by the system uid. */
    public static final int JOB_ID = 27873781;

    @NonNull private final Injector mInjector;

    @NonNull private final BlockingQueue<Runnable> mWorkQueue = new LinkedBlockingQueue<>();

    /**
     * Serializes mutations to the global state of Pre-reboot Dexopt, including mounts, staged
     * files, and stats.
     */
    @NonNull
    private final ThreadPoolExecutor mSerializedExecutor =
            new ThreadPoolExecutor(1 /* corePoolSize */, 1 /* maximumPoolSize */,
                    60 /* keepAliveTime */, TimeUnit.SECONDS, mWorkQueue);

    // Job state variables.
    @GuardedBy("this") @Nullable private CompletableFuture<Boolean> mRunningJob = null;
    @GuardedBy("this") @Nullable private CancellationSignal mCancellationSignal = null;

    /** The slot that contains the OTA update, "_a" or "_b", or null for a Mainline update. */
    @GuardedBy("this") @Nullable private String mOtaSlot = null;

    /**
     * Whether the job has started at least once, meaning the device is expected to have staged
     * files, no matter it succeed, failed, or cancelled.
     *
     * Note that this flag is not persisted across system server restarts. It's possible that the
     * value is lost due to a system server restart caused by a crash, but this is a minor case, so
     * we don't handle it here for simplicity.
     */
    @GuardedBy("this") private boolean mHasStarted = false;

    public PreRebootDexoptJob(@NonNull Context context) {
        this(new Injector(context));
    }

    @VisibleForTesting
    public PreRebootDexoptJob(@NonNull Injector injector) {
        mInjector = injector;
        // Recycle the thread if it's not used for `keepAliveTime`.
        mSerializedExecutor.allowsCoreThreadTimeOut();
    }

    @Override
    public boolean onStartJob(
            @NonNull BackgroundDexoptJobService jobService, @NonNull JobParameters params) {
        // No need to handle exceptions thrown by the future because exceptions are handled inside
        // the future itself.
        var unused = start().thenAcceptAsync((cancelled) -> {
            try {
                // If it failed, it means something went wrong, so we don't reschedule the job
                // because it will likely fail again. If it's cancelled, the job will be rescheduled
                // because the return value of `onStopJob` will be respected, and this call will be
                // skipped.
                if (!cancelled) {
                    jobService.jobFinished(params, false /* wantsReschedule */);
                }
            } catch (RuntimeException e) {
                AsLog.wtf("Unexpected exception", e);
            }
        });
        // "true" means the job will continue running until `jobFinished` is called.
        return true;
    }

    @Override
    public boolean onStopJob(@NonNull JobParameters params) {
        cancel(false /* blocking */);
        // "true" means to execute again with the default retry policy.
        return true;
    }

    /**
     * Notifies this class that an update (OTA or Mainline) is ready.
     *
     * @param otaSlot The slot that contains the OTA update, "_a" or "_b", or null for a Mainline
     *         update.
     */
    public @ScheduleStatus int onUpdateReady(@Nullable String otaSlot) {
        unschedule();
        mSerializedExecutor.execute(() -> mInjector.getStatsReporter().reset());
        updateOtaSlot(otaSlot);
        return schedule();
    }

    @VisibleForTesting
    public @ScheduleStatus int schedule() {
        if (this != BackgroundDexoptJobService.getJob(JOB_ID)) {
            throw new IllegalStateException("This job cannot be scheduled");
        }

        boolean syspropEnable =
                SystemProperties.getBoolean("dalvik.vm.enable_pr_dexopt", false /* def */);
        boolean deviceConfigEnable = DeviceConfig.getBoolean(
                DeviceConfig.NAMESPACE_RUNTIME, "enable_pr_dexopt", false /* defaultValue */);
        boolean deviceConfigForceDisable = DeviceConfig.getBoolean(DeviceConfig.NAMESPACE_RUNTIME,
                "force_disable_pr_dexopt", false /* defaultValue */);
        if ((!syspropEnable && !deviceConfigEnable) || deviceConfigForceDisable) {
            AsLog.i(String.format(
                    "Pre-reboot Dexopt Job is not enabled (sysprop:dalvik.vm.enable_pr_dexopt=%b, "
                    + "device_config:enable_pr_dexopt=%b, "
                    + "device_config:force_disable_pr_dexopt=%b)",
                    syspropEnable, deviceConfigEnable, deviceConfigForceDisable));
            return ArtFlags.SCHEDULE_DISABLED_BY_SYSPROP;
        }

        // If `pm.dexopt.disable_bg_dexopt` is set, the user probably means to disable any dexopt
        // jobs in the background.
        if (SystemProperties.getBoolean("pm.dexopt.disable_bg_dexopt", false /* def */)) {
            AsLog.i("Pre-reboot Dexopt Job is disabled by system property "
                    + "'pm.dexopt.disable_bg_dexopt'");
            return ArtFlags.SCHEDULE_DISABLED_BY_SYSPROP;
        }

        JobInfo info = new JobInfo
                               .Builder(JOB_ID,
                                       new ComponentName(JOB_PKG_NAME,
                                               BackgroundDexoptJobService.class.getName()))
                               .setRequiresDeviceIdle(true)
                               .setRequiresCharging(true)
                               .setRequiresBatteryNotLow(true)
                               // The latency is to wait for update_engine to finish.
                               .setMinimumLatency(Duration.ofMinutes(10).toMillis())
                               .build();

        /* @JobScheduler.Result */ int result = mInjector.getJobScheduler().schedule(info);
        if (result == JobScheduler.RESULT_SUCCESS) {
            AsLog.i("Pre-reboot Dexopt Job scheduled");
            mSerializedExecutor.execute(() -> mInjector.getStatsReporter().recordJobScheduled());
            return ArtFlags.SCHEDULE_SUCCESS;
        } else {
            AsLog.i("Failed to schedule Pre-reboot Dexopt Job");
            return ArtFlags.SCHEDULE_JOB_SCHEDULER_FAILURE;
        }
    }

    @VisibleForTesting
    public void unschedule() {
        if (this != BackgroundDexoptJobService.getJob(JOB_ID)) {
            throw new IllegalStateException("This job cannot be unscheduled");
        }

        mInjector.getJobScheduler().cancel(JOB_ID);
    }

    /** The future returns true if the job is cancelled by the job scheduler. */
    @VisibleForTesting
    @NonNull
    public synchronized CompletableFuture<Boolean> start() {
        if (mRunningJob != null) {
            // We can get here only if the previous run has been cancelled but has not exited yet.
            // This should be very rare. In this case, just queue the new run, as the previous run
            // will exit soon.
            Utils.check(mCancellationSignal.isCanceled());
        }

        String otaSlot = mOtaSlot;
        var cancellationSignal = mCancellationSignal = new CancellationSignal();
        mHasStarted = true;
        mRunningJob = new CompletableFuture().supplyAsync(() -> {
            try {
                mInjector.getPreRebootDriver().run(
                        otaSlot, cancellationSignal, mInjector.getStatsReporter());
            } catch (RuntimeException e) {
                AsLog.e("Fatal error", e);
            } finally {
                synchronized (this) {
                    if (cancellationSignal == mCancellationSignal) {
                        mRunningJob = null;
                        mCancellationSignal = null;
                    }
                }
            }
            return cancellationSignal.isCanceled();
        }, mSerializedExecutor);
        return mRunningJob;
    }

    /**
     * Cancels the job.
     *
     * @param blocking whether to wait for the job to exit.
     */
    @VisibleForTesting
    public void cancel(boolean blocking) {
        CompletableFuture<Boolean> runningJob = null;
        synchronized (this) {
            if (mRunningJob == null) {
                return;
            }

            mCancellationSignal.cancel();
            AsLog.i("Job cancelled");
            runningJob = mRunningJob;
        }
        if (blocking) {
            Utils.getFuture(runningJob);
        }
    }

    @VisibleForTesting
    public synchronized void updateOtaSlot(@Nullable String value) {
        Utils.check(value == null || value.equals("_a") || value.equals("_b"));
        // It's not possible that this method is called with two different slots.
        Utils.check(mOtaSlot == null || value == null || Objects.equals(mOtaSlot, value));
        // An OTA update has a higher priority than a Mainline update. When there are both a pending
        // OTA update and a pending Mainline update, the system discards the Mainline update on the
        // reboot.
        if (mOtaSlot == null && value != null) {
            mOtaSlot = value;
        }
    }

    public synchronized boolean hasStarted() {
        return mHasStarted;
    }

    /**
     * Injector pattern for testing purpose.
     *
     * @hide
     */
    @VisibleForTesting
    public static class Injector {
        @NonNull private final Context mContext;
        @NonNull private final PreRebootStatsReporter mStatsReporter;

        Injector(@NonNull Context context) {
            mContext = context;
            mStatsReporter = new PreRebootStatsReporter();
        }

        @NonNull
        public JobScheduler getJobScheduler() {
            return Objects.requireNonNull(mContext.getSystemService(JobScheduler.class));
        }

        @NonNull
        public PreRebootDriver getPreRebootDriver() {
            return new PreRebootDriver(mContext);
        }

        @NonNull
        public PreRebootStatsReporter getStatsReporter() {
            return mStatsReporter;
        }
    }
}
