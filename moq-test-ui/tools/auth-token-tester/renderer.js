const METHODS = [
  'client_setup', 'publish_namespace', 'subscribe_namespace',
  'subscribe', 'publish', 'fetch', 'track_status',
];

export default class {
  init(containerEl) {
    this.container = containerEl;
    this.report = null;
    containerEl.replaceChildren();
    this.context = document.createElement('p');
    this.context.className = 'muted';
    this.context.textContent = 'Waiting for token test results...';
    this.table = document.createElement('table');
    this.table.className = 'metrics-table';
    const header = this.table.createTHead().insertRow();
    for (const label of ['Method', 'Authorization', 'Request result', 'Details', 'Latency']) {
      const cell = document.createElement('th');
      cell.textContent = label;
      header.appendChild(cell);
    }
    this.body = this.table.createTBody();
    this.summary = document.createElement('div');
    containerEl.append(this.context, this.table, this.summary);
    this.renderRows();
  }

  onLine(line) {
    if (!line.trimStart().startsWith('{')) return;
    let report;
    try {
      report = JSON.parse(line);
    } catch (error) {
      this.context.textContent = `Invalid test report: ${error.message}`;
      return;
    }
    if (report.type !== 'auth-token-test') return;
    this.report = report;
    this.context.textContent = report.error
      ? `Token test failed: ${report.error}`
      : `${report.url} | ${report.transport} | Authority: ${report.authority ?? '-'} | Path: ${report.path ?? '-'} | Draft ${report.draft} | Token type: ${report.token_type ?? 0} | Namespace: ${report.namespace} | Track: ${report.track}`;
    this.renderRows();
  }

  renderRows() {
    this.body.replaceChildren();
    for (const method of METHODS) {
      const result = this.report?.tests?.[method];
      const authorization = {
        allowed: 'Allowed',
        allowed_inferred: 'Allowed (inferred)',
        denied: 'Denied',
        unknown: 'Unknown',
      }[result?.authorization] || (result?.allowed ? 'Allowed' : 'Unknown');
      const row = this.body.insertRow();
      const authorizationAllowed = result?.authorization === 'allowed' ||
        result?.authorization === 'allowed_inferred' ||
        (result?.authorization === undefined && result?.allowed);
      if (result) row.className = authorizationAllowed ? 'status-pass' : 'status-fail';
      for (const value of [
        method,
        result ? authorization : 'Pending',
        result ? (result.allowed ? 'Accepted' : result.authorization === 'allowed_inferred' ? 'Namespace/track not found' : 'Not accepted') : 'Pending',
        result?.detail || '',
        result?.latency_ms != null ? `${result.latency_ms} ms` : '-',
      ]) {
        row.insertCell().textContent = value;
      }
      if (authorizationAllowed) row.children[1].className = 'summary-passed';
    }
  }

  getSummary() {
    const tests = Object.values(this.report?.tests || {});
    if (!tests.length) return null;
    const allowed = tests.filter(test => test.authorization === 'allowed' ||
      test.authorization === 'allowed_inferred' ||
      (test.authorization === undefined && test.allowed)).length;
    return `${allowed}/${METHODS.length} requests authorizations allowed (or inferred)`;
  }

  onComplete(exitCode) {
    const complete = METHODS.every(method => this.report?.tests?.[method]);
    this.summary.className = `renderer-summary ${exitCode === 0 && complete ? 'success' : 'failure'}`;
    this.summary.textContent = exitCode === 0 && complete
      ? this.getSummary()
      : `Token test failed or incomplete. ${this.report?.error || this.getSummary() || 'No results received; see raw output.'}`;
  }

  destroy() {
    this.container = null;
    this.report = null;
    this.context = null;
    this.table = null;
    this.body = null;
    this.summary = null;
  }
}
