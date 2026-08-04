/** @file xrm_structs.h
 * @brief Declarations for key XRM structures for internal and wire (external) use.
 * - @ref FMT
 * - @ref SOE_LUT
 * - @ref SOE_HOLD_TABLE
 * xrm_structs.h
 *
 *  Created on: 6 Feb 2026
 *      Author: pgm
 */

#include <epicsTypes.h>

#ifndef XRMIOCAPP_SRC_XRM_STRUCTS_H_
#define XRMIOCAPP_SRC_XRM_STRUCTS_H_

#include <cstring>			// memset()

/** @brief FMT_ROW defines a row of FMT:  */
struct FMT_ROW {
	epicsUInt16 event;           /**< FNAL Event number					*/
	epicsUInt16 pad;             /**< 32 bit alignment is best, available for future 	*/
	epicsUInt32 client_data;     /**< opaque value to pass back				*/
	epicsInt64 timestamp;        /**< 64 bit WR timestamp in usec from EPOCH		*/
};

/* ** we're losing one bit here, but it's OK, we have time..
 * >>> usec_per_year=365*24*3600*1000
 * >>> max_int64 = 2**63
 * max_int64//usec_per_year
292471208
 * this is probably why CERN has integer nsec :-)
 */
const epicsUInt16 EV99 = 65535U;	     // denotes last event in table.

/** define number of rows in FMT. */
const int FMT_ROWS = 64;
//#define FMT_ROWS 64

/** @brief FMT : FNAL Multicast Table

 * input from plant: 20Hz

 * This is the binary implementation that goes out on the wire.

 * each row of the FMT is a @ref struct FMT_ROW.

 ROW  | event | pad | client_data | timestamp
------|-------|-----|-------------|----------
 0    | u16   | u16 | u32         | int64
 1    | u16   | u16 | u32         | int64
 2    | u16   | u16 | u32         | int64
 ..   | ...   | ... | ...         | ...
 64   | u16   | u16 | u32         | int64

 */
typedef struct FMT_ROW  FMT[FMT_ROWS];

static inline void clean(FMT fmt) {
	memset(fmt, 0, sizeof(FMT));
}

/** @brief SOE_LUT_ROW defines a row of SOE_LUT. */
struct SOE_LUT_ROW {
	epicsUInt16 event;           /**< FNAL Event number.					*/
	epicsUInt16 pad;             /**< 32 bit alignment is best, available for future.	*/
	epicsUInt32 pv_id;	     /**< PV id match event to this PV				*/
	epicsInt32 offset_us;        /**< time offset Before or After Event, to the limit of data this cycle */
};

const int SOE_LUT_ROWS = 64;

/** @brief SOE_LUT Sample On Event Lookup Table definition.
 *
 * matches events to PV's with a selectable time offset.

 * input from user at user-timescale.

 * in EPICS, we provide a "ROW_EDIT" PV to enable update, this works well for Phoebus and P4P

 * @@todo: unsure how to make a direct pvput for the whole table.
  - Currently this would have to be an array of u32
  - Don't know how to make a EPICS V4 array for this
  - Workaround: the "ROW EDIT" is available to change a row at a time, or a set of rows.

 ROW  | event | pad | pv_id | offset_us
------|-------|-----|-------|----------
 0    | u16   | u16 | u32   | int32
 1    | u16   | u16 | u32   | int32
 2    | u16   | u16 | u32   | int32
 ..   | ...   | ... | ...   | ...
 64   | u16   | u16 | u32   | int32

 */
typedef struct SOE_LUT_ROW  SOE_LUT[SOE_LUT_ROWS];

/** @brief SOE_HOLD_HEADER Header for Hold Table.
 * lut_row is a copy of the relevant SOE_LUT_ROW, for traceability
 * + we embed a version id 'S'<<8 | VERSION to detect current and future updates.
 *
 */

struct SOE_HOLD_HEADER {
	SOE_LUT_ROW lut_row;		/**< LUT entry that triggered this HOLD */
	epicsUInt32 client_data;	/**< copied from FMT (if required) @todo more required?	*/

