const Redis = require('ioredis');

const WORKER_ID = 'worker-1';
const redis = new Redis('redis://redis:6379');

async function processTasks() {
  while (true) {
    const taskId = await redis.lpop('tasks_queue');
    if (!taskId) {
      await sleep(1000);
      continue;
    }

    try {
      // Get task
      const task = await redis.hgetall(`task:${taskId}`);
      if (!task.id) continue;

      // Update status
      await redis.hset(`task:${taskId}`, {
        status: 'processing',
        worker: WORKER_ID
      });

      // Process
      console.log(`Processing ${task.type} task ${taskId}`);
      await sleep(task.type === 'slow' ? 5000 : 1000);

      // Complete
      await redis.hset(`task:${taskId}`, {
        status: 'completed',
        completed: new Date().toISOString()
      });

    } catch (err) {
      console.error(`Failed ${taskId}:`, err.message);
      await redis.hset(`task:${taskId}`, { status: 'failed' });
    }
  }
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// Start worker
processTasks().catch(console.error);
console.log(`Worker ${WORKER_ID} started`);

// Clean shutdown
process.on('SIGTERM', async () => {
  await redis.quit();
  process.exit(0);
});