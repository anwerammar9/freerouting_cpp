package app.freerouting;

import app.freerouting.board.BoardLoader;
import app.freerouting.constants.Constants;
import app.freerouting.core.RoutingJob;
import app.freerouting.core.RoutingJobState;
import app.freerouting.drc.DesignRulesChecker;
import app.freerouting.logger.FRLogger;
import app.freerouting.management.SessionManager;
import app.freerouting.management.TextManager;
import app.freerouting.settings.GlobalSettings;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Instant;
import java.util.Arrays;
import java.util.Locale;
import java.util.UUID;

/* Entry point class of the application */
public class Freerouting {

  public static final String WEB_URL = "https://www.freerouting.app";
  public static final String VERSION_NUMBER_STRING = "v" + Constants.FREEROUTING_VERSION + " (build-date: "
      + Constants.FREEROUTING_BUILD_DATE + ")";
  public static GlobalSettings globalSettings;

  private static boolean InitializeCLI(GlobalSettings globalSettings) {
    if ((globalSettings.initialInputFile == null) || (globalSettings.initialOutputFile == null)) {
      FRLogger.error(
          "Both an input file and an output file must be specified with command line arguments if you are running in CLI mode.",
          null);
      return false;
    }

    // Start a new Freerouting session
    var cliSession = SessionManager
        .getInstance()
        .createSession(UUID.fromString(globalSettings.userProfileSettings.userId),
            "Freerouting/" + globalSettings.version);

    // Create a new routing job
    RoutingJob routingJob = new RoutingJob(cliSession.id);
    try {
      routingJob.setInput(globalSettings.initialInputFile);
    } catch (Exception e) {
      FRLogger.error("Couldn't load the input file '" + globalSettings.initialInputFile + "'", e);
    }
    cliSession.addJob(routingJob);

    var desiredOutputFile = new File(globalSettings.initialOutputFile);
    if ((desiredOutputFile != null) && desiredOutputFile.exists()) {
      if (!desiredOutputFile.delete()) {
        FRLogger.warn("Couldn't delete the file '" + globalSettings.initialOutputFile + "'");
      }
    }

    routingJob.tryToSetOutputFile(new File(globalSettings.initialOutputFile));

    routingJob.routerSettings = Freerouting.globalSettings.routerSettings.clone();
    routingJob.routerSettings.setLayerCount(routingJob.input.statistics.layers.totalCount);
    routingJob.drcSettings = Freerouting.globalSettings.drcSettings.clone();
    routingJob.state = RoutingJobState.READY_TO_START;

    // Wait for the RoutingJobScheduler to do its work
    while ((routingJob.state != RoutingJobState.COMPLETED) && (routingJob.state != RoutingJobState.TERMINATED)) {
      try {
        Thread.sleep(500);
      } catch (InterruptedException e) {
        routingJob.state = RoutingJobState.CANCELLED;
        break;
      }
    }

    // Save the output file
    if (routingJob.state == RoutingJobState.COMPLETED) {
      try {
        Path outputFilePath = Path.of(globalSettings.initialOutputFile);
        Files.write(outputFilePath, routingJob.output
            .getData()
            .readAllBytes());
      } catch (IOException e) {
        FRLogger.error("Couldn't save the output file '" + globalSettings.initialOutputFile + "'", e);
      }
    }

    return true;
  }

  private static boolean InitializeDRC(GlobalSettings globalSettings) {
    if (globalSettings.initialInputFile == null) {
      FRLogger.error("An input file must be specified with -de argument in DRC mode.", null);
      return false;
    }

    // Start a new Freerouting session
    var drcSession = SessionManager
        .getInstance()
        .createSession(UUID.fromString(globalSettings.userProfileSettings.userId),
            "Freerouting/" + globalSettings.version);

    // Create a new routing job (but won't route it)
    RoutingJob drcJob = new RoutingJob(drcSession.id);
    drcJob.drc = globalSettings.drc_report_file;
    try {
      drcJob.setInput(globalSettings.initialInputFile);
    } catch (Exception e) {
      FRLogger.error("Couldn't load the input file '" + globalSettings.initialInputFile + "'", e);
      System.exit(1);
    }

    // Load the board without routing
    if (!BoardLoader.loadBoardIfNeeded(drcJob)) {
      FRLogger.error("Failed to load board for DRC check", null);
      System.exit(1);
    }

    // Run DRC check
    DesignRulesChecker drcChecker = new DesignRulesChecker(drcJob.board, globalSettings.drcSettings);

    // Determine coordinate unit (default to mm)
    String coordinateUnit = "mm";

    // Generate DRC report
    String sourceFileName = new File(globalSettings.initialInputFile).getName();
    String drcReportJson = drcChecker.generateReportJson(sourceFileName, coordinateUnit);

    // Output the DRC report
    if (drcJob.drc != null) {
      String outputFileName = drcJob.drc.getAbsolutePath();
      // Write to file
      try {
        Path outputFilePath = Path.of(outputFileName);
        Files.write(outputFilePath, drcReportJson.getBytes(StandardCharsets.UTF_8));
        FRLogger.info("DRC report written to: " + outputFileName);
      } catch (IOException e) {
        FRLogger.error("Couldn't save the DRC report to '" + outputFileName + "'", e);
        System.exit(1);
      }
    } else {
      // Print to console
      System.out.println(drcReportJson);
    }

    return true;
  }

