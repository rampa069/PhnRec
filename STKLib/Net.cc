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
#include "Models.h"
#include "labels.h"
#include "dict.h"
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



#define SIGNIFICANT_PROB_DIFFERENCE (0.01)


// CODE
//

namespace STK
{
  //***************************************************************************
  //***************************************************************************
  void 
  FreeNetwork(Node * pNode) 
  {
    Node *  tnode;
    
    while (pNode) 
    {
      tnode = pNode->mpNext;
      free(pNode->mpLinks);
      free(pNode->mpBackLinks);
      free(pNode);
      pNode = tnode;
    }
  }
  
  //***************************************************************************
  //***************************************************************************
  Node *
  MakeNetworkFromLabels(Label * pLabels, NodeType nodeType)
  {
    Label * lp;
    Node  * first;
    Node  * last = NULL;
    Node  * node;
  
    if ((first             = (Node *) calloc(1, sizeof(Node))) == NULL ||
        (last              = (Node *) calloc(1, sizeof(Node))) == NULL ||
        (first->mpLinks    = (Link *) malloc(sizeof(Link))) == NULL    ||
        (last->mpBackLinks = (Link *) malloc(sizeof(Link))) == NULL) 
    {
      Error("Insufficient memory");
    }

    first->mpName       = last->mpName      = NULL;     
    first->mType        = last->mType       = NT_WORD;
    first->mpPronun     = last->mpPronun    = NULL;
    first->mNLinks      = last->mNBackLinks = 1;
    first->mNBackLinks  = last->mNLinks     = 0;
    first->mpBackLinks  = last->mpLinks     = NULL;
    first->mStart       = last->mStart      = UNDEF_TIME;
    first->mStop        = last->mStop       = UNDEF_TIME;
  //  first->mpTokens        = last->mpTokens        = NULL;
  //  first->mpExitToken     = last->mpExitToken     = NULL;
  
    node = first;
    
    for (lp = pLabels; lp != NULL; lp = lp->mpNext) 
    {
      Node *  tnode;
  
      if ((tnode              = (Node *) calloc(1, sizeof(Node))) == NULL ||
          (tnode->mpLinks     = (Link *) malloc(sizeof(Link))) == NULL    ||
          (tnode->mpBackLinks = (Link *) malloc(sizeof(Link))) == NULL) 
      {
        Error("Insufficient memory");
      }
      
      tnode->mpName = NULL;
      
      node->mpLinks[0].mpNode = tnode;
      node->mpLinks[0].mLike  = 0.0;
      
      switch (nodeType) 
      {
        case NT_WORD:  tnode->mpPronun = ((Word *) lp->mpData)->pronuns[0]; break;
        case NT_MODEL: tnode->mpHmm    =   (Hmm *) lp->mpData;              break;
        case NT_PHONE: tnode->mpName   =  (char *) lp->mpData;              break;
        default:       Error("Fatal: Invalid node type");
      }
      
      tnode->mType       = nodeType;
      tnode->mNLinks     = 1;
      tnode->mNBackLinks = 1;
      tnode->mStart      = lp->mStart;
      tnode->mStop       = lp->mStop;
      tnode->mpBackLinks[0].mpNode = node;
      tnode->mpBackLinks[0].mLike   = 0.0;
      node->mpNext = tnode;
      node = tnode;
    }
    
    node->mpNext = last;
    node->mpLinks[0].mpNode    = last;
    node->mpLinks[0].mLike    = 0.0;
    last->mpBackLinks[0].mpNode = node;
    last->mpBackLinks[0].mLike = 0.0;
    last->mpNext = NULL;
    return first;
  }
  
  
  void ExpandWordNetworkByDictionary(
    Node *        pFirstNode,
    MyHSearchData *dict,
    int keep_word_nodes,
    int multiple_pronun)
  {
    Node *node, *prev = NULL;
    int i, j;
  
    Pronun singlePronun, *singlePronunPtr;
    Word singlePronunWrd;
    singlePronunWrd.npronuns = 1;
    singlePronunWrd.pronuns  = &singlePronunPtr;
    singlePronunPtr = &singlePronun;
  
    assert(pFirstNode != NULL || pFirstNode->mType & NT_WORD || pFirstNode->mpPronun == NULL);
  
    for (node = pFirstNode; node != NULL; prev = node, node = node->mpNext) {
  
      if (!(node->mType & NT_WORD)) continue;
  
      if (node->mpPronun == NULL) continue;
      Word *word = node->mpPronun->mpWord;
  
      //Do not expand non-dictionary words, which where added by ReadSTKNetwork
      if (word->npronunsInDict == 0) continue;
  
      if (!multiple_pronun) {
        singlePronunWrd.mpName = node->mpPronun->mpWord->mpName;
        word = &singlePronunWrd;
        *word->pronuns = node->mpPronun;
      }
  
      // Remove links to current node form backlinked nodes and realloc
      // link arrays of backlinked nodes to hold word->npronuns more backlinks
      for (i = 0; i < node->mNBackLinks; i++) {
        Node *bakcnode = node->mpBackLinks[i].mpNode;
        for (j=0; j<bakcnode->mNLinks && bakcnode->mpLinks[j].mpNode!=node; j++);
        assert(j < bakcnode->mNLinks); // Otherwise link to 'node' is missing
                                      // from which backlink exists
        bakcnode->mpLinks[j] = bakcnode->mpLinks[bakcnode->mNLinks-1];
  
        bakcnode->mpLinks = (Link *)
          realloc(bakcnode->mpLinks,
                (bakcnode->mNLinks - 1 + word->npronuns) * sizeof(Link));
        if (bakcnode->mpLinks == NULL) Error("Insufficient memory");
        bakcnode->mNLinks--;// += word->npronuns-1;
      }
  
      // Remove backlinks to current node form linked nodes and realloc
      // backlink arrays of linked nodes to hold word->npronuns more backlinks
      for (i=0; i < node->mNLinks; i++) {
        Node *forwnode = node->mpLinks[i].mpNode;
        for (j=0;j<forwnode->mNBackLinks&&forwnode->mpBackLinks[j].mpNode!=node;j++);
        assert(j < forwnode->mNBackLinks);
        // Otherwise link to 'node' is missing from which backlink exists
        forwnode->mpBackLinks[j] = forwnode->mpBackLinks[forwnode->mNBackLinks-1];
  
        forwnode->mpBackLinks = (Link *)
          realloc(forwnode->mpBackLinks,
                (forwnode->mNBackLinks - 1 + word->npronuns) * sizeof(Link));
        if (forwnode->mpBackLinks == NULL) Error("Insufficient memory");
        forwnode->mNBackLinks--;
      }
      for (i = 0; i < word->npronuns; i++) {
        Pronun *pronun = word->pronuns[i];
        Node *pronun_first = NULL, *pronun_prev = NULL, *tnode;
  
        for (j = 0; j < pronun->nmodels; j++) 
        {
          tnode = (Node *) calloc(1, sizeof(Node));
          
          if (tnode == NULL) 
            Error("Insufficient memory");
  
          tnode->mType       = NT_PHONE | (node->mType & NT_TRUE);
          tnode->mpName      = pronun->model[j].mpName;
          tnode->mStart      = node->mStart;
          tnode->mStop       = node->mStop;
          tnode->mPhoneAccuracy = 1.0;
  
          if (j == 0) 
          {
            pronun_first = tnode;
          } 
          else 
          {
            if ((pronun_prev->mpLinks = (Link *) malloc(sizeof(Link))) == NULL ||
                (tnode->mpBackLinks   = (Link *) malloc(sizeof(Link))) == NULL) 
            {
              Error("Insufficient memory");
            }
            
            tnode->mNBackLinks              = 1;
            tnode->mpBackLinks[0].mpNode    = pronun_prev;
            tnode->mpBackLinks[0].mLike     = 0.0;
            pronun_prev->mNLinks            = 1;
            pronun_prev->mpLinks[0].mpNode  = tnode;
            pronun_prev->mpLinks[0].mLike   = 0.0;
            pronun_prev->mpNext             = tnode;
          }
          pronun_prev = tnode;
        }
        
        if (keep_word_nodes || j == 0) 
        {
          tnode = (Node *) calloc(1, sizeof(Node));
          
          if (tnode == NULL) Error("Insufficient memory");
  
          tnode->mpName   = NULL;
          tnode->mType    = NT_WORD | (node->mType & NT_TRUE);
          tnode->mpPronun = keep_word_nodes ? word->pronuns[i] : NULL;
          tnode->mStart   = node->mStart;
          tnode->mStop    = node->mStop;
  
          if (j == 0) {
            pronun_first = tnode;
          } else {
            if ((pronun_prev->mpLinks = (Link *) malloc(sizeof(Link))) == NULL ||
              (tnode->mpBackLinks   = (Link *) malloc(sizeof(Link))) == NULL) {
              Error("Insufficient memory");
            }
            tnode->mNBackLinks          = 1;
            tnode->mpBackLinks[0].mpNode   = pronun_prev;
            tnode->mpBackLinks[0].mLike   = 0.0;
            pronun_prev->mNLinks        = 1;
            pronun_prev->mpLinks[0].mpNode = tnode;
            pronun_prev->mpLinks[0].mLike = 0.0;
            pronun_prev->mpNext          = tnode;
          }
          pronun_prev = tnode;
        }
        if ((pronun_prev->mpLinks =
              (Link *) malloc(sizeof(Link) * node->mNLinks))==NULL ||
          (pronun_first->mpBackLinks =
              (Link *) malloc(sizeof(Link) * node->mNBackLinks)) == NULL) {
          Error("Insufficient memory");
        }
        pronun_prev->mNLinks      = node->mNLinks;
        pronun_first->mNBackLinks = node->mNBackLinks;
  
        for (j = 0; j < node->mNBackLinks; j++) {
          Node *backnode = node->mpBackLinks[j].mpNode;
          backnode->mpLinks[backnode->mNLinks  ].mpNode = pronun_first;
          backnode->mpLinks[backnode->mNLinks++].mLike = node->mpBackLinks[j].mLike;
          pronun_first->mpBackLinks[j] = node->mpBackLinks[j];
        }
        for (j=0; j < node->mNLinks; j++) {
          Node *forwnode = node->mpLinks[j].mpNode;
          forwnode->mpBackLinks[forwnode->mNBackLinks  ].mpNode = pronun_prev;
          forwnode->mpBackLinks[forwnode->mNBackLinks++].mLike = node->mpLinks[j].mLike;
          pronun_prev->mpLinks[j] = node->mpLinks[j];
        }
        if (prev != NULL) prev->mpNext = pronun_first;
        prev = pronun_prev;
      }
      prev->mpNext = node->mpNext;
      free(node->mpLinks);
      free(node->mpBackLinks);
      free(node);
      node = prev;
    }
  }
  
  
  Node *DiscardUnwantedInfoInNetwork(Node *pFirstNode, STKNetworkOutputFormat format)
  // The function discard the information in network records that is not to be
  // saved to the output. This should allow for more effective network
  // optimization, which will be run after calling this function and before
  // saving network to file.
  {
    Node *node;
    int  i;
  
    for (node = pFirstNode; node != NULL; node = node->mpNext)  {
      if (format.mNoLMLikes) {
        for (i=0; i < node->mNLinks;     i++) node->mpLinks    [i].mLike = 0.0;
        for (i=0; i < node->mNBackLinks; i++) node->mpBackLinks[i].mLike = 0.0;
      }
      if (format.mNoTimes) {
        node->mStop = node->mStart = UNDEF_TIME;
      }
      if (format.mNoWordNodes && node->mType & NT_WORD) {
        node->mpPronun = NULL;
      }
      if (format.mNoModelNodes && (node->mType&NT_MODEL || node->mType&NT_PHONE)) {
        node->mType = NT_WORD;
        node->mpPronun = NULL;
      }
      if (format.mNoPronunVars && node->mType & NT_WORD && node->mpPronun != NULL) {
        node->mpPronun = node->mpPronun->mpWord->pronuns[0];
      }
    }
    return pFirstNode;
  }

  
  //***************************************************************************
  //***************************************************************************
  static int lnkcmp(const void *a, const void *b)
  {
  //  return ((Link *) a)->mpNode - ((Link *) b)->mpNode;
  //  Did not work with gcc, probably bug in gcc pointer arithmetic
    return (char *)((Link *) a)->mpNode - (char *)((Link *) b)->mpNode;
  }
  
