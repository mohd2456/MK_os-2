@echo off
chcp 65001 >nul 2>&1
color 0B
title MK OS - Agent Status

echo.
echo  ╔═══════════════════════════════════════════╗
echo  ║        MK OS - PC Agent Status            ║
echo  ╚═══════════════════════════════════════════╝
echo.

:: ─── Check if agent is running ────────────────────────────────────────────────

echo  ┌─ AGENT STATUS ─────────────────────────────
echo  │

wmic process where "commandline like '%%agent.py%%'" get processid 2>nul | find /v "ProcessId" | find /r "." >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    color 0A
    echo  │  Status:  RUNNING ✓
) else (
    color 0C
    echo  │  Status:  STOPPED ✗
)

echo  │
echo  └─────────────────────────────────────────────
echo.

:: ─── Show IP Address ──────────────────────────────────────────────────────────

echo  ┌─ NETWORK INFO ─────────────────────────────
echo  │
echo  │  IP Address(es):
for /f "tokens=2 delims=:" %%a in ('ipconfig ^| find "IPv4"') do echo  │       %%a
echo  │
echo  │  Port: 9876
echo  │
echo  └─────────────────────────────────────────────
echo.

:: ─── Show Token ───────────────────────────────────────────────────────────────

echo  ┌─ AUTH TOKEN ────────────────────────────────
echo  │

set TOKEN=
if exist "%USERPROFILE%\.mk_agent_token" (
    set /p TOKEN=<"%USERPROFILE%\.mk_agent_token"
)

if "%TOKEN%"=="" (
    if exist "%~dp0config.txt" (
        for /f "tokens=2 delims==" %%a in ('findstr /i "TOKEN=" "%~dp0config.txt"') do set TOKEN=%%a
    )
)

if "%TOKEN%"=="" (
    echo  │  Token: NOT SET (run INSTALL.bat first)
) else (
    echo  │  Token: %TOKEN%
)

echo  │
echo  │  Token file: %USERPROFILE%\.mk_agent_token
echo  │
echo  └─────────────────────────────────────────────
echo.

:: ─── Connection String ────────────────────────────────────────────────────────

echo  ┌─ FOR YOUR MAC ─────────────────────────────
echo  │
echo  │  Use these settings in MK on your Mac:
echo  │
for /f "tokens=2 delims=:" %%a in ('ipconfig ^| find "IPv4"') do echo  │    IP:    %%a
echo  │    Port:  9876
if not "%TOKEN%"=="" echo  │    Token: %TOKEN%
echo  │
echo  └─────────────────────────────────────────────
echo.

pause
