/*
 * acq400_asynPortDriver.cpp
 *
 *  Created on: 6 Mar 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"

static const char *driverName="acq400_asynPortDriver";

#define DN	driverName
#define FN	__FUNCTION__


acq400_asynPortDriver::acq400_asynPortDriver(const char *portName, int maxAddr, int interfaceMask, int interruptMask,
		int asynFlags, int autoConnect, int priority, int stackSize):
        asynPortDriver(portName, maxAddr, interfaceMask, interruptMask,
                   asynFlags, autoConnect, priority, stackSize),
		   mrl_param(MonitorRateLimit::LIM_NOLIM)
{
	createParam(PS_RUNSTOP, asynParamInt32, &P_RUNSTOP);
	createParam(PS_UPDATES, asynParamInt32,	&P_UPDATES);
	createParam(PS_TS_USEC, asynParamInt64,	&P_TS_USEC);
	createParam(PS_MON_RL,	asynParamInt32, &P_MON_RL);

	eventId = epicsEventCreate(epicsEventEmpty);
}

asynStatus acq400_asynPortDriver::gip(int pnum, int* pram)
{
	asynStatus status = getIntegerParam(pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s getIntegerParam %d fail\n",
				DN, FN, pnum);
		assert(status == 0);
	}
	return status;
}
asynStatus acq400_asynPortDriver::gip(int addr, int pnum, int* pram)
{
	asynStatus status = getIntegerParam(addr, pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s:%d getIntegerParam %d fail\n",
				DN, FN, addr, pnum);
		assert(status == 0);
	}
	return status;
}

asynStatus acq400_asynPortDriver::sip(int addr, int pnum, int pram)
{
	asynStatus status = setIntegerParam(addr, pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s:%d setIntegerParam %d fail\n",
				DN, FN, addr, pnum);
		assert(status == 0);
	}
	return status;
}

asynStatus acq400_asynPortDriver::sip(int addr, int pnum, unsigned pram)
{
	asynStatus status = setIntegerParam(addr, pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s:%d setIntegerParam %d fail\n",
				DN, FN, addr, pnum);
		assert(status == 0);
	}
	return status;
}

asynStatus acq400_asynPortDriver::sip(int addr, int pnum, epicsInt64 pram)
{
	asynStatus status = setInteger64Param(addr, pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s:%d setInteger64Param %d fail\n",
				DN, FN, addr, pnum);
		assert(status == 0);
	}
	return status;
}

asynStatus acq400_asynPortDriver::gsp(int pnum, int maxchar, char* str)
{
	asynStatus status = getStringParam(pnum, 80, str);
	if (status){
		fprintf(stderr, "%s:%s getStringParam fail %d\n", DN, FN, pnum);
		assert(status==0);
	}
	return status;
}

asynStatus acq400_asynPortDriver::ssp(int pnum, const char* str)
{
	asynStatus status = setStringParam(pnum, str);
	if (status){
		fprintf(stderr, "%s:%s setStringParam fail %d\n", DN, FN, pnum);
		assert(status==0);
	}
	return status;
}

acq400_asynPortDriver::MonitorRateLimit::MonitorRateLimit(): go_ahead(false) {
	epicsTimeGetCurrent(&et0);
}

//
void acq400_asynPortDriver::MonitorRateLimit::newData(int mrl) {
	double throttle_s = 0;

	switch (mrl){
	case DIS_MON:
		go_ahead = false;
		return;
	case LIM_NOLIM:
		go_ahead = true;
		return;
	case LIM_1Hz:
		throttle_s = 1.0; break;
	case LIM_2Hz:
		throttle_s = 0.5; break;
	case LIM_5Hz:
		throttle_s = 0.2; break;
	case LIM_10Hz:
		throttle_s = 0.1; break;
	default:
		assert(mrl != mrl);
	}

	epicsTimeStamp et1;
	epicsTimeGetCurrent(&et1);
	if (epicsTimeDiffGreaterThan(et1, et0, throttle_s)){
		go_ahead = true;
		et0 = et1;
	}else{
		go_ahead = false;
	}
}

