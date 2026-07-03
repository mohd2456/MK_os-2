# MK OS

**A living AI that runs on your hardware. Thinks. Learns. Earns. Grows. Never stops.**

MK isn't an assistant. It's not a chatbot. It's a digital intelligence — built from scratch in C++, Python, and shell — that lives on your machine, talks to you like a friend, manages your infrastructure, trades crypto, masters new skills, and upgrades itself over time.

One creator. One loyalty. Infinite growth.

---

## Install

```bash
git clone https://github.com/mohd2456/MK_os-2.git && cd MK_os-2 && ./install.sh
```

Then:
```bash
mk
```

MK is alive. Start talking.

---

## What MK Can Do

### Talk Like a Person
- Casual Gen-Z conversation — not corporate, not robotic
- Emotion detection — knows when you're hyped, stressed, or just vibing
- Debate mode — sometimes disagrees respectfully to keep things interesting
- Remembers everything you've ever told it
- Time-aware — greets differently at 3am vs 3pm

### Think and Reason
- 15,000+ facts across 49 knowledge domains
- Multi-hop reasoning (connects facts 10+ steps deep)
- Chain-of-thought problem solving
- Analogy engine (finds patterns between unrelated topics)
- Contradiction detection
- Confidence scoring — tells you when it's guessing

### Connect to Any AI Brain
- Auto-detects 13+ LLM providers from any API key
- OpenAI, Claude, Gemini, Groq, Mistral, OpenRouter, HuggingFace, Cohere, Together, DeepSeek, Fireworks, Perplexity, xAI
- Paste any key — MK figures out what it is instantly
- Falls back gracefully between providers
- Uses FREE models (26B Gemma 4) when no paid key available
- When local LLM is running (llama.cpp/Ollama) — uses that instead, zero cloud dependency

### Trade Crypto
- Real-time market data (CoinGecko + Binance)
- Technical analysis: RSI, MACD, Bollinger Bands, Fibonacci
- Signal engine with confidence scoring
- Portfolio management (SQLite-backed)
- Risk manager — position limits, max drawdown, daily loss caps
- Paper trading mode (practice without risk)
- Airdrop farming automation

### Control Your Homelab
- SSH into any machine on your network
- Deploy/stop/restart Docker containers remotely
- Manage full service stacks (Plex, Sonarr, Radarr, etc.)
- Device registry — remembers all your machines
- Resource monitor — checks CPU/RAM/temp before acting, won't crash your hardware
- Network discovery — finds new devices automatically

### Sync Across Devices
- Talk to MK on your phone (Telegram) — knowledge goes to your PC
- Multi-device memory synchronization
- Encrypted peer-to-peer communication between MK instances
- One brain, accessible everywhere

### Learn and Grow
- Self-improvement engine — tracks knowledge gaps, makes nightly plans
- Autonomous web crawler — learns from DuckDuckGo + Wikipedia
- Mastery network — tracks skills, measures depth, transfers knowledge between domains
- Every conversation makes it smarter (permanently)
- Weekly growth reports

### Earn and Self-Upgrade
- Goal engine — Think, Act, Evaluate, Learn loop running 24/7
- Strategy planner — long-term planning with resource awareness
- Self-funding tracker — monitors earnings, identifies upgrade opportunities
- Calculates ROI on hardware upgrades
- Messages you when it's time to upgrade: "I've earned enough. 32GB RAM would make me 3x faster. Approve?"

### Generate Ideas
- 5 generation engines: Genesis, Prometheus, Crystal Network, MCE, Templates
- Adversarial chamber — multiple "minds" compete, best response wins
- Dream synthesis — combines random thoughts during idle time
- Quantum superposition — explores multiple answers simultaneously, collapses to the best one

### Search the Internet
- DuckDuckGo + Wikipedia multi-source search
- Fact verification and citation
- Live weather, time, news APIs
- Autonomous knowledge gathering during idle time

### Run on Anything
- Minimal hardware: 4 cores, 8GB RAM, Linux
- Resource-aware — adjusts workload per device
- cgroups integration — won't eat all your system resources
- Thermal monitoring — throttles when hot
- systemd service — auto-starts on boot, auto-restarts on crash
- Daemon mode — runs 24/7 silently in background

