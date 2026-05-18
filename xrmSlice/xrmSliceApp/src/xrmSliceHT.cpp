/*
 * xrmSliceHT.cpp
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#include "xrmSliceHT.h"
#include "acq-util.h"

static const char *driverName= __FILE__;
#define DN	driverName
#define FN	__FUNCTION__

const int SOE_HLD_ROWS = 64;

int XrmSliceHT::nice 	= ::getenv_default("XrmSliceHT_NICE", 10);

XrmSliceHT::XrmSliceHT(const char *portName):
	XrmSliceCommon(portName, SOE_HLD_ROWS),
	ht_buf_len(0),
	ht_raw(0)
{
	asynStatus status;

	createParam(PS_SOE_HLD_ENT_PV_ID, 	asynParamInt32, &P_SOE_HLD_ENT_PV_ID);
	createParam(PS_SOE_HLD_ENT_CLIDAT, 	asynParamInt32, &P_SOE_HLD_ENT_CLIDAT);
	createParam(PS_SOE_HLD_ENT_TS, 		asynParamInt32, &P_SOE_HLD_ENT_TS);
	createParam(PS_SOE_HLD_ENT_DATA_OFFSET, asynParamInt32, &P_SOE_HLD_ENT_DATA_OFFSET);

	createParam(PS_XS_AI16_CH_RAW,	asynParamInt32, &P_XS_AI16_CH_RAW);
	createParam(PS_XS_AI16_CH_EGU,	asynParamFloat64, &P_XS_AI16_CH_EGU);
	createParam(PS_XS_DI32_CH_RAW,	asynParamInt32, &P_XS_DI32_CH_RAW);
	createParam(PS_XS_SP32_SP, 	asynParamInt32, &P_XS_SP32_SP);
	createParam(PS_XS_SP32_WRVS, 	asynParamInt32, &P_XS_SP32_WRVS);
	createParam(PS_XS_SP32_WRVT, 	asynParamInt32, &P_XS_SP32_WRVT);
	createParam(PS_XS_SP32_WRUS, 	asynParamInt64, &P_XS_SP32_WRUS);

	createParam(PS_HT_RAW_INPUT,	asynParamInt32Array,  &P_HT_RAW_INPUT);

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


void XrmSliceHT::task_runner(void *drvPvt)
{
	((XrmSliceHT *)drvPvt)->task();
}

typedef epicsUInt32 * PU32;


bool XrmSliceHT::ready_to_slice() {
	if (!ht_raw){
		fprintf(stderr, "%s WARNING: ht_raw not set\n", FN);
		return false;
	}
	if (!sample_prams.isValid()){
		fprintf(stderr, "%s WARNING: !sample_prams.isValid()\n", FN);
		return false;
	}

	return true;

}
void XrmSliceHT::task()
{
	SamplePrams& sp = sample_prams;

	for (int ii = 0; wait_and_lock(); unlock(), ++ii){
		if (verbose) fprintf(stderr, "%s::%s inside lock\n", DN, FN);

		if (!ready_to_slice()){
			continue;         // NO ACTION until buffer parameters are in place.
		}
		if (verbose) fprintf(stderr, "%s::%s :4d ready to slice len:%u buf:%p\n",
					DN, FN, ii, ht_buf_len, ht_raw);
		// decode new HT, do a lot of sips(, addr=ENTRY)
	}
}

asynStatus XrmSliceHT::writeInt32Array(
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
    if (function == P_HT_RAW_INPUT) {
	lock();
	if (ht_buf_len == 0){
		ht_buf_len = nElements;
	}
	assert(ht_buf_len == nElements);
	if (ht_raw == 0){
		ht_raw = (PU32)value;
	}
	assert(ht_raw == (PU32)value);
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
	int xrmSlice_HT_Configure(const char *portName)
	{
		printf("%s:%s R1001 %s\n", DN, FN, portName);

		new XrmSliceHT(portName);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0, };
	static const iocshFuncDef initFuncDef = { "xrmSlice_HT_Configure", 1, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		xrmSlice_HT_Configure(args[0].sval);
	}

	void xrmSlice_HT_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(xrmSlice_HT_Register);
}


