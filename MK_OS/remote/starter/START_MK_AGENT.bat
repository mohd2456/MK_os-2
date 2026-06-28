@echo off
chcp 65001 >nul 2>&1
color 0A
title MK OS - PC Agent

echo.
echo  ╔═══════════════════════════════════════════╗
echo  ║        MK OS - PC Agent Starting          ║
echo  ╚═══════════════════════════════════════════╝
echo.

:: ─── Check agent.py exists ────────────────────────────────────────────────────

if not exist "%~dp0agent.py" (
    color 0C
    echo  ERROR: agent.py not found!
    echo.
    echo  Please make sure agent.py is in the same folder as this .bat file.
    echo  If you haven't set up yet, run INSTALL.bat first.
    echo.
    pause
    exit /b 1
)

:: ─── Read Token ───────────────────────────────────────────────────────────────

set TOKEN=

:: Try reading from user profile first
if exist "%USERPROFILE%\.mk_agent_token" (
    set /p TOKEN=<"%USERPROFILE%\.mk_agent_token"
)

:: Fallback: try config.txt
if "%TOKEN%"=="" (
    if exist "%~dp0config.txt" (
        for /f "tokens=2 delims==" %%a in ('findstr /i "TOKEN=" "%~dp0config.txt"') do set TOKEN=%%a
    )
)

:: No token found
if "%TOKEN%"=="" (
    color 0C
    echo  ERROR: No auth token found!
    echo.
    echo  Please run INSTALL.bat first to generate a token.
    echo.
    pause
    exit /b 1
)

:: ─── Show Connection Info ─────────────────────────────────────────────────────

echo  Agent is starting on port 9876
echo.
echo  ┌─────────────────────────────────────────┐
echo  │  YOUR PC's IP ADDRESS:                  │
echo  └─────────────────────────────────────────┘
for /f "tokens=2 delims=:" %%a in ('ipconfig ^| find "IPv4"') do echo       %%a
echo.
echo  Port:  9876
echo  Token: %TOKEN:~0,8%... (hidden for security)
echo.
echo  ─────────────────────────────────────────────
echo  Discovery beacon: ACTIVE (MK will find this PC automatically)
echo  On your Mac, configure MK with this IP + token.
echo  Keep this window open! Close it to stop the agent.
echo  ─────────────────────────────────────────────
echo.

:: ─── Start Agent ──────────────────────────────────────────────────────────────

python "%~dp0agent.py" --token %TOKEN%

:: If we get here, agent has stopped
echo.
echo  Agent has stopped.
echo.
pause
