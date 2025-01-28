#!/usr/bin/env python3

import os
import time
import epics
import argparse

"""
Usage:
    ./ht_stream_launcher.py kamino
"""

def run_main(args):
    while True:

        print('Waiting for RUNSTOP')
        while True:
            value = caget(f"{args.prefix}:RUNSTOP")
            if value == 1:
                break
            time.sleep(1)

        print('RUNSTOP signal received')

        env = {
            "concat": 99999,
            "stream_prog": args.stream_prog,
            "secs": caget(f"{args.prefix}:SHOT_TIME"),
            "uuts": caget(f"{args.prefix}:UUTS"),
        }
        cmd_def = "{stream_prog} --secs={secs} --concat={concat} {uuts}"
        cmd = cmd_def.format(**env)

        print(f"Running cmd: {cmd}")
        #os.system(cmd)
        epics.caput(f"{args.prefix}:RUNSTOP", 0) # Set RUNSTOP to false

def caget(pvname):
    try:
        pv = epics.PV(pvname)
        if not pv.connected: raise ValueError(f"Failed to connect to PV: {pvname}")

        value = pv.get(timeout=2.0)
        if value is None: raise ValueError(f"Failed to retrieve value for PV: {pvname}")

        return value

    except Exception as e:
        exit(f"Error: {e}")

def get_parser():
    parser = argparse.ArgumentParser(description='HT Stream launcher')

    parser.add_argument('--stream_prog', default="~/PROJECTS/HTSCOPE/htscope1/scripts/ht_stream.py", help='Stream command path')
    parser.add_argument('prefix', help="host prefix")

    return parser

if __name__ == '__main__':
    run_main(get_parser().parse_args())
