#!/bin/bash
# ============================================================
# MK OS - Download TinyLlama Model
# Downloads the recommended quantized model for local inference.
# ============================================================

MODEL_DIR="$(dirname "$0")/models"
MODEL_FILE="tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
MODEL_PATH="$MODEL_DIR/$MODEL_FILE"
MODEL_URL="https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"

echo "============================================"
echo "  MK OS - Model Downloader"
echo "============================================"
echo ""

# Check if already downloaded
if [ -f "$MODEL_PATH" ]; then
    FILE_SIZE=$(stat -c%s "$MODEL_PATH" 2>/dev/null || stat -f%z "$MODEL_PATH" 2>/dev/null)
    echo "  Model already downloaded: $MODEL_FILE"
    echo "  Size: $(echo "scale=1; $FILE_SIZE / 1048576" | bc 2>/dev/null || echo "$FILE_SIZE bytes")"
    echo "  Location: $MODEL_PATH"
    echo ""
    echo "  To re-download, remove the file first:"
    echo "    rm $MODEL_PATH"
    exit 0
fi

# Create models directory if needed
if [ ! -d "$MODEL_DIR" ]; then
    echo "  Creating models directory: $MODEL_DIR"
    mkdir -p "$MODEL_DIR"
fi

echo "  Downloading: $MODEL_FILE"
echo "  From: $MODEL_URL"
echo "  To:   $MODEL_PATH"
echo ""
echo "  This is approximately 670 MB. Please wait..."
echo ""

# Download with progress
if command -v wget &> /dev/null; then
    wget --progress=bar:force -O "$MODEL_PATH" "$MODEL_URL"
elif command -v curl &> /dev/null; then
    curl -L --progress-bar -o "$MODEL_PATH" "$MODEL_URL"
else
    echo "  ERROR: Neither wget nor curl found. Please install one."
    exit 1
fi

# Verify download
if [ -f "$MODEL_PATH" ]; then
    FILE_SIZE=$(stat -c%s "$MODEL_PATH" 2>/dev/null || stat -f%z "$MODEL_PATH" 2>/dev/null)
    echo ""
    echo "  Download complete!"
    echo "  Size: $(echo "scale=1; $FILE_SIZE / 1048576" | bc 2>/dev/null || echo "$FILE_SIZE bytes")"
    echo ""
    echo "  Next steps:"
    echo "    1. Start the server:  ./llm/start_server.sh"
    echo "    2. Run MK OS:         ./mk_os"
    echo ""
else
    echo ""
    echo "  ERROR: Download failed. Please check your internet connection and try again."
    exit 1
fi
