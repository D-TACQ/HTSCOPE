/*
 * acq400_asyn_common.h
 *
 *  Created on: 21 Feb 2025
 *      Author: pgm
 */

#ifndef ACQ400IOCAPP_SRC_ACQ400_ASYN_COMMON_H_
#define ACQ400IOCAPP_SRC_ACQ400_ASYN_COMMON_H_

#include <epicsExport.h>
#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <iocsh.h>

#include <vector>
#include <split2.h>

#include "asynPortDriver.h"
#include "acq400_asynPortDriver.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define SP0	0
#define SP1	1
#define SP2	2
#define SP3	3

typedef short AI16_t;
typedef unsigned long DI32_t;
typedef unsigned long DO32_t;
typedef unsigned long SP32_t;

static inline
std::vector<int> csv2int(const char* csv) {
	std::vector<std::string> strings;
	split2(csv, strings, ',');
	std::vector<int> vi;
	for (std::string s: strings){
		vi.push_back(stoi(s));
	}
	return vi;
}

static inline
bool epicsTimeDiffLessThan(epicsTimeStamp& t1, epicsTimeStamp& t0, double tgts)
{
	epicsTime et1 = t1;
	epicsTime et0 = t0;


	return (et1 - et0) < tgts;
}

static inline
bool epicsTimeDiffGreaterThan(epicsTimeStamp& t1, epicsTimeStamp& t0, double tgts)
{
	epicsTime et1 = t1;
	epicsTime et0 = t0;


	return (et1 - et0) > tgts;
}

#define TICKSPERUS	40
#define USPS		1000000


static inline
epicsInt64 getWrTsUs(unsigned wrse, unsigned wrv)
/**< create full Timestamp from <wrse> White Rabbit Seconds from Epoch and
 * <wrv> : White Rabbit Vernier, the coded output from the WR firmware.
 */
{
	unsigned sec = (wrv >> 28)&0x07;
	if ((wrse&7) == sec){
		sec = wrse;
	}else if (((++wrse)&7) == sec){
		sec = wrse;
	}else{
		// this is going to be REALLY obvious!
	}
	int usec = (wrv&0x0fffffff)/TICKSPERUS;
	epicsInt64 ts = ((epicsInt64)sec)*USPS + usec;
	return ts;
}

#define NSPERTICK	25
#define NSPS		1000000000

static inline
epicsInt64 getWrTsNs(unsigned wrse, unsigned wrv)
/**< create full Timestamp from <wrse> White Rabbit Seconds from Epoch and
 * <wrv> : White Rabbit Vernier, the coded output from the WR firmware.
 * wrv max = 40e6. 40e6 * 25 = 0x369aca00 -> fits a u32
 * .. our 28 bit vernier is good to 250MHz.
 */
{
	unsigned sec = (wrv >> 28)&0x07;
	if ((wrse&7) == sec){
		sec = wrse;
	}else if (((++wrse)&7) == sec){
		sec = wrse;
	}else{
		// this is going to be REALLY obvious!
	}
	unsigned nsec = (wrv&0x0fffffff)*NSPERTICK;
	epicsInt64 ts = ((epicsInt64)sec)*NSPS + nsec;
	return ts;
}

static inline
epicsInt64 getWrTs(unsigned wrse, unsigned wrv)
{
	return getWrTsUs(wrse, wrv);
}

#endif /* ACQ400IOCAPP_SRC_ACQ400_ASYN_COMMON_H_ */
