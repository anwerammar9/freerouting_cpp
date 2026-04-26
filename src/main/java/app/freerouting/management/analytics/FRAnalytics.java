package app.freerouting.management.analytics;

import static app.freerouting.Freerouting.globalSettings;

import app.freerouting.logger.FRLogger;
import app.freerouting.management.analytics.dto.Properties;
import app.freerouting.management.analytics.dto.Traits;
import app.freerouting.management.gson.GsonProvider;
import java.time.Instant;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;

/**
 * A class to manage analytics for the application.
 */
public class FRAnalytics {

  private static final HashMap<String, String> appLocationTable = new HashMap<>();
  private static AnalyticsClient analytics;
  private static String permanent_user_id;
  private static String permanent_user_email;
  private static String appPreviousLocation = "";
  private static String appCurrentLocation = "";
  private static String appWindowTitle = "";
  private static long appStartedAt;
  private static int sessionCount;
  private static long totalAutorouterRuntime;
  private static long totalRouteOptimizerRuntime;
  private static long autorouterStartedAt;
  private static long routeOptimizerStartedAt;

  private FRAnalytics() {
  }

  public static void setAccessKey(String libraryVersion, String key) {
    // analytics = new SegmentClient(libraryVersion, key);
    // analytics = new BigQueryClient(libraryVersion, key);
    analytics = new FreeroutingAnalyticsClient(libraryVersion, key);
  }

  public static void setUserId(String userId, String userEmail) {
    permanent_user_id = userId;
    permanent_user_email = userEmail;
  }

  private static void identifyUser(String userId, Map<String, String> traits) {
    if (analytics == null) {
      return;
    }

    try {
      Traits t = new Traits();
      t.putAll(traits);

      analytics.identify(userId, null, t);
    } catch (Exception e) {
      FRLogger.error("Exception in FRAnalytics.identifyUser: " + e.getMessage(), e);
    }
  }

  private static void identifyAnonymous(String anonymousId, Map<String, String> traits) {
    if (analytics == null) {
      return;
    }

    try {
      Traits t = new Traits();
      t.putAll(traits);

      analytics.identify(null, anonymousId, t);
    } catch (Exception e) {
      FRLogger.error("Exception in FRAnalytics.identifyAnonymous: " + e.getMessage(), e);
    }
  }

  private static void trackAnonymousAction(String anonymousId, String action, Map<String, String> properties) {
    if (analytics == null) {
      return;
    }

    try {
      Properties p = new Properties();
      p.put("current_time_utc", Instant
          .now()
          .toString());
      p.put("user_id", permanent_user_id);
      p.put("user_email", permanent_user_email);
      p.put("app_current_location", appCurrentLocation);
      p.put("app_previous_location", appPreviousLocation);
      p.put("app_window_title", appWindowTitle);
      if (properties != null) {
        p.putAll(properties);
      }

      analytics.track(null, anonymousId, action, p);
    } catch (Exception e) {
      FRLogger.error("Exception in FRAnalytics.trackAnonymousAction: " + e.getMessage(), e);
    }
  }

  public static void identify() {
    Map<String, String> traits = new HashMap<>();
    traits.put("anonymous", "true");
    traits.put("user_id", permanent_user_id);
    traits.put("user_email", permanent_user_email);
    // identifyUser(permanent_user_id, traits);
    identifyAnonymous(permanent_user_id, traits);
  }

  public static void setAppLocation(String windowClassName, String windowTitle) {
  }

  public static void buttonClicked(String buttonClassName, String buttonText) {
  }

  private static String translateClassNameToUrl(String appLocation) {
    return appLocation;
  }

  public static void setEnabled(boolean enabled) {
    if (analytics == null) {
      return;
    }

    analytics.setEnabled(enabled);
  }

  public static void appStarted(String freeroutingVersion, String freeroutingBuildDate, String commandLineArguments,
      String osName, String osArchitecture, String osVersion, String javaVersion,
      String javaVendor, Locale systemLanguage, Locale guiLanguage, int cpuCoreCount, long ramAmount, String host,
      int width, int height, int dpi) {
    appStartedAt = Instant
        .now()
        .getEpochSecond();

    Map<String, String> properties = new HashMap<>();
    properties.put("build_version", freeroutingVersion);
    properties.put("build_date", freeroutingBuildDate);
    properties.put("command_line_arguments", commandLineArguments);
    properties.put("os_name", osName);
    properties.put("os_architecture", osArchitecture);
    properties.put("os_version", osVersion);
    properties.put("java_version", javaVersion);
    properties.put("java_vendor", javaVendor);
    properties.put("system_language", systemLanguage.toString());
    properties.put("gui_language", guiLanguage.toString());
    properties.put("cpu_core_count", Integer.toString(cpuCoreCount));
    properties.put("ram_amount", Long.toString(ramAmount));
    properties.put("host", host);
    properties.put("screen_width", Integer.toString(width));
    properties.put("screen_height", Integer.toString(height));
    properties.put("screen_dpi", Integer.toString(dpi));
    trackAnonymousAction(permanent_user_id, "Application Started", properties);
  }

