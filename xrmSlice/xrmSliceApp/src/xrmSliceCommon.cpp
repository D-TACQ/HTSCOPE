/** @file xrmSliceCommon.cpp
 *  @brief impl for xrmSlice Common ABC.
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#include "xrmSliceCommon.h"
#include "acq-util.h"
#include <string.h>

static const char *driverName= __FILE__;
#define DN	driverName
#define FN	__FUNCTION__

int XrmSliceCommon::verbose = ::getenv_default("XrmSliceCommon_VERBOSE", 0);

typedef std::map<const std::string, UUT_Prams*> UutPramsMap;

#define CS "0123456789"

static const char* _getUutId(const char* portName){
	static char id[8];
	int i0 = strcspn(portName, CS);
	int i1 = strspn(portName+i0, CS);

	strncpy(id, portName, i0+i1);
	return id;
}
UUT_Prams& XrmSliceCommon::getUutPrams(const char* portName)
{
	static UutPramsMap uut_prams_map;

	const char* uut_id = _getUutId(portName);

	if (uut_prams_map.count(uut_id) == 0){
		UUT_Prams* prams = new UUT_Prams;
		uut_prams_map[uut_id] = prams;
	}

	if (verbose > 1){
		fprintf(stderr, "%s iterate map count:%u\n", FN, uut_prams_map.size());
		for (auto const &ent: uut_prams_map){
			fprintf(stderr, "%s map[%s] -> %p\n", FN, ent.first.c_str(), ent.second);
		}

	}
	if (verbose){
		fprintf(stderr, "%s map[%s] return *%p\n", FN, uut_id, uut_prams_map[uut_id]);
	}
	return *uut_prams_map[uut_id];
}

bool XrmSliceCommon::wait_and_lock()  {
	epicsEventWait(eventId);
	lock();
	return true;
}

XrmSliceCommon::XrmSliceCommon(const char* portName, int max_addr):
	acq400_asynPortDriver(portName,
	/* maxAddr */		max_addr,    /* number of elements */
	/* Interface mask */    asynEnumMask|asynOctetMask|asynUInt32DigitalMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|
				asynFloat32ArrayMask|asynInt64ArrayMask|asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynUInt32DigitalMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|
				asynFloat32ArrayMask|asynInt64ArrayMask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0),
	uut_p(getUutPrams(portName))
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
	fprintf(stderr, "%s final param: %s %d\n", FN, PS_XS_SMPL_SP_INDEX, P_XS_SMPL_SP_INDEX);
}

XrmSliceCommon::~XrmSliceCommon() {

}

/* @@todo .. the static members are convenient, but limit us to ONE peer per IOC */
#ifdef PGMCOMOUT
SamplePrams XrmSliceCommon::sample_prams;
SamplePrams XrmSliceCommon::sample_prams_field_has_been_written;

VF XrmSliceCommon::eslo;      // index from zero
VF XrmSliceCommon::eoff;
#endif

#define SET_SAMPLE_PRAMS_FIELD(function, FIELD) \
	if (function == P_XS_SMPL_##FIELD) {			\
		uut_p.sample_prams.FIELD = value;				\
		uut_p.sample_prams_field_has_been_written.FIELD = 1;	\
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

	    if (uut_p.sample_prams.validate(uut_p.sample_prams_field_has_been_written)){
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
	if (vx.size()){
		if (vx.size() != mysize){
			fprintf(stderr, "WARNING: CAL size change? " FMTSZT " -> " FMTSZT "\n",
							vx.size(), mysize);
		}
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
		if (verbose) fprintf(stderr, "%s function:%d:%s nelems:" FMTSZT "\n",
			FN, function,
			    function==P_XS_EOFF? PS_XS_EOFF:
			    function==P_XS_ESLO? PS_XS_ESLO: "unknown", nElements);
		update_cal(function==P_XS_EOFF? uut_p.eoff: uut_p.eslo, value, nElements);
		unlock();
		//epicsEventSignal(eventId);
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
