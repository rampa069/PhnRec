#include <assert.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
  #include <io.h>
#else
  #include <unistd.h>
  #include <sys/types.h>
#endif

#include "sxmlparser.h"

SXMLProperties::SXMLProperties()
{
   mDefaultString = "";
}

SXMLProperties::~SXMLProperties()
{
}

void SXMLProperties::Clear()
{
   mList.clear();
}   

std::string &SXMLProperties::GetValue(std::string name)
{
   return mList[name];
}

std::string &SXMLProperties::operator [] (std::string name)
{
   return GetValue(name);
}

void SXMLProperties::Set(std::string name, std::string arg)
{
   mList[name] = arg;
}

void SXMLProperties::FirstName()
{
   mListPointer = mList.begin();
}

std::string SXMLProperties::GetName()
{
   if(mListPointer == mList.end())
   {
      return "";	
   }
   std::string ret_str = mListPointer->first;
   mListPointer++;
   return ret_str;
}

bool SXMLProperties::Exists(std::string name)
{
   std::map<std::string, std::string>::iterator it = mList.find(name);
   if(it == mList.end())
   {
      return false;
   }
   return true;
}

bool SXMLProperties::Remove(std::string name)
{
   std::map<std::string, std::string>::iterator it = mList.find(name);
   if(it == mList.end())
   {
      return false;
   }
   mList.erase(it);
   return true;
}

void SXMLProperties::GetAsText(std::string &rText)
{
   rText = "";
   std::map<std::string, std::string>::iterator it;

   for(it = mList.begin(); it != mList.end(); it++)
   {
      if(rText.size() != 0)
      {
         rText += " ";
      }
      rText += it->first;
      rText += "=\"";
      rText += it->second;
      rText += "\"";
   }
}
      
int SXMLProperties::GetNumber()
{
   return mList.size();
}
 
SXMLProperties & SXMLProperties::operator = (SXMLProperties &rProp)
{
   mList.clear();

   std::map<std::string, std::string>::iterator it;
   for(it = rProp.mList.begin(); it != rProp.mList.end(); it++)
   {
      mList[it->first] = it->second;
   }	

   FirstName();
   
   return *this;
}    

void SXMLProperties::Add(SXMLProperties *pProperties)
{
   pProperties->FirstName();

   std::string name;
   while((name = pProperties->GetName()) != "")
   {
      std::string value = pProperties->GetValue(name);
      mList[name] = value;
   }

   FirstName();
}
      
//--------------------------------------------------------------------------------

SXMLNode::SXMLNode() :
   mName(""),
   mText(""),
   mQuoted(false),
   mpData(0),
   mStartFileLine(0),
   mStopFileLine(0),
   mpDocument(0),
   mpParentNode(0)
{
   mProperties.Clear();
   mChilds.clear();	
}

SXMLNode::~SXMLNode()
{
   RemoveAllChilds();
}

bool SXMLNode::operator == (std::string &rText) const
{
   if(mText == rText)
      return true;
   return false;
}

bool SXMLNode::operator == (SXMLNode *pNode) const
{
   if(this == pNode)
      return true;
   return false;
}

void SXMLNode::SetStartFileLine(int startFileLine)
{
   mStartFileLine = startFileLine;
}

void SXMLNode::SetStopFileLine(int stopFileLine)
{
   mStopFileLine = stopFileLine;
}

int SXMLNode::GetStartFileLine()
{
   return mStartFileLine;
}

int SXMLNode::GetStopFileLine()
{
   return mStopFileLine;
}

SXMLProperties *SXMLNode::GetProperties()
{
   return &mProperties;
}

std::string &SXMLNode::GetName()
{
   return mName;
}

std::string &SXMLNode::GetText()
{
   return mText;
}

void *SXMLNode::GetData()
{
   return mpData;
}

void SXMLNode::SetData(void *pdata)
{
   mpData = pdata;
}
  
void SXMLNode::SetName(std::string name)
{
   mName = name;
}

void SXMLNode::SetText(std::string text)
{
   mText = text;
}

bool SXMLNode::GetQuoted()
{
   return mQuoted;
}

void SXMLNode::SetQuoted(bool quoted)
{
   mQuoted = quoted;
}

void SXMLNode::SetParent(SXMLNode *pNode)
{
   mpParentNode = pNode;
}

SXMLNode *SXMLNode::GetParent()
{
   return mpParentNode;
}

