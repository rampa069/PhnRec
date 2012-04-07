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

#include "wfsource.h"
#include <stdio.h>

CWFSource::CWFSource()
{
    device = 0;
    sampleFreq = 16000;
    channels = 1;
    bitsPerSample = 16;
    buffer = 0;
    isRecording = false;

    event = CreateEvent(0, true, true, 0);
    ResetEvent(event);
}
           
CWFSource::~CWFSource()
{
    close();
    CloseHandle(event);
}

void CWFSource::open()
{
    if(device)
        return;

    // open device
    memset((char *)&wf, 0, sizeof(wf));	        
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = (WORD)channels;
    wf.nSamplesPerSec = (DWORD)sampleFreq;
    wf.wBitsPerSample = (WORD)bitsPerSample;
    wf.nAvgBytesPerSec = (DWORD)(channels * sampleFreq * (bitsPerSample / 8));
    wf.nBlockAlign = (WORD)(channels*(bitsPerSample / 8));    

    if(waveInOpen(&device, WAVE_MAPPER, &wf, (DWORD)MMSCallback, (DWORD)this, CALLBACK_FUNCTION))
    {
         device = 0;
         throw wf_error("Can not open sound device");
    }

    // allocate buffer
    bufferLen = (int)((float)(wf.nBlockAlign * sampleFreq * CWFS_BUFFERLENGTH) / 1000.0f + 0.5f);
    frameLen = bufferLen * CWFS_FRAMELENGTH / CWFS_BUFFERLENGTH;
    bufferLen -= (bufferLen % frameLen);

    if(bufferLen == 0 || frameLen == 0)
    {
         waveInClose(device);
         device = 0;
         throw wf_error("Bad seting of the CWFS_BUFFERLENGTH or the CWFS_FRAMELENGTH constant");
    }

    try
    {
        buffer = new char[bufferLen];
        bufferLastByte = buffer + bufferLen - 1;
    }
    catch(std::bad_alloc &)        // Handler acording ANSI C++
    {
	waveInClose(device);
        device = 0;
        throw std::bad_alloc();
    }
    if(!buffer)               // VC
    {
         waveInClose(device);
         device = 0;
         throw std::bad_alloc();
    }
}

void CWFSource::close()
{
    if(!device)
        return;
        
    isRecording = false;	
    waveInReset(device);

    ResetEvent(event);
    while(recFrames != 0)
    {
        WaitForSingleObject(event, INFINITE);
        ResetEvent(event);
    }

    waveInClose(device);
    device = 0;
}

void CWFSource::recordFrame()
{	

    // Prepare buffers. This process locks memory blocks and allows DMA transfer.
    memset((char *)header, 0, sizeof(*header));
    header->lpData = storing;
    header->dwBufferLength = frameLen;
    header->dwFlags = 0;
    header->dwLoops = 0;

    storing += frameLen;
    if(storing > bufferLastByte)
    storing = buffer;
	
    // Prepare the buffer, DMA channel etc.
    if(waveInPrepareHeader(device, header, sizeof(*header)) != 0)
    {
        throw wf_error("Can not prepare a sound buffer");
    }

    // Start recording the buffer
    if(waveInAddBuffer(device, header, sizeof(*header)) != 0)
    {
        waveInUnprepareHeader(device, header, sizeof(*header));
        throw wf_error("Can not add a buffer");
    }

    // Number of actualy being recorded frames
    header = (header == &header1 ? &header2 : &header1);
    recFrames++;   
}

void CWFSource::read(char *buff, int len)
{
    if(!isRecording)
    {
        if(!device)
            throw wf_error("A sound device is not open");

        isRecording = true;
        header = &header1;
        reading = buffer;
        storing = buffer;
        actFrame = 0;
        bytesRecorded = 0;
        recFrames = 0;
        actFrame = 0;

        recordFrame();
        recordFrame();
        if(waveInStart(device) != 0)
        {
            isRecording = false;
            waveInReset(device);             
            throw wf_error("Can not start recording");
        }
    }

    int i;
    for(i = 0; isRecording && i < len; ++i)
    {    
        while(isRecording && bytesRecorded == 0)
        {
            ResetEvent(event);
            if(bytesRecorded == 0)
            WaitForSingleObject(event, INFINITE);
        }

        if(isRecording)
        {
            buff[i] = *reading;
            reading++;
            if(reading > bufferLastByte)
            reading = buffer;
	    bytesRecorded--;
        }
    }
}


#ifdef  _MSC_VER
#pragma warning(disable:4100)
#endif

void CALLBACK CWFSource::MMSCallback(HWAVEIN hwi, UINT uMSG, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    CWFSource *S;
    
    S = (CWFSource *)dwInstance;

    switch(uMSG)
    {
        case WIM_CLOSE:
            break;
        case WIM_DATA:
            S->onData();    
            break;
        case WIM_OPEN:
            break;                
    }
}

#ifdef  _MSC_VER
#pragma warning(default:4100)
#endif


void CWFSource::onData()
{
    WAVEHDR *recHeader = (actFrame == 0 ? &header1 : &header2);

    // Unprepare the frame
    if(isRecording && recHeader->dwFlags & WHDR_PREPARED)
        waveInUnprepareHeader(device, recHeader, sizeof(*recHeader));
	
    actFrame = (actFrame == 1 ? 0 : 1);

    bytesRecorded += frameLen;
	recFrames--;
    
    if(bytesRecorded + frameLen <= bufferLen)
    {
        if(isRecording)
            recordFrame();
    }
    else
    {
        if(recFrames == 0)
            isRecording = false;   // Stop recording, nobody reads data
    }
    SetEvent(event);		
}


bool CWFSource::isAvailable(char *ret_whay_not)
{
    if(device)
        return true;
    
    // test if a device exists
    if(waveInGetNumDevs() == 0)

    {
        if(ret_whay_not)
            strcpy(ret_whay_not, "no device exists");		
        return false;
    }
  
    // try to open the device
    memset((char *)&wf, 0, sizeof(wf));	        
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = (WORD)channels;
    wf.nSamplesPerSec = (DWORD)sampleFreq;
    wf.wBitsPerSample = (WORD)bitsPerSample;
    wf.nAvgBytesPerSec = (DWORD)channels * sampleFreq * (bitsPerSample / 8);
    wf.nBlockAlign = (WORD)(channels*(bitsPerSample / 8));    	
	
    if(waveInOpen(&device, WAVE_MAPPER, &wf, (DWORD)MMSCallback, (DWORD)this, CALLBACK_FUNCTION))
    {
        if(ret_whay_not)
            strcpy(ret_whay_not, "can not open device");
        return false;
    }				

    waveInReset(device);
    waveInClose(device);	       

    return true;
}
