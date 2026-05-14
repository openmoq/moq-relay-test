import { WsClient } from './ws-client.js';
import { ParamForm } from './param-form.js';
import { OutputPanel } from './output-panel.js';
import { RendererLoader } from './renderer-loader.js';
import { SelfTestUI } from './self-test-ui.js';
import { HistoryUI } from './history-ui.js';

class App {
  constructor() {
    this.endpointCookieName = 'moq_test_ui_endpoint_url';
    this.endpointQueryValue = null;
    this.autoRunRequested = false;
    this.defaultEndpointUrl = '';
    this.autoRunSelfTestStarted = false;
    this.boundRelayInputIds = new Set();

    this.initEndpointDefaults();

    this.tools = [];
    this.selectedTool = null;
    this.currentRunId = null;
    this.paramForm = new ParamForm();
    this.outputPanel = new OutputPanel();
    this.rendererLoader = new RendererLoader();
    this.selfTestUI = new SelfTestUI();
    this.historyUI = new HistoryUI();

    this.ws = new WsClient();
    this.ws.on('session', (data) => this.onSession(data));
    this.ws.on('tools-list', (data) => this.onToolsList(data));
    this.ws.on('run-started', (data) => this.onRunStarted(data));
    this.ws.on('output', (data) => this.onOutput(data));
    this.ws.on('run-complete', (data) => this.onRunComplete(data));
    this.ws.on('run-error', (data) => this.onRunError(data));
    this.ws.on('run-stopped', (data) => this.onRunStopped(data));
    this.ws.on('self-test-progress', (data) => this.selfTestUI.onProgress(data));
    this.ws.on('self-test-complete', (data) => this.selfTestUI.onComplete(data));
    this.ws.on('results-list', (data) => this.historyUI.onResultsList(data));
    this.ws.on('result-data', (data) => this.historyUI.onResultData(data));
    this.ws.on('connected', () => this.onConnected());
    this.ws.on('disconnected', () => this.onDisconnected());

    this.bindUI();
    this.ws.connect();
  }

  initEndpointDefaults() {
    const query = new URLSearchParams(window.location.search);
    const endpoint =
      query.get('endpoint') ||
      query.get('endpoint_url') ||
      query.get('endpointUrl') ||
      query.get('relay_url') ||
      query.get('relayUrl') ||
      '';

    const autoRunRaw = query.get('auto_run') || query.get('autorun') || query.get('autoRun') || '';

    this.endpointQueryValue = endpoint.trim();
    this.autoRunRequested = this.parseBooleanFlag(autoRunRaw);

    if (this.autoRunRequested && !this.endpointQueryValue) {
      console.warn('Ignoring auto-run because no endpoint URL was provided in query parameters.');
      this.autoRunRequested = false;
    }

    const cookieEndpoint = this.getCookie(this.endpointCookieName).trim();
    this.defaultEndpointUrl = this.endpointQueryValue || cookieEndpoint;
    if (this.endpointQueryValue) {
      this.setEndpointUrl(this.endpointQueryValue);
    }
  }

  parseBooleanFlag(value) {
    const normalized = String(value || '').trim().toLowerCase();
    return ['1', 'true', 'yes', 'y', 'on'].includes(normalized);
  }

  getCookie(name) {
    const prefix = `${name}=`;
    const parts = document.cookie ? document.cookie.split(';') : [];
    for (const part of parts) {
      const trimmed = part.trim();
      if (trimmed.startsWith(prefix)) {
        return decodeURIComponent(trimmed.slice(prefix.length));
      }
    }
    return '';
  }

  setCookie(name, value, days = 365) {
    const expires = new Date(Date.now() + days * 24 * 60 * 60 * 1000).toUTCString();
    document.cookie = `${name}=${encodeURIComponent(value)}; expires=${expires}; path=/; SameSite=Lax`;
  }

  setEndpointUrl(url) {
    const trimmed = (url || '').trim();
    if (!trimmed) return;
    this.defaultEndpointUrl = trimmed;
    this.setCookie(this.endpointCookieName, trimmed);
    this.applyEndpointToRelayInputs(trimmed);
  }

  isRelayParam(param) {
    if (!param) return false;
    const id = String(param.id || '').toLowerCase();
    const label = String(param.label || '').toLowerCase();
    return id === 'relay_url' || id === 'endpoint_url' || label.includes('relay url');
  }

  applyEndpointToRelayInputs(url) {
    const value = (url || '').trim();
    if (!value) return;

    const selfTestRelay = document.getElementById('st-relay-url');
    if (selfTestRelay) {
      selfTestRelay.value = value;
    }

    if (!Array.isArray(this.paramForm?.params) || this.paramForm.params.length === 0) return;
    for (const param of this.paramForm.params) {
      if (!this.isRelayParam(param)) continue;
      const input = document.getElementById(`param-${param.id}`);
      if (input) {
        input.value = value;
      }
    }
  }