void SXMLNode::SetDocument(SXMLDocument *pDocument)
{
   mpDocument = pDocument;
}

SXMLDocument *SXMLNode::GetDocument()
{
   return mpDocument;
}

void SXMLNode::AddChild(SXMLNode *pNode)
{
   mChilds.push_back(pNode);
}

void SXMLNode::RemoveChild(SXMLNode *pNode)
{
   mChilds.remove(pNode);
   delete pNode;
}

bool SXMLNode::RemoveChild(std::string name, char *pProperties)
{
   SXMLProperties properties;
   
   if(!mpDocument->ParseProperties(pProperties, &properties, "RemoveChild()", 0, 0, 0))
   {
      return 0;
   }   
   int count = properties.GetNumber();
	
   std::list<SXMLNode *>::iterator it;
   it = mChilds.begin();

   while(it != mChilds.end())
   {
      if(name == (*it)->GetName())
      {
         bool remove = true;
         if(count != 0)     // check if values of all properties are equal to the query  
         {
            std::string prop_name;
            properties.FirstName();
            do
            {
         	   prop_name = properties.GetName();
               if(prop_name != "" && !(*it)->GetProperties()->Exists(prop_name))
               {
                  remove = false;
         	   }
            } while(prop_name != "");
         }
         if(remove)
         {
            (*it)->RemoveAllChilds();
            SXMLNode *pnode = *it;
            it = mChilds.erase(it);
            delete pnode;
         }
      }
      else
      {
         it++;
      }	
   }
	
   return true;
}

SXMLNode *SXMLNode::GetChild(std::string name, char *pProperties)
{
   SXMLProperties properties;
   
   if(!mpDocument->ParseProperties(pProperties, &properties, "GetChild()", 0, 0, 0))
   {
      return 0;
   }   
   int count = properties.GetNumber();
	
   std::list<SXMLNode *>::iterator it;
   
   for(it = mChilds.begin(); it != mChilds.end(); it++)
   {
      if(name == (*it)->GetName())
      {
         bool ok = true;
         
         if(count != 0)     // check that values of all properties are equal to the query  
         {
            std::string prop_name;
            properties.FirstName();
            do
            {
         	   prop_name = properties.GetName();
         	   if(prop_name != "" && (*it)->GetProperties()->GetValue(prop_name) != properties.GetValue(prop_name))
         	   {
         		   ok = false;
         	   }
            } while(prop_name != "");
         }
         if(ok)
         {
            return (*it);
         }
      }
   }
	
   return 0;
}

void SXMLNode::RemoveAllChilds()
{
   std::list<SXMLNode *>::iterator it;
   it = mChilds.begin();

   while(it != mChilds.end())
   {
      (*it)->RemoveAllChilds();
      SXMLNode *pnode = *it;
      it = mChilds.erase(it);
      delete pnode;	
   }	
}

void SXMLNode::FirstChild()
{
   mChildPointer = mChilds.begin();
}

SXMLNode *SXMLNode::GetChild()
{
   if(mChildPointer == mChilds.end())
   {
      return 0;	
   }
   SXMLNode *pret_node = *mChildPointer;
   mChildPointer++;	
   return pret_node;
}

void SXMLNode::GetChildsAsText(std::string &rText, int spaceLevel)
{
   std::list<SXMLNode *>::iterator it;
      
   for(it = mChilds.begin(); it != mChilds.end(); it++)
   {
      std::string child_nodes_text;
      (*it)->GetAsText(child_nodes_text, spaceLevel);
      rText += child_nodes_text;
   } 	
}

void SXMLNode::GetAsText(std::string &rText, int spaceLevel)
{
   // indent
   std::string indent = "";
   int i;
   for(i = 0; i < spaceLevel; i++)	
      indent += " ";
	
   // node start + name
   rText = indent + "<" + mName;

   // add properties   
   std::string prop_text;
   GetProperties()->GetAsText(prop_text);  
   if(prop_text.size() != 0)
   {
      rText += " ";
      rText += prop_text;
   }
   
   // add node end or node body separator
   int nchilds = mChilds.size();
   if(mText.size() != 0 || nchilds != 0)
   {
      rText += ">\n";
   }
   else
   {
   	  rText += "/>\n";
   	  return;
   }
   
   // add node text
   if(mText.size() != 0)
   {
      rText += indent + "   ";
      if(mQuoted)
      {
         rText += "\"";
      }
      rText += mText;
      if(mQuoted)
      {
         rText += "\"";
      }
      rText += "\n";
   }

   // add child nodes
   if(nchilds != 0)
   {
      std::string child_nodes_text;
      GetChildsAsText(child_nodes_text, spaceLevel + 3);
      rText += child_nodes_text;
   }
      
   // add node termination
   rText += indent;
   rText += "</";
   rText += mName;
   rText += ">\n";
}

