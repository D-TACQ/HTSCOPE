/*
 * xrmSlicePM.cpp
 * Operation:
 * INPUTS: P_PM_RAW_INPUT  : the blob duplicated from ACQ400 xrmIoc
 * OUTPUT: P_XS_AI16_CH_RAW, P_XS_AI16_CH_EGU etc
 *
 * The slicing is done by the asyn task. task() operates on the buffers directly
 * allocated by records. No bounce buffers.
 * task() has to wait until buffers have been allocated, and indeed does not
 * know where the buffers are until after they have been notified with
 * read*Array(), write*Array().
 * The read/write methods copy the buffer pointers for task() to use.
 * We ASSUME that these buffer pointers are constant (how could they not be)
 * We have to harden against initial times when there's no buffer for a task() and also
 * for the initial call to read(), write() where we have the buffer, but no data yet.
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#include "xrmSlicePM.h"
#include "acq-util.h"

static const char *driverName= __FILE__;
#define DN	driverName
#define FN	__FUNCTION__

int XrmSlicePM::nice 	= ::getenv_default("XrmSlicePM_NICE", 10);
int XrmSlicePM::verbose = ::getenv_default("XrmSlicePM_VERBOSE", 0);

XrmSlicePM::XrmSlicePM(const char *portName, int max_addr):
	XrmSliceCommon(portName, max_addr),
	pm_raw(0), p_AI16(0), p_DI32(0), p_SP32(0), pm_buf_len(0)
{
	asynStatus status;

	createParam(PS_XS_AI16_CH_RAW,	asynParamInt16Array, &P_XS_AI16_CH_RAW);
	createParam(PS_XS_AI16_CH_EGU,	asynParamFloat32Array, &P_XS_AI16_CH_EGU);
	createParam(PS_XS_DI32_CH_RAW,	asynParamInt32Array, &P_XS_DI32_CH_RAW);
	createParam(PS_XS_SP32_SP0, 	asynParamInt32Array, &P_XS_SP32_SP0);
	createParam(PS_XS_SP32_SP1, 	asynParamInt32Array, &P_XS_SP32_SP1);
	createParam(PS_XS_SP32_SP2, 	asynParamInt32Array, &P_XS_SP32_SP2);
	createParam(PS_XS_SP32_SP3, 	asynParamInt32Array, &P_XS_SP32_SP3);
	createParam(PS_XS_SP32_WRVS, 	asynParamInt32Array, &P_XS_SP32_WRVS);
	createParam(PS_XS_SP32_WRVT, 	asynParamInt32Array, &P_XS_SP32_WRVT);
	createParam(PS_XS_SP32_WRUS, 	asynParamInt64Array, &P_XS_SP32_WRUS);

	createParam(PS_PM_RAW_INPUT,	asynParamInt32Array,  &P_PM_RAW_INPUT);

	/* Create the thread that computes the waveforms in the background */
	char* taskname = new char[32];
	snprintf(taskname, 32, "%s_task", portName);
	status = (asynStatus)(epicsThreadCreate(taskname,
			epicsThreadPriorityHigh - nice,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)task_runner,
			this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
		return;
	}
}

bool XrmSlicePM::wait_and_lock()  {
	epicsEventWait(eventId);
	lock();
	return true;
}

bool XrmSlicePM::ready_to_slice() {
	if (!pm_raw){
		fprintf(stderr, "%s WARNING: pm_raw not set\n", FN);
		return false;
	}
	if (!sample_prams.isValid()){
		fprintf(stderr, "%s WARNING: !sample_prams.isValid()\n", FN);
		return false;
	}
	if (!p_AI16 && sample_prams.AI_COUNT > 0){
		fprintf(stderr, "%s WARNING: p_AI16 not set\n", FN);
		return false;
	}
	if (!p_DI32 && sample_prams.DI_COUNT > 0){
		fprintf(stderr, "%s WARNING: p_DI32 not set\n", FN);
		return false;
	}
	if (!p_SP32 && sample_prams.SP_COUNT > 0){
		fprintf(stderr, "%s WARNING: p_SP32 not set\n", FN);
		return false;
	}

	return true;
}

