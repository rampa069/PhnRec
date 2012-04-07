#include "labelreader.h"
#include "common.h"
#include <iostream>
#include <stdexcept>

using namespace std;
namespace STK
{
  //******************************************************************************
  MlfDefType MlfDefinition(const string & line)
  {
    // TODO
    // we are only looking for the -> and => symbols, but we don't really parse
    // line now, so next step would be to really parse the line
    // TODO

    size_t pos = 0;

    while (((pos = line.find_first_of("=-", pos)) != line.npos) ||
           (pos < line.size()))
    {
      if (line[pos++] == '>')
        return MLF_DEF_SUB_DIR_DEF;
    }
    return MLF_DEF_IMMEDIATE_TRANSCRIPTION;
  } // MlfDefType MlfDefinition(const string & line)

  //******************************************************************************
  LabelStore::
  ~LabelStore()
  {
    while (!this->mLabelList.empty())
    {
      delete this->mLabelList.back();
      this->mLabelList.pop_back();
    }
  }

  //******************************************************************************
  size_t
  LabelStore::
  DirDepth(const string & rPath)
  {
    size_t depth     = 0;
    size_t length    = rPath.length();
    const char * s   = rPath.c_str();

    for (size_t i = 0; i < length; i++)
    {
      if (*s == '/' || *s == '\\')
      {
        depth++;
      }
      s++;
    }
    return depth;
  }


  //******************************************************************************
  void
  LabelStore::
  Insert(const string &  rLabel,
         istream *       pStream,
         std::streampos  Pos)
  {
    LabelStream     ls;
    size_t          depth;
    LabelStream     tmp_ls;

    // we need to compute the depth of the label path if
    // wildcard is used
    // do we have a wildcard???
    if (rLabel[0] == '*')
    {
      depth = this->DirDepth(rLabel);
    }
    else
    {
      depth = MAX_LABEL_DEPTH;
    }

    // perhaps we want to store the depth of the path in the label for the wildcards
    // to work
    this->mDepths.insert(depth);

    // store the values
    ls.mpStream       = pStream;
    ls.mStreamPos     = Pos;
    ls.miLabelListLimit = mLabelList.end();


    if (mLabelList.begin() != mLabelList.end())
      ls.miLabelListLimit--;

    if (rLabel.find_first_of("*?%",1) == rLabel.npos)
    {
      if (!Find(rLabel, tmp_ls))
      {
        // look in the
        this->mLabelMap[rLabel] = ls;
      }
      else
      {
        // TODO
        // we want to print out some warning here, that user is adding
        // a label which will never be found because more general
        // definition was found in the list before
        // TODO
        std::cerr << "Warning... more general definition found when inserting " << rLabel << " ... ";
        std::cerr << "file: " << dynamic_cast<IStkStream *>(tmp_ls.mpStream)->name() << " ";
        std::cerr << "label: " << MatchedPattern() ;
        std::cerr << std::endl;
      }
    }
    else
    {
      this->mLabelList.push_back(new std::pair<string,LabelStream>(rLabel, ls));
    }
  };

  //******************************************************************************
  void
  LabelStore::
  Insert(const string & rLabel, istream * pStream)
  {
    this->Insert(rLabel, pStream, pStream->tellg());
  };


