/*
 * xrm_structs.h
 *
 *  Created on: 6 Feb 2026
 *      Author: pgm
 */

#include <epicsTypes.h>

#ifndef XRMIOCAPP_SRC_XRM_STRUCTS_H_
#define XRMIOCAPP_SRC_XRM_STRUCTS_H_

/* FMT : FNAL Multicast Table
 * input from plant: 20Hz
 */

struct FMT_ROW {
	epicsUInt16 event;           // FNAL Event number
	epicsUInt16 pad;             // 32 bit alignment is best, available for future
	epicsUInt32 client_data;     // opaque value to pass back
	epicsInt64 timestamp;       // WR usec from EPOCH **
};

/* ** we're losing one bit here, but it's OK, we have time..
 * >>> usec_per_year=365*24*3600*1000
 * >>> max_int64 = 2**63
 * max_int64//usec_per_year
292471208
 * this is probably why CERN has integer nsec :-)
 */
const epicsUInt16 EV99 = 65535U;	     // denotes last event in table.

const int FMT_ROWS = 64;
//#define FMT_ROWS 64

typedef struct FMT_ROW  FMT[FMT_ROWS];

/* SOE_LUT : Sample On Event Lookup Table
 * input from plant: as required (EPICS PV)
 */
struct SOE_LUT_ROW {
	epicsUInt16 event;           // FNAL Event number
	epicsUInt16 pad;             // 32 bit alignment is best, available for future
	epicsUInt32 pv_id;
	epicsInt32 offset_us;        // Before or After Event, to the limit of data this cycle
};

const int SOE_LUT_ROWS = 64;

typedef struct SOE_LUT_ROW  SOE_LUT[SOE_LUT_ROWS];

/** SOE HOLD TABLE
 * This is the OUTPUT from each CYCLE
 * For N events, the OUTPUT comprises:
 * N+1 SOE_HOLD_HEADER rows, headers for N events + 1 row of zeros
 * N RAW SAMPLE rows.
 *
 * The data structure has N fixed format HEADER, a delimiter tow and N raw sample entries.
 * The HEADER includes the geometry of the raw sample entries, so the data is self-describing.
 *
 * The RAW SAMPLE row varies per unit type, the fixed header includes info to access the RAW SAMPLE.
 * We prefer to offer the RAW sample because
 * 1. Blitting off a row of data is our most efficient transfer
 * 2. No conversion to EGU's. User to do that thanks to $UUT:*:EOFF,ESLO
 * 3. RAW sample includes METADATA for checking purposes.
 *
 * To interpret a received HOLD DATA:
 * iterate the SOE_HOLD_HEADER rows until zero
 * use data_offset to access the data.
 *
 * Actual wire protocol:
 * We meet the letter of the requirement by sending as a PVA ARRAY of U32
 * where NORD gives the overall size of the table, including DATA.
 *
 * We've attempted to meet the spirit of the requirement using the PVXS API
 * to create an Array of Groups, but this has not been a success.
 * Happy to revisit later when we have an example that works.
 */

struct SOE_HOLD_HEADER {
	epicsUInt32 pv_id;		// links Event and Offset
	epicsUInt32 client_data;	// copied from FMT (if required)
	epicsInt64 timestamp;		// cross check: which FMT update this derives from.
	epicsUInt16 data_offset;	// offset of RAW DATA in u32 from start of table.
	/* description of raw sample from hardware
	 * it's not totally raw because all AI are presented as calibrated V.
	 * but after that a series of U32 representing DI, SPAD
	 * this is not in the spec, but will be useful for validation.
	 */
	epicsUInt8  ss_u32;		// sample size  (u32)
	epicsUInt8  ai_count;           // number of AI (floats) in data
	epicsUInt8  di_count;           // number of DI (u32) in data
	epicsUInt8  sp_count;           // number of SP (u32) in data
};

const int SPAD0_SC = 0;                   // SPAD[0] is sample count (u32)
const int SPAD1_TS = 1;                   // SPAD[1] is WR TS 3 bit seconds, 28 bit ticks

const int SOE_HLD_ROWS = 64;

typedef epicsUInt32 	U32;


typedef struct SOE_HOLD_HEADER* SOE_HOLD_TABLE;   // many more than 1 of course..


/* For data allocation, what is the MAXIMUM size of table:
 * HOLD_MAXSIZE = 64*20+64*(128*2+16*4) =  21760B = 6400LW
 * */

static inline const int HOLD_DATA_OFF() { return (SOE_HLD_ROWS+1)*sizeof(SOE_HOLD_HEADER); };
static inline const int HOLD_MAXSIZE(unsigned ssb) { return  HOLD_DATA_OFF() + SOE_HLD_ROWS*ssb; }
static inline const int HOLD_MAX_NELM(unsigned ssb) { return HOLD_MAXSIZE(ssb)/sizeof(long); }



/* actual sample data:
 */

const int XRM_MAGPS_AI32 = 32;
const int XRM_MAGPS_DI32 =  2;
const int XRM_MAGPS_SP32 =  6;   // pad to round number

struct XRM_MAGPS_SAMPLE {
	epicsFloat32 ai[XRM_MAGPS_AI32];
	epicsUInt32  di[XRM_MAGPS_DI32];
	epicsUInt32  sp[XRM_MAGPS_SP32];
};


const int XRM_QPMS_AI32 = 128;
const int XRM_QPMS_DI32 =   0;
const int XRM_QPMS_SP32 =   8;

struct XRM_QPMS_SAMPLE{
	epicsFloat32 ai[XRM_MAGPS_AI32];
	epicsUInt32  di[XRM_MAGPS_DI32];
	epicsUInt32  sp[XRM_MAGPS_SP32];
};

const int XRM_INST_A_AI32 = 32;
const int XRM_INST_A_DI32 =  0;
const int XRM_INST_A_SP32 =  4;

struct XRM_INST_A_SAMPLE {
	epicsFloat32 ai[XRM_INST_A_AI32];
	epicsUInt32  di[XRM_INST_A_DI32];
	epicsUInt32  sp[XRM_INST_A_SP32];
};

const int XRM_INST_B_AI32 = 32;
const int XRM_INST_B_DI32 =  1;
const int XRM_INST_B_SP32 =  3;

struct XRM_INST_B_SAMPLE {
	epicsFloat32 ai[XRM_INST_A_AI32];
	epicsUInt32  di[XRM_INST_A_DI32];
	epicsUInt32  sp[XRM_INST_A_SP32];
};


void print(FMT& fmt, bool verbose = false);
void print(SOE_LUT& lut, bool verbose = false);
void print(SOE_HOLD_HEADER* ht, bool verbose = false);




#endif /* XRMIOCAPP_SRC_XRM_STRUCTS_H_ */
