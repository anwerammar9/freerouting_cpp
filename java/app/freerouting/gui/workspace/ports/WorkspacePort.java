package app.freerouting.gui.workspace.ports;

import app.freerouting.gui.workspace.progress.RoutingSummaryData;

/**
 * Stub workspace port for the headless baseline. Only the callbacks still referenced by the kept
 * {@link app.freerouting.gui.workspace.session.InteractiveActionThread} are declared.
 */
public interface WorkspacePort {

  void showRoutingSummary(RoutingSummaryData summary);

  void showProfileDialog();
}