void SXMLNode::CopyNodeNoChilds(SXMLNode &rNode)
{
   RemoveAllChilds();

   mName = rNode.mName;
   mText = rNode.mText;
   mQuoted = rNode.mQuoted;
   mStartFileLine = rNode.mStartFileLine;
   mStopFileLine = rNode.mStopFileLine;
   mProperties = rNode.mProperties; 
   // mpDocument;      // keep original document
   // mpParentNode;    // keep original parent node
}


void SXMLNode::CopyChilds(SXMLNode &rNode)
{
   std::list<SXMLNode *>::iterator it;
      
   for(it = rNode.mChilds.begin(); it != rNode.mChilds.end(); it++)
   {
      SXMLNode *pnode = new SXMLNode;
      *pnode = *(*it);
      AddChild(pnode);
   } 	
}

SXMLNode & SXMLNode::operator = (SXMLNode &rNode)
{
   CopyNodeNoChilds(rNode);
   CopyChilds(rNode);   
   return *this;   
}

std::string SXMLNode::GetPath(SXMLProperties *pIncludeProperties)
{
   std::string ret = "";
   SXMLNode *pnode = this;

   while(pnode != 0 && pnode->GetParent() != 0)  // node exists and is not the root node
   {
      if(ret.size() != 0)
      {
         ret = "." + ret;
      }

      std::string props_str = "";

      if(pIncludeProperties != 0 && pIncludeProperties->GetNumber() != 0)
      {
         std::string prop_name = "";
         SXMLProperties *pprops = pnode->GetProperties();
        
         pIncludeProperties->FirstName();
         while((prop_name = pIncludeProperties->GetName()) != "")
         {
            if(pprops->Exists(prop_name))
            {
               if(props_str.size() != 0)
               {
                  props_str += " ";
               }
               props_str += prop_name + "=\"" + pprops->GetValue(prop_name) + "\"";
            }
         }	
      }

      if(props_str.size() != 0)
      {
         ret = pnode->GetName() + "(" + props_str + ")" + ret;
      }
      else
      {   
         ret = pnode->GetName() + ret;
      }

      pnode = pnode->GetParent();
   }
   return ret;
}

//--------------------------------------------------------------------------------

SXMLDocument::SXMLDocument() :
   mpTextOutFunc(&DefaultErrorHandler),
   mpRootNode(0)
{
   mVersion = "1.0";
   mEncoding = "ISO-8859-1";
   mFile = "";
   mDefaultString = "";
   mpTextOutArg = this;

   mpRootNode = new SXMLNode;
   mpRootNode->SetParent(0);
   mpRootNode->SetDocument(this);

   mDefaultNode.SetName("error");
   mDefaultNode.SetDocument(this);
}

SXMLDocument::~SXMLDocument()
{
   delete mpRootNode;	
}

void SXMLDocument::SetTextOutFunc(TextOutFuncPtrType pTextOutFunc, void *pArg)
{
   mpTextOutFunc = pTextOutFunc;
   mpTextOutArg = pArg;
}

void SXMLDocument::Error(char *pErrorText)
{
   assert(mpTextOutFunc != 0);
   (*mpTextOutFunc)(TO_ERROR, pErrorText, mpTextOutArg);
}

void SXMLDocument::Warning(char *pWarningText)
{
   assert(mpTextOutFunc != 0);
   (*mpTextOutFunc)(TO_WARNING, pWarningText, mpTextOutArg);
}

void SXMLDocument::Log(char *pLogText)
{
   assert(mpTextOutFunc != 0);
   (*mpTextOutFunc)(TO_LOG, pLogText, mpTextOutArg);
}

void SXMLDocument::DefaultErrorHandler(unsigned int textType, char *pText, void *pArg)
{
   switch(textType)
   {
      case TO_ERROR:
           fprintf(stderr, "ERROR: %s\n", pText);
           exit(1);
      case TO_WARNING:
           fprintf(stderr, "WARNING: %s\n", pText);
           break;           
      case TO_LOG:    
           fprintf(stdout, "%s\n", pText);
           break;           
   }
}

