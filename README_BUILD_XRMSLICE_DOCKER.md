1. Get all the files you need:
```
git clone https://github.com/D-TACQ/ACQ400_ESW_TOP
cd ACQ400_ESW_TOP/
git submodule update --init --recursive DRIVERS-OOK/ ACQ400DRV/ ACQ400COMMON/ EPICS7/
git clone https://github.com/D-TACQ/HTSCOPE XRMSLICE
```

2. Checkout the desired branch:
```
cd XRMSLICE
```

```
git status
  On branch main
  Your branch is up to date with 'origin/main'.

nothing to commit, working tree clean
```

```
git branch --remote
  origin/HEAD -> origin/main
  origin/feature/docker-xrmslice
  origin/main
  origin/os_system_deadend
  origin/xrmSlice
```

```
git checkout xrmSlice
```

3. copy dockerfile and the dockerignore files attached to this mail into `ACQ400_ESW_TOP` directory

```
cp xrmSlice/Dockerfile.* .
```

4. Build the Docker image:
```
$ docker build -f Dockerfile.xrmslice -t xrmslice-ioc .
```

5. Run the docker image:
```
$ docker run -it --network host --rm --name xrmSlicer --entrypoint /bin/bash xrmslice-ioc:latest
```

6. Run these commands in the image (the run command will drop you in to a shell on the container automatically):
```
python scripts/make_xrmslice_st.cmd.py --output iocBoot/iocxrmSlice/st2.cmd acq2206_588,10.12.197.110:44000 acq2206_598,10.12.197.116:44000 acq2206_501,10.12.197.89:44000
```

cd /opt/xrmSlice/iocBoot/iocxrmSlice
../../bin/linux-x86_64/xrmSlice st2.cmd
```

```
... epics initialises here ...

epics> dbl

And you will see your records...
```

Remember to replace the python script UUT and IP address args with your UUTs/IPs.
