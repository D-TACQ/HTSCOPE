/*
 * xrmSlicePM.cpp
 * Operation:
 * INPUTS: P_PM_RAW_INPUT  : the blob duplicated from ACQ400 xrmIoc
 * OUTPUT: P_XS_AI16_CH_RAW, P_XS_AI16_CH_EGU etc
 *
 * The slicing is done by the asyn task.
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
	pm_raw(0), p_AI16(0), p_AI_EGU(0), p_DI32(0),
	p_SP32(0), p_WRVS(0), p_WRVT(0), p_WRUS(0), pm_buf_len(0)
{
	asynStatus status;

	createParam(PS_XS_AI16_CH_RAW,	asynParamInt16Array, &P_XS_AI16_CH_RAW);
	createParam(PS_XS_AI16_CH_EGU,	asynParamFloat32Array, &P_XS_AI16_CH_EGU);
	createParam(PS_XS_DI32_CH_RAW,	asynParamInt32Array, &P_XS_DI32_CH_RAW);
	createParam(PS_XS_SP32_SP, 	asynParamInt32Array, &P_XS_SP32_SP);
	createParam(PS_XS_SP32_WRVS, 	asynParamInt32Array, &P_XS_SP32_WRVS);
	createParam(PS_XS_SP32_WRVT, 	asynParamInt32Array, &P_XS_SP32_WRVT);
	createParam(PS_XS_SP32_WRUS, 	asynParamInt64Array, &P_XS_SP32_WRUS);

	createParam(PS_PM_RAW_INPUT,	asynParamInt32Array,  &P_PM_RAW_INPUT);
	fprintf(stderr, "%s final param: %s %d\n", FN, PS_PM_RAW_INPUT, P_PM_RAW_INPUT);


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
	const int ndata = sample_prams.NSAM - 1;

	/* allocate local buffers.
	 * The XrmSlicePM instance stays live for the duration of the process,
	 * and the OS recovers the data on process exit => no leak, no need for delete
	 */
	if (!p_AI16 && sample_prams.AI_COUNT > 0){
		p_AI16 = new epicsInt16* [sample_prams.AI_COUNT];
		for (int ai = 0; ai < sample_prams.AI_COUNT; ++ai){
			p_AI16[ai] = new epicsInt16 [ndata];
		}
	}
	if (!p_AI_EGU && sample_prams.AI_COUNT > 0){
		p_AI_EGU = new epicsFloat32* [sample_prams.AI_COUNT];
		for (int ai = 0; ai < sample_prams.AI_COUNT; ++ai){
			p_AI_EGU[ai] = new epicsFloat32 [ndata];
		}
	}
	if (!p_DI32 && sample_prams.DI_COUNT > 0){
		p_DI32 = new epicsUInt32* [sample_prams.DI_COUNT];
		for (int di = 0; di < sample_prams.DI_COUNT; ++di){
			p_DI32[di] = new epicsUInt32 [ndata];
		}
	}
	if (!p_SP32 && sample_prams.SP_COUNT > 0){
		p_SP32 = new epicsUInt32* [sample_prams.SP_COUNT];
		for (int sp = 0; sp < sample_prams.SP_COUNT; ++sp){
			p_SP32[sp] = new epicsUInt32 [ndata];
		}
	}
	(p_WRVS == 0) && (p_WRVS = new epicsUInt32 [ndata]);
	(p_WRVT == 0) && (p_WRVT = new epicsUInt32 [ndata]);
	(p_WRUS == 0) && (p_WRUS = new epicsUInt64 [ndata]);

	return true;
}


#define SP0	0
#define SP1	1
#define SP2	2
#define SP3	3


#define SPAD_LIM 8        // nothing to see in higher order SPADs, don't waste time on them