  //******************************************************************************
  bool
  LabelStore::
  FindInHash(const string & rLabel, LabelStream & rLS)
  {
    bool run   = true;
    bool found = false;

    string str;

    // current depth within the str
    DepthType  current_depth    = MAX_LABEL_DEPTH;

    // current search position within the str
    size_t     prev             = rLabel.size() + 1;

    // we will walk through the set depts bacwards so we begin at the end and move
    // to the front...
    std::set<DepthType>::reverse_iterator ri    (this->mDepths.end());
    std::set<DepthType>::reverse_iterator rlast (this->mDepths.begin());
    LabelHashType::iterator               lab;

    // we perform the search until we run to the end of the set or we find something
    while ((!found) && (ri != rlast))
    {
      // we don't need to do anything with the string if the depth is set to
      // max label depth since it contains no *
      if (*ri == MAX_LABEL_DEPTH)
      {
        found = ((lab=this->mLabelMap.find(rLabel)) != this->mLabelMap.end());
        if (found) str = rLabel;
      }
      // we will crop the string and put * in the begining and try to search
      else
      {
        // we know that we walk backwards in the depths, so we need to first find
        // the last / and
        if (current_depth == MAX_LABEL_DEPTH)
        {
          if (*ri > 0)
          {
            // we find the ri-th / from back
            for (DepthType i=1; (i <= *ri) && (prev != rLabel.npos); i++)
            {
              prev = rLabel.find_last_of("/\\", prev-1);
            }
          }
          else
          {
            prev = 0;
          }

          // check if finding succeeded (prev == str.npos => failure, see STL)
          if (prev != rLabel.npos)
          {
            // construct the new string beign sought for
            str.assign(rLabel, prev, rLabel.size());
            str = '*' + str;

            // now we try to find
            found = ((lab=this->mLabelMap.find(str)) != this->mLabelMap.end());

            // say, that current depth is *ri
            current_depth = *ri;
          }
          else
          {
            prev = rLabel.size() + 1;
          }
        }     // if (current_depth == MAX_LABEL_DEPTH)
        else
        {
          // now we know at which / we are from the back, so we search forward now
          // and we need to reach the ri-th /
          while (current_depth > *ri)
          {
            // we try to find next /
            if ((prev = rLabel.find_first_of("/\\", prev+1)) != rLabel.npos)
              current_depth--;
            else
              return false;
          }

          // construct the new string beign sought for
          str.assign(rLabel, prev, rLabel.size());
          str = '*' + str;

          // now we try to find
          found = ((lab=this->mLabelMap.find(str)) != this->mLabelMap.end());
        }
      }

      // move one element further (jump to next observed depth)
      ri++;
    } // while (run)

    // some debug info
    if (found)
    {
      rLS                   = lab->second;
      this->mMatchedPattern = str;
    }

    return found;
  }


  //******************************************************************************
  bool
  LabelStore::
  FindInList(const string & rLabel, LabelStream & rLS, bool limitSearch)
  {

    bool                      found = false;
    string                    str;
    LabelListType::iterator   lab   = mLabelList.begin();
    LabelListType::iterator   limit;

    if (limitSearch && (rLS.miLabelListLimit != NULL))
    {
      limit = rLS.miLabelListLimit;
      limit++;
    }
    else
    {
      limit = this->mLabelList.end();
    }

    // we perform sequential search until we run to the end of the list or we find
    // something
    while ((!found) && (lab != limit))
    {
      if (ProcessMask(rLabel, (*lab)->mpFirst, str))
      {
        found = true;
      }
      else
      {
        lab++;
      }
    } // while (run)

    // some debug info
    if (found)
    {
      rLS                       = (*lab)->second;
      this->mMatchedPattern     = (*lab)->mpFirst;
      this->mMatchedPatternMask = str;
    }
    return found;
  }


  //******************************************************************************
  bool
  LabelStore::
  Find(const string & rLabel, LabelStream & rLS)
  {
    // try to find the label in the Hash
    if (FindInHash(rLabel, rLS))
    {
      // we look in the list, but we limit the search.
      FindInList(rLabel, rLS, true);
      return true;
    } //if (this->mLabelStore.FindInHash(rLabel, label_stream))
    else
    {
      // we didn't find it in the hash so we look in the list
      return FindInList(rLabel, rLS);
    }
  }


  //****************************************************************************
  LabelReader::
  ~LabelReader()
  {
    if (mpTempStream != NULL)
    {
      delete mpTempStream;
    }

    // go through the list and delete every stream object = close the streams
    while (!mStreamList.empty())
    {
      delete mStreamList.front();
      mStreamList.pop_front();
    }
  }



