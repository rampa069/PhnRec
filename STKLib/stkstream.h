

/** @file stkstream.h
 *  This is an STK C++ Library header.
 */


#ifndef STK_StkStream_h
#define STK_StkStream_h

#include "common.h"
#include <fstream>
#include <string>


#pragma GCC system_header

using std::ios_base;
using std::ios;
using std::streambuf;
using std::string;
using std::istream;
using std::ostream;


//extern const char * gpFilterWldcrd;

namespace STK
{

  /**
   *  @brief Expands a filter command into a runnable form
   *
   *  This function replaces all occurances of *filter_wldcard in *command by
   *  *filename
   */
  //char * ExpandFilterCommand(const char *command, const char *filename);

  /**
   *  @brief Provides a layer of compatibility for C/POSIX.
   *
   *  This GNU extension provides extensions for working with standard C
   *  FILE*'s and POSIX file descriptors.  It must be instantiated by the
   *  user with the type of character used in the file stream, e.g.,
   *  basic_stkbuf<char>.
   */
  template<typename _CharT, typename _Traits = std::char_traits<_CharT> >
    class basic_stkbuf : public std::basic_filebuf<_CharT, _Traits>
    {
    public:


      // Types:
      typedef _CharT                        char_type;
      typedef _Traits                       traits_type;
      typedef basic_stkbuf<char_type, traits_type>     __filebuf_type;

      typedef typename traits_type::int_type        int_type;
      typedef typename traits_type::pos_type        pos_type;
      typedef typename traits_type::off_type        off_type;
      typedef std::size_t                           size_t;

    public:

      /// @{
      /// Type of streambuffer
      static const unsigned int t_undef  = 0; ///< undefined
      static const unsigned int t_file   = 1; ///< file stream
      static const unsigned int t_pipe   = 2; ///< pipe
      static const unsigned int t_filter = 4; ///< filter
      static const unsigned int t_stdio  = 8; ///< standard input/output
      /// @}

    private:
      string         filter;

      /// Holds the full file name
      string         filename;

      ios_base::openmode       mode;

      /// Holds a pointer to the main FILE structure
      FILE *         fileptr;

      /// converts the ios::xxx mode to stdio style
      static void open_mode(ios_base::openmode __mode, int&, int&,  char* __c_mode);

      /// tells what kind of stream we use (stdio, file, pipe)
      unsigned int stream_type;

      /**
       *  @param  __f  An open @c FILE*.
       *  @param  __mode  Same meaning as in a standard filebuf.
       *  @param  __size  Optimal or preferred size of internal buffer, in chars.
       *                Defaults to system's @c BUFSIZ.
       *
       *  This method associates a file stream buffer with an open
       *  C @c FILE*.  The @c FILE* will not be automatically closed when the
       *  basic_stkbuf is closed/destroyed. It is equivalent to one of the constructors
       *  of the stdio_filebuf class defined in GNU ISO C++ ext/stdio_filebuf.h
      */
      void superopen(std::__c_file* __f, std::ios_base::openmode __mode,
            size_t __size = static_cast<size_t>(BUFSIZ));

    public:

      /**
      * deferred initialization
      */
      basic_stkbuf() : std::basic_filebuf<_CharT, _Traits>(),
        filter(""), filename(""), fileptr(0), stream_type(t_undef){}

      /**
       *  @brief  Opens a stream.
       *  @param  fName  The name of the file.
       *  @param  m      The open mode flags.
       *  @param  filter The filter command to use
       *  @return  @c this on success, NULL on failure
       *
       *  If a file is already open, this function immediately fails.
       *  Otherwise it tries to open the file named @a s using the flags
       *  given in @a mode.
       *
       *  [Table 92 gives the relation between openmode combinations and the
       *  equivalent fopen() flags, but the table has not been copied yet.]
       */
      basic_stkbuf(const string & fName, std::ios_base::openmode m, const string & filter="");

      /**
      *  @return  The underlying FILE*.
      *
      *  This function can be used to access the underlying "C" file pointer.
      *  Note that there is no way for the library to track what you do
      *  with the file, so be careful.
      */
      std::__c_file*
      file() { return this->_M_file.file(); }

      /**
       *  @brief  Opens an external file.
       *  @param  fName  The name of the file.
       *  @param  m      The open mode flags.
       *  @param  filter The filter command to use
       *  @return  @c this on success, NULL on failure
       *
       *  If a file is already open, this function immediately fails.
       *  Otherwise it tries to open the file named @a s using the flags
       *  given in @a mode.
       *
       *  [Table 92 gives the relation between openmode combinations and the
       *  equivalent fopen() flags, but the table has not been copied yet.]
       */
      __filebuf_type *
      open(const string & fName, ios_base::openmode m, const string & filter="");
      /**
       *  @brief  Closes the currently associated file.
       *  @return  @c this on success, NULL on failure
       *
       *  If no file is currently open, this function immediately fails.
       *
       *  If a "put buffer area" exists, @c overflow(eof) is called to flush
       *  all the characters.  The file is then closed.
       *
       *  If any operations fail, this function also fails.
       */
      __filebuf_type *
      close();

      /**
      *  Closes the external data stream if the file descriptor constructor
      *  was used.
      */
      virtual
      ~basic_stkbuf() {close();};

      /// Returns the file name
      const string name() const {return filename;}

    };


  /**
   * We define some implicit stkbuf class
   */
  ///@{
  typedef basic_stkbuf<char>          stkbuf;
#ifdef _GLIBCPP_USE_WCHAR_T
  typedef basic_stkbuf<wchar_t>       stkbuf;           ///< @isiosfwd
#endif
  /// @}


