# HTSCOPE : Interactive Scope display for HTS data store in HOST ramdisk

The plan is to have an local IOC that maps the data to WF records, and to display this data in 
cs-studio (phoebus and classic)

Data set size:
16ch x 2b x 2M x 10s = 6.4GB,  maybe 4 devices so 24GB.

We know from experience that cs-studio can display 100k points.
Let's assume that each WF record will span 100k points.

To browse the data rapidly, we have two controls:

1. Zoom  aka stride. 10s data is 2M points.
 - Z0: STRIDE=200              # SPAN=10s 2M / 200 = 100k
 - Z1: STRIDE=100              # SPAN= 5s
 - Z2: STRIDE= 50              # SPAN= 2.5s
 - Z3: STRIDE= 20              # SPAN= 1s
 - Z4: STRIDE= 10              # SPAN= 0.5s
 - Z5: STRIDE=  5              # SPAN= 0.25s
 - Z6: STRIDE=  2              # SPAN= 0.1s
 - Z7: STRIDE=  1              # SPAN= 0.05s

2. PAN  : increment/decrement DELAY
 - Moves start of data SPAN/2 to the right.



## Build

### Install packages

* compiler 
* apt-get install expect     # unbuffer .. used in scripting
* apt-get install pyepics

### Build Detail

1. EPICS_BASE

We assume there is already a build of EPICS BASE, ASYN, STREAMDEVICE, SNC
Peter used EPICS7 from ACQ400_ESW_TOP

```
https://github.com/D-TACQ/ACQ400_ESW_TOP
```

