# MK OS — Hybrid AI That Runs Locally

A custom-built AI system with knowledge graphs, reasoning chains, internet access, Telegram integration, and self-improvement. Runs on your machine. Never lies. Gets smarter every day.

## Quick Install (Arch Linux)

One command to rule them all:

```bash
git clone https://github.com/mohd2456/MK_os-2.git && cd MK_os-2 && ./install.sh
```

Then just type:
```bash
mk
```

That's it. MK is now running. Start chatting.

## Alternative Install (curl one-liner)

```bash
curl -sSL https://raw.githubusercontent.com/mohd2456/MK_os-2/main/install.sh | bash
```

## What Happens When You Run `./install.sh`

1. Detects your Linux distro (Arch, Ubuntu, Fedora)
2. Installs all dependencies automatically (clang, curl, sqlite, etc.)
3. Builds MK OS from source (C++17, optimized)
4. Installs the binary to `~/.mk_os/mk_os`
5. Copies knowledge files (7,400+ facts)
6. Creates the `mk` command at `/usr/local/bin/mk`
7. Asks for Telegram bot token (optional)
8. Done! Type `mk` from anywhere.

## What MK Can Do

- Answer questions using 7,400+ facts + logical reasoning
- Search the internet and cite sources
- Weather, time, news in real-time
- Learn new facts you teach it
- Correct itself when you say "that's wrong"
- Execute Python, Bash, C++ code
- Read files and images
- Math/science calculations
- Self-improve every night (researches gaps, generates new rules)
- Auto-update from GitHub
- Telegram bot (message from your phone)
- Remote PC control (from Mac to your PC)

## Requirements

- Linux (Arch, Ubuntu, Fedora) or macOS
- g++ or clang++ with C++17 support
- libcurl and libsqlite3
- ~100MB disk space
- 512MB+ RAM

## Telegram Setup (optional)

1. Message @BotFather on Telegram
2. Create a new bot, get the token
3. During install, paste the token when asked
4. Or set it later: `echo 'MK_TELEGRAM_TOKEN="your_token"' >> ~/.mk_os/config`
5. Restart MK — it auto-connects to Telegram
6. Now message your bot from your phone!

## Commands

Type naturally or use slash commands:
```
/help        Show all commands
/ask <q>     Knowledge graph lookup
/think <q>   Deep multi-step reasoning
/search <q>  Internet search (cited)
/learn s|r|t Teach MK a fact
/weather     Live weather
/time        Current time
/news        Tech headlines
/calc        Math/science solver
/read <path> Read a file
/run python  Execute code
/status      System diagnostics
/update      Check for updates
/grow        Research knowledge gaps
/quit        Save and exit
```

## File Structure After Install

```
~/.mk_os/
├── mk_os              # The binary
├── config             # Your settings (Telegram token, etc.)
├── knowledge/         # Knowledge files (7,400+ facts)
│   └── knowledge_data.txt
└── logs/              # Runtime logs

/usr/local/bin/mk      # The launcher (type 'mk' anywhere)
```

## Architecture

```
┌─────────────────────────────────────────────┐
│           MK OS — Unified Entry Point       │
├─────────────────────────────────────────────┤
│  Smart Router → routes queries to:          │
│    • Knowledge Graph (7,400+ facts)         │
│    • Deep Reasoner (multi-hop logic)        │
│    • Internet Search (DuckDuckGo + Wiki)    │
│    • Real-time APIs (weather/time/news)     │
│    • Math Solver                            │
│    • Code Runner                            │
├─────────────────────────────────────────────┤
│  Self-Improvement:                          │
│    • Nightly knowledge growth               │
│    • Auto-generated reasoning rules         │
│    • Auto-update from GitHub                │
├─────────────────────────────────────────────┤
│  Communication:                             │
│    • Terminal REPL (beautiful CLI)           │
│    • Telegram bot (24/7)                    │
│    • Remote PC control                      │
└─────────────────────────────────────────────┘
```

## Update MK

```bash
cd MK_os-2
git pull
./install.sh
```

Or from inside MK: type `/update`

## Uninstall

```bash
sudo rm /usr/local/bin/mk
rm -rf ~/.mk_os
```

## License

Personal project by Mohammed. All rights reserved.
