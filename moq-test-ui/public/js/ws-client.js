/**
 * WebSocket client with auto-reconnect and event emitter.
 */
export class WsClient {
  constructor() {
    this.ws = null;
    this.listeners = {};
    this.reconnectDelay = 1000;
    this.maxReconnectDelay = 30000;
    this.currentDelay = this.reconnectDelay;
  }

  on(event, fn) {
    if (!this.listeners[event]) this.listeners[event] = [];
    this.listeners[event].push(fn);
  }

  emit(event, data) {
    for (const fn of this.listeners[event] || []) {
      fn(data);
    }
  }

  connect() {
    const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
    this.ws = new WebSocket(`${proto}//${location.host}/ws`);

    this.ws.onopen = () => {
      this.currentDelay = this.reconnectDelay;
      this.emit('connected');
    };

    this.ws.onclose = () => {
      this.emit('disconnected');
      setTimeout(() => this.connect(), this.currentDelay);
      this.currentDelay = Math.min(this.currentDelay * 2, this.maxReconnectDelay);
    };

    this.ws.onerror = () => {
      this.ws.close();
    };

    this.ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data);
        this.emit(msg.type, msg);
      } catch (err) {
        console.error('WS parse error:', err);
      }
    };
  }

  send(type, data = {}) {
    if (this.ws?.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify({ type, ...data }));
    }
  }
}