  //***************************************************************************
  //***************************************************************************
  static int LatticeLocalOptimization_ForwardPass(Node *pFirstNode, int strictTiming)
  {
    int i, j, k, l, m, rep;
    Node *node, *tnode;
    int node_removed = 0;
    FLOAT tlike;
    for (node = pFirstNode; node != NULL; node = node->mpNext) 
    {
      for (i = 0; i < node->mNLinks; i++) 
      {
      //for (tnode = inode; tnode != NULL; tnode = (tnode == inode ? jnode : NULL)) {
      
        tnode = node->mpLinks[i].mpNode;
        
        if (tnode->mNLinks == 0) 
          continue;
  
        // Weight pushing
        tlike = tnode->mpBackLinks[0].mLike;
        
        for (l=1; l <  tnode->mNBackLinks; l++) 
          tlike = HIGHER_OF(tlike, tnode->mpBackLinks[l].mLike);
        
        for (l=0; l < tnode->mNBackLinks; l++) 
        {
          Node *backnode = tnode->mpBackLinks[l].mpNode;
          tnode->mpBackLinks[l].mLike -= tlike;
          
          for (k=0; k<backnode->mNLinks && backnode->mpLinks[k].mpNode!=tnode; k++)
          {}
          
          assert(k < backnode->mNLinks);
          backnode->mpLinks[k].mLike -= tlike;
      #ifndef NDEBUG
          for (k++; k<backnode->mNLinks && backnode->mpLinks[k].mpNode!=tnode; k++)
          {}
      #endif
          assert(k == backnode->mNLinks);
        }
        
        for (l=0; l < tnode->mNLinks; l++) 
        {
          Node *forwnode = tnode->mpLinks[l].mpNode;
          tnode->mpLinks[l].mLike += tlike;
          
          for (k=0; k<forwnode->mNBackLinks && forwnode->mpBackLinks[k].mpNode!=tnode;k++)
          {}
          
          assert(k < forwnode->mNBackLinks);
          forwnode->mpBackLinks[k].mLike += tlike;
      #ifndef NDEBUG
          for (k++; k<forwnode->mNBackLinks && forwnode->mpBackLinks[k].mpNode!=tnode;k++)
          {}
      #endif
          assert(k == forwnode->mNBackLinks);
        }
      }
  //dnet(pFirstNode, 1, node);
  
      // For current node 'node', check for each possible pair of its successors
      // ('inode' and 'jnode') whether the pair may be merged to single node.
      for (i = 0; i < node->mNLinks-1; i++) 
      {
        for (j = i+1; j < node->mNLinks; j++) 
        {
          Node *inode = node->mpLinks[i].mpNode;
          Node *jnode = node->mpLinks[j].mpNode;

          // Final node may be never merged.
          if (inode->mNLinks == 0 || jnode->mNLinks == 0) 
            continue;


          // Two nodes ('inode' and 'jnode') may be mergeg if they are of the 
          // same type, name, ... with the same predecessors and with the same
          // weights on the links from predecesors.
          if ((inode->mType & ~NT_TRUE) != (jnode->mType & ~NT_TRUE)
          || ( inode->mType & NT_PHONE && inode->mpName   != jnode->mpName)
          || ( inode->mType & NT_WORD  && inode->mpPronun != jnode->mpPronun)

  //          &&  (inode->mpPronun == NULL ||
  //             jnode->mpPronun == NULL ||
  //             inode->mpPronun->mpWord       != jnode->mpPronun->mpWord ||
  //             inode->mpPronun->outSymbol  != jnode->mpPronun->outSymbol ||
  //             inode->mpPronun->variant_no != jnode->mpPronun->variant_no ||
  //             inode->mpPronun->prob       != jnode->mpPronun->prob)
          || (inode->mNBackLinks != jnode->mNBackLinks)) 
          {
            continue;
          }
          
          if (strictTiming && (inode->mStart != jnode->mStart
                          ||  inode->mStop  != jnode->mStop)) 
          {
            continue;
          }

//Weights on the links from predecesors does not have to be exactely the same, but the must not
//differ more than by SIGNIFICANT_PROB_DIFFERENCE
          for (l=0; l < inode->mNBackLinks; l++) 
          {
            if (inode->mpBackLinks[l].mpNode != jnode->mpBackLinks[l].mpNode) break;
            FLOAT ldiff =  inode->mpBackLinks[l].mLike - jnode->mpBackLinks[l].mLike;
            if (ldiff < -SIGNIFICANT_PROB_DIFFERENCE ||
              ldiff >  SIGNIFICANT_PROB_DIFFERENCE) 
            {
              break;
            }
          }
          
          if (l < inode->mNBackLinks) 
            continue;
  
  /*        if (memcmp(inode->mpBackLinks, jnode->mpBackLinks,
                    inode->mNBackLinks * sizeof(Link))) {
            continue;
          }*/
            // inode and jnode are the same nodes with the same predeccessors
            // Remove jnode and add its links to inode
  
          assert(inode->mNLinks && jnode->mNLinks);
  
            // Remove links to jnode form predeccessors
          for (l=0; l < jnode->mNBackLinks; l++) 
          {
            Node *backnode = jnode->mpBackLinks[l].mpNode;
            for (k=0; k<backnode->mNLinks && backnode->mpLinks[k].mpNode!=jnode; k++);
            assert(k < backnode->mNLinks);
            // Otherwise link to 'node' is missing from which backlink exists
            memmove(backnode->mpLinks+k, backnode->mpLinks+k+1,
                    (backnode->mNLinks-k-1) * sizeof(Link));
            backnode->mNLinks--;
          }
  
          // Merge jnode's links with inode links
  
          //Count jnode's links not present among inode's links
          rep = l = k = 0;
          while (k < jnode->mNLinks) 
          {
            Link *ill = inode->mpLinks+l;
            Link *jlk = jnode->mpLinks+k;
            if (l == inode->mNLinks || ill->mpNode > jlk->mpNode)
            {
              // k-th link of jnode will be included among inode's links.
              // Redirect corresponding baclink to inode
              for (m = 0; m < jlk->mpNode->mNBackLinks
                          && jlk->mpNode->mpBackLinks[m].mpNode != jnode; m++)
              {}
              
              assert(m < jlk->mpNode->mNBackLinks);
              jlk->mpNode->mpBackLinks[m].mpNode = inode;
              qsort(jlk->mpNode->mpBackLinks, jlk->mpNode->mNBackLinks,
                    sizeof(Link), lnkcmp);
              k++;
            } 
            else  if (ill->mpNode == jlk->mpNode) 
            {
              // l-th link of inode and k-th link of jnode points to
              // the same node. Link from jnode is redundant.
              // Remove backlinks to jnode form jnode's succesors
              for (m = 0; m < jlk->mpNode->mNBackLinks
                        && jlk->mpNode->mpBackLinks[m].mpNode != jnode; m++);
              {}
              
              assert(m < jlk->mpNode->mNBackLinks);
              memmove(jlk->mpNode->mpBackLinks+m, jlk->mpNode->mpBackLinks+m+1,
                      (jlk->mpNode->mNBackLinks-m-1) * sizeof(Link));
              jlk->mpNode->mNBackLinks--;
  
              ill->mLike = HIGHER_OF(ill->mLike, jlk->mLike);
              jlk->mpNode = NULL; // Mark link to be removed
              rep++; k++, l++;
            } 
            else 
            {
              l++;
            }
          }
          
          l = inode->mNLinks;
          inode->mNLinks += jnode->mNLinks-rep;
          inode->mpLinks = (Link *) realloc(inode->mpLinks,
                                          inode->mNLinks * sizeof(Link));
          
          if (inode->mpLinks == NULL) 
            Error("Insufficient memory");
  
          for (k = 0; k < jnode->mNLinks; k++) 
          {
            if (jnode->mpLinks[k].mpNode != NULL) 
              inode->mpLinks[l++] = jnode->mpLinks[k];
          }
          
          qsort(inode->mpLinks, inode->mNLinks, sizeof(Link), lnkcmp);
  
          inode->mStart = inode->mStart == UNDEF_TIME || jnode->mStart == UNDEF_TIME
                        ? UNDEF_TIME : LOWER_OF(inode->mStart, jnode->mStart);
  
          inode->mStop  = inode->mStop == UNDEF_TIME || jnode->mStop == UNDEF_TIME
                        ? UNDEF_TIME : HIGHER_OF(inode->mStop, jnode->mStop);
  
          if (inode->mAux > jnode->mAux) 
          {
          // Make sure that topological order of new inode's links
          // (inherited from jnode) is higher than inode's order.
          // In the 'next' list, move inode to jnode's lower position
            inode->mpBackNext->mpNext = inode->mpNext;
            inode->mpNext->mpBackNext = inode->mpBackNext;
            inode->mpNext           = jnode->mpNext;
            inode->mpBackNext       = jnode->mpBackNext;
            inode->mpBackNext->mpNext = inode;
            inode->mpNext->mpBackNext = inode;
            inode->mAux = jnode->mAux;
          } 
          else
          {
            jnode->mpNext->mpBackNext = jnode->mpBackNext;
            jnode->mpBackNext->mpNext = jnode->mpNext;
          }
          
          inode->mType |= jnode->mType & NT_TRUE;
          free(jnode->mpLinks);
          free(jnode->mpBackLinks);
          free(jnode);
          --j; // Process j-th node again
              // there is new shifted node on this index
  
          node_removed = 1;
        }
      }
    }
    return node_removed;
  }
  
