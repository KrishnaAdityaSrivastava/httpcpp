# From Sockets to Throughput Curves: A Story of Building (and Stressing) My HTTP Server

---

## 1) The Guiding Mental Model

I built this HTTP server to answer one question: **where does performance go as load increases?**

The key model that emerged is simple: as architecture improves, bottlenecks move across layers:

1. **Application/queueing behavior**
2. **Concurrency model**
3. **I/O readiness strategy**
4. **Kernel/network stack cost**

So this project is not “one optimization.” It is a sequence of bottleneck migrations measured with `wrk` and `perf`.

---

## 2) The Starting Architecture

The implementation is layered:

1. **Socket setup + accept loop**
2. **HTTP I/O and parsing**
3. **Routing / handler dispatch**
4. **Response writing and static file path**
5. **Concurrency mode (thread pool and/or epoll)**

That separation made each performance step measurable and explainable.

---

## 3) Chapter One: `BACKLOG = MAX` and Saturation

`BACKLOG = MAX` means the accept queue is configured to be as large as practical, reducing dropped incoming connections when bursts arrive.

### Workload snapshots (20s, `wrk -t4`)

- **50 conns**: 5009.75 req/s, avg latency 9.87ms
- **100 conns**: 5087.10 req/s, avg latency 19.68ms
- **200 conns**: 4973.69 req/s, avg latency 40.40ms
- **400 conns**: 5137.53 req/s, avg latency 77.40ms
- **800 conns**: 5135.72 req/s, avg latency 154.08ms

### Why this output happened

Throughput stayed near **5.0K–5.1K req/s** while latency rose sharply. This is M/M/1-like queueing behavior: think **one service lane with a growing line**. Once service capacity saturates, new connections mostly add waiting time, not completed work.

So the first bottleneck was queueing/saturation, not raw CPU arithmetic.

---

## 4) Chapter Two: Thread Pool Only

### Measurement (4 threads / 800 conns)

- **21,489.98 req/s**
- **avg latency 23.50ms**
- **p99 48.85ms**

### What changed

This stage removed concrete bottlenecks from the baseline path:

- less time blocked per connection
- more request handling in parallel
- less idle CPU while other sockets wait

Result: throughput jumped from ~5.1K to ~21.5K req/s because worker capacity was used continuously instead of stalling behind connection-level blocking.

---

## 5) Chapter Three: Epoll Only

### Measurement (4 threads / 800 conns)

- **43,308.99 req/s**
- **avg latency 18.40ms**
- **p99 21.92ms**

### What changed

Epoll replaced O(N) polling/checking with readiness-driven handling:

- no full scan across inactive sockets
- kernel reports only ready file descriptors
- wakeups align with actionable I/O

This is a **scalability shift**, not a micro-optimization. At high connection counts, readiness-based I/O cuts wasted work and improves both throughput and tail stability.

---

## 6) Chapter Four: Epoll + Threading

### Measurement (4 threads / 1200 conns)

- **57,650.29 req/s**
- **avg latency 20.50ms**
- **p99 30.39ms**

### What changed

This combines both scaling levers:

1. readiness-based I/O selection (epoll)
2. parallel execution capacity (threads)

So the server handles many open connections and many ready events simultaneously, pushing throughput close to the local ceiling for this design.

---

## 7) Perf Interpretation (Explain Once, Reuse)

Across stages, perf repeatedly emphasized syscall and networking paths (`writev`/write, `tcp_sendmsg`, `tcp_write_xmit`, IP output, softirq/NAPI).

The important progression is:

- early stage: queueing/concurrency limits dominate
- later stages: those limits are reduced
- high-throughput stages: kernel networking and syscall cost become primary

`writev` is a useful detail here: vectored writes improve batching and reduce per-response syscall overhead.

(Sections above reference this same shift instead of re-explaining it each time.)

---

## 8) NGINX Comparison (Optimization, Not Magic)

### Measurement (NGINX, 4 threads / 800 conns)

- **60,438.67 req/s**
- **avg latency 13.09ms**
- **p99 32.54ms**

NGINX’s advantage is engineering refinement in the same problem space: lower userspace overhead, tighter event-loop control flow, efficient buffering, and strong batching behavior around the kernel I/O path.

So the gap is not secret logic; it is optimized steady-state execution.

---

## 9) Compact Progression Summary

- **Blocking / backlog-max baseline** → **~5.0K–5.1K req/s** (queueing bottleneck)
- **Thread pool** → **21,489.98 req/s** (parallelism + less blocked handling)
- **Epoll** → **43,308.99 req/s** (readiness-driven I/O efficiency)
- **Epoll + threads** → **57,650.29 req/s** (combined scaling)
- **NGINX** → **60,438.67 req/s** (optimized steady state)

---

## 10) Final Takeaway

The bottleneck path became concrete:

**queueing saturation → concurrency limits → I/O readiness efficiency → kernel networking cost**.

That progression is the main outcome of this project, and the benchmark/perf data supports it end-to-end.
