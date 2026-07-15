/** @file xrmSliceHT.cpp
 *  @brief impl. for the HT (Hold Table) slicer.
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#include "xrmSliceHT.h"
#include "acq-util.h"

static const char *driverName= __FILE__;
#define DN	driverName
#define FN	__FUNCTION__

int XrmSliceHT::nice 	= ::getenv_default("XrmSliceHT_NICE", 10);
std::vector<XrmSliceHT*> XrmSliceHT::rowHandlers;

XrmSliceHT::XrmSliceHT(const char *portName, int addr /* ai_cols */ ):
	XrmSliceCommon(portName, addr),
	ht_buf_len(0),
	ht_data(0)
{
	asynStatus status;

	createParam(PS_SOE_HLD_ENT_PV_ID, 	asynParamInt32, &P_SOE_HLD_ENT_PV_ID);
	createParam(PS_SOE_HLD_ENT_CLIDAT, 	asynParamInt32, &P_SOE_HLD_ENT_CLIDAT);
	createParam(PS_SOE_HLD_ENT_TS, 		asynParamInt32, &P_SOE_HLD_ENT_TS);
	createParam(PS_SOE_HLD_ENT_DATA_OFFSET, asynParamInt32, &P_SOE_HLD_ENT_DATA_OFFSET);

	createParam(PS_XS_AI16_CH_RAW,	asynParamInt32, &P_XS_AI16_CH_RAW);
	createParam(PS_XS_AI16_CH_EGU,	asynParamFloat64, &P_XS_AI16_CH_EGU);
	createParam(PS_XS_DI32_CH_RAW,	asynParamInt32, &P_XS_DI32_CH_RAW);
	createParam(PS_XS_SP32_SP, 	asynParamInt64, &P_XS_SP32_SP);
	createParam(PS_XS_SP32_WRVS, 	asynParamInt32, &P_XS_SP32_WRVS);
	createParam(PS_XS_SP32_WRVT, 	asynParamInt32, &P_XS_SP32_WRVT);
	createParam(PS_XS_SP32_WRUS, 	asynParamInt64, &P_XS_SP32_WRUS);

	createParam(PS_HT_RAW_INPUT,	asynParamInt32Array,  &P_HT_RAW_INPUT);

	if (rowHandlers.size() == 0){
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
		}
	}
}


void XrmSliceHT::task_runner(void *drvPvt)
{
	((XrmSliceHT *)drvPvt)->task();
}

typedef epicsUInt32 * PU32;


bool XrmSliceHT::ready_to_slice() {

	if (!ht_data){
		fprintf(stderr, "%s WARNING: ht_raw not set\n", FN);
		return false;
	}
	const SamplePrams& sp = uut_p.sample_prams;
	if (!sp.isValid()){
		fprintf(stderr, "%s WARNING: !sample_prams.isValid()\n", FN);
		return false;
	}

	return true;

}

