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

# Install location
PREFIX    := /usr/local/bin

# ============================================================
# Targets
# ============================================================

.PHONY: all clean run install tools help

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
	@rm -f $(TARGET) $(INGEST) $(TELEGRAM) $(LLM_EATER)
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
