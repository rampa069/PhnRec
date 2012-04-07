#ifndef STK_StkStream_tcc
#define STK_StkStream_tcc

#include <iostream>
#include <string>
#include <string.h>
#pragma GCC system_header

namespace STK
{
  /*
  char * ExpandFilterCommand(const char *command, const char *filename)
  {

    char *out, *outend;
    const char *chrptr = command;
    int ndollars = 0;
    int fnlen = strlen(filename);

    while (*chrptr++) ndollars += (*chrptr ==  *gpFilterWldcrd);

    out = (char*) malloc(strlen(command) - ndollars + ndollars * fnlen + 1);
    //if (out == NULL) Error("Insufficient memory");

    outend = out;

    for (chrptr = command; *chrptr; chrptr++) {
      if (*chrptr ==  *gpFilterWldcrd) {
        strcpy(outend, filename);
        outend += fnlen;
      } else {
        *outend++ = *chrptr;
      }
    }
    *outend = '\0';
    return out;
  };
  */


  //******************************************************************************
  template<typename _CharT, typename _Traits>
  basic_stkbuf<_CharT, _Traits> *
  basic_stkbuf<_CharT, _Traits>::
  close(void)
  {
    // we only want to close an opened file
    if (this->is_open())
    {
      // we want to call the parent close() procedure
      // :TODO: WHY??? 
      std::basic_filebuf<_CharT, _Traits>::close();

      // and for different stream type we perform different closing
      if (stream_type == basic_stkbuf::t_file)
      {
        fclose(fileptr);
      }
      else if (stream_type == basic_stkbuf::t_pipe)
      {
        pclose(fileptr);
      }
      else if (stream_type == basic_stkbuf::t_stdio)
      {

      }
      
      fileptr     = 0;
      filename    = "";
      mode        = ios_base::openmode(0);
      stream_type = basic_stkbuf::t_undef;
      return this;
    }
    else
      return 0;
  }


  template<typename _CharT, typename _Traits>
  void
  basic_stkbuf<_CharT, _Traits>::
  open_mode(ios_base::openmode __mode, int&, int&,  char* __c_mode)
  {
    bool __testb = __mode & ios_base::binary;
    bool __testi = __mode & ios_base::in;
    bool __testo = __mode & ios_base::out;
    bool __testt = __mode & ios_base::trunc;
    bool __testa = __mode & ios_base::app;

    if (!__testi && __testo && !__testt && !__testa)
      strcpy(__c_mode, "w");
    if (!__testi && __testo && !__testt && __testa)
      strcpy(__c_mode, "a");
    if (!__testi && __testo && __testt && !__testa)
      strcpy(__c_mode, "w");
    if (__testi && !__testo && !__testt && !__testa)
      strcpy(__c_mode, "r");
    if (__testi && __testo && !__testt && !__testa)
      strcpy(__c_mode, "r+");
    if (__testi && __testo && __testt && !__testa)
      strcpy(__c_mode, "w+");
    if (__testb)
      strcat(__c_mode, "b");
  }


  //******************************************************************************
  template<typename _CharT, typename _Traits>
  basic_stkbuf<_CharT, _Traits> *
  basic_stkbuf<_CharT, _Traits>::
  open(const string & fName, ios::openmode m, const string & filter)
  {
    basic_stkbuf<_CharT, _Traits> * _ret = NULL;

    // we need to assure, that the stream is not open
    if (!this->is_open())
    {


      char mstr[4] = {'\0', '\0', '\0', '\0'};
      int __p_mode = 0;
      int __rw_mode = 0;

      // now we decide, what kind of file we open

      if ( !fName.compare("-") )
      {
        if      ((m & ios::in) && !(m & ios::out))
        {
          fileptr = stdin;
          mode = ios::in;
          filename = fName;
          stream_type = t_stdio;
          _ret = this;
        }
        else if ((m & ios::out) && !(m & ios::in))
        {
          fileptr = stdout;
          mode = ios::out;
          filename = fName;
          stream_type = t_stdio;
          _ret = this;
        }
        else
          _ret = NULL;
      }
      else if ( fName[0] == '|' )
      {
        const char * command = fName.c_str() + 1;

        if      ((m & ios::in) && !(m & ios::out)) m = ios::in;
        else if ((m & ios::out) && !(m & ios::in)) m = ios::out;
        else return NULL;

        // we need to make some conversion
        // iostream -> stdio open mode string
        this->open_mode(m, __p_mode, __rw_mode, mstr);

        if (fileptr = popen(command, mstr))
        {
          filename = command;
          mode     = m;
          stream_type = t_pipe;
          _ret = this;
        }
        else
          _ret = 0;
      }
      else
      {
        // maybe we have a filter specified
        if (!filter.empty())
        {
          char * command = expandFilterCommand(filter.c_str(), fName.c_str());

          if      ((m & ios::in) && !(m & ios::out)) m = ios::in;
          else if ((m & ios::out) && !(m & ios::in)) m = ios::out;
          else return NULL;

          // we need to make some conversion
          // iostream -> stdio open mode string
          this->open_mode(m, __p_mode, __rw_mode, mstr);

          if (fileptr = popen(command, mstr))
          {
            filename = fName;
            this->filter = filter;
            mode     = m;
            stream_type = t_pipe;
            _ret = this;
          }
          else
            _ret = 0;
        }
        else // if (!filter.empty())
        {
          // we need to make some conversion
          // iostream -> stdio open mode string
          this->open_mode(m, __p_mode, __rw_mode, mstr);

          if (fileptr = fopen(fName.c_str(), mstr))
          {
            filename = fName;
            mode     = m;
            stream_type = t_file;
            _ret = this;
          }
          else
            _ret = NULL;
        }
      }

      // here we perform what the stdio_filebuf would do
      if (_ret)
      {
        superopen(fileptr, m);
      }
    } //if (!isopen)

    return _ret;
  }

  //******************************************************************************
  template<typename _CharT, typename _Traits>
  void
  basic_stkbuf<_CharT, _Traits>::
  superopen(std::__c_file* __f, std::ios_base::openmode __mode,
        size_t __size)
  {
    this->_M_file.sys_open(__f, __mode);
    if (this->is_open())
    {
      this->_M_mode = __mode;
      this->_M_buf_size = __size;
      this->_M_allocate_internal_buffer();
      this->_M_reading = false;
      this->_M_writing = false;
      this->_M_set_buffer(-1);
    }
  }
}

// STK_StkStream_tcc
#endif