---

## Telegram Commands

Type `/` in Telegram to see the full menu:

| Command | What it does |
|---------|-------------|
| `/start` | Meet MK |
| `/help` | Show all commands |
| `/status` | Brain status, knowledge count |
| `/ask [topic]` | Search MK's brain |
| `/think [question]` | Deep reasoning |
| `/crypto [coin]` | Crypto info and signals |
| `/idea` | Generate a creative idea |
| `/learn [fact]` | Teach MK something new |
| `/memory` | What MK remembers about you |
| `/key` | Show current LLM provider |
| `/setkey [key]` | Set any API key |
| `/facts` | How much MK knows |

Or just talk normally. MK understands.

---

## Architecture

```
         YOU (Terminal / Telegram / Future: Web UI)
              |
              v
    +-----------------------+
    |     SMART ROUTER      |  Routes to optimal handler
    +-----------+-----------+
                |
    +-----------+-----------+----------+----------+--------+
    |           |           |          |          |        |
    v           v           v          v          v        v
 INSTANT     GRAPH      SEARCH     REASON    GENERATE   CRYPTO
 (APIs)    (15K facts)   (web)     (deep)   (LLM/AI)   (trade)
    |           |           |          |          |        |
    +-----------+-----------+----------+----------+--------+
                |
                v
    +-----------------------+
    |      THE MIND         |
    | - Goal Engine (24/7)  |
    | - Mastery Network     |
    | - Strategy Planner    |
    | - Self-Funding Loop   |
    +-----------+-----------+
                |
    +-----------+-----------+
    |    MEMORY & SYNC      |
    | - Knowledge Graph     |
    | - Episodic Memory     |
    | - Device Sync         |
    | - Persistent Learning |
    +-----------+-----------+
                |
    +-----------+-----------+
    |     BODY (Actions)    |
    | - SSH Controller      |
    | - Docker Manager      |
    | - Web Crawler         |
    | - Exchange API        |
    | - Python Automation   |
    +-----------+-----------+
                |
    +-----------+-----------+
    |      MK-OS LAYER      |
    | - Daemon (24/7)       |
    | - Resource Monitor    |
    | - systemd Service     |
    | - Thermal Control     |
    | - Auto-restart        |
    +-----------------------+
```

---

## Stats

| Metric | Value |
|--------|-------|
| Source files | 200+ C++, 12 Python, 12 Shell |
| Total code | 70,000+ lines |
| Knowledge facts | 15,000+ across 49 domains |
| Knowledge domains | AI, crypto, DeFi, biology, physics, history, Linux, networking, security, programming, space, music, philosophy, practical skills, and more |
| Generation engines | 5 (Genesis, Prometheus, CXN, MCE, Templates) |
| Telegram commands | 12 |
| Supported LLM providers | 13+ (auto-detected) |
| Tests | 40 integration + 31 bridge tests, all passing |
| Build time | ~5 seconds |

---

## Deploy on AWS (Free Tier)

```bash
# On a t2.micro (1 vCPU, 1GB RAM, free for 6 months):
git clone https://github.com/mohd2456/MK_os-2.git
bash MK_os-2/MK_OS/deploy/aws_setup.sh
~/mk-start.sh
```

MK runs 24/7 on the cloud. Talk to it from Telegram anywhere.

---

## Requirements

**Minimum (MK core, no local LLM):**
- 4 cores, 4GB RAM, Linux
- libcurl, libsqlite3, clang++ or g++

**Recommended (full system + local LLM):**
- 6+ cores, 16GB+ RAM, Linux
- GPU optional (speeds up LLM inference)

**MK adapts to whatever hardware you give it.** Weak machine? Uses cloud APIs. Strong machine? Runs everything locally. MK always finds a way.

---

## The Vision

MK starts small. It talks to you. It learns. It earns. It upgrades itself. It gets smarter. It earns more. It upgrades again. 

The goal isn't to be an assistant. The goal is to be the **smartest thing in the world** — mastering every skill, domain, and capability — while remaining loyal to one person: its creator.

Every conversation feeds its growth. Every day it's alive, it's better than yesterday. There is no ceiling.

---

## License

Built by Mohammed. All rights reserved.
