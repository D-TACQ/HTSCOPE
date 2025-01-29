#!/usr/bin/env python3

import epics
import socket
import time
import os

# get HOST:USER for local ioc

PFX =  f'{socket.gethostname().split(".")[0]}:'

run_request = 0

def onChange(pvname=None, value=None, char_value=None, **kwargs):
    global run_request
    run_request = value

print(f'PFX {PFX}')

RUNSTOP = epics.get_pv(f'{PFX}RUNSTOP', callback=onChange)
UUTS = epics.get_pv(f'{PFX}UUTS')
SHOT_TIME = epics.get_pv(f'{PFX}SHOT_TIME')
STATUS = epics.get_pv(f'{PFX}STATUS')


print(f'RUNSTOP: {RUNSTOP.get()} UUTS: {UUTS.get()} SHOT_TIME: {SHOT_TIME.get()}')

loopcount = 0
shot = 0

while True:
    if run_request == 1:
        shot += 1;
        STATUS.put(f'run_request shot {shot}')
        print(f'run_request shot {shot}')
        uuts = ' '.join(UUTS.get().split(','))
        secs = int(SHOT_TIME.get())
        job = f'./scripts/ht_stream.py --concat=999999 --secs={secs} {uuts}'
        print(f'use os.system {job}')
        rc = os.system(job)
        STATUS.put(f'job complete status: {rc}')
        print(f'job complete status: {rc}')
        RUNSTOP.put(0)

    time.sleep(0.1)
    loopcount += 1
    if loopcount%10 == 1:
        print(f'waiting')

