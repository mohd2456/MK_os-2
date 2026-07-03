# MK OS — AWS Deployment Guide

## Quick Start

### 1. Launch EC2 Instance
- Go to AWS Console → EC2 → Launch Instance
- **AMI:** Amazon Linux 2023 (free tier eligible)
- **Instance type:** t2.micro (free tier)
- **Key pair:** Create one (download the .pem file!)
- **Network:** Allow SSH (port 22) and optionally HTTP (port 80)
- **Storage:** 8GB (default, free)
- Launch it

### 2. Connect via SSH
```bash
chmod 400 your-key.pem
ssh -i your-key.pem ec2-user@YOUR_EC2_PUBLIC_IP
```

### 3. Run Setup Script
```bash
curl -sSL https://raw.githubusercontent.com/mohd2456/MK_os-2/mk-living-ai-mega-upgrade/MK_OS/deploy/aws_setup.sh | bash
```

Or manually:
```bash
git clone https://github.com/mohd2456/MK_os-2.git
cd MK_os-2/MK_OS/deploy
bash aws_setup.sh
```

### 4. Add LLM API Key (for natural language)
```bash
nano ~/.mk_os/env.sh
# Uncomment and fill: GROQ_API_KEY="gsk_your_key_here"
```

Get free key: https://console.groq.com (email signup, no card)

### 5. Start MK
```bash
~/mk-start.sh
```

### 6. Talk to MK
Open Telegram → search **@Mohdk5610_bot** → send a message.
MK is alive.

---

## Commands

| Command | What it does |
|---------|-------------|
| `~/mk-start.sh` | Start MK as 24/7 daemon |
| `~/mk-stop.sh` | Stop MK |
| `~/mk-status.sh` | Check MK status + memory usage |
| `~/mk-update.sh` | Pull latest code from GitHub, rebuild, restart |
| `~/mk-interactive.sh` | Run MK in terminal (for testing/chatting directly) |

## Memory Management (t2.micro = 1GB)

MK is configured to stay under 800MB RAM:
- Knowledge graph loads all 15,000+ facts (~300-500MB)
- Telegram polling uses minimal resources
- LLM calls go to cloud API (no local model = no RAM cost)
- systemd MemoryMax prevents OOM kills

## Logs

```bash
tail -f ~/.mk_os/mk.log        # Main output
tail -f ~/.mk_os/mk_error.log  # Errors
```

## Updating

When you push changes to the repo:
```bash
~/mk-update.sh
```
This pulls, rebuilds, and restarts MK automatically.

## Cost

- **t2.micro:** Free for 6 months (750h/month)
- **Storage (8GB):** Free for 6 months
- **Data transfer:** 100GB/month free
- **After 6 months:** ~$8.50/month if you keep it running

## Security

- Telegram token is stored in `~/.mk_os/env.sh` (not in the repo)
- API keys stored same way
- SSH key required to access the instance
- No ports open except SSH (and optionally 80 for future web UI)
