MK OS - Windows PC Agent
=========================

This lets MK (running on your Mac) control this PC remotely.

SETUP (one time):
1. Double-click INSTALL.bat
2. Follow the instructions

DAILY USE:
1. Double-click START_MK_AGENT.bat (leave it running)
2. On your Mac, MK will auto-connect

COMMANDS:
- INSTALL.bat        → First-time setup
- START_MK_AGENT.bat → Start the agent (leave running)
- STOP_MK_AGENT.bat  → Stop the agent
- STATUS.bat         → Check if running + show IP/token

SECURITY:
- Keep your token secret
- The agent only accepts connections with the correct token
- Only use on your home network

TROUBLESHOOTING:
- "Python not found" → Install Python from python.org, check "Add to PATH"
- "Token not found"  → Run INSTALL.bat again
- Can't connect      → Check Windows Firewall allows port 9876
- Agent crashes      → Check if port 9876 is already in use

REQUIREMENTS:
- Windows 10 or later
- Python 3.8 or later
- Network connection (same network as your Mac)
