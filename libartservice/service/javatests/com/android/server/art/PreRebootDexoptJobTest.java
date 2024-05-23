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

import static com.android.server.art.PreRebootDexoptJob.JOB_ID;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyBoolean;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.isNull;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.os.CancellationSignal;
import android.os.SystemProperties;
import android.provider.DeviceConfig;

import androidx.test.filters.SmallTest;

import com.android.server.art.model.ArtFlags;
import com.android.server.art.prereboot.PreRebootDriver;
import com.android.server.art.prereboot.PreRebootStatsReporter;
import com.android.server.art.testing.StaticMockitoRule;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.io.File;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Future;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@SmallTest
@RunWith(MockitoJUnitRunner.StrictStubs.class)
public class PreRebootDexoptJobTest {
    private static final long TIMEOUT_SEC = 10;

    @Rule
    public StaticMockitoRule mockitoRule = new StaticMockitoRule(SystemProperties.class,
            BackgroundDexoptJobService.class, DeviceConfig.class, ArtJni.class);

    @Mock private PreRebootDexoptJob.Injector mInjector;
    @Mock private JobScheduler mJobScheduler;
    @Mock private PreRebootDriver mPreRebootDriver;
    @Mock private PreRebootStatsReporter.Injector mPreRebootStatsReporterInjector;
    private PreRebootDexoptJob mPreRebootDexoptJob;

    @Before
    public void setUp() throws Exception {
        // By default, the job is enabled by a build-time flag.
        lenient()
                .when(SystemProperties.getBoolean(eq("pm.dexopt.disable_bg_dexopt"), anyBoolean()))
                .thenReturn(false);
        lenient()
                .when(SystemProperties.getBoolean(eq("dalvik.vm.enable_pr_dexopt"), anyBoolean()))
                .thenReturn(true);
        lenient()
                .when(DeviceConfig.getBoolean(
                        eq(DeviceConfig.NAMESPACE_RUNTIME), eq("enable_pr_dexopt"), anyBoolean()))
                .thenReturn(false);

        lenient()
                .when(SystemProperties.getBoolean(
                        eq("dalvik.vm.pre-reboot.has-started"), anyBoolean()))
                .thenReturn(false);

        lenient().when(mInjector.getJobScheduler()).thenReturn(mJobScheduler);
        lenient().when(mInjector.getPreRebootDriver()).thenReturn(mPreRebootDriver);
        lenient()
                .when(mInjector.getStatsReporter())
                .thenReturn(new PreRebootStatsReporter(mPreRebootStatsReporterInjector));

        File tempFile = File.createTempFile("pre-reboot-stats", ".pb");
        tempFile.deleteOnExit();
        lenient()
                .when(mPreRebootStatsReporterInjector.getFilename())
                .thenReturn(tempFile.getAbsolutePath());

        mPreRebootDexoptJob = new PreRebootDexoptJob(mInjector);
        lenient().when(BackgroundDexoptJobService.getJob(JOB_ID)).thenReturn(mPreRebootDexoptJob);
    }

    @Test
    public void testSchedule() throws Exception {
        var captor = ArgumentCaptor.forClass(JobInfo.class);
        when(mJobScheduler.schedule(captor.capture())).thenReturn(JobScheduler.RESULT_SUCCESS);

        assertThat(mPreRebootDexoptJob.schedule()).isEqualTo(ArtFlags.SCHEDULE_SUCCESS);

        JobInfo jobInfo = captor.getValue();
        assertThat(jobInfo.isPeriodic()).isFalse();
        assertThat(jobInfo.isRequireDeviceIdle()).isTrue();
        assertThat(jobInfo.isRequireCharging()).isTrue();
        assertThat(jobInfo.isRequireBatteryNotLow()).isTrue();
    }

