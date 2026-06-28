# MK OS - Local LLM Integration

MK OS supports local LLM inference for enhanced text understanding and generation.
The system auto-detects available LLM servers at boot time.

## Recommended Models

| Model | RAM Required | Quality | Use Case |
|-------|-------------|---------|----------|
| **TinyLlama 1.1B Q4** | 2 GB minimum | Good for basic tasks | Low-resource systems, quick responses |
| **Mistral 7B Q4** | 8 GB minimum | High quality | Better reasoning, longer context |

For most users, **TinyLlama 1.1B Q4** is the best starting point. It runs on nearly
any modern machine and provides meaningful AI-assisted text cleanup, intent detection,
and short generation tasks.

If you have 8+ GB of free RAM, **Mistral 7B Q4** offers significantly better quality
for reasoning, summarization, and creative tasks.

## Hardware Requirements

- **Minimum:** 2 GB free RAM (for TinyLlama 1.1B Q4)
- **Recommended:** 8 GB free RAM (for Mistral 7B Q4)
- **CPU:** Any modern x86_64 or ARM64 processor
- **Disk:** ~670 MB for TinyLlama, ~4.4 GB for Mistral 7B

MK OS will automatically disable LLM features and warn you at boot if your system
does not have enough free RAM.

## Quick Setup

### Option 1: llama.cpp (Recommended)

```bash
# 1. Download the model
./llm/download_model.sh

# 2. Start the server
./llm/start_server.sh

# 3. Run MK OS (in another terminal)
./mk_os
```

### Option 2: Ollama

```bash
# 1. Install Ollama
curl -fsSL https://ollama.ai/install.sh | sh

# 2. Pull and run TinyLlama
ollama run tinyllama

# 3. Run MK OS (in another terminal)
./mk_os
```

## How It Works

1. At boot, MK OS checks available system RAM
2. If RAM is sufficient (>= 2 GB free), it probes for LLM servers:
   - First checks llama.cpp server at `localhost:8080/health`
   - Then checks Ollama at `localhost:11434/api/tags`
3. If a server is found, LLM-powered features are enabled:
   - Enhanced input preprocessing (AI text cleanup)
   - Natural language generation
4. If no server is running, MK OS falls back to its rule-based pipeline

## Supported Backends

| Backend | Default Port | Auto-detected |
|---------|-------------|---------------|
| llama.cpp (llama-server) | 8080 | Yes |
| Ollama | 11434 | Yes |

## Troubleshooting

**"No local LLM server detected"**
- Make sure either llama.cpp server or Ollama is running
- Check if the server is healthy: `curl http://localhost:8080/health`
- For Ollama: `curl http://localhost:11434/api/tags`

**"Hardware insufficient"**
- Close other applications to free RAM
- Consider using a smaller quantized model
- Check available RAM: `free -h` (Linux) or `vm_stat` (macOS)

**Server starts but MK OS does not detect it**
- Ensure the server is fully started before launching MK OS
- The health check uses a 1-second timeout; slow startups may be missed
- After the server is ready, restart MK OS or it will use rule-based mode

## File Structure

```
llm/
  llm_engine.cpp      - LLM engine class (server detection, generation)
  model_manager.cpp   - Hardware detection and RAM checking
  download_model.sh   - Script to download TinyLlama model
  start_server.sh     - Script to launch llama-server
  README.md           - This file
  models/             - Downloaded model files (git-ignored)
```
