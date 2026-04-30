/*
 * xrmSlicePM.cpp
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#include "xrmSlicePM.h"

static const char *driverName="acq400_PM";
#define DN	driverName
#define FN	__FUNCTION__

XrmSlicePM::XrmSlicePM(const char *portName, int max_addr):
	XrmSliceCommon(portName, max_addr)
{

}


extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int xrmSlice_PM_Configure(const char *portName, int addr)
	{
		printf("%s:%s R1001 %s\n", DN, FN, portName);

		new XrmSlicePM(portName, addr);
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

	void acq400_PM_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_PM_Register);
}
