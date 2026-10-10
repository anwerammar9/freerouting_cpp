import app.freerouting.board.facade.BasicBoard;
import app.freerouting.board.model.items.ComponentObstacleArea;
import app.freerouting.board.model.items.ComponentOutline;
import app.freerouting.board.model.items.ConductionArea;
import app.freerouting.board.model.items.ObstacleArea;
import app.freerouting.board.model.items.Pin;
import app.freerouting.board.model.items.Via;
import app.freerouting.board.model.items.ViaObstacleArea;
import app.freerouting.board.model.structure.BoardOutline;
import app.freerouting.board.trace.PolylineTrace;
import app.freerouting.io.BoardReadResult;
import app.freerouting.io.specctra.DsnReader;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.List;

/**
 * T007 oracle harness: loads a DSN exactly like the CLI (DsnReader.readBoard) and prints the board
 * state as JSON with the same schema the C++ `--dump-board-counts` (T117) must emit.
 *
 * <p>Usage: java BoardCounts &lt;dsn-path&gt; [out.json] [--omit-items]
 *
 * <p>The JSON goes to the 2nd argument (a file) when given, so the jar's stdout log noise cannot
 * corrupt it; otherwise it is printed after a `===JSON===` marker line.
 */
public class BoardCounts {

  public static void main(String[] args) throws Exception {
    String dsnPath = args[0];
    String outPath = null;
    boolean omitItems = false;
    for (int i = 1; i < args.length; i++) {
      if ("--omit-items".equals(args[i])) {
        omitItems = true;
      } else {
        outPath = args[i];
      }
    }

    BoardReadResult readResult;
    try (FileInputStream is = new FileInputStream(dsnPath)) {
      readResult = DsnReader.readBoard(is, null, null, Path.of(dsnPath).getFileName().toString());
    }

    String json = render(readResult, dsnPath, omitItems);

    if (outPath != null) {
      try (OutputStreamWriter w =
          new OutputStreamWriter(new FileOutputStream(outPath), StandardCharsets.UTF_8)) {
        w.write(json);
      }
    } else {
      PrintStream out = System.out;
      out.println("===JSON===");
      out.print(json);
    }
  }

  static String render(BoardReadResult readResult, String dsnPath, boolean omitItems) {
    StringBuilder sb = new StringBuilder(1 << 16);
    sb.append("{\n");
    ind(sb, 1).append("\"file\": ").append(jsonString(dsnPath)).append(",\n");

    if (readResult instanceof BoardReadResult.Success s) {
      appendBoard(sb, s.board(), s.warnings(), "success", null, omitItems);
    } else if (readResult instanceof BoardReadResult.OutlineMissing o) {
      appendBoard(sb, o.board(), o.warnings(), "outline_missing", null, omitItems);
    } else if (readResult instanceof BoardReadResult.ParseError e) {
      appendBase(sb, "parse_error", e.location() + ": " + e.detail(), 1);
    } else if (readResult instanceof BoardReadResult.IoError io) {
      appendBase(sb, "io_error", String.valueOf(io.cause()), 1);
    } else {
      appendBase(sb, "io_error", "unexpected BoardReadResult: " + readResult, 1);
    }

    sb.append("}\n");
    return sb.toString();
  }

  private static void appendBase(StringBuilder sb, String result, String error, int indent) {
    ind(sb, indent).append("\"result\": \"").append(result).append("\",\n");
    ind(sb, indent).append("\"error\": ").append(jsonString(error)).append(",\n");
    ind(sb, indent).append("\"layers\": [],\n");
    ind(sb, indent).append("\"counts\": ").append(emptyCounts()).append(",\n");
    ind(sb, indent).append("\"clearance_classes\": [],\n");
    ind(sb, indent).append("\"clearance_matrix\": [],\n");
    ind(sb, indent).append("\"items_omitted\": false,\n");
    ind(sb, indent).append("\"items\": [],\n");
    ind(sb, indent).append("\"warnings\": 0");
  }