  //***************************************************************************
  //***************************************************************************
  Node *ReverseNetwork(Node *pFirstNode)
  {
    Node *  node;
    Node *  last = NULL;
    
    for (node = pFirstNode; node != NULL; node = node->mpBackNext) 
    {
      Link *  links       = node->mpLinks;
      int     nlinks      = node->mNLinks;
      node->mpLinks       = node->mpBackLinks;
      node->mNLinks       = node->mNBackLinks;
      node->mpBackLinks   = links;
      node->mNBackLinks   = nlinks;
      Node *  next        = node->mpNext;
      node->mpNext        = node->mpBackNext;
      node->mpBackNext    = next;
      node->mAux          = -node->mAux;
      last                = node;
    }
    return last;
  }
  
  //***************************************************************************
  //***************************************************************************
  static int LatticeLocalOptimization_BackwardPass(Node *pFirstNode, int strictTiming)
  {
    int node_removed;
    Node *last = ReverseNetwork(pFirstNode);
    node_removed = LatticeLocalOptimization_ForwardPass(last, strictTiming);
  //  if (!node_removed) dnet(last, 0);
    ReverseNetwork(last);
  //  if (!node_removed) dnet(pFirstNode, 0);
    return node_removed;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  int nbacklinkscmp(const void *a, const void *b) 
  {
    return (*(Node **) a)->mNBackLinks - (*(Node **) b)->mNBackLinks;
  }
  
  //***************************************************************************
  //***************************************************************************
  void LatticeLocalOptimization(Node *pFirstNode, int strictTiming, int trace_flag)
  {
    Node *    node;
    Node *    lastnode;
    int       i;
    int       j;
    int       unreachable = 0;
    
    // For each node, sort links by pointer value to allow
    // for easy comparison whether two nodes have the same set of links
    for (node = pFirstNode; node != NULL; node = node->mpNext)  
    {
      node->mAux = 0;
      node->mpBackNext = node->mpNext;
      qsort(node->mpLinks, node->mNLinks, sizeof(Link), lnkcmp);
      qsort(node->mpBackLinks, node->mNBackLinks, sizeof(Link), lnkcmp);
    }
  
  
    // Sort nodes in topological order
    // printf("Sorting nodes...\n");
    pFirstNode->mAux = 1;
    for (lastnode = node = pFirstNode; node != NULL; node = node->mpNext) 
    {
      for (i=0; i < node->mNLinks; i++) 
      {
        Node *lnknode = node->mpLinks[i].mpNode;
        
        if (lnknode->mAux == 0) 
        {
          for (j=0; j<lnknode->mNBackLinks && lnknode->mpBackLinks[j].mpNode->mAux==1; j++)
          {}
          
          if (j == lnknode->mNBackLinks) 
          {
            lastnode->mpNext = lnknode;
            lastnode  = lnknode;
            lnknode->mAux = 1;
            lnknode->mpNext = NULL;
          }
        }
      }
    }
    
    if (lastnode->mNLinks != 0) 
    {
      // There is a cycle in graph so we cannot sort nodes
      // topologicaly, so sort it at least somehow. :o|
      // Anyway this optimization algorithm is not optimal for graphs with cycles.
      for (node = pFirstNode; node != NULL; node = node->mpBackNext) 
      {
        node->mAux = 0;
      }
        
      pFirstNode->mAux = 1;
      for (lastnode = node = pFirstNode; node != NULL; node = node->mpNext) 
      {
        for (i=0; i < node->mNLinks; i++) 
        {
          Node *lnknode = node->mpLinks[i].mpNode;
          if (lnknode->mAux == 0) 
          {
            lastnode->mpNext = lnknode;
            lastnode  = lnknode;
            lnknode->mAux = 1;
            lnknode->mpNext = NULL;
          }
        }
      }
      
      for (node=pFirstNode; node->mpNext->mNLinks != 0; node=node->mpNext)
      {}
  
      // Final node is not at the and of chain
      if (node->mpNext->mpNext) 
      { 
        lastnode->mpNext = node->mpNext;
        node->mpNext = node->mpNext->mpNext;
        lastnode = lastnode->mpNext;
        lastnode->mpNext = NULL;
      }
    }
  
    // !!! Unreachable nodes must be removed before sorting !!!
  
    for (node=pFirstNode; node != NULL; node=node->mpBackNext) 
    {
      while (node->mpBackNext && node->mpBackNext->mAux == 0) 
      {
        Node *tnode = node->mpBackNext;
        node->mpBackNext = node->mpBackNext->mpBackNext;
        unreachable++;
        free(tnode->mpLinks);
        free(tnode->mpBackLinks);
        free(tnode);
      }
    }
  
    //  if (unreachable) Warning("Removing %d unreachable nodes", unreachable);
    if (unreachable) 
      Error("Networks contains unreachable nodes");
  
    pFirstNode->mpBackNext = NULL;
    
    for (node=pFirstNode; node->mpNext != NULL; node=node->mpNext) 
    {
      node->mpNext->mpBackNext = node;
    }
    
    for (i=1, node=pFirstNode; node != NULL; node = node->mpNext, i++) 
    {
      node->mAux=i;
    }
    
    for (;;) 
    {
      if (trace_flag & 2) 
      {
        for (i=0,node=pFirstNode; node; node=node->mpNext,i++)
        {}
        
        TraceLog("Forward pass.... (number of nodes: %d)", i);
      }
      
      LatticeLocalOptimization_ForwardPass(pFirstNode, strictTiming);
  
      if (trace_flag & 2) 
      {
        for (i=0,node=pFirstNode; node; node=node->mpNext,i++)
        {}
        
        TraceLog("Backward pass... (number of nodes: %d)", i);
      }
      
      if (!LatticeLocalOptimization_BackwardPass(pFirstNode, strictTiming)) 
        break;
    }
  }
  
  //***************************************************************************
  //***************************************************************************
  void ExpandMonophoneNetworkToTriphones(
    Node *  pFirstNode,
    MyHSearchData *nonCDphones,
    MyHSearchData *CDphones)
  {
    Node *  node;
    int     did_we_clone;
    int     i;
    int     j;
    int     k;
    // Find all Tee model, Word, and Null nodes (except the pFirstNode and last Null node)
    // and clone those not having single input and output
  
    do 
    {
      ENTRY     e;
      ENTRY *   ep;
      Node *    prev = NULL;
      
      did_we_clone = 0;
      
      for (node = pFirstNode; node != NULL; prev = node, node = node->mpNext) 
      {
        if ((node->mNLinks == 0 || node->mNBackLinks == 0) ||
            (node->mNLinks == 1 && node->mNBackLinks == 1)) 
        {
          continue;
        }
        
        if (node->mType & NT_PHONE) 
        {
          e.key = node->mpName;
          my_hsearch_r(e, FIND, &ep, nonCDphones);
          if (ep == NULL || !reinterpret_cast<size_t>(ep->data))
            continue; // Node is not a Tee model
        }
        
        did_we_clone = 1;
        assert(prev != NULL); //Otherwise pFirstNode node is not Null node
  
        // Remove links to current node form back-linked nodes and realloc
        // link arrays of back-linked nodes to hold node->mNLinks more links
        for (j=0; j < node->mNBackLinks; j++) 
        {
          Node *backnode = node->mpBackLinks[j].mpNode;
          
          for (k=0; k<backnode->mNLinks && backnode->mpLinks[k].mpNode!=node; k++)
          {}
          
          assert(k < backnode->mNLinks);
          
          // Otherwise link to 'node' is missing from which backlink exists
          backnode->mpLinks[k] = backnode->mpLinks[backnode->mNLinks-1];
  
          backnode->mpLinks = 
            (Link *) realloc((backnode->mpLinks), 
                             (backnode->mNLinks-1+node->mNLinks)*sizeof(Link));
          
          if (backnode->mpLinks == NULL) 
            Error("Insufficient memory");
          
          backnode->mNLinks--;
        }
        
        // Remove backlinks to current node form linked nodes and realloc
        // backlink arrays of linked nodes to hold node->mNBackLinks more backlinks
        for (j=0; j < node->mNLinks; j++) 
        {
          Node *forwnode = node->mpLinks[j].mpNode;
          
          for (k=0;k<forwnode->mNBackLinks&&forwnode->mpBackLinks[k].mpNode!=node;k++)
          {}
          
          assert(k < forwnode->mNBackLinks);
          // Otherwise link to 'node' is missing from which backlink exists
          forwnode->mpBackLinks[k] = forwnode->mpBackLinks[forwnode->mNBackLinks-1];
  
          forwnode->mpBackLinks = 
            (Link *) realloc((forwnode->mpBackLinks),
                             (forwnode->mNBackLinks-1+node->mNBackLinks)*sizeof(Link));
                             
          if (forwnode->mpBackLinks == NULL) 
            Error("Insufficient memory");
            
          forwnode->mNBackLinks--;
        }
        // Alloc new node->mNLinks * node->mNBackLinks nodes and create new links
        // so that each backlinked node is conected with each linked node through
        // one new node.
        for (i=0; i < node->mNLinks; i++) 
        {
          for (j=0; j < node->mNBackLinks; j++) 
          {
            Node *  tnode;
            Link    forwlink = node->mpLinks[i];
            Link    backlink = node->mpBackLinks[j];
  
            if ((tnode = (Node *) calloc(1, sizeof(Node))) == NULL)
              Error("Insufficient memory");
            
            tnode->mpName = NULL;
            *tnode = *node;
  
            if ((tnode->mpLinks     = (Link *) malloc(sizeof(Link))) == NULL ||
              (tnode->mpBackLinks = (Link *) malloc(sizeof(Link))) == NULL) 
            {
              Error("Insufficient memory");
            }
            
            tnode->mNLinks       = 1;
            tnode->mNBackLinks   = 1;
            tnode->mpLinks[0]     = forwlink;
            tnode->mpBackLinks[0] = backlink;
            
            forwlink.mpNode->mpBackLinks[forwlink.mpNode->mNBackLinks  ].mpNode = tnode;
            forwlink.mpNode->mpBackLinks[forwlink.mpNode->mNBackLinks++].mLike  = forwlink.mLike;
            backlink.mpNode->mpLinks    [backlink.mpNode->mNLinks      ].mpNode = tnode;
            backlink.mpNode->mpLinks    [backlink.mpNode->mNLinks++    ].mLike  = backlink.mLike;
            
            prev->mpNext = tnode;
            prev = tnode;
          }
        }
        prev->mpNext = node->mpNext;
        free(node->mpLinks);
        free(node->mpBackLinks);
        free(node);
        node = prev;
      }
    } while (did_we_clone);
  
    // Assign to each node unique number, which will later allow to find groups of
    // expanded triphone nodes corresponding to original monophone nodes.
    int nbackmononodes;
    int nforwmononodes;
    int id = 0;
    
    for (node = pFirstNode; node != NULL; node = node->mpNext) 
      node->mAux = id++;
  
    // Expand monophone nodes to triphone nodes
    Node *prev = NULL;
    for (node = pFirstNode; node != NULL; prev = node, node = node->mpNext) 
    {
      ENTRY   e;
      ENTRY * ep;
  
      if ((node->mType & NT_WORD) ||
          (node->mNLinks == 1 && node->mNBackLinks == 1)) 
      {
        continue;
      }
      
      assert(node->mType & NT_PHONE);
      e.key = node->mpName;
      my_hsearch_r(e, FIND, &ep, nonCDphones);
      if (ep != NULL && reinterpret_cast<size_t>(ep->data)) continue; // Node is a Tee model
  
      assert(prev != NULL); //Otherwise first node is not Null node
  
      // Count groups of backlinked nodes corresponding to different monophones
      id = -1;
      nbackmononodes = 0;
      for (j=0; j < node->mNBackLinks; j++) 
      {
        if (node->mpBackLinks[j].mpNode->mAux != id) 
        {
          id = node->mpBackLinks[j].mpNode->mAux;
          nbackmononodes++;
        }
      }
      // Count groups of linked nodes corresponding to different monophones
      id = -1;
      nforwmononodes = 0;
      for (j=0; j < node->mNLinks; j++) 
      {
        if (node->mpLinks[j].mpNode->mAux != id) 
        {
          id = node->mpLinks[j].mpNode->mAux;
          nforwmononodes++;
        }
      }
  
      // Remove links to current node form backlinked nodes and realloc
      // link arrays of backlinked nodes to hold nforwmononodes more links
      for (j=0; j < node->mNBackLinks; j++) 
      {
        Node *backnode = node->mpBackLinks[j].mpNode;
        for (k=0; k<backnode->mNLinks && backnode->mpLinks[k].mpNode!=node; k++);
        assert(k < backnode->mNLinks);
        // Otherwise link to 'node' is missing from which backlink exists
        memmove(backnode->mpLinks+k, backnode->mpLinks+k+1,
                (backnode->mNLinks-k-1) * sizeof(Link));
  
        backnode->mpLinks = (Link *)
          realloc(backnode->mpLinks,
                (backnode->mNLinks-1+nforwmononodes)*sizeof(Link));
        if (backnode->mpLinks == NULL) Error("Insufficient memory");
        backnode->mNLinks--;
      }
  
      // Remove backlinks to current node form linked nodes and realloc
      // backlink arrays of linked nodes to hold nbackmononodes more backlinks
      for (j=0; j < node->mNLinks; j++) 
      {
        Node *forwnode = node->mpLinks[j].mpNode;
        for (k=0;k<forwnode->mNBackLinks&&forwnode->mpBackLinks[k].mpNode!=node;k++);
        assert(k < forwnode->mNBackLinks);
        // Otherwise link to 'node' is missing from which backlink exists
        memmove(forwnode->mpBackLinks+k, forwnode->mpBackLinks+k+1,
                (forwnode->mNBackLinks-k-1) * sizeof(Link));
  
        forwnode->mpBackLinks = (Link *)
          realloc(forwnode->mpBackLinks,
                (forwnode->mNBackLinks-1+nbackmononodes)*sizeof(Link));
        if (forwnode->mpBackLinks == NULL) Error("Insufficient memory");
        forwnode->mNBackLinks--;
      }
  
      // Alloc new nforwmononodes * nbackmononodes nodes and create new links
      // so that each backlinked node is conected through one new node with all
      // linked nodes belonging to one monophone group and vice versa each
      // linked node is conected through one new node with all backlinked nodes
      // belonging to one monophone group
      Link *forwmono_start, *forwmono_end = node->mpLinks;
      for (i=0; i < nforwmononodes; i++) 
      {
        for (forwmono_start = forwmono_end;
            forwmono_end < node->mpLinks+node->mNLinks &&
            forwmono_start->mpNode->mAux == forwmono_end->mpNode->mAux;
            forwmono_end++)
        {}
  
        assert((i <  nforwmononodes-1 && forwmono_end <  node->mpLinks+node->mNLinks) ||
              (i == nforwmononodes-1 && forwmono_end == node->mpLinks+node->mNLinks));
  
        Link *tlink, *backmono_start, *backmono_end = node->mpBackLinks;
        
        for (j=0; j < nbackmononodes; j++) 
        {
          for (backmono_start = backmono_end;
            backmono_end < node->mpBackLinks+node->mNBackLinks &&
            backmono_start->mpNode->mAux == backmono_end->mpNode->mAux;
            backmono_end++)
          {}
  
          assert((j <  nbackmononodes-1 && backmono_end <  node->mpBackLinks+node->mNBackLinks) ||
                 (j == nbackmononodes-1 && backmono_end == node->mpBackLinks+node->mNBackLinks));
  
          Node * tnode;
          if ((tnode = (Node *) calloc(1, sizeof(Node))) == NULL)
            Error("Insufficient memory");
          
          *tnode = *node;
          tnode->mNLinks       = forwmono_end-forwmono_start;
          tnode->mNBackLinks   = backmono_end-backmono_start;
  
          if ((tnode->mpLinks =
              (Link *) malloc(tnode->mNLinks * sizeof(Link))) == NULL ||
            (tnode->mpBackLinks =
              (Link *) malloc(tnode->mNBackLinks * sizeof(Link))) == NULL) 
          {
            Error("Insufficient memory");
          }
          
          for (tlink = forwmono_start; tlink < forwmono_end; tlink++) 
          {
            tnode->mpLinks[tlink-forwmono_start] = *tlink;
            tlink->mpNode->mpBackLinks[tlink->mpNode->mNBackLinks  ].mpNode = tnode;
            tlink->mpNode->mpBackLinks[tlink->mpNode->mNBackLinks++].mLike = tlink->mLike;
          }
          
          for (tlink = backmono_start; tlink < backmono_end; tlink++) 
          {
            tnode->mpBackLinks[tlink-backmono_start] = *tlink;
            tlink->mpNode->mpLinks[tlink->mpNode->mNLinks  ].mpNode = tnode;
            tlink->mpNode->mpLinks[tlink->mpNode->mNLinks++].mLike = tlink->mLike;
          }
          
          prev->mpNext = tnode;
          prev = tnode;
        }
      }
      prev->mpNext = node->mpNext;
      free(node->mpLinks);
      free(node->mpBackLinks);
      free(node);
      node = prev;
    }
  
    // Give triphone names to phone nodes and create hash of these names
    for (node = pFirstNode; node != NULL; node = node->mpNext) 
    {
      ENTRY     e;
      ENTRY *   ep      = NULL;
      Node  *   lc      = NULL;
      Node  *   rc      = NULL;
      char  *   lcname  = NULL;
      char  *   rcname  = NULL;
      char  *   triname = NULL;
      int       lcnlen  = 0;
      int       rcnlen  = 0;
  
      if (!(node->mType & NT_PHONE)) 
        continue;
  
      if (nonCDphones) 
      {
        e.key  = node->mpName;
        my_hsearch_r(e, FIND, &ep, nonCDphones);
      } 
      else 
      {
        ep = NULL;
      }
      
      if (ep != NULL) 
      {
        lc = rc = NULL;
      } 
      else 
      {
        for (lc = node;;) 
        {
          lc = lc->mNBackLinks ? lc->mpBackLinks[0].mpNode : NULL;
          
          if (lc == NULL)               break;
          if (!(lc->mType & NT_PHONE))  continue;
          if (nonCDphones == NULL)      break;
          
          e.key  = lc->mpName;
          my_hsearch_r(e, FIND, &ep, nonCDphones);
          if (ep == NULL || !reinterpret_cast<size_t>(ep->data)) break; // Node represents Tee model
        }
        
        for (rc = node;;) 
        {
          rc = rc->mNLinks ? rc->mpLinks[0].mpNode : NULL;
          
          if (rc == NULL)               break;
          if (!(rc->mType & NT_PHONE))  continue;
          if (nonCDphones == NULL)      break;
          
          e.key  = rc->mpName;
          my_hsearch_r(e, FIND, &ep, nonCDphones);
          if (ep == NULL || !reinterpret_cast<size_t>(ep->data)) break; // Node represents Tee model
        }
      }
      
      lcnlen = -1;
      if (lc != NULL) 
      {
        lcname = strrchr(lc->mpName, '-');
        
        if (lcname == NULL) 
          lcname = lc->mpName;
        else 
          lcname++;
          
        lcnlen = strcspn(lcname, "+");
      }
      
      rcnlen = -1;
      if (rc != NULL) 
      {
        rcname = strrchr(rc->mpName, '-');
        
        if (rcname == NULL) 
          rcname = rc->mpName;
        else 
          rcname++;
          
        rcnlen = strcspn(rcname, "+");
      }
      
      triname = (char *) malloc(lcnlen+1+strlen(node->mpName)+1+rcnlen+1);
      
      if (triname == NULL) 
        Error("Insufficient memory");
  
      triname[0] = '\0';
  
      if (lcnlen > 0) 
        strcat(strncat(triname, lcname, lcnlen), "-");
        
      strcat(triname, node->mpName);
      if (rcnlen > 0) 
        strncat(strcat(triname, "+"), rcname, rcnlen);
  
      e.key  = triname;
      my_hsearch_r(e, FIND, &ep, CDphones);
  
      if (ep == NULL) 
      {
        e.key  = triname;
        e.data = e.key;
  
        if (e.key == NULL || !my_hsearch_r(e, ENTER, &ep, CDphones))
          Error("Insufficient memory");
        
        node->mpName = triname;
      } 
      else 
      {
        free(triname);
        node->mpName = ep->key;
      }
    }
  }
  
  
  //***************************************************************************
  //***************************************************************************
  // Insert null node to self links. Self links are not supported
  // in network manipulation functions  
  void SelfLinksToNullNodes(Node * pFirstNode)
  {
    int i, j;
    Node *node, *tnode;
  
    for (node = pFirstNode; node != NULL; node = node->mpNext) 
    {
      for (i=0; i < node->mNLinks; i++) 
      {
        if (node->mpLinks[i].mpNode == node) 
        {
          if ((tnode           = (Node *) calloc(1, sizeof(Node))) == NULL ||
              (tnode->mpLinks     = (Link *) malloc(sizeof(Link))) == NULL ||
              (tnode->mpBackLinks = (Link *) malloc(sizeof(Link))) == NULL) 
          {
            Error("Insufficient memory");
          }
  
          tnode->mpName = NULL;
          node->mpLinks[i].mpNode = tnode;
          
          for (j=0; j<node->mNBackLinks && node->mpBackLinks[j].mpNode!=node; j++)
          {}
          
          assert(j<node->mNBackLinks);
          
          node->mpBackLinks[j].mpNode = tnode;
          node->mpBackLinks[j].mLike = 0.0;
  
          tnode->mType       = NT_WORD;
          tnode->mpPronun     = NULL;
          tnode->mNLinks     = 1;
          tnode->mNBackLinks = 1;
          tnode->mStart      = UNDEF_TIME;
          tnode->mStop       = UNDEF_TIME;
  //        tnode->mpTokens     = NULL;
  //        tnode->mpExitToken  = NULL;
          tnode->mpLinks[0].mpNode     = node;
          tnode->mpLinks[0].mLike     = 0.0;
          tnode->mpBackLinks[0].mpNode = node;
          tnode->mpBackLinks[0].mLike = node->mpLinks[i].mLike;
          tnode->mpNext = node->mpNext;
          node->mpNext = tnode;
        }
      }
    }
  }
  
  //***************************************************************************
  //***************************************************************************
  // Remove null nones having less than three predecessors or less than three successors
  int RemoveRedundantNullNodes(Node *pFirstNode)
  {
    Node *    node;
    Node *    tnode;
    int       i;
    int       j;
    int       k;
    int       node_removed = 0;
  
    pFirstNode->mpBackNext = NULL;
    for (node = pFirstNode; node->mpNext != NULL; node = node->mpNext) {
      node->mpNext->mpBackNext = node;
    }
    for (node = pFirstNode; node != NULL; node = node->mpNext) {
      if (node->mType & NT_WORD && node->mpPronun == NULL &&
          node->mNLinks != 0 && node->mNBackLinks != 0  &&
        (node->mNLinks == 1 || node->mNBackLinks == 1 ||
        (node->mNLinks == 2 && node->mNBackLinks == 2))) {
  
      node_removed = 1;
  
      // Remove links to current node form backlinked nodes and realloc
      // link arrays of backlinked nodes to hold node->mNLinks more backlinks
        for (i = 0; i < node->mNBackLinks; i++) {
          Node *bakcnode = node->mpBackLinks[i].mpNode;
          for (j=0; j<bakcnode->mNLinks && bakcnode->mpLinks[j].mpNode!=node; j++);
          assert(j < bakcnode->mNLinks); // Otherwise link to 'node' is missing
                                        // from which backlink exists
          bakcnode->mpLinks[j] = bakcnode->mpLinks[bakcnode->mNLinks-1];
  
          bakcnode->mpLinks = (Link *)
            realloc(bakcnode->mpLinks,
                  (bakcnode->mNLinks - 1 + node->mNLinks) * sizeof(Link));
          if (bakcnode->mpLinks == NULL) Error("Insufficient memory");
          bakcnode->mNLinks--;// += word->npronuns-1;
        }
  
        // Remove backlinks to current node form linked nodes and realloc
        // backlink arrays of linked nodes to hold word->npronuns more backlinks
        for (i=0; i < node->mNLinks; i++) {
          Node *forwnode = node->mpLinks[i].mpNode;
          for (j=0;j<forwnode->mNBackLinks&&forwnode->mpBackLinks[j].mpNode!=node;j++);
          assert(j < forwnode->mNBackLinks);
          // Otherwise link to 'node' is missing from which backlink exists
          forwnode->mpBackLinks[j] = forwnode->mpBackLinks[forwnode->mNBackLinks-1];
  
          forwnode->mpBackLinks = (Link *)
            realloc(forwnode->mpBackLinks,
                  (forwnode->mNBackLinks - 1 + node->mNBackLinks) * sizeof(Link));
          if (forwnode->mpBackLinks == NULL) Error("Insufficient memory");
          forwnode->mNBackLinks--;
        }
        for (j = 0; j < node->mNBackLinks; j++) {
          Node *backnode = node->mpBackLinks[j].mpNode;
          int orig_nlinks = backnode->mNLinks;
  
          for (i=0; i < node->mNLinks; i++) {
            for(k = 0; k < orig_nlinks && backnode->mpLinks[k].mpNode != node->mpLinks[i].mpNode; k++);
            if(k < orig_nlinks) {
              // Link which is to be created already exists. Its duplication must be avoided.
              backnode->mpLinks[k].mLike = HIGHER_OF(backnode->mpLinks[k].mLike, 
                                                  node->mpLinks[i].mLike + node->mpBackLinks[j].mLike);
            } else {
              backnode->mpLinks[backnode->mNLinks  ].mpNode = node->mpLinks[i].mpNode;
              backnode->mpLinks[backnode->mNLinks++].mLike
                = node->mpLinks[i].mLike + node->mpBackLinks[j].mLike;
            }
          }
        }
        for (j = 0; j < node->mNLinks; j++) {
          Node *forwnode = node->mpLinks[j].mpNode;
          int orig_nbacklinks = forwnode->mNBackLinks;
  
          for (i=0; i < node->mNBackLinks; i++) {
            for(k = 0; k < orig_nbacklinks && forwnode->mpBackLinks[k].mpNode != node->mpBackLinks[i].mpNode; k++);
            if (k < orig_nbacklinks) {
              // Link which is to be created already exists. Its duplication must be avoided.
              forwnode->mpBackLinks[k].mLike = HIGHER_OF(forwnode->mpBackLinks[k].mLike, 
                                                      node->mpBackLinks[i].mLike + node->mpLinks[j].mLike);
            } else {
              forwnode->mpBackLinks[forwnode->mNBackLinks  ].mpNode = node->mpBackLinks[i].mpNode;
              forwnode->mpBackLinks[forwnode->mNBackLinks++].mLike
                = node->mpBackLinks[i].mLike + node->mpLinks[j].mLike;
            }
          }
        }
        node->mpBackNext->mpNext = node->mpNext;
        node->mpNext->mpBackNext = node->mpBackNext;
        tnode = node;
        node = node->mpBackNext;
        free(tnode->mpLinks);
        free(tnode->mpBackLinks);
        free(tnode);
      }
    }
    return node_removed;
  }
  
  struct CorrPhnRec 
  {
    Node      *   mpNode;
    long long     maxStopTimeTillNow;
    int           mId;
  };
  
  //***************************************************************************
  //***************************************************************************
  static int cmp_starts(const void *a, const void *b)
  {
    long long diff = ((struct CorrPhnRec *) a)->mpNode->mStart
                  - ((struct CorrPhnRec *) b)->mpNode->mStart;
  
    if (diff != 0)
      return diff;
  
    return ((struct CorrPhnRec *) a)->mpNode->mStop
        - ((struct CorrPhnRec *) b)->mpNode->mStop;
  }
  
  //***************************************************************************
  //***************************************************************************
  static int cmp_maxstop(const void *key, const void *elem)
  {
    struct CorrPhnRec *corr_phn = (struct CorrPhnRec *) elem;
  
    if (((Node *) key)->mStart < corr_phn->maxStopTimeTillNow) {
      if (corr_phn->mId == 0 || // first fiead in the array
        ((Node *) key)->mStart >= (corr_phn-1)->maxStopTimeTillNow)
          return  0;
      else return -1;
    } else return  1;
  }
  
  //***************************************************************************
  //***************************************************************************
  // Check whether strings a and b represents the same monophone
  int SamePhoneme(char *a, char *b)
  {
    char *  chptr;
    size_t  len;
  
    chptr = strrchr(a, '-');
    if (chptr) a = chptr+1;
    len = strcspn(a, "+");
  
    chptr = strrchr(b, '-');
    
    if (chptr) 
      b = chptr+1;
      
    if (strcspn(b, "+") != len) 
      return 0;
  
    return strncmp(a, b, len) == 0;
  }
  
  
  //***************************************************************************
  //***************************************************************************
  void ComputeAproximatePhoneAccuracy(Node *pFirstNode, int type)
  {
    Node *node;
    struct CorrPhnRec *corr_phn, *overlaped;
    int ncorr_phns = 0, i = 0;
    long long maxStopTime;
  
    for (node = pFirstNode; node != NULL; node = node->mpNext) {
      if (node->mType & NT_PHONE && node->mType & NT_TRUE) ncorr_phns++;
    }
    if (ncorr_phns == 0) Error("No correct phoneme node in network");
  
    corr_phn = (struct CorrPhnRec *)malloc(sizeof(struct CorrPhnRec)*ncorr_phns);
    if (corr_phn == NULL) Error("Insufficient memory");
  
    for (node = pFirstNode; node != NULL; node = node->mpNext) {
      if (node->mType & NT_PHONE && node->mType & NT_TRUE) {
        corr_phn[i++].mpNode = node;
      }
    }
  
    qsort(corr_phn, ncorr_phns, sizeof(struct CorrPhnRec), cmp_starts);
  
    maxStopTime = corr_phn[0].mpNode->mStop;
    for (i = 0; i < ncorr_phns; i++) {
      corr_phn[i].mId = i;
      maxStopTime = HIGHER_OF(maxStopTime, corr_phn[i].mpNode->mStop);
      corr_phn[i].maxStopTimeTillNow = maxStopTime;
    }
    for (node = pFirstNode; node != NULL; node = node->mpNext) {
      if (!(node->mType & NT_PHONE)) continue;
  
      if (node->mStop  <= node->mStart ||
        !strcmp(node->mpName, "sil") ||
        !strcmp(node->mpName, "sp")) {
        //!!! List of ignored phonemes should be provided by some switch !!!
        node->mPhoneAccuracy = 0.0;
      } else {
        node->mPhoneAccuracy = -1.0;
        overlaped = (CorrPhnRec*)bsearch(node, corr_phn, ncorr_phns,
                            sizeof(struct CorrPhnRec), 
          cmp_maxstop);
  
        if (overlaped) {
          for (; overlaped < corr_phn + ncorr_phns &&
                overlaped->mpNode->mStart < node->mStop; overlaped++) {
            if (overlaped->mpNode->mStop  <= overlaped->mpNode->mStart ||
              overlaped->mpNode->mStop  <= node->mStart) continue;
  
            node->mPhoneAccuracy =
              HIGHER_OF(node->mPhoneAccuracy,
                        (SamePhoneme(overlaped->mpNode->mpName, node->mpName) + 1.0) *
                        (LOWER_OF(overlaped->mpNode->mStop, node->mStop) -
                         HIGHER_OF(overlaped->mpNode->mStart, node->mStart)) /
                        (overlaped->mpNode->mStop - overlaped->mpNode->mStart) - 1.0);
  
            if (node->mPhoneAccuracy >= 1.0) break;
          }
        }
      }
    }
    free(corr_phn);
  }
  
  
  
  
  // Debug function showing network using AT&T dot utility
  void dnet(Node *net, int nAuxNodePtrs, ...)
  {
    static int dnetcnt=1;
    va_list ap;
    Node *node;
    int i = 1;
  
    FILE *fp = popen("cat | (tf=`mktemp /tmp/netps.XXXXXX`;"
                    "dot -Tps > $tf; gv -scale -4 $tf; rm $tf)",
                    "w");
  //  FILE *fp = stdout;
  
    if (fp == NULL) return;
  
    for (node = net; node != NULL; node = node->mpNext) {
      node->mEmittingStateId = i++;
    }
    fprintf(fp, "digraph \"dnet%d\" {\nrankdir=LR\n", dnetcnt++);
  
  
    for (node = net; node != NULL; node = node->mpNext) {
      fprintf(fp, "n%d [shape=%s,label=\"%d:%s", node->mEmittingStateId,
              node->mType & NT_WORD ? "box" : "ellipse", node->mEmittingStateId,
              node->mType & NT_WORD ? (node->mpPronun ?
                                      node->mpPronun->mpWord->mpName : "-"):
              node->mType & NT_PHONE? node->mpName :
              node->mType & NT_MODEL? node->mpHmm->mpMacro->mpName : "???");
  
      if (node->mType & NT_WORD && node->mpPronun != NULL) {
        if (node->mpPronun != node->mpPronun->mpWord->pronuns[0]) {
          fprintf(fp, ":%d", node->mpPronun->variant_no);
        }
        fprintf(fp, "\\n");
  
        if (node->mpPronun->outSymbol != node->mpPronun->mpWord->mpName) {
          fprintf(fp, "[%s]", node->mpPronun->outSymbol ?
                              node->mpPronun->outSymbol : "");
        }
        if (node->mpPronun->prob != 0.0) {
          fprintf(fp, " "FLOAT_FMT, node->mpPronun->prob);
        }
      }
      fprintf(fp, "\"];\n");
  //    if (node->mpNext != NULL) {
  //     fprintf(fp,"n%d -> n%d [color=black,weight=1]\n",
  //             node->mEmittingStateId, node->mpNext->mEmittingStateId);
  //    }
  //    if (node->mpBackNext != NULL) {
  //     fprintf(fp,"n%d -> n%d [color=gray,weight=1]\n",
  //             node->mEmittingStateId, node->mpBackNext->mEmittingStateId);
  //    }
    }
    for (node = net; node != NULL; node = node->mpNext) {
      for (i = 0; i < node->mNLinks; i++) {
        fprintf(fp,"n%d -> n%d [color=blue,weight=1",
                node->mEmittingStateId,node->mpLinks[i].mpNode->mEmittingStateId);
        if (node->mpLinks[i].mLike != 0.0) {
          fprintf(fp,",label=\""FLOAT_FMT"\"", node->mpLinks[i].mLike);
        }
        fprintf(fp,"];\n");
      }
  //    for (i = 0; i < node->mNBackLinks; i++) {
  //      fprintf(fp,"n%d -> n%d [color=red,weight=1",
  //              node->mEmittingStateId,node->mpBackLinks[i].mpNode->mEmittingStateId);
  //      if (node->mpBackLinks[i].mLike != 0.0) {
  //        fprintf(fp,",label=\""FLOAT_FMT"\"", node->mpBackLinks[i].mLike);
  //      }
  //      fprintf(fp,"];\n");
  //    }
    }
    va_start(ap, nAuxNodePtrs);
    for (i = 0; i < nAuxNodePtrs; i++) {
      Node *ptr = va_arg(ap, Node *);
      fprintf(fp, "AuxPtr%d [shape=plaintext];\nAuxPtr%d -> n%d\n",
              i, i, ptr->mEmittingStateId);
    }
    va_end(ap);
  
    fprintf(fp, "}\n");
    pclose(fp);
  }
  
  
  
  void NetworkExpansionsAndOptimizations(
    Node *node,
    ExpansionOptions expOptions,
    STKNetworkOutputFormat out_net_fmt,
    MyHSearchData *wordHash,
    MyHSearchData *nonCDphHash,
    MyHSearchData *triphHash)
  {

/*  
<<<<<<< .mine Ondra Glembek
    if (expOptions.mNoWordExpansion &&  expOptions.mCDPhoneExpansion
    && expOptions.mNoOptimization   && !out_net_fmt.mNoLMLikes
    &&!out_net_fmt.mNoTimes         && !out_net_fmt.mNoWordNodes &&
      !out_net_fmt.mNoModelNodes   && !out_net_fmt.mNoPronunVars) return;
=======
    if (expOptions.no_word_expansion && !expOptions.CD_phone_expansion
    && expOptions.no_optimization    && !out_net_fmt.no_LM_likes
    &&!out_net_fmt.no_times          && !out_net_fmt.no_word_nodes &&
      !out_net_fmt.no_model_nodes    && !out_net_fmt.no_pronun_vars) return;
>>>>>>> .r44
*/
    
    if (expOptions.mNoWordExpansion  && !expOptions.mCDPhoneExpansion &&
        expOptions.mNoOptimization   && !out_net_fmt.mNoLMLikes &&
        !out_net_fmt.mNoTimes        && !out_net_fmt.mNoWordNodes &&
        !out_net_fmt.mNoModelNodes   && !out_net_fmt.mNoPronunVars) 
    {
      return;
    }
    
    SelfLinksToNullNodes(node);
    if (!expOptions.mNoWordExpansion) {
      if (!expOptions.mNoOptimization) {
        LatticeLocalOptimization(node, expOptions.mStrictTiming, expOptions.mTraceFlag);
      }
      ExpandWordNetworkByDictionary(node, wordHash, !expOptions.mRemoveWordsNodes,
                                                    !expOptions.mRespectPronunVar);
    }
    if (expOptions.mCDPhoneExpansion) {
      if (!expOptions.mNoOptimization) {
        LatticeLocalOptimization(node, expOptions.mStrictTiming, expOptions.mTraceFlag);
      }
      ExpandMonophoneNetworkToTriphones(node, nonCDphHash, triphHash);
    }
    DiscardUnwantedInfoInNetwork(node, out_net_fmt);
  
    if (!expOptions.mNoOptimization) {
      LatticeLocalOptimization(node, expOptions.mStrictTiming, expOptions.mTraceFlag);
    }
    RemoveRedundantNullNodes(node);
  }

} // namespace STK
