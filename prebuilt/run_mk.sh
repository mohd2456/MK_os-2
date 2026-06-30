#!/bin/bash
# MK OS Quick Run Script for ARM64
# Just copy this folder to your Arch Linux ARM and run this script
chmod +x mk_os_arm64
cd "$(dirname "$0")"
mkdir -p ai_core/hre/knowledge_files
cp *.mk ai_core/hre/knowledge_files/ 2>/dev/null
./mk_os_arm64