void XrmSlicePM::task()
/**< slicing takes place here. */
{
	SamplePrams& sp = sample_prams;


	for (int ii = 0; wait_and_lock(); unlock(), ++ii){
		if (verbose) fprintf(stderr, "%s inside lock\n", FN);

		if (!ready_to_slice()){
			continue;         // NO ACTION until buffer parameters are in place.
		}
		if (verbose) fprintf(stderr, "%s slice\n", FN);

		for (int row = 1; row < sp.NSAM; ++row){
			int outrow = row-1;

			epicsUInt32* psrc32 = pm_raw + (row * sp.SSB/sizeof(epicsUInt32));
			epicsInt16* psrc16 = (epicsInt16*)psrc32;

			for (int ai = 0; ai < sp.AI_COUNT; ++ai){
				if (p_AI16[ai]){
					p_AI16[ai][outrow] = psrc16[ai];
				}
			}
			for (int di = 0; di < sp.DI_COUNT; ++di){
				if (p_DI32[di]){
					p_DI32[di][outrow] = psrc32[sp.DI_INDEX+di];
				}
			}
			for (int spad = 0; spad < sp.SP_COUNT; ++spad){
				if (p_SP32[spad]){
					p_SP32[spad][outrow] = psrc32[sp.SP_INDEX+spad];
				}
			}
		}
		if (verbose) fprintf(stderr, "%s callbacks\n", FN);

		const int NDATA = sp.NSAM-1;

		for (int ai = 0; ai < sp.AI_COUNT; ++ai){
			if (p_AI16[ai]){
				doCallbacksInt16Array(p_AI16[ai], NDATA, P_XS_AI16_CH_RAW, ai);
			}
		}
		for (int di = 0; di < sp.DI_COUNT; ++di){
			if (p_DI32[di]){
				doCallbacksInt32Array((epicsInt32*)p_DI32[di], NDATA, P_XS_DI32_CH_RAW, di);
			}
		}
		for (int spad = 0; spad < sp.SP_COUNT; ++spad){
			if (p_SP32[spad]){
				doCallbacksInt32Array((epicsInt32*)p_SP32[spad], NDATA, P_XS_SP32_SP0+spad, 0);
			}
		}
		doCallbacksInt32Array(pm_raw, pm_buf_len, P_PM_RAW_INPUT, 0);

		if (verbose) fprintf(stderr, "%s leaving lock()\n", FN);
	}

}

void XrmSlicePM::task_runner(void *drvPvt)
{
	((XrmSlicePM *)drvPvt)->task();
}

asynStatus XrmSlicePM::readInt16Array(asynUser *pasynUser, epicsInt16 *value,
		size_t nElements, size_t *nIn)
{
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *paramName;
	int addr = 0;

	getParamName(function, &paramName);
	if (maxAddr > 1){
		status = pasynManager->getAddr(pasynUser, &addr);
		if(status!=asynSuccess) return status;
	}

	asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
			"%s: Port %s, Param %s, nElements " FMTSZT "\n", FN,
			portName, paramName, nElements);

	lock();
	if (function == P_XS_AI16_CH_RAW){
		if (!sample_prams.isValid()){
			fprintf(stderr, "%s WARNING sample_prams not valid\n", FN);
			status = asynError;
		}
		if (p_AI16){
			if (!p_AI16[addr]){
				/* @@todo this is sketchy! What are we doing?
				 * first call to read() sets the internal dst buffer to value,
				 * assumed to be pre-allocated WF array from record.
				 * later the task() will fill this, but meanwhile, return 0 bytes,
				 * so clients don't see garbage.
				 * Couple problems
				 * - maybe should be clearing nElements so that the parent read does nothing.
				 * - _can_ we rely on updated callbacks. Not all clients monitor(), some of them just read() ..
				 *    .. well, then just read again, pick up last value left by task. Let's see..
				 */
				p_AI16[addr] = value;
				*nIn = 0;
			}
			assert(p_AI16[addr] == value);
		}
	}
	unlock();

	if (status == asynSuccess){
		status = asynPortDriver::readInt16Array(
				pasynUser, value, nElements, nIn);
	}
	asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
			"%s: Port %s, Param %s, nElements " FMTSZT " return %d\n", FN,
			portName, paramName, nElements, status);

	return status;
}

