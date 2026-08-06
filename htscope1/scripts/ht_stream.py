#!/usr/bin/env python3

import argparse
import os
import time
import subprocess
import threading

from acq400_hapi import afhba404, factory, acq400_logger, PR, pv
"""
Usage:

    ./ht_stream.py acq1102_010 acq1102_015 --concat=99999 --secs=10

"""

log = None

class DotDict(dict):
    __delattr__ = dict.__delitem__
    __getattr__ = dict.__getitem__
    __setattr__ = dict.__setitem__

class HTS(dict):

    def __init__(self, uuts, map, outroot):

        self.uuts = {}
        self.streams = []

        self.master_uut = uuts[0]

        conns = afhba404.get_connections()
        for conn in conns.values():
            print(f"start {conn}")
            if conn.uut not in uuts: continue

            serial = conn.uut[-3:]
            rport = conn.cx
            lport = conn.dev

            if 'ALL' in map: map_port = map['ALL']
            elif serial in map: map_port = map[serial]
            else: continue

            if 'BOTH' in map_port: sites = map_port['BOTH']
            elif rport in map_port: sites = map_port[rport]
            else: continue

            self.uuts.setdefault(conn.uut, factory(conn.uut))

            uut = self.uuts[conn.uut]

            uut.hostname = conn.uut
            uut.cstate = None
            uut.stop_flag = True

            if sites == 'ALL': sites = uut.get_site_types()["AISITES"]
            else: sites = sites.split(',')

            if not hasattr(uut, 'streams'): uut.streams = {}

            uut.streams[rport] = Stream(uut, rport, lport, sites, outroot)
            self.streams.append(uut.streams[rport])


    def config_host(self, args):
        if not os.path.ismount("/mnt"):
            die(f'Error: /mnt is not a ramdisk')

        if args.delete:
            log.warning(f'Erasing /mnt/afhba.*')
            cmd = 'rm  -rf /mnt/afhba.*'
            os.system(cmd)

        if args.secs:
            args.nbuffers = 9999999999

    def config_uuts(self, args):
        for name, uut in self.uuts.items():
            log.info(f"Configuring {name}")

            if args.spad:
                log.info(f"{name} spad = {args.spad}")
                uut.s0.spad = args.spad

            if args.RTM_TRANSLEN:
                log.info(f"{name} RTM_TRANSLEN = {args.RTM_TRANSLEN}")
                uut.s1.RTM_TRANSLEN = self.args.RTM_TRANSLEN

            run0_cmd = f'{uut.s0.sites} {uut.s0.spad}'
            log.debug(f"{name} run0 = {run0_cmd}")
            uut.s0.run0 = run0_cmd

            uut.spadlen = int(uut.s0.spad.split(',')[1])

            for rport, stream in uut.streams.items():
                stream.setup_aggregators()


    def init_streams(self, args):
        log.info('init_streams')

        for stream in self.streams:
            stream.init_stream(args)

    def start_uut(self, uut):
        log.debug('start_uut')
        self.get_cstate(uut)
        if uut.cstate != 'IDLE':
            uut.s0.streamtonowhered = "stop"
            time.sleep(2)
        uut.s0.streamtonowhered = "start"

    def get_cstate(self, uut):
        log.debug('get_cstate')
        uut.cstate = pv(uut.s0.CONTINUOUS_STATE)

    def start_uuts(self):
        log.debug('start_uuts')

        @background_task
        def update_wrapper(uut):
            while True:
                self.get_cstate(uut)
                time.sleep(1)

        for uut in self.uuts.values():
            log.debug(f"Starting {uut.uut}")
            uut.stop_flag = False
            uut.update_th = update_wrapper(uut)
            if uut.hostname != self.master_uut: self.start_uut(uut)
        
        while not self.state_all('ARM', [self.master_uut]):
            log.info("waiting for slave arm")
            time.sleep(1)

        self.start_uut(self.uuts[self.master_uut])

        time.sleep(1)

    def stop_uuts(self):
        log.debug('stop_uuts')

        @background_task
        def wrapper(uut):
            log.debug(f'{uut.uut} Stopping')
            uut.stop_flag = True
            while uut.cstate != 'IDLE':
                uut.s0.streamtonowhered = "stop"
                time.sleep(2)

        for uut in self.uuts.values():
            if uut.stop_flag: continue
            uut.stop_th = wrapper(uut)

    def setup_trigger(self, args):
        pass # TODO

    def trigger_uuts(self):
        pass # TODO

    def state_all(self, state='IDLE', exclude=[]):
        log.debug('state_all')
        for uut in self.uuts.values():
            if uut.hostname in exclude: continue
            if uut.cstate != state: return False
        return True

    def state_any(self, state='IDLE'):
        log.debug('state_any')
        for uut in self.uuts.values():
            if uut.cstate == state: return True
        return False

    def all_ended(self):
        log.debug('all_ended')
        for stream in self.streams:
            if stream.state.STATUS != 'STOP_DONE': return False
        return True

    def uut_cstate(self):
        log.debug('uut_cstate')
        return [self.uuts[name].cstate for name in self.uuts]


