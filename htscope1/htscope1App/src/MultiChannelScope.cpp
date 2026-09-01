/*
 * MultichannelScope.cpp
 *
 *  Created on: 6 Dec 2024
 *      Author: pgm
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <iocsh.h>
#include <epicsExport.h>

#include "MultiChannelScope.h"

#include <string>


#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include <filesystem>

long GetFileSize(const std::string& filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

#define PRDEB(n) (debug >= n) && printf

MultiChannelScope::MultiChannelScope(const char *portName, int numChannels, int maxPoints, unsigned _data_size) :
		 asynPortDriver(portName,
							 numChannels,
							 asynInt32Mask | asynInt64Mask | asynFloat64Mask | asynFloat64ArrayMask | asynDrvUserMask | asynInt32ArrayMask | asynInt64ArrayMask | asynOctetMask,
							 asynInt32Mask | asynInt64Mask | asynFloat64Mask | asynFloat64ArrayMask | asynInt32ArrayMask | asynInt64ArrayMask | asynOctetMask,
							 ASYN_CANBLOCK | ASYN_MULTIDEVICE,
							 1,
							 0,
							 0),
							 nchan(numChannels), nsam(maxPoints), data_size(_data_size),
							 ssb(numChannels*data_size),
							 RAW(0), fp(0), stride(1), startoff(0), EGU(0)
{
	createParam(PS_NCHAN,               asynParamInt32,         	&P_NCHAN);
	createParam(PS_NSAM,                asynParamInt32,         	&P_NSAM);
	createParam(PS_CHANNEL,             asynParamFloat64Array,      &P_CHANNEL);
	createParam(PS_TB,                  asynParamFloat64Array,      &P_TB);
	createParam(PS_REFRESH,				asynParamInt32,             &P_REFRESH);
	createParam(PS_REFRESHr,			asynParamInt32,             &P_REFRESHr);
	createParam(PS_MMAPUNMAP,			asynParamInt32,             &P_MMAPUNMAP);
	createParam(PS_MMAPUNMAPr,			asynParamInt32,             &P_MMAPUNMAPr);
	createParam(PS_FS,                  asynParamFloat64,           &P_FS);
	createParam(PS_STRIDE,              asynParamInt32,             &P_STRIDE);
	createParam(PS_DELAY,              	asynParamFloat64,           &P_DELAY);
	createParam(PS_ESLO,             	asynParamFloat64Array,      &P_ESLO);
	createParam(PS_EOFF,             	asynParamFloat64Array,      &P_EOFF);
	createParam(PS_EGU,  				asynParamInt32,             &P_EGU);
	createParam(PS_DEBUG,               asynParamInt32,             &P_DEBUG);
	createParam(PS_EVENTINDEX,               asynParamInt64Array,             &P_EVENTINDEX);
	createParam(PS_PRE,               asynParamInt64,             &P_PRE);
	createParam(PS_POST,               asynParamInt64,             &P_POST);
	createParam(PS_SAVE_EVENT,               asynParamInt32,             &P_SAVE_EVENT);
	createParam(PS_N_EVENTS_DETECTED,               asynParamInt32,             &P_N_EVENTS_DETECTED);
	createParam(PS_SAVE_PATH,               asynParamOctet,             &P_SAVE_PATH);

	setIntegerParam(P_NCHAN, 			nchan);
	setIntegerParam(P_NSAM, 			nsam);
	setInteger64Param(P_PRE, 			0);
	setInteger64Param(P_POST, 			0);
	setStringParam(P_SAVE_PATH, 			"/tmp");
	/*
	// Create parameters for each channel
	for (int ii = 0; ii < numChannels; ii++) {
		char paramName[20];
		sprintf(paramName, "CH_%02d", ii);
		createParam(paramName, asynParamFloat64, &channelParams[i]);
	}
	*/

	printf("P_NCHAN:%d P_NSAM:%d P_CHANNEL:%d P_REFRESH:%d\n", P_NCHAN, P_NSAM, P_CHANNEL, P_REFRESH);





    /* Create the thread that computes the waveforms in the background */
	asynStatus status = (asynStatus)(epicsThreadCreate("MultiChannelScopeTask",
                          epicsThreadPriorityMedium,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          (EPICSTHREADFUNC)::runDisplayTask,
                          this) == NULL);
	asynStatus status2 = (asynStatus)(epicsThreadCreate("MultiChannelScopeTask2",
		    epicsThreadPriorityLow,
		    epicsThreadGetStackSize(epicsThreadStackMedium),
		    (EPICSTHREADFUNC)::runSearchTask,
		    this) == NULL);
    if (status) {
        printf("%s:%s: epicsThreadCreate failure\n", "mcScope displayTask", __FUNCTION__);
        return;
    }
    if (status2) {
        printf("%s:%s: epicsThreadCreate failure\n", "mcScope searchTask", __FUNCTION__);
        return;
    }

    EVENTINDEX = new epicsInt64[MAX_NUM_EVENTS];
    ESLO = new epicsFloat64[nchan];
    EOFF = new epicsFloat64[nchan];
    for (unsigned ic = 0; ic < nchan; ++ic){
        ESLO[ic] = 10.0/0x7fff;
        EOFF[ic] = 0;

    }
    for (unsigned ic = 0; ic < MAX_NUM_EVENTS; ++ic){
	EVENTINDEX[ic] = 0;
    }
}



