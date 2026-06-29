```
        ███╗   ███╗██╗  ██╗     ██████╗ ███████╗
        ████╗ ████║██║ ██╔╝    ██╔═══██╗██╔════╝
        ██╔████╔██║█████╔╝     ██║   ██║███████╗
        ██║╚██╔╝██║██╔═██╗     ██║   ██║╚════██║
        ██║ ╚═╝ ██║██║  ██╗    ╚██████╔╝███████║
        ╚═╝     ╚═╝╚═╝  ╚═╝     ╚═════╝ ╚══════╝
```

<div align="center">

**A local-first AI operating system with fluid intelligence, knowledge graphs, and self-evolution.**

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)]()
[![Lines of Code](https://img.shields.io/badge/lines-58%2C000%2B-brightgreen.svg)]()
[![Knowledge Facts](https://img.shields.io/badge/facts-9%2C000%2B-orange.svg)]()
[![License](https://img.shields.io/badge/license-All%20Rights%20Reserved-red.svg)]()

</div>

---

## Features

| Category | Capabilities |
|----------|-------------|
| **Intelligence** | Knowledge graph (9,000+ facts), multi-hop reasoning, semantic search, pattern matching |
| **Generation** | Prometheus Engine, Crystal Network, Consciousness Engine, Idea Engine |
| **Learning** | Learns from corrections, self-improves nightly, auto-generates reasoning rules |
| **Internet** | DuckDuckGo/Wikipedia search with citations, live weather, time, news |
| **Code** | Executes Python, Bash, C++ in sandbox with timeout protection |
| **Math/Science** | Quadratic solver, calculus, unit conversions, physics formulas |
| **Communication** | Beautiful terminal REPL, Telegram bot (24/7), remote PC control |
| **Files** | Read text/code/PDF, image metadata extraction, file summarization |
| **Security** | AES-256 encryption, token auth, rate limiting, input sanitization |
| **LLM** | Optional local LLM (Ollama/llama.cpp) with graceful fallback |
| **Self-Evolution** | Knowledge growth, auto-update from GitHub, self-programming |

---

## Quick Install

```bash
git clone https://github.com/mohd2456/MK_os-2.git
cd MK_os-2
./install.sh
```

Then from anywhere:

```bash
mk
```

---

## Project Structure

```
MK_OS/
├── ai_core/                 # Central intelligence layer
│   ├── prometheus/          #   Prometheus Engine (fluid generation)
│   ├── cxn/                 #   Crystal Network (resonance-based responses)
│   ├── mce/                 #   Consciousness Engine (thought assembly)
│   ├── hre/                 #   Hybrid Reasoning Engine + knowledge files
│   ├── smart_router.cpp     #   Query classification and dispatch
│   ├── conversation_mode.cpp#   602 conversation templates
│   ├── idea_engine.cpp      #   Creative concept bridging
│   ├── math_solver.cpp      #   Advanced math/science solver
│   ├── neural_net.cpp       #   Lightweight neural network
│   └── self_improver.cpp    #   Nightly self-improvement
├── mk_brain/                # Learning, memory, personality, embeddings
├── system/                  # Entry point, daemon, shell, boot, scheduler
├── network/                 # HTTP, search, real-time APIs, web agent
├── plugins/                 # Telegram, plugin API, mesh network, monitors
├── security/                # AES-256, auth tokens, rate limiting, sanitization
├── tools/                   # Code runner, file reader, image analyzer, knowledge tools
├── llm/                     # Local LLM integration (Ollama / llama.cpp)
├── Kernel/                  # Low-level OS kernel for bare-metal deployment
├── bootloader/              # Boot image builder for hardware/VM targets
├── tests/                   # Automated test suite
└── Makefile                 # Build system (clang++, C++17)
```

---

## Commands Reference

| Command | Description |
|---------|-------------|
| `/help` | Show all available commands |
| `/ask <query>` | Knowledge graph lookup |
| `/think <query>` | Deep multi-step reasoning |
| `/search <query>` | Internet search with citations |
| `/learn source\|rel\|target` | Teach MK a new fact |
| `/math <expr>` | Math/science solver |
| `/run <lang> <code>` | Execute code (Python/Bash/C++) |
| `/read <path>` | Read and summarize a file |
| `/image <path>` | Analyze image metadata |
| `/weather` | Live weather report |
| `/time` | Current time and timezone |
| `/news` | Latest tech headlines |
| `/schedule` | Manage scheduled tasks |
| `/todo` | To-do list management |
| `/speak` | Text-to-speech output |
| `/status` | System diagnostics |
| `/grow` | Research knowledge gaps |
| `/update` | Check for updates |
| `/quit` | Save state and exit |

---

## The Generation Chain

MK OS uses a unique 4-engine cascade for generating responses:

### Prometheus Engine
The master engine. Models language as **fluid dynamics** -- words are droplets with
momentum, surface tension, and flow. A "Birth Chamber" crystallizes droplets into
coherent sentences. Handles both conversation and code generation through a unified
precision spectrum.

### Crystal Network (CXN)
Stores knowledge as **crystals** with resonance frequencies. Input triggers resonate
with stored crystals; the strongest resonance wins. A Crystallizer refines raw
matches into polished responses. Falls back from Prometheus when fluid generation
needs more structure.

### Consciousness Engine (MCE)
Processes thought in 5 layers: Thought Atoms (decompose input), Chain Reactor
(trigger associated memories), Thought Assembler (build response blueprint),
Word Weaver (generate from word graph), and Echo Memory (adapt to user speech
patterns). Provides the deepest level of response assembly.

### Idea Engine
Creative divergent thinking. Randomly bridges unrelated concepts from the knowledge
graph to generate novel ideas, analogies, and unexpected connections.

**Fallback chain:** Prometheus -> CXN -> MCE -> Templates

---

## Architecture

```
                    ┌──────────────────────────┐
                    │       User Input          │
                    └─────────────┬────────────┘
                                  │
                    ┌─────────────▼────────────┐
                    │    Input Preprocessor     │
                    │  (normalize, detect lang) │
                    └─────────────┬────────────┘
                                  │
                    ┌─────────────▼────────────┐
                    │      Smart Router         │
                    │  (classify query type)    │
                    └──┬────┬────┬────┬────┬───┘
                       │    │    │    │    │
          ┌────────────┘    │    │    │    └────────────┐
          ▼                 ▼    │    ▼                  ▼
    ┌───────────┐   ┌──────────┐│ ┌──────────┐   ┌──────────┐
    │ Knowledge │   │ Internet ││ │   Math   │   │   Code   │
    │   Graph   │   │  Search  ││ │  Solver  │   │  Runner  │
    │  9,000+   │   │ DuckDuck ││ │ Calculus │   │ Py/Bash/ │
    │   facts   │   │  + Wiki  ││ │ Physics  │   │   C++    │
    └───────────┘   └──────────┘│ └──────────┘   └──────────┘
                                │
                    ┌───────────▼───────────┐
                    │   Generation Chain    │
                    │                       │
                    │  Prometheus Engine    │
                    │       ↓ fallback      │
                    │  Crystal Network     │
                    │       ↓ fallback      │
                    │  Consciousness Eng.  │
                    │       ↓ fallback      │
                    │  Template System     │
                    └───────────┬───────────┘
                                │
                    ┌───────────▼───────────┐
                    │     Output Layer      │
                    │  REPL / Telegram /    │
                    │  Remote / Voice       │
                    └──────────────────────┘
```

---

## The Four Dimensions

MK OS intelligence operates across four dimensions:

| Dimension | Engine | Purpose |
|-----------|--------|---------|
| **Meaning** | Knowledge Graph + HRE | Understanding what things are and how they relate |
| **Structure** | CXN + Prometheus | Organizing thoughts into coherent language |
| **Self** | MCE + Echo Memory | Adapting to the user's communication style |
| **Evolution** | Self-Improver + Knowledge Grower | Getting smarter autonomously over time |

---

## Stats

| Metric | Value |
|--------|-------|
| Language | C++17 (clang++) |
| Total Lines of Code | 58,000+ |
| Knowledge Facts | 9,000+ (in .mk format) |
| Conversation Templates | 602 |
| Code Fragments | 145 |
| Test Assertions | 160+ |
| Build Time | ~5 seconds |
| Binary Size | ~1.5 MB |
| Total Project Size | ~53 MB |

---

## Requirements

- Linux (Arch, Ubuntu, Fedora) or macOS
- clang++ or g++ with C++17 support
- libcurl and libsqlite3
- ~100 MB disk space
- 512 MB+ RAM (8 GB+ recommended for local LLM)

---

## Build from Source

```bash
cd MK_OS
make clean && make        # Build the binary
make test                 # Run all tests
./mk_os                   # Launch MK OS
```

---

## Telegram Setup (Optional)

1. Message [@BotFather](https://t.me/BotFather) on Telegram
2. Create a new bot and copy the token
3. Set it: `echo 'MK_TELEGRAM_TOKEN="your_token"' >> ~/.mk_os/config`
4. Restart MK -- it auto-connects and runs 24/7

---

## Update

```bash
cd MK_os-2 && git pull && ./install.sh
```

Or from inside MK: `/update`

---

## License

Personal project by Mohammed. All rights reserved.