  //****************************************************************************
  bool
  LabelReader::
  RegisterMLF(const string & rFName)
  {
    IStkStream *   new_stream = new IStkStream;

    // open the MLF
    new_stream->open(rFName);

    // if OK, we add it to the MLF list
    if (new_stream->good())
    {
      // add to the list
      mStreamList.push_back(new_stream);
      return true;
    }    // if (new_stream.good())
    else // else we delete the object and trow an error
    {
      delete new_stream;
      throw std::logic_error("Cannot open " + rFName);
      return false;
    }
  }; //RegisterMLF



  //****************************************************************************
  bool
  LabelReader::
  JumpToNextDefinition()
  {
    string buffer;
    bool   in_mlf_def= false;
    bool   done      = false;
    bool   read_lines = true;

    // we start reading
    if (mpCurStream == NULL)
    {
      if (!mStreamList.empty())
      {
        miCurStream = mStreamList.begin();
        mpCurStream = *miCurStream;
        mStateFlags = 0;
      }
      else
      {
        return false;
      }
    }

    // done when next label reached
    while (!done)
    {
      // read lines
      while (read_lines)
      {
        // maybe there's EOL set... then we know, that next read line might be
        if (getline(*mpCurStream, buffer))
        {
          // maybe there's EOL set which means that current line is the MLF
          // definition
          if (this->EOL())
          {
            read_lines = false;
            done       = true;

            ParseHTKString(buffer, mCurLabelName);
            mLabelStore.Insert(mCurLabelName, mpCurStream);   // insert into the storage
            this->Clear();                                    // clear all flags
            return true;
          } // if (this->EOL())

          // this was the initial MLF string ID
          else if (buffer == "#!MLF!#")
          {
            // we assume that next line is MLF_DEF, so we try to read it
            if (getline(*mpCurStream, buffer))
            {
              read_lines = false;
              done       = true;

              ParseHTKString(buffer, mCurLabelName);
              mLabelStore.Insert(mCurLabelName, mpCurStream);   // insert into the storage
              this->Clear();                                    // clear all flags
              return true;
            }
            else
            {
              read_lines = false;
            }
          }
          // this was the final period
          else if (buffer == ".")
          {
            // we assume that next line is MLF_DEF, so we try to read it
            if (getline(*mpCurStream, buffer))
            {
              read_lines = false;
              done       = true;

              // store the parsed string into the member atribute
              ParseHTKString(buffer, mCurLabelName);
              mLabelStore.Insert(mCurLabelName, mpCurStream);   // insert into the storage
              this->Clear();                                    // clear all flags
              return true;
            }
            else
            {
              read_lines = false;
            }
          }
        }
        else
        {
          read_lines = false;
        }
      } // while (read_lines)

      // if not done, we move to the next stream and read there
      if (!done)
      {
        miCurStream++;
        mpCurStream = *miCurStream;
        done = (miCurStream == mStreamList.end());
      }

      read_lines = true;
    }

    // if we got to this point, we didn't find any label definition line
    // so we assume we are at the very end of the file. We return false to indicate
    // we didn't jump to next block and we set the mIsHashed to true, because so far
    // we went through all MLF definitions an hashed each
    mIsHashed = true;
    return false;
  }


  //****************************************************************************
  bool
  LabelReader::
  FindInLabelStore(const string & rLabel, LabelStream & rLS)
  {
    // try to find the label in the Hash
    if (mLabelStore.Find(rLabel, rLS))
    {
      // we look in the list, but we limit the search.
      mpCurStream = rLS.mpStream;
      mpCurStream->seekg(rLS.mStreamPos);
      return true;
    } //if (this->mLabelStore.FindInHash(rLabel, label_stream))
    else
    {
      return false;
    }
  }


