package app.freerouting.io.kicad;

/**
 * Stub KiCad JSON board writer for the headless baseline.
 *
 * <p>The full writer serializes the board model to KiCad board JSON. The headless baseline routes
 * DSN to SES only, so multi-format output ({@code -do *.kicad.json}) is not produced here.
 */
public final class KiCadJsonWriter {

  private KiCadJsonWriter() {}

  public static String write(Object board, String designName) {
    return "";
  }
}