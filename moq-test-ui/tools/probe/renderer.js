/**
 * Renderer for probe output: collects all stdout, extracts the JSON report,
 * and renders a relay status table on completion.
 */
export default {
  container: null,
  buffer: [],
  _liveEp: null,
  _totalEp: 0,

  init(containerEl) {
    this.container = containerEl;
    this.buffer = [];
    this._liveEp = null;
    this._totalEp = 0;

    containerEl.innerHTML = `
      <div class="renderer-probe">
        <div id="probe-results"><p class="muted">Waiting for probe output…</p></div>
      </div>
    `;
  },

  onLine(line) {
    this.buffer.push(line);
    // Attempt parse on every line — cheap, and ensures we don't miss the closing
    // brace regardless of whether it arrives with a trailing newline or not.
    this._tryParse();
  },

  _tryParse() {
    const lines = this.buffer;
    // Find the first line that opens the top-level JSON object
    const startIdx = lines.findIndex(l => /^\s*\{/.test(l));
    if (startIdx === -1) return;
    // Try progressively longer slices, stopping at each line that starts with '}'.
    // This correctly handles nested '}' lines inside the JSON while ignoring
    // post-JSON log lines that don't contain '}'.
    for (let end = startIdx; end < lines.length; end++) {
      if (/^\s*\}/.test(lines[end])) {
        const candidate = lines.slice(startIdx, end + 1).join('\n');
        try {
          const data = JSON.parse(candidate);
          this._render(data);
          return;
        } catch (_) {
          // Not a complete object yet — keep scanning
        }
      }
    }
  },

  _render(data) {
    const el = this.container?.querySelector('#probe-results');
    if (!el) return;

    // Count individual draft probes (not just top-level endpoints)
    let totalProbes = 0, liveProbes = 0;
    const draftTally = {};  // draft → { live, total }
    for (const relay of Object.values(data.relays || {})) {
      for (const ep of relay.endpoints || []) {
        for (const probe of ep.probes || []) {
          totalProbes++;
          if (probe.live) liveProbes++;
          const d = probe.draft || 'unknown';
          if (!draftTally[d]) draftTally[d] = { live: 0, total: 0 };
          draftTally[d].total++;
          if (probe.live) draftTally[d].live++;
        }
        // Fallback: endpoint with no probes array
        if (!ep.probes?.length) {
          totalProbes++;
          if (ep.live) liveProbes++;
        }
      }
    }
    this._liveProbes = liveProbes;
    this._totalProbes = totalProbes;
    this._draftTally = draftTally;

    const allLive = liveProbes === totalProbes;
    const summaryClass = liveProbes === 0 ? 'failure' : allLive ? 'success' : 'partial';

    // Draft breakdown chips
    const draftChips = Object.entries(draftTally)
      .sort(([a], [b]) => a.localeCompare(b))
      .map(([draft, t]) => {
        const cls = t.live === t.total ? 'probe-chip-live' : t.live === 0 ? 'probe-chip-down' : 'probe-chip-partial';
        return `<span class="probe-chip ${cls}">${draft}: ${t.live}/${t.total}</span>`;
      }).join('');

    let html = `
      <div class="probe-summary ${summaryClass}">
        <span class="probe-summary-count">${liveProbes} / ${totalProbes}</span>
        <span class="probe-summary-label">probes live</span>
        <span class="probe-draft-chips">${draftChips}</span>
        <span class="probe-summary-ts">· ${data.timestamp ? new Date(data.timestamp).toLocaleTimeString() : 'N/A'}</span>
      </div>`;
    html += '<table class="metrics-table">';
    html += `<thead><tr>
      <th>Relay</th><th>Endpoint</th><th>Transport</th>
      <th>Live</th><th>Drafts</th><th>Latency</th><th>Error</th>
    </tr></thead><tbody>`;

    for (const [id, relay] of Object.entries(data.relays || {})) {
      for (const ep of relay.endpoints || []) {
        const liveClass = ep.live ? 'status-pass' : 'status-fail';
        const drafts = (ep.drafts || []).join(', ');
        html += `<tr class="${liveClass}">
          <td>${relay.name || id}</td>
          <td class="url-cell">${ep.url || '—'}</td>
          <td>${ep.transport || '—'}</td>
          <td>${ep.live ? '✓ Live' : '✗ Down'}</td>
          <td>${drafts || '—'}</td>
          <td>${ep.latency_ms != null ? ep.latency_ms + ' ms' : '—'}</td>
          <td>${ep.error || '—'}</td>
        </tr>`;

        for (const probe of ep.probes || []) {
          const pClass = probe.live ? 'status-pass' : 'status-fail';
          html += `<tr class="probe-detail ${pClass}">
            <td></td>
            <td class="indent">↳ ${probe.alpn || 'Probe'}</td>
            <td>${probe.draft || '—'}</td>
            <td>${probe.live ? '✓' : '✗'}</td>
            <td>${probe.version_hex || '—'}</td>
            <td></td>
            <td>${probe.error || '—'}</td>
          </tr>`;
        }
      }
    }

    html += '</tbody></table>';
    el.innerHTML = html;
  },

  getSummary() {
    if (this._totalProbes == null) return null;
    const down = this._totalProbes - this._liveProbes;
    /*
    const draftParts = Object.entries(this._draftTally || {})
      .sort(([a], [b]) => a.localeCompare(b))
      .map(([d, t]) => `${d}: ${t.live}/${t.total}`)
      .join(', ');
    */
    const base = `Probes: ${this._liveProbes} live / ${down} down`;
    return base;
  },

  onComplete(exitCode) {
    if (!this.container) return;

    // Final attempt in case the closing '}' wasn't matched incrementally
    this._tryParse();

    const summary = document.createElement('div');
    summary.className = `renderer-summary ${exitCode === 0 ? 'success' : 'failure'}`;
    summary.textContent = this.getSummary() || 'Probe complete';
    this.container.appendChild(summary);
  },

  destroy() {
    this.container = null;
    this.buffer = [];
  },
};