	epicsInt64 timestamp;		/**< cross check: which FMT update this derives from.	*/
	epicsUInt16 data_offset;	/**< offset of RAW DATA in u32 from start of table.	*/
	/* description of raw sample from hardware
	 * it's not totally raw because all AI are presented as calibrated V.
	 * but after that a series of U32 representing DI, SPAD
	 * this is not in the spec, but will be useful for validation.
	 */
	epicsUInt8  ss_u32;		/**< sample size  (u32)					*/
	epicsUInt8  ai_count;           /**< number of AI (floats) in data			*/
	epicsUInt8  di_count;           /**< number of DI (u32) in data				*/
	epicsUInt8  sp_count;           /**< number of SP (u32) in data	Scratch Pad (meta-data) */
};

#define SOE_HOLD_HEADER_VERSION	1

static inline int setVersion(SOE_HOLD_HEADER& soe)
{
	soe.lut_row.pad = ('S'<<8) | SOE_HOLD_HEADER_VERSION;
	return SOE_HOLD_HEADER_VERSION;
}

static inline int getVersion(SOE_HOLD_HEADER& soe)
{
	if (soe.lut_row.pad>>8 == 'S'){
		return soe.lut_row.pad&0x0ff;
	}
	return 0;
}


const int SPAD0_SC = 0;                   /**< SPAD[0] is sample count (u32)			*/
const int SPAD1_TS = 1;                   /**< SPAD[1] is WR TS 3 bit seconds, 28 bit ticks	*/

const int SOE_HLD_ROWS = 64;

typedef epicsUInt32 	U32;

/** @brief SOE_HOLD_TABLE
 - This is the OUTPUT from each CYCLE

 - For N events, the OUTPUT comprises:

  1. N+1 SOE_HOLD_HEADER rows, headers for N events + 1 row of zeros (DELIMITER)
  2. N RAW SAMPLE rows

 - The HEADER includes the geometry of the raw sample entries, so the data is self-describing.

 - The RAW SAMPLE row varies per unit type, the fixed header includes info to access the RAW SAMPLE.

 - We prefer to offer the RAW sample because

  + Blitting off a row of data is our most efficient transfer
  + No conversion to EGU's. User to do that thanks to $UUT:*:EOFF,ESLO
  + RAW sample includes METADATA for checking purposes.

 - To interpret a received HOLD DATA:

  + iterate the SOE_HOLD_HEADER rows until zero

  + use data_offset to access the data.

 - In summary, the memory layout looks like this:

 ROW  | pv_id | client_data | timestamp | data_offset | ss_u32 | ai_count | di_count | sp_count
------|-------|-------------|-----------|-------------|--------|----------|----------|---------
 0    | u32   | u32         | int64     | u16         | u8     | u8       | u8       | u8
 1    | u32   | u32         | int64     | u16         | u8     | u8       | u8       | u8
 ...  | u32   | u32         | int64     | u16         | u8     | u8       | u8       | u8
 DEL  | 0     | 0           | 0         | 0           | 0      | 0        | 0        | 0

 RAW  | ai | di | sp
 -----|----|----|---
 R0   | i16|u32 |U32
 R1   | i16|u32 |U32


```
  Example 4 entries
  sizeof(SOE_HOLD_HEADER==22)
  XRMMAGPS: SSB=128 ruler in byte*2:
  00000111112222233333444445555566666777778888899999AAAAABBBBBCCCC
  0246802468024680246802468024680246802468024680246802468024680246
 |SOE HOLD 1 |
 |SOE HOLD 2 |
 |SOE HOLD 3 |
 |SOE HOLD 4 |
 |00000000000|
 |RAW SMPL 1 AIAIAIAIAIAIAIAIAIAIAIDIDISPSPSPSPSPSPSPSPSPSPSPSPSP|
 |RAW SMPL 2 AIAIAIAIAIAIAIAIAIAIAIDIDISPSPSPSPSPSPSPSPSPSPSPSPSP|
 |RAW SMPL 3 AIAIAIAIAIAIAIAIAIAIAIDIDISPSPSPSPSPSPSPSPSPSPSPSPSP|
 |RAW SMPL 4 AIAIAIAIAIAIAIAIAIAIAIDIDISPSPSPSPSPSPSPSPSPSPSPSPSP|


 showing the SOE HOLD struct on a line of bytes
 0123456789012345678901|24680246802468024680246802468024680246802468024680246
 PVID                  |    epicsUInt32 pv_id;
 ____CLID______________|    epicsUInt32 client_data;
 ________TIMSTAMP______|    epicsInt64 timestamp;
 ________________DO____|    epicsUInt16 data_offset;
 __________________SADP|    epicsUInt8  ss_u32, ai_count, di_count, sp_count;

 ```

 Actual wire protocol:
 - We meet the letter of the requirement by sending as a PVA ARRAY of U32
  - where NORD gives the overall size of the table, including DATA.

 - In the above example, MAGPS with 4 EVENTS,
   - NORD = (5 * sizeof(SOE_HOLD_HEADER))/sizeof(int32) + 4*SSB/sizeof(int32)
   - NORD = (5 * 22)/4 + 128
   - NORD = 156 @@todo 22 is not a good size for the structure..

 @@todo We've attempted to meet the spirit of the requirement using the PVXS API
 to create an Array of Groups, but this has not been a success.
 Happy to revisit later when we have an example that works.
 */
