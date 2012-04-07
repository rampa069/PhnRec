/**
 *  @file   labelreader.h
 *
 *  @brief  Classes for mass label input manipulation
 *
 *  This file defines classes for mass label manipulation. Its main purpose is
 *  to offer optimized access to (master) label files (MLFs), i.e. hashing
 *  capabilities.
 *
 *  Searching diagram is <a href="../../Documentation/figures/eps/hashing.eps">
 *  doc/Documentation/figures/hashing.eps</a>
 */

#ifndef STK_labelreader_h
#define STK_labelreader_h

#include <map>
#include <list>
#include <set>
#include "stkstream.h"

#define MAX_LABEL_DEPTH 1024


namespace STK
{
  class LabelStream;
  class LabelStore;


  /// type of the container used to store the labels
  typedef  std::map<string, LabelStream>               LabelHashType;

  /// this container stores the lables in linear order as they came
  /// i.e. they cannot be hashed
  typedef  std::list< std::pair<string,LabelStream> *> LabelListType;

  /**
   *  @brief Describes type of MLF definition
   *
   *  See HTK book for MLF structure. Terms used in STK are
   *  compatible with those in HTK book.
   */
  enum MlfDefType
  {
    MLF_DEF_UNKNOWN = 0,              ///< unknown definition
    MLF_DEF_IMMEDIATE_TRANSCRIPTION,  ///< immediate transcription
    MLF_DEF_SUB_DIR_DEF               ///< subdirectory definition
  };


  /**
   *  @brief Returns a type of MLF definition
   *  @param rLine string to be parsed
   *
   */
  extern MlfDefType MlfDefinition(const string & line);

  /**
   *  @brief Holds association between label and stream
   */
  class LabelStream
  {
    void *                   pData;
    void *                   mpSubDirDef;

  public:
    LabelStream() : miLabelListLimit(NULL), mpSubDirDef(NULL) {}

    ~LabelStream()
    {
      if (mpSubDirDef == NULL)
        delete pSubDirDef();
    }

    /// definition type
    MlfDefType                mDefType;

    /// pointer to the stream
    istream *                 mpStream;

    /// position of the label in the stream
    int                       mStreamPos;

    /**
     *  @brief points to the current end of the LabelList
     *
     *  The reason for storing this value is to know when we inserted
     *  a label into the hash. It is possible, that the hash label came
     *  after list label, in which case the list label is prefered
     */
    LabelListType::iterator   miLabelListLimit;

    istream *
    pSubDirDef()
    {
      return static_cast<istream *> (mpSubDirDef);
    }
  };


  /**
   *  @brief Provides an interface to label hierarchy and searching
   *
   *  This class stores label files in a map structure. When a wildcard
   *  convence is used, the class stores the labels in separate maps according
   *  to level of wildcard abstraction. By level we mean the directory structure
   *  depth.
   */
  class LabelStore
  {
  private:

    /// type used for directory depth notation
    typedef  size_t                 DepthType;


    /// this set stores depths of * labels observed at insertion
    std::set<DepthType>             mDepths;

    /// stores the labels
    LabelHashType                   mLabelMap;
    LabelListType                   mLabelList;

    /// true if labels are to be sought by hashing function (fast) or by
    /// sequential search (slow)
    bool                            mUseHashedSearch;

    /// if Find matches the label, this var stores the pattern that matched the
    /// query
    std::string                     mMatchedPattern;

    /// if Find matches the label, this var stores the the masked characters.
    /// The mask is given by '%' symbols
    std::string                     mMatchedPatternMask;

    /**
     *  @brief Returns the directory depth of path
     */
    size_t
    DirDepth(const string & path);


  public:
    /// The constructor
    LabelStore() : mUseHashedSearch(true) {}

    /// The destructor
    ~LabelStore();

    /**
     *  @brief Inserts new label to the hash structure
     */
    void
    Insert(
      const string &      rLabel,
      istream *           pStream);

    /**
     *  @brief Inserts new label to the hash structure
     */
    void
    Insert(
      const string &      rLabel,
      istream *           pStream,
      std::streampos      Pos);


    /**
     *  @brief Looks for a record in the hash
     */
    bool
    FindInHash(
      const string &      rLabel,
      LabelStream &       rLS);

    /**
     *  @brief Looks for a record in the list
     *  @param rLabel Label to look for
     *  @param rLS    Structure to fill with found data
     *  @param limitSearch If true @p rLS's @c mLabelListLimit gives the limiting position in the list
     */
    bool
    FindInList(
      const string &      rLabel,
      LabelStream &       rLS,
      bool                limitSearch = false);

    /**
     *  @brief Looks for a record
     */
    bool
    Find(
      const string &      rLabel,
      LabelStream &       rLS);