class Stream():

    def __init__(self, uut, rport, lport, sites, outroot):
        self.uut = uut
        self.rport = rport
        self.lport = lport
        self.sites = sites

        self.state = None
        self.sites_str = ','.join(map(str, sites))

        env = {
            "lport": self.lport,
            "rport": self.rport,
            "uut": self.uut.uut,
        }
        self.outroot = outroot.format(**env)
        self.logfile = f"{self.outroot}/isramp.log"
        self.bl = afhba404.get_buffer_len(self.lport)
        self.bl_MB = self.bl >> 20

        self.kill_if_active()
        log.debug(f"Stream {self.uut.uut}({self.sites_str}):[{self.rport}] --> [{self.lport}]:{self.outroot}")

    def kill_if_active(self):
        """Ensure no existing stream is running on port"""
        log.debug('kill_if_active')
        pid = afhba404.get_stream_pid(self.lport)
        if pid == 0: return
        log.warning(f'Killing afhba.{self.lport} with pid: {pid}')
        cmd = 'kill -9 {}'.format(pid)
        os.system(cmd)
        time.sleep(1)
        pid = afhba404.get_stream_pid(self.lport)
        if pid == 0: return
        die(f'Stream {self.lport} failed to die')

    def setup_aggregators(self):
        log.debug('setup_aggregators')
        agg_str = f'sites={self.sites_str} on'
        self.uut[self.rport].aggregator = agg_str
        self.uut[self.rport].spad = bool(self.uut.spadlen)

        log.debug(f"Stream {self.uut.uut} {self.rport} {agg_str} / spad {self.uut.spadlen}")

    def get_state(self):
        return afhba404.get_stream_state(self.lport)

    def init_stream(self, args):
        log.debug('init_stream')

        @background_task
        def update_wrapper():
            while True:
                self.state = self.get_state()
                time.sleep(1)

        self.update_th = update_wrapper()

        ssb = 0
        for site in self.sites:
            nchan = int(self.uut[site].NCHAN)
            datalen = 4 if int(self.uut[site].data32) else 2
            ssb += (nchan * datalen)
        ssb += (self.uut.spadlen * 4)
        ssl = ssb // 4 # sample size in longs

        env = {
            'lport' : self.lport,
            'nbuffers': args.nbuffers,
            'concat': args.concat,
            'recycle': args.recycle,
            'outroot': self.outroot,
            'ssl': ssl,
            'spadstart': ssl - self.uut.spadlen,
            'step': 1, #TODO attach to args.decimate
            'logfile': self.logfile,
            'stream_prog': args.stream_prog,
            'check_prog': args.check_prog,
        }

        stream_cmd = "RTM_DEVNUM={lport} NBUFS={nbuffers} CONCAT={concat} RECYCLE={recycle} OUTROOT={outroot} {stream_prog}"
        check_cmd = " | {check_prog} -N1 -m {ssl} -c {spadstart} -s {step} -i 1 -L {logfile}"

        cmd = stream_cmd.format(**env)
        if args.check_spad:
            cmd += check_cmd.format(**env)

        os.umask(0o000)
        os.makedirs(self.outroot, mode=0o777, exist_ok=True)

        log.debug(f"{self.uut.uut}[{self.rport}] stream cmd: {cmd}")
        self.process = subprocess.Popen(cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        t0 = time.time()
        self.pid = afhba404.get_stream_pid(self.lport)
        while True:
            if self.pid > 0:
                PR.Green(f'Started afhba.{self.lport} with PID {self.pid}')
                break
            if time.time() - t0 > 5:
                die(f'Error: afhba.{self.lport} failed to start')
            self.pid = afhba404.get_stream_pid(self.lport)
            time.sleep(0.5)

def run_main(args):
    print(args)
    global log
    log = acq400_logger('ht_stream', level=args.loglevel, logfile='ht_stream.log')

    log.debug(args)
    hts = HTS(args.uuts, args.map, args.outroot)
    log.debug("configging host")
    hts.config_host(args)
    log.debug("configging uuts")
    hts.config_uuts(args)
    log.debug("initting streams")
    hts.init_streams(args)
    log.debug("starting uuts")
    hts.start_uuts()

    t0 = 0
    exit_code = 0
    stopping = False

    try:
        while True:

            if t0 == 0 and hts.state_any('RUN'):
                t0 = time.time()

            if not stopping:
                t1 = int(min(time.time() - t0, t0))

            for uut in hts.uuts.values():

                for stream in uut.streams.values():
                    rate = stream.state.rx_rate * stream.bl_MB
                    total = stream.state.rx * stream.bl_MB
                    print(f"runtime={t1} uut={uut.uut} cstate={uut.cstate} rport={stream.rport} lport={stream.lport} rate={rate} total={total}")


            if args.secs and t1 > args.secs:
                if not stopping:
                    print('Time Limit Reached Stopping')
                    hts.stop_uuts()
                    stopping = True
                if hts.state_all('IDLE'): break

            if hts.all_ended():
                if not stopping:
                    print('Buffer limit Reached Stopping')
                    hts.stop_uuts()
                    stopping = True
                if hts.state_all('IDLE'): break

            time.sleep(1)

    except KeyboardInterrupt:
         log.info("Interrupt!")

    except Exception as e:
        log.info("exception has been called")
        log.error(e)
        exit_code = 1

    hts.stop_uuts()
    while not hts.state_all('IDLE'):
        print("wait for stop")
        time.sleep(1)

    log.info('Done')
    exit(exit_code)

def background_task(func):
    """Runs decorated function in the background returns thread"""
    def wrapper(*args, **kwargs):
        thread = threading.Thread(target=func, args=[*args], kwargs=kwargs)
        thread.daemon = True
        thread.start()
        return thread
    return wrapper

def die(message):
    log.error(message)
    os._exit(1)

def map_type(arg):
    mapdef = {}
    for mapval in arg.upper().split('/'):
        host, rport, chans = mapval.split(':')
        mapdef.setdefault(host, {})
        mapdef[host].update({rport: chans})
    return mapdef

def get_parser():
    parser = argparse.ArgumentParser(description='High Throughput Stream from up to 16 UUTS')

    parser.add_argument('--spad', default=None, help="scratchpad, eg 1,16,0")
    parser.add_argument('--RTM_TRANSLEN', default=None, help='Set burst length (RTM_TRANSLEN) for each uut')

    parser.add_argument('--map', default="ALL:BOTH:ALL", type=map_type, help='uut:port:site ie --map=67:A:1/67:B:2/130:BOTH:ALL ')

    parser.add_argument('--nbuffers', default=5000, type=int, help='max capture in buffers')
    parser.add_argument('--secs', default=None, type=int, help="max capture in seconds")

    parser.add_argument('--recycle', default=1, type=int, help='overwrite data')
    parser.add_argument('--delete', default=1, type=int, help='delete /mnt/afhba.* ')
    parser.add_argument('--concat', default=0, type=int, help='concatenate buffers')

    parser.add_argument('--loglevel', default=9, type=int, help="loglevel (<= 10 for debug)")

    parser.add_argument('--check_spad', default=0, type=int, help='check spad is sequential')

    parser.add_argument('--stream_prog', default="~/PROJECTS/AFHBA404/STREAM/rtm-t-stream-disk", help='path to stream program')
    parser.add_argument('--check_prog', default="~/PROJECTS/AFHBA404/FUNCTIONAL_TESTS/isramp", help='path to check program')
    parser.add_argument('--outroot', default="/mnt/afhba.{lport}/{uut}", help='path to save data')

    parser.add_argument('uuts', nargs='+', help="uut hostnames")

    return parser

if __name__ == '__main__':
    run_main(get_parser().parse_args())
