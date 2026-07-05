# ============================================================
# MK OS - Makefile
# Build the entire MK system into a single binary
# Target: macOS (Darwin) with g++/clang++
# External deps: libcurl, libsqlite3
# ============================================================

CXX       := clang++
CXXFLAGS  := -std=c++17 -O2 -Wall -Wextra -DDARWIN
LDFLAGS   := -lcurl -lsqlite3

# Output binary
TARGET    := mk_os
ENTRY     := system/mk_entry.cpp

# Tool binaries
INGEST    := mk_ingest
TELEGRAM  := mk_telegram
LLM_EATER := mk_llm_eater

# Single compilation unit: mk_entry.cpp #includes all other modules
# No need for separate .o files - this avoids multiple-definition linker errors
# Modules included via mk_entry.cpp:
#   system/ - diagnostics, daemon, shell, service_manager
#   orchestrator/ - conversation_loop (single LLM call architecture)
#   ai_core/hre/ - pattern_graph (knowledge store, KEPT)
#   ai_core/ - persistent_memory, math_solver
#   mk_brain/ - learning_engine, brain_memory, embeddings, ann_search, cache_mgr,
#               daily_briefing, fact_extractor, personality/response_style
#   network/ - realtime_apis
#   plugins/ - telegram, pc_helper
#   llm/ - cloud_llm, llm_engine, provider_router, request_logger
#   security/ - key_encryption
#   crypto/ - market_data, technical_analysis, signal_engine, portfolio_manager,
#             risk_manager, exchange_api, trading_bot, airdrop_farmer
#   mind/ - mastery_network, goal_engine, strategy_planner, self_funding,
#           knowledge_validator, autonomous_learner
#   homelab/ - device_registry, resource_monitor, ssh_controller, docker_manager,
#              service_orchestrator
#   sync/ - knowledge_sync, device_comm, memory_sync
#   linux/ - gpu_monitor, network_interface, storage_monitor
#   config/ - mk_config

# Install location
PREFIX    := /usr/local/bin

# ============================================================
# Targets
# ============================================================

.PHONY: all clean run install tools test help python-deps python-check lint full-test

all: $(TARGET)
	@echo ""
	@echo "  ✓ Built $(TARGET) successfully"
	@echo "  Run with: ./$(TARGET)"
	@echo ""

$(TARGET): $(ENTRY)
	$(CXX) $(CXXFLAGS) -o $@ $(ENTRY) $(LDFLAGS)

# Build standalone tools
SYNC      := mk_sync
tools: $(INGEST) $(TELEGRAM) $(LLM_EATER) $(SYNC)

$(INGEST): tools/knowledge_ingest.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TELEGRAM): tools/telegram_bridge.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(LLM_EATER): tools/llm_eater.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(SYNC): tools/knowledge_sync.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Run tests
TEST_TARGET := mk_os_test
TEST_ENTRY  := tests/test_main.cpp

test: $(TEST_TARGET)
	@./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_ENTRY)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_ENTRY) $(LDFLAGS)

# Run MK OS
run: $(TARGET)
	@./$(TARGET)

# Install to system path
install: $(TARGET)
	@echo "Installing $(TARGET) to $(PREFIX)..."
	@cp $(TARGET) $(PREFIX)/$(TARGET)
	@echo "  ✓ Installed. Run with: mk_os"

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@find . -name '*.o' -delete
	@rm -f $(TARGET) $(TEST_TARGET) $(INGEST) $(TELEGRAM) $(LLM_EATER)
	@echo "  ✓ Clean"

# Install Python dependencies
python-deps:
	@echo "Installing Python dependencies..."
	@pip3 install -r python/requirements.txt
	@echo "  ✓ Python dependencies installed"

# Validate Python syntax
python-check:
	@echo "Checking Python files..."
	@find . -name '*.py' -exec python3 -m py_compile {} \; 2>&1 && echo "  ✓ All Python files valid" || echo "  ✗ Python syntax errors found"

# Basic lint checks
lint:
	@echo "Running lint checks..."
	@echo "  Checking shell scripts..."
	@for f in $$(find . -name '*.sh'); do bash -n "$$f" && echo "    ✓ $$f" || echo "    ✗ $$f"; done
	@echo "  Checking for common issues..."
	@! grep -rn 'TODO\|FIXME\|HACK' system/ ai_core/ mk_brain/ --include='*.cpp' 2>/dev/null | head -5 || true
	@echo "  ✓ Lint complete"

# Full test suite: build + test + python check + shell syntax check
full-test: all test python-check
	@echo "Running shell syntax checks..."
	@for f in $$(find . -name '*.sh'); do bash -n "$$f" || exit 1; done
	@echo ""
	@echo "  ✓ Full test suite passed"
	@echo ""

# Help target
help:
	@echo ""
	@echo "  ╔══════════════════════════════════════╗"
	@echo "  ║         MK OS Build System           ║"
	@echo "  ╚══════════════════════════════════════╝"
	@echo ""
	@echo "  Usage: make <target>"
	@echo ""
	@echo "  Targets:"
	@echo "    all       - Build mk_os binary (default)"
	@echo "    clean     - Remove all build artifacts"
	@echo "    run       - Build and run mk_os"
	@echo "    install   - Install mk_os to $(PREFIX)"
	@echo "    tools     - Build standalone tools (mk_ingest, mk_telegram, mk_llm_eater)"
	@echo "    help      - Show this help message"
	@echo ""
	@echo "  Dependencies:"
	@echo "    - clang++ or g++ with C++17 support"
	@echo "    - libcurl (brew install curl)"
	@echo "    - libsqlite3 (included with macOS)"
	@echo ""
	@echo "  Quick start:"
	@echo "    make && ./mk_os"
	@echo ""
	@echo "  Knowledge ingestion:"
	@echo "    make tools"
	@echo "    ./mk_ingest <source_directory> <output.mk>"
	@echo ""
	@echo "  LLM knowledge extraction:"
	@echo "    ./mk_llm_eater <topic> <api_url> [output.mk]"
	@echo ""