  private static void appendBoard(
      StringBuilder sb, BasicBoard board, List<String> warnings, String result, String error,
      boolean omitItems) {
    int indent = 1;
    ind(sb, indent).append("\"result\": \"").append(result).append("\",\n");
    ind(sb, indent).append("\"error\": ").append(error == null ? "null" : jsonString(error)).append(",\n");

    // layers
    StringBuilder layers = new StringBuilder("[");
    var layerArr =
        (board.layerStructure == null) ? null : board.layerStructure.layers;
    if (layerArr != null) {
      for (int i = 0; i < layerArr.length; i++) {
        if (i > 0) {
          layers.append(", ");
        }
        var l = layerArr[i];
        layers
            .append("{\"name\": ")
            .append(jsonString(l.name))
            .append(", \"signal\": ")
            .append(l.isSignal)
            .append("}");
      }
    }
    layers.append("]");

    // clearance classes + matrix
    boolean matrixOk = board.rules != null && board.rules.clearanceMatrix != null;
    int classCount = matrixOk ? board.rules.clearanceMatrix.getClassCount() : 0;
    StringBuilder classesB = new StringBuilder("[");
    for (int i = 0; i < classCount; i++) {
      if (i > 0) {
        classesB.append(", ");
      }
      classesB.append(jsonString(board.rules.clearanceMatrix.getName(i)));
    }
    classesB.append("]");

    StringBuilder matrix = new StringBuilder("[");
    int layerCount = matrixOk ? board.rules.clearanceMatrix.getLayerCount() : 0;
    for (int ci = 0; ci < classCount; ci++) {
      if (ci > 0) {
        matrix.append(", ");
      }
      matrix.append("[");
      for (int cj = 0; cj < classCount; cj++) {
        if (cj > 0) {
          matrix.append(", ");
        }
        matrix.append("[");
        for (int li = 0; li < layerCount; li++) {
          if (li > 0) {
            matrix.append(", ");
          }
          matrix.append(board.rules.clearanceMatrix.getValue(ci, cj, li, false));
        }
        matrix.append("]");
      }
      matrix.append("]");
    }
    matrix.append("]");

    // one pass over getItems() in item-list order (descending id): render items + total the counts
    Counts c = new Counts();
    StringBuilder items = new StringBuilder();
    if (omitItems) {
      items.append("[]");
      for (var item : board.getItems()) {
        c.add(item);
      }
    } else {
      items.append("[");
      boolean first = true;
      for (var item : board.getItems()) {
        if (!first) {
          items.append(",\n");
        }
        first = false;
        items.append(renderItem(item));
        c.add(item);
      }
      items.append("]");
    }
    c.boardOutline = board.getOutline() != null ? 1 : 0;

    ind(sb, indent).append("\"layers\": ").append(layers).append(",\n");
    ind(sb, indent).append("\"counts\": ").append(c.toJson(board)).append(",\n");
    ind(sb, indent).append("\"clearance_classes\": ").append(classesB).append(",\n");
    ind(sb, indent).append("\"clearance_matrix\": ").append(matrix).append(",\n");
    ind(sb, indent).append("\"items_omitted\": ").append(omitItems).append(",\n");
    ind(sb, indent).append("\"items\": ").append(items).append(",\n");
    ind(sb, indent).append("\"warnings\": ").append(warnings == null ? 0 : warnings.size());
  }

  private static final class Counts {
    int nets;
    int netClasses;
    int viaInfos;
    int viaRules;
    int padstacks;
    int packages;
    int components;
    int pins;
    int vias;
    int traces;
    int conductionAreas;
    int obstacleAreas;
    int viaObstacleAreas;
    int componentObstacleAreas;
    int componentOutlines;
    int boardOutline;

    void add(app.freerouting.board.model.items.Item item) {
      if (item instanceof Pin) {
        pins++;
      }
      if (item instanceof Via) {
        vias++;
      }
      if (item instanceof PolylineTrace) {
        traces++;
      }
      if (item instanceof ConductionArea) {
        conductionAreas++;
      }
      if (item instanceof ViaObstacleArea) {
        viaObstacleAreas++;
      }
      if (item instanceof ComponentObstacleArea) {
        componentObstacleAreas++;
      }
      if (item instanceof ComponentOutline) {
        componentOutlines++;
      }
      if (item.getClass() == ObstacleArea.class) {
        obstacleAreas++;
      }
    }

