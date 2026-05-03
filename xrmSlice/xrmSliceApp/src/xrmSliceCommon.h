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


class XrmSliceCommon: public acq400_asynPortDriver {
	static SamplePrams sample_prams_field_has_been_written;
protected:
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

	static SamplePrams sample_prams;
public:
	XrmSliceCommon(const char *portName, int max_addr);
	virtual ~XrmSliceCommon();

	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	/**< clients MUST call if own writeInt32 does not "see function" */
};

#endif /* XRMSLICE_XRMSLICEAPP_SRC_XRMSLICECOMMON_H_ */

