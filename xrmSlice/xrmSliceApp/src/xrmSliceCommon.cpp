/*
 * xrmSliceCommon.cpp
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#include "xrmSliceCommon.h"

static const char *driverName= __FILE__;
#define DN	driverName
#define FN	__FUNCTION__

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

	createParam(PS_XS_SMPL_SSB,		asynParamInt32,      &P_XS_SMPL_SSB);
	createParam(PS_XS_SMPL_NSAM,		asynParamInt32,      &P_XS_SMPL_NSAM);
	createParam(PS_XS_SMPL_AI_COUNT,	asynParamInt32,	     &P_XS_SMPL_AI_COUNT);
	createParam(PS_XS_SMPL_AI_INDEX,	asynParamInt32,	     &P_XS_SMPL_AI_INDEX);
	createParam(PS_XS_SMPL_DI_COUNT,	asynParamInt32,	     &P_XS_SMPL_DI_COUNT);
	createParam(PS_XS_SMPL_DI_INDEX, 	asynParamInt32,	     &P_XS_SMPL_DI_INDEX);
	createParam(PS_XS_SMPL_SP_COUNT,	asynParamInt32,	     &P_XS_SMPL_SP_COUNT);
	createParam(PS_XS_SMPL_SP_INDEX, 	asynParamInt32,	     &P_XS_SMPL_SP_INDEX);
}

XrmSliceCommon::~XrmSliceCommon() {

}

SamplePrams XrmSliceCommon::sample_prams;
SamplePrams XrmSliceCommon::sample_prams_field_has_been_written;

#define SET_SAMPLE_PRAMS_FIELD(function, FIELD) \
	if (function == P_XS_SMPL_##FIELD) {			\
	    sample_prams.FIELD = value;				\
	    sample_prams_field_has_been_written.FIELD = 1;	\
	    break;						\
	}


asynStatus XrmSliceCommon::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	    int function = pasynUser->reason;
	    asynStatus status = asynSuccess;
	    const char *paramName;
	    int addr = 0;

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    if (maxAddr > 1){
		    status = pasynManager->getAddr(pasynUser, &addr);
		    if(status!=asynSuccess) return status;
	    }

	    /* Set the parameter in the parameter library. */
	    status = (asynStatus) setIntegerParam(addr, function, value);

	    fprintf(stderr,
	    	              "%s:%s: function=%d, addr=%d, name=%s, value=%d\n",
	    	              DN, FN, function, addr, paramName, value);

	    /** SamplePrams is meant to be a group, but here we have to handle individually.
	     *  @@todo not at all "atomic" !! @@todo
	     */
	    do {
		    SET_SAMPLE_PRAMS_FIELD(function, SSB);
		    SET_SAMPLE_PRAMS_FIELD(function, NSAM);
		    SET_SAMPLE_PRAMS_FIELD(function, AI_COUNT);
		    SET_SAMPLE_PRAMS_FIELD(function, AI_INDEX);
		    SET_SAMPLE_PRAMS_FIELD(function, DI_COUNT);
		    SET_SAMPLE_PRAMS_FIELD(function, DI_INDEX);
		    SET_SAMPLE_PRAMS_FIELD(function, SP_COUNT);
		    SET_SAMPLE_PRAMS_FIELD(function, SP_INDEX);
	    } while(0);

	    if (sample_prams.validate(sample_prams_field_has_been_written)){
		    fprintf(stderr,
				    "%s:%s: function=%d, name=%s, value=%d sample_prams valid!\n",
		    		    DN, FN, function, paramName, value);

		    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
		    	              "%s:%s: function=%d, name=%s, value=%d sample_prams valid!\n",
		    	              DN, FN, function, paramName, value);
	    }
	    /* Do callbacks so higher layers see any changes */
	    status = (asynStatus) callParamCallbacks();

	    if (status)
	        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
	                  "%s:%s: status=%d, function=%d, name=%s, value=%d",
	                  DN, FN, status, function, paramName, value);
	    else
	        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
	              "%s:%s: function=%d, name=%s, value=%d\n",
	              DN, FN, function, paramName, value);
	    return status;
}

void XrmSliceCommon::update_cal(VF& vx, epicsFloat32 *value, size_t nElements)
{
	const size_t mysize = nElements-1;
	if (vx.size() != mysize && vx.size() != 0){
		fprintf(stderr, "WARNING: CAL size change? " FMTSZT " -> " FMTSZT "\n",
				vx.size(), mysize);
		vx.clear();
	}
	for (size_t ii = 0; ii < mysize; ++ii){
		vx.push_back(value[ii+1]);
	}
}

asynStatus XrmSliceCommon::writeFloat32Array(
	asynUser *pasynUser, epicsFloat32 *value, size_t nElements)
{
	    int function = pasynUser->reason;
	    asynStatus status = asynSuccess;
	    const char *paramName;
	    int addr = 0;

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    if (maxAddr > 1){
		    status = pasynManager->getAddr(pasynUser, &addr);
		    if(status!=asynSuccess) return status;
	    }

	    if (function == P_XS_EOFF || function == P_XS_ESLO) {
		lock();
		update_cal(function==P_XS_EOFF? p_eoff: p_eslo, value, nElements);
		unlock();
		epicsEventSignal(eventId);
	    } else {
	        // Fall back to base class for standard parameters
	        status = XrmSliceCommon::writeFloat32Array(pasynUser, value, nElements);
	    }

	    /* Do callbacks so higher layers see any changes */
	    status = (asynStatus) callParamCallbacks();

	    if (status)
	        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
	                  "%s:%s: status=%d, function=%d, name=%s, value=%p",
	                  DN, FN, status, function, paramName, value);
	    else
	        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
	              "%s:%s: function=%d, name=%s, value=%p\n",
	              DN, FN, function, paramName, value);
	    return status;
}
