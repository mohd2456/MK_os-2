# MK OS — Hybrid AI Operating System

> A complete AI operating system built from scratch in C++. No cloud. No APIs. Fully local intelligence.

Built by **mohd2456**

---

## What is MK OS?

MK OS is a custom-built operating system designed to run as a **24/7 always-on AI assistant** on local hardware (originally a 2010 MacBook Pro, now also runs on iPhone via UTM SE). It does NOT depend on cloud services like ChatGPT or Claude. Instead, it builds intelligence from:

- A custom **kernel** that manages hardware resources
- A from-scratch **neural network inference engine** with INT8 quantization
- A **knowledge graph + logical reasoning engine** (the Hybrid Reasoning Engine)
- **Internet search + fact verification** for accuracy
- A **self-improvement loop** that tracks knowledge gaps and learns over time

### Core Philosophy

| Principle | Description |
|-----------|-------------|
| **Never Hallucinate** | Uses fact verification + honesty engine. Admits when it doesn't know. |
| **No Cloud Dependency** | Everything runs locally. Your data stays on your device. |
| **Thermal Safety** | 24/7 operation with aggressive thermal management. Won't overheat your machine. |
| **Self-Improving** | Tracks gaps in knowledge, generates study plans, auto-researches topics. |
| **Memory Efficient** | Slab allocators, demand-paged weights, INT8 quantization for weak hardware. |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     USER INTERACTION LAYER                        │
│  system/shell.cpp  │  system/mk_entry.cpp  │  plugins/telegram   │
├─────────────────────────────────────────────────────────────────┤
│                     AI REASONING LAYER                            │
│  ai_core/hre/  │  ai_core/smart_router  │  ai_core/confidence   │
│  deep_reasoner │  code_intelligence     │  debate_engine         │
├─────────────────────────────────────────────────────────────────┤
│                     KNOWLEDGE & MEMORY LAYER                     │
│  mk_brain/  │  knowledge/  │  ai_core/persistent_memory         │
│  embeddings │  vector_search │  fact_verifier │ freshness_mgr    │
├─────────────────────────────────────────────────────────────────┤
│                     NEURAL INFERENCE LAYER                        │
│  ai_core/neural_net  │  ai_core/tokenizer  │  ai_core/hal       │
│  ai_core/weight_pager │  ai_core/nexus (slab allocator)         │
├─────────────────────────────────────────────────────────────────┤
│                     NETWORK & TOOLS LAYER                        │
│  network/search_engine  │  network/autonomous_agent              │
│  network/realtime_apis  │  tools/telegram_bridge                 │
├─────────────────────────────────────────────────────────────────┤
│                     KERNEL LAYER                                  │
│  Kernel/core/  │  Kernel/driver/  │  Kernel/sched/              │
│  Kernel/fs/    │  Kernel/ipc/     │  Kernel/power/              │
│  Kernel/mm/    │  Kernel/net/     │  Kernel/security/           │
├─────────────────────────────────────────────────────────────────┤
│                     HARDWARE                                     │
│  CPU  │  RAM  │  SSD  │  WiFi  │  GPU                          │
└─────────────────────────────────────────────────────────────────┘
```

---

## What MK OS Can Do

### AI & Reasoning
- **Answer questions** using knowledge graph + logical inference (not statistical guessing)
- **Multi-step reasoning** — breaks complex problems into sub-questions and solves each
- **Chain-of-thought** — shows its reasoning process step by step
- **Analogy engine** — finds structural similarities between concepts
- **Causal reasoning** — "If X then Y" predictions with probability
- **Debate engine** — when sources conflict, scores both sides and declares a winner
- **Never guesses** — admits uncertainty honestly when confidence is low

### Knowledge & Learning
- **Knowledge graph** — stores facts as nodes and relationships (subject → relation → object)
- **Instant learning** — teach it a fact and it remembers forever
- **Fact verification** — cross-references claims against multiple internet sources
- **Freshness tracking** — knows when facts might be outdated (politics=7 days, science=365 days)
- **Self-improvement** — logs every question it can't answer, generates research plans
- **Persistent memory** — remembers conversations and user preferences across sessions

### Code & Programming
- **Code generation** — generates Python, JavaScript, C++ from goal descriptions
- **Project planning** — breaks "build me a web scraper" into files and components
- **Bug diagnosis** — analyzes error messages and suggests fixes
- **Code compilation** — writes, compiles, and runs C++ code autonomously
- **Template library** — thousands of indexed code patterns for instant assembly

### Neural Network (From Scratch)
- **Transformer architecture** — self-attention, feed-forward, layer normalization
- **BPE Tokenizer** — byte-pair encoding built from scratch
- **INT8 Quantization** — runs on weak hardware by quantizing weights to 8-bit integers
- **Demand-paged weights** — streams model layers from SSD, only keeps active ones in RAM
- **SIMD acceleration** — SSE2/NEON vectorized math operations

### System & Hardware
- **Real hardware monitoring** — CPU temp, RAM usage, battery, disk, network (actual system values)
- **Thermal governor** — throttles workload to prevent overheating during 24/7 operation
- **Task automation** — multi-step tasks with safety checks (blocks dangerous commands)
- **Process management** — priority scheduler, process table, zombie reaping
- **Virtual filesystem** — journaled filesystem with search, permissions, mount points
- **Network stack** — DNS cache, firewall, connection pooling, bandwidth monitoring

### Communication
- **Telegram bot** — control MK OS from your phone via Telegram
- **Web monitor** — browser dashboard for system status
- **SSH access** — remote terminal access
- **Mesh networking** — multiple MK instances can communicate

---

## File Structure

### `Kernel/` — The Operating System Core

| File | What It Does |
|------|-------------|
| `kernel_main.cpp` | Boot sequence (9 phases) + interactive command shell |
| `core/process.cpp` | Process table — create, kill, sleep, wake processes with PID tracking |
| `core/syscalls.cpp` | System call dispatch table (LOG, HALT, ALLOC, SPAWN, KILL, etc.) |
| `core/panic.cpp` | Kernel panic handler with crash dump and halt |
| `sched/scheduler_core.cpp` | Priority queue scheduler (REALTIME → HIGH → NORMAL → LOW → IDLE) |
| `mm/pager.cpp` | Virtual memory page table with pinned pages and access counting |
| `ipc/ring_buffer.cpp` | Lock-free ring buffer for kernel ↔ AI communication with CRC32 checksums |
| `fs/filesystem.cpp` | Full virtual filesystem with journaling, directories, mount points, search |
| `net/network_stack.cpp` | TCP/UDP sockets, DNS cache, firewall rules, connection pooling |
| `power/thermal_governor.cpp` | 24/7 thermal protection — 5 zones (COOL → CRITICAL) with predictive throttling |
| `security/permissions.cpp` | Users, groups, capabilities, audit log, privilege escalation with timeouts |
| `driver/cpu_driver.cpp` | Real CPU detection — model, cores, frequency, cache, temperature |
| `driver/memory_driver.cpp` | Real RAM stats from system APIs |
| `driver/gpu_driver.cpp` | GPU detection and monitoring |
| `driver/battery_driver.cpp` | Battery level, health, cycle count, power draw |
| `driver/disk_driver.cpp` | Block device with write cache and flush |
| `driver/network_driver.cpp` | Network interface scanning, IP/MAC detection, traffic stats |
| `driver/display_driver.cpp` | Display resolution, brightness control |
| `driver/audio_driver.cpp` | Audio device detection and volume control |
| `driver/usb_driver.cpp` | USB device enumeration |
| `driver/timer_driver.cpp` | System timer with registered events |
| `driver/console_driver.cpp` | Console I/O with log levels |
| `driver/mk_echo.c` | Actual Linux kernel module (character device for testing) |

### `ai_core/` — The Intelligence Engine

| File | What It Does |
|------|-------------|
| `main.cpp` | Entry point — wires all AI modules together and runs demo |
| `neural_net.cpp` | Full transformer neural network from scratch (attention, GELU, softmax, INT8) |
| `tokenizer.cpp` | Byte-Pair Encoding tokenizer — trains, encodes, decodes text |
| `hal.cpp` | Hardware Abstraction Layer — runtime SIMD detection, SSE2/NEON math dispatch |
| `weight_pager.cpp` | Demand-paged weight streaming from SSD with LRU eviction and prefetch |
| `nexus.cpp` | 64MB preallocated slab allocator + heuristic query bypass |
| `knowledge_integrator.cpp` | Orchestrates: search → verify → store with freshness tracking |
| `fact_verifier.cpp` | Cross-references facts against multiple sources, detects contradictions |
| `freshness_manager.cpp` | Tracks fact expiry by category (Science=365d, Tech=90d, Sports=1d) |
| `honesty_engine.cpp` | Never guesses — admits uncertainty with suggestions for alternatives |
| `debate_engine.cpp` | Conflict resolution — scores both sides and declares winner |
| `cited_response.cpp` | All answers include source URLs, confidence bars, timestamps |
| `confidence.cpp` | Entropy-based confidence scoring — decides: answer / escalate / mark unknown |
| `smart_router.cpp` | Routes queries to optimal path: INSTANT / GRAPH / SEARCH / REASON / GENERATE |
| `self_improver.cpp` | Tracks knowledge gaps, categorizes by topic, generates improvement plans |
| `persistent_memory.cpp` | Cross-session memory — interactions, preferences, Q&A history |
| `task_automation.cpp` | Multi-step task executor with dangerous command blocking |
| `reasoning.cpp` | Mathematical expression evaluator with operator precedence |
| `language.cpp` | Response personality router (casual / technical / creative) |
| `coding.cpp` | Write code to file + compile with g++ + run |
| `writing.cpp` | Template-based essay/content generator |
| `execution.cpp` | System command execution |
| `feedback.cpp` | Compiler error diagnosis |
| `memory.cpp` | Key-value short-term memory store |

### `ai_core/hre/` — Hybrid Reasoning Engine (The Brain)

| File | What It Does |
|------|-------------|
| `hre_main.cpp` | Main REPL loop — parses input → routes intent → searches graph → reasons → responds |
| `pattern_graph.cpp` | Knowledge graph — nodes (concepts) connected by edges (relations), multi-hop queries |
| `reasoning_chains.cpp` | Logical inference rules: "If A is_a B and B can X, then A can X" |
| `query_parser.cpp` | Natural language understanding — classifies intent, extracts subject/relation/object |
| `composer.cpp` | Builds natural language responses from templates + facts + personality |
| `deep_reasoner.cpp` | Chain-of-thought, analogy engine, causal reasoning, planning engine |
| `code_intelligence.cpp` | Code template library, project planner, multi-language generator, bug diagnosis |
| `code_synthesizer.cpp` | Goal-to-code synthesis — understands programming concepts, outputs Python/JS/C++ |
| `adaptive_depth.cpp` | Thermal-aware reasoning depth — adjusts thinking complexity based on CPU temperature |
| `knowledge_gate.cpp` | Quality gating for knowledge entering the graph |
| `knowledge_compiler.cpp` | Compiles raw text into structured knowledge graph entries |
| `meta_cognition.cpp` | MK thinks about its own thinking (confidence calibration) |
| `semantic_match.cpp` | Fuzzy/semantic matching between queries and graph nodes |
| `generative_composer.cpp` | Advanced response generation with paragraph structure |
| `speed_engine.cpp` | Fast-path optimizations for common queries |
| `context_embedding.cpp` | Maintains conversation context window |
| `compositional_logic.cpp` | Combines multiple facts into complex answers |
| `conversation_engine.cpp` | Multi-turn dialogue management |
| `self_programmer.cpp` | MK can write code to extend its own capabilities |

### `system/` — OS Services

| File | What It Does |
|------|-------------|
| `mk_entry.cpp` | True system entry point — boots kernel, initializes everything, starts AI |
| `boot.cpp` | Boot sequence orchestration |
| `shell.cpp` | Interactive command shell with history |
| `daemon.cpp` | Background service management |
| `service_manager.cpp` | Start/stop/monitor system services |
| `auto_maintenance.cpp` | Automatic disk cleanup, log rotation, health checks |
| `diagnostics.cpp` | System health diagnostics and reporting |
| `remote_access.cpp` | Remote control interface |
| `logger.cpp` | Centralized logging system |
| `error.cpp` | Error handling and recovery |
| `api.cpp` | External API interface |
| `main_system.cpp` | System module coordinator |

### `network/` — Internet & Communication

| File | What It Does |
|------|-------------|
| `search_engine.cpp` | Internet search via multiple sources |
| `autonomous_agent.cpp` | Self-directed research — MK autonomously browses and learns |
| `realtime_apis.cpp` | Weather, time, exchange rates, news APIs |
| `http.cpp` | HTTP client implementation |
| `parser.cpp` | HTML/JSON response parsing |
| `update.cpp` | Self-update mechanism |

### `plugins/` — Extensions

| File | What It Does |
|------|-------------|
| `telegram.cpp` | Telegram bot — control MK from your phone |
| `web_monitor.cpp` | Web dashboard for monitoring |
| `thermal_monitor.cpp` | Temperature monitoring plugin |
| `mesh_network.cpp` | Device mesh networking (multiple MK instances) |
| `self_correction.cpp` | Auto-corrects mistakes over time |
| `pc_helper.cpp` | PC maintenance helper |
| `custom.cpp` | Custom plugin loader |
| `manager.cpp` | Plugin lifecycle management |

### `mk_brain/` — Long-term Intelligence

| Directory | What It Does |
|-----------|-------------|
| `personality/` | Response style, personality traits, tone control |
| `learning/` | Learning engine — integrates new knowledge over time |
| `memory/` | Long-term brain memory storage |
| `embeddings/` | Vector embeddings for semantic similarity |
| `vector_search/` | Approximate nearest neighbor search |
| `reasoning/` | High-level reasoning coordination |
| `fact_extractor/` | Extracts structured facts from raw text |
| `cashe/` | Cache management for frequently accessed knowledge |

### `tools/` — Offline Utilities

| File | What It Does |
|------|-------------|
| `knowledge_ingest.cpp` | Bulk-imports knowledge from files into the graph |
| `knowledge_sync.cpp` | Syncs knowledge between devices/instances |
| `llm_eater.cpp` | Converts LLM outputs into structured graph knowledge |
| `telegram_bridge.cpp` | Full Telegram integration bridge |

### `datasets/` — Data Management

| File | What It Does |
|------|-------------|
| `loader.cpp` | Load datasets from TXT files |
| `parser.cpp` | Parse CSV and JSON data files |
| `db.cpp` | SQLite database interface |
| `compressor.cpp` | Data compression/decompression |

### `memory_mgmt/` — Memory Management

| File | What It Does |
|------|-------------|
| `allocator.cpp` | Memory allocator |
| `cow_snapshots.cpp` | Copy-on-write memory snapshots |
| `garbage.cpp` | Garbage collection |
| `swap.cpp` | Swap space management |

### `io_routing/` — I/O System

| File | What It Does |
|------|-------------|
| `console.cpp` | Console I/O routing |
| `device.cpp` | Device I/O |
| `file.cpp` | File I/O operations |
| `network_io.cpp` | Network I/O routing |

### `security/` — Security Layer

| File | What It Does |
|------|-------------|
| `encrypted_shards.cpp` | Encrypted data storage with sharding |
| `main_security.cpp` | Security module coordinator |

### `scheduler/` — Task Scheduling

| File | What It Does |
|------|-------------|
| `token_scheduler.cpp` | Token-based task scheduling for AI inference |
| `cpu.cpp` | CPU scheduling |
| `queue.cpp` | Task queue management |
| `task.cpp` | Task definitions |

### `knowledge/` — Knowledge Loading

| File | What It Does |
|------|-------------|
| `knowledge_loader.cpp` | Loads knowledge files into the system |

### `bootloader/` — Boot & Deployment

| File | What It Does |
|------|-------------|
| `build_image.sh` | Builds bootable disk image for UTM SE / QEMU |
| `Makefile.arm64` | Cross-compilation for ARM64 (iPhone) and x86_64 |
| `mk_boot.cfg` | Bootloader configuration (extlinux) |
| `mk_init.sh` | VM initialization script (runs on boot) |
| `mk_service` | OpenRC service file (auto-starts MK OS) |
| `network_setup.sh` | Network configuration for VM |
| `alpine_packages.txt` | Required Alpine Linux packages |
| `utm_guide.md` | Complete guide for running on iPhone via UTM SE |
| `init.mk` | Legacy init script for direct hardware boot |

---

## How It Works (Query → Answer Flow)

```
1. User types a query
2. smart_router.cpp classifies: INSTANT / GRAPH / SEARCH / REASON / GENERATE
3. adaptive_depth.cpp checks thermal state, caps reasoning depth if hot
4. If GRAPH: pattern_graph.cpp looks up facts directly
5. If REASON: deep_reasoner.cpp decomposes into sub-questions + causal logic
6. If SEARCH: search_engine.cpp queries internet → fact_verifier.cpp cross-checks
7. confidence.cpp evaluates output quality
8. If confidence too low → honesty_engine.cpp admits uncertainty
9. composer.cpp builds natural language response with citations
10. persistent_memory.cpp logs the interaction
11. self_improver.cpp checks: was this a knowledge gap? Should MK learn more?
```

---

## Running MK OS

### On iPhone (UTM SE)
```bash
# Use the Arch Linux VM with MK OS installed
# Login: root
# Start MK: /opt/mk_os/mk_os
```

### On Mac/Linux (compile from source)
```bash
cd MK_OS/
make all
./mk_os
```

### Cross-compile for ARM64
```bash
make -f bootloader/Makefile.arm64 static
make -f bootloader/Makefile.arm64 install
```

---

## Stats

- **100+ source files**
- **25,000+ lines of C++ code**
- **Zero external dependencies** (pure C++ STL)
- **Covers:** kernel, drivers, filesystem, networking, security, neural networks, tokenizers, knowledge graphs, NLU, reasoning, code generation, self-improvement, thermal management, plugins
- **Runs on:** macOS, Linux, iPhone (via UTM SE), any POSIX system

---

## License

Built by mohd2456. All rights reserved.