void SXMLDocument::New()
{
	delete mpRootNode;
	mpRootNode = new SXMLNode;
	mpRootNode->SetParent(0);
	mpRootNode->SetDocument(this);
	mFile = "";
	mFileLine = 1;
}
               
bool SXMLDocument::Load(char *pFile)
{     
   FILE *pf;
   pf = fopen(pFile, "rb");
   if(!pf)
   {
   	  char pmsg[1024];
      sprintf(pmsg, "Can not open XML file '%s'", pFile);
      Error(pmsg);
      return false;
   }
     
   int len = FileLength(pf);
   char *pdata = new char[len];
    
   if(fread(pdata, sizeof(char), len, pf) != (size_t)len)
   {
      char pmsg[1024];
      sprintf(pmsg, "Unable to read XML file: '%s'", pFile);
      Error(pmsg);
      return false;   
   } 
   
   mFile = pFile;
   bool ok = ParseText(pdata);
    
   delete [] pdata;  
   fclose(pf);
     
   return ok;
}

bool SXMLDocument::Save(char *pFile, unsigned int flags)
{
   std::string text;
   GetAsText(text, flags);
   int len = text.size();

   FILE *pf = fopen(pFile, "wb");
   if(pf == 0)
   {
      char pmsg[1024];
      sprintf(pmsg, "Can not create XML file '%s'", pFile);
      Error(pmsg);
      return false;
   }

   if(fwrite(text.c_str(), sizeof(char), len, pf) != (size_t)len)
   {
      char pmsg[1024];
      sprintf(pmsg, "Unable to write XML file: '%s'", pFile);
      Error(pmsg);
      return false;   
   } 
   
   fclose(pf);
   return true;
}

char *SXMLDocument::SkipSpaces(char *pText, int *pFileLine)
{
   char *pt = pText;
   while(*pt == ' ' || *pt == '\n' || *pt == '\r' || *pt == '\t')
   {
   	  if(*pt == '\n' && pFileLine != 0)
   	  {
   	     (*pFileLine)++;
   	  }
      pt++;
   }
   return pt;
}

char *SXMLDocument::SkipTag(char *pText)
{
   char *pt = pText;
   while(*pt != '\0' && *pt != '>')
   {
   	  if(*pt == '\n')
   	  {
   	     mFileLine++;
   	  }
      pt++;	
   }
   if(*pt != '\0')
   {
      pt++;
   }
   return pt;
}

char *SXMLDocument::GetTag(char *pText)
{
   char *pt = pText;
   int len = 0;
   while(*pt != '\0' && *pt != '>')
   {
      pt++;
      len++;
   }
   if(*pt != '\0')
      len++;
   
   char *pOut = new char [len + 1];
   strncpy(pOut, pText, len);
   pOut[len] = '\0';
   return pOut;
}      

char *SXMLDocument::GetText(char *pText, char *pSeparators, int maxQuotes, char **ppNextText, int *pFileLine, bool includeQuotes)
{
   bool quoted = false;
   int quoted_start = 1;
   int n_quotes = 0;
   int n_new_lines = 0;

   // Count the length
   char *pt = pText;
   int len = 0;
   while(*pt != '\0' && (strchr(pSeparators, *pt) == 0 || quoted))
   {
      if(*pt == '"')
      {
         quoted = !quoted;
         quoted_start = mFileLine;
         n_quotes++;
         if(n_quotes > maxQuotes && maxQuotes != -1)
         {
            char pmsg[1024];
            if(mFile == "")
            {
               sprintf(pmsg, "XML parsing error - improper using of quotas in text starting at line %d", quoted_start);
            } 
            else
            {
               sprintf(pmsg, "XML parsing error - improper using of quotas in text starting at line %d, file '%s'", quoted_start, mFile.c_str());
            }
            Error(pmsg);
            return 0;            	         	         
         }
      }	
      else
      {
       	 len++;
      }
      if(*pt == '\n')
      {
         n_new_lines++;
      }
      pt++;
   }
      
   if(quoted)
   {
      char pmsg[1024];
      if(mFile == "")
      {
         sprintf(pmsg, "XML parsing error - untermined quoted text started at line %d", quoted_start);
      }
      else
      {
         sprintf(pmsg, "XML parsing error - untermined quoted text started at line %d, file '%s'", quoted_start, mFile.c_str());
      }
      Error(pmsg);
      return 0;            	         	         
   }
      
   // Allocate memory and extract the text
   char *pout = new char [len + 1];	

   pt = pText;
   quoted = false;
   char *po = pout;
   while(*pt != '\0' && (strchr(pSeparators, *pt) == 0 || quoted))
   {
      if(*pt == '"')
      {
         quoted = !quoted;
      }
      if(*pt != '"' || includeQuotes)
      {   	
         *po = *pt;
         po++;
      }
      pt++;
   }
   *po = '\0';
   
   if(ppNextText != 0)
   {
      *ppNextText = pt;
   }
   if(pFileLine != 0)
   {
      (*pFileLine) += n_new_lines;
   }   

   return pout;
}

