import express from 'express';
import http from 'http';
import path from 'path';
import { fileURLToPath } from 'url';
import { ToolRegistry } from './tool-registry.js';
import { DockerExecutor } from './docker-executor.js';
import { ResultsStore } from './results-store.js';
import { SelfTestOrchestrator } from './self-test.js';
import { createWSServer } from './ws-server.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.join(__dirname, '..');

const app = express();
const server = http.createServer(app);
const PORT = process.env.PORT || 3000;

// --- Initialize modules ---
const toolRegistry = new ToolRegistry(path.join(ROOT, 'tools'));
toolRegistry.load();

const dockerExecutor = new DockerExecutor();
const resultsStore = new ResultsStore(path.join(ROOT, 'results'));
const selfTestOrchestrator = new SelfTestOrchestrator(toolRegistry, dockerExecutor, resultsStore);

// --- WebSocket ---
createWSServer(server, toolRegistry, dockerExecutor, resultsStore, selfTestOrchestrator);

// --- REST API ---
app.use(express.json());

// Serve static frontend
app.use(express.static(path.join(ROOT, 'public')));

// Serve tool renderers
app.use('/tools', express.static(path.join(ROOT, 'tools')));

// API: list tools
app.get('/api/tools', (req, res) => {
  res.json(toolRegistry.list());
});

// API: list results
app.get('/api/results', (req, res) => {
  res.json(resultsStore.list(req.query.tool || null));
});

// API: get a specific result
app.get('/api/results/:tool/:filename', (req, res) => {
  const result = resultsStore.load(req.params.tool, req.params.filename);
  if (!result) return res.status(404).json({ error: 'Not found' });
  res.json(result);
});

// --- Start ---
server.listen(PORT, () => {
  console.log(`Generic Runner listening on http://localhost:${PORT}`);
});

// Graceful shutdown
process.on('SIGTERM', async () => {
  console.log('Shutting down...');
  await dockerExecutor.stopAll();
  server.close();
  process.exit(0);
});

process.on('SIGINT', async () => {
  console.log('Shutting down...');
  await dockerExecutor.stopAll();
  server.close();
  process.exit(0);
});
