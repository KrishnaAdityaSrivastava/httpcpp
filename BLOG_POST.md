# From Sockets to Throughput Curves: A Story of Building (and Stressing) My HTTP Server

---

## 1) The Question That Started It

I started this project with a simple curiosity: if I remove frameworks and build the HTTP stack myself in C++, where does performance actually go under load?

So this server became an experiment, not a clone of NGINX. I wanted measured answers to four things:

- throughput (requests/second)
- latency (especially p90/p99)
- kernel vs user-space CPU cost
- scaling behavior as concurrent connections rise

The recurring question through every benchmark run was:

> **What becomes the bottleneck next, and why?**

---

## 2) The Starting Architecture

The implementation is intentionally layered:

1. **Socket setup + accept loop**
2. **HTTP I/O and parsing**
3. **Routing / handler dispatch**
4. **Response writing and optional static file serving**
5. **Concurrency strategy (thread pool and/or epoll mode)**

That structure made iteration easier. Each benchmark mode changed one major part of the runtime story, so I could see where time shifted.

---

## 3) Chapter One: `BACKLOG = MAX` and the Plateau

### Workload snapshots

With backlog maxed and blocking behavior in place, I ran 20-second tests (`wrk` with 4 threads) at increasing connection counts:

- **50 conns**: 5009.75 req/s, avg latency 9.87ms
- **100 conns**: 5087.10 req/s, avg latency 19.68ms
- **200 conns**: 4973.69 req/s, avg latency 40.40ms
- **400 conns**: 5137.53 req/s, avg latency 77.40ms
- **800 conns**: 5135.72 req/s, avg latency 154.08ms

### Why this output happened

This was the first big lesson:

- Throughput stayed near **~5K req/s**
- Latency kept stretching as connections increased

That is queueing saturation (M/M/1-like behavior): once service capacity is full, extra concurrency no longer creates extra completed work. It only increases waiting time.

So the result was consistent and expected: **same amount of work done per second, much longer time spent waiting per request**.

---

## 4) Chapter Two: Thread Pool Only (~21.5K req/s)

### Measurement

At 4 threads / 800 connections:

- **21,489.98 req/s**
- **avg latency 23.50ms**
- p99 **48.85ms**

### Why this output improved

Compared to the ~5K plateau, this suggests the runtime removed major serialization/idle points:

- more real worker parallelism
- less wall-clock stalling around per-connection flow
- better pipeline utilization of CPU + socket work

### What perf says here

The perf report is heavy in syscall/network paths:

- `entry_SYSCALL_64_after_hwframe`, `do_syscall_64`
- `write`, `ksys_write`, `vfs_write`
- `tcp_sendmsg`, `tcp_write_xmit`, `__tcp_transmit_skb`
- `ip_output`, `__dev_queue_xmit`, softirq/NAPI paths

Interpretation: bottlenecks moved away from obvious app-layer logic and toward **kernel-mediated I/O work**. That is usually a sign that the server is finally feeding the network stack effectively.

---

## 5) Chapter Three: Epoll Only (~43.3K req/s)

### Measurement

At 4 threads / 800 connections:

- **43,308.99 req/s**
- **avg latency 18.40ms**
- p99 **21.92ms**

### Why this output jumped

Epoll changed the shape of the runtime:

- no O(N) socket scanning
- kernel signals exactly which fds are ready
- less time blocked on inactive sockets
- wakeups map better to useful work

So at high connection counts, the server spends more cycles doing work and fewer cycles checking sockets that are not ready.

### What perf says here

Perf still concentrates on syscall and TCP stack symbols (`writev`, `do_writev`, `sock_write_iter`, `tcp_sendmsg`, `ip_output`, softirq/NAPI), with user-space launch/router still visible.

That indicates:

- app dispatch remains in play
- but most heavy cost has shifted into efficient send/receive kernel paths
- `writev` suggests helpful batching/vectored output behavior

The low spread between median and tail latencies matches that interpretation.

---

## 6) Chapter Four: Epoll + Threading (~57.7K req/s)

### Measurement

At 4 threads / 1200 connections:

- **57,650.29 req/s**
- **avg latency 20.50ms**
- p99 **30.39ms**

### Why this became the best mode

This combines the two strongest levers:

1. **Readiness-driven I/O (epoll)**
2. **Parallel execution capacity (threading)**

So the server can handle:

- many connected clients
- many simultaneous ready events

without falling back to O(N) checks or single-lane execution.

### What perf says here

Perf shows substantial weight in thread-pool worker frames plus syscall/TCP transmit paths. In practice that means workers are busy and the kernel/networking path is now the practical frontier.

---

## 7) NGINX Comparison: The Last Gap

### Measurement (NGINX @ 4 threads / 800 conns)

- **60,438.67 req/s**
- **avg latency 13.09ms**
- p99 **32.54ms**

### Why NGINX still leads

Your notes are accurate: most time is kernel-side, while NGINX user-space cost is relatively small.

This suggests NGINX is doing what mature event-driven servers do best:

- minimal userspace overhead per request
- efficient buffering and control flow
- fast handoff to the same Linux networking primitives

So the gap is less about “secret logic,” more about years of runtime-path refinement.

---

## 8) The Perf Story Across All Modes

Across all runs, perf repeatedly highlights:

- syscall entry/exit
- socket read/write paths
- TCP transmit/receive internals
- IP output/queueing
- softirq/NAPI packet processing

Taken together, the progression is clear:

1. Early architecture hit queueing saturation.
2. Better concurrency (thread pool, epoll) increased useful throughput.
3. At higher throughput, the dominant cost migrated into kernel networking work.

That is exactly the evolution you want to see in a server that is maturing.

---

## 9) Why Epoll Was Non-Negotiable

The practical issue was simple: I needed threads to stop idling on inactive clients.

At high connection counts, checking every socket each cycle is O(N) and wastes CPU. Epoll flips that model: the kernel reports only ready sockets.

So epoll here is not a micro-optimization; it is the mechanism that makes high-connection scaling feasible.

---

## 10) Final Takeaway

If I had to summarize this project in one line:

> It started as “build an HTTP server,” and became “learn how bottlenecks move as architecture improves.”

The strongest lesson is that performance gains came mainly from architecture changes, then from implementation details. The numbers and perf traces now tell one consistent story from first prototype to near-NGINX territory.
