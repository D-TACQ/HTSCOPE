/*
 * xrmSlicePM.h  : one instance per PM delay (20 instances expected)
 * IN: receive PM as raw BLOB from UUT
 * OUT: presents channelised, EGU data. Channel is the ADDR dimension (max 128).
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#ifndef XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEPM_H_
#define XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEPM_H_


#include "xrmSliceCommon.h"

#define PS_PM_RAW_INPUT	"PM_RAW_INPUT"   /** one per port, single addr=0 */

class XrmSlicePM: public XrmSliceCommon {

protected:
	int P_PM_RAW_INPUT;
	epicsUInt32* pm_raw;
	epicsInt16** p_AI16;
	epicsFloat32** p_AI_EGU;
	epicsUInt32** p_DI32;
	epicsUInt32** p_SP32;
	size_t pm_buf_len;

	bool wait_and_lock();
	bool ready_to_slice();
	virtual void task();

	static void task_runner(void *drvPvt);
	static int nice;
	static int verbose;

public:
	XrmSlicePM(const char *portName, int max_addr);
	virtual ~XrmSlicePM() {}

	virtual asynStatus writeInt32Array(asynUser *pasynUser, epicsInt32 *value,
	                                        size_t nElements);
};
#endif /* XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEPM_H_ */
