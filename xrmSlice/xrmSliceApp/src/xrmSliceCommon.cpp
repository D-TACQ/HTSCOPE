/*
 * xrmSliceCommon.cpp
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#include "xrmSliceCommon.h"

XrmSliceCommon::XrmSliceCommon(const char* portName, int max_addr):
	acq400_asynPortDriver(portName,
	/* maxAddr */		max_addr,    /* number of elements */
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|
				asynFloat32ArrayMask|asynInt64ArrayMask|asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|
				asynFloat32ArrayMask|asynInt64ArrayMask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0)
{
	createParam(PS_XS_UPTIME,		asynParamInt32,      &P_XS_UPTIME);
	createParam(PS_XS_ESLO,			asynParamFloat32Array,&P_XS_ESLO);
	createParam(PS_XS_EOFF,			asynParamFloat32Array,&P_XS_EOFF);

	createParam(PS_XS_AGG_SITES,		asynParamOctet,      &P_XS_AGG_SITES);
	createParam(PS_XS_SITE_SSB,		asynParamInt32,      &P_XS_SITE_SSB);
	createParam(PS_XS_SITE_IS_ADC,		asynParamInt32,      &P_XS_SITE_IS_ADC);
	createParam(PS_XS_SMPL_SS_U32,		asynParamInt32,      &P_XS_SMPL_SS_U32);
	createParam(PS_XS_SMPL_NSAM,		asynParamInt32,      &P_XS_SMPL_NSAM);
	createParam(PS_XS_SMPL_AI_COUNT,	asynParamInt32,	     &P_XS_SMPL_AI_COUNT);
	createParam(PS_XS_SMPL_DI_COUNT,	asynParamInt32,	     &P_XS_SMPL_DI_COUNT);
	createParam(PS_XS_SMPL_SP_COUNT,	asynParamInt32,	     &P_XS_SMPL_SP_COUNT);
	createParam(PS_XS_SMPL_DI_INDEX, 	asynParamInt32,	     &P_XS_SMPL_DI_INDEX);
	createParam(PS_XS_SMPL_SP_INDEX, 	asynParamInt32,	     &P_XS_SMPL_SP_INDEX);

}

XrmSliceCommon::~XrmSliceCommon() {

}
