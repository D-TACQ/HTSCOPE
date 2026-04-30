/*
 * xrmSliceHT.h: singleton
 * IN: receive latest HT as raw BLOB from UUT
 * OUT: present channelised, egu data as a vector of channels.
 *
 * EVT is the addr dimension (max SOE_HLD_ROWS, 64)
 *
 *  Created on: 30 Apr 2026
 *      Author: pgm
 */

#ifndef XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEHT_H_
#define XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEHT_H_

#include "xrmSliceCommon.h"

class XrmSliceHT: public XrmSliceCommon {

protected:



public:
	XrmSliceHT(const char *portName);
	virtual ~XrmSliceHT() {}
};



#endif /* XRMSLICE_XRMSLICEAPP_SRC_XRMSLICEHT_H_ */