MultiChannelScope::~MultiChannelScope() {
	if (mmap_active){
		mmap_active = 0;
		unmap_uut_data();
	}
}

int MultiChannelScope::debug = 0;

/* Configuration routine.  Called directly, or from the iocsh function below */


void runSearchTask(void *drvPvt)
{
	MultiChannelScope *pPvt = (MultiChannelScope *)drvPvt;
	pPvt->searchTask();
}

void runDisplayTask(void *drvPvt)
{
	MultiChannelScope *pPvt = (MultiChannelScope *)drvPvt;
	pPvt->displayTask();
}

void MultiChannelScope::displayTask(void)
{
	asynStatus status = asynSuccess;

	lock();
	init_data();

	while(1){
		unlock();
		epicsThreadSleep(1);
		lock();
		if (mmap_active && refresh){
			get_tb();
			status = setIntegerParam(P_REFRESHr, refresh = 0);
			printf("%s %s rc %d\n", __FUNCTION__, "setIntegerParam", status);
#if 0
			if (status)
				epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
				              "%s:%s: status=%d, function=%d, name=%s, value=%d",
				               driverName, functionName, status, function, paramName, value);
			else
				asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
				              "%s:%s: function=%d, name=%s, value=%d\n",
				              driverName, functionName, function, paramName, value);
#endif
			status = callParamCallbacks();

			PRDEB(3)("%s %s rc %d\n", __FUNCTION__, "callParamCallbacks", status);
			PRDEB(2)("%s cleared refresh, callParamCallbacks() done\n", __FUNCTION__);
		}
	}
}

void MultiChannelScope::searchTask(void) {
	epicsThreadSleep(5.0); // wait for iocInit to finish
	lock();

	while(1) {
		unlock();
		epicsThreadSleep(0.2);
		lock();

		if (mmap_active) {
		    process_data();
		}
	}
}

void MultiChannelScope::init_data() {
	CHANNELS = new CTYPE* [nchan];
	for (unsigned ic = 0; ic < nchan; ++ic){
		CHANNELS[ic] = new CTYPE [nsam];
		for (unsigned isam = 0; isam < nsam; ++isam){
			CHANNELS[ic][isam] = (ic+1)*0.1;
		}
    	doCallbacksFloat64Array(CHANNELS[ic], nsam, P_CHANNEL, ic);
	}
	TB = new TBTYPE[nsam];
	for (unsigned isam = 0; isam < nsam; ++isam){
		TB[isam] = isam;
	}
	doCallbacksFloat64Array(TB, nsam, P_TB, 0);
}

bool MultiChannelScope::mmap_uut_data() {
	char datafile[128];
	last_scanned_offset = 0;
	current_event_count = 0;
	memset(EVENTINDEX, 0, MAX_NUM_EVENTS * sizeof(epicsInt64));

	sprintf(datafile, "%s/%s", getenv("HOME") ? getenv("HOME") : "/tmp", portName);

	fp = fopen(datafile, "r");
	if (fp == 0){
		perror(datafile);
		return false;
	}
	long raw_file_size = GetFileSize(datafile);
	if (raw_file_size < 0) {
		perror(datafile);
		return false;
	}

	size_t file_size = (size_t)raw_file_size;

	printf("datafile: %s size: %ld", datafile, file_size);


	data_len = file_size - file_size%ssb ;

	data_len_words   = data_len/data_size;
	data_len_samples = data_len/ssb;

	PRDEB(1)("data_len: %ld words: %ld samples %ld\n", data_len, data_len_words, data_len_samples);
	return true;

}

