/***************************************************************************
 *   copyright           : (C) 2004-2005 by Lukas Burget,UPGM,FIT,VUT,Brno *
 *   email               : burget@fit.vutbr.cz                             *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#define VERSION "0.2 "__TIME__" "__DATE__


// PROJECT INCLUDES
//
#include "labels.h"
#include "Net.h"
#include "common.h"


// SYSTEM INCLUDES
//
#ifndef WIN32
#include <unistd.h>
#else
#include "getopt.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <malloc.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <sstream>

#define INIT_NODE_HASH_SIZE 1000

// CODE
//
namespace STK
{
  
  //###########################################################################
  //###########################################################################
  // GENERAL FUNCTIONS
  //###########################################################################
  //###########################################################################
  
  //***************************************************************************
  //***************************************************************************
  Node *
  find_or_create_node(struct MyHSearchData *node_hash, char *node_id, Node **last)
  // Auxiliary function used by ReadHTKLattice_new. (Optionally initialize
  // uninitialized node_hash) Search for node record at key node_id. If found,
  // pointer to this record is returned, otherwise new node record is allocated
  // with type set to NT_UNDEF and entered to has at key node_id and pointer
  // to this new record is returned.
  {
    Node *node;
    ENTRY e, *ep;
  
    if (node_hash->mTabSize == 0 && !my_hcreate_r(INIT_NODE_HASH_SIZE, node_hash))
      Error("Insufficient memory");
    
    e.key = node_id;
    my_hsearch_r(e, FIND, &ep, node_hash);
  
    if (ep != NULL) 
      return (Node *) ep->data;  
  
    node = (Node *) calloc(1, sizeof(Node));
    
    if (node == NULL) 
      Error("Insufficient memory");
  
    node->mNLinks     = 0;
    node->mpLinks      = NULL;
    node->mNBackLinks = 0;
    node->mpBackLinks  = NULL;
    node->mType       = NT_WORD;
    node->mStart      = UNDEF_TIME;
    node->mStop       = UNDEF_TIME;
    node->mpPronun     = NULL;
    node->mpBackNext   = *last;
    node->mPhoneAccuracy = 1.0;
    
    *last  = node;
    e.key  = strdup(node_id);
    e.data = node;
  
    if (e.key == NULL || !my_hsearch_r(e, ENTER, &ep, node_hash))
      Error("Insufficient memory");
    
    return node;
  } // find_or_create_node(...)
  
  
  //###########################################################################
  //###########################################################################
  // OUTPUT FUNCTIONS
  //###########################################################################
  //###########################################################################
  
  //***************************************************************************
  //***************************************************************************
  int fprintBase62(FILE *fp, int v)
  {
    int i = 0;
    char str[16];
    char *tab="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (v == 0) {
      fputc(tab[0], fp);
      return 1;
    }
    while (v) {
      str[i++] = tab[v % 62];
      v /= 62;
    }
    v=i;
    while (i--) fputc(str[i], fp);
    return v;
  }
  
  //***************************************************************************
  //***************************************************************************
  void WriteSTKNetwork(
    FILE                   *lfp,
    Node                   *first,
    STKNetworkOutputFormat format,
    long                   sampPeriod,
    const char             *net_file,
    const char             *out_MNF)
  {
    int n, l=0;
    Node *node;
  
    for (n = 0, node = first; node != NULL; node = node->mpNext, n++)  
    {
      node->mAux = n;
      l += node->mNLinks;
    }
    
    fprintf(lfp,"N=%d L=%d\n", n, l);
    for (node = first; node != NULL; node = node->mpNext) {
      int j;
  
      if (format.mAllFieldNames) fputs("I=", lfp);
      if (format.mBase62Labels) fprintBase62(lfp, node->mAux);
      else                     fprintf(lfp,"%d", node->mAux);
  
      if (!format.mNoTimes && node->mStop != UNDEF_TIME) {
        fputs(" t=", lfp);
  
        if (node->mStart != UNDEF_TIME && format.mStartTimes) {
          fprintf(lfp,"%g,", node->mStart * 1.0e-7 * sampPeriod);
        }
        fprintf(  lfp,"%g",  node->mStop  * 1.0e-7 * sampPeriod);
      }
      if (!(node->mType & NT_WORD && node->mpPronun == NULL)
        || !format.mNoDefaults) {
        putc(' ', lfp);
        putc(node->mType & NT_WORD   ? 'W' :
            node->mType & NT_SUBNET ? 'S' :
                                      'M', lfp); // NT_MODEL, NT_PHONE
        putc('=', lfp);
        fprintHTKstr(lfp, node->mType & NT_MODEL   ? node->mpHmm->mpMacro->mpName   :
                          node->mType & NT_WORD    ? (!node->mpPronun ? "!NULL" :
                                                    node->mpPronun->mpWord->mpName) :
                                                    node->mpName); // NT_PHONE (NT_SUBNET)
      }
      if (!format.mNoPronunVars && node->mType & NT_WORD
      && node->mpPronun != NULL && node->mpPronun->mpWord->npronuns > 1
      && (node->mpPronun->variant_no > 1 || !format.mNoDefaults)) {
        fprintf(lfp," v=%d", node->mpPronun->variant_no);
      }
      if (node->mType & NT_TRUE || node->mType & NT_STICKY) {
        fputs(" f=", lfp);
        if (node->mType & NT_TRUE)   putc('T', lfp);
        if (node->mType & NT_STICKY) putc('K', lfp);
      }
      if (node->mType & NT_PHONE && node->mPhoneAccuracy != 1.0) {
        fprintf(lfp," p="FLOAT_FMT, node->mPhoneAccuracy);
      }
      if (!format.mArcDefsToEnd) {
        if (format.mAllFieldNames) fprintf(lfp," J=%d", node->mNLinks);
  
        for (j = 0; j < node->mNLinks; j ++) {
          putc(' ', lfp);
          if (format.mAllFieldNames) fputs("E=", lfp);
          if (format.mBase62Labels) fprintBase62(lfp, node->mpLinks[j].mpNode->mAux);
          else                     fprintf(lfp,"%d", node->mpLinks[j].mpNode->mAux);
          if (node->mpLinks[j].mLike != 0.0 && !format.mNoLMLikes) {
            fprintf(lfp," l="FLOAT_FMT, node->mpLinks[j].mLike);
          }
        }
      }
      fputs("\n", lfp);
      if (ferror(lfp)) {
        Error("Cannot write to output network file %s", out_MNF ? out_MNF : net_file);
      }
    }
  
    if (format.mArcDefsToEnd) {
      l = 0;
      for (node = first; node != NULL; node = node->mpNext) {
        int j;
  
        for (j = 0; j < node->mNLinks; j ++) {
          if (format.mAllFieldNames) {
            fprintf(lfp, format.mArcDefsWithJ ? "J=%d S=" : "I=", l++);
          }
          if (format.mBase62Labels) fprintBase62(lfp, node->mAux);
          else                     fprintf(lfp,"%d", node->mAux);
          putc(' ', lfp); // space = ' ';
          if (format.mAllFieldNames) fputs("E=", lfp);
          if (format.mBase62Labels) fprintBase62(lfp, node->mpLinks[j].mpNode->mAux);
          else                     fprintf(lfp,"%d", node->mpLinks[j].mpNode->mAux);
          if (node->mpLinks[j].mLike != 0.0 && !format.mNoLMLikes) {
            fprintf(lfp," l="FLOAT_FMT, node->mpLinks[j].mLike);
          }
          fputs("\n", lfp);
          if (ferror(lfp)) {
            Error("Cannot write to output network file %s", out_MNF ? out_MNF : net_file);
          }
        }
      }
    }
  }
  
  //***************************************************************************
  //***************************************************************************
  void WriteSTKNetworkInOldFormat(
    FILE        *lfp,
    Node       *first,
    LabelFormat labelFormat,
    long        sampPeriod,
    const char  *net_file,
    const char  *out_MNF)
  {
    int i;
    Node *node;
  
    for (i = 0, node = first; node != NULL; node = node->mpNext, i++)  {
      node->mAux = i;
    }
    fprintf(lfp,"NUMNODES: %d\n", i);
    for (i = 0, node = first; node != NULL; node = node->mpNext, i++) {
      int j,
      type = node->mType & NT_MODEL       ? 'M'  :
            node->mType & NT_PHONE       ? 'M'  :
            node->mType & NT_SUBNET      ? 'S'  :
            node->mType & NT_WORD        ?
              (node->mpPronun == NULL     ?
                (node->mType & NT_STICKY ? 'F'  :
                                          'N') :
                (node->mType & NT_STICKY ? 'K'  :
                                          'W')):
                                          '?';
      if (!(node->mType & NT_TRUE)) {
        type = tolower(type);
      }
      fprintf(lfp,"%d\t%c %s",
              i, type,
              node->mType & NT_MODEL   ? node->mpHmm->mpMacro->mpName :
              node->mType & NT_PHONE   ? node->mpName :
              node->mType & NT_SUBNET  ? node->mpName :
              node->mType & NT_WORD    ?
                (node->mpPronun == NULL ? "-" :
                                        node->mpPronun->mpWord->mpName):
                                        "?");
      if (node->mType & NT_WORD && node->mpPronun) {
        if (node->mpPronun->mpWord->mpName != node->mpPronun->outSymbol) {
          fprintf(lfp," [%s]", node->mpPronun->outSymbol);
        }
        if (node->mpPronun->prob != 0.0 || node->mpPronun->mpWord->npronuns > 1) {
          fprintf(lfp," {%d "FLOAT_FMT"}",
                  node->mpPronun->variant_no,
                  node->mpPronun->prob);
        }
      }
      if (node->mType & NT_PHONE && node->mPhoneAccuracy != 1.0) {
        fprintf(lfp," {"FLOAT_FMT"}", node->mPhoneAccuracy);
      }
      if (!(labelFormat.TIMES_OFF) &&
        node->mStart != UNDEF_TIME && node->mStop != UNDEF_TIME) {
        int ctm = labelFormat.CENTRE_TM;
        fprintf   (lfp," (");
        fprintf_ll(lfp, sampPeriod * (2 * node->mStart + ctm) / 2 - labelFormat.left_extent);
        fprintf   (lfp," ");
        fprintf_ll(lfp, sampPeriod * (2 * node->mStop - ctm)  / 2 + labelFormat.right_extent);
        fprintf   (lfp,")");
      }
      fprintf(lfp,"\t%d", node->mNLinks);
      for (j = 0; j < node->mNLinks; j ++) {
        fprintf(lfp," %d",node->mpLinks[j].mpNode->mAux);
        if (node->mpLinks[j].mLike != 0.0) {
          fprintf(lfp," {"FLOAT_FMT"}", node->mpLinks[j].mLike);
        }
      }
      fputs("\n", lfp);
      if (ferror(lfp)) {
        Error("Cannot write to output network file %s", out_MNF ? out_MNF : net_file);
      }
    }
  }

  
  
    
  //###########################################################################
  //###########################################################################
  // INPUT FUNCTIONS
  //###########################################################################
  //###########################################################################
  
  //***************************************************************************
  //***************************************************************************
  static int getInteger(char *str, char **endPtr,
                        const char *file_name, int line_no)
  {
    long l = strtoul(str, endPtr, 10);
  
    if (str == *endPtr || (**endPtr && !isspace(**endPtr))) {
      Error("Invalid integral value (%s:%d)", file_name, line_no);
    }
    while (isspace(**endPtr)) ++*endPtr;
  
    return l;
  }
  
  //***************************************************************************
  //***************************************************************************
  static float getFloat(char *str, char **endPtr,
                        const char *file_name, int line_no)
  {
    double d = strtod(str, endPtr);
  
    if (str == *endPtr || (**endPtr && !isspace(**endPtr))) {
      Error("Invalid float value (%s:%d)", file_name, line_no);
    }
    while (isspace(**endPtr)) ++*endPtr;
  
    return d;
  }
  
  //***************************************************************************
  //***************************************************************************
  static int getNodeNumber(int nnodes, char *str, char **endPtr,
                          const char *file_name,int line_no)
  {
    long node_id = getInteger(str, endPtr, file_name, line_no);
  
    if (node_id < 0 || node_id >= nnodes) {
      Error("Node number out of range (%s:%d)", file_name, line_no);
    }
    return node_id;
  }
    
  //***************************************************************************
  //***************************************************************************
  int RemoveCommentLines(FILE *fp)
  {
    int lines = 0, ch;
  
    for (;;) {
      while (isspace(ch=fgetc(fp))) {
        if (ch == '\n') {
          lines ++;
        }
      }
      if (ch != '#') {
        ungetc(ch, fp);
        return lines;
      }
      lines++;
      while ((ch=fgetc(fp)) != '\n' && ch != EOF);
    }
  }
  
  //***************************************************************************
  //***************************************************************************
  Node *ReadSTKNetworkInOldFormat(
    FILE*                     lfp,
    struct MyHSearchData*     word_hash,
    struct MyHSearchData*     phone_hash,
    LabelFormat               labelFormat,
    long                      sampPeriod,
    const char*               file_name,
    const char*               in_MLF)
  {
    size_t    numOfNodes;
    size_t    i;
    size_t    j;
    size_t    ch;
    
    int       nodeId = 0;
    int       linkId;
    int       numOfLinks;
    int       pronunVar;
    long long start;
    long long stop;
    char      nodeType;
    char      wordOrModelName[1024] = {'\0'};
    double    linkLike;
    double    pronunProb;
    Node*     node;
    Node**    nodes;
  
    std::stringstream ss; 
    
    RemoveCommentLines(lfp);
  
    if (fscanf(lfp," %1023[^0-9]", wordOrModelName) == 1) {
      for ( i =0; i < strlen(wordOrModelName); i++) {
        wordOrModelName[i] = toupper(wordOrModelName[i]);
      }
      while (--i>=0 && (wordOrModelName[i] == '='||isspace(wordOrModelName[i]))) {
        wordOrModelName[i] = '\0';
      }
    }

    int t1, t2;    
    if ((strcmp(wordOrModelName, "NUMNODES:") &&
        strcmp(wordOrModelName, "NUMBEROFNODES")) ||        //Obsolete NumerOfNodes
      fscanf(lfp," %d NumberOfArcs=%d", &t1, &t2)<1){       //Obsolete NumerOfArcs
      Error("Syntax error in file %s\nKeyword NumNodes: is missing", file_name);
    }
    numOfNodes = t1;
    
    if ((nodes = (Node **) calloc(numOfNodes, sizeof(Node *))) == NULL) {
      Error("Insufficient memory");
    }
    for (i=0; i < numOfNodes; i++) {
      if ((nodes[i] = (Node *) calloc(1, sizeof(Node))) == NULL) {
        Error("Insufficient memory");
      }
      nodes[i]->mType = NT_UNDEF;
    }
    for (i=0; i < numOfNodes; i++) {
      RemoveCommentLines(lfp);
  
      switch (fscanf(lfp, "%d %c %1023s", &nodeId, &nodeType, wordOrModelName)) 
      {
        case  3:
          break; //OK
  
        case -1:
          for (j=0; j < numOfNodes; j++) 
          {
            if (nodes[j] == NULL) {
              Error("Node %d is not defined in file %s", (int) j, file_name);
            }
          }
          
        default:
          Error("Invalid syntax in definition of node %d in file %s",
                nodeId, file_name);
      }
      
      if (static_cast<size_t>(nodeId) >= numOfNodes) {
        Error("Invalid definition of node %d in file %s.\n"
              "Node Id is bigger than number of nodes", nodeId, file_name);
      }
      node = nodes[nodeId];
  
      if (node->mType != NT_UNDEF)
        Error("Redefinition of node %d in file %s", nodeId, file_name);
      
      if (toupper(nodeType) != nodeType) {
        nodeType = toupper(nodeType);
        node->mType = 0;
      } else {
        node->mType = NT_TRUE;
      }
      if (nodeType != 'M' && nodeType != 'W' && nodeType != 'N' &&
        nodeType != 'S' && nodeType != 'K' && nodeType != 'F') {
        Error("Invalid definition of node %d in file %s.\n"
              "Supported values for node type are: M - model, W - word, N - null, S - subnet, K - keyword, F - filler",
              nodeId, file_name);
      }
      node->mStart = node->mStop = UNDEF_TIME;
      node->mPhoneAccuracy = 1.0;
  //    node->mAux = *totalNumOfNodes;
  //    ++*totalNumOfNodes;
  
      if (nodeType == 'S') {
        FILE *snfp;
        Node *subnetFirst;
  
  //      --*totalNumOfNodes; // Subnet node doesn't count
  
        if ((snfp = fopen(wordOrModelName, "rt")) == NULL) {
          Error("Cannot open network file: %s", wordOrModelName);
        }
        subnetFirst = ReadSTKNetworkInOldFormat(
                        snfp, word_hash, phone_hash, labelFormat,
                        sampPeriod, wordOrModelName, NULL);
        subnetFirst->mNBackLinks = node->mNBackLinks;
        *node = *subnetFirst;
        free(subnetFirst);
        fclose(snfp);
      } else if (nodeType == 'M') {
        ENTRY e, *ep;
        node->mType |= NT_PHONE;
        e.key  = wordOrModelName;
        e.data = NULL;
        my_hsearch_r(e, FIND, &ep, phone_hash);
  
        if (ep == NULL) {
          e.key  = strdup(wordOrModelName);
          e.data = e.key;
  
          if (e.key == NULL || !my_hsearch_r(e, ENTER, &ep, phone_hash)) {
            Error("Insufficient memory");
          }
          ep->data = e.data;
        }
        node->mpName = (char *) ep->data;
        fscanf(lfp, " {%lf}", &pronunProb);
        // We are not interested in PhoneAccuracy
      } else {
        node->mType |= NT_WORD;
  
        if (nodeType == 'K' || nodeType == 'F') {
        node->mType |= NT_STICKY;
        nodeType = nodeType == 'K' ? 'W' : 'N';
        }
        if (nodeType == 'W') {
          ENTRY e, *ep;
          Word *word = NULL;
  
          e.key  = wordOrModelName;
          my_hsearch_r(e, FIND, &ep, word_hash);
  
          if (ep == NULL) {
            Error("Invalid definition of node %d in file %s.\n"
                  "Unknown word '%s'", nodeId, file_name, e.key);
          }
          word = (Word *) ep->data;
  
          while (isspace(ch = fgetc(lfp)));
          if (ch != '[') {
            ungetc(ch, lfp);
          } else {
            if (fscanf(lfp, "%1023[^]]", wordOrModelName) != 1) {
  //           pronun.outSymbol = NULL;
            } else{
  //           pronun.outSymbol = wordOrModelName;
            }
            if (fgetc(lfp) != ']'){
              Error("Invalid definition of node %d in file %s.\n"
                    "Missing ']' after output symbol definition", nodeId, file_name);
            }
          }
          if (fscanf(lfp, "{%d %lf}", &pronunVar, &pronunProb) != 2) {
            pronunProb = 0.0;
            pronunVar = 0;
          } else {
            pronunVar--;
          }
          if (word->npronuns <= pronunVar) {
            Error("Invalid definition of node %d in file %s.\n"
                  "Word %s does not have pronunciation varian %d",
                  nodeId, file_name, word->mpName, pronunVar+1);
          }
          node->mpPronun = word ? word->pronuns[pronunVar] : NULL;
        } else {
          node->mpPronun = NULL;
        }
      }
      if (nodeType != 'S') {
        if (fscanf(lfp, " (%lld %lld)", &start, &stop)==2 && !(labelFormat.TIMES_OFF)) {
          long center_shift = labelFormat.CENTRE_TM ? sampPeriod / 2 : 0;
          node->mStart = (start - center_shift - labelFormat.left_extent)  / sampPeriod;
          node->mStop  = (stop  + center_shift + labelFormat.right_extent) / sampPeriod;
        }
      }
      if (fscanf(lfp, "%d ", &numOfLinks) != 1) {
        Error("Invalid definition of node %d in file %s.\n"
              "Number of links is expected", nodeId, file_name);
      }
      if (nodeType == 'S') { // Add links to the final node of the subnetwork
        while (node->mpNext != NULL) node = node->mpNext;
      }
      if (numOfLinks) {
        if ((node->mpLinks = (Link *) malloc(numOfLinks * sizeof(Link))) == NULL) {
          Error("Insufficient memory");
        }
      } else {
        if (nodeType == 'M') {
          Error("Invalid definition of node %d in file %s.\n"
                "Model node must have at least one link", nodeId, file_name);
        }
        node->mpLinks = NULL;
      }
      node->mNLinks = numOfLinks;
  
      for (j=0; j < static_cast<size_t>(numOfLinks); j++) {
        if (fscanf(lfp, "%d ", &linkId) != 1) {
          Error("Invalid definition of node %d in file %s.\n"
                "Link Id is expected in link list", nodeId, file_name);
        }
        if (static_cast<size_t>(linkId) >= numOfNodes) {
          Error("Invalid definition of node %d in file %s.\n"
                "Link Id is bigger than number of nodes", nodeId, file_name);
        }
        if (fscanf(lfp, "{%lf} ", &linkLike) != 1) {
          linkLike = 0.0;
        }
        node->mpLinks[j].mpNode = nodes[linkId];
        node->mpLinks[j].mLike = linkLike;
        nodes[linkId]->mNBackLinks++;
      }
    }
    for (i = 1; i < numOfNodes-1; i++) {
      if (nodes[i]->mNLinks == 0) {
        if (nodes[numOfNodes-1]->mNLinks == 0) {
          Error("Network contains multiple nodes with no successors (%s)",
                file_name);
        }
        node = nodes[numOfNodes-1];
        nodes[numOfNodes-1] = nodes[i];
        nodes[i] = node;
      }
      if (nodes[i]->mNBackLinks == 0) {
        if (nodes[0]->mNBackLinks == 0) {
          Error("Network contains multiple nodes with no predecessor (%s)",
                file_name);
        }
        node = nodes[0];
        nodes[0] = nodes[i];
        nodes[i] = node;
        i--;
        continue; // Check this node again. Could be the first one
      }
    }
    if (nodes[0]->mNBackLinks != 0 || nodes[numOfNodes-1]->mNLinks != 0) {
      Error("Network contain no start node or no final node (%s)", file_name);
    }
    if (!(nodes[0]           ->mType & NT_WORD) || nodes[0]           ->mpPronun != NULL ||
      !(nodes[numOfNodes-1]->mType & NT_WORD) || nodes[numOfNodes-1]->mpPronun != NULL) {
      Error("Start node and final node must be Null nodes (%s)", file_name);
    }
    for (i = 0; i < numOfNodes-1; i++) {
      nodes[i]->mpNext = nodes[i+1];
    }
  
    // create back links
    for (i = 0; i < numOfNodes; i++) {
      if (!nodes[i]->mpBackLinks) // Could be already alocated for subnetwork
        nodes[i]->mpBackLinks = (Link *) malloc(nodes[i]->mNBackLinks * sizeof(Link));
      if (nodes[i]->mpBackLinks == NULL) Error("Insufficient memory");
      nodes[i]->mNBackLinks = 0;
    }
    for (i = 0; i < numOfNodes; i++) {
      for (j=0; j < static_cast<size_t>(nodes[i]->mNLinks); j++) {
        Node *forwNode = nodes[i]->mpLinks[j].mpNode;
        forwNode->mpBackLinks[forwNode->mNBackLinks].mpNode = nodes[i];
        forwNode->mpBackLinks[forwNode->mNBackLinks].mLike = nodes[i]->mpLinks[j].mLike;
        forwNode->mNBackLinks++;
      }
    }
    node = nodes[0];
    free(nodes);
  
    if (in_MLF) {
      char *chptr;
      do {
          char line[1024];
          if (fgets(line, sizeof(line), lfp) == NULL) {
            Error("Missing '.' at the end of network '%s' in NMF '%s'",
                  file_name, in_MLF);
          }
          chptr = line + strspn(line, " \n\t");
      } while (!*chptr);
  
      if ((chptr[0] != '.' || (chptr[1] != '\0' && !isspace(chptr[1])))) {
        Error("Missing '.' at the end of network '%s' in NMF '%s'",
              file_name, in_MLF);
      }
    }
    return node;
  }
  
  Node *ReadSTKNetwork(
    FILE *lfp,
    struct MyHSearchData *word_hash,
    struct MyHSearchData *phone_hash,
    int notInDict,
    LabelFormat labelFormat,
    long sampPeriod,
    const char *file_name,
    const char *in_MLF)
  {
    Node *node, *enode = NULL,
        *first = NULL, *last = NULL,
        *fnode = NULL, *lnode;
    char *line;
    int line_no   =  0;
    char *chptr, *valptr, *phn_marks = NULL;
    Word *word     = NULL;
    int  i, pron_var  = 1;
    enum {LINE_START, AFTER_J, HEADER_DEF, ARC_DEF, NODE_DEF} state;
    MyHSearchData node_hash = {0};
    struct ReadlineData   rld       = {0};
  
    for (;;) 
    {
      do 
      {
        if ((chptr = line = readline(lfp, &rld)) == NULL) 
          break;
          
        if (chptr[0] == '.' && (chptr[1] == '\0' || isspace(chptr[1]))) {
          chptr = NULL;
          break;
        }
        line_no++;
        while (isspace(*chptr)) chptr++;
      } while (!*chptr || *chptr == '#');
  
      if (chptr == NULL) break; // End of file
  
      state = LINE_START;
      node = NULL;
      while (*chptr) {
        for (valptr=chptr; isalnum(*valptr); valptr++);
  
        if (*valptr == '=') {
          *valptr = '\0';
          valptr++;
        } else if (!*valptr || isspace(*valptr)) { // label definition (field without '=' )
          valptr = chptr;
          chptr = "";
        } else {
          Error("Invalid character '%c' (%s:%d, char %d)",
                *valptr, file_name, line_no, valptr-line+1);
        }
        if (state == LINE_START && !strcmp(chptr, "J")) {
          getInteger(valptr, &chptr, file_name, line_no);
          state = AFTER_J;
          continue;
        }
        if (state == AFTER_J) {
          if (*chptr && strcmp(chptr,"START") && strcmp(chptr,"S")) {
            Error("Term 'J=' must be followed by term 'S=' (%s:%d)", file_name, line_no);
          }
          state = LINE_START;
          chptr="";
        }
        if (state == LINE_START) {
          if (!*chptr || !strcmp(chptr, "I")) {
            if (getHTKstr(valptr, &chptr)) {
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
            node = find_or_create_node(&node_hash, valptr, &last);
            word     = NULL;
            pron_var = 1;
            state    = NODE_DEF;
            continue;
          }
          state = HEADER_DEF;
        }
        if (state == HEADER_DEF) { // label definition
          if (!strcmp(chptr, "S") || !strcmp(chptr, "SUBLAT")) {
            Error("%s not supported (%s:%d)", chptr, file_name, line_no);
          } else if (!strcmp(chptr, "N") || !strcmp(chptr, "NODES")) {
            int nnodes  = getInteger(valptr, &chptr, file_name, line_no);
  
            if (node_hash.mTabSize == 0 && !my_hcreate_r(nnodes, &node_hash)) {
              Error("Insufficient memory");
            }
          } else { // Skip unknown header term
            if (getHTKstr(valptr, &chptr)) {
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
          }
          continue;
        }
        if (state == NODE_DEF) {
          if ((!strcmp(chptr, "time") || !strcmp(chptr, "t")) && !(labelFormat.TIMES_OFF)) {
            char *colonptr=valptr;
            while (*colonptr && !isspace(*colonptr) && *colonptr != ',') colonptr++;
  
            if (*colonptr == ',') {
              if (colonptr != valptr) {
                *colonptr = ' ';
                node->mStart = 100 * (long long) (0.5 + 1e5 *
                              getFloat(valptr, &chptr, file_name, line_no));
              }
              valptr = colonptr+1;
            }
            node->mStop = 100 * (long long) (0.5 + 1e5 *
                        getFloat(valptr, &chptr, file_name, line_no));
          } else if (!strcmp(chptr, "var") || !strcmp(chptr, "v")) {
            pron_var = getInteger(valptr, &chptr, file_name, line_no);
            if (pron_var < 1) {
              Error("Invalid pronunciation variant (%s:%d)", file_name, line_no);
            }
          } else if (!strcmp(chptr, "p")) {
            node->mPhoneAccuracy = getFloat(valptr, &chptr, file_name, line_no);
          } else if (!strcmp(chptr, "flag") || !strcmp(chptr, "f")) {
            if (getHTKstr(valptr, &chptr)) {
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
            for (; *valptr; valptr++) {
              switch (toupper(*valptr)) {
                case 'K':
                case 'F':  node->mType |= NT_STICKY; break;
                case 'T':  node->mType |= NT_TRUE;   break;
                default:
                  Error("Invalid flag '%c' (%s:%d)", *valptr, file_name, line_no);
              }
            }
          } else if (!strcmp(chptr, "L")) {
            Error("Sub-lattice nodes are not yet supported (%s:%d)",
                  *valptr, file_name, line_no);
          } else if (!strcmp(chptr, "WORD") || !strcmp(chptr, "W")) {
            ENTRY e, *ep;
            if (getHTKstr(e.key = valptr, &chptr)) {
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
            if (!strcmp(e.key, "!NULL")) {
              word = NULL;
            } else {
              my_hsearch_r(e, FIND, &ep, word_hash);
  
              if (ep != NULL) {
                word = (Word *) ep->data;
              } else {
                if (notInDict & WORD_NOT_IN_DIC_ERROR) {
                  Error("Word '%s' not in dictionary (%s:%d)", e.key, file_name, line_no);
                } else if (notInDict & WORD_NOT_IN_DIC_WARN) {
                  Warning("Word '%s' not in dictionary (%s:%d)", e.key, file_name, line_no);
                }
  
                e.key  = strdup(e.key);
                word = (Word *) malloc(sizeof(Word));
                if (e.key == NULL || word  == NULL) {
                  Error("Insufficient memory");
                }
                word->mpName = e.key;
                word->npronuns = 0;
                word->npronunsInDict = 0;
                word->pronuns  = NULL;
                e.data = word;
  
                if (!my_hsearch_r(e, ENTER, &ep, word_hash)) {
                  Error("Insufficient memory");
                }
              }
            }
            node->mType &= ~(NT_MODEL | NT_PHONE);
            node->mType |= NT_WORD;
          } else if (!strcmp(chptr, "MODEL") || !strcmp(chptr, "M")) {
            ENTRY e, *ep;
  
            if (getHTKstr(e.key = valptr, &chptr)) {
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
            my_hsearch_r(e, FIND, &ep, phone_hash);
  
            if (ep == NULL) {
              e.key  = strdup(valptr);
              e.data = e.key;
  
              if (e.key == NULL || !my_hsearch_r(e, ENTER, &ep, phone_hash)) {
                Error("Insufficient memory");
              }
              ep->data = e.data;
            }
            node->mpName = (char *) ep->data;
            node->mType &= ~NT_WORD;
            node->mType |= NT_PHONE;
          } else if (*chptr=='\0' || !strcmp(chptr,"END") || !strcmp(chptr,"E")) {
            state = ARC_DEF;
          } else if (getHTKstr(valptr, &chptr)) { // Skip unknown term
            Error("%s (%s:%d)", chptr, file_name, line_no);
          }
          if (state == ARC_DEF || *chptr == '\0') {
            // Node definition is over. For NT_WORD, select right pronun according to
            // word and pron_var; and continue with parsing the arc definition below
            if (node->mType & NT_WORD && word != NULL) {
              if (word->npronuns < pron_var) {
                // Word does not have so many pronuns; add new empty pronuns...
                if (notInDict & PRON_NOT_IN_DIC_ERROR && word->npronuns != 0) {
                  Error("Word '%s' does not have pronunciation variant %d (%s:%d)",
                        word->mpName, pron_var, file_name, line_no);
                }
  
                word->pronuns = (Pronun **) realloc(word->pronuns,
                                                    pron_var * sizeof(Pronun *));
                if (word->pronuns == NULL) Error("Insufficient memory");
  
                for (i = word->npronuns; i < pron_var; i++) {
                  word->pronuns[i] = (Pronun *) malloc(sizeof(Pronun));
                  if (word->pronuns[i] == NULL) Error("Insufficient memory");
  
                  word->pronuns[i]->mpWord       = word;
                  word->pronuns[i]->outSymbol  = word->mpName;
                  word->pronuns[i]->nmodels    = 0;
                  word->pronuns[i]->model      = NULL;
                  word->pronuns[i]->variant_no = i+1;
                  word->pronuns[i]->prob       = 0.0;
                }
                word->npronuns = pron_var;
              }
              node->mpPronun = word->pronuns[pron_var-1];
            }
          }
        }
        if (state == ARC_DEF) {
          if (!*chptr || !strcmp(chptr, "END") || !strcmp(chptr, "E")) {
            if (getHTKstr(valptr, &chptr)) {
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
            enode = find_or_create_node(&node_hash, valptr, &last);
  
            int nl = ++node->mNLinks;
            node->mpLinks = (Link *) realloc(node->mpLinks, nl * sizeof(Link));
            if (node->mpLinks == NULL) Error("Insufficient memory");
            node->mpLinks[nl-1].mpNode = enode;
            node->mpLinks[nl-1].mLike = 0.0;
  
            nl = ++enode->mNBackLinks;
            enode->mpBackLinks = (Link *) realloc(enode->mpBackLinks, nl * sizeof(Link));
            if (enode->mpBackLinks == NULL) Error("Insufficient memory");
            enode->mpBackLinks[nl-1].mpNode = node;
            enode->mpBackLinks[nl-1].mLike = 0.0;
  
          } else if (!strcmp(chptr, "language") || !strcmp(chptr, "l")) {
            FLOAT mLike = getFloat(valptr, &chptr, file_name, line_no);
            //Set LM score to link pointing to enode. This link can possibly start
            //from a phone node already inserted (div=) between 'node' ans 'enode'
            Node *last = enode->mpBackLinks[enode->mNBackLinks-1].mpNode;
            last->mpLinks[last->mNLinks-1].mLike = mLike;
            enode->mpBackLinks[enode->mNBackLinks-1].mLike = mLike;
          } else if (!strcmp(chptr, "div") || !strcmp(chptr, "d")) {
            ENTRY e, *ep;
            char  name[1024];
            float time;
            int   n;
            Node  *last = node;
            FLOAT mLike  = node->mpLinks[node->mNLinks-1].mLike;
  
            if (node->mpLinks[node->mNLinks-1].mpNode != enode) {
              Error("Redefinition of  (%s:%d)", chptr, file_name, line_no);
            }
            if (getHTKstr(phn_marks=valptr, &chptr)) {
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
            time = -FLT_MAX;
            while (sscanf(phn_marks, ":%[^,:]%n,%f%n", name, &n, &time, &n) > 0) {
              Node *tnode;
              phn_marks+=n;
  
              if ((tnode            = (Node *) calloc(1, sizeof(Node))) == NULL ||
                (tnode->mpLinks     = (Link *) malloc(sizeof(Link))) == NULL ||
                (tnode->mpBackLinks = (Link *) malloc(sizeof(Link))) == NULL) {
                Error("Insufficient memory");
              }
  
              //Use special type to mark nodes inserted by d=..., they will need
              //special treatment. Later, they will become ordinary NT_PHONE nodes
              tnode->mType      = NT_PHONE | NT_MODEL;
              tnode->mNLinks    = tnode->mNBackLinks = 1;
              tnode->mpBackNext  = enode->mpBackNext;
              enode->mpBackNext  = tnode;
              tnode->mPhoneAccuracy = 1.0;
              //Store phone durations now. Will be replaced by absolute times below.
              tnode->mStart  = time != -FLT_MAX ? 100 * (long long) (0.5 + 1e5 * time) : UNDEF_TIME;
              tnode->mStop   = UNDEF_TIME;
              e.key  = name;
              e.data = NULL;
              my_hsearch_r(e, FIND, &ep, phone_hash);
  
              if (ep == NULL) {
                e.key  = strdup(name);
                e.data = e.key;
  
                if (e.key == NULL || !my_hsearch_r(e, ENTER, &ep, phone_hash)) {
                  Error("Insufficient memory");
                }
                ep->data = e.data;
              }
              tnode->mpName = (char *) ep->data;
              last->mpLinks[last->mNLinks-1].mpNode = tnode;
              last->mpLinks[last->mNLinks-1].mLike = 0.0;
              tnode->mpBackLinks[0].mpNode = last;
              tnode->mpBackLinks[0].mLike = 0.0;
              last = tnode;
            }
            if (strcmp(phn_marks,":")) {
              Error("Invalid specification of phone marks (d=) (%s:%d)",
                    file_name, line_no);
            }
            last->mpLinks[last->mNLinks-1].mpNode = enode;
            last->mpLinks[last->mNLinks-1].mLike = mLike;
            enode->mpBackLinks[enode->mNBackLinks-1].mpNode = last;
            enode->mpBackLinks[enode->mNBackLinks-1].mLike = mLike;
  
          } else if (getHTKstr(valptr, &chptr)) { // Skip unknown term
            Error("%s (%s:%d)", chptr, file_name, line_no);
          }
        }
      }
    }
    my_hdestroy_r(&node_hash, 1);
    lnode = last;
    first = last = NULL;
    if (lnode) lnode->mpNext = NULL;
    for (node = lnode; node != NULL; fnode = node, node = node->mpBackNext) {
      if (node->mpBackNext) node->mpBackNext->mpNext = node;
  
      if (node->mNLinks == 0) {
        if (last)
          Error("Network has multiple nodes with no successors (%s)", file_name);
        last = node;
      }
      if (node->mNBackLinks == 0) {
        if (first)
          Error("Network has multiple nodes with no predecessor (%s)", file_name);
        first = node;
      }
      //If only stop time is specified, set start time to lowest predecessor stop time
      if (node->mStart == UNDEF_TIME && node->mStop != UNDEF_TIME) {
        int i;
        for (i = 0; i < node->mNBackLinks; i++) {
          Node *backnode = node->mpBackLinks[i].mpNode;
          //When seraring predecessors, skip nodes inserted by d=...
          while (backnode->mType == (NT_PHONE | NT_MODEL)) {
            assert(backnode->mNBackLinks == 1);
            backnode = backnode->mpBackLinks[0].mpNode;
          }
          if (backnode->mStop != UNDEF_TIME) {
            node->mStart = node->mStart == UNDEF_TIME
                          ?          backnode->mStop
                          : LOWER_OF(backnode->mStop, node->mStart);
          }
        }
        if (node->mStart == UNDEF_TIME) node->mStart = 0;
      }
      //For model nodes defined by d=... (NT_PHONE | NT_MODEL), node->mStart contains
      //only phone durations. Absolute times must be computed derived starting from
      //the end time of the node to which arc with d=... definition points.
      if (node->mType == (NT_PHONE | NT_MODEL)) {
        assert(node->mNLinks == 1);
        node->mStop = node->mpLinks[0].mpNode->mType == (NT_PHONE | NT_MODEL)
                    && node->mStart != UNDEF_TIME
                    ? node->mpLinks[0].mpNode->mStart : node->mpLinks[0].mpNode->mStop;
        node->mStart = node->mStart != UNDEF_TIME && node->mStop != UNDEF_TIME
                      ? node->mStop - node->mStart : node->mpLinks[0].mpNode->mStart;
      }
    }
    if (!first || !last) {
      Error("Network contain no start node or no final node (%s)", file_name);
    }
    if (first != fnode) {
      if (first->mpNext)     first->mpNext->mpBackNext = first->mpBackNext;
      if (first->mpBackNext) first->mpBackNext->mpNext = first->mpNext;
      if (first == lnode)  lnode = first->mpBackNext;
  
      first->mpBackNext = NULL;
      fnode->mpBackNext = first;
      first->mpNext = fnode;
    }
    if (last != lnode) {
      if (last->mpNext)     last->mpNext->mpBackNext = last->mpBackNext;
      if (last->mpBackNext) last->mpBackNext->mpNext = last->mpNext;
      last->mpNext = NULL;
      lnode->mpNext = last;
      last->mpBackNext = lnode;
    }
    for (node = first; node != NULL; node = node->mpNext) {
      if (node->mType == (NT_PHONE | NT_MODEL)) {
        node->mType = NT_PHONE;
      }
      if (node->mStart != UNDEF_TIME) {
        node->mStart = (node->mStart - labelFormat.left_extent) / sampPeriod;
      }
      if (node->mStop  != UNDEF_TIME) {
        node->mStop  = (node->mStop + labelFormat.right_extent) / sampPeriod;
      }
    }
    if (first->mpPronun != NULL) {
      node = (Node *) calloc(1, sizeof(Node));
      if (node == NULL) Error("Insufficient memory");
      node->mpNext       = first;
      node->mpBackNext   = NULL;
      first->mpBackNext  = node;
      node->mType       = NT_WORD;
      node->mpPronun     = NULL;
      node->mStart      = UNDEF_TIME;
      node->mStop       = UNDEF_TIME;
      node->mNBackLinks = 0;
      node->mpBackLinks  = NULL;
      node->mNLinks     = 1;
      node->mpLinks      = (Link*) malloc(sizeof(Link));
      if (node->mpLinks == NULL) Error("Insufficient memory");
      node->mpLinks[0].mLike = 0.0;
      node->mpLinks[0].mpNode = first;
      first->mNBackLinks = 1;
      first->mpBackLinks  = (Link*) malloc(sizeof(Link));
      if (first->mpBackLinks == NULL) Error("Insufficient memory");
      first->mpBackLinks[0].mLike = 0.0;
      first->mpBackLinks[0].mpNode = node;
      first = node;
    }
    if (last->mpPronun != NULL) {
      node = (Node *) calloc(1, sizeof(Node));
      if (node == NULL) Error("Insufficient memory");
      last->mpNext      = node;
      node->mpNext      = NULL;
      node->mpBackNext  = last;
      node->mType      = NT_WORD;
      node->mpPronun    = NULL;
      node->mStart     = UNDEF_TIME;
      node->mStop      = UNDEF_TIME;
      node->mNLinks    = 0;
      node->mpLinks     = NULL;
      last->mNLinks = 1;
      last->mpLinks  = (Link*) malloc(sizeof(Link));
      if (last->mpLinks == NULL) Error("Insufficient memory");
      last->mpLinks[0].mLike = 0.0;
      last->mpLinks[0].mpNode = node;
      node->mNBackLinks = 1;
      node->mpBackLinks  = (Link*) malloc(sizeof(Link));
      if (node->mpBackLinks == NULL) Error("Insufficient memory");
      node->mpBackLinks[0].mLike = 0.0;
      node->mpBackLinks[0].mpNode = last;
    }
    return first;
  }
  
  //***************************************************************************
  //***************************************************************************
  Node *ReadHTKLattice(
    FILE *            lfp,
    MyHSearchData *   word_hash,
    MyHSearchData *   phone_hash,
    LabelFormat       labelFormat,
    long              sampPeriod,
    const char *      file_name)
  {
    Node **nodes = NULL, *node,
        *first = NULL, *last = NULL,
        *fnode, *lnode = NULL;
    char line[1024];
    int nnodes  = -1;
    int nnread  =  0;
    int node_id =  0;
    int line_no =  0;
    char *chptr, *valptr, *phn_marks = NULL;
    int       arc_start = 0;
    int       arc_end   = 0;
    FLOAT     arc_like  = 0.0;
    long long node_time = UNDEF_TIME;
    int       node_var  = 0;
    Word     *node_word = NULL;
    enum {LINE_START, ARC_DEF, NODE_DEF} state;
  
    for (;;) {
      do {
        if (fgets(line, sizeof(line), lfp) == NULL) {
          chptr = NULL;
          break;
        }
        line_no++;
        if (strlen(line) == sizeof(line) - 1) {
          Error("Line is too long (%s:%d)", file_name, line_no);
        }
        chptr = line + strspn(line, " \n\t");
      } while (!*chptr || *chptr == '#');
  
      if (chptr == NULL) break; // End of file
  
      state = LINE_START;
      while (*chptr) {
        if ((valptr = strchr(chptr, '=')) == NULL) {
          Error("'=' expected (%s:%d)", file_name, line_no);
        }
        valptr++;
  
        if (state == LINE_START) {
  
          if (chptr[0] == 'S') {
            Error("%s not supported (%s:%d)", chptr, file_name, line_no);
          }
          if (chptr[0] == 'N') {
            if (nnodes > -1) {
              Error("Redefinition of number of nodes (%s:%d)",file_name,line_no);
            }
            nnodes = strtoull(valptr, NULL, 10);
            nodes  = (Node **) calloc(nnodes, sizeof(Node *));
            if (nodes == NULL) Error("Insufficient memory");
            break;
          }
          if (chptr[0] == 'I') {
            state = NODE_DEF;
            node_id = getNodeNumber(nnodes, valptr, &chptr, file_name, line_no);
  
            if (nodes[node_id] != NULL) {
              Error("Redefinition of node %d (%s:%d)",
              node_id, file_name, line_no);
            }
            nodes[node_id] = (Node *) calloc(1, sizeof(Node));
            if (nodes[node_id] == NULL) Error("Insufficient memory");
  
            if (last == NULL) {
              first = nodes[node_id];
            } else {
              last->mpNext = nodes[node_id];
            }
            last = nodes[node_id];
            nodes[node_id]->mpNext = NULL;
  
            node_word = NULL;
            node_var  = 0;
            node_time = UNDEF_TIME;
            nnread++;
          } else if (chptr[0] == 'J') {
            if (nnodes < nnread) {
              for (node_id = 0; nodes[node_id] != NULL; node_id++);
              Error("Definition of node %d is missing (%s:%d)",
                    node_id, file_name, line_no);
            }
            state = ARC_DEF;
            getInteger(valptr, &chptr, file_name, line_no);
            arc_like  = 0.0;
            arc_start = -1;
            arc_end   = -1;
            phn_marks = NULL;
          } else {
            break; // Ignore line with unknown initial term
          }
        } else if (state == NODE_DEF) {
          if (chptr[0] == 't' && !(labelFormat.TIMES_OFF)) {
            node_time = 100 * (long long)
                        (0.5 + 1e5*getFloat(valptr, &chptr, file_name, line_no));
          } else if (chptr[0] == 'v') {
            node_var = getInteger(valptr, &chptr, file_name, line_no) - 1;
          } else if (chptr[0] == 'W') {
            ENTRY e, *ep;
            if (getHTKstr(e.key = valptr, &chptr)) {
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
  
            if (strcmp(e.key, "!NULL")) {
              my_hsearch_r(e, FIND, &ep, word_hash);
  
              if (ep == NULL) {
                Error("Unknown word '%s' (%s:%d)", e.key, file_name, line_no);
              }
              node_word = (Word *) ep->data;
            }
          } else {
            if (getHTKstr(valptr, &chptr)) { // Skip unknown term
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
          }
        } else {
          assert(state == ARC_DEF);
          if (chptr[0] == 'S') {
            arc_start = getNodeNumber(nnodes, valptr, &chptr, file_name,line_no);
          } else if (chptr[0] == 'E') {
            arc_end   = getNodeNumber(nnodes, valptr, &chptr, file_name,line_no);
          } else if (chptr[0] == 'l') {
            arc_like  = getFloat(valptr, &chptr, file_name, line_no);
          } else if (chptr[0] == 'd') {
            if (getHTKstr(phn_marks=valptr, &chptr)) { // Skip unknown term
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
          } else {
            if (getHTKstr(valptr, &chptr)) { // Skip unknown term
              Error("%s (%s:%d)", chptr, file_name, line_no);
            }
          }
        }
      }
      if (state == NODE_DEF) {
        nodes[node_id]->mType       = NT_WORD;
        nodes[node_id]->mNLinks     = 0;
        nodes[node_id]->mNBackLinks = 0;
        nodes[node_id]->mpLinks      = NULL;
        nodes[node_id]->mpBackLinks  = NULL;
        if (node_word && node_word->npronuns <= node_var) {
          Error("Word %s does not have pronunciation varian %d (%s:%d)",
                node_word->mpName, node_var+1, file_name, line_no);
        }
        nodes[node_id]->mpPronun     = node_word ? node_word->pronuns[node_var]
                                              : NULL;
        nodes[node_id]->mStart     = UNDEF_TIME;
        nodes[node_id]->mStop      = node_time + labelFormat.right_extent;
      } else if (state == ARC_DEF) {
        if (arc_start == -1 || arc_end == -1) {
          Error("Start node or end node not defined (%s:%d)", file_name, line_no);
        }
  
        int linkId = nodes[arc_start]->mNLinks++;
        nodes[arc_start]->mpLinks =
          (Link *) realloc(nodes[arc_start]->mpLinks, (linkId+1) * sizeof(Link));
  
        if (nodes[arc_start]->mpLinks == NULL) Error("Insufficient memory");
  
        last = nodes[arc_start];
  
        if (phn_marks) {
          ENTRY e, *ep;
          char  name[1024];
          float time;
          int   n;
  
          while (sscanf(phn_marks, ":%[^,],%f%n", name, &time, &n) > 1) {
            phn_marks+=n;
  
            if ((node            = (Node *) calloc(1, sizeof(Node))) == NULL ||
              (node->mpLinks     = (Link *) malloc(sizeof(Link))) == NULL ||
              (node->mpBackLinks = (Link *) malloc(sizeof(Link))) == NULL) {
              Error("Insufficient memory");
            }
            node->mType = NT_PHONE;
            node->mNLinks = node->mNBackLinks = 1;
            node->mpNext   = last->mpNext;
            last->mpNext = node;
            node->mPhoneAccuracy = 1.0;
  
            if (!(labelFormat.TIMES_OFF)) {
              node->mStart = last->mStop - labelFormat.left_extent - labelFormat.right_extent;
              node->mStop  = last->mStop + 100 * (long long) (0.5 + 1e5 * time)
                                      + labelFormat.right_extent;
            } else {
              node->mStart  = UNDEF_TIME;
              node->mStop   = UNDEF_TIME;
            }
            e.key  = name;
            e.data = NULL;
            my_hsearch_r(e, FIND, &ep, phone_hash);
  
            if (ep == NULL) {
              e.key  = strdup(name);
              e.data = e.key;
  
              if (e.key == NULL || !my_hsearch_r(e, ENTER, &ep, phone_hash)) {
                Error("Insufficient memory");
              }
              ep->data = e.data;
            }
            node->mpName = (char *) ep->data;
            last->mpLinks[last->mNLinks-1].mpNode = node;
            last->mpLinks[last->mNLinks-1].mLike = 0.0;
            node->mpBackLinks[0].mpNode = last;
            node->mpBackLinks[0].mLike = 0.0;
            last = node;
          }
          if (strcmp(phn_marks,":")) {
            Error("Invalid specification of phone marks (d=) (%s:%d)",
                  file_name, line_no);
          }
        }
        linkId = nodes[arc_end]->mNBackLinks++;
        nodes[arc_end]->mpBackLinks =
          (Link *) realloc(nodes[arc_end]->mpBackLinks, (linkId+1) * sizeof(Link));
  
        if (nodes[arc_end]->mpBackLinks == NULL) Error("Insufficient memory");
  
        last->mpLinks[last->mNLinks-1].mpNode = nodes[arc_end];
        last->mpLinks[last->mNLinks-1].mLike = arc_like;
        nodes[arc_end]->mpBackLinks[linkId].mpNode = last;
        nodes[arc_end]->mpBackLinks[linkId].mLike = arc_like;
  
        if (nodes[arc_start]->mStop != UNDEF_TIME) {
          if (nodes[arc_end]->mStart == UNDEF_TIME) {
            nodes[arc_end]->mStart = nodes[arc_start]->mStop - labelFormat.left_extent
                                                          - labelFormat.right_extent;
          } else {
            nodes[arc_end]->mStart = LOWER_OF(nodes[arc_end]->mStart,
                                            nodes[arc_start]->mStop - labelFormat.left_extent
                                                                    - labelFormat.right_extent);
          }
        }
      }
    }
    free(nodes);
    fnode = first;
    first = last = NULL;
    if (fnode) fnode->mpBackNext = NULL;
    for (node = fnode; node != NULL; lnode = node, node = node->mpNext) {
      if (node->mpNext) node->mpNext->mpBackNext = node;
  
      if (node->mNLinks == 0) {
        if (last)
          Error("Network has multiple nodes with no successors (%s)", file_name);
        last = node;
      }
      if (node->mNBackLinks == 0) {
        if (first)
          Error("Network has multiple nodes with no predecessor (%s)", file_name);
        first = node;
      }
    }
    if (!first || !last) {
      Error("Network contain no start node or no final node (%s)", file_name);
    }
    if (first != fnode) {
      if (first->mpNext) first->mpNext->mpBackNext = first->mpBackNext;
      first->mpBackNext->mpNext = first->mpNext;
      first->mpBackNext = NULL;
      fnode->mpBackNext = first;
      first->mpNext = fnode;
    }
    if (last != lnode) {
      if (last->mpBackNext) last->mpBackNext->mpNext = last->mpNext;
      last->mpNext->mpBackNext = last->mpBackNext;
      last->mpNext = NULL;
      lnode->mpNext = last;
      last->mpBackNext = lnode;
    }
    if (first->mpPronun != NULL) {
      node = (Node *) calloc(1, sizeof(Node));
      if (node == NULL) Error("Insufficient memory");
      node->mpNext       = first;
      node->mpBackNext   = NULL;
      first->mpBackNext  = node;
      node->mType       = NT_WORD;
      node->mpPronun     = NULL;
      node->mStart      = UNDEF_TIME;
      node->mStop       = UNDEF_TIME;
      node->mNBackLinks = 0;
      node->mpBackLinks  = NULL;
      node->mNLinks     = 1;
      node->mpLinks      = (Link*) malloc(sizeof(Link));
      if (node->mpLinks == NULL) Error("Insufficient memory");
      node->mpLinks[0].mLike = 0.0;
      node->mpLinks[0].mpNode = first;
  
      first->mNBackLinks = 1;
      first->mpBackLinks  = (Link*) malloc(sizeof(Link));
      if (first->mpBackLinks == NULL) Error("Insufficient memory");
      first->mpBackLinks[0].mLike = 0.0;
      first->mpBackLinks[0].mpNode = node;
      first = node;
    }
    if (last->mpPronun != NULL) {
      node = (Node *) calloc(1, sizeof(Node));
      if (node == NULL) Error("Insufficient memory");
      last->mpNext      = node;
      node->mpNext      = NULL;
      node->mpBackNext  = last;
  
      node->mType      = NT_WORD;
      node->mpPronun    = NULL;
      node->mStart     = UNDEF_TIME;
      node->mStop      = UNDEF_TIME;
      node->mNLinks    = 0;
      node->mpLinks     = NULL;
  
      last->mNLinks = 1;
      last->mpLinks  = (Link*) malloc(sizeof(Link));
      if (last->mpLinks == NULL) Error("Insufficient memory");
      last->mpLinks[0].mLike = 0.0;
      last->mpLinks[0].mpNode = node;
  
      node->mNBackLinks = 1;
      node->mpBackLinks  = (Link*) malloc(sizeof(Link));
      if (node->mpBackLinks == NULL) Error("Insufficient memory");
      node->mpBackLinks[0].mLike = 0.0;
      node->mpBackLinks[0].mpNode = last;
    }
    return first;
  }
  
  
  
} // namespace STK
