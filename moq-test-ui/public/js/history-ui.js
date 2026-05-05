/**
 * History UI: lists saved run results, allows viewing details.
 */
export class HistoryUI {
  constructor() {
    this.tools = [];
    this.onViewResult = null; // callback set by app
  }

  setTools(tools) {
    this.tools = tools;
    const filter = document.getElementById('history-tool-filter');
    if (!filter) return;
    // Keep the "All Tools" option, add tool options
    while (filter.options.length > 1) filter.remove(1);
    for (const t of tools) {
      const opt = document.createElement('option');
      opt.value = t.name;
      opt.textContent = t.displayName;
      filter.appendChild(opt);
    }
  }

  onResultsList(data) {
    const tbody = document.getElementById('history-tbody');
    if (!tbody) return;
    tbody.innerHTML = '';

    if (!data.results || data.results.length === 0) {
      tbody.innerHTML = '<tr><td colspan="4" class="muted">No results found</td></tr>';
      return;
    }

    for (const r of data.results) {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td>${r.tool}</td>
        <td>${new Date(r.mtime).toLocaleString()}</td>
        <td>${r.size > 0 ? '—' : '—'}</td>
        <td><button class="btn btn-small btn-view" data-tool="${r.tool}" data-file="${r.filename}">View</button></td>
      `;
      const btn = tr.querySelector('.btn-view');
      btn.addEventListener('click', () => {
        if (this.onViewResult) this.onViewResult(r.tool, r.filename);
      });
      tbody.appendChild(tr);
    }
  }

  onResultData(data) {
    if (!data.result) return;
    const r = data.result;

    // Show in a modal-like overlay
    const overlay = document.createElement('div');
    overlay.className = 'result-overlay';
    overlay.innerHTML = `
      <div class="result-modal">
        <div class="result-modal-header">
          <h3>${r.tool} — ${new Date(r.startedAt).toLocaleString()}</h3>
          <button class="btn btn-small btn-close-modal">&times;</button>
        </div>
        <div class="result-meta">
          <span>Exit code: ${r.exitCode}</span>
          <span>Duration: ${r.startedAt && r.completedAt ?
            Math.round((new Date(r.completedAt) - new Date(r.startedAt)) / 1000) + 's' : 'N/A'}</span>
          <span>Params: ${JSON.stringify(r.params)}</span>
        </div>
        <pre class="result-output">${(r.output || []).map(o => o.line).join('\n')}</pre>
      </div>
    `;
    overlay.querySelector('.btn-close-modal').addEventListener('click', () => overlay.remove());
    overlay.addEventListener('click', (e) => { if (e.target === overlay) overlay.remove(); });
    document.body.appendChild(overlay);
  }
}