void MultiChannelScope::unmap_uut_data() {
	if (RAW){
		RAW = 0;
	}
	if (fp){
		fclose(fp);
		fp = 0;
	}
}

#include <unistd.h>
#include <vector>

void MultiChannelScope::get_data() {
    double delay = 0;
    double fs = 2e6;

    // Safety check: ensure file is actually open
    if (!fp) {
        printf("ERROR: %s file pointer is null. Is the DAQ file open?\n", __FUNCTION__);
        return;
    }

    asynStatus status = getDoubleParam(P_DELAY, &delay);
    if (status != asynSuccess){
        delay = 0;
        printf("ERROR: %s failed to retrieve P_DELAY, setting default %.3g\n", __FUNCTION__, delay);
    }
    status = getDoubleParam(P_FS, &fs);
    if (status != asynSuccess){
        fs = 2e6;
        printf("ERROR: %s failed to retrieve P_FS, setting default %.3g\n", __FUNCTION__, fs);
    }
    
    startoff = delay * fs;
    printf("%s startoff:%ld (samples) stride:%u\n", __FUNCTION__, startoff, stride);

    unsigned long start_cursor = startoff * nchan;
    int overlimit = 0;

    // =========================================================================
    // OPTIMIZATION: If stride is 1, read the entire chunk in one fast disk hit
    // =========================================================================
    if (stride == 1) {
        unsigned long total_words = nsam * nchan;
        size_t bytes_to_read = total_words * sizeof(epicsInt16);
        unsigned long byte_offset = start_cursor * sizeof(epicsInt16);

        // Allocate a temporary local buffer for this waveform grab
        std::vector<epicsInt16> bulk_buffer(total_words, 0); 
        
        ssize_t bytes_read = pread(fileno(fp), bulk_buffer.data(), bytes_to_read, byte_offset);
        if (bytes_read < 0) bytes_read = 0;
        
        size_t words_read = bytes_read / sizeof(epicsInt16);

        for (unsigned isam = 0; isam < nsam; ++isam) {
            for (unsigned ic = 0; ic < nchan; ++ic) {
                unsigned long idx = isam * nchan + ic;
                
                if (idx < words_read) {
                    if (EGU) {
                        CHANNELS[ic][isam] = bulk_buffer[idx]; // * ESLO[ic] + EOFF[ic];
                    } else {
                        CHANNELS[ic][isam] = bulk_buffer[idx];
                    }
                } else {
                    if (!overlimit) {
                        PRDEB(1)("Reached limit of data, fill with zeros\n");
                        overlimit = 1;
                    }
                    CHANNELS[ic][isam] = 0;
                }
            }
        }
    } 
    // =========================================================================
    // STRIDED VIEW: Read frame-by-frame to skip unneeded disk data
    // =========================================================================
    else {
        // Buffer to hold one frame (all channels for a single sample time)
        std::vector<epicsInt16> frame_buffer(nchan, 0);
        size_t frame_bytes = nchan * sizeof(epicsInt16);

        for (unsigned isam = 0; isam < nsam; ++isam) {
            unsigned long cursor = start_cursor + isam * nchan * stride;
            
            if (cursor <= data_len_words + nchan * data_size) {
                unsigned long byte_offset = cursor * sizeof(epicsInt16);
                
                ssize_t bytes_read = pread(fileno(fp), frame_buffer.data(), frame_bytes, byte_offset);
                
                for (unsigned ic = 0; ic < nchan; ++ic) {
                    if (bytes_read > (ssize_t)(ic * sizeof(epicsInt16))) {
                        if (EGU) {
                            CHANNELS[ic][isam] = frame_buffer[ic]; // * ESLO[ic] + EOFF[ic];
                        } else {
                            CHANNELS[ic][isam] = frame_buffer[ic];
                        }
                    } else {
                        CHANNELS[ic][isam] = 0;
                    }
                }
            } else {
                if (!overlimit) {
                    PRDEB(1)("cursor %lu reached limit of data at %d/%d fill with zeros\n", cursor, isam, nsam);
                    overlimit = 1;
                }
                for (unsigned ic = 0; ic < nchan; ++ic) {
                    CHANNELS[ic][isam] = 0;
                }
            }
        }
    }

    // Callbacks to push arrays to EPICS records
    for (unsigned ic = 0; ic < nchan; ic++){
        PRDEB(4)("%s ic:%d nsam:%d P_CHANNEL:%d\n", __FUNCTION__, ic, nsam, P_CHANNEL);
        asynStatus stat = doCallbacksFloat64Array(CHANNELS[ic], nsam, P_CHANNEL, ic);
        if (ic == 0) {
            printf("%s: CH:01 callback attempted nsam=%d stat=%d\n", __FUNCTION__, nsam, stat);
        }
    }
    printf("%s 99 nchan:%d nsam:%d\n", __FUNCTION__, nchan, nsam);
}


