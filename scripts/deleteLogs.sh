#!/bin/bash

if [ "$1" -eq 2 ]
    then
        rm /home/debian/logs/*.log
    else
        if [ $(ls -dp /home/debian/logs/*.log | wc -l) -gt 7 ]
          then
          ls -dp -1t /home/debian/logs/*.log | tail -n +5 | xargs rm
        fi
fi
