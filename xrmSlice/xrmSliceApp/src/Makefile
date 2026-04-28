TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Use typed rset structure (see 3.16.1 release notes)
USR_CPPFLAGS += -DUSE_TYPED_RSET

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC += xxxRecord

# Install xxxSupport.dbd into <top>/dbd
DBD += xxxSupport.dbd

# Build an IOC support library
LIBRARY_IOC += xrmSliceSupport

# Compile and add code to the support library
xrmSliceSupport_SRCS += xxxRecord.c
xrmSliceSupport_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application, that
# causes problems on Windows DLL builds
xrmSliceSupport_SRCS += dbSubExample.c
xrmSliceSupport_SRCS += devxrmSliceVersion.c
xrmSliceSupport_SRCS += xrmSliceHello.c
xrmSliceSupport_SRCS += initTrace.c

xrmSliceSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# Auto-generate a header file containing a version string.
# Version comes from the VCS if available, else date+time.
GENVERSION = xrmSliceVersion.h
# Macro name
GENVERSIONMACRO = xrmSliceVERSION

# Build the IOC application
PROD_IOC = xrmSlice

# xrmSlice.dbd will be created and installed
DBD += xrmSlice.dbd

# xrmSlice.dbd will include these files:
xrmSlice_DBD += base.dbd
xrmSlice_DBD += xxxSupport.dbd
xrmSlice_DBD += dbSubExample.dbd
xrmSlice_DBD += devxrmSliceVersion.dbd
xrmSlice_DBD += xrmSliceHello.dbd
xrmSlice_DBD += initTrace.dbd

# xrmSlice_registerRecordDeviceDriver.cpp derives from xrmSlice.dbd
xrmSlice_SRCS += xrmSlice_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
xrmSlice_SRCS_DEFAULT += xrmSliceMain.cpp
xrmSlice_SRCS_vxWorks += -nil-

# Link in the code from our support library
xrmSlice_LIBS += xrmSliceSupport

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    # Build sncExample into xrmSliceSupport
    sncExample_SNCFLAGS += +r
    xrmSlice_DBD += sncExample.dbd
    # A .stt sequence program is *not* pre-processed:
    xrmSliceSupport_SRCS += sncExample.stt
    xrmSliceSupport_LIBS += seq pv
    xrmSlice_LIBS += seq pv

    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    # A .st sequence program *is* pre-processed:
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Link QSRV (pvAccess Server) if available
ifdef EPICS_QSRV_MAJOR_VERSION
    xrmSlice_LIBS += qsrv
    xrmSlice_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
    xrmSlice_DBD += PVAServerRegister.dbd
    xrmSlice_DBD += qsrv.dbd
endif

# Finally link IOC to the EPICS Base libraries
xrmSlice_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

# Explicit dependency needed for generated header file
devxrmSliceVersion$(DEP): $(COMMON_DIR)/$(GENVERSION)
