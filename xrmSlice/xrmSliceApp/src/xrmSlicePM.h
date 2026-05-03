/*
 * xrmSlicePM.h  : one instance per PM delay (20 instances expected)
 * IN: receive PM as raw BLOB from UUT
 * OUT: presents channelised, EGU data. Channel is the ADDR dimension (max 128).
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#ifndef XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEPM_H_
#define XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEPM_H_


#include "xrmSliceCommon.h"

class XrmSlicePM: public XrmSliceCommon {

protected:


public:
	XrmSlicePM(const char *portName, int max_addr);
	virtual ~XrmSlicePM() {}

};
#endif /* XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEPM_H_ */
