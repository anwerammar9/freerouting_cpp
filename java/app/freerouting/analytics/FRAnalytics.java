package app.freerouting.analytics;

import app.freerouting.analytics.model.ActorType;
import app.freerouting.analytics.model.PipelineType;

/**
 * No-op analytics stub for the headless-only build.
 *
 * <p>The full analytics pipeline (PostHog reporting, exception tracking, proxy config) is removed
 * from this baseline. All reporting methods are no-ops so the Java baseline stays buildable and
 * headless-runnable while routing DSNs.
 */
public final class FRAnalytics {
  private FRAnalytics() {}

  public static void flush(int seconds) {}

  public static void appClosed() {}

  public static void setEnabled(boolean enabled) {}

  public static void setAccessKey(String version, String loggerKey) {}

  public static void setUserId(String userId, String userEmail) {}

  public static void setExecutionContext(
      PipelineType pipeline, ActorType actor, String host, String contextId) {}

  public static void identify() {}

  public static void refreshIdentity() {}

  public static void setAppLocation(String app, String name) {}

  public static void appStarted(
      String version,
      String buildDate,
      String args,
      String osName,
      String osArch,
      String osVersion,
      String javaVersion,
      String javaVendor,
      java.util.Locale locale,
      java.util.Locale currentLocale,
      int cpuCores,
      long ram,
      int cpuScore,
      String host,
      int width,
      int height,
      int dpi) {}

  public static void recordBatchJobSummary(
      String jobId,
      String sessionId,
      String inputBasename,
      int cliExitCode,
      app.freerouting.analytics.model.JobLifecycleStatus status,
      String failureReason,
      int netsTotal,
      int netsIncomplete,
      int clearanceViolations,
      double normalizedScore,
      int totalPasses,
      double runtimeSeconds,
      float cpuTimeUsed,
      float peakMemoryUsed,
      String host,
      String unused) {}

  public static void fileLoaded(String format, String json) {}

  public static void boardLoaded(
      String hostCad, String hostVersion, int layerCount, int componentCount, int maxNetNumber) {}

  public static void recordJobLifecycle(Object... args) {}

  public static void recordSessionLifecycle(Object... args) {}

  public static void autorouterStarted() {}

  public static void autorouterFinished(
      int netsTotal,
      int incompleteCount,
      int clearanceViolations,
      String boardHash,
      float normalizedScore) {}

  public static void routeOptimizerStarted() {}

  public static void routeOptimizerFinished() {}

  public static PipelineType getCurrentPipeline() {
    return PipelineType.CLI;
  }

  public static ActorType getCurrentActorType() {
    return ActorType.AUTOMATED_BATCH;
  }
}