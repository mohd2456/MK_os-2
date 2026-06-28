@echo off
chcp 65001 >nul 2>&1
color 0E
title MK OS - Stop Agent

echo.
echo  ╔═══════════════════════════════════════════╗
echo  ║       MK OS - Stopping PC Agent           ║
echo  ╚═══════════════════════════════════════════╝
echo.

:: ─── Kill agent.py processes ──────────────────────────────────────────────────

echo  Looking for running agent processes...
echo.

tasklist /fi "imagename eq python.exe" /v 2>nul | find "agent.py" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo  Found running agent. Stopping...
    taskkill /f /fi "windowtitle eq MK OS - PC Agent" >nul 2>&1
    wmic process where "commandline like '%%agent.py%%'" call terminate >nul 2>&1
    timeout /t 2 /nobreak >nul
    echo.
    color 0A
    echo  ╔═══════════════════════════════════════════╗
    echo  ║         AGENT STOPPED SUCCESSFULLY        ║
    echo  ╚═══════════════════════════════════════════╝
) else (
    :: Try broader check
    wmic process where "commandline like '%%agent.py%%'" get processid 2>nul | find /v "ProcessId" | find /v "" >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo  Found running agent. Stopping...
        wmic process where "commandline like '%%agent.py%%'" call terminate >nul 2>&1
        timeout /t 2 /nobreak >nul
        echo.
        color 0A
        echo  ╔═══════════════════════════════════════════╗
        echo  ║         AGENT STOPPED SUCCESSFULLY        ║
        echo  ╚═══════════════════════════════════════════╝
    ) else (
        echo  No running agent found. It may already be stopped.
        echo.
        echo  TIP: You can also stop the agent by closing its window.
    )
)

echo.
pause
