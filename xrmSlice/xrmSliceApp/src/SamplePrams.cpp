/*
 * SamplePrams.cpp
 *
 *  Created on: 6 Apr 2026
 *      Author: pgm
 */


#include <stdio.h>
#include <string.h>

#include "SamplePrams.h"

// Binary pattern stored here for client apps to find

#define SAMPLE_PRAMS_FILE	"/dev/shm/SamplePrams"
#define SP_MAGIC	(('S' << 24)|('M' << 16)|('P' << 8)|'L')

SamplePrams::SamplePrams()
{
	memset(this, 0, sizeof(SamplePrams));
	MAGIC = SP_MAGIC;
}

bool SamplePrams::isValid() const
{
	return MAGIC == SP_MAGIC;
}
int SamplePrams::store(const SamplePrams& samplePrams)
{
	FILE* fp = fopen(SAMPLE_PRAMS_FILE, "w");
	if (fp == 0){
		return -1;
	}else{
		int rc = fwrite(&samplePrams, sizeof(SamplePrams), 1, fp);
		fclose(fp);
		return rc;
	}
}

int SamplePrams::load(SamplePrams& samplePrams)
{
	FILE* fp = fopen(SAMPLE_PRAMS_FILE, "r");
	if (fp == 0){
		return -1;
	}else{
		int rc = fread(&samplePrams, sizeof(SamplePrams), 1, fp);
		fclose(fp);
		if (rc == 1){
			if (samplePrams.MAGIC == SP_MAGIC){
				return 1;
			}else{
				return -2;
			}
		}else{
			return rc;
		}
	}
}

