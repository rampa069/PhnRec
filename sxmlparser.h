#ifndef _SXMLPARSER
#define _SXMLPARSER

#include <stdio.h>
#include <map>
#include <list>
#include <string>

// kinds of text outputs 
#ifndef  TO_CONSTANTS
#define  TO_CONSTANTS
const unsigned int TO_ERROR        = 1;
const unsigned int TO_WARNING      = 2;
const unsigned int TO_LOG          = 3;
#endif

// format of xml output
const unsigned int SXMLT_NONE      = 0;
const unsigned int SXMLT_ADDHEADER = 1;

class SXMLDocument;

// XML properties
class SXMLProperties
{
   public:      
      SXMLProperties();
      ~SXMLProperties(); 
      void Clear();   
      std::string &GetValue(std::string name);
      void Set(std::string name, std::string arg = "");      
      void FirstName();
      std::string GetName();
      bool Exists(std::string name);
      bool Remove(std::string name);
      void GetAsText(std::string &rText);
      int GetNumber();
      std::string & operator [] (std::string name);
      SXMLProperties & operator = (SXMLProperties &rProp);
      void Add(SXMLProperties *pProperties);
   protected:   
      std::map<std::string, std::string> mList;
      std::map<std::string, std::string>::iterator mListPointer;
      std::string mDefaultString;
};

// XML node
class SXMLNode
{
   public:
      friend class SXMLDocument;       

      SXMLNode();
      ~SXMLNode();    

      std::string &GetName();
      std::string &GetText();
      SXMLProperties *GetProperties();
      void *GetData();
      void SetData(void *pdata);
      void SetName(std::string name);
      void SetText(std::string text);
      void SetStartFileLine(int startFileLine);
      void SetStopFileLine(int stopFileLine);
      int GetStartFileLine();
      int GetStopFileLine();
      bool GetQuoted();
      void SetQuoted(bool quoted);
      void SetDocument(SXMLDocument *pDocument);
      SXMLDocument *GetDocument();
      void SetParent(SXMLNode *pNode);
      SXMLNode *GetParent();
      void AddChild(SXMLNode *pNode);
      void RemoveChild(SXMLNode *pNode);
      bool RemoveChild(std::string name, char *pProperties);
      void RemoveAllChilds();      
      void FirstChild();      
      SXMLNode *GetChild();
      SXMLNode *GetChild(std::string name, char *pProperties = "");
      void GetAsText(std::string &rText, int spaceLevel = 0);
      void GetChildsAsText(std::string &rText, int spaceLevel = 0);
      std::string GetPath(SXMLProperties *pIncludeProperties = 0);

      bool operator == (std::string &rText) const;
      bool operator == (SXMLNode *pNode) const;      

      void CopyNodeNoChilds(SXMLNode &rNode);
      void CopyChilds(SXMLNode &rNode);
      SXMLNode & operator = (SXMLNode &rNode);
      
   protected:
      std::string mName;
      std::string mText;
      bool mQuoted;
      void *mpData;
      int mStartFileLine;
      int mStopFileLine;
      SXMLDocument *mpDocument;
      SXMLNode *mpParentNode;
      SXMLProperties mProperties; 
      std::list<SXMLNode *> mChilds;
      std::list<SXMLNode *>::iterator mChildPointer;
};

// XML document
class SXMLDocument
{
   public:
      friend class SXMLNode;   // access to the ParseProperties function 

      SXMLDocument();
      ~SXMLDocument();
                
      // set error handler
      typedef void (*TextOutFuncPtrType)(unsigned int textType, char *pText, void *pArg);
      void SetTextOutFunc(TextOutFuncPtrType pTextOutFunc, void *pArg = 0);

      // new document
      void New();

      // load XML document
      bool Load(char *pFile);
      
      // save XML document
      bool Save(char *pFile, unsigned int flags = SXMLT_ADDHEADER);

      // parsing functions
      bool ParseText(char *pText);
      bool ParseText(std::string text);

      // create XML text
      void GetAsText(std::string &rText, unsigned int flags = SXMLT_ADDHEADER);

      // node manipulation functions
      SXMLNode &GetRootNode();      
      SXMLNode &GetNode(std::string key, bool insertNodes = true);
      void RemoveNode(SXMLNode *pNode);
      void RemoveNode(std::string key);
      bool Exists(std::string key);

      // access operator
      std::string &operator [] (std::string key);

      SXMLDocument & operator = (SXMLDocument &rXMLDoc);

   protected:
      // logging of errors
      TextOutFuncPtrType mpTextOutFunc;
      void *mpTextOutArg;

      SXMLNode *mpRootNode;
      SXMLNode mDefaultNode;

      // document header      
      std::string mVersion;
      std::string mEncoding;
      std::string mFile;
      int mFileLine;
      
      // help string
      std::string mDefaultString;
      
      // error logging functions
      void Error(char *pErrorText);
      void Warning(char *pWarningText);
      void Log(char *pLogText);
      static void DefaultErrorHandler(unsigned int textType, char *pText, void *pArg);
      
      // parsing functions
      char *SkipSpaces(char *pText, int *pFileLine);          
      char *GetTag(char *pText);
      char *SkipTag(char *pText);
      char *GetText(char *pText, char *pSeparators, int maxQuotes, char **ppNextText, int *pFileLine, bool includeQuotes = false);      
      SXMLNode *ParseNode(char *pText, char **ppNextText);
      bool ParseProperties(char *pText, SXMLProperties *pProperties, char *pNodeName, char *pFile, int *pFileLine, char **ppNextText);
      bool ParseBody(char *pText, SXMLNode *pNode, char **ppNextText);
      void CutOffSpaces(char *pText);

      // help functions
      int FileLength(FILE *pF);      
};

#endif
