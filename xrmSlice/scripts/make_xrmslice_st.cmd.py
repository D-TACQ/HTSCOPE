#!/usr/bin/env python3

import argparse
import sys
import socket
import os
import subprocess
import re

from typing import NamedTuple

# typed languages rool ok!

class NameAndAddress(NamedTuple):
    name: str
    ip: str
    
class SampleGeometry(NamedTuple):
    AI_COUNT: int
    AI_INDEX: int
    DI_COUNT: int
    DI_INDEX: int
    SP_COUNT: int
    SP_INDEX: int
    SSB: int
    NSAM: int

# AI_COUNT.value int32_t = 32

pvxget_value_pattern = re.compile("^[ ]*([A-Z_]+).value.*= ([0-9]+)")

# @@todo: surely p4p does it better..

def getSampleGeometry(peer):
    my_env = os.environ.copy()
    my_env["EPICS_PVA_NAME_SERVERS"] = peer.ip
#    print(my_env)
    process = subprocess.Popen(
        ["./scripts/pvxget_value", f"{peer.name}:SMPL"], 
        env=my_env, stdout=subprocess.PIPE, text=True)
        
    lines = process.communicate()[0].strip().split('\n')
    if process.returncode != 0:
        print(f"ERROR: pvxget_value failed {process.returncode}")
        exit(1)

    fields = {}
    for line in lines:
        m = pvxget_value_pattern.match(line)
        if m:
            fields[m.group(1)] = int(m.group(2))
    sg = SampleGeometry(**fields)
    print(f"output: {sg}")
    return sg
    
def print_preamble(args):
    if args.output == '-':
        args.fp = sys.stdout
    else:
        args.fp = open(args.output, "w")
    args.fp.write("# preamble\n")
    args.fp.write(f'# command\n#{" ".join(sys.argv)}')
    args.fp.write(f"""
dbLoadDatabase("./dbd/xrmSlice.dbd")
xrmSlice_registerRecordDeviceDriver(pdbbase)

""")

def print_peer_pm(args, ii, peer, CHFMT):
    for CYCLE in range(20):
        SPORT =  SPORT = f'XRM{ii}PM{CYCLE:02d}'
        args.fp.write(f"""
xrmSlice_PM_Configure(f"{SPORT}", args.geometries[ii].AI_COUNT)""")

        for ix in range(args.geometries[ii].AI_COUNT):
            ch = CHFMT.format(ix+1)
            args.fp.write(f"""
dbLoadRecords("./db/xrmSliceAI_PM.db", "HOST={args.host},UUT={peer.name},PORT={SPORT},ADDR={ix},CH={ch}")""")

        for ix in range(args.geometries[ii].DI_COUNT):
            args.fp.write(f"""
dbLoadRecords("./db/xrmSliceDI_PM.db", "HOST={args.host},UUT={peer.name},PORT={SPORT},ADDR={ix},CH={ix+1}")""")

        args.fp.write(f"""
dbLoadRecords("./db/xrmSliceSP_PM.db", "HOST={args.host},UUT={peer.name},PORT={SPORT}")""")


            
def print_peer_ht(args, ii, peer, CHFMT):
    for HTROW in range(64):
        SPORT =  SPORT = f'XRM{ii}HT{HTROW:02d}'
        args.fp.write(f"""
xrmSlice_PM_Configure(f"{SPORT}", args.geometries[ii].AI_COUNT)""")

        for ix in range(args.geometries[ii].AI_COUNT):
            ch = CHFMT.format(ix+1)
            args.fp.write(f"""
dbLoadRecords("./db/xrmSliceAI_HT.db", "HOST={args.host},UUT={peer.name},PORT={SPORT},ADDR={ix},CH={ch}")""")

        for ix in range(args.geometries[ii].DI_COUNT):
            args.fp.write(f"""
dbLoadRecords("./db/xrmSliceDI_HT.db", "HOST={args.host},UUT={peer.name},PORT={SPORT},ADDR={ix},CH={ix+1}")""")

        args.fp.write(f"""
dbLoadRecords("./db/xrmSliceSP_HT.db", "HOST={args.host},UUT={peer.name},PORT={SPORT}")""")

        
def print_peer(args, ii, peer):
    print('@@todo print_peer')
    print(f'peer: {peer.name} {peer.ip}')
    print(f'smpl: {args.geometries[ii]}')
     
    SPORT = f'XRM{ii}'
    args.fp.write(f"""    
# Turn on asynTraceFlow and asynTraceError for global trace, i.e. no connected asynUser.
#asynSetTraceMask("", 0, 17)
drvAsynIPPortConfigure("{SPORT}", "{peer.ip}")
dbLoadRecords("db/asynRecord.db", "P={args.host}:,R={SPORT},PORT={SPORT},ADDR=0,IMAX=100,OMAX=100,TB3=0,TIB0=0")
epicsEnvSet("STREAM_PROTOCOL_PATH","./protocols")
dbLoadRecords("./db/xrmSlice.db", "HOST={args.host},UUT={peer.name}")
""")

    if args.geometries[ii].AI_COUNT > 99:
        CHFMT = '{:03d}'
    else:
        CHFMT = '{:02d}'
        
    print_peer_pm(args, ii, peer, CHFMT)
    print_peer_ht(args, ii, peer, CHFMT)


def print_postamble(args):
    args.fp.write("\n# postamble\n")
    args.fp.write("iocInit()\n")
    args.fp.write("dbl > records.dbl\n")
    args.fp.write("# end\n")
    args.fp.close()

def isValidGeometry(geometry):
    return geometry.AI_COUNT > 0 or geometry.DI_COUNT > 0

def init(args):
    args.peers = []
    args.geometries = []
    for uut in args.uuts:
        try:
            name, ip = uut.split(',')
        except ValueError:
            name = uut
            ip = uut
        name_and_address = NameAndAddress(name, ip)
        geometry = getSampleGeometry(name_and_address)
        
        if isValidGeometry(geometry):
            args.peers.append(name_and_address)
            args.geometries.append(geometry)
        else:
            print(f"ERROR PEER {name_and_address.name} does NOT have valid geometry")



def run_main(args):
    init(args)
    print_preamble(args)
    for ii, peer in enumerate(args.peers):
        print_peer(args, ii, peer)
    print_postamble(args)

def default_host():
    return f'{socket.gethostname().split(".")[0]}'

def default_user():
    return f'{os.environ.get("USER")}'

def get_parser():
    parser = argparse.ArgumentParser(description="create htscope epics record definition")
    parser.add_argument('--output', '-O', default="st.cmd", help="record definition file name [st.cmd]")    
    parser.add_argument('--host', default=default_host(), type=str, help='prefix for PV\'s, default="$(hostname)"')
    parser.add_argument('--user',   default=default_user(), help='one or more users (must be at least one) eg --user=tom,dick,harry default="$USER"')
    parser.add_argument('uuts', nargs='+', help="uut1[ uut2...]")
    return parser

if __name__ == '__main__':
    run_main(get_parser().parse_args())
