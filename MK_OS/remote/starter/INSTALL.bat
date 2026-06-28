@echo off
chcp 65001 >nul 2>&1
color 0A
title MK OS - PC Agent Installer

echo.
echo  ╔═══════════════════════════════════════════╗
echo  ║       MK OS - PC Agent Installer          ║
echo  ╚═══════════════════════════════════════════╝
echo.

:: ─── Check for Python ─────────────────────────────────────────────────────────

echo  [1/4] Checking for Python...
echo.

python --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    for /f "tokens=*" %%i in ('python --version 2^>^&1') do set PYTHON_VER=%%i
    echo        Found: %PYTHON_VER%
    set PYTHON_CMD=python
    goto :python_found
)

python3 --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    for /f "tokens=*" %%i in ('python3 --version 2^>^&1') do set PYTHON_VER=%%i
    echo        Found: %PYTHON_VER%
    set PYTHON_CMD=python3
    goto :python_found
)

:: Python not found
color 0C
echo.
echo  ╔═══════════════════════════════════════════╗
echo  ║  ERROR: Python is not installed!          ║
echo  ╚═══════════════════════════════════════════╝
echo.
echo  Python is required to run the MK PC Agent.
echo  Opening Python download page...
echo.
start https://www.python.org/downloads/
echo  After installing Python:
echo    - Make sure to check "Add Python to PATH"
echo    - Restart this installer
echo.
pause
exit /b 1

:python_found
echo.

:: ─── Install Dependencies ─────────────────────────────────────────────────────

echo  [2/5] Installing dependencies...
echo.
%PYTHON_CMD% -m pip install --upgrade pip >nul 2>&1
%PYTHON_CMD% -m pip install psutil pyperclip Pillow pystray
echo.
if %ERRORLEVEL% NEQ 0 (
    color 0E
    echo  WARNING: Some dependencies may have failed to install.
    echo  The agent will still work but some features may be limited.
    echo.
)

:: ─── Get Mac IP Address ────────────────────────────────────────────────────────

echo  [3/5] Configure MK Server connection...
echo.
echo  Enter your Mac's IP address (where MK runs):
echo  (Press Enter to skip if you don't know yet)
set /p MAC_IP=       IP: 
echo.
if "%MAC_IP%"=="" (
    set MAC_IP=127.0.0.1
    echo        Using default: 127.0.0.1 (update config.txt later)
) else (
    echo        Mac IP set to: %MAC_IP%
)
echo.

:: ─── Generate Auth Token ──────────────────────────────────────────────────────

echo  [4/5] Generating secure auth token...
echo.

for /f "tokens=*" %%t in ('%PYTHON_CMD% -c "import secrets; print(secrets.token_hex(16))"') do set TOKEN=%%t

:: Save token to user profile
echo %TOKEN%> "%USERPROFILE%\.mk_agent_token"
echo        Token saved to: %USERPROFILE%\.mk_agent_token

:: ─── Create Config File ───────────────────────────────────────────────────────

echo  [5/5] Creating config file...
echo.

(
echo # MK OS PC Agent Configuration
echo # Generated on %date% at %time%
echo.
echo TOKEN=%TOKEN%
echo PORT=9876
echo HOST=0.0.0.0
echo MK_SERVER_IP=%MAC_IP%
echo MK_SERVER_PORT=9878
) > "%~dp0config.txt"

echo        Config saved to: %~dp0config.txt

:: ─── Done ─────────────────────────────────────────────────────────────────────

echo.
echo  ╔═══════════════════════════════════════════╗
echo  ║         INSTALLATION COMPLETE!            ║
echo  ╚═══════════════════════════════════════════╝
echo.
echo  Your auth token: %TOKEN%
echo  Mac IP (MK server): %MAC_IP%
echo.
echo  ┌─────────────────────────────────────────┐
echo  │  AVAILABLE TOOLS:                       │
echo  │                                         │
echo  │  START_MK_AGENT.bat  - PC Agent (recv)  │
echo  │  MK_CLI.bat          - Terminal chat    │
echo  │  START_MK_CHAT.bat   - Tray chat GUI   │
echo  └─────────────────────────────────────────┘
echo.
echo  ┌─────────────────────────────────────────┐
echo  │  NEXT STEPS:                            │
echo  │                                         │
echo  │  1. START_MK_AGENT.bat  (receives cmds) │
echo  │  2. MK_CLI.bat or START_MK_CHAT.bat     │
echo  │     (to chat with MK on your Mac)       │
echo  └─────────────────────────────────────────┘
echo.
echo  Your PC's IP address(es):
for /f "tokens=2 delims=:" %%a in ('ipconfig ^| find "IPv4"') do echo       %%a
echo.
echo  Agent Port: 9876 (Mac connects here)
echo  Query Port: 9878 (CLI/Chat connects to Mac)
echo.
pause