  /**
   *  @brief This extension wraps stkbuf stream buffer into the standard ios class.
   *
   *  This class is inherited by (i/o)stkstream classes which make explicit use of
   *  the custom stream buffer
   */
  class stkios : virtual public ios
  {
  protected:
    stkbuf  buf;
  
  public:
    stkios() : buf() { init(&buf) ;};
    stkios(const string& fName, ios::openmode m, const string & filter) :
      buf(fName, m, filter) { init(&buf) ; }

    const stkbuf & rdbuf() const  { return buf; }
  };


  /**
   *  @brief  Controlling input for files.
   *
   *  This class supports reading from named files, using the inherited
   *  functions from std::istream.  To control the associated
   *  sequence, an instance of std::stkbuf is used.
  */
  class IStkStream : public stkios, public istream
  {
  public:
    // Constructors:
    /**
     *  @brief  Default constructor.
     *
     *  Initializes @c buf using its default constructor, and passes
     *  @c &sb to the base class initializer.  Does not open any files
     *  (you haven't given it a filename to open).
     */
    IStkStream() : stkios() {};

    /**
     *  @brief  Create an input file stream.
     *  @param  fName  String specifying the filename.
     *  @param  m      Open file in specified mode (see std::ios_base).
     *  @param  filter String specifying filter command to use on fName
     *
     *  @c ios_base::in is automatically included in
     *  @a m.
     *
     *  Tip:  When using std::string to hold the filename, you must use
     *  .c_str() before passing it to this constructor.
    */
    IStkStream(const string& fName, ios::openmode m=ios::out, const string & filter=""):
      stkios() {this->open(fName, ios::in, filter);}

    ~IStkStream() 
    {
      this->close();
    }
      
    /**
    *  @brief  Opens an external file.
    *  @param  s  The name of the file.
    *  @param  mode  The open mode flags.
    *  @param  filter The filter command to use
       *
    *  Calls @c std::basic_filebuf::open(s,mode|in).  If that function
    *  fails, @c failbit is set in the stream's error state.
    *
    *  Tip:  When using std::string to hold the filename, you must use
    *  .c_str() before passing it to this constructor.
    */
    void open(const string & fName, ios::openmode m=ios::in, const string & filter = "")
    {
      if (!buf.open(fName, m | ios_base::in, filter))
        this->setstate(ios_base::failbit);
      else
      // Closing an fstream should clear error state
        this->clear();
    }

    /**
    *  @brief  Returns true if the external file is open.
    */
    bool is_open() const {return buf.is_open();}

    /**
    *  @brief  Closes the stream
    */
    void close() {buf.close();}

    /**
    *  @brief  Returns the filename
    */
    const string name() const {return buf.name();}

    /// Returns a pointer to the main FILE structure
    std::__c_file*
    file() {return buf.file();}

    /**
     *  @brief  Reads a single line
     *
     *  This is a specialized function as std::getline does not provide a way to
     *  read multiple end-of-line symbols (we need both '\n' and EOF to delimit
     *  the line)
     */
    void
    GetLine(string & rLine);

  }; // class IStkStream


  /**
   *  @brief  Controlling output for files.
   *
   *  This class supports reading from named files, using the inherited
   *  functions from std::ostream.  To control the associated
   *  sequence, an instance of STK::stkbuf is used.
  */
  class OStkStream : public stkios, public ostream
  {
  public:

    // Constructors:
    /**
     *  @brief  Default constructor.
     *
     *  Initializes @c sb using its default constructor, and passes
     *  @c &sb to the base class initializer.  Does not open any files
     *  (you haven't given it a filename to open).
     */
    OStkStream() : stkios() {};

    /**
     *  @brief  Create an output file stream.
     *  @param  fName  String specifying the filename.
     *  @param  m      Open file in specified mode (see std::ios_base).
     *  @param  filter String specifying filter command to use on fName
     *
     *  @c ios_base::out|ios_base::trunc is automatically included in
     *  @a mode.
     *
     *  Tip:  When using std::string to hold the filename, you must use
     *  .c_str() before passing it to this constructor.
     */
    OStkStream(const string& fName, ios::openmode m=ios::out, const string & filter=""):
      stkios(fName, m, filter) {}

    /**
    *  @brief  Opens an external file.
    *  @param  fName  The name of the file.
    *  @param  m  The open mode flags.
    *  @param  filter String specifying filter command to use on fName
    *
    *  Calls @c std::basic_filebuf::open(s,mode|out).  If that function
    *  fails, @c failbit is set in the stream's error state.
    *
    *  Tip:  When using std::string to hold the filename, you must use
    *  .c_str() before passing it to this constructor.
    */
    void open(const string & fName, ios::openmode m=ios::out, const string & filter="")
    {
      if (!buf.open(fName, m | ios_base::out, filter))
        this->setstate(ios_base::failbit);
      else
      // Closing an fstream should clear error state
        this->clear();
    }

    /**
    *  @brief  Returns true if the external file is open.
    */
    bool is_open() const {return buf.is_open();}

    /**
    *  @brief  Closes the stream
    */
    void close() {buf.close();}

    /**
    *  @brief  Returns the filename
    */
    const string name() const {return buf.name();}

    /// Returns a pointer to the main FILE structure
    std::__c_file*
    file() {return buf.file();}

  }; // class OStkStream
}; // namespace STK

# include "stkstream.tcc"

// STK_StkStream_h
#endif 
