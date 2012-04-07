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

#include "lwfsource.h"
#include "soundcard.h"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

LWFSource::LWFSource()
{
	device = WFS_DEFAULTDEVICE;
	dp = 0;
	sampleFreq = 16000;
	channels = 1;
	bitsPerSample = 16;
	buffer = 0;
	isRecording = false;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
}
           
LWFSource::~LWFSource()
{
	close();
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}

void LWFSource::open()
{
	if(dp)
		return;

	// check setting
	assert(channels == 1 || channels == 2);
	assert(bitsPerSample == 8 || bitsPerSample == 16);
	
	// open device
	dp = fopen(device, "rb");
	if(!dp)
	{
		char msg[1024];
		sprintf(msg, "Can not open the sound device: %900s.", device);
		throw wf_error(msg);
	}
    
	// An input can be either device or file. If it is a device, configure it.
	struct stat st;
	fstat(fileno(dp), &st);
	if(S_ISCHR(st.st_mode))
	{
		ioctl(fileno(dp), SNDCTL_DSP_RESET, 0);
	
		int stereo_req = (channels == 1 ? 0 : 1);   
		int stereo = stereo_req;
		if(ioctl(fileno(dp), SNDCTL_DSP_STEREO, &stereo) == -1 || stereo != stereo_req)
		{
			fclose(dp);
			dp = 0;
			throw wf_error("Can not set mono mode for the sound card.");
		}
			
		int audio_format_req = (bitsPerSample == 8 ? AFMT_S8 : AFMT_S16_LE);
		int audio_format = audio_format_req;
		if(ioctl(fileno(dp), SNDCTL_DSP_SETFMT, &audio_format) == -1 || audio_format != audio_format_req)
		{
			fclose(dp);
			dp = 0;
			throw wf_error("The sound card does not support little-endien shorts.");
		}
		
		int sample_freq = sampleFreq;
		if(ioctl(fileno(dp), SNDCTL_DSP_SPEED, &sample_freq) == -1 || sample_freq != sampleFreq)
		{
			fclose(dp);
			dp = 0;
			char msg[1024];
			sprintf(msg, "The sound card does not support desired sample frequncy: %d.", sampleFreq);
			throw wf_error(msg);
		}
	}   
    
	// block align
	blockAlign = (bitsPerSample == 8 ? 1 : 2);
	if(channels == 2)
		blockAlign *= 2;	

	// allocate buffer
	bufferLen = (int)((float)(blockAlign * sampleFreq * WFS_BUFFERLENGTH) / 1000.0f + 0.5f);
	frameLen = bufferLen * WFS_FRAMELENGTH / WFS_BUFFERLENGTH;
	bufferLen -= (bufferLen % frameLen);

	if(bufferLen == 0 || frameLen == 0)
	{
		fclose(dp);
		dp = 0;
		throw wf_error("Bad seting of the WFS_BUFFERLENGTH or the the WFS_FRAMELENGTH constant.");
	}

	try
	{
		buffer = new char[bufferLen];
		bufferLastByte = buffer + bufferLen - 1;
	}
	catch(std::bad_alloc &)        // Handler acording ANSI C++
	{
		fclose(dp);
        	dp = 0;
        	throw std::bad_alloc();
	}
	if(!buffer)                    // VC
	{
		fclose(dp);
		dp = 0;
		throw std::bad_alloc();
	}
}

void LWFSource::runRecording()
{
	isRecording = true;
	reading = buffer;
	storing = buffer;
	bytesRecorded = 0;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if(pthread_create(&thread, &attr, recordingBypass, this) != 0) 
		throw wf_error("Not enough system resources for creating thread.");
	
	pthread_attr_destroy(&attr);
}

void *LWFSource::recordingBypass(void *arg) 
{
	((LWFSource *)arg)->recording(); 
	pthread_exit(0);
};

void LWFSource::recording()
{
	pthread_mutex_lock(&mutex);

	while(bytesRecorded + frameLen <= bufferLen && isRecording)
	{ 
 		pthread_cond_signal(&cond);
		pthread_mutex_unlock (&mutex);
		
		::read(fileno(dp), storing, frameLen);
	
		pthread_mutex_lock(&mutex);
	
		storing += frameLen;
		if(storing > bufferLastByte)
			storing = buffer;
		bytesRecorded += frameLen;
	}
	
	isRecording = false;	
	
	pthread_mutex_unlock (&mutex);
	
}


void LWFSource::close()
{
	if(!dp)
		return;
        
	if(isRecording)
	{
		isRecording = false;

		int status;
		pthread_join(thread, (void **)&status);
	}
	
	fclose(dp);
	dp = 0;
}

void LWFSource::read(char *buff, int len)
{
	if(!dp)
		throw wf_error("The device is not open.");
		
	if(!isRecording)
		runRecording();
	
		
	pthread_mutex_lock(&mutex);
	
	int i;
	for(i = 0; isRecording && i < len; ++i)
	{    
		while(isRecording && bytesRecorded == 0)
			pthread_cond_wait(&cond, &mutex);
			
		if(isRecording)
		{
			buff[i] = *reading;
			reading++;
			if(reading > bufferLastByte)
				reading = buffer;
			bytesRecorded--;
		}
	}
	
	pthread_mutex_unlock (&mutex);
}


bool LWFSource::isAvailable(char *ret_whay_not)
{
	if(dp)
		return true;
    
	FILE *dp1;
	dp1 = fopen(device, "rb");
	if(dp1)
	{
		fclose(dp1);
		return true;
	}
	
	if(ret_whay_not)
		strcpy(ret_whay_not, "can not open device");	       

	return false;
}

