@echo off
chcp 65001 >nul 2>&1
title MK OS — CLI
color 0F
python "%~dp0mk.py" %*
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo  Error: Python could not run mk.py
    echo  Make sure Python is installed and in PATH.
    echo.
    pause
)
