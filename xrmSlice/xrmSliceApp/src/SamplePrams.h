/*
 * SamplePrams.h
 *
 *  Created on: 9 Apr 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_SAMPLEPRAMS_H_
#define XRMIOCAPP_SRC_SAMPLEPRAMS_H_

struct SamplePrams {
	int MAGIC;
	int SSB;
	int NSAM;
	int AI_COUNT;
	int AI_INDEX;
	int DI_COUNT;
	int DI_INDEX;
	int SP_COUNT;
	int SP_INDEX;
	SamplePrams();
	bool isValid() const;
	static int store(const SamplePrams& samplePrams);
	static int load(SamplePrams& samplePrams);
};

const SamplePrams* get_acq400_SamplePrams();



#endif /* XRMIOCAPP_SRC_SAMPLEPRAMS_H_ */
