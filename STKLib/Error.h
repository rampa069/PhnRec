//
// C++ Interface: %{MODULE}
//
// Description: 
//
//
// Author: %{AUTHOR} <%{EMAIL}>, (C) %{YEAR}
//
// Copyright: See COPYING file that comes with this distribution
//
//

/** @file Error.h
 *  This header defines several types and functions relating to the
 *  handling of exceptions in STK.
 */
 
#ifndef STK_Error_h
#define STK_Error_h

#include <stdexcept>
#include <string>


#define Error(...) _Error_(__func__, __FILE__, __LINE__, __VA_ARGS__)
#define Warning(...) _Warning_(__func__, __FILE__, __LINE__, __VA_ARGS__)


namespace STK
{
  /** 
   * @brief StackRecord represents a record in the traced stack
   */
  struct StackRecord
  {
  private:
    std::string       mMessage;     ///< Message to be displayed
    StackRecord *     mpNext;       ///< pointer to the next record

  public:  
    // Class Exception will be our friend so it has access to the private
    // members
    friend class Exception;
    
    std::string       mFileName;    ///< Name of the source file
    int               mLineNumber;  ///< Line in the source file
    
    /// Basic constructor
    StackRecord();                  
    
    /// Constructor
    StackRecord(const std::string & rMessage);
    
    ///< The destructor
    ~StackRecord() throw ();
    
    
    /**
     * @brief Dumps a signle record
     * @param rOs output stream 
     * @param rRecord which record to print
     * @return output stream
     */
    friend std::ostream &
    operator << (std::ostream & rOs, const StackRecord & rRecord);

    /**
     * @brief Dumps the list of records
     * @param rOs output stream
     * @param pRecord which recrod to start with
     * @return output stream
     *
     * This operator dumps each record until mpNext == NULL
     */
    friend std::ostream &
    operator << (std::ostream & rOs, const StackRecord * pRecord);
  };
  
  
  /**
   * @brief This class represents the basic exception to throw
   */
  class Exception : public std::exception
  {
    
  private:
    /// Represents basic message to display
    std::string         mMessage;
    
    /// Stack representation
    StackRecord *       mpStackTop;
    StackRecord *       mpStackBottom;
    
  public:
    Exception(const std::string & rMsg);
    
    ~Exception() throw ();
    
    
    /**
     * @brief Adds a record to the stack
     * @param rStack data to add
     */
    void
    PushBack(const StackRecord & rStack);    
    
    virtual const char *
    what() const throw();
    
    /**
     * @brief Dumps the exception to the specified ostream object
     * @param rOs output stream
     * @param rException exception to dump
     * @return output stream
     */
    friend std::ostream &
    operator << (std::ostream & rOs, const STK::Exception & rException);
  }; // class Exception
  
  
  
  /**
   *  @brief Error throwing function
   */
  void _Error_(const char *func, const char *file, int line, char *msg, ...);
  
  /**
   *  @brief Warning handling function
   */
  void _Warning_(const char *func, const char *file, int line, char *msg, ...);
    
  /**
   *  @brief Warning handlingfunction
   */
  void TraceLog(char *msg, ...);

}; // namespace STK

//#define STK_Error_h
#endif
