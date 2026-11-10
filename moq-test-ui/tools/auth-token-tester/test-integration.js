import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { test } from 'node:test';
import { DockerExecutor } from '../../src/docker-executor.js';
import { ToolRegistry } from '../../src/tool-registry.js';
import Renderer from './renderer.js';

const toolsDir = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

test('registry places tester directly below generator with all required inputs', () => {
  const registry = new ToolRegistry(toolsDir);
  registry.load();
  const auth = registry.list().filter(tool => tool.category === 'authentication')
    .sort((a, b) => a.order - b.order);
  const generatorIndex = auth.findIndex(tool => tool.name === 'auth-token-generator');
  assert.equal(auth[generatorIndex + 1].name, 'auth-token-tester');
  const tool = registry.get('auth-token-tester');
  assert.equal(tool._hasRenderer, true);
  assert.equal(tool.persistResults, false);
  assert.deepEqual(tool.parameters.map(param => param.id), [
    'relay_url', 'transport', 'draft_version', 'namespace', 'track_name', 'token_type', 'auth_token',
  ]);
  assert.equal(tool.parameters.find(param => param.id === 'track_name').required, false);
  assert.equal(tool.docker.image, registry.get('probe').docker.image);
});

test('secret config is mounted silently, preserves exact values and never enters CLI', () => {
  const registry = new ToolRegistry(toolsDir);
  registry.load();
  const tool = registry.get('auth-token-tester');
  const executor = new DockerExecutor();
  const params = {
    relay_url: 'https://localhost/moq', transport: 'wt', draft_version: '22',
    namespace: 'namespace with spaces/"quoted"', track_name: '',
    token_type: 16,
    auth_token: 'secret with spaces; $(do-not-run) "quoted"\nsecond line',
  };
  const logs = [];
  const prepared = executor.prepareTempFiles(tool, params, 'test-auth-token', (_, line) => logs.push(line));
  try {
    assert.deepEqual(executor.buildArgs(tool, params), []);
    assert.equal(JSON.stringify(tool.docker).includes(params.auth_token), false);
    const configBind = prepared.binds.find(bind => bind.includes('/tmp/auth_token_test.json:'));
    const configPath = configBind.slice(0, configBind.indexOf(':/tmp/'));
    assert.deepEqual(JSON.parse(fs.readFileSync(configPath, 'utf8')), params);
    assert.equal(logs.join('\n').includes(params.auth_token), false);
    assert.equal(logs.join('\n').includes(params.namespace), false);
  } finally {
    prepared.cleanup();
  }
});

class Element {
  constructor() {
    this.children = [];
    this.textContent = '';
  }
  append(...elements) { this.children.push(...elements); }
  appendChild(element) { this.append(element); }
  replaceChildren(...elements) { this.children = elements; }
  createTHead() { return this.addElement(); }
  createTBody() { return this.addElement(); }
  insertRow() { return this.addElement(); }
  insertCell() { return this.addElement(); }
  addElement() {
    const element = new Element();
    this.append(element);
    return element;
  }
}

test('renderer shows seven rows, literal untrusted details and incomplete/failure states', () => {
  const previous = globalThis.document;
  globalThis.document = { createElement: () => new Element() };
  const renderer = new Renderer();
  try {
    renderer.init(new Element());
    assert.equal(renderer.body.children.length, 7);
    assert.equal(renderer.body.children[0].children[1].textContent, 'Pending');
    const methods = [
      'client_setup', 'publish_namespace', 'subscribe_namespace',
      'subscribe', 'publish', 'fetch', 'track_status',
    ];
    const report = {
      type: 'auth-token-test', url: 'https://relay.example', transport: 'wt',
      draft: 16, token_type: 16, namespace: 'live', track: 'video',
      authority: 'relay.example', path: '/moq-relay',
      tests: Object.fromEntries(methods.map((method, index) => [
        method, { allowed: index === 0, detail: '<img src=x onerror=alert(1)>', latency_ms: 2 },
      ])),
    };
    renderer.onLine('[runner] starting');
    renderer.onLine(JSON.stringify(report));
    assert.match(renderer.context.textContent, /Token type: 16/);
    assert.match(renderer.context.textContent, /Authority: relay.example \| Path: \/moq-relay/);
    assert.equal(renderer.body.children[0].children[1].textContent, 'Allowed');
    assert.equal(renderer.body.children[1].children[1].textContent, 'Unknown');
    assert.equal(renderer.body.children[1].children[2].textContent, 'Not accepted');
    assert.equal(renderer.body.children[1].children[3].textContent, '<img src=x onerror=alert(1)>');
    renderer.onComplete(0);
    assert.equal(renderer.summary.textContent, '1/7 requests authorizations allowed (or inferred)');
    report.tests.subscribe.authorization = 'allowed_inferred';
    report.tests.track_status.authorization = 'allowed_inferred';
    report.tests.fetch.authorization = 'denied';
    renderer.onLine(JSON.stringify(report));
    assert.equal(renderer.body.children[3].children[1].textContent, 'Allowed (inferred)');
    assert.equal(renderer.body.children[3].children[2].textContent, 'Namespace/track not found');
    assert.equal(renderer.body.children[5].children[1].textContent, 'Denied');
    assert.equal(renderer.body.children[3].className, 'status-pass');
    assert.equal(renderer.body.children[3].children[1].className, 'summary-passed');
    assert.equal(renderer.body.children[6].children[1].textContent, 'Allowed (inferred)');
    assert.equal(renderer.body.children[6].className, 'status-pass');
    assert.equal(renderer.body.children[5].className, 'status-fail');
    renderer.onComplete(0);
    assert.equal(renderer.summary.textContent, '3/7 requests authorizations allowed (or inferred)');
    delete report.tests.fetch;
    renderer.onLine(JSON.stringify(report));
    renderer.onComplete(0);
    assert.match(renderer.summary.textContent, /failed or incomplete/);
    renderer.onLine(JSON.stringify({ type: 'auth-token-test', error: 'Invalid token' }));
    renderer.onComplete(1);
    assert.match(renderer.context.textContent, /Invalid token/);
    assert.equal(renderer.summary.className, 'renderer-summary failure');
  } finally {
    renderer.destroy();
    globalThis.document = previous;
  }
});
