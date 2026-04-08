# From Sockets to Server: What I Learned Building My Own Web Server

---

## 1. Introduction

Most of us interact with web servers every day, but usually through frameworks and platforms that hide the hard parts. You ship an API, deploy it, and move on.

I wanted to understand what happens before all that convenience.

So I built a small HTTP server in C++ from scratch—starting with POSIX sockets, then layering in request parsing, routing, static file serving, and concurrency. No external web framework. Just syscalls, strings, and a lot of edge cases.

At first glance, a web server sounds simple: accept a connection, read bytes, send bytes back. In practice, each of those steps has sharp corners.

This post is a walkthrough of that journey—the architecture choices, the trade-offs, what broke first, and what changed once I tested it under load.

---

## 2. Motivation

There was no business requirement for this project.

This was about replacing assumptions with understanding.

I’d used frameworks long enough to start asking questions they rarely force you to think about:

- How do you know when an HTTP request is fully received?
- What happens if headers arrive in fragments?
- Why does a simple server feel fine with 10 users but collapse with 500?
- How do mature servers like Nginx stay fast under sustained concurrency?

Building a server from scratch turned those questions from abstract ideas into implementation constraints.

My target wasn’t “production-ready Nginx competitor.” It was:

- learn low-level networking mechanics
- implement a clean request → route → response pipeline
- understand where throughput and latency are actually lost

---

## 3. High-Level Architecture

Before writing code, I defined a strict request pipeline:

```text
Client → Socket Accept → HTTP Read/Parse → Route Dispatch → Response Write → Client
```

In code, that maps to clear components:

- **Socket layer**: TCP setup (`socket`, `bind`, `listen`, `accept`)
- **HTTP I/O layer**: read from socket, parse request line/body, write response
- **Router**: match method/path, including dynamic path params
- **Server runtime**: connection loop + thread pool dispatch

This separation ended up being the biggest force multiplier in the project. When parsing failed, I didn’t have to wonder whether it was routing logic. When concurrency issues showed up, I could focus on server runtime and I/O behavior.

---

## 4. Networking Layer (POSIX Sockets)

Everything starts with classic socket setup:

```cpp
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
bind(server_fd, ...);
listen(server_fd, backlog);
```

Then the server blocks on:

```cpp
int client_fd = accept(server_fd, ...);
```

Each accepted socket becomes a unit of work for request handling.

### Why `SO_REUSEADDR` matters

Without `SO_REUSEADDR`, restarting during development often fails with “address already in use” because the old socket can remain in `TIME_WAIT`. This is one of those tiny lines of code that saves a lot of frustration.

### The lifecycle (the real one)

On paper:

1. accept
2. read request
3. process
4. write response
5. close

In reality:

- reads are partial
- clients disconnect mid-write
- malformed requests appear constantly in stress tests
- “close or keep-alive?” changes control flow everywhere

---

## 5. Request Handling Pipeline

This was where the project became less “toy” and more “protocol engineering.”

### Partial reads are the default, not the exception

A single `read()` does **not** guarantee a complete HTTP request. Data can arrive in chunks for many reasons (network buffering, sender behavior, packet boundaries).

So the parser accumulates bytes into a buffer and scans for the header delimiter:

```text
\r\n\r\n
```

Until that delimiter appears, headers are incomplete.

### Determining body length safely

After headers are complete, `Content-Length` determines how much body data to read.

A key implementation decision here was defensive parsing:

- cap header size (e.g., 16 KB)
- cap body size (e.g., 1 MB)
- parse `Content-Length` with integer conversion checks
- reject malformed or unsupported request lines early

Those checks improved stability more than any performance tweak.

### Parsing into structured request data

From raw bytes, the server extracts:

- method (`GET`, `POST`, etc.)
- path (`/`, `/data/123`, ...)
- version (`HTTP/1.1`, `HTTP/1.0`)
- body (if present)

Once you do this yourself, you realize HTTP parsing is mostly careful boundary management.

---

## 6. Routing System

After parsing, the next question is: “Which handler should run?”

### Static routes

The straightforward case:

- `GET /`
- `POST /submit`

### Dynamic routes

I added support for patterns like:

```text
/data/:id
```

Matching is done by splitting route/request paths into segments and binding parameter values where route segments start with `:`.

So `/data/sample.html` can map `id = sample.html` and pass that into handler logic.

### Error behavior

Routing also forced explicit failure handling:

- no route match → default empty/404-style response path
- handler throws → catch and return `500 Internal Server Error`

This is one of the first places “framework ergonomics” become visible—you end up rebuilding conventions one decision at a time.

---

## 7. Response & Static File Serving

### Response assembly

For regular responses, the server builds status line + headers + body:

```text
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: ...
Connection: keep-alive

<body...>
```

One practical optimization here is using `writev()` to send headers and body in a single vectored write path.

### Static files with `sendfile`

For file responses, I used `sendfile()`:

```cpp
sendfile(client_socket, file_fd, &offset, file_size - offset);
```

This avoids extra user-space copies and noticeably reduces overhead for larger files.

### Security guardrails

Serving files safely required path controls:

- decode URL path
- resolve against a fixed `public/` base directory
- canonicalize and verify requested file stays under that base

That prevents directory traversal attempts such as `../../etc/passwd` from escaping the static root.

---

## 8. Concurrency Model

The server accepts connections on the main thread and pushes work into a fixed-size thread pool.

### Why a thread pool

It’s a good middle ground:

- simpler than a full event-loop architecture
- avoids unbounded thread creation
- easy to reason about ownership and task flow

### Trade-offs

**Pros**

- predictable worker count
- straightforward implementation with mutex + condition variable
- good enough for moderate concurrency

**Cons**

- still uses blocking I/O per worker
- pool saturation introduces queueing latency
- not ideal for very high connection counts

This is the classic systems lesson: architecture choices dominate before micro-optimizations even matter.

---

## 9. Benchmarking & Performance

This was the most educational part of the project.

I tested with tools like `wrk` and `ab` using varying thread/connection combinations and request patterns (small text responses vs static file responses).

### What I measured

I focused on:

- **Requests/sec (throughput)**
- **p50/p95/p99 latency**
- **behavior under increasing concurrency**
- **error rate under stress**

### How to interpret results (beyond “bigger number wins”)

A useful way to read benchmark output is by regime:

1. **Low concurrency**: server feels fast; little contention
2. **Moderate concurrency**: throughput rises, latency starts widening
3. **Saturation**: throughput plateaus; tail latency spikes

That saturation point is the real signal. It tells you where your architecture stops scaling efficiently.

### Why mature servers still outperform this design

Compared to event-driven servers (Nginx-style), my server pays higher per-request overhead from:

- blocking reads/writes per worker
- context switching and queue contention
- repeated string parsing/allocations

Even with `sendfile` helping the static path, the concurrency model remains the dominant limiter at higher loads.

### Most important insight from benchmarking

Performance tuning was less about “faster parsing tricks” and more about removing structural bottlenecks.

In other words:

- **first fix architecture**, then optimize implementation details

That single idea changed how I approach backend performance work.

---

## 10. Challenges & Learnings

### Partial reads broke naive assumptions

Expecting complete requests from single `read()` calls caused early parsing bugs.

### Broken pipes happen in normal operation

Clients can disconnect before a write finishes. Ignoring `SIGPIPE` and handling short/failed writes made behavior much more robust.

### Real HTTP traffic is messy

Malformed request lines, missing/invalid lengths, and odd edge cases show up quickly once you load test.

### Concurrency bugs are often timing-sensitive

Issues are harder to reproduce consistently, which makes observability and isolated component boundaries crucial.

---

## 11. Security & Edge Cases

Even for a learning project, basic hardening matters.

I added protections around:

- **directory traversal** via canonical path checks
- **oversized input** using header/body size limits
- **request validation** (method/version and basic syntax checks)

The broader lesson: correctness and safety are part of server performance too—because crashes, stalls, and undefined behavior are the worst latency of all.

---

## 12. Future Improvements

If I were taking this further, I’d prioritize these upgrades:

1. **Event-driven I/O (`epoll`)** to reduce blocking-worker overhead
2. **Better HTTP compliance** (header parsing, chunked transfer encoding)
3. **Connection management** improvements (keep-alive limits, idle eviction)
4. **Response/file caching** to cut repeated disk work
5. **Metrics + observability** (request timings, queue depth, error counters)

That roadmap moves it from “educational server” toward a real systems platform.

---

## 13. Conclusion

Building a web server from scratch gave me a more concrete mental model of backend systems than any framework tutorial could.

The biggest takeaways:

- networking APIs are simple to call, hard to use correctly under load
- throughput and latency are mostly architecture outcomes
- reliability comes from defensive parsing and failure handling
- abstractions are powerful—but understanding what they abstract is even more powerful

I started this project to satisfy curiosity. I finished with a clearer engineering instinct for performance, protocol boundaries, and systems trade-offs.

---

## 14. References / Links

- GitHub Repository: *(insert repo link)*
- Beej’s Guide to Network Programming
- Nginx architecture docs
- RFC 7230 / RFC 9112 (HTTP/1.1 messaging)

---

If you want to make this post more memorable, the best next enhancement is to add **benchmark graphs** (throughput and p95 latency vs concurrent connections). Numbers are useful; curves tell the story.