bool SXMLDocument::ParseProperties(char *pText, SXMLProperties *pProperties, char *pNodeName, char *pFile, int *pFileLine, char **ppNextText)
{
   char *pt = pText;
   pt = SkipSpaces(pt, pFileLine);
   
   while(*pt != '\0' && strncmp(pt, ">", strlen(">")) != 0 && strncmp(pt, "/>", strlen("/>")) != 0 && strncmp(pt, "?>", strlen("?>")) != 0)
   {
      char *pproperty = GetText(pt, " =?>/\n\t\r", 0, &pt, pFileLine);
      if(pproperty == 0)
      {
         return false;            	         	         
      }
      
      pt = SkipSpaces(pt, pFileLine);

      if(*pt != '=')
      {
         char pmsg[1024];
         if(pFile == 0 || strlen(pFile) == 0)
         {
         	sprintf(pmsg, "XML parsing error - missing argument for property '%s' in node '%s', line %d", pproperty, pNodeName, *pFileLine);
         }
         else
         {
         	sprintf(pmsg, "XML parsing error - missing argument for property '%s' in node '%s', file '%s', line %d", pproperty, pNodeName, pFile, *pFileLine);
         }
         Error(pmsg);
         return false;            	         	         
      }

      pt++;      // skip '='
      pt = SkipSpaces(pt, pFileLine);

      char *pargument = GetText(pt, " =?>/\n\t\r", 2, &pt, pFileLine);
      if(pargument == 0)
      {
         return false;            	         	         
      }

      pProperties->Set(pproperty, pargument);
      delete [] pproperty;
      delete [] pargument;

      pt = SkipSpaces(pt, pFileLine);
   }

   if(ppNextText != 0)
   {
      *ppNextText = pt;
   }

   return true;
}

void SXMLDocument::CutOffSpaces(char *pText)
{
   char *pt = pText + strlen(pText) - 1;
   while(pt >= pText)
   {
      if(strchr(" \t\n\r", *pt) != 0)
      {
         *pt = '\0';
      }
      else
      {
         break;
      }
      pt--;
   }   
}

bool SXMLDocument::ParseBody(char *pText, SXMLNode *pNode, char **ppNextText)
{
   char *pt = pText;
   pt = SkipSpaces(pt, &mFileLine);
     	     	
   std::string end_sequence = "</" + pNode->GetName() + ">";   
   while(*pt != '\0' && strncmp(pt, end_sequence.c_str(), strlen(end_sequence.c_str())) != 0)
   {
      if(strncmp(pt, "<", strlen("<")) == 0)    // a child node or error
      {
         if(strncmp(pt, "</", strlen("</")) == 0)
         {
            int  line = mFileLine;  
            char *ptag = GetTag(pt);
            char pmsg[1024];
            if(mFile == "")
            {
               sprintf(pmsg, "XML parsing error - missing termination for node '%s' ('</%s>'), starting at line %d (reached '%s' at line %d)", pNode->GetName().c_str(), pNode->GetName().c_str(), pNode->GetStartFileLine(), ptag, line);
            }
            else
            {
               sprintf(pmsg, "XML parsing error - missing termination for node '%s' ('</%s>'), file %s, starting at line %d (reached '%s' at line %d)", pNode->GetName().c_str(), pNode->GetName().c_str(), mFile.c_str(), pNode->GetStartFileLine(), ptag, line);
            }
            delete [] ptag;
            Error(pmsg);
            return false;            	         
         }
      
         SXMLNode *pchild_node = ParseNode(pt, &pt);
         if(pchild_node == 0)
         {
            delete pNode;
            return false;
         }
         pchild_node->SetParent(pNode);
         pNode->AddChild(pchild_node);
      }
      else                                     // plain text
      {
      	 if(*pt == '"')
      	 {
      	    pNode->SetQuoted(true);
      	 }
      	 char *pplain_text = GetText(pt, "<>", -1, &pt, &mFileLine);
      	 CutOffSpaces(pplain_text);
      	 
      	 if(pplain_text == 0)
      	 {
            return false;            	         	         
         }      	 
      	 pNode->SetText(pNode->GetText() + pplain_text);
      	 delete [] pplain_text;
      }
      pt = SkipSpaces(pt, &mFileLine);
   }
      
   if(ppNextText != 0)
   {
      *ppNextText = pt;
   }
   
   return true;
}

