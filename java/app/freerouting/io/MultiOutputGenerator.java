package app.freerouting.io;

import app.freerouting.board.facade.BasicBoard;
import app.freerouting.io.specctra.SesWriter;
import java.io.ByteArrayOutputStream;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;

/**
 * Reduced multi-format output generator for the headless baseline.
 *
 * <p>The full implementation also emits KiCad session/design JSON, SCR and DRC_JSON outputs. The
 * headless baseline keeps the most important one ({@link FileFormat#SES}, written through the real
 * {@link SesWriter}); the other formats are not produced here.
 */
public final class MultiOutputGenerator {

  private MultiOutputGenerator() {}

  /** Aggregates the generated output files per requested format. */
  public static final class MultiOutputResult {

    private final Map<FileFormat, byte[]> files = new LinkedHashMap<>();

    public Map<FileFormat, byte[]> getFiles() {
      return files;
    }

    public byte[] getFile(FileFormat format) {
      return files.get(format);
    }

    public String getFileAsText(FileFormat format) {
      byte[] data = getFile(format);
      return data == null ? null : new String(data, java.nio.charset.StandardCharsets.UTF_8);
    }

    public void putFile(FileFormat format, byte[] data) {
      files.put(format, data);
    }
  }

  public static MultiOutputResult generateOutputs(
      BasicBoard board,
      String designName,
      Set<FileFormat> requestedFormats,
      Object drcSettings,
      boolean includeDrcSummary) {

    MultiOutputResult result = new MultiOutputResult();
    if (board == null || requestedFormats == null) {
      return result;
    }

    String effectiveDesignName =
        designName != null && !designName.isBlank() ? designName : "design";

    if (requestedFormats.contains(FileFormat.SES)) {
      byte[] sesBytes = generateSesBytes(board, effectiveDesignName);
      if (sesBytes != null) {
        result.putFile(FileFormat.SES, sesBytes);
      }
    }

    return result;
  }

  private static byte[] generateSesBytes(BasicBoard board, String designName) {
    try {
      ByteArrayOutputStream out = new ByteArrayOutputStream();
      SesWriter.write(board, out, designName);
      return out.toByteArray();
    } catch (Exception e) {
      return null;
    }
  }
}