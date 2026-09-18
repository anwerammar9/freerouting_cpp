package app.freerouting.io.kicad;

import app.freerouting.io.BoardReadResult;
import java.io.Reader;

/**
 * Stub KiCad JSON board reader for the headless baseline.
 *
 * <p>The full reader translates KiCad board JSON into the internal model. The headless baseline only
 * loads Specctra DSN boards, so these methods are no-ops that report the format was not recognized.
 * {@link #importSession} is similarly a no-op (KiCad JSON session import is not exercised in the
 * headless-only routing path).
 */
public final class KiCadJsonReader {

  private KiCadJsonReader() {}

  public static BoardReadResult readBoard(Reader reader, Object boardObservers, Object idGenerator) {
    return null;
  }

  public static void importSession(Reader reader, Object board) {}
}