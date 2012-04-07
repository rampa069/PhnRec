/**************************************************************************
 *  copyright           : (C) 2004-2006 by Lukas Burget UPGM,FIT,VUT,Brno *
 *  email               : burget@fit.vutbr.cz                             *
 **************************************************************************
 *                                                                        *
 *  This program is free software; you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation; either version 2 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 **************************************************************************/

#ifndef _DECODER_H
#define _DECODER_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#ifndef long_long
  #if defined WIN32 && !defined __CYGWIN__
    typedef __int64 long_long;
  #else
    typedef long long long_long;
  #endif
#endif
  
// callback function definition
typedef void (*DECODER_CALLBACK)(unsigned int Message, char *word, long_long start, long_long stop, float score, void *tmp);
  
// decoder modes
#define DECMODE_DECODE     1
#define DECMODE_KWS        2
  
// error modes
#define DECERR_NONE        0
#define DECERR_INPUTFILE   1
#define DECERR_OUTPUTFILE  2
#define DECERR_NOTINIT     3
#define DECERR_NETWORK     4
#define DECERR_ALREADYINIT 5
  
// messages
#define DECMSG_WORD        1
#define DECMSG_NEWESTIM    2
  
  
class Decoder
{
  protected:              
    DECODER_CALLBACK  callbackFunc;
    void*             callbackTmpParam;
    unsigned int      mode;	
    float             wPenalty;
            
  public:
    Decoder();
    virtual ~Decoder();
        
    // Batch processing
    int ProcessFile(char *features, char *label_file, float *score);
      
    // Per frame processing
    virtual int Init(char *label_file = 0);
    virtual void ProcessFrame(float *frame);
    virtual float Done();	
  
    // Callback function called by ProcessFrame, Done or by ProcessFile function
    void SetCallbackFunc(DECODER_CALLBACK func, void *param) {callbackFunc = func; callbackTmpParam = param;};	
  
    // Configuration
    virtual void SetMode(int m) {mode = m;};
    virtual void SetWPenalty(float v) {wPenalty = v;};
};

#endif
