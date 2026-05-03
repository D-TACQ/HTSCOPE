/*
 * xrmSliceHT.cpp
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#include "xrmSliceHT.h"

static const char *driverName= __FILE__;
#define DN	driverName
#define FN	__FUNCTION__

const int SOE_HLD_ROWS = 64;

XrmSliceHT::XrmSliceHT(const char *portName):
	XrmSliceCommon(portName, SOE_HLD_ROWS)
{
	createParam(PS_XS_AI16_CH_RAW,	asynParamInt32, &P_XS_AI16_CH_RAW);
	createParam(PS_XS_AI16_CH_EGU,	asynParamFloat64, &P_XS_AI16_CH_EGU);
	createParam(PS_XS_DI32_CH_RAW,	asynParamInt32, &P_XS_DI32_CH_RAW);
	createParam(PS_XS_SP32_SP0, 	asynParamInt32, &P_XS_SP32_SP0);
	createParam(PS_XS_SP32_SP1, 	asynParamInt32, &P_XS_SP32_SP1);
	createParam(PS_XS_SP32_SP2, 	asynParamInt32, &P_XS_SP32_SP2);
	createParam(PS_XS_SP32_SP3, 	asynParamInt32, &P_XS_SP32_SP3);
	createParam(PS_XS_SP32_WRVS, 	asynParamInt32, &P_XS_SP32_WRVS);
	createParam(PS_XS_SP32_WRVT, 	asynParamInt32, &P_XS_SP32_WRVT);
	createParam(PS_XS_SP32_WRUS, 	asynParamInt64, &P_XS_SP32_WRUS);

	createParam(PS_HT_RAW_INPUT,	asynParamInt32Array,  &P_HT_RAW_INPUT);
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