// Helper to print memory usage to the console
void print_memory_usage(const char* prefix) {
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.compare(0, 6, "VmRSS:") == 0) { // VmRSS is the actual Physical RAM used
            printf("DEBUG MEM [%s]: %s\n", prefix, line.c_str());
            break;
        }
    }
}

void MultiChannelScope::process_data() {
    if (!fp || current_event_count >= MAX_NUM_EVENTS) return;

    struct stat stat_buf;
    if (fstat(fileno(fp), &stat_buf) != 0) return;
    
    unsigned long current_file_size = (unsigned long)stat_buf.st_size;

    if (current_file_size > data_len) current_file_size = data_len;
    if (current_file_size < sizeof(uint32_t)) return;

    size_t limit = current_file_size - sizeof(uint32_t);
    size_t aligned_start = (last_scanned_offset + 3) & ~3;

    if (aligned_start > limit) return;

    printf("DEBUG [%s]: Starting scan at offset %zu, file limit %zu\n", portName, aligned_start, limit);
    print_memory_usage(portName);

    const uint32_t MAGIC_NUM = 0xAA55F151;
    bool found_new_events = false;

    size_t chunk_size = 10 * 1024 * 1024;
    std::vector<uint8_t> buffer(chunk_size);
    
    int loop_counter = 0;

    // SCANNING LOOP
    while (aligned_start <= limit && current_event_count < MAX_NUM_EVENTS) {
        
        size_t bytes_to_read = chunk_size;
        if (aligned_start + bytes_to_read > current_file_size) {
            bytes_to_read = current_file_size - aligned_start;
        }

        // Print progress every 500 MB (50 loops of 10MB)
        if (loop_counter % 50 == 0) {
            printf("DEBUG [%s]: Scanning offset %zu (%.1f%% complete)\n", portName, aligned_start, (double)aligned_start / limit * 100.0);
            print_memory_usage(portName);
        }
        loop_counter++;

        ssize_t bytes_read = pread(fileno(fp), buffer.data(), bytes_to_read, aligned_start);
        if (bytes_read <= 0) {
            printf("DEBUG [%s]: pread failed or EOF. bytes_read=%zd\n", portName, bytes_read);
            break;
        }

        size_t valid_limit = (size_t)bytes_read;
        if (valid_limit >= sizeof(uint32_t)) {
            valid_limit -= sizeof(uint32_t);
        } else {
            break;
        }

        size_t buffer_i = 0;
        while (buffer_i <= valid_limit) {
            const uint32_t* current_val = reinterpret_cast<const uint32_t*>(buffer.data() + buffer_i);
            
            if (*current_val == MAGIC_NUM) {
                EVENTINDEX[current_event_count] = (epicsInt64)(aligned_start + buffer_i);
                current_event_count++;
                found_new_events = true;
                n_events_detected += 1;
                
                printf("DEBUG [%s]: Found event %d at absolute byte %zu\n", portName, current_event_count, aligned_start + buffer_i);

                if (ssb > 4) buffer_i += (ssb - 4);

                if (current_event_count >= MAX_NUM_EVENTS) {
                    aligned_start += buffer_i + 4;
                    break;
                }
            }
            buffer_i += 4;
        }

        // --- CRITICAL KERNEL CACHE FLUSH ---
        // Force the OS to instantly delete this 10MB chunk from physical RAM (Page Cache)
        posix_fadvise(fileno(fp), aligned_start, bytes_read, POSIX_FADV_DONTNEED);

        if (current_event_count >= MAX_NUM_EVENTS) {
            last_scanned_offset = aligned_start;
            break;
        }

        aligned_start += bytes_read;
        last_scanned_offset = aligned_start;
    }

    printf("DEBUG [%s]: Finished scan round. New offset: %zu\n", portName, last_scanned_offset);
    print_memory_usage(portName);

    if (found_new_events) {
        setIntegerParam(P_N_EVENTS_DETECTED, current_event_count);
        callParamCallbacks();
        doCallbacksInt64Array(EVENTINDEX, MAX_NUM_EVENTS, P_EVENTINDEX, 0);
    }
}

