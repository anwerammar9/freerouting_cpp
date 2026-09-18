package app.freerouting.gui.workspace.progress;

import app.freerouting.core.RoutingJob;
import app.freerouting.gui.workspace.ports.WorkspacePort;
import app.freerouting.gui.workspace.session.InteractiveActionThread;
import app.freerouting.gui.workspace.session.RunGeneration;

/**
 * Stub GUI routing job worker for the headless baseline.
 *
 * <p>The real worker drives the interactive autorouter thread with GUI progress reporting. Headless
 * builds route through {@code RoutingJobSchedulerActionThread} instead, so this stub only satisfies
 * the factory in {@link InteractiveActionThread}. {@link #threadAction()} is a no-op to keep the
 * thread from doing any actual routing work when invoked.
 */
public class GuiRoutingJobWorker extends InteractiveActionThread {

  public GuiRoutingJobWorker(
      WorkspacePort sessionPort, RunGeneration generation, RoutingJob job) {
    super(sessionPort, generation, job);
  }

  @Override
  protected void threadAction() {}

  public RoutingSummaryData getSummaryData() {
    return new RoutingSummaryData();
  }
}