SXMLNode *SXMLDocument::ParseNode(char *pText, char **ppNextText)
{
   // ininial sign '<' or '<?'
   char *pt = pText;
   int start_line = mFileLine;
   if(strncmp(pt, "<", strlen("<")) != 0 && strncmp(pt, "<?", strlen("<?")) != 0)
   {
      char pmsg[1024];
      if(mFile == "")
      {
         sprintf(pmsg, "XML parsing error - missing node start ('<' or '<?'), line %d", start_line);
      }
      else
      {
         sprintf(pmsg, "XML parsing error - missing node start ('<' or '<?'), file '%s', line %d", mFile.c_str(), start_line);
      }
      Error(pmsg);
      return 0;            	
   }

   // skip node start sequence   
   pt++;             // skip '<'
   if(*pt == '?')
   {
      pt++;          // skip '?'
   }
   
   // get node name  <name ...
   char *pname = GetText(pt, " =?>/", 0, &pt, &mFileLine);
   if(pname == 0)
   {
      return 0;
   }
   if(strlen(pname) == 0)
   {
      char pmsg[1024];
      if(mFile == "")
      {
         sprintf(pmsg, "XML parsing error - missing node name (sentence of < >, < /> or <? ?>), line %d", start_line);
      }
      else
      {
         sprintf(pmsg, "XML parsing error - missing node name (sentence of < >, < /> or <? ?>), file '%s', line %d", mFile.c_str(), start_line);
      }
      Error(pmsg);
      return 0;            	         	
   }
   SXMLNode *pnode = new SXMLNode;
   pnode->SetName(pname);
   pnode->SetDocument(this);
   pnode->SetStartFileLine(start_line);
   delete [] pname;
   pt = SkipSpaces(pt, &mFileLine);

   // parse properties
   if(!ParseProperties(pt, pnode->GetProperties(), (char *)pnode->GetName().c_str(), (char *)mFile.c_str(), &mFileLine, &pt))
   {
      delete pnode;
      return 0;
   }

   if(*pt == '\0')
   {
      char pmsg[1024];
      if(mFile == "")
      {
         sprintf(pmsg, "XML parsing error - missing node termination >, /> or ?>, line %d", mFileLine);
      }
      else
      {
         sprintf(pmsg, "XML parsing error - missing node termination >, /> or ?>, file '%s', line %d", mFile.c_str(), mFileLine);
      }
      Error(pmsg);
      return 0;            	
   }

   // check if there is a node end or body separation sequence 
   if(strncmp(pt, ">", strlen(">")) != 0 && strncmp(pt, "/>", strlen("/>")) != 0 && strncmp(pt, "?>", strlen("?>")) != 0)
   {
      char pmsg[1024];
      if(mFile == "")
      {
         sprintf(pmsg, "XML parsing error - missing node end or body separation ('>', '?>' or '/>') for node '%s', node starts at line %d", pnode->GetName().c_str(), pnode->GetStartFileLine());
      }
      else
      {
         sprintf(pmsg, "XML parsing error - missing node end or body separation ('>', '?>' or '/>') for node '%s', file %s, node starts at line %d", pnode->GetName().c_str(), mFile.c_str(), pnode->GetStartFileLine());
      }
      Error(pmsg);
      return 0;            	
   }

   // check if the node has a body
   bool has_body = false;
   if(strncmp(pt, ">", strlen(">")) == 0)
   {
   		has_body = true;
   }

   // skip node end or body separation sequence 
   pt++;             // skip '/' or '?' or '>'
   if(*pt == '>')
   {
      pt++;          // skip '>'
   }   

   if(has_body)
   {
   	  pt = SkipSpaces(pt, &mFileLine);
   	
      if(!ParseBody(pt, pnode, &pt))
      {
      	 delete pnode;
         return 0;
      }
      
      std::string end_sequence = "</" + pnode->GetName() + ">";      
      if(strncmp(pt, end_sequence.c_str(), strlen(end_sequence.c_str())) != 0)
      {
      	 delete pnode;
         char pmsg[1024];
         if(mFile == "")
         {
            sprintf(pmsg, "XML parsing error - missing node end for node '%s' ('%s'), node starts at line %d", pnode->GetName().c_str(), end_sequence.c_str(), pnode->GetStartFileLine());
         }
         else
         {
            sprintf(pmsg, "XML parsing error - missing node end for node '%s' ('%s'), file '%s', node starts at line %d", pnode->GetName().c_str(), end_sequence.c_str(), mFile.c_str(), pnode->GetStartFileLine());
         }
         Error(pmsg);
         return 0;            	
      }      
      
      pt = SkipTag(pt);
   }
   
   pnode->SetStopFileLine(mFileLine);
   pt = SkipSpaces(pt, &mFileLine);
      
   // pointer to the next node in XML file      
   if(ppNextText != 0)
   {
      *ppNextText = pt;
   }

   return pnode;
}


