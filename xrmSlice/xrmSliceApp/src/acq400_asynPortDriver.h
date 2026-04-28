/*
 * acq400_asynPortDriver.h
 *
 *  Created on: 6 Mar 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_ASYNPORTDRIVER_H_
#define XRMIOCAPP_SRC_ACQ400_ASYNPORTDRIVER_H_

#include "acq400_asyn_common.h"

#define PS_RUNSTOP	"RUNSTOP"	/* asynInt32, r/w */
#define PS_UPDATES	"UPDATES"	/* asynInt32, r/c */
#define PS_TS_USEC	"TS_USEC"       /* asynInt32, ro  */
#define PS_MON_RL	"MON_RL"	/* asynInt32, r/w, "Monitor Rate Limit" */

class acq400_asynPortDriver: public asynPortDriver {
protected:
	asynStatus gip(int pnum, int* pram);
	asynStatus gip(int addr, int pnum, int* pram);
	asynStatus sip(int addr, int pnum, int pram);
	asynStatus sip(int addr, int pnum, unsigned pram);
	asynStatus sip(int addr, int pnum, epicsInt64 pram);

	asynStatus gsp(int pnum, int maxchar, char* str);
	asynStatus ssp(int pnum, const char* str);

	epicsEventId eventId;

	int P_RUNSTOP;
	int P_UPDATES;
	int P_TS_USEC;
	int P_MON_RL;

	class MonitorRateLimit {
		epicsTimeStamp et0;
		bool go_ahead;
	public:
		enum MRL {
			DIS_MON,
			LIM_1Hz,
			LIM_2Hz,
			LIM_5Hz,
			LIM_10Hz,
			LIM_NOLIM
		};

		MonitorRateLimit();
		bool goAhead() {
			return go_ahead;
		}
		void newData(int mrl);
	};
	int mrl_param;                    // MUST be explicitly set by client ::writeInt32()
public:
	acq400_asynPortDriver(const char *portName, int maxAddr, int interfaceMask, int interruptMask,
			int asynFlags, int autoConnect, int priority, int stackSize);
	virtual ~acq400_asynPortDriver() {}
};


#endif /* XRMIOCAPP_SRC_ACQ400_ASYNPORTDRIVER_H_ */
