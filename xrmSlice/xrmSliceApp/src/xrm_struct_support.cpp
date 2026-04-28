/*
 * xrm_struct_support.cpp
 *
 *  Created on: 6 Feb 2026
 *      Author: pgm
 */

#include <stdio.h>
#include "xrm_structs.h"

void print(FMT& fmt, bool verbose)
{
	for (int row = 0; row < FMT_ROWS; ++row){
		if (fmt[row].event != 0){
			printf("%2d: $%04x\t0x%04x\t0x%08x\t%llu\n",
				row,
				fmt[row].event,
				fmt[row].pad,
				fmt[row].client_data,
				fmt[row].timestamp);
		}
	}
}
void print(SOE_LUT& lut, bool verbose)
{
	for (int row = 0; row < SOE_LUT_ROWS; ++row){
		if (lut[row].event != 0){
			printf("%2d: $%04x\t%0x04x\t%10d\t%d\n",
				row,
				lut[row].event,
				lut[row].pad,
				lut[row].pv_id,
				lut[row].offset_us);
		}
	}
}

#define SHOW_SP	3   // always show this many SP32 values

void print(SOE_HOLD_HEADER* ht, bool verbose)
{
	for (int row = 0; row < SOE_HLD_ROWS; ++row){
		if (!ht[row].pv_id){
			break;
		}

		printf("%2d: %10d 0x%08x %3d %2d %2d %llu\n",
			row,
			ht[row].pv_id,
			ht[row].client_data,
			ht[row].data_offset,
			ht[row].ai_count,
			ht[row].di_count,
			ht[row].timestamp);
	}

	for (int row = 0; row < SOE_HLD_ROWS; ++row){
		if (!ht[row].pv_id){
			break;
		}

		printf("%2d: ", row);
		short* ai = (short*)((long*)ht + ht[row].data_offset);
		for (int ic = 0; ic < ht[row].ai_count; ++ic){
			printf("%6.4f ", *ai++ * 10.0/32768);
		}
		epicsUInt32* di = (epicsUInt32*)ai;
		for (int ix = 0; ix < ht[row].di_count+SHOW_SP; ++ix){
			printf("0x%08x ", *di++);
		}
		printf("\n");
	}
}


