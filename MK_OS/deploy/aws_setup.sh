#!/bin/bash
# ============================================================
# MK OS — AWS t2.micro Deployment Script
# ============================================================
# Run this ON the EC2 instance after SSH'ing in.
# Sets up everything MK needs to run 24/7 with Telegram.
#
# Instance: t2.micro (1 vCPU, 1GB RAM, Amazon Linux 2023)
# Usage: bash aws_setup.sh
# ============================================================

set -e

echo "=========================================="
echo "  MK OS — AWS Deployment"
echo "  Setting up your living AI..."
echo "=========================================="

# ─────────────────────────────────────────────
# 1. Install dependencies
# ─────────────────────────────────────────────
echo "[1/7] Installing system packages..."
sudo dnf update -y -q
sudo dnf install -y -q git gcc-c++ clang libcurl-devel sqlite-devel python3 python3-pip tmux htop

# ─────────────────────────────────────────────
# 2. Clone MK OS repo
# ─────────────────────────────────────────────
echo "[2/7] Cloning MK OS..."
cd ~
if [ -d "MK_os-2" ]; then
    cd MK_os-2 && git pull && cd ..
else
    git clone https://github.com/mohd2456/MK_os-2.git
fi

# ─────────────────────────────────────────────
# 3. Build MK
# ─────────────────────────────────────────────
echo "[3/7] Building MK OS..."
cd ~/MK_os-2/MK_OS
clang++ -std=c++17 -O2 -DLINUX -o mk_os system/mk_entry.cpp -lcurl -lsqlite3
echo "  ✓ Build successful"

# ─────────────────────────────────────────────
# 4. Install Python dependencies
# ─────────────────────────────────────────────
echo "[4/7] Installing Python packages..."
pip3 install -q requests

# ─────────────────────────────────────────────
# 5. Configure environment (tokens & keys)
# ─────────────────────────────────────────────
echo "[5/7] Configuring MK..."

# Create config directory
mkdir -p ~/.mk_os

# Set up environment file
cat > ~/.mk_os/env.sh << 'ENVEOF'
# MK OS Environment Variables
# Edit these values with your actual tokens

# Telegram Bot Token (from BotFather)
export MK_TELEGRAM_TOKEN="8694681522:AAHQ0SNSAm-wMZ2enbbn-4cNF8vV3eBsPqc"

# Free LLM API Key (get from console.groq.com — email signup, no card)
# Uncomment and fill ONE of these:
# export GROQ_API_KEY="your_key_here"
# export OPENROUTER_API_KEY="your_key_here"
# export HF_API_KEY="your_key_here"
ENVEOF

# Source it
echo 'source ~/.mk_os/env.sh' >> ~/.bashrc
source ~/.mk_os/env.sh

echo "  ✓ Telegram token configured"
echo "  ! Add a Groq/OpenRouter API key to ~/.mk_os/env.sh for full brain power"

# ─────────────────────────────────────────────
# 6. Create systemd service for 24/7 operation
# ─────────────────────────────────────────────
echo "[6/7] Setting up 24/7 daemon..."

sudo tee /etc/systemd/system/mk-os.service > /dev/null << 'SVCEOF'
[Unit]
Description=MK OS Living AI
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=ec2-user
WorkingDirectory=/home/ec2-user/MK_os-2/MK_OS
EnvironmentFile=/home/ec2-user/.mk_os/env.sh
ExecStart=/home/ec2-user/MK_os-2/MK_OS/mk_os --daemon
Restart=always
RestartSec=10
StandardOutput=append:/home/ec2-user/.mk_os/mk.log
StandardError=append:/home/ec2-user/.mk_os/mk_error.log

# Memory limit to prevent OOM on t2.micro
MemoryMax=800M
MemoryHigh=600M

[Install]
WantedBy=multi-user.target
SVCEOF

sudo systemctl daemon-reload
sudo systemctl enable mk-os
echo "  ✓ MK OS service created (auto-starts on boot)"

# ─────────────────────────────────────────────
# 7. Create helper scripts
# ─────────────────────────────────────────────
echo "[7/7] Creating helper scripts..."

# Start MK
cat > ~/mk-start.sh << 'EOF'
#!/bin/bash
source ~/.mk_os/env.sh
sudo systemctl start mk-os
echo "MK started. Check logs: tail -f ~/.mk_os/mk.log"
EOF
chmod +x ~/mk-start.sh

# Stop MK
cat > ~/mk-stop.sh << 'EOF'
#!/bin/bash
sudo systemctl stop mk-os
echo "MK stopped."
EOF
chmod +x ~/mk-stop.sh

# Check status
cat > ~/mk-status.sh << 'EOF'
#!/bin/bash
echo "=== MK OS Status ==="
sudo systemctl status mk-os --no-pager
echo ""
echo "=== Memory ==="
free -h
echo ""
echo "=== Last 20 log lines ==="
tail -20 ~/.mk_os/mk.log 2>/dev/null || echo "No logs yet"
EOF
chmod +x ~/mk-status.sh

# Update MK (pull latest, rebuild)
cat > ~/mk-update.sh << 'EOF'
#!/bin/bash
echo "Updating MK OS..."
sudo systemctl stop mk-os
cd ~/MK_os-2 && git pull
cd MK_OS && clang++ -std=c++17 -O2 -DLINUX -o mk_os system/mk_entry.cpp -lcurl -lsqlite3
sudo systemctl start mk-os
echo "MK updated and restarted."
EOF
chmod +x ~/mk-update.sh

# Interactive mode (for testing)
cat > ~/mk-interactive.sh << 'EOF'
#!/bin/bash
source ~/.mk_os/env.sh
cd ~/MK_os-2/MK_OS
./mk_os
EOF
chmod +x ~/mk-interactive.sh

echo ""
echo "=========================================="
echo "  ✓ MK OS DEPLOYMENT COMPLETE"
echo "=========================================="
echo ""
echo "  Commands:"
echo "    ~/mk-start.sh       Start MK (24/7 daemon)"
echo "    ~/mk-stop.sh        Stop MK"
echo "    ~/mk-status.sh      Check if MK is running"
echo "    ~/mk-update.sh      Pull latest code & rebuild"
echo "    ~/mk-interactive.sh Run MK in terminal (for testing)"
echo ""
echo "  Telegram: @Mohdk5610_bot — talk to MK from your phone"
echo ""
echo "  IMPORTANT: Add a free LLM API key for full brain power:"
echo "    nano ~/.mk_os/env.sh"
echo "    (uncomment and fill GROQ_API_KEY or OPENROUTER_API_KEY)"
echo "    then: ~/mk-update.sh"
echo ""
echo "  Logs: tail -f ~/.mk_os/mk.log"
echo ""
echo "  To start now: ~/mk-start.sh"
echo "=========================================="
