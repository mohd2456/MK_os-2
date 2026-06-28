#!/bin/bash
# ═══════════════════════════════════════════
# MK OS Launcher — Type 'mk' to start chat
# ═══════════════════════════════════════════

MK_HOME="${HOME}/.mk_os"
MK_BINARY="${MK_HOME}/mk_os"

# Ensure directories exist
mkdir -p "${MK_HOME}/knowledge"
mkdir -p "${MK_HOME}/logs"

# Check binary exists
if [ ! -f "${MK_BINARY}" ]; then
    echo "ERROR: MK OS binary not found at ${MK_BINARY}"
    echo "Run the installer again: cd MK_os-2 && ./install.sh"
    exit 1
fi

# Load config
if [ -f "${MK_HOME}/config" ]; then
    set -a
    source "${MK_HOME}/config"
    set +a
fi

# Export important paths
export MK_HOME
export MK_KNOWLEDGE_DIR="${MK_HOME}/knowledge"

# Run MK from its home directory (so knowledge files are found)
cd "${MK_HOME}"
exec "${MK_BINARY}" "$@"
