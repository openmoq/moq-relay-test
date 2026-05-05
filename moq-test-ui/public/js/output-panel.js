/**
 * Output panel with scrolling log display and regex-based filter toggles.
 * Uses a simple pre-based display (xterm.js can be swapped in later).
 */
export class OutputPanel {
  constructor() {
    this.lines = [];       // All raw lines
    this.filters = [];     // { id, label, pattern, default, compiled }
    this.activeFilters = new Set();
    this.containerEl = null;
    this.outputEl = null;
    this.autoScroll = true;
  }

  setFilters(filters) {
    this.filters = (filters || []).map(f => ({
      ...f,
      compiled: new RegExp(f.pattern, 'i'),
    }));
    this.activeFilters.clear();
    // Enable all filters by default for cleanest output
    for (const f of this.filters) {
      this.activeFilters.add(f.id);
    }
    this.renderFilterToggles();
  }

  renderFilterToggles() {
    const container = document.getElementById('filter-toggles');
    if (!container) return;
    container.innerHTML = '';

    for (const filter of this.filters) {
      const label = document.createElement('label');
      label.className = 'filter-toggle';
      const cb = document.createElement('input');
      cb.type = 'checkbox';
      cb.checked = this.activeFilters.has(filter.id);
      cb.addEventListener('change', () => {
        if (cb.checked) {
          this.activeFilters.add(filter.id);
        } else {
          this.activeFilters.delete(filter.id);
        }
        this.rerender();
      });
      label.appendChild(cb);
      label.appendChild(document.createTextNode(' ' + filter.label));
      container.appendChild(label);
    }
  }

  isFiltered(line) {
    for (const filter of this.filters) {
      if (this.activeFilters.has(filter.id) && filter.compiled.test(line)) {
        return true;
      }
    }
    return false;
  }

  clear() {
    this.lines = [];
    this.ensureOutputEl();
    if (this.outputEl) this.outputEl.textContent = '';
  }

  ensureOutputEl() {
    const container = document.getElementById('terminal-container');
    if (!container) return;
    if (!this.outputEl || this.outputEl.parentElement !== container) {
      container.innerHTML = '';
      this.outputEl = document.createElement('pre');
      this.outputEl.className = 'output-log';
      container.appendChild(this.outputEl);

      // Auto-scroll detection
      container.addEventListener('scroll', () => {
        const { scrollTop, scrollHeight, clientHeight } = container;
        this.autoScroll = (scrollHeight - scrollTop - clientHeight) < 50;
      });
    }
  }

  appendLine(line, ts) {
    this.lines.push({ line, ts });
    if (!this.isFiltered(line)) {
      this.ensureOutputEl();
      if (this.outputEl) {
        const span = document.createElement('span');
        span.className = 'output-line';
        span.textContent = line + '\n';
        this.outputEl.appendChild(span);
        this.scrollToBottom();
      }
    }
  }

  appendSystem(text) {
    this.ensureOutputEl();
    if (this.outputEl) {
      const span = document.createElement('span');
      span.className = 'output-line system';
      span.textContent = text + '\n';
      this.outputEl.appendChild(span);
      this.scrollToBottom();
    }
  }

  scrollToBottom() {
    if (!this.autoScroll) return;
    const container = document.getElementById('terminal-container');
    if (container) {
      requestAnimationFrame(() => {
        container.scrollTop = container.scrollHeight;
      });
    }
  }

  rerender() {
    this.ensureOutputEl();
    if (!this.outputEl) return;
    this.outputEl.innerHTML = '';
    for (const { line } of this.lines) {
      if (!this.isFiltered(line)) {
        const span = document.createElement('span');
        span.className = 'output-line';
        span.textContent = line + '\n';
        this.outputEl.appendChild(span);
      }
    }
    this.scrollToBottom();
  }
}
