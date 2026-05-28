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

def sg2db_prams(sg):
       tmp = re.sub(r'SampleGeometry\(', '', str(sg))
       tmp = re.sub(r'\)', '', tmp)
       return re.sub(r' ', '', tmp)

# @@todo: surely p4p does it better..

def getSampleGeometry(peer):
    my_env = os.environ.copy()
    my_env["EPICS_PVA_NAME_SERVERS"] = peer.ip
#    print(my_env)
    print(f"getSampleGeometry EPICS_PVA_NAME_SERVERS={peer.ip} ./scripts/pvxget_value {peer.name}:SMPL")
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
    pm_cycles = int(os.getenv("XRMSLICE_PM_CYCLES", 20))
    geo = args.geometries[ii]
    for CYCLE in range(pm_cycles):
        SPORT =  SPORT = f'XRM{ii}PM{CYCLE:02d}'
        cyc = "{:02d}".format(CYCLE)
        hupc = f"HOST={args.host},UUT={peer.name},PORT={SPORT},CYCLE={cyc},TIMEOUT=10"
        port_count = geo.AI_COUNT

        args.fp.write(f"""
xrmSlice_PM_Configure("{SPORT}", {port_count})""")
# common
        if CYCLE == 0:
                aip1 = f"AICOUNT1={geo.AI_COUNT+1}"
                args.fp.write(f"""
dbLoadRecords("./db/xrmSliceCommon.db", "{hupc},{sg2db_prams(geo)},{aip1}")""")

# blob
        pmbn = f"PM_BUF_NELM={geo.SSB*geo.NSAM//4}"
        args.fp.write(f"""
dbLoadRecords("./db/xrmSlice_PM.db", "{hupc},{pmbn},ADDR=0,NSAM={geo.NSAM-1}")""")

# slices
        hupcn = f"{hupc},NSAM={geo.NSAM-1}"             # index from RAW[1], skip ES

        expanded_ai_db = f"./db/xrmSliceAI_PM_{args.geometries[ii].AI_COUNT}CH.db"
        if os.path.isfile(expanded_ai_db):
            args.fp.write(f"""
dbLoadRecords("{expanded_ai_db}", "{hupcn}")""")
        else:
            for ix in range(args.geometries[ii].AI_COUNT):
                ch = CHFMT.format(ix+1)
                args.fp.write(f"""
dbLoadRecords("./db/xrmSliceAI_PM.db", "{hupcn},ADDR={ix},CH={ch}")""")

        for ix in range(args.geometries[ii].DI_COUNT):
            args.fp.write(f"""
dbLoadRecords("./db/xrmSliceDI_PM.db", "{hupcn},ADDR={ix},DI={ix}")""")

        if args.geometries[ii].SP_COUNT >= 8:
            args.fp.write(f"""
dbLoadRecords("./db/xrmSliceSP_PM_8.db", "{hupcn}")""")
        else:
            for sp in range(args.geometries[ii].SP_COUNT):
                args.fp.write(f"""
dbLoadRecords("./db/xrmSliceSP_PM.db", "{hupcn},ADDR={sp},SP={sp:02d}")""")

HT_HEADER_SIZE = 24
            
def print_peer_ht(args, ii, peer, CHFMT):
    geo = args.geometries[ii]
    HTROWS = port_count = int(os.getenv("XRMSLICE_HT_ROWS", 64))

    for HTROW in range(port_count):
        row = "{:02d}".format(HTROW)
        SPORT = f'XRM{ii}HT{row}'

        hupc = f"HOST={args.host},UUT={peer.name},PORT={SPORT},ROW={row},TIMEOUT=10"

        args.fp.write(f"""
xrmSlice_HT_Configure("{SPORT}", {port_count})"""
        )

        if HTROW == 0:
                htbn = f"HT_BUF_NELM={(HTROWS*(HT_HEADER_SIZE+geo.SSB)+HT_HEADER_SIZE)//4}"
                args.fp.write(f"""
 dbLoadRecords("./db/xrmSlice_HT.db", "{hupc},ADDR=0,{htbn}")""")

        args.fp.write(f"""
 dbLoadRecords("./db/xrmSlice_HT_HDR.db", "{hupc},ADDR=0")""")
 
# save time (well, st.cmd length at least) using expansion, if available
        expanded_ai_db = f"./db/xrmSliceAI_HT_{args.geometries[ii].AI_COUNT}CH.db"
        if os.path.isfile(expanded_ai_db):
            args.fp.write(f"""
dbLoadRecords("{expanded_ai_db}", "{hupc}")""")
        else:
            for ix in range(args.geometries[ii].AI_COUNT):
                ch = CHFMT.format(ix+1)
                args.fp.write(f"""
dbLoadRecords("./db/xrmSliceAI_HT.db", "{hupc},ADDR={ix},CH={ch}")""")

        for ix in range(args.geometries[ii].DI_COUNT):
            args.fp.write(f"""
dbLoadRecords("./db/xrmSliceDI_HT.db", "{hupc},ADDR={ix},DI={ix}")""")

        if args.geometries[ii].SP_COUNT >= 8:
            args.fp.write(f"""
dbLoadRecords("./db/xrmSliceSP_HT_8.db", "{hupc}")""")
        else:
            for sp in range(args.geometries[ii].SP_COUNT):
                args.fp.write(f"""
dbLoadRecords("./db/xrmSliceSP_HT.db", "{hupc},ADDR={sp},SP={sp:02d}")""")

        args.fp.write(f"""
dbLoadRecords("./db/xrmSliceET_HT.db", "{hupc}")""")


        
def print_peer(args, ii, peer):
    print('@@todo print_peer')
    print(f'peer: {peer.name} {peer.ip}')
    print(f'smpl: {args.geometries[ii]}')
     
    SPORT = f'XRM{ii}'
    args.fp.write(f"""
# peer: {peer.name} {peer.ip}
# smpl: {args.geometries[ii]}
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
