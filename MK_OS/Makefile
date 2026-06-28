# ============================================================
# MK OS - Makefile
# Build the entire MK system into a single binary
# Target: Linux (Arch/Ubuntu/Fedora) and macOS (Darwin)
# External deps: libcurl, libsqlite3
# ============================================================

# Auto-detect OS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    OS_FLAG := -DLINUX
else ifeq ($(UNAME_S),Darwin)
    OS_FLAG := -DDARWIN
else
    OS_FLAG := -DLINUX
endif

# Auto-detect compiler (prefer clang++, fallback to g++)
ifeq ($(shell command -v clang++ 2>/dev/null),)
    CXX := g++
else
    CXX := clang++
endif

CXXFLAGS  := -std=c++17 -O2 -Wall -Wextra $(OS_FLAG)
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

# Install location
PREFIX    := /usr/local/bin
MK_HOME   := $(HOME)/.mk_os

# ============================================================
# Targets
# ============================================================

.PHONY: all clean run install install-arch install-full tools test help

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

# Install to system path (basic)
install: $(TARGET)
	@echo "Installing $(TARGET) to $(PREFIX)..."
	@sudo cp $(TARGET) $(PREFIX)/$(TARGET)
	@sudo chmod +x $(PREFIX)/$(TARGET)
	@echo "  ✓ Installed. Run with: mk_os"

# Install for Arch Linux (installs as 'mk' command)
install-arch: $(TARGET)
	@echo "Installing MK OS to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/mk_os_bin
	@sudo chmod +x /usr/local/bin/mk_os_bin
	@sudo cp mk_wrapper.sh /usr/local/bin/mk
	@sudo chmod +x /usr/local/bin/mk
	@echo "  ✓ Done! Type 'mk' to start."

# Full install: binary + knowledge + config + launcher
install-full: $(TARGET)
	@echo "Full MK OS installation..."
	@mkdir -p $(MK_HOME)/knowledge
	@mkdir -p $(MK_HOME)/logs
	@cp $(TARGET) $(MK_HOME)/mk_os
	@chmod +x $(MK_HOME)/mk_os
	@if [ -f knowledge/knowledge_data.txt ]; then \
		cp knowledge/knowledge_data.txt $(MK_HOME)/knowledge/; \
	fi
	@find . -name "*.mk" -exec cp {} $(MK_HOME)/knowledge/ \; 2>/dev/null || true
	@sudo cp mk_wrapper.sh /usr/local/bin/mk
	@sudo chmod +x /usr/local/bin/mk
	@echo ""
	@echo "  ✓ MK OS fully installed!"
	@echo "  Type 'mk' from anywhere to start chatting."
	@echo ""

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@find . -name '*.o' -delete
	@rm -f $(TARGET) $(TEST_TARGET) $(INGEST) $(TELEGRAM) $(LLM_EATER)
	@echo "  ✓ Clean"

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
	@echo "    all          - Build mk_os binary (default)"
	@echo "    clean        - Remove all build artifacts"
	@echo "    run          - Build and run mk_os"
	@echo "    install      - Install mk_os to $(PREFIX)"
	@echo "    install-arch - Install as 'mk' command (Arch Linux)"
	@echo "    install-full - Full install with knowledge + config"
	@echo "    tools        - Build standalone tools (mk_ingest, mk_telegram, mk_llm_eater)"
	@echo "    help         - Show this help message"
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
