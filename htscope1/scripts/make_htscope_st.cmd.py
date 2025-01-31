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
    args.fp.write("""
dbLoadDatabase("./dbd/htscope1.dbd")
htscope1_registerRecordDeviceDriver(pdbbase)

# Turn on asynTraceFlow and asynTraceError for global trace, i.e. no connected asynUser.
#asynSetTraceMask("", 0, 17)
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

next_link = _hands_out_outlinks()

def print_uut(uut, args):
    print(f'print_uut {uut}')
    wsize=4 if args.data32 == 1 else 2
    args.fp.write(f"\n# uut {uut}\n")
    args.fp.write(f"""
multiChannelScopeConfigure("{args.prefix}{uut}", {args.nchan}, {args.ndata}, {wsize})
    """)
    tm = "TIMEOUT=0"
    uutdb="./db/htscope1.db"
    args.fp.write(f"""
dbLoadRecords("{uutdb}","HOST={args.host_ioc},PFX={args.prefix},UUT={uut},{tm},RUNFAN={next_link()}")
""")
    chdb = "./db/htscope1_ch.db"
    for ix in range(args.nchan):
        ch = f"{ix+1:02}"
        args.fp.write(f"""
dbLoadRecords("{chdb}","HOST={args.host_ioc},PFX={args.prefix},UUT={uut},CH={ch},IX={ix},{tm},NPOINTS={args.ndata}")""")
    args.fp.write(f"""
asynSetTraceMask("{args.prefix}{uut}",0,0xff))
    """)

def print_postamble(args):
    args.fp.write("\n# postamble\n")
    maindb = "./db/htscope1_main.db"
    uuts= ','.join(args.uuts)
    args.fp.write(f"""
dbLoadRecords("{maindb}","HOST={args.host_ioc},UUTS=\'{uuts}\'")
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
    args.host_ioc=args.prefix.split(':')[0] + ':'

def run_main(args):
    init(args)
    print_preamble(args)
    for uut in args.uuts:
        print_uut(uut, args)

    if args.user2 is not None:
        for user2 in args.user2.split(','):
            args.prefix = args.host_ioc + user2 + ':'
            for uut in args.uuts:
                print_uut(uut, args)

    print_postamble(args)

def default_prefix():
    return f'{socket.gethostname().split(".")[0]}:{os.environ.get("USER")}:'

def get_parser():
    parser = argparse.ArgumentParser(description="create htscope epics record definition")
    parser.add_argument('--output', '-O', default="st.cmd", help="record definition file name [st.cmd]")
    parser.add_argument('--nchan', default=None, type=int, help="specify number of channels (or use hapi to automate)")
    parser.add_argument('--data32', default=None, type=int, help="set to 1 for d32 data (or use hapi to automate)")
    parser.add_argument('--ndata',  default=100000, type=int, help="number of data elements in WF")
    parser.add_argument('--prefix', default=default_prefix(), type=str, help='prefix for PV\'s, default="$(hostname):$USER"')
    parser.add_argument('--user2',   default=None, help='specify a second user, use existing prefix')
    parser.add_argument('uuts', nargs='+', help="uut1[, uut2...]")
    return parser

if __name__ == '__main__':
    run_main(get_parser().parse_args())
