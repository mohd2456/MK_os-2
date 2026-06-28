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

echo  [2/4] Installing dependencies...
echo.
%PYTHON_CMD% -m pip install --upgrade pip >nul 2>&1
%PYTHON_CMD% -m pip install psutil pyperclip Pillow
echo.
if %ERRORLEVEL% NEQ 0 (
    color 0E
    echo  WARNING: Some dependencies may have failed to install.
    echo  The agent will still work but some features may be limited.
    echo.
)

:: ─── Generate Auth Token ──────────────────────────────────────────────────────

echo  [3/4] Generating secure auth token...
echo.

for /f "tokens=*" %%t in ('%PYTHON_CMD% -c "import secrets; print(secrets.token_hex(16))"') do set TOKEN=%%t

:: Save token to user profile
echo %TOKEN%> "%USERPROFILE%\.mk_agent_token"
echo        Token saved to: %USERPROFILE%\.mk_agent_token

:: ─── Create Config File ───────────────────────────────────────────────────────

echo  [4/4] Creating config file...
echo.

(
echo # MK OS PC Agent Configuration
echo # Generated on %date% at %time%
echo.
echo TOKEN=%TOKEN%
echo PORT=9876
echo HOST=0.0.0.0
) > "%~dp0config.txt"

echo        Config saved to: %~dp0config.txt

:: ─── Done ─────────────────────────────────────────────────────────────────────

echo.
echo  ╔═══════════════════════════════════════════╗
echo  ║         INSTALLATION COMPLETE!            ║
echo  ╚═══════════════════════════════════════════╝
echo.
echo  Your auth token: %TOKEN%
echo.
echo  ┌─────────────────────────────────────────┐
echo  │  NEXT STEPS:                            │
echo  │                                         │
echo  │  1. Double-click START_MK_AGENT.bat     │
echo  │  2. Leave it running                    │
echo  │  3. On your Mac, MK will auto-connect   │
echo  └─────────────────────────────────────────┘
echo.
echo  Your PC's IP address(es):
for /f "tokens=2 delims=:" %%a in ('ipconfig ^| find "IPv4"') do echo       %%a
echo.
echo  Port: 9876
echo.
pause
