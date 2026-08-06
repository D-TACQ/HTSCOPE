#!/usr/bin/env python3

import epics
import socket
import time

import subprocess
import shlex
import select
import signal

# https://www.thecodingforums.com/threads/can-read-be-non-blocking.643344/

run_request = 0


def run_process_with_live_output(command):
    pp = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    # universal_newlines=True, bufsize=1)

    while True:
        rc = select.select([pp.stdout.fileno()], [], [], 1)
        if len(rc[0]) > 0:
            output = pp.stdout.readline()
            if not output:
                break
            output = output.strip()
            print(output)

            if output.startswith("runtime"):
                state = dict(item.split("=") for item in output.split())
                state['host'] = HOST
                pvname = "{host}:{uut}:STREAM:STATUS".format(**state)
                pvvalue = "[{uut}][{rport}]({cstate}) -> [{host}][{lport}]({rate}MB/s | {total}MB)".format(**state)
                epics.caput(pvname, pvvalue)
                print(output)

        if run_request != 1:
            print(f'STOP requested')
            pp.send_signal(signal.SIGINT)
            pp.wait()

    return pp.wait()


# get HOST:USER for local ioc

HOST = socket.gethostname().split(".")[0]


def onChange(pvname=None, value=None, char_value=None, **kwargs):
    global run_request
    run_request = value

print(f'HOST {HOST}')

UUTS = [epics.caget(pv) for pv in [f'{HOST}:UUT1', f'{HOST}:UUT2', f'{HOST}:UUT3', f'{HOST}:UUT4']]
UUTS = ' '.join(UUTS).strip()

RUNSTOP = epics.get_pv(f'{HOST}:RUNSTOP', callback=onChange)
SHOT_TIME = epics.get_pv(f'{HOST}:SHOT_TIME')
STATUS = epics.get_pv(f'{HOST}:STATUS')


print(f'RUNSTOP: {RUNSTOP.get()} UUTS: {UUTS} SHOT_TIME: {SHOT_TIME.get()}')

loopcount = 0
shot = 0

while True:
    loopcount += 1
    if run_request == 1:
        shot += 1
        STATUS.put(f'run_request shot {shot}')
        print(f'run_request shot {shot}')
        secs = int(SHOT_TIME.get())
        job = f'unbuffer ./scripts/ht_stream.py --concat=999999 --secs={secs} {UUTS}'
        STATUS.put(f'run job {shot}')
        print(f'run job {shot} {job}')
        rc = run_process_with_live_output(job)
        STATUS.put(f'htstream complete rc {rc}')
        print(f'htstream complete rc {rc}')
        RUNSTOP.put(0)
    else:
        if loopcount % 30 == 0:
            print(f'waiting {loopcount}')

    time.sleep(0.5)