void XrmSliceHT::ht_slice(size_t row, SOE_HOLD_HEADER* header, epicsUInt32* sample)
{
	if (verbose) fprintf(stderr, "%s:%s:" FMTSZT " pv:%d sample:%p\n", DN, FN, row, header->pv_id, sample);
	const SamplePrams& sp = uut_p.sample_prams;

	if (row < rowHandlers.size()){
		XrmSliceHT* sliceHT = rowHandlers[row];
		sliceHT->sip(0, P_SOE_HLD_ENT_PV_ID, header->pv_id);
		// @@todo now do the rest

		epicsUInt32* psrc32 = sample;
		epicsInt16* psrc16 = (epicsInt16*)psrc32;

		for (int ai = 0; ai < sp.AI_COUNT; ++ai){
			const epicsInt16 raw = psrc16[ai];
			double egu = (double)raw*uut_p.eslo[ai] + uut_p.eoff[ai];

			if (verbose > 1 && ai == 0){
				fprintf(stderr, "%s:%s: %d raw:%04x egu:%.3f eoff:%.3f eslo:%.3f\n",
						DN, FN, ai, raw, egu, uut_p.eoff[ai], uut_p.eslo[ai]);
			}
			sliceHT->sip(ai, P_XS_AI16_CH_RAW, raw);
			sliceHT->sfp(ai, P_XS_AI16_CH_EGU, egu);
		}
		for (int di = 0; di < sp.DI_COUNT; ++di){
			sliceHT->sip(di, P_XS_DI32_CH_RAW, psrc32[sp.DI_INDEX+di]);
		}

		epicsUInt32* p_SP32 = psrc32+sp.SP_INDEX;

		for (int spad = 0; spad < sp.SP_COUNT && spad < SPAD_LIM; ++spad){
			sliceHT->sip(spad, P_XS_SP32_SP, (epicsInt64)p_SP32[spad]);
		}

		unsigned wrv = p_SP32[SP2];
		unsigned wrs = p_SP32[SP3];

		sliceHT->sip(0, P_XS_SP32_WRVS, (wrv >> 28)&0x07);
		sliceHT->sip(0, P_XS_SP32_WRVT, wrv & 0x0fffffff);
		sliceHT->sip(0, P_XS_SP32_WRUS, getWrTs(wrs, wrv));

		sliceHT->callParamCallbacks();
		for (int ai = 1; ai < sp.AI_COUNT; ++ai){
			sliceHT->callParamCallbacks(ai);
		}
	}else{
		fprintf(stderr, "%s::%s WARNING: row " FMTSZT " > available handlers " FMTSZT "\n",
				DN, FN, row, rowHandlers.size());
	}
}
void XrmSliceHT::task()
{
	for (int ii = 0; wait_and_lock(); unlock(), ++ii){
		if (verbose) fprintf(stderr, "%s::%s inside lock\n", DN, FN);

		if (!ready_to_slice()){
			continue;         // NO ACTION until buffer parameters are in place.
		}
		if (verbose) fprintf(stderr, "%s::%s:%4d ready to slice len: " FMTSZT " buf:%p\n",
					DN, FN, ii, ht_buf_len, ht_data);
		// decode new HT, do a lot of sips(, addr=ENTRY)

		SOE_HOLD_HEADER* header = (SOE_HOLD_HEADER*)ht_data;
		for (size_t row = 0; header->pv_id != 0; ++row, ++header){
			ht_slice(row, header, ht_data+header->data_offset);
		}
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
	if (ht_data == 0){
		ht_data = (PU32)value;
	}
	assert(ht_data == (PU32)value);
	unlock();
	epicsEventSignal(eventId);
    } else {
        // Fall back to base class for standard parameters
        status = XrmSliceCommon::writeInt32Array(pasynUser, value, nElements);
    }

    return status;
}



bool XrmSliceHT::exists(const char* portName)
{
	for (auto pht: rowHandlers){
		if (strcmp(portName, pht->getPortName()) == 0){
			return true;
		}
	}
	return false;
}

XrmSliceHT* XrmSliceHT::factory(const char* portName, int addr)
{
	assert(!exists(portName));

	XrmSliceHT* current = new XrmSliceHT(portName, addr);
	rowHandlers.push_back(current);
	return current;
}

extern "C" {
	/** EPICS iocsh callable function to call constructor for the XrmSliceHT class.
	  * @param[in] portName The name of the asyn port driver to be created.
	  */
	int xrmSlice_HT_Configure(const char *portName, int addr)
	{
		printf("%s:%s R1001 %s ai_columns:%d\n", DN, FN, portName, addr);

		XrmSliceHT::factory(portName, addr);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg initArg1 = { "addr", iocshArgInt };
	static const iocshArg * const initArgs[] = { &initArg0, &initArg1 };
	static const iocshFuncDef initFuncDef = { "xrmSlice_HT_Configure", 2, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		xrmSlice_HT_Configure(args[0].sval, args[1].ival);
	}

	void xrmSlice_HT_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(xrmSlice_HT_Register);
}


