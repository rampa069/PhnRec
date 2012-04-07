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

#ifndef _WFSOURCE_H
#define _WFSOURCE_H

#include <windows.h>
#include <mmsystem.h>	
#include <exception>
#include <new>

#define CWFS_BUFFERLENGTH    1000     // 1 s
#define CWFS_FRAMELENGTH      100     // 100 ms

// VC directive
//#pragma comment(lib, "winmm.lib")     // link the WINMM.LIB library
//                                      // if this library does not exist, import it with the libimp 
//                                      // or the tlibimp tool from WINMM.DLL

#ifndef CALLBACK
   #define CALLBACK _pascal
#endif

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


class CWFSource
{
protected:
    HANDLE event;

    int sampleFreq;
    int channels;
    int bitsPerSample;

    HWAVEIN device;
    tWAVEFORMATEX wf;
    WAVEHDR header1;
    WAVEHDR header2;
    WAVEHDR *header;

    char *buffer;
    char *bufferLastByte;
    int bufferLen;
    int frameLen;
    char *reading;
    char *storing;
    volatile int bytesRecorded;
    volatile int recFrames;
    volatile int actFrame;
    volatile bool isRecording;

    static void CALLBACK MMSCallback(HWAVEIN hwi, UINT uMSG, DWORD dwInstance,
                                     DWORD dwParam1, DWORD dwParam2);
    void onData();
    void recordFrame();
public:
    CWFSource();
    ~CWFSource();
    bool isAvailable(char* ret_whay_not = 0);
    void open();  // throw(wf_error, std::bad_alloc)
    void close();
    void read(char *buff, int n); // throw(wf_error);
    void setFormat(int sf, int ch, int bps) {sampleFreq = sf; channels = ch; bitsPerSample = bps;};
};

#endif
