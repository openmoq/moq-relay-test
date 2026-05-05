import { v4 as uuidv4 } from 'uuid';

/**
 * Self-test orchestrator.
 * Runs all self-test-enabled tools sequentially against a given relay.
 */
export class SelfTestOrchestrator {
  constructor(toolRegistry, dockerExecutor, resultsStore) {
    this.registry = toolRegistry;
    this.executor = dockerExecutor;
    this.results = resultsStore;
    this.activeSelfTests = new Map(); // selfTestId -> { aborted, ... }
  }

  /**
   * Run all self-test-enabled tools sequentially.
   * @param {object} config - { relayUrl, transport, draft }
   * @param {string} sessionId
   * @param {function} onProgress - (selfTestId, toolName, status, data) => void
   * @param {function} onComplete - (selfTestId, results) => void
   */
  async run(config, sessionId, onProgress, onComplete) {
    const selfTestId = uuidv4();
    const tools = this.registry.getSelfTestTools();
    const startedAt = new Date().toISOString();

    const state = { aborted: false, currentRunId: null };
    this.activeSelfTests.set(selfTestId, state);

    onProgress(selfTestId, null, 'started', {
      tools: tools.map(t => t._selfTestEntry.label || t.name),
      relayUrl: config.relayUrl,
    });

    const toolResults = [];

    for (const tool of tools) {
      if (state.aborted) break;

      const entry = tool._selfTestEntry;
      const runLabel = entry.label || tool.name;

      // Merge self-test defaults with shared config
      const params = {
        ...entry.defaults,
        relay_url: config.relayUrl,
      };
      if (config.transport && !entry.defaults?.transport) params.transport = config.transport;
      if (config.draft && !entry.defaults?.draft) params.draft = config.draft;

      onProgress(selfTestId, runLabel, 'running', { params });

      const toolOutput = [];

      try {
        const result = await new Promise((resolve, reject) => {
          if (state.aborted) return reject(new Error('aborted'));

          this.executor.startRun(
            tool,
            params,
            (runId, line, ts) => {
              toolOutput.push({ ts, line });
              onProgress(selfTestId, runLabel, 'output', { runId, line, ts });
            },
            (runId, exitCode) => {
              resolve({ runId, exitCode });
            },
            (runId, error) => {
              reject(new Error(error));
            },
          ).then(({ runId }) => {
            state.currentRunId = runId;
          });
        });

        toolResults.push({
          name: runLabel,
          tool: tool.name,
          status: result.exitCode === 0 ? 'pass' : 'fail',
          exitCode: result.exitCode,
          output: toolOutput,
        });

        onProgress(selfTestId, runLabel, result.exitCode === 0 ? 'pass' : 'fail', {
          exitCode: result.exitCode,
        });
      } catch (err) {
        toolResults.push({
          name: runLabel,
          tool: tool.name,
          status: state.aborted ? 'aborted' : 'error',
          error: err.message,
          output: toolOutput,
        });

        onProgress(selfTestId, runLabel, 'error', { error: err.message });
      }
    }

    this.activeSelfTests.delete(selfTestId);

    const selfTestResult = {
      selfTestId,
      sessionId,
      relayUrl: config.relayUrl,
      startedAt,
      completedAt: new Date().toISOString(),
      aborted: state.aborted,
      tools: toolResults,
    };

    this.results.saveSelfTest(selfTestResult);
    onComplete(selfTestId, selfTestResult);
  }

  /**
   * Abort a running self-test.
   */
  async abort(selfTestId) {
    const state = this.activeSelfTests.get(selfTestId);
    if (!state) return false;
    state.aborted = true;
    if (state.currentRunId) {
      await this.executor.stopRun(state.currentRunId);
    }
    return true;
  }
}
