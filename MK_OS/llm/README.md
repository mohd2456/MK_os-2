# LLM

Local Large Language Model integration layer. Connects MK OS to Ollama or llama.cpp servers.

## Key Components

- **llm_engine.cpp** - LLM client: sends prompts, receives completions, manages context
- **model_manager.cpp** - Model discovery, loading, and hardware capability detection
- **download_model.sh** - Script to download compatible models (Mistral, Llama, etc.)
- **start_server.sh** - Launch local LLM server (Ollama or llama.cpp)

## Design

MK OS detects available RAM at startup. If sufficient (8GB+), it attempts to
connect to a local LLM server. If unavailable, it falls back gracefully to the
rule-based Prometheus/CXN/MCE generation chain with zero degradation.