void MultiChannelScope::get_tb() {
	double fs;                                   // pick a reasonable default
	int stride;
	double isi;
	double delay = 0;
	asynStatus status = (asynStatus) getDoubleParam(P_FS, &fs);
	if (status != asynSuccess){
		fs = 1e6;
		printf("ERROR: %s failed to retrieve fs, setting default %.3e\n", __FUNCTION__, fs);
	}
	status = (asynStatus) getIntegerParam(P_STRIDE, &stride);
	if (status != asynSuccess){
		stride = 1;
		printf("ERROR: %s failed to retrieve stride, setting default %d\n", __FUNCTION__, stride);
	}
	isi = (double)stride/fs;

	status = (asynStatus) getDoubleParam(P_DELAY, &delay);
	if (status != asynSuccess){
		delay = 0;
		printf("ERROR: %s failed to retrieve delay, setting default %.3e\n", __FUNCTION__, delay);
	}
	PRDEB(1)("%s create TB delay:%f isi:%.4g\n", __FUNCTION__, delay, isi);

	for (unsigned isam = 0; isam < nsam; ++isam, delay += isi){
		TB[isam] = delay;
		/*
		if (isam == 0 || (isam+1)%10000 == 0)
			printf("%s [%d] create TB delay:%f isi:%.4g\n", __FUNCTION__, isam, delay, isi);
		 */
	}
	PRDEB(1)("%s doCallbacksFloat64Array(%p, %d, %d, %d)\n", __FUNCTION__, TB, nsam, P_TB, 0);
	doCallbacksFloat64Array(TB, nsam, P_TB, 0);
}

