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

  // Canonical transport codes used throughout self-test: 'quic', 'wt', or 'all'
  // (meaning "no preference" / "runs both").
  normalizeTransport(value) {
    const v = String(value || '').trim().toLowerCase();
    if (!v || v === 'all' || v === 'both') return 'all';
    if (v === 'q' || v === 'quic') return 'quic';
    if (v === 'wt' || v === 'webtransport' || v === 'h3') return 'wt';
    return 'all';
  }

  // Which transport a self-test entry runs against, derived from its manifest
  // declaration rather than guessing from its label.
  entryTransport(entry, tool) {
    const explicit = this.normalizeTransport(entry?.selfTestTransport);
    if (explicit !== 'all') return explicit;

    const fixed = this.normalizeTransport(tool?.fixedTransport);
    if (fixed !== 'all') return fixed;

    return 'all';
  }

  shouldRunEntry(selectedTransport, entryTransport) {
    if (selectedTransport === 'all') return true;
    if (entryTransport === 'all') return true;
    return selectedTransport === entryTransport;
  }

  shouldRunDraft(selectedDraft, entryDrafts) {
    if (!selectedDraft || String(selectedDraft).toLowerCase() === 'all') return true;
    return (entryDrafts || []).map(String).includes(String(selectedDraft));
  }

  // Use the entry's declared transport for split runs. For transport-agnostic
  // tools, narrow a "both" default when the user selects one transport.
  applySelectedTransport(params, selectedTransport, entryTransport, tool) {
    if (!tool.parameters?.some((param) => param.id === 'transport')) return;
    if (entryTransport !== 'all') {
      params.transport = entryTransport;
    } else if (selectedTransport !== 'all') {
      params.transport = selectedTransport;
    }
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
    const selectedTransport = this.normalizeTransport(config.transport);
    const tools = this.registry.getSelfTestTools().filter((tool) => {
      const entry = tool._selfTestEntry || {};
      return this.shouldRunEntry(selectedTransport, this.entryTransport(entry, tool)) &&
        this.shouldRunDraft(config.draft, entry.selfTestDrafts);
    });
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
      this.applySelectedTransport(params, selectedTransport, this.entryTransport(entry, tool), tool);
      if (config.draft && String(config.draft).toLowerCase() !== 'all') {
        const draftParam = tool.parameters.find((param) =>
          ['draft', 'draft_version'].includes(param.id));
        if (draftParam) params[draftParam.id] = config.draft;
      }

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
