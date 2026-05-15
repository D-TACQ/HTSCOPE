/*
 * xrmSliceHT.h: singleton
 * IN: receive latest HT as raw BLOB from UUT
 * OUT: present channelised, egu data as a vector of channels.
 *
 * EVT is the addr dimension (max SOE_HLD_ROWS, 64)
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#ifndef XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEHT_H_
#define XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEHT_H_

#include "xrmSliceCommon.h"

#define PS_HT_RAW_INPUT	"HT_RAW_INPUT"   /** single port==0 single addr=0 */

class XrmSliceHT: public XrmSliceCommon {

protected:
	int P_HT_RAW_INPUT;

	size_t ht_buf_len;
	epicsUInt32* ht_raw;

	virtual void task();
	static void task_runner(void *drvPvt);

	bool ready_to_slice();
	static int nice;
public:
	XrmSliceHT(const char *portName);
	virtual ~XrmSliceHT() {}

	virtual asynStatus writeInt32Array(
			asynUser *pasynUser, epicsInt32 *value, size_t nElements);
};



#endif /* XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEHT_H_ */
