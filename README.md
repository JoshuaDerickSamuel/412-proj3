# Project 3 - Code Documentation

## Project Overview

This project simulates a web request load-balancing system in C++. The simulation models incoming requests, a queue, and dynamically scaled web servers over clock cycles. It includes request generation, firewall blocking, queue dispatching, and optional switch-based routing between streaming and processing workloads.

## Main Design Description

At startup, the simulation creates an initial queue of requests. Each clock cycle then:

1. Processes active requests on each server.
2. Assigns queued requests to idle servers.
3. Generates new runtime requests.
4. Applies firewall checks for blocked IP ranges.
5. Scales servers up/down based on queue pressure thresholds.

The system logs events and prints summary metrics including generated/accepted/blocked/assigned/completed requests, peak queue size, and final server counts.

## Required Classes

- **Request** (`Request.h`): stores request payload information (`ipIn`, `ipOut`, `timeRequired`, `jobType`, and metadata).
- **WebServer** (`WebServer.h`): simulates one server processing one request at a time.
- **LoadBalancer** (`LoadBalancer.h`): manages request queue, server pool, dispatch logic, scaling rules, and statistics.

Additional support types include `RequestQueue` (custom FIFO queue) and `Switch` (optional routing by job type).

## Build and Run

```bash
make
./load_balancer_sim
```

## Documentation Output

Generate Doxygen HTML:

```bash
make docs
```

Generated site entry point:

- `docs/html/index.html`