bool MultiChannelScope::save_clipped_event(int event_array_index) {
        // Validate requested event exists
	if (event_array_index < 0 || event_array_index >= current_event_count) {
	        printf("%s: event %d does not exist. Max event is %d", __FUNCTION__, event_array_index, current_event_count);
	        return false;
	}

	// Ensure the main data file actually open
	if (!fp) {
		printf("ERROR: %s main file pointer is null.\n", __FUNCTION__);
		return false;
	}

	// Fetch PRE and POST from EPICS parameters
	epicsInt64 pre_samples = 0;
	epicsInt64 post_samples = 0;
	getInteger64Param(P_PRE, &pre_samples);
	getInteger64Param(P_POST, &post_samples);
        
	char datafile[256];
	snprintf(datafile, sizeof(datafile), "%s/%s", getenv("HOME") ? getenv("HOME") : "/tmp", portName);
	long raw_file_size = GetFileSize(datafile);
	if (raw_file_size < 0) {
	    perror(datafile);
	    return false;
	}
	epicsInt64 current_file_size = (epicsInt64)raw_file_size;
	
	// Get byte offset of specific event from event index array
	epicsInt64 byte_offset = EVENTINDEX[event_array_index];

	// calculate the window to clip in bytes
	epicsInt64 total_samples = pre_samples + 1 + post_samples;
	epicsInt64 total_bytes = total_samples * ssb;

	// Calculate where to start copying
	epicsInt64 start_byte = byte_offset - (pre_samples * ssb);

	printf("ssb is: %d total_samples: %lld\n", ssb, (long long)total_samples); 
	// Do a boundary check
	// Did PRE push us before the beginning or POST after the end?
	if (start_byte < 0) {
		epicsInt64 bytes_lost = 0 - start_byte;
		start_byte = 0;
		total_bytes -= bytes_lost;
		printf("%s: Warning - PRE window truncated to start of file. start_byte idx: %lld PRE: %lld\n", 
				__FUNCTION__, (long long)start_byte, (long long)pre_samples);
	}

	epicsInt64 end_byte = current_file_size;
	if (start_byte + total_bytes > current_file_size) {
		total_bytes = end_byte - start_byte;
		printf("%s: Warning - POST window truncated to end of file. start_byte idx: %lld POST: %lld file_size: %lld\n",
				__FUNCTION__, (long long)start_byte, (long long)post_samples, (long long)current_file_size);
	}

	if (total_bytes <= 0) {
		printf("%s: Error- calculated clip size is 0 bytes.\n", __FUNCTION__);
		return false;
	}

	// Write to disk
	// Fetch user defined path
	char base_path[256];
	getStringParam(P_SAVE_PATH, sizeof(base_path), base_path);
	// Ensure there is a trailing slash
	std::string path_str(base_path);
	if (!path_str.empty() && path_str.back() != '/') {
		path_str += "/";
	}
        // make sure dir exists
	std::string full_dir_path = "/tmp/" + path_str;
	std::error_code ec;
        std::filesystem::create_directories(full_dir_path, ec);
	if (ec) {
		printf("Failed to create directory structure %s: %s\n", full_dir_path, ec);
		return false;
	}
	else {
		printf("Created directory structure %s: %d\n", full_dir_path.c_str(), ec);
	}


	// Create unique filename
	char filename[512];
	snprintf(filename, sizeof(filename), "%sevent-%d-%lld-%lld-%lld.dat", full_dir_path.c_str(),  event_array_index, (long long)start_byte, (long long)pre_samples, (long long)post_samples);
	printf("Saving to: %s\n", filename);

	FILE *out_fp = fopen(filename, "wb");
	if (!out_fp) {
		printf("Failed to open file for writing %s. Does the directory exist?\n", filename);
		return false;
	}

	// Pull exact bytes straight from disk
	std::vector<uint8_t> clip_buffer(total_bytes);
	ssize_t bytes_read = pread(fileno(fp), clip_buffer.data(), total_bytes, start_byte);

	if (bytes_read <= 0) {
		printf("ERROR: %s failed to read data from source file (pread returned %zd)\n", __FUNCTION__, bytes_read);
		fclose(out_fp);
		return false;
	}

	// dump buffer to new file
	size_t written = fwrite(clip_buffer.data(), 1, bytes_read, out_fp);
	fclose(out_fp);

	printf("%s: SUCCESS. Saved event %d to %s. (requested %lld bytes, saved %zu bytes)\n",
			__FUNCTION__, event_array_index, filename, (long long)(total_samples * ssb), written);
	
	// send file over FTP to FTP server
	const char* ip = getenv("FTP_IP");
	const char* user = getenv("FTP_USER");
	const char* pass = getenv("FTP_PASS");

	if (ip && user && pass) {
	    char cmd[1024];
	    // -s silent and -T to upload file
	    snprintf(cmd, sizeof(cmd), "curl -s -T %s ftp://%s/%s --user %s:%s &",
		    filename, ip, path_str.c_str(), user, pass);

	    int rc = std::system(cmd);
	    if (rc != 0) {
		printf("Failed to open file for writing via FTP. path:%s file:%s ip:%s user:%s pass:%s", path_str.c_str(), filename, ip, user, pass);
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
			"failed to spawn background FTP transfer\n.");
	    }
	} else {
	    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
		    "FTP environment variables not set in st.cmd FTP_USER FTP_PASS FTP_IP\n");
	}

	return true;
}