    String toJson(BasicBoard board) {
      nets = board.rules == null || board.rules.nets == null ? 0 : board.rules.nets.maxNetNumber();
      netClasses = board.rules == null ? 0 : board.rules.netClasses.count();
      viaInfos = board.rules == null ? 0 : board.rules.viaInfos.count();
      viaRules = board.rules == null ? 0 : board.rules.viaRules.size();
      padstacks = board.library == null || board.library.padstacks == null ? 0 : board.library.padstacks.count();
      packages = board.library == null || board.library.packages == null ? 0 : board.library.packages.count();
      components = board.components == null ? 0 : board.components.count();
      return "{\"nets\": "
          + nets
          + ", \"net_classes\": "
          + netClasses
          + ", \"via_infos\": "
          + viaInfos
          + ", \"via_rules\": "
          + viaRules
          + ", \"padstacks\": "
          + padstacks
          + ", \"packages\": "
          + packages
          + ", \"components\": "
          + components
          + ", \"pins\": "
          + pins
          + ", \"vias\": "
          + vias
          + ", \"traces\": "
          + traces
          + ", \"conduction_areas\": "
          + conductionAreas
          + ", \"obstacle_areas\": "
          + obstacleAreas
          + ", \"via_obstacle_areas\": "
          + viaObstacleAreas
          + ", \"component_obstacle_areas\": "
          + componentObstacleAreas
          + ", \"component_outlines\": "
          + componentOutlines
          + ", \"board_outline\": "
          + boardOutline
          + "}";
    }
  }

  private static StringBuilder renderItem(app.freerouting.board.model.items.Item item) {
    StringBuilder r = new StringBuilder();
    String type = item.getClass().getSimpleName();
    r.append("{\"id\": ").append(item.getId());
    r.append(", \"type\": ").append(jsonString(type));
    r.append(", \"layers\": [")
        .append(item.firstLayer())
        .append(", ")
        .append(item.lastLayer())
        .append("]");
    r.append(", \"nets\": [");
    for (int i = 0; i < item.netCount(); i++) {
      if (i > 0) {
        r.append(", ");
      }
      r.append(item.getNetNumber(i));
    }
    r.append("]");
    String fixed = item.getFixedState() == null ? "UNFIXED" : item.getFixedState().name();
    r.append(", \"fixed\": ").append(jsonString(fixed));
    if (item instanceof Pin pin) {
      r.append(", \"component\": ").append(item.getComponentId());
      r.append(", \"pin\": ").append(pin.pinIndex);
    }
    int corners = 0;
    if (item instanceof PolylineTrace trace) {
      corners = trace.cornerCount();
    }
    r.append(", \"corners\": ").append(corners);
    r.append("}");
    return r;
  }

  private static String emptyCounts() {
    return "{\"nets\": 0, \"net_classes\": 0, \"via_infos\": 0, \"via_rules\": 0, "
        + "\"padstacks\": 0, \"packages\": 0, \"components\": 0, \"pins\": 0, \"vias\": 0, "
        + "\"traces\": 0, \"conduction_areas\": 0, \"obstacle_areas\": 0, "
        + "\"via_obstacle_areas\": 0, \"component_obstacle_areas\": 0, "
        + "\"component_outlines\": 0, \"board_outline\": 0}";
  }

  private static StringBuilder ind(StringBuilder sb, int indent) {
    for (int i = 0; i < indent; i++) {
      sb.append("  ");
    }
    return sb;
  }

  static String jsonString(String s) {
    if (s == null) {
      return "null";
    }
    StringBuilder out = new StringBuilder(s.length() + 2);
    out.append('"');
    for (int i = 0; i < s.length(); i++) {
      char c = s.charAt(i);
      switch (c) {
        case '"' -> out.append("\\\"");
        case '\\' -> out.append("\\\\");
        case '\n' -> out.append("\\n");
        case '\r' -> out.append("\\r");
        case '\t' -> out.append("\\t");
        case '\b' -> out.append("\\b");
        case '\f' -> out.append("\\f");
        default -> {
          if (c < 0x20) {
            out.append(String.format("\\u%04x", (int) c));
          } else {
            out.append(c);
          }
        }
      }
    }
    out.append('"');
    return out.toString();
  }
}