typedef struct SOE_HOLD_HEADER* SOE_HOLD_TABLE;   // many more than 1 of course..


/* For data allocation, what is the MAXIMUM size of table:
 * HOLD_MAXSIZE = 64*20+64*(128*2+16*4) =  21760B = 6400LW
 * */

static inline const int HOLD_DATA_OFF() { return (SOE_HLD_ROWS+1)*sizeof(SOE_HOLD_HEADER); };
static inline const int HOLD_MAXSIZE(unsigned ssb) { return  HOLD_DATA_OFF() + SOE_HLD_ROWS*ssb; }
static inline const int HOLD_MAX_NELM(unsigned ssb) { return HOLD_MAXSIZE(ssb)/sizeof(long); }



/* actual sample data:
 */

const int XRM_MAGPS_AI16 = 32;
const int XRM_MAGPS_DI32 =  2;
const int XRM_MAGPS_SP32 =  6;   // pad to round number

struct XRM_MAGPS_SAMPLE {
	epicsInt16  ai[XRM_MAGPS_AI16];
	epicsUInt32 di[XRM_MAGPS_DI32];
	epicsUInt32 sp[XRM_MAGPS_SP32];
};


const int XRM_QPMS_AI16 = 128;
const int XRM_QPMS_DI32 =   0;
const int XRM_QPMS_SP32 =   8;

struct XRM_QPMS_SAMPLE{
	epicsInt16  ai[XRM_MAGPS_AI16];
	epicsUInt32 di[XRM_MAGPS_DI32];
	epicsUInt32 sp[XRM_MAGPS_SP32];
};

const int XRM_INST_A_AI16 = 32;
const int XRM_INST_A_DI32 =  0;
const int XRM_INST_A_SP32 =  4;

struct XRM_INST_A_SAMPLE {
	epicsInt16  ai[XRM_INST_A_AI16];
	epicsUInt32 di[XRM_INST_A_DI32];
	epicsUInt32 sp[XRM_INST_A_SP32];
};

const int XRM_INST_B_AI16 = 32;
const int XRM_INST_B_DI32 =  1;
const int XRM_INST_B_SP32 =  3;

struct XRM_INST_B_SAMPLE {
	epicsInt16  ai[XRM_INST_A_AI16];
	epicsUInt32 di[XRM_INST_A_DI32];
	epicsUInt32 sp[XRM_INST_A_SP32];
};


void print(FMT& fmt, bool verbose = false);
void print(SOE_LUT& lut, bool verbose = false);
void print(SOE_HOLD_HEADER* ht, bool verbose = false);




#endif /* XRMIOCAPP_SRC_XRM_STRUCTS_H_ */
