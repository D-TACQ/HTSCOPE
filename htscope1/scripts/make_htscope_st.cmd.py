#!/usr/bin/env python3

import argparse
import sys
import socket
import os

def print_preamble(args):
    if args.output == '-':
        args.fp = sys.stdout
    else:
        args.fp = open(args.output, "w")
    args.fp.write("# preamble\n")
    args.fp.write(f'# command\n#{" ".join(sys.argv)}')
    args.fp.write(f"""
< envPaths
dbLoadDatabase("$(TOP)/dbd/htscope1.dbd")
htscope1_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("DB_TOP", "$(TOP)/db")
# Turn on asynTraceFlow and asynTraceError for global trace, i.e. no connected asynUser.
#asynSetTraceMask("", 0, 17)
drvAsynIPPortConfigure("HTS", "127.0.0.1:8843")
dbLoadRecords("db/asynRecord.db","P={args.host}:,R=asyn:HTS,PORT=HTS,ADDR=0,IMAX=100,OMAX=100,TB3=0,TIB0=0")
epicsEnvSet("STREAM_PROTOCOL_PATH","$(TOP)/protocols")
dbLoadRecords("$(DB_TOP)/hts_wrapper.db","HOST={args.host},SPORT=HTS")
""")

def _hands_out_outlinks():
#    the_links = ( 'OUTA', 'OUTB', 'OUTC', 'OUTD', 'OUTE', 'OUTF', 'OUTG', 'OUTH' )
    the_links = ( 'LNK0', 'LNK1', 'LNK2', 'LNK3', 'LNK4', 'LNK5', 'LNK6', 'LNK7', 
                  'LNK8', 'LNK9', 'LNKA', 'LNKB', 'LNKC', 'LNKD', 'LNKE', 'LNKF'  )
    cursor = 0
    def _func():
        nonlocal cursor
        try:
            link = the_links[cursor]
            cursor += 1
            return link
        except IndexError as e:
            print(f'ERROR: we have maximum 8 links in dfanout eg 4UUTS * 2USERS. For large system, we need a pyramid')
            raise e
    return _func

global_link = _hands_out_outlinks()
user_link = None

def print_cal(uut, user, args):
    """ for each uut, get SITES, iterate sites creating a remote cal anchor record (dummy),
        then spawn a SM to copy data to channel record on demand.
    """
    pass

def print_uut(uut, user, ufan, args):
    print(f'print_uut {uut}')
    wsize=4 if args.data32 == 1 else 2
    args.fp.write(f"\n# uut {uut}\n")
    args.fp.write(f"""
multiChannelScopeConfigure("{args.host}:{user}:{uut}", {args.nchan}, {args.ndata}, {wsize})
    """)
    tm = "TIMEOUT=0"
    uutdb="$(DB_TOP)/htscope1.db"
    args.fp.write(f"""
dbLoadRecords("{uutdb}","HOST={args.host},USER={user},UUT={uut},{tm},GFAN={global_link()},UFAN={ufan},NCHAN={args.nchan}")
""")
    chdb = "$(DB_TOP)/htscope1_ch.db"
    for ix in range(args.nchan):
        ch = f"{ix+1:02}"
        args.fp.write(f"""
dbLoadRecords("{chdb}","HOST={args.host},USER={user},UUT={uut},CH={ch},IX={ix},{tm},NPOINTS={args.ndata}")
""")

    print_cal(uut, user, args)
    args.fp.write(f"""
asynSetTraceMask("{args.host}:{user}:{uut}",0,0xff))
    """)

def print_postamble(args):
    args.fp.write("\n# postamble\n")
    maindb = "$(DB_TOP)/htscope1_main.db"
    uuts = ["", "", "", ""]
    uuts[:len(args.uuts)] = args.uuts
    uuts = ','.join(f'UUT{i+1}={value}' for i, value in enumerate(uuts))
    args.fp.write(f"""
dbLoadRecords("{maindb}","HOST={args.host},{uuts}")
""")
    args.fp.write("iocInit()\n")
    args.fp.write("dbl > records.dbl\n")
    args.fp.write("# end\n")
    args.fp.close()

def init(args):
    if args.nchan is None:
        print("@@todo gather nchan: we need HAPI for this")
    if args.data32 is None:
        print("@@todo gather data32: we need HAPI for this")

def run_main(args):
    global user_link
    init(args)
    print_preamble(args)
    for user in args.user.split(','):    
        user_link = _hands_out_outlinks()
        for uut in args.uuts:    
            print_uut(uut, user, user_link(), args)

    print_postamble(args)

def default_host():
    return f'{socket.gethostname().split(".")[0]}'

def default_user():
    return f'{os.environ.get("USER")}'

def get_parser():
    parser = argparse.ArgumentParser(description="create htscope epics record definition")
    parser.add_argument('--output', '-O', default="st.cmd", help="record definition file name [st.cmd]")
    parser.add_argument('--nchan', default=None, type=int, help="specify number of channels (or use hapi to automate)")
    parser.add_argument('--data32', default=None, type=int, help="set to 1 for d32 data (or use hapi to automate)")
    parser.add_argument('--ndata',  default=100000, type=int, help="number of data elements in WF")
    parser.add_argument('--host', default=default_host(), type=str, help='prefix for PV\'s, default="$(hostname)"')
    parser.add_argument('--user',   default=default_user(), help='one or more users (must be at least one) eg --user=tom,dick,harry default="$USER"')
    parser.add_argument('uuts', nargs='+', help="uut1[ uut2...]")
    return parser

if __name__ == '__main__':
    run_main(get_parser().parse_args())
