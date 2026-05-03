/*
 * xrmSliceCommon.h
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#ifndef XRMSLICE_XRMSLICEAPP_SRC_XRMSLICECOMMON_H_
#define XRMSLICE_XRMSLICEAPP_SRC_XRMSLICECOMMON_H_

#include "SamplePrams.h"
#include "acq400_asyn_common.h"

#define PS_XS_UPTIME		"XS_UPTIME"
#define PS_XS_EOFF    		"XS_EOFF"
#define PS_XS_ESLO    		"XS_ESLO"

#define PS_XS_SMPL_SSB		"XS_SMPL_SSB"
#define PS_XS_SMPL_NSAM		"XS_SMPL_NSAM"
#define PS_XS_SMPL_AI_COUNT 	"XS_SMPL_AI_COUNT"
#define PS_XS_SMPL_AI_INDEX 	"XS_SMPL_AI_INDEX"
#define PS_XS_SMPL_DI_COUNT 	"XS_SMPL_DI_COUNT"
#define PS_XS_SMPL_DI_INDEX 	"XS_SMPL_DI_INDEX"
#define PS_XS_SMPL_SP_COUNT 	"XS_SMPL_SP_COUNT"
#define PS_XS_SMPL_SP_INDEX 	"XS_SMPL_SP_INDEX"

/* these parameters are defined in subclasses (type varies scalar/vector)
 * but the defs are here for DRY reasons.
 */

#define PS_XS_AI16_CH_RAW	"XS_AI16_CH_RAW"
#define PS_XS_AI16_CH_EGU	"XS_AI16_CH_EGU"
#define PS_XS_DI32_CH_RAW	"XS_DI32_CH_RAW"
#define PS_XS_SP32_SP0		"XS_SP32_SP0"
#define PS_XS_SP32_SP1		"XS_SP32_SP1"
#define PS_XS_SP32_SP2		"XS_SP32_SP2"
#define PS_XS_SP32_SP3		"XS_SP32_SP3"
#define PS_XS_SP32_WRVS		"XS_SP32_WRVS"  // WR Vernier, seconds
#define PS_XS_SP32_WRVT     	"XS_SP32_WRVT"  // WR Vernier, ticks
#define PS_XS_SP32_WRUS     	"XS_SP32_WRUS"  // WR time, usec since epoch


class XrmSliceCommon: public acq400_asynPortDriver {
	static SamplePrams sample_prams_field_has_been_written;
protected:
	static SamplePrams sample_prams;

	int P_XS_UPTIME;
	int P_XS_EOFF;
	int P_XS_ESLO;

	int P_XS_SMPL_SSB;
	int P_XS_SMPL_NSAM;
	int P_XS_SMPL_AI_COUNT;
	int P_XS_SMPL_AI_INDEX;
	int P_XS_SMPL_DI_COUNT;
	int P_XS_SMPL_SP_COUNT;
	int P_XS_SMPL_DI_INDEX;
	int P_XS_SMPL_SP_INDEX;

	/* set by subclass */
	int P_XS_AI16_CH_RAW;
	int P_XS_AI16_CH_EGU;
	int P_XS_DI32_CH_RAW;
	int P_XS_SP32_SP0;
	int P_XS_SP32_SP1;
	int P_XS_SP32_SP2;
	int P_XS_SP32_SP3;
	int P_XS_SP32_WRVS;
	int P_XS_SP32_WRVT;
	int P_XS_SP32_WRUS;
public:
	XrmSliceCommon(const char *portName, int max_addr);
	virtual ~XrmSliceCommon();

	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	/**< clients MUST call if own writeInt32 does not "see function" */
};

#endif /* XRMSLICE_XRMSLICEAPP_SRC_XRMSLICECOMMON_H_ */