void XrmSlicePM::task()
/**< slicing takes place here. */
{
	SamplePrams& sp = sample_prams;
	bool vectors_checked = false;

	for (int ii = 0; wait_and_lock(); unlock(), ++ii){
		if (verbose) fprintf(stderr, "%s inside lock\n", FN);

		if (!ready_to_slice()){
			continue;         // NO ACTION until buffer parameters are in place.
		}
		if (!vectors_checked){
			for (int ai = 0; ai < sp.AI_COUNT; ++ai){
				assert(p_AI16[ai] != 0);
				assert(p_AI_EGU[ai] != 0);
			}
			for (int di = 0; di < sp.DI_COUNT; ++di){
				assert(p_DI32[di] != 0);
			}
			for (int spad = 0; spad < sp.SP_COUNT; ++spad){
				assert(p_SP32[spad] != 0);
			}
			vectors_checked = true;
		}
		if (verbose) fprintf(stderr, "%s slice\n", FN);

		for (int row = 1; row < sp.NSAM; ++row){
			const int outrow = row-1;

			epicsUInt32* psrc32 = pm_raw + (row * sp.SSB/sizeof(epicsUInt32));
			epicsInt16* psrc16 = (epicsInt16*)psrc32;

			for (int ai = 0; ai < sp.AI_COUNT; ++ai){
				const epicsInt16 raw = psrc16[ai];
				p_AI16[ai][outrow] = raw;
				p_AI_EGU[ai][outrow] = (float)raw*eslo[ai] + eoff[ai];
			}
			for (int di = 0; di < sp.DI_COUNT; ++di){
				if (p_DI32[di]){
					p_DI32[di][outrow] = psrc32[sp.DI_INDEX+di];
				}
			}
			for (int spad = 0; spad < sp.SP_COUNT && spad < SPAD_LIM; ++spad){
				if (p_SP32[spad]){
					p_SP32[spad][outrow] = psrc32[sp.SP_INDEX+spad];
				}
			}
		}

		for (int row = 1; row < sp.NSAM; ++row){
			const int outrow = row-1;
			unsigned wrv = p_SP32[SP2][outrow];
			unsigned wrs = p_SP32[SP3][outrow];

			p_WRVS[outrow] = (wrv >> 28)&0x07;
			p_WRVT[outrow] = wrv & 0x0fffffff;
			p_WRUS[outrow] = getWrTs(wrs, wrv);
		}
		if (verbose) fprintf(stderr, "%s callbacks\n", FN);

		const int NDATA = sp.NSAM-1;

		for (int ai = 0; ai < sp.AI_COUNT; ++ai){
			if (verbose > 1) fprintf(stderr, "%s P:%d A:%d\n", FN, P_XS_AI16_CH_RAW, ai);
			doCallbacksInt16Array(p_AI16[ai], NDATA, P_XS_AI16_CH_RAW, ai);
			if (verbose > 1) fprintf(stderr, "%s P:%d A:%d\n", FN, P_XS_AI16_CH_EGU, ai);
			doCallbacksFloat32Array(p_AI_EGU[ai], NDATA, P_XS_AI16_CH_EGU, ai);
		}
		for (int di = 0; di < sp.DI_COUNT; ++di){
			doCallbacksInt32Array((epicsInt32*)p_DI32[di], NDATA, P_XS_DI32_CH_RAW, di);
		}
		for (int spad = 0; spad < sp.SP_COUNT && spad < SPAD_LIM; ++spad){
			doCallbacksInt32Array((epicsInt32*)p_SP32[spad], NDATA, P_XS_SP32_SP, spad);
		}
		doCallbacksInt32Array((epicsInt32*)p_WRVS, NDATA, P_XS_SP32_WRVS, 0);
		doCallbacksInt32Array((epicsInt32*)p_WRVT, NDATA, P_XS_SP32_WRVT, 0);
		doCallbacksInt64Array((epicsInt64*)p_WRUS, NDATA, P_XS_SP32_WRUS, 0);

		doCallbacksInt32Array((epicsInt32*)pm_raw, pm_buf_len, P_PM_RAW_INPUT, 0);

		if (verbose) fprintf(stderr, "%s leaving lock()\n", FN);
	}

}

void XrmSlicePM::task_runner(void *drvPvt)
{
	((XrmSlicePM *)drvPvt)->task();
}


typedef epicsUInt32 * PU32;


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
        status = XrmSliceCommon::writeInt32Array(pasynUser, value, nElements);
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
