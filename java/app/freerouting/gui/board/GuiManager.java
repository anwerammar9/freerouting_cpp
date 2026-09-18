package app.freerouting.gui.board;

import app.freerouting.settings.GlobalSettings;

/**
 * No-op stub for the removed GUI. Headless baseline never initializes the interactive board
 * manager, so this always reports that the GUI is unavailable.
 */
public class GuiManager {

  private GuiManager() {}

  public static boolean initializeGUI(GlobalSettings globalSettings) {
    return false;
  }
}