typedef epicsUInt32 * PU32;

asynStatus XrmSlicePM::readInt32Array(
		asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *paramName;
	int addr = 0;

	getParamName(function, &paramName);
	if (maxAddr > 1){
		status = pasynManager->getAddr(pasynUser, &addr);
		if(status!=asynSuccess) return status;
	}

	asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
			"%s: Port %s, Param %s, nElements " FMTSZT "\n", FN,
			portName, paramName, nElements);
	lock();
	if (function == P_XS_DI32_CH_RAW){
		if (!sample_prams.isValid()){
			fprintf(stderr, "%s WARNING sample_prams not valid\n", FN);
			status = asynError;
		}
		if (!p_DI32){
			p_DI32 = new PU32 [sample_prams.DI_COUNT];
		}

		if (p_DI32){
			assert(addr < sample_prams.DI_COUNT);
			if (!p_DI32[addr]){
				p_DI32[addr] = (PU32)value;
				*nIn = 0;
			}
			assert(p_DI32[addr] == (PU32)value);
		}
	}
	if (function == P_XS_SP32_SP0 ||
			function == P_XS_SP32_SP1 ||
			function == P_XS_SP32_SP2 ||
			function == P_XS_SP32_SP3 	){

		assert(P_XS_SP32_SP1-P_XS_SP32_SP0 == 1);
		assert(P_XS_SP32_SP2-P_XS_SP32_SP0 == 2);
		assert(P_XS_SP32_SP3-P_XS_SP32_SP0 == 3);

		if (!sample_prams.isValid()){
			fprintf(stderr, "%s WARNING sample_prams not valid\n", FN);
			status = asynError;
		}
		if (!p_SP32){
			p_SP32 = new PU32 [sample_prams.SP_COUNT];
		}
		assert(addr == 0);
		if (p_SP32){
			int spad = function - P_XS_SP32_SP0;
			if (!p_SP32[spad]){
				p_SP32[spad] = (PU32)value;
				*nIn = 0;
			}
			assert(p_SP32[spad] == (PU32)value);
		}
	}
	unlock();

	if (status == asynSuccess){
		status = asynPortDriver::readInt32Array(
				pasynUser, value, nElements, nIn);
	}
	asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
			"%s: Port %s, Param %s, nElements " FMTSZT " return %d\n", FN,
			portName, paramName, nElements, status);

	return status;
}

asynStatus XrmSlicePM::writeInt32Array(
		asynUser *pasynUser, epicsInt32 *value, size_t nElements)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *paramName;
    int addr = 0;

    getParamName(function, &paramName);
    if (maxAddr > 1){
	    status = pasynManager->getAddr(pasynUser, &addr);
	    if(status!=asynSuccess) return status;
    }

    // Log the action
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s: Port %s, Param %s, nElements " FMTSZT "\n", FN,
              portName, paramName, nElements);
/*
    fprintf(stderr, "%s: Port %s, Param %s, nElements %u\n", FN,
	              portName, paramName, nElements);
*/
    if (function == P_PM_RAW_INPUT) {
	lock();
	if (pm_buf_len == 0){
		pm_buf_len = nElements;
	}
	assert(pm_buf_len == nElements);
	if (pm_raw == 0){
		pm_raw = (PU32)value;
	}
	assert(pm_raw == (PU32)value);
	unlock();
	epicsEventSignal(eventId);
    } else {
        // Fall back to base class for standard parameters
        status = asynPortDriver::writeInt32Array(pasynUser, value, nElements);
    }

    return status;
}


extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int xrmSlice_PM_Configure(const char *portName, int maxAddr)
	{
		printf("%s:%s R1001 %s\n", DN, FN, portName);

		new XrmSlicePM(portName, maxAddr);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg initArg1 = { "depth", iocshArgInt };
	static const iocshArg * const initArgs[] = { &initArg0, &initArg1 };
	static const iocshFuncDef initFuncDef = { "xrmSlice_PM_Configure", 2, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		xrmSlice_PM_Configure(args[0].sval, args[1].ival);
	}

	void xrmSlice_PM_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(xrmSlice_PM_Register);
}