bool SXMLDocument::ParseText(char *pText)
{
   // reset line counting
   mFileLine = 1;
	
   // parse text
   char *pt = pText;
   pt = SkipSpaces(pt, &mFileLine); 
   
   if(strncmp(pt, "<?xml", strlen("<?xml")) == 0)
   {
      SXMLNode *pNode = ParseNode(pt, &pt);
      if(pNode == 0)
      {
      		delete mpRootNode;
      		mpRootNode = 0;
      		return false;
      }
      mVersion = pNode->GetProperties()->GetValue("version");
      mEncoding = pNode->GetProperties()->GetValue("encoding");
      delete pNode;
   }

   pt = SkipSpaces(pt, &mFileLine);

   if(!ParseBody(pt, mpRootNode, &pt))
   {
      delete mpRootNode;
      mpRootNode = 0;
      return false;      	
   }
   
   return true;
}

bool SXMLDocument::ParseText(std::string text)
{
   return ParseText((char *)text.c_str());
}

int SXMLDocument::FileLength(FILE *pF)
{
   int f = fileno(pF);
   long oldpos = ftell(pF);
   long size = lseek(f, 0, SEEK_END);
   lseek(f, oldpos, SEEK_SET);
   return size;
}

void SXMLDocument::GetAsText(std::string &rText, unsigned int flags)
{
   rText = "";
   if(flags & SXMLT_ADDHEADER)
   {
      rText += "<?xml version=\"" + mVersion + "\" encoding=\"" + mEncoding + "\"?>\n";
   }
   if(mpRootNode != 0)
   {
      std::string child_nodes;
      mpRootNode->GetChildsAsText(child_nodes, 0);
      rText += child_nodes;
   }
}

