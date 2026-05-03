/*
 * xrmSlicePM.cpp
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#include "xrmSlicePM.h"

static const char *driverName= __FILE__;
#define DN	driverName
#define FN	__FUNCTION__

XrmSlicePM::XrmSlicePM(const char *portName, int max_addr):
	XrmSliceCommon(portName, max_addr)
{
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