  bindRelayInputSync() {
    const selfTestRelay = document.getElementById('st-relay-url');
    if (selfTestRelay && !this.boundRelayInputIds.has('st-relay-url')) {
      selfTestRelay.addEventListener('change', () => {
        const value = selfTestRelay.value.trim();
        if (value) {
          this.setEndpointUrl(value);
        }
      });
      this.boundRelayInputIds.add('st-relay-url');
    }

    if (!Array.isArray(this.paramForm?.params)) return;
    for (const param of this.paramForm.params) {
      if (!this.isRelayParam(param)) continue;
      const input = document.getElementById(`param-${param.id}`);
      if (!input) continue;
      if (this.boundRelayInputIds.has(input.id)) continue;
      input.addEventListener('change', () => {
        const value = input.value.trim();
        if (value) {
          this.setEndpointUrl(value);
        }
      });
      this.boundRelayInputIds.add(input.id);
    }
  }

  maybeAutoRunSelfTest() {
    if (!this.autoRunRequested || this.autoRunSelfTestStarted) return;
    this.autoRunSelfTestStarted = true;
    this.switchView('self-test');
    this.startSelfTest();
  }

  bindUI() {
    // Navigation
    document.querySelectorAll('.nav-btn').forEach(btn => {
      btn.addEventListener('click', () => this.switchView(btn.dataset.view));
    });

    // Run / Stop
    document.getElementById('btn-run').addEventListener('click', () => this.startRun());
    document.getElementById('btn-stop').addEventListener('click', () => this.stopRun());
    document.getElementById('btn-clear-output').addEventListener('click', () => this.outputPanel.clear());

    // Self-test
    document.getElementById('self-test-form').addEventListener('submit', (e) => {
      e.preventDefault();
      this.startSelfTest();
    });
    document.getElementById('btn-self-test-stop').addEventListener('click', () => this.stopSelfTest());

    this.applyEndpointToRelayInputs(this.defaultEndpointUrl);
    this.bindRelayInputSync();

    // History
    document.getElementById('btn-refresh-history').addEventListener('click', () => {
      this.ws.send('list-results', { tool: document.getElementById('history-tool-filter').value || undefined });
    });
    document.getElementById('history-tool-filter').addEventListener('change', () => {
      this.ws.send('list-results', { tool: document.getElementById('history-tool-filter').value || undefined });
    });
    this.historyUI.onViewResult = (tool, filename) => {
      this.ws.send('get-result', { tool, filename });
    };
  }

  switchView(view) {
    document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
    document.querySelector(`.nav-btn[data-view="${view}"]`).classList.add('active');
    document.querySelectorAll('.view').forEach(v => v.classList.add('hidden'));
    document.getElementById(`view-${view}`).classList.remove('hidden');

    if (view === 'history') {
      this.ws.send('list-results', {});
    }
  }

  onConnected() {
    const dot = document.getElementById('connection-status');
    dot.classList.remove('disconnected');
    dot.classList.add('connected');
    dot.title = 'Connected';
    this.ws.send('list-tools', {});
  }

  onDisconnected() {
    const dot = document.getElementById('connection-status');
    dot.classList.remove('connected');
    dot.classList.add('disconnected');
    dot.title = 'Disconnected';
  }

  onSession(data) {
    this.sessionId = data.sessionId;
  }

  onToolsList(data) {
    this.tools = data.tools;
    this.renderToolList();
    this.selfTestUI.setTools(this.tools);
    this.historyUI.setTools(this.tools);
    this.maybeAutoRunSelfTest();
  }

  renderToolList() {
    const container = document.getElementById('tool-list');
    container.innerHTML = '';

    const CATEGORY_ORDER = ['diagnostics', 'conformance', 'performance'];
    const grouped = {};
    for (const tool of this.tools) {
      const cat = (tool.category || 'other').toLowerCase();
      if (!grouped[cat]) grouped[cat] = [];
      grouped[cat].push(tool);
    }

    const order = [
      ...CATEGORY_ORDER.filter(c => grouped[c]),
      ...Object.keys(grouped).filter(c => !CATEGORY_ORDER.includes(c)),
    ];

    for (const cat of order) {
      const header = document.createElement('div');
      header.className = 'tool-list-section';
      header.textContent = cat.charAt(0).toUpperCase() + cat.slice(1);
      container.appendChild(header);

      for (const tool of grouped[cat]) {
        const card = document.createElement('div');
        card.className = 'tool-card';
        card.dataset.tool = tool.name;
        card.innerHTML = `<div class="tool-card-name">${tool.displayName}</div>`;
        card.addEventListener('click', () => this.selectTool(tool.name));
        container.appendChild(card);
      }
    }
  }

