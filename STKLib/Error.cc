//
// C++ Implementation: %{MODULE}
//
// Description:
//
//
// Author: %{AUTHOR} <%{EMAIL}>, (C) %{YEAR}
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "Error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

namespace STK
{
  //****************************************************************************
  //****************************************************************************
  void _Error_(const char *func, const char *file, int line, char *msg, ...) 
  {
    va_list ap;
    va_start(ap, msg);
    fflush(stdout);
    fprintf(stderr, "ERROR (%s:%d) %s: ", file, line, func);
    vfprintf(stderr, msg, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(1);
  }
  
  //****************************************************************************
  //****************************************************************************
  void _Warning_(const char *func, const char *file, int line, char *msg, ...) 
  {
    va_list ap;
    va_start(ap, msg);
    fflush(stdout);
    fprintf(stderr, "WARNING (%s:%d) %s: ", file, line, func);
    vfprintf(stderr, msg, ap);
    fputc('\n', stderr);
    va_end(ap);
  }
  
  //****************************************************************************
  //****************************************************************************
  void TraceLog(char *msg, ...) 
  {
    va_list ap;
    va_start(ap, msg);
    fflush(stderr);
    vfprintf(stdout, msg, ap);
    fputc('\n', stdout);
    va_end(ap);
  }

  
  //############################################################################
  //############################################################################
  // StackRecord section
  //############################################################################
  //############################################################################
  StackRecord::
  StackRecord() : mMessage(""), mpNext(NULL)
  {
  } // StackRecord()

  StackRecord::
  StackRecord(const std::string & rMessage) : mMessage(rMessage), mpNext(NULL)
  {    
  } //StackRecord(const std::string & rMessage)
    
  //****************************************************************************
  //****************************************************************************
  StackRecord::
  ~StackRecord() throw ()
  {
    if (mpNext != NULL)
      delete mpNext;
  } // ~ StackRecord()


  //############################################################################
  //############################################################################
  // Exception section
  //############################################################################
  //############################################################################
  Exception::
  Exception(const std::string & rMsg) :
    mMessage(rMsg), mpStackTop(NULL), mpStackBottom(NULL)
  {
  } // Exception(const char * pMsg)
    
  //****************************************************************************
  //****************************************************************************
  Exception::
  ~Exception() throw ()
  {
    // delete the stack (will recursively delete each next element)
    if (mpStackTop != NULL)
      delete mpStackTop;
  } // ~Exception()
   
  //****************************************************************************
  //****************************************************************************
  void
  Exception::
  PushBack(const StackRecord & rStack)
  {
    StackRecord * ret;
    
    // we put everything in a try block as we don't want any other exception
    // to be thrown
    try
    {
      ret = new StackRecord(rStack);
      
      if (mpStackTop == NULL)
      {
        mpStackTop = mpStackBottom = ret;
      }
      else
      {
        // set next link to current stack bottom
        mpStackBottom->mpNext = ret;
        // update stack bottom
        mpStackBottom = ret;
      }
    }
    catch(...)
    {
    }
  } // PushBack(const StackRecord & rStack)
  
  //****************************************************************************
  //****************************************************************************
  /* virtual */
  const char *
  Exception::
  what() const throw()
  {
    return mMessage.c_str();
  } // what() const throw()
      
  //****************************************************************************
  //****************************************************************************
  /* friend */
  /*
  std::ostream &
  operator << (std::ostream & rOs, const STK::Exception & rException)
  {
    rOs << rException.what();
    return rOs;
  }
  */
} // namespace STK
