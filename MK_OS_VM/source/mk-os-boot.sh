#!/bin/bash
export TERM=linux
stty sane 2>/dev/null
/opt/mk-os/mk-splash.sh
/opt/mk-os/mk-loading.sh
while true; do
    /usr/bin/python3 /opt/mk-os/mk-desktop.py
    if [ $? -eq 0 ]; then break; fi
    sleep 1
done