Not because we want to run this on a ZYNQ (well, it's an idea!), but because it saved time with a pre-cooked build environment. This ends up with the full BASE under /use/local/epics/base, with an $EPICS_BASE pre-defined.

For general users who do NOT want the ZYNQ build, change CONFIG_SITE as follows:

Ref: [BUILD_EPICS_FROM_SCRATCH.md](./BUILD_EPICS_FROM_SCRATCH.md)

2. Build this project
```
mkdir PROJECTS; cd PROJECTS
git clone https://github.com/D-TACQ/HTSCOPE
cd HTSCOPE; source /usr/local/epics/base/setup.env; make
```

3. Make Data Sources

AFHBA404/HTSTREAM will store RT data to a ramdisk, typically

```
/mnt/afhba.0/acq1102_010/000000/0.00
```

We want to keep HTSCOPE away from this complexity, by using top level links in the user directory
Our HTSCOPE user follows D-TACQ tradition as "dt100". There can also be any number of secondary "users" who share views of the data. The secondary users don't have to be "users" in the Unix sense. Our host is "kamino":

Showing links in $HOME to the data for two UUT's

```
[dt100@kamino ~]$ls -lt | grep kamino | cut -c 50-
kamino:dt100:acq1102_010 -> /mnt/afhba.0/acq1102_010/000000/0.00
kamino:dt100:acq1102_015 -> /mnt/afhba.2/acq1102_015/000000/2.00
```

Showing links for an additional two "secondary users", "mike" and "fred"

```
kamino:mike:acq1102_015 -> kamino:dt100:acq1102_015
kamino:fred:acq1102_015 -> kamino:dt100:acq1102_015

kamino:mike:acq1102_010 -> kamino:dt100:acq1102_010
kamino:fred:acq1102_010 -> kamino:dt100:acq1102_010
```

We have a convenience script to do this automatically for ALL connected hosts
```
./scripts/create_symlinks.sh $USER [$USER2..]
```
example
```
[dt100@kamino htscope1 (main)]$./scripts/create_symlinks.sh $USER
/home/dt100/kamino:dt100:acq1102_010 --> /mnt/afhba.0/acq1102_010/000000/0.00
/home/dt100/kamino:dt100:acq1102_047 --> /mnt/afhba.1/acq1102_047/000000/1.00
```
Alternatively, we can make test data to use instead of real data..

3.1 Make Test data, at $HOME:

```
ramp  800000000 1 4 1 > Downloads/ramp800M-1-4-1
ramp 1600000000 1 2 1 > Downloads/ramp1600M-1-2-1
ramp  100000000 1 2 16 > Downloads/ramp100M-1-2-16
```
for best results:

```
ln -s Downloads/ramp800M-1-4-1 kamino:dt100:acq1102_123
```

4. Make a cs-studio workspace 
.. and add PROJECTS/HTSCOPE/OPI as a project

5. Generate st.cmd
```
cd htscope1
./scripts/make_htscope_st.cmd.py --nchan=16 --data32=0 --ndata=100000 acq1102_123
```

6. Run the IOC
```
cd bin/linux-x86_64
./htscope1 ../../st.cmd
```

Now run the opi set from OPI

Press Refresh for new data
Stride and Start are effective.
For best large span results, disable fast [ODD] channels
for pictures see https://github.com/D-TACQ/HTSCOPE/releases/download/v0.0.1/HTSCOPE-concept.pdf

## Running a composite system:

### db name convention:

```
$(HOST):FROB                   # PV that is global to the HOST eg RUNSTOP
$(HOST):$(USER):FLIP           # PV that is common to all UUT's in USER's view of the HOST. eg $(HOST):$(USER):ZOOM_IN
$(HOST):$(USER):$(UUT):CH:01   # PV with user data view 
```

### Define db
```
pgm@hoy6:~/PROJECTS/HTSCOPE/htscope1$ ./scripts/make_htscope_st.cmd.py --help
usage: make_htscope_st.cmd.py [-h] [--output OUTPUT] [--nchan NCHAN] [--data32 DATA32] [--ndata NDATA] [--host HOST]
                              [--user USER]
                              uuts [uuts ...]

create htscope epics record definition

positional arguments:
  uuts                 uut1[ uut2...]

options:
  -h, --help           show this help message and exit
  --output, -O OUTPUT  record definition file name [st.cmd]
  --nchan NCHAN        specify number of channels (or use hapi to automate)
  --data32 DATA32      set to 1 for d32 data (or use hapi to automate)
  --ndata NDATA        number of data elements in WF
  --host HOST          prefix for PV's, default="$(hostname)"
  --user USER          one or more users (must be at least one) eg --user=tom,dick,harry default="$USER"
```

* HOST:USER:UUT   : Host is the computer name, ideally this is task focussed.
              : USER 
```
eg
magnetics1          host for magnetics experiment. (hostname used by IOC. does not _have_ to be the physical hostname)
magnetics1:adam     adam's personal view (with custom PAN/ZOOM) (does not _have_ to be the actual LINUX USER adam)
magnetics1:bruce    bruce's personable view (with custom PAN/ZOOM)
```

Having an IOC per user isn't really so wasteful, it lets each user control their own view.
Of course, only one user should be allowed to press "RUN/STOP" !



### Toplevel control
htscope1_main.db defines records that are common to the whole system:
```
./scripts/make_htscope_st.cmd.py --nchan=16 acq1102_001 --data32=0 acq1102_002

tail st.cmd
dbloadRecords("./db/htscope1_main.db","PFX=host:user:,UUTS=acq1102_001,acq1102_002")
```

* $(HOST):SHOT_TIME   	:: shot run length in s
* $(HOST):UUTS        	:: list of UUTs in set
* $(HOST):RUNSTOP		:: starts/stops a shot

Glue: we suggest a pyepics wrapper that blocks on RUNSTOP and starts/stops htstream.py as a spawnd task
This is implemented as:

```
run_htstream_wrapper.py
```

### Run the system

Everything runs under procServ:

```
./init/start_servers
```

### cs-studio-phoebus UI
Full support here:

#### Linux client
```
cs-studio/phoebus/
# run this script to start:
run_ht_scope_ui.sh
```

#### windows client
```
# first, customise cs-studio/phoebus/windows/* to suit your configuration then run:
run_ht_scope_ui.bat
```


### cs-studio-classic UI

* OPI set written for cs-studio CLASSIC, not Phoebus, 
* create your own unique WORKSPACE per HOST, link HTSCOPE/OPI as a project.
* Define the following macros in the WORKSPACE (Edit|Preferences|CSS Applications|Display|BOY|OPI Runtime
<pre>
UUT        eg acq1102_015
SITE       1
CHX        1
USER       user       eg dt100:
HOST       host       eg kamino
</pre>
* Set Channel Access params (Edit|Preferences|CSS Core|Data Sources|Channel Access)
<pre>
# may need to set 
Address List : localhost, uncheck  "AutoAddressList"
# definately need to set:
Max Array Size (bytes) : 1000000
</pre>


### procServ
Build and make (.deb didn't work for me..)

```
cd PROJECTS
wget https://launchpad.net/ubuntu/+archive/primary/+sourcefiles/procserv/2.7.0-2/procserv_2.7.0.orig.tar.gz
tar xvzf procserv_2.7.0.orig.tar.gz 
cd procServ-2.7.0
./configure 
make
make install
sudo make install
procServ 
[dt100@kamino ~]$procServ -h
Usage: procServ [options] -P <endpoint>... <command args ...>    (-h for help)
       procServ [options] <endpoint> <command args ...>

kickoff:
./init/start_servers
```

### Service sockets

To view the service consoles
```
./scripts/epics-console
./scripts/htstream-console
```

* start up detail:
```
user@host:~/PROJECTS/HTSCOPE/htscope1$ cat init/start_servers 
#!/bin/bash

# run the IOC on control socket 8841
procServ -L ioc.log -l 8842 -P 8841 ./scripts/run_ioc

# run the htstream wrapper on control socket 8843
procServ -L htstream_wrapper.log -l 8844 -P 8843 ./scripts/run_htstream_wrapper.py
```


## handling multiple users
Multiple users want multiple independent views.
By default the PV prefix is host:user  where user is the current user, eg for kamino:dt100
There's provision for a second user view e.g
```
user@host:~/PROJECTS/HTSCOPE/htscope1$ ./scripts/make_htscope_st.cmd.py --user2=fred --nchan=32 --data32=0 acq1102_015 acq1102_010
```
Multiple second users are supported, eg
```
user@host:~/PROJECTS/HTSCOPE/htscope1$ ./scripts/make_htscope_st.cmd.py --user2=fred,jim --nchan=32 --data32=0 acq1102_015 acq1102_010
```
** NB ** each second user needs a data file alias @@todo WBN to use a single mapping
e.g
```
user@host:~$ ln -s host:user:acq1102_010 host:fred:acq1102_010
user@host:~$ ln -s host:user:acq1102_015 host:fred:acq1102_015
```
### What PV's do we get?

### Global
```
user@host:~/PROJECTS/HTSCOPE/htscope1$ cat records.dbl | grep -v user | grep -v fred
host:SHOT_TIME                   # User specified run time
host:RUNSTOP                     # User sets RUN to start, worker set s STOP on completion
host:UUTS                        # User sets list of UUTS in fleet
host:STATUS                      # worker reports current status in one-liner
```
### Primary User
```
user@host:~/PROJECTS/HTSCOPE/htscope1$ cat records.dbl | grep -v fred | grep CH:01
host:user:acq1102_015:CH:01
host:user:acq1102_010:CH:01
```
### user2
```
user@host:~/PROJECTS/HTSCOPE/htscope1$ cat records.dbl | grep -v user | grep CH:01
host:fred:acq1102_015:CH:01
host:fred:acq1102_010:CH:01
```


### @@TODO

* run IOC from procServ
* run HTSTREAM_LAUNCHER from procServ
* make_htscope_st.cmd.py .. avoid HAPI if possible, suggest use pyEpics to pick up nchan, data32 values from UUT.
* HUGE Max Array Size 100000 .. we're using DOUBLES. Maybe get better mileage with FLOATS .. this is a _network_ protocol
* Prototype implemented with Channel Access CA .. maybe easy to switch to the new PV Access thanks to QSRV .. but is it any better?
* Data source is ~/host:user:uut, usually an s-link to the real landing area on ramdisk. Currently, s-link made by hand
eg
<pre>
dt100@kamino ~]$ls -l /home/dt100/kamino\:dt100\:acq1102_015
lrwxrwxrwx 1 dt100 dt100 36 Jan 27 16:08 /home/dt100/kamino:dt100:acq1102_015 -> /mnt/afhba.2/acq1102_015/000000/2.00
</pre>
* Data source @@todo .. what happens a/ at the get go when there is no data, b/ when the data file is rubbed out on a new capture. Currently, our best move is to force an IOC restart..
* use StreamDevice to monitor the output from ht_stream and keep a status PV up to date.0

* We have a bootstrap problem: IOC won't run until we have one data set. So we need to make the device support open/mmap file on demand (and close it on a file delete on new run).

Typical bootstrap command 
```
/scripts/ht_stream.py --concat=999999 --secs=10 acq1102_015 acq1102_010
```

* making /mnt/ writable: Peter hacked as follows: works for folks who follow instructions to create 
user "dt100"

--- a/scripts/mount-ramdisk
+++ b/scripts/mount-ramdisk
@@ -1,2 +1,4 @@
 mount -t ramfs dram /mnt
+sudo chown -R dt100.dt100 /mnt
+

* aborting the stream can crash the box - probably best to suppress abort once data is flowing (the shot is only 10s after all)