    /**
     *  @brief Returns the matched pattern
     */
    const string &
    MatchedPattern() const
    {
      return mMatchedPattern;
    }

    /**
     *  @brief Returns the matched pattern mask (%%%)
     */
    const string &
    MatchedPatternMask() const
    {
      return mMatchedPatternMask;
    }
  };


  /**
   *  @brief Provides an interactive interface to label manipulation in terms of reading
   */
  class LabelReader
  {
  public:
    /// This container stores the open MLFs
    typedef std::list<istream *>
                              StreamListType;

  public:
    /// State flag constants
    static const unsigned int FLAG_EOL = 1;

  protected:
    /// current istream iterator to read from
    StreamListType::iterator  miCurStream;

    /// current istream to read from
    std::istream *            mpCurStream;

    /// this is a temporary stream. Non-NULL pointer indicates that the stream
    /// should be closed and the stream object deleted after usage.
    std::istream *            mpTempStream;

    /// holds current label name
    std::string               mCurLabelName;

    /// the labels are hashed here
    LabelStore                mLabelStore;

    /// the registered streams are put here...
    StreamListType            mStreamList;

    /// true if label are hashed
    bool                      mIsHashed;

    /// holds state flags
    unsigned int              mStateFlags;

    /// if Find matches the label, this var stores the pattern that matched the
    /// query
    std::string               mMatchedPattern;

    /// if Find matches the label, this var stores the the masked characters.
    /// The mask is given by '%' symbols
    std::string               mMatchedPatternMask;

    /// holds last read line
    std::string               mLineBuffer;


    /// Opens a new label file
    bool
    OpenLabelFile(const string & lFile);

  public:
    typedef LabelReader       ThisType;

    /// The constructor
    LabelReader() :
      mpCurStream(NULL),
      mpTempStream(NULL),
      mCurLabelName(""),
      mIsHashed(false)
    {}

    /**
     *  @brief The destructor
     *
     *  All open streams need to be closed.
     */
    ~LabelReader();

    /**
     *  @brief Registeres one MLF for use in application
     *  @param fName MLF full filename
     *  @return Pointer to this instance on success, otherwise NULL pointer
     */

    bool
    RegisterMLF(const string & rFName);


    /**
     *  @brief Gives access to the input stream
     *  @return pointer to an IStkStream which is set up by this class (propper
     *          position, file, etc.)
     */
    istream *
    pInput();


    /**
     *  @brief Looks for a record in the MLF stream list
     *  @param rLabel Label to look for
     *  @param rLS    Structure to fill with found data
     *  @return True on success
     */
    bool
    FindInStreamList(
      const string &      rLabel,
      LabelStream &       rLS);


    /**
     *  @brief Looks for a record in the LabelStore
     *  @param rLabel Label to look for
     *  @param rLS    Structure to fill with found data
     *  @return True on success
     */
    bool
    FindInLabelStore(
      const string &      rLabel,
      LabelStream &       rLS);


    /**
     *  @brief Tries to open a specified label for reading
     *  @param label label to open
     *  @return true on success, false on failure
     *
     *  This method looks for the label in the registered MLF's first. On success,
     *  the curStream is set to the MLF and its position is set properly. On failure,
     *  file of the name label is opened. On failure, NULL pointer is returned.
     */
    bool
    Open(const string & rLabel);


    /**
     *  @brief Returns true if End-Of-Label reached
     *  @param s pointer to an array of characters
     */
    bool
    JumpToNextDefinition();


    /**
     *  @brief Returns current label name
     */
    const std::string &
    CurLabelName() const
    {
      return mCurLabelName;
    }


    /**
     *  @brief Reads a single line from the current label
     *  @param rLine string to read into
     *
     *  This should really be the only access to the label because the LabelReader
     *  class manages reading from different files and the only way to controll
     *  this is to have internal function(s) for reading.
     */
    bool
    GetLine(std::string & rLine);


    /**
     *  @brief Returns true if End-Of-Label reached
     */
    bool
    EOL() const
    {
      return (this->mStateFlags & FLAG_EOL);
    }

    /**
     *  @brief Returns true if no state flags are set
     */
    bool
    Good() const
    {
      return this->mStateFlags == 0;
    }


    void
    Clear()
    {
      this->mStateFlags = 0;
    }


    /**
     *  @brief Returns true if labels in MLF are hashed
     */
    bool
    IsHashed() const
    {
      return mIsHashed;
    }


    /**
     *  @brief Creates a full hash of all files so quick search can be performed
     */
    void
    HashAll();
  }; // class labelstore

}; // namespace STK


#endif // ifndef ...