  public static void appClosed() {
    long appClosedAt = Instant
        .now()
        .getEpochSecond();

    Map<String, String> properties = new HashMap<>();
    properties.put("session_count", String.valueOf(sessionCount));
    properties.put("total_autorouter_runtime", String.valueOf(totalAutorouterRuntime));
    properties.put("total_route_optimizer_runtime", String.valueOf(totalRouteOptimizerRuntime));
    properties.put("application_runtime", String.valueOf(appClosedAt - appStartedAt));
    properties.put("statistics_start_time", String.valueOf(globalSettings.statistics.startTime));
    properties.put("statistics_end_time", String.valueOf(globalSettings.statistics.endTime));
    properties.put("statistics_sessions_total", String.valueOf(globalSettings.statistics.sessionsTotal));
    properties.put("statistics_jobs_started", String.valueOf(globalSettings.statistics.jobsStarted));
    properties.put("statistics_jobs_completed", String.valueOf(globalSettings.statistics.jobsCompleted));

    trackAnonymousAction(permanent_user_id, "Application Closed", properties);
    try {
      Thread.sleep(500);
    } catch (InterruptedException e) {
      throw new RuntimeException(e);
    }
  }

  public static void autorouterStarted() {
    autorouterStartedAt = Instant
        .now()
        .getEpochSecond();
    sessionCount++;

    Map<String, String> properties = new HashMap<>();
    properties.put("settings", GsonProvider.GSON.toJson(globalSettings));
    properties.put("session_count", String.valueOf(sessionCount));

    trackAnonymousAction(permanent_user_id, "Auto-router Started", properties);
  }

  public static void autorouterFinished() {
    long autorouterFinishedAt = Instant
        .now()
        .getEpochSecond();
    long autorouterRuntime = autorouterFinishedAt - autorouterStartedAt;
    totalAutorouterRuntime += autorouterRuntime;

    Map<String, String> properties = new HashMap<>();
    properties.put("settings", GsonProvider.GSON.toJson(globalSettings));
    properties.put("session_count", String.valueOf(sessionCount));
    properties.put("autorouter_runtime", String.valueOf(autorouterRuntime));

    trackAnonymousAction(permanent_user_id, "Auto-router Finished", properties);
  }

  public static void routeOptimizerStarted() {
    routeOptimizerStartedAt = Instant
        .now()
        .getEpochSecond();

    Map<String, String> properties = new HashMap<>();
    properties.put("settings", GsonProvider.GSON.toJson(globalSettings));
    properties.put("session_count", String.valueOf(sessionCount));
    trackAnonymousAction(permanent_user_id, "Route Optimizer Started", properties);
  }

  public static void routeOptimizerFinished() {
    long routeOptimizerFinishedAt = Instant
        .now()
        .getEpochSecond();
    long routeOptimizerRuntime = routeOptimizerFinishedAt - routeOptimizerStartedAt;
    totalRouteOptimizerRuntime += routeOptimizerRuntime;

    Map<String, String> properties = new HashMap<>();
    properties.put("settings", GsonProvider.GSON.toJson(globalSettings));
    properties.put("session_count", String.valueOf(sessionCount));
    properties.put("route_optimizer_runtime", String.valueOf(routeOptimizerRuntime));

    trackAnonymousAction(permanent_user_id, "Route Optimizer Finished", properties);
  }

  public static void fileLoaded(String fileFormat, String fileDetails) {
    Map<String, String> properties = new HashMap<>();
    properties.put("file_format", fileFormat);
    properties.put("file_details", fileDetails);

    trackAnonymousAction(permanent_user_id, "File Loaded", properties);
  }

  public static void boardLoaded(String hostName, String hostVersion, int layerCount, int componentCount,
      int netCount) {
    Map<String, String> properties = new HashMap<>();
    properties.put("host_name", hostName);
    properties.put("host_version", hostVersion);
    properties.put("layer_count", Integer.toString(layerCount));
    properties.put("component_count", Integer.toString(componentCount));
    properties.put("net_count", Integer.toString(netCount));

    trackAnonymousAction(permanent_user_id, "Board Loaded", properties);
  }

  public static void fileSaved(String fileFormat, String fileDetails) {
    Map<String, String> properties = new HashMap<>();
    properties.put("file_format", fileFormat);
    properties.put("file_details", fileDetails);

    trackAnonymousAction(permanent_user_id, "File Saved", properties);
  }

  public static void exceptionThrown(String localizedMessage, Throwable e) {
    StringBuilder sb = new StringBuilder();
    for (StackTraceElement ste : e.getStackTrace()) {
      sb.append(ste.toString());
      sb.append("\n");
    }

    Map<String, String> properties = new HashMap<>();
    properties.put("exception_message", localizedMessage);
    properties.put("exception_details", e.toString());
    properties.put("exception_stacktrace", sb.toString());

    trackAnonymousAction(permanent_user_id, "Exception Thrown", properties);
  }

  public static void apiEndpointCalled(String apiMethod, String requestBody, String responseBody) {
  }
}