    @Test
    public void testScheduleDisabled() {
        when(SystemProperties.getBoolean(eq("pm.dexopt.disable_bg_dexopt"), anyBoolean()))
                .thenReturn(true);

        assertThat(mPreRebootDexoptJob.schedule()).isEqualTo(ArtFlags.SCHEDULE_DISABLED_BY_SYSPROP);

        verify(mJobScheduler, never()).schedule(any());
    }

    @Test
    public void testScheduleNotEnabled() {
        when(SystemProperties.getBoolean(eq("dalvik.vm.enable_pr_dexopt"), anyBoolean()))
                .thenReturn(false);

        assertThat(mPreRebootDexoptJob.schedule()).isEqualTo(ArtFlags.SCHEDULE_DISABLED_BY_SYSPROP);

        verify(mJobScheduler, never()).schedule(any());
    }

    @Test
    public void testScheduleEnabledByPhenotypeFlag() {
        lenient()
                .when(SystemProperties.getBoolean(eq("dalvik.vm.enable_pr_dexopt"), anyBoolean()))
                .thenReturn(false);
        lenient()
                .when(DeviceConfig.getBoolean(
                        eq(DeviceConfig.NAMESPACE_RUNTIME), eq("enable_pr_dexopt"), anyBoolean()))
                .thenReturn(true);
        when(mJobScheduler.schedule(any())).thenReturn(JobScheduler.RESULT_SUCCESS);

        assertThat(mPreRebootDexoptJob.schedule()).isEqualTo(ArtFlags.SCHEDULE_SUCCESS);

        verify(mJobScheduler).schedule(any());
    }

    @Test
    public void testUnschedule() {
        mPreRebootDexoptJob.unschedule();
        verify(mJobScheduler).cancel(JOB_ID);
    }

    @Test
    public void testStart() throws Exception {
        var jobStarted = new Semaphore(0);
        when(mPreRebootDriver.run(any(), any(), any())).thenAnswer(invocation -> {
            jobStarted.release();
            return true;
        });

        when(ArtJni.setProperty("dalvik.vm.pre-reboot.has-started", "true"))
                .thenAnswer(invocation -> {
                    when(SystemProperties.getBoolean(
                                 eq("dalvik.vm.pre-reboot.has-started"), anyBoolean()))
                            .thenReturn(true);
                    return null;
                });

        assertThat(mPreRebootDexoptJob.hasStarted()).isFalse();
        Future<Boolean> future = mPreRebootDexoptJob.start();
        assertThat(jobStarted.tryAcquire(TIMEOUT_SEC, TimeUnit.SECONDS)).isTrue();
        assertThat(mPreRebootDexoptJob.hasStarted()).isTrue();

        Utils.getFuture(future);
    }

    @Test
    public void testStartAnother() {
        when(mPreRebootDriver.run(any(), any(), any())).thenReturn(true);

        Future<Boolean> future1 = mPreRebootDexoptJob.start();
        Utils.getFuture(future1);
        Future<Boolean> future2 = mPreRebootDexoptJob.start();
        Utils.getFuture(future2);
        assertThat(future1).isNotSameInstanceAs(future2);
    }

    @Test
    public void testCancel() {
        Semaphore dexoptCancelled = new Semaphore(0);
        Semaphore jobExited = new Semaphore(0);
        when(mPreRebootDriver.run(any(), any(), any())).thenAnswer(invocation -> {
            assertThat(dexoptCancelled.tryAcquire(TIMEOUT_SEC, TimeUnit.SECONDS)).isTrue();
            var cancellationSignal = invocation.<CancellationSignal>getArgument(1);
            assertThat(cancellationSignal.isCanceled()).isTrue();
            jobExited.release();
            return true;
        });

        var unused = mPreRebootDexoptJob.start();
        Future<Void> future = new CompletableFuture().runAsync(() -> {
            mPreRebootDexoptJob.cancel(false /* blocking */);
            dexoptCancelled.release();
            mPreRebootDexoptJob.cancel(true /* blocking */);
        });
        Utils.getFuture(future);
        // Check that `cancel(true)` is really blocking. If it wasn't, the check below might still
        // pass due to a race, but we would have a flaky test.
        assertThat(jobExited.tryAcquire()).isTrue();
    }

