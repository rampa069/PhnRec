/**************************************************************************
 *  copyright            : (C) 2004-2006 by Petr Schwarz & Pavel Matejka  *
 *                                        UPGM,FIT,VUT,Brno               *
 *  email                : {schwarzp,matejkap}@fit.vutbr.cz               *
 **************************************************************************
 *                                                                        *
 *  This program is free software; you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation; either version 2 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 **************************************************************************/

#ifndef _LWFSOURCE_H
#define _LWFSOURCE_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <exception>
#include <new>

#define WFS_BUFFERLENGTH        2000      // 2 s
// JC zkousim 10 mms namisto 100  - pro labs ne. 
#define WFS_FRAMELENGTH          100      // 100 ms
#define WFS_DEFAULTDEVICE    "/dev/dsp"

// exceptions
class wf_error : public std::exception
{
	protected:
		char str[256];
	public:
		wf_error(const char *what_arg) throw() {strncpy(str, what_arg, 256);};
		virtual const char *what () const throw () {return str;}; 
		virtual ~wf_error() throw() {};
};


class LWFSource
{
	protected:
		char *device;
		FILE *dp;
    
		int sampleFreq;
		int channels;
		int bitsPerSample;
		int blockAlign;

		char *buffer;
		char *bufferLastByte;
		volatile int bufferLen;
		volatile int frameLen;
		char *reading;
		char *storing;
		volatile int bytesRecorded;
		volatile bool isRecording;
		
		pthread_t thread; 
		pthread_mutex_t mutex;
		pthread_cond_t cond;

		void runRecording();
		static void *recordingBypass(void *arg);
		void recording();
		
		void onData();
		void recordFrame();
	public:
		LWFSource();
		~LWFSource();
		bool isAvailable(char* ret_whay_not = 0);
		bool isOpen() {return dp != 0;};
		void open(); 
		void close();
		void read(char *buff, int n); 
		void setFormat(int sf, int ch, int bps) {sampleFreq = sf; channels = ch; bitsPerSample = bps;};
		void setDevice(char *d) {device = d;};
};

#endif

