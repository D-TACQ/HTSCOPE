/*
 * xrmSliceCommon.h
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#ifndef XRMSLICE_XRMSLICEAPP_SRC_XRMSLICECOMMON_H_
#define XRMSLICE_XRMSLICEAPP_SRC_XRMSLICECOMMON_H_


#include "acq400_asyn_common.h"

#define PS_XS_UPTIME		"XS_UPTIME"
#define PS_XS_EOFF    		"XS_EOFF"
#define PS_XS_ESLO    		"XS_ESLO"

#define PS_XS_AGG_SITES		"XS_AGG_SITES"
#define PS_XS_SITE_SSB		"XS_SITE_SSB" /* addr per site */
#define PS_XS_SMPL_NSAM		"XS_SMPL_NSAM"
#define PS_XS_SITE_IS_ADC	"XS_SITE_IS_ADC"  /* addr per site */
#define PS_XS_SMPL_SS_U32	"XS_SMPL_SS_U32"
#define PS_XS_SMPL_AI_COUNT 	"XS_SMPL_AI_COUNT"
#define PS_XS_SMPL_DI_COUNT 	"XS_SMPL_DI_COUNT"
#define PS_XS_SMPL_SP_COUNT 	"XS_SMPL_SP_COUNT"
#define PS_XS_SMPL_DI_INDEX 	"XS_SMPL_DI_INDEX"
#define PS_XS_SMPL_SP_INDEX 	"XS_SMPL_SP_INDEX"


class XrmSliceCommon: public acq400_asynPortDriver {

protected:
	int P_XS_UPTIME;
	int P_XS_EOFF;
	int P_XS_ESLO;

	int P_XS_AGG_SITES;
	int P_XS_SITE_SSB;
	int P_XS_SMPL_NSAM;
	int P_XS_SITE_IS_ADC;
	int P_XS_SMPL_SS_U32;
	int P_XS_SMPL_AI_COUNT;
	int P_XS_SMPL_DI_COUNT;
	int P_XS_SMPL_SP_COUNT;
	int P_XS_SMPL_DI_INDEX;
	int P_XS_SMPL_SP_INDEX;
public:
	XrmSliceCommon(const char *portName, int max_addr);
	virtual ~XrmSliceCommon();

};


#endif /* XRMSLICE_XRMSLICEAPP_SRC_XRMSLICECOMMON_H_ */