  //****************************************************************************
  bool
  LabelReader::
  FindInStreamList(const string & rLabel, LabelStream & rLS)
  {
    bool                       found = false;
    string                     line;
    string                     tmp_line;
    string                     mask_string;
    bool                       skip_next_reading = false;

    // read the MLFs and parse them
    while (!found && !IsHashed())
    {
      // if we cannot jump to next definition, it means that
      // we cannot move any further so searching ends...
      if (this->JumpToNextDefinition())
      {
        found = ProcessMask(rLabel, mCurLabelName, mask_string);
      }
      else
      {
        return false;
      }
    }
    return found;
  }


  //****************************************************************************
  bool
  LabelReader::
  OpenLabelFile(const string & rLabel)
  {
    // we try to open the file
    mpTempStream = new IStkStream;
    dynamic_cast<IStkStream *>(mpTempStream)->open(rLabel);

    if (!mpTempStream->good())  // on error return false and clear the object
    {
      delete mpTempStream;
      mpTempStream = NULL;
      return false;
    }
    else                        // on success return true and set current
                                // stream
    {
      mpCurStream = mpTempStream;
      mCurLabelName = rLabel;
      return true;
    }

  } // OpenLabelFile(const string & lFile)


  //****************************************************************************
  bool
  LabelReader::
  Open(const string & rLabel)
  {
    // helping strcuture when seeking in the LabelStore
    LabelStream       label_stream;
    bool              found = false;
    string            buffer;

    // it is possilble, that we look for a label that is currently
    // mpTempStream (only for IStkStream)
    if ((mpTempStream != NULL) && (typeid(mpTempStream) == typeid(IStkStream)))
    {
      // if we want to read from the same label, we only rewind
      if (dynamic_cast<IStkStream *>(mpCurStream)->name() == rLabel)
      {
        // rewind the stream to the begining and read again
        mpCurStream->clear();
        mpCurStream->seekg(0, ios::beg);
        return true;
      }
      // else we delete the object and continue searching
      else
      {
        delete mpTempStream;
        mpTempStream = NULL;
      }
    }  // if (mpTempStream != NULL)

    // try to find the label in the Hash
    if (FindInLabelStore(rLabel, label_stream))
    {
      //cerr << "Found in LabelStore ... " << endl;
      mpCurStream = label_stream.mpStream;
      mpCurStream->clear();
      mpCurStream->seekg(label_stream.mStreamPos);
      mCurLabelName = mLabelStore.MatchedPattern();
      found = true;
    } //if (this->mLabelStore.FindInHash(rLabel, label_stream))
    else
    {
      // neither in hash nor in list => if all MLF's hashed, find file
      // else keep searching in MLF's and then find file
      if (IsHashed())
      {
        found = this->OpenLabelFile(rLabel);
      } // if (this->mIsHashed)
      else
      {
        if (!(found = FindInStreamList(rLabel, label_stream)))
        {
          found = this->OpenLabelFile(rLabel);
        }
      } // else (this->mIsHashed)
    } // else (FindInLabelStore(rLabel, label_stream))

    if (found)
    {
      this->Clear(); // clear state flags
    }

    return found;
  }; // Open(const string & rLabel)



  //****************************************************************************
  bool
  LabelReader::
  GetLine(std::string & rLine)
  {
    // only if
    if (this->Good())
    {
      if (std::getline(*(this->mpCurStream), mLineBuffer))
      {
        rLine = mLineBuffer;

        if (rLine == ".")
        {
          this->mStateFlags |= LabelReader::FLAG_EOL;
          return false;
        }

        else if (this->mpCurStream->eof())
        {
          this->mStateFlags |= LabelReader::FLAG_EOL;
        }

        return true;
      }
      else
      {
        this->mStateFlags |= LabelReader::FLAG_EOL;
      }
    }

    return false;
  }


//****************************************************************************
  void
  LabelReader::
  HashAll()
  {
    // do nothing but go through all registered MLF's. Hashing is performed
    // automatically
    while (JumpToNextDefinition())
      ;
  }

}; // namespace STK