asynStatus MultiChannelScope::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    asynStatus status = asynSuccess;
    int function = pasynUser->reason;
    const char *paramName;
    int addr;
    const char* functionName = "writeInt32";

    status = parseAsynUser(pasynUser, &function, &addr, &paramName);
    if (status != asynSuccess) return status;

    PRDEB(1)("%s addr:%d f:%d %s v:%d\n", __FUNCTION__, function, addr, paramName, value);
    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(addr, function, value);


    if (function == P_REFRESH){
    	refresh = 1;

    	FILE* fp = fopen("params.txt", "w");
    	reportParams(fp, 3);
    	fclose(fp);
    	status = (asynStatus) setIntegerParam(addr, P_REFRESHr, 0);
    	printf("%s %s rc %d\n", __FUNCTION__, "setIntegerParam", status);
    	status = callParamCallbacks();
    	printf("%s %s rc %d\n", __FUNCTION__, "callParamCallbacks", status);
    }else if (function == P_STRIDE){
    	stride = value;
    }else if (function == P_DEBUG){
    	debug = value;
    }else if (function == P_EGU){
    	EGU = value;
    }else if (function == P_MMAPUNMAP){
    	if (value == 1){
    		mmap_active = mmap_uut_data();
    	}else{
    		if (mmap_active){
    			mmap_active = 0;
    			unmap_uut_data();
    		}
    	}
    	status = (asynStatus) setIntegerParam(addr, P_MMAPUNMAPr, value);
    	if (status != 0) printf("ERROR %s setIntegerParam %d, %d, %d fail\n", __FUNCTION__,  addr, P_MMAPUNMAPr, value);
    }
    else if (function == P_SAVE_EVENT) {
	    save_clipped_event(value);
    }
    //else if (function == P_N_EVENTS_DETECTED) {
    //	    n_events_detected = value;
    //}

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks(addr, addr);

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, name=%s, value=%d",
				  __FUNCTION__, functionName, status, function, paramName, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, name=%s, value=%d\n",
			  __FUNCTION__, functionName, function, paramName, value);
    return status;
}

asynStatus MultiChannelScope::writeInt32Array(asynUser *pasynUser, epicsInt32 *value,
					size_t nElements)
{
	return asynPortDriver::writeInt32Array(pasynUser, value, nElements);
}

asynStatus MultiChannelScope::writeInt64Array(asynUser *pasynUser, epicsInt64 *value,
					size_t nElements)
{
	return asynPortDriver::writeInt64Array(pasynUser, value, nElements);
}

asynStatus MultiChannelScope::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
	return asynPortDriver::writeFloat64(pasynUser, value);
}

asynStatus MultiChannelScope::writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value,
                                        size_t nElements)
{
	asynStatus status = asynSuccess;
    const char *paramName;
    int addr;
    int function;
    epicsFloat64* pcal;
    status = parseAsynUser(pasynUser, &function, &addr, &paramName);
    if (status != asynSuccess) return status;

    PRDEB(1)("%s addr:%d f:%d %s\n", __FUNCTION__, addr, function, paramName);
    if (function == P_ESLO){
        pcal = ESLO;
    }else if (function == P_EOFF){
        pcal = EOFF;
    }else{
        return asynPortDriver::writeFloat64Array(pasynUser, value, nElements);
    }

    assert(nElements <= nchan);
    for (unsigned ic = 0; ic < nchan; ++ic){
    	PRDEB(2)("%s addr:%d %.4f\n", __FUNCTION__, function, value[ic]);
        pcal[ic] = value[ic];
    }
    return status;
}

extern "C" {

	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  * \param[in] maxPoints The maximum  number of points in the volt and time arrays */
	int multiChannelScopeConfigure(const char *portName, int nchan, int maxPoints, unsigned data_size)
	{
		//return MultiChannelScope::factory(portName, nchan, maxPoints, data_size);
		printf("%s, %s, %d, %d %d\n", __FUNCTION__, portName, nchan, maxPoints, data_size);
		new MultiChannelScope(portName, nchan, maxPoints, data_size);

		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "uut", iocshArgString };
	static const iocshArg initArg1 = { "max chan", iocshArgInt };
	static const iocshArg initArg2 = { "max points", iocshArgInt };
	static const iocshArg initArg3 = { "data size", iocshArgInt };
	static const iocshArg * const initArgs[] = { &initArg0, &initArg1, &initArg2, &initArg3 };
	static const iocshFuncDef initFuncDef = { "multiChannelScopeConfigure", 4, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		multiChannelScopeConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].ival);
	}

	void multiChannelScopeRegister(void)
	{
	    iocshRegister(&initFuncDef,initCallFunc);
	}

	epicsExportRegistrar(multiChannelScopeRegister);
}
