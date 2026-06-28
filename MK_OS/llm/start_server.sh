#!/bin/bash
# ============================================================
# MK OS - Start LLM Server
# Launches llama-server with the downloaded model on port 8080.
# ============================================================

SCRIPT_DIR="$(dirname "$0")"
MODEL_DIR="$SCRIPT_DIR/models"
MODEL_FILE="tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
MODEL_PATH="$MODEL_DIR/$MODEL_FILE"
PORT=8080

echo "============================================"
echo "  MK OS - LLM Server Launcher"
echo "============================================"
echo ""

# Check if model file exists
if [ ! -f "$MODEL_PATH" ]; then
    echo "  ERROR: Model file not found!"
    echo ""
    echo "  Expected: $MODEL_PATH"
    echo ""
    echo "  Please download the model first:"
    echo "    ./llm/download_model.sh"
    echo ""
    exit 1
fi

# Check if llama-server is available
LLAMA_SERVER=""
if command -v llama-server &> /dev/null; then
    LLAMA_SERVER="llama-server"
elif command -v ./llama-server &> /dev/null; then
    LLAMA_SERVER="./llama-server"
elif [ -f "$SCRIPT_DIR/llama-server" ]; then
    LLAMA_SERVER="$SCRIPT_DIR/llama-server"
elif command -v server &> /dev/null; then
    LLAMA_SERVER="server"
else
    echo "  ERROR: llama-server not found!"
    echo ""
    echo "  Please install llama.cpp and ensure 'llama-server' is in your PATH."
    echo ""
    echo "  Quick install:"
    echo "    git clone https://github.com/ggerganov/llama.cpp"
    echo "    cd llama.cpp && make -j"
    echo "    cp llama-server /usr/local/bin/"
    echo ""
    echo "  Or use Ollama instead (auto-detected by MK OS):"
    echo "    curl -fsSL https://ollama.ai/install.sh | sh"
    echo "    ollama run tinyllama"
    echo ""
    exit 1
fi

# Check if port is already in use
if command -v lsof &> /dev/null; then
    if lsof -i :$PORT &> /dev/null; then
        echo "  WARNING: Port $PORT is already in use."
        echo "  Another server may already be running."
        echo ""
        echo "  To check: curl http://localhost:$PORT/health"
        echo "  To kill:  lsof -ti :$PORT | xargs kill"
        echo ""
        exit 1
    fi
elif command -v ss &> /dev/null; then
    if ss -tlnp | grep -q ":$PORT " 2>/dev/null; then
        echo "  WARNING: Port $PORT is already in use."
        echo ""
        exit 1
    fi
fi

echo "  Starting LLM server..."
echo "  Model:  $MODEL_FILE"
echo "  Port:   $PORT"
echo "  Server: $LLAMA_SERVER"
echo ""
echo "  Press Ctrl+C to stop the server."
echo "  ----------------------------------------"
echo ""

# Launch the server
exec $LLAMA_SERVER \
    --model "$MODEL_PATH" \
    --port $PORT \
    --ctx-size 2048 \
    --threads 4