  private static void ShutdownApplication() {
    // Application is shutting down
  }

  /**
   * The entry point of the Freerouting application
   *
   * @param args
   */
  public static void main(String[] args) {
    FRLogger.traceEntry("MainApplication.main()");

    // determine the user directory
    Path userdataPath = Path.of(System.getProperty("java.io.tmpdir"), "freerouting");
    if (System.getenv("FREEROUTING__USER_DATA_PATH") != null) {
      userdataPath = Path.of(System.getenv("FREEROUTING__USER_DATA_PATH"));
    }
    if (args.length > 0 && Arrays
        .stream(args)
        .anyMatch(s -> s.startsWith("--user_data_path="))) {
      var userDataPathArg = Arrays
          .stream(args)
          .filter(s -> s.startsWith("--user_data_path="))
          .findFirst();

      if (userDataPathArg.isPresent()) {
        userdataPath = Path.of(userDataPathArg
            .get()
            .substring("--user_data_path=".length()));
      }
    }
    if (!userdataPath.toFile().exists()) {
      userdataPath.toFile().mkdirs();
    }
    if (userdataPath.toFile().exists()) {
      GlobalSettings.setUserDataPath(userdataPath);
    }
    GlobalSettings.lockUserDataPath();

    if (args.length > 0 && Arrays.asList(args).contains("-dl")) {
      FRLogger.disableLogging();
    } else if (args.length > 0 && Arrays.asList(args).contains("-ll")) {
      int logLevelIndex = Arrays.asList(args)
          .indexOf("-ll") + 1;
      if (logLevelIndex < args.length) {
        FRLogger.changeFileLogLevel(args[logLevelIndex]);
      }
    }

    FRLogger.info("Freerouting " + VERSION_NUMBER_STRING);

    try {
      globalSettings = GlobalSettings.load();
      FRLogger.debug("Settings were loaded from freerouting.json");
    } catch (Exception e) {
    }

    if ((globalSettings == null) || (globalSettings.version != Constants.FREEROUTING_VERSION)) {
      String userId = globalSettings == null ? UUID.randomUUID().toString() : globalSettings.userProfileSettings.userId;

      globalSettings = new GlobalSettings();
      globalSettings.userProfileSettings.userId = userId;
      globalSettings.version = Constants.FREEROUTING_VERSION;

      try {
        GlobalSettings.saveAsJson(globalSettings);
      } catch (Exception e) {
      }
    }

    globalSettings.applyNonRouterEnvironmentVariables();

    globalSettings.runtimeEnvironment.freeroutingVersion = Constants.FREEROUTING_VERSION + ","
        + Constants.FREEROUTING_BUILD_DATE;
    globalSettings.runtimeEnvironment.appStartedAt = Instant.now();
    globalSettings.runtimeEnvironment.commandLineArguments = String.join(" ", args);
    globalSettings.runtimeEnvironment.architecture = System.getProperty("os.name") + ","
        + System.getProperty("os.arch") + "," + System.getProperty("os.version");
    globalSettings.runtimeEnvironment.java = System.getProperty("java.version") + ","
        + System.getProperty("java.vendor");
    globalSettings.runtimeEnvironment.systemLanguage = Locale
        .getDefault()
        .getLanguage() + "," + Locale.getDefault();
    globalSettings.runtimeEnvironment.cpuCores = Runtime
        .getRuntime()
        .availableProcessors();
    globalSettings.runtimeEnvironment.ram = (int) (Runtime
        .getRuntime()
        .maxMemory() / 1024 / 1024);
    FRLogger.debug("Version: " + globalSettings.runtimeEnvironment.freeroutingVersion);
    FRLogger.debug("Command line arguments: '" + globalSettings.runtimeEnvironment.commandLineArguments + "'");
    FRLogger.debug("Architecture: " + globalSettings.runtimeEnvironment.architecture);
    FRLogger.debug("Java: " + globalSettings.runtimeEnvironment.java);
    FRLogger.debug("System Language: " + globalSettings.runtimeEnvironment.systemLanguage);
    FRLogger.debug("Hardware: " + globalSettings.runtimeEnvironment.cpuCores + " CPU cores,"
        + globalSettings.runtimeEnvironment.ram + " MB RAM");
    FRLogger.debug("UTC Time: " + globalSettings.runtimeEnvironment.appStartedAt);

    globalSettings.applyCommandLineArguments(args);

    if (globalSettings.show_help_option) {
      TextManager ctm = new TextManager(Freerouting.class, globalSettings.currentLocale);
      System.out.print(ctm.getText("command_line_help"));
      System.exit(0);
    }

    boolean cliResult;
    if (globalSettings.drcSettings.enabled) {
      cliResult = InitializeDRC(globalSettings);
    } else {
      cliResult = InitializeCLI(globalSettings);
    }

    if (!cliResult) {
      ShutdownApplication();
      FRLogger.traceExit("MainApplication.main()");
      System.exit(1);
    }

    ShutdownApplication();
    FRLogger.traceExit("MainApplication.main()");
    System.exit(0);
  }

}
