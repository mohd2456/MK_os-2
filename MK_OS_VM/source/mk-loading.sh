#!/bin/bash
tput civis
clear
COLS=$(tput cols)
LINES=$(tput lines)
CYAN='\033[1;36m'
WHITE='\033[1;37m'
GREEN='\033[1;32m'
GRAY='\033[0;90m'
RESET='\033[0m'
SMALL_LOGO=("  ╔╦╗╦╔═  ╔═╗╔═╗" "  ║║║╠╩╗  ║ ║╚═╗" "  ╩ ╩╩ ╩  ╚═╝╚═╝")
LOGO_START=$(( (LINES / 2) - 5 ))
for i in "${!SMALL_LOGO[@]}"; do
    PAD=$(( (COLS - 18) / 2 ))
    tput cup $((LOGO_START + i)) $PAD
    printf "${CYAN}${SMALL_LOGO[$i]}${RESET}"
done
LOADING_TEXT="Loading MK OS..."
LOAD_PAD=$(( (COLS - ${#LOADING_TEXT}) / 2 ))
LOAD_LINE=$((LOGO_START + 5))
tput cup $LOAD_LINE $LOAD_PAD
printf "${WHITE}${LOADING_TEXT}${RESET}"
BAR_WIDTH=40
BAR_PAD=$(( (COLS - BAR_WIDTH - 2) / 2 ))
BAR_LINE=$((LOAD_LINE + 2))
MESSAGES=("Initializing kernel..." "Loading system modules..." "Starting MK services..." "Configuring display..." "Loading user profile..." "Starting MK Desktop...")
MSG_LINE=$((BAR_LINE + 2))
for step in $(seq 0 100); do
    FILLED=$(( step * BAR_WIDTH / 100 ))
    EMPTY=$(( BAR_WIDTH - FILLED ))
    tput cup $BAR_LINE $BAR_PAD
    printf "${WHITE}[${RESET}${GREEN}"
    for f in $(seq 1 $FILLED); do printf "█"; done
    printf "${GRAY}"
    for e in $(seq 1 $EMPTY); do printf "░"; done
    printf "${WHITE}]${RESET}"
    tput cup $BAR_LINE $((BAR_PAD + BAR_WIDTH + 3))
    printf "${WHITE}%3d%%${RESET}" $step
    MSG_IDX=$(( step * ${#MESSAGES[@]} / 101 ))
    MSG="${MESSAGES[$MSG_IDX]}"
    MSG_PAD=$(( (COLS - ${#MSG}) / 2 ))
    tput cup $MSG_LINE 0
    printf "%${COLS}s" ""
    tput cup $MSG_LINE $MSG_PAD
    printf "${GRAY}${MSG}${RESET}"
    if [ $step -lt 20 ]; then sleep 0.02
    elif [ $step -lt 50 ]; then sleep 0.04
    elif [ $step -lt 80 ]; then sleep 0.03
    else sleep 0.01; fi
done
tput cup $MSG_LINE 0
printf "%${COLS}s" ""
DONE_MSG="Welcome to MK OS"
DONE_PAD=$(( (COLS - ${#DONE_MSG}) / 2 ))
tput cup $MSG_LINE $DONE_PAD
printf "${GREEN}${DONE_MSG}${RESET}"
sleep 1
clear
