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

#define PS_SOE_HLD_ENT_PV_ID    	"SOE_HLD_COL_PV_ID"
#define PS_SOE_HLD_ENT_CLIDAT    	"SOE_HLD_COL_CLIDAT"
#define PS_SOE_HLD_ENT_TS        	"SOE_HLD_COL_TS"
#define PS_SOE_HLD_ENT_DATA_OFFSET 	"SOE_HLD_COL_DATA_OFFSET"

class XrmSliceHT: public XrmSliceCommon {

protected:
	int P_HT_RAW_INPUT;

	size_t ht_buf_len;
	epicsUInt32* ht_raw;

	int P_SOE_HLD_ENT_PV_ID;
	int P_SOE_HLD_ENT_CLIDAT;
	int P_SOE_HLD_ENT_TS;
	int P_SOE_HLD_ENT_DATA_OFFSET;

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