SXMLNode &SXMLDocument::GetNode(std::string key, bool insertNodes)
{
   SXMLNode *pparent_node = mpRootNode;
   SXMLNode *pnode = 0;
   int line;
      
   char *pt = (char *)key.c_str();
   while(*pt != '\0')
   {
   	  line = 1;
      char *pnode_str = GetText(pt, ".", -1, &pt, &line, true);
      
      if(pnode_str == 0 || strlen(pnode_str) == 0)
      {
      	 if(pnode_str != 0)
      	 {       	
      	    delete [] pnode_str;
      	 }      	
         char pmsg[1024];
         sprintf(pmsg, "Operator [] - parsing error - string: '%s'", key.c_str());
         Error(pmsg);      	 
         return mDefaultNode;
      }
      
      char *pnode_name = pnode_str;
      char *pnode_prop = new char [1];
      strcpy(pnode_prop, "");

      if(strchr(pnode_str, '(') != 0 || strchr(pnode_str, ')') != 0)   // node property filter is used
      {
      	 char *pbuff = pnode_str;      	
      	 line = 1;
         pnode_name = GetText(pbuff, "()", -1, &pbuff, &line, false);
         if(pnode_name == 0 || strlen(pnode_name) == 0)
         {
            delete [] pnode_str;
            delete [] pnode_prop;
            if(pnode_name != 0)
            {
               delete [] pnode_name;
            }      	 
            char pmsg[1024];
            sprintf(pmsg, "Operator [] - parsing error - missing node name front of '(', string: '%s'", key.c_str());
            Error(pmsg);      	 
            return mDefaultNode;
         }
      
         if(*pbuff != '(')
         {
            delete [] pnode_str;
            delete [] pnode_name;
            delete [] pnode_prop;      	 
            char pmsg[1024];
            sprintf(pmsg, "Operator [] - parsing error - missing '(', string: '%s'", key.c_str());
            Error(pmsg);      	 
            return mDefaultNode;         	
         }
         
         pbuff++;
         delete [] pnode_prop;
         pnode_prop = GetText(pbuff, ")", -1, &pbuff, &line, false);
         if(pnode_prop == 0)
         {
            delete [] pnode_str;
            delete [] pnode_name;
            if(pnode_prop != 0)
            {
               delete [] pnode_prop;
            }      	 
            char pmsg[1024];
            sprintf(pmsg, "Operator [] - parsing error - missing ')', string: '%s'", key.c_str());
            Error(pmsg);      	 
            return mDefaultNode;         	
         }
         if(*pbuff != ')')
         {
            delete [] pnode_str;
            delete [] pnode_name;
            delete [] pnode_prop;
            char pmsg[1024];
            sprintf(pmsg, "Operator [] - parsing error - missing ')', string: '%s'", key.c_str());
            Error(pmsg);      	 
            return mDefaultNode;         	         	
         }         
         delete [] pnode_str;
      }
      
      pnode = pparent_node->GetChild(pnode_name, pnode_prop);

      if(pnode == 0)
      {
      	 if(!insertNodes)
      	 {
            return mDefaultNode;         	         	
      	 }
         pnode = new SXMLNode;
         pnode->SetName(pnode_name);
         pnode->SetParent(pparent_node);
         pnode->SetDocument(this);
         pnode->SetStartFileLine(1);
         pparent_node->AddChild(pnode);
      }

      pparent_node = pnode;
                             
      delete [] pnode_name;
      delete [] pnode_prop;

      if(*pt == '.')
      {
         pt++;
      }      
   }
   return *pparent_node;	
}

void SXMLDocument::RemoveNode(SXMLNode *pNode)
{
	pNode->GetParent()->RemoveChild(pNode);
}

void SXMLDocument::RemoveNode(std::string key)
{
	RemoveNode(&GetNode(key));
}

std::string &SXMLDocument::operator [] (std::string key)
{
   char *pt = (char *)key.c_str();
   int line = 1;
   char *pnode_str = GetText(pt, "#", -1, &pt, &line, true);
	
   SXMLNode *pnode = &GetNode(pnode_str);
   if(pnode == &mDefaultNode)
   {
      mDefaultString = "";
      return mDefaultString;
   }

   if(*pt == '#')
   {
      pt++;
      if(strlen(pt) == 0)
      {
         char pmsg[1024];
         sprintf(pmsg, "Operator [] - missing property name (#xxx) for node: '%s'", pnode_str);
         Error(pmsg);      	 
         mDefaultString = "";
         return mDefaultString;         	         	         
      }
      return (*pnode->GetProperties())[pt];
   }      
      
   return pnode->GetText();
}

bool SXMLDocument::Exists(std::string key)
{
   char *pt = (char *)key.c_str();
   int line = 1;
   char *pnode_str = GetText(pt, "#", -1, &pt, &line, true);
	
   SXMLNode *pnode = &GetNode(pnode_str, false);
   if(pnode == &mDefaultNode || pnode == &mDefaultNode)
   {
      return false;
   }

   if(*pt == '#')
   {
      pt++;
      if(strlen(pt) == 0)
      {
         char pmsg[1024];
         sprintf(pmsg, "Operator [] - missing property name (#xxx) for node: '%s'", pnode_str);
         Error(pmsg);      	 
         return false;         	         	         
      }
      return pnode->GetProperties()->Exists(pt);
   }      
      
   return true;
}

SXMLNode &SXMLDocument::GetRootNode()
{
   return *mpRootNode;
}


SXMLDocument & SXMLDocument::operator = (SXMLDocument &rXMLDoc)
{
   *mpRootNode = rXMLDoc.GetRootNode();
   return *this;
}
