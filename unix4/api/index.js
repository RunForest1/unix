const express = require('express');
const Redis = require('ioredis');

const app = express();
const PORT = 3000;
const INSTANCE_ID = 'api-1';
const redis = new Redis('redis://redis:6379');

app.use(express.json());

// Health check
app.get('/health', async (req, res) => {
  try {
    await redis.ping();
    res.json({ status: 'ok', instance: INSTANCE_ID });
  } catch {
    res.status(500).json({ status: 'error' });
  }
});

// Main endpoint
app.get('/', async (req, res) => {

  const total = await redis.incr('requests_total');
  const instanceCount = await redis.incr(`requests_${INSTANCE_ID}`);

  res.json({
    instance: INSTANCE_ID,
    total,
    instanceCount,
    timestamp: Date.now()
  });
});

// Create task
app.post('/tasks', async (req, res) => {

  const { type } = req.body;
  if (!type) return res.status(400).json({ error: 'Type required' });

  const taskId = `task_${Date.now()}`;
  const task = {
    id: taskId,
    type,
    status: 'pending',
    created: new Date().toISOString()
  };

  await redis.hset(`task:${taskId}`, task); // Сохраняем задачу
  await redis.rpush('tasks_queue', taskId); // Добавляем в очередь

  res.json({ id: taskId, status: 'created' });
});

// Get task
app.get('/tasks/:id', async (req, res) => {
  const task = await redis.hgetall(`task:${req.params.id}`);
  task ? res.json(task) : res.status(404).json({ error: 'Not found' });
});

// List tasks
app.get('/tasks', async (req, res) => {

  const keys = await redis.keys('task:*');
  const tasks = await Promise.all(
    keys.slice(0, 10).map(k => redis.hgetall(k))
  );
  res.json({ tasks, count: tasks.length });
});

// Start server
app.listen(PORT, () => {
  console.log(`API ${INSTANCE_ID} on port ${PORT}`);
  // Register instance
  redis.sadd('instances', INSTANCE_ID);
});

// Clean shutdown
process.on('SIGTERM', async () => {
  await redis.srem('instances', INSTANCE_ID);
  await redis.quit();
  process.exit(0);
});