  selectTool(name) {
    this.selectedTool = this.tools.find(t => t.name === name);
    if (!this.selectedTool) return;

    // Highlight selected card
    document.querySelectorAll('.tool-card').forEach(c => c.classList.remove('selected'));
    document.querySelector(`.tool-card[data-tool="${name}"]`)?.classList.add('selected');

    // Show tool panel
    document.getElementById('no-tool-selected').classList.add('hidden');
    const panel = document.getElementById('tool-panel');
    panel.classList.remove('hidden');

    document.getElementById('tool-title').textContent = this.selectedTool.displayName;
    document.getElementById('tool-description').textContent = this.selectedTool.description || '';

    this.paramForm.render(this.selectedTool.parameters, document.getElementById('param-form'));
    this.applyEndpointToRelayInputs(this.defaultEndpointUrl);
    this.bindRelayInputSync();
    this.outputPanel.setFilters(this.selectedTool.filters);
    this.rendererLoader.unload();

    document.getElementById('output-section').classList.add('hidden');
    document.getElementById('btn-run').classList.remove('hidden');
    document.getElementById('btn-stop').classList.add('hidden');
    this.currentRunId = null;
  }

  startRun() {
    if (!this.selectedTool) return;
    const params = this.paramForm.getValues();
    if (!params) return; // Validation failed

    const endpoint = (params.relay_url || params.endpoint_url || '').trim();
    if (endpoint) {
      this.setEndpointUrl(endpoint);
    }

    this.ws.send('start-run', { toolId: this.selectedTool.name, params });
    document.getElementById('btn-run').classList.add('hidden');
    document.getElementById('btn-stop').classList.remove('hidden');
  }

  stopRun() {
    if (this.currentRunId) {
      this.ws.send('stop-run', { runId: this.currentRunId });
    }
  }

  onRunStarted(data) {
    this.currentRunId = data.runId;
    this.outputPanel.clear();
    document.getElementById('output-section').classList.remove('hidden');

    // Load renderer if tool has one
    if (this.selectedTool?.hasRenderer) {
      this.rendererLoader.load(this.selectedTool.name, document.getElementById('renderer-container'));
      document.getElementById('renderer-container').classList.remove('hidden');
    }

    this.updateActiveRuns();
  }

  onOutput(data) {
    this.outputPanel.appendLine(data.line, data.ts);
    this.rendererLoader.onLine(data.line);
  }

  onRunComplete(data) {
    this.outputPanel.appendSystem(`\n--- Run complete (exit code: ${data.exitCode}) ---`);
    this.rendererLoader.onComplete(data.exitCode);
    document.getElementById('btn-run').classList.remove('hidden');
    document.getElementById('btn-stop').classList.add('hidden');
    this.currentRunId = null;
    this.updateActiveRuns();
  }

  onRunError(data) {
    this.outputPanel.appendSystem(`\n--- Error: ${data.error} ---`);
    document.getElementById('btn-run').classList.remove('hidden');
    document.getElementById('btn-stop').classList.add('hidden');
    this.currentRunId = null;
    this.updateActiveRuns();
  }

  onRunStopped(data) {
    this.outputPanel.appendSystem('\n--- Run stopped ---');
    document.getElementById('btn-run').classList.remove('hidden');
    document.getElementById('btn-stop').classList.add('hidden');
    this.currentRunId = null;
    this.updateActiveRuns();
  }

  updateActiveRuns() {
    const list = document.getElementById('active-runs-list');
    const runs = [];
    if (this.currentRunId) {
      runs.push({ runId: this.currentRunId, tool: this.selectedTool?.name });
    }
    if (runs.length === 0) {
      list.innerHTML = '<p class="muted">No active runs</p>';
    } else {
      list.innerHTML = runs.map(r => `<div class="active-run">${r.tool} <span class="run-id">${r.runId.slice(0, 8)}</span></div>`).join('');
    }
  }

  startSelfTest() {
    const relayUrl = document.getElementById('st-relay-url').value.trim();
    const transport = document.getElementById('st-transport').value;
    const draft = document.getElementById('st-draft').value;
    if (relayUrl) {
      this.setEndpointUrl(relayUrl);
    }
    this.ws.send('start-self-test', { relayUrl, transport, draft });
    document.getElementById('btn-self-test-run').classList.add('hidden');
    document.getElementById('btn-self-test-stop').classList.remove('hidden');
    this.selfTestUI.start();
  }

  stopSelfTest() {
    if (this.selfTestUI.selfTestId) {
      this.ws.send('stop-self-test', { selfTestId: this.selfTestUI.selfTestId });
    }
    document.getElementById('btn-self-test-run').classList.remove('hidden');
    document.getElementById('btn-self-test-stop').classList.add('hidden');
  }
}

// Boot
new App();
