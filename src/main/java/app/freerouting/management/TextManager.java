package app.freerouting.management;

import app.freerouting.logger.FRLogger;
import java.io.IOException;
import java.time.Duration;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.Locale;
import java.util.ResourceBundle;
import java.util.concurrent.ThreadLocalRandom;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Singleton class to manage the text resources for the application
 */
public class TextManager {

  private Locale currentLocale;
  private String currentBaseName;
  private ResourceBundle defaultMessages;
  private ResourceBundle classMessages;
  private ResourceBundle englishClassMessages;

  public TextManager(Class baseClass, Locale locale) {
    this.currentLocale = locale;
    loadResourceBundle(baseClass.getName());
  }

  public static String convertInstantToString(Instant instant) {
    return convertInstantToString(instant, "yyyyMMdd_HHmmss");
  }

  public static String convertInstantToString(Instant instant, String format) {
    DateTimeFormatter formatter = DateTimeFormatter.ofPattern(format);
    LocalDateTime localDateTime = LocalDateTime.ofInstant(instant, ZoneId.systemDefault());
    return localDateTime.format(formatter);
  }

  public static String generateRandomAlphanumericString(int length) {
    String characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    StringBuilder randomString = new StringBuilder();
    for (int i = 0; i < length; i++) {
      int index = (int) (characters.length() * ThreadLocalRandom.current().nextDouble());
      randomString.append(characters.charAt(index));
    }
    return randomString.toString();
  }

  public static Long parseTimespanString(String timespanString) {
    try {
      // convert the string from "HH:mm:ss" or "mm:ss" or "ss" format to "PnDTnHnMn.nS" format
      var durationString = convertFromTimespanToDurationFormat(timespanString);
      // parse the duration
      Duration duration = Duration.parse(durationString);
      return duration.getSeconds();
    } catch (DateTimeParseException ignored) {
      return null;
    }
  }

  public static String convertFromTimespanToDurationFormat(String timespanString) {
    String[] parts = timespanString.split(":");
    StringBuilder durationString = new StringBuilder("PT");

    if (parts.length == 3) {
      durationString
          .append(parts[0])
          .append("H")
          .append(parts[1])
          .append("M")
          .append(parts[2])
          .append("S");
    } else if (parts.length == 2) {
      durationString
          .append(parts[0])
          .append("M")
          .append(parts[1])
          .append("S");
    } else if (parts.length == 1) {
      durationString
          .append(parts[0])
          .append("S");
    }

    return durationString.toString();
  }

  /**
   * Shortens a string to a specified number of characters by replacing the middle part with dots
   *
   * @param text               The text to shorten
   * @param peakCharacterCount The number of characters to keep at the beginning and end of the text Example: shortenString("This is a long text", 3) -> "Thi...ext" shortenString("This is a long
   *                           text", 5) -> "This ... text" shortenString("This is a long text", 10) -> "This is a long text"
   * @return The shortened text
   */
  public static String shortenString(String text, int peakCharacterCount) {
    String shortenedText = text;
    if (text.length() > peakCharacterCount * 2) {
      shortenedText = shortenedText.substring(0, peakCharacterCount) + "..." + text.substring(text.length() - peakCharacterCount);
    }
    return shortenedText;
  }

  /**
   * Removes quotes from the beginning and end of a string
   *
   * @param text The text to remove quotes from
   * @return The text without quotes
   */
  public static String removeQuotes(String text) {
    if ((text == null) || (text.length() < 2)) {
      return text;
    }

    if (text.startsWith("\"") && text.endsWith("\"")) {
      text = text.substring(1, text.length() - 1);
    }

    return text;
  }

  public static String unescapeUnicode(String text) {
    Pattern pattern = Pattern.compile("\\\\u(\\p{XDigit}{4})");
    Matcher matcher = pattern.matcher(text);
    StringBuffer result = new StringBuffer();

    while (matcher.find()) {
      String hexCode = matcher.group(1);
      char unicode = (char) Integer.parseInt(hexCode, 16);
      matcher.appendReplacement(result, String.valueOf(unicode));
    }
    matcher.appendTail(result);

    return result.toString();
  }

  public static String longToHexadecimalString(Long longValue) {
    return "0x%016X".formatted(longValue);
  }

  public static Long hexadecimalStringToLong(String hexString) {
    if (hexString.startsWith("0x") || hexString.startsWith("0X")) {
      hexString = hexString.substring(2);
      return Long.parseUnsignedLong(hexString, 16);
    } else {
      return Long.parseUnsignedLong(hexString, 10);
    }
  }

  private void loadResourceBundle(String baseName) {
    this.currentBaseName = baseName;

    // Load the default messages that are common to all classes
    try {
      defaultMessages = ResourceBundle.getBundle("app.freerouting.Common", currentLocale);
    } catch (Exception ignored) {
      FRLogger.warn("There was a problem loading the resource bundle 'app.freerouting.Common' of locale '" + currentLocale + "'");
      try {
        defaultMessages = ResourceBundle.getBundle("app.freerouting.Common", Locale.forLanguageTag("en-US"));
      } catch (Exception ignored) {
        defaultMessages = null;
        FRLogger.error("There was a problem loading the resource bundle 'app.freerouting.Common' of locale 'en-US'", null);
      }
    }

    // Load the class-specific messages
    try {
      classMessages = ResourceBundle.getBundle(currentBaseName, currentLocale);
    } catch (Exception ignored) {
      try {
        classMessages = ResourceBundle.getBundle(currentBaseName, Locale.forLanguageTag("en-US"));
      } catch (Exception ignored) {
        classMessages = null;
      }
    }

    // Load the fallback English messages
    try {
      englishClassMessages = ResourceBundle.getBundle(currentBaseName, Locale.forLanguageTag("en"));
    } catch (Exception ignored) {
    }
  }

  public String getText(String key, String... args) {
    String text;
    if ((classMessages != null) && (classMessages.containsKey(key))) {
      text = classMessages.getString(key);
    } else if ((defaultMessages != null) && (defaultMessages.containsKey(key))) {
      text = defaultMessages.getString(key);
    } else if ((englishClassMessages != null) && (englishClassMessages.containsKey(key))) {
      text = englishClassMessages.getString(key);
    } else {
      return key;
    }

    // Pattern to match {{variable_name}} placeholders
    Pattern pattern = Pattern.compile("\\{\\{(.+?)\\}\\}");
    Matcher matcher = pattern.matcher(text);

    // Find and replace all matches
    int argIndex = 0;
    while (matcher.find()) {
      // Entire match including {{ and }}
      String placeholder = matcher.group(0);

      if (!placeholder.startsWith("{{icon:") && argIndex < args.length) {
        // replace the placeholder with the value
        text = text.replace(placeholder, args[argIndex]);
        argIndex++;
      }
    }

    return text;
  }

  public Locale getLocale() {
    return currentLocale;
  }

  public void setLocale(Locale locale) {
    this.currentLocale = locale;
    loadResourceBundle(currentBaseName);
  }

}