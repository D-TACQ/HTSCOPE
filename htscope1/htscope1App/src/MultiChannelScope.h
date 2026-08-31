/*
 * MultiChannelScope.h
 *
 *  Created on: 7 Dec 2024
 *      Author: pgm
 */

#ifndef HTSCOPE1_HTSCOPE1APP_SRC_MULTICHANNELSCOPE_H_
#define HTSCOPE1_HTSCOPE1APP_SRC_MULTICHANNELSCOPE_H_

#include "asynPortDriver.h"

#define PS_NCHAN 		"NCHAN"				/* asynInt32, 		r/o */
#define PS_NSAM			"NSAM"				/* asynInt32,       r/o */
#define PS_CHANNEL		"CHANNEL"			/* ,       r/o */
#define PS_TB			"TIMEBASE"			/*,        r/o */
#define PS_REFRESH		"REFRESH"			/* asynInt32, 		r/w */
#define PS_REFRESHr		"REFRESHr"			/* asynInt32,       r   */
#define PS_MMAPUNMAP	"MMAPUNMAP"			/* asynInt32,       r/w */
#define PS_MMAPUNMAPr	"MMAPUNMAPr"		/* asynInt32,       r */
#define PS_FS           "FS"                /* asynFloat64,     r/w */
#define PS_STRIDE		"STRIDE"			/* asynInt32,       r/w */
#define PS_DELAY		"DELAY"				/* asynFloat64,     r/w */
#define PS_ESLO			"ESLO"				/* asynFloat64,     r/w */
#define PS_EOFF			"EOFF"				/* asynFloat64,     r/w */
#define PS_EGU			"EGU"				/* asynInt32,       r/w */
#define PS_DEBUG		"DEBUG"				/* asynInt32,       r/w */
#define PS_EVENTINDEX		"EVENTINDEX"			/* asynInt64,       r/w */
#define PS_PRE		"PRE"			/* asynInt64,       r/w */
#define PS_POST		"POST"			/* asynInt64,       r/w */
#define PS_SAVE_EVENT "SAVE_EVENT"
#define PS_N_EVENTS_DETECTED "N_EVENTS_DETECTED"
#define PS_SAVE_PATH "SAVE_PATH"                /* asynOctet */

#define MAX_NUM_EVENTS 256

typedef epicsFloat64 CTYPE;
typedef epicsFloat64 TBTYPE;
typedef epicsInt64   EVTYPE;

void runDisplayTask(void *drvPvt);
void runSearchTask(void *drvPvt);

class MultiChannelScope : public asynPortDriver {
public:
    MultiChannelScope(const char *portName, int numChannels, int maxPoints, unsigned data_size);

    // Add methods for scope functionality
    ~MultiChannelScope();
    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                        size_t nElements);
    virtual asynStatus writeInt64Array(asynUser *pasynUser, epicsInt64 *value,
                                        size_t nElements);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value,
                                        size_t nElements);
    void displayTask(void);
    void searchTask(void);

protected:
    size_t last_scanned_offset;
    int current_event_count;

private:
    static int debug;
    const unsigned nchan;
    const unsigned nsam;
    const unsigned data_size;
    const int ssb;

	int P_NCHAN;
	int P_NSAM;
	int P_CHANNEL;
	int P_TB;
	int P_REFRESH;
	int P_REFRESHr;
	int P_MMAPUNMAP;
	int P_MMAPUNMAPr;
	int P_FS;
	int P_STRIDE;
	int P_DELAY;
	int P_ESLO;
	int P_EOFF;
	int P_EGU;
	int P_DEBUG;
	int P_EVENTINDEX;
	int P_PRE;
	int P_POST;
	int P_SAVE_EVENT;
	int P_N_EVENTS_DETECTED;
	int P_SAVE_PATH;

    // Add private members for scope data
    unsigned long data_len;
    unsigned long data_len_words;
    unsigned long data_len_samples;
    epicsInt16* RAW;		    // array [SAMPLE][CH]

    CTYPE** CHANNELS;			// array [CH][SAMPLE]
    TBTYPE* TB;                 // array [SAMPLE]
    FILE* fp;
    unsigned stride;
    unsigned long startoff;
    bool refresh;
    bool mmap_active;

    epicsFloat64* ESLO;
    epicsFloat64* EOFF;
    epicsInt64* EVENTINDEX;
    epicsInt32 n_events_detected;
    int EGU;                    /* EGU if set, RAW if not set */

    void get_tb();
    void get_data();
    void process_data();
    void init_data();
    bool mmap_uut_data();   // return True on success
    void unmap_uut_data();
    bool save_clipped_event(int);
};

#endif /* HTSCOPE1_HTSCOPE1APP_SRC_MULTICHANNELSCOPE_H_ */