    @Test
    public void testUpdateOtaSlotOtaThenMainline() {
        mPreRebootDexoptJob.updateOtaSlot("_b");
        mPreRebootDexoptJob.updateOtaSlot(null);

        when(mPreRebootDriver.run(eq("_b"), any(), any())).thenReturn(true);

        Utils.getFuture(mPreRebootDexoptJob.start());
    }

    @Test
    public void testUpdateOtaSlotMainlineThenOta() {
        mPreRebootDexoptJob.updateOtaSlot(null);
        mPreRebootDexoptJob.updateOtaSlot("_a");

        when(mPreRebootDriver.run(eq("_a"), any(), any())).thenReturn(true);

        Utils.getFuture(mPreRebootDexoptJob.start());
    }

    @Test
    public void testUpdateOtaSlotMainlineThenMainline() {
        mPreRebootDexoptJob.updateOtaSlot(null);
        mPreRebootDexoptJob.updateOtaSlot(null);

        when(mPreRebootDriver.run(isNull(), any(), any())).thenReturn(true);

        Utils.getFuture(mPreRebootDexoptJob.start());
    }

    @Test
    public void testUpdateOtaSlotOtaThenOta() {
        mPreRebootDexoptJob.updateOtaSlot("_b");
        mPreRebootDexoptJob.updateOtaSlot("_b");

        when(mPreRebootDriver.run(eq("_b"), any(), any())).thenReturn(true);

        Utils.getFuture(mPreRebootDexoptJob.start());
    }

    @Test(expected = IllegalStateException.class)
    public void testUpdateOtaSlotOtaThenOtaDifferentSlots() {
        mPreRebootDexoptJob.updateOtaSlot("_b");
        mPreRebootDexoptJob.updateOtaSlot("_a");
    }

    @Test(expected = IllegalStateException.class)
    public void testUpdateOtaSlotOtaBogusSlot() {
        mPreRebootDexoptJob.updateOtaSlot("_bogus");
    }

    /**
     * Tests the case where new runs are requested (due to rescheduling on update ready or job
     * scheduler retries) when old runs haven't exited after cancelled.
     */
    @Test
    public void testFrequentReruns() {
        var globalState = new Semaphore(1);
        var jobBlocker = new Semaphore(0);

        when(mPreRebootDriver.run(any(), any(), any())).thenAnswer(invocation -> {
            // Step 2, 5, 8.

            // Verify that different runs don't mutate the global state concurrently.
            assertThat(globalState.tryAcquire()).isTrue();

            // Simulates that the job is still blocking for a while after being cancelled.
            assertThat(jobBlocker.tryAcquire(TIMEOUT_SEC, TimeUnit.SECONDS)).isTrue();

            // Step 4, 7, 10.

            // Verify that cancellation signals are properly delivered to each run.
            var cancellationSignal = invocation.<CancellationSignal>getArgument(1);
            assertThat(cancellationSignal.isCanceled()).isTrue();

            globalState.release();
            return true;
        });

        // Step 1.
        Future<Boolean> future1 = mPreRebootDexoptJob.start();
        mPreRebootDexoptJob.cancel(false /* blocking */); // For the 1st run.
        Future<Boolean> future2 = mPreRebootDexoptJob.start();
        mPreRebootDexoptJob.cancel(false /* blocking */); // For the 2nd run.
        Future<Boolean> future3 = mPreRebootDexoptJob.start();

        // Step 3.
        jobBlocker.release();
        Utils.getFuture(future1);
        // Step 6.
        mPreRebootDexoptJob.cancel(false /* blocking */); // For the 3rd run.
        jobBlocker.release();
        Utils.getFuture(future2);
        // Step 9.
        jobBlocker.release();
        Utils.getFuture(future3);
    }
}
