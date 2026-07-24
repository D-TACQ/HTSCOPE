TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Use typed rset structure (see 3.16.1 release notes)
USR_CPPFLAGS += -DUSE_TYPED_RSET

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord

# Install htscope1Support.dbd into <top>/dbd
DBD += htscope1Support.dbd

# Build an IOC support library
LIBRARY_IOC += htscope1Support

# Compile and add code to the support library
#htscope1Support_SRCS += xxxRecord.c
#htscope1Support_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application, that
# causes problems on Windows DLL builds
htscope1Support_SRCS += devhtscope1Version.c
htscope1Support_SRCS += htscope1Hello.c
htscope1Support_SRCS += initTrace.c
htscope1Support_SRCS += MultiChannelScope.cpp

htscope1Support_LIBS += $(EPICS_BASE_IOC_LIBS)

# Auto-generate a header file containing a version string.
# Version comes from the VCS if available, else date+time.
GENVERSION = htscope1Version.h
# Macro name
GENVERSIONMACRO = htscope1VERSION

# Build the IOC application
PROD_IOC = htscope1

# htscope1.dbd will be created and installed
DBD += htscope1.dbd

# htscope1.dbd will include these files:
htscope1_DBD += base.dbd
htscope1_DBD += devhtscope1Version.dbd
htscope1_DBD += htscope1Hello.dbd
htscope1_DBD += initTrace.dbd

# htscope1_registerRecordDeviceDriver.cpp derives from htscope1.dbd
htscope1_SRCS += htscope1_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
htscope1_SRCS_DEFAULT += htscope1Main.cpp
htscope1_SRCS_vxWorks += -nil-

htscope1_LIBS += stream
htscope1_LIBS += asyn
# Link in the code from our support library
htscope1_LIBS += htscope1Support

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
#ifneq ($(SNCSEQ),)
    # Build sncExample into htscope1Support
    #sncExample_SNCFLAGS += +r
    #htscope1_DBD += sncExample.dbd
    # A .stt sequence program is *not* pre-processed:
    #htscope1Support_SRCS += sncExample.stt
    #htscope1Support_LIBS += seq pv
    #htscope1_LIBS += seq pv

    # Build sncProgram as a standalone program
    #PROD_HOST += sncProgram
    #sncProgram_SNCFLAGS += +m
    # A .st sequence program *is* pre-processed:
    #sncProgram_SRCS += sncProgram.st
    #sncProgram_LIBS += seq pv
    #sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
#endif

# Link QSRV (pvAccess Server) if available
ifdef EPICS_QSRV_MAJOR_VERSION
    htscope1_LIBS += qsrv
    htscope1_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
    htscope1_DBD += PVAServerRegister.dbd
    htscope1_DBD += qsrv.dbd
endif

# Finally link IOC to the EPICS Base libraries
htscope1_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

# Explicit dependency needed for generated header file
devhtscope1Version$(DEP): $(COMMON_DIR)/$(GENVERSION)
