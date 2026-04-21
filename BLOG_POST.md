# From Sockets to Server: What I Learned Building My Own Web Server

---

## 1) The Question That Started It

I started this project with a simple curiosity: if I remove frameworks and build the HTTP stack myself in C++, where does performance actually go under load?

The mental model that guided every benchmark is this: as architecture improves, bottlenecks move across layers:

**application queueing → concurrency model → I/O readiness → kernel/network stack**.

So this server became an experiment in bottleneck migration, not a clone of NGINX.

---

## 2) The Starting Architecture

The implementation is intentionally layered:

1. Socket setup + accept loop
2. HTTP I/O and parsing
3. Routing / handler dispatch
4. Response writing and optional static file serving
5. Concurrency strategy (thread pool and/or epoll)

That separation made each performance step easier to isolate and explain.

---

## 3) Chapter One: `BACKLOG = MAX` and the Plateau

`BACKLOG = MAX` means configuring the accept queue as large as practical so bursty arrivals are less likely to be dropped at accept.

### Workload snapshots (20s test, `wrk -t4`)

- 50 conns: **5009.75 req/s**, avg latency **9.87ms**
- 100 conns: **5087.10 req/s**, avg latency **19.68ms**
- 200 conns: **4973.69 req/s**, avg latency **40.40ms**
- 400 conns: **5137.53 req/s**, avg latency **77.40ms**
- 800 conns: **5135.72 req/s**, avg latency **154.08ms**

### Why this output happened

Throughput plateaued around **~5K req/s** while latency kept rising. Intuitively, this is M/M/1-like behavior: a **single service lane with a growing queue**. Once capacity saturates, extra concurrency mostly increases waiting time.

So the first bottleneck was queueing saturation, not compute.

---

## 4) Chapter Two: Thread Pool Only

### Measurement (4 threads / 800 conns)

- **21,489.98 req/s**
- avg latency **23.50ms**
- p99 **48.85ms**

### Why this output improved

The thread pool removed concrete baseline bottlenecks:

- less blocking per connection on the critical path
- more parallel request handling
- less idle CPU while waiting on other sockets

That is why throughput jumped from ~5K to ~21.5K req/s.

### Perf note

At this stage, perf already shifts strongly toward syscall/TCP paths (`write`, `tcp_sendmsg`, transmit/output paths), which is the beginning of the kernel-side frontier discussed in Section 8.

---

## 5) Chapter Three: Epoll Only

### Measurement (4 threads / 800 conns)

- **43,308.99 req/s**
- avg latency **18.40ms**
- p99 **21.92ms**

### Why this output jumped

Epoll changes the scaling mechanism from O(N) socket checking to readiness-driven dispatch:

- no scanning across inactive sockets
- kernel reports exactly which fds are ready
- wakeups map to useful I/O work

This is a fundamental scalability improvement, not a small optimization.

### Perf note

Perf still shows heavy syscall/TCP symbols, but now with higher useful throughput and tighter tail latency. `writev` appears prominently, consistent with better batching/vectored output behavior.

---

## 6) Chapter Four: Epoll + Threading

### Measurement (4 threads / 1200 conns)

- **57,650.29 req/s**
- avg latency **20.50ms**
- p99 **30.39ms**

### Why this became the best mode

This combines both major levers:

1. readiness-based I/O (epoll)
2. parallel execution capacity (threads)

So the server can scale across many connected clients and many simultaneously ready events.

### Perf note

Worker threads stay busy while networking/syscall work dominates the remaining cost envelope, which matches the bottleneck-migration model.

---

## 7) NGINX Comparison: The Last Gap

### Measurement (NGINX @ 4 threads / 800 conns)

- **60,438.67 req/s**
- avg latency **13.09ms**
- p99 **32.54ms**

### Why NGINX still leads

This gap is refinement, not mystery: lower userspace overhead, tighter event-loop control, better buffering, and strong batching around the same kernel networking primitives.

---

## 8) The Perf Story (Explained Once)

Across all modes, perf repeatedly highlights syscall and networking paths (`writev`/write, `tcp_sendmsg`, transmit/output paths, softirq/NAPI).

What changed by stage:

- early: queueing and concurrency limits dominate
- middle: parallelism and readiness reduce idle/blocked time
- late: kernel networking cost becomes the primary frontier

So the same core insight applies throughout, but the dominant limiting layer moves as architecture improves.

---

## 9) Compact Progression Summary

- Blocking + backlog-max baseline → **~5K req/s** (queueing bottleneck)
- Thread pool → **21,489.98 req/s** (parallelism)
- Epoll → **43,308.99 req/s** (I/O readiness efficiency)
- Epoll + threads → **57,650.29 req/s** (combined scaling)
- NGINX → **60,438.67 req/s** (optimized steady state)

---

## 10) Final Takeaway

If I had to summarize this project in one line:

> It started as “build an HTTP server,” and became “track how bottlenecks move as architecture improves.”

Concretely, the path was:

**queueing → concurrency → I/O readiness → kernel networking**.

That progression is the core result, and the benchmark/perf data supports it end-to-end.
