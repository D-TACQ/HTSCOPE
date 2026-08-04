/** @file xrmSliceHT.h
 *  @brief interface for the HT (Hold Table) slicer.
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 * xrmSliceHT.h: singleton
 * IN: receive latest HT as raw BLOB from UUT
 * OUT: present channelised, egu data as a vector of channels.
 *
 * EVT is the PORT dimension (max SOE_HLD_ROWS, 64)
 * CHAN is the ADDR dimension (max AI_CHAN, depends on UUT)
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#ifndef XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEHT_H_
#define XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEHT_H_

#include "xrmSliceCommon.h"
#include "xrm_structs.h"

#define PS_HT_RAW_INPUT	"HT_RAW_INPUT"   /** single port==0 single addr=0 */

#define PS_SOE_HLD_ENT_PV_ID    	"SOE_HLD_ENT_PV_ID"
#define PS_SOE_HLD_ENT_EVENT		"SOE_HLD_ENT_EVENT"
#define PS_SOE_HLD_ENT_OFFSET_US	"SOE_HLD_ENT_OFFSET_US"
#define PS_SOE_HLD_ENT_CLIDAT    	"SOE_HLD_ENT_CLIDAT"
#define PS_SOE_HLD_ENT_TS        	"SOE_HLD_ENT_TS"
#define PS_SOE_HLD_ENT_DATA_OFFSET 	"SOE_HLD_ENT_DATA_OFFSET"


class XrmSliceHT;

typedef std::vector<XrmSliceHT*> XrmSliceHT_V;
typedef std::map<const std::string, std::vector<XrmSliceHT*>*> XrmSliceHT_VM;

class XrmSliceHT: public XrmSliceCommon {
	/**
	 * for every UUT, Instantiate one instance XrmSliceHT for every HT row.
	 * for every UUT, hold one list XrmSliceHT_V of the related XrmSliceHT instances as "RowHandlers"
	 * hold the lists in rowHandlersMap, containing N UUTS entryes.
	 * EVERY XrmSliceHT self registers via registerRowHandler
	 * In practice, only the HEAD XrmSliceHT instance accesses the RowHandlers list, obtaining it from rowHandlersMap.
	 * */
	static XrmSliceHT_VM rowHandlersMap;
	bool registerRowHandler();  /**< return true is HEAD instance */

protected:

	int P_HT_RAW_INPUT;

	size_t ht_buf_len;
	epicsUInt32* ht_data;

	int P_SOE_HLD_ENT_PV_ID;
	int P_SOE_HLD_ENT_EVENT;
	int P_SOE_HLD_ENT_OFFSET_US;
	int P_SOE_HLD_ENT_CLIDAT;
	int P_SOE_HLD_ENT_TS;
	int P_SOE_HLD_ENT_DATA_OFFSET;

	virtual void task();
	static void task_runner(void *drvPvt);

	bool ready_to_slice();

	void ht_slice(XrmSliceHT& sliceHT, SOE_HOLD_HEADER* header, epicsUInt32* sample);
	/** slice a single row of the ht. runs in lock() context */

	static int nice;


	XrmSliceHT(const char *portName, int addr);
public:
	static XrmSliceHT* factory(const char* portName, int addr);

	virtual ~XrmSliceHT() {}

	virtual asynStatus writeInt32Array(
			asynUser *pasynUser, epicsInt32 *value, size_t nElements);
};



#endif /* XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEHT_H_ */
