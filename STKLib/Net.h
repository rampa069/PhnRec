/***************************************************************************
 *   copyright            : (C) 2004 by Lukas Burget,UPGM,FIT,VUT,Brno     *
 *   email                : burget@fit.vutbr.cz                            *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef STK_Net_h
#define STK_Net_h

#include "Models.h"
#include "labels.h"
#include "dict.h"
#include "common.h"

namespace STK
{
  // Class list (overview)
  //
  class Node;
  class Link;
  class Token;
  class FWBWR;
  
  
  // Enums
  //
  enum NodeType 
  {
    NT_UNDEF  = 0x00,
    NT_WORD   = 0x01,
    NT_MODEL  = 0x02,
    NT_PHONE  = 0x04,
    NT_SUBNET = 0x08,
    NT_TEE    = 0x10,
    NT_STICKY = 0x20,
    NT_TRUE   = 0x40
  };
  
  enum NotInDictActionType
  {
    WORD_NOT_IN_DIC_UNSET = 0,
    WORD_NOT_IN_DIC_ERROR = 1,
    WORD_NOT_IN_DIC_WARN  = 2,
    PRON_NOT_IN_DIC_ERROR = 4
  };
    
  
  // Class declarations
  //
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Network link representation class
   */
  class Link 
  {
  public:
    Node *        mpNode;
    FLOAT         mLike;
  }; // Link
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Network node representation class
   */
  class Node
  {
  public:
    char*         mpName;
    Hmm*          mpHmm;
    Hmm*          mpHmmToUpdate;
    Pronun*       mpPronun;
  
    int           mAux;
    int           mType;
    Node*         mpNext;
    Node*         mpBackNext;
    int           mNLinks;
    int           mNBackLinks;
    Link*         mpLinks;
    Link*         mpBackLinks;
  
    //time range when model can be active - apply only for model type
    long long     mStart;
    long long     mStop;
    FLOAT         mPhoneAccuracy;
  
#   ifndef EXPANDNET_ONLY
    Token*        mpTokens;
    Token*        mpExitToken;
  
    //id of first emiting state - apply only for model type
    int           mEmittingStateId;
  
    FWBWR*        mpAlphaBetaList;
    FWBWR*        mpAlphaBetaListReverse;
  
    Node*         mpNextActiveModel;
    Node*         mpPrevActiveModel;
  
    Node*         mpNextActiveNode;
    Node*         mpPrevActiveNode;
    
    int           mIsActive;
    int           mIsActiveNode;
#   ifndef NDEBUG
    int           mAux2;
#   endif
#   endif
  }; // class Node
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief STK network specific output format options
   */
  class STKNetworkOutputFormat 
  {
  public:
    unsigned mNoLMLikes      : 1 ;
    unsigned mNoTimes        : 1 ;
    unsigned mStartTimes     : 1 ;
    unsigned mNoWordNodes    : 1 ;
    unsigned mNoModelNodes   : 1 ;
    unsigned mNoPronunVars   : 1 ;
    unsigned mNoDefaults     : 1 ;
    unsigned mAllFieldNames  : 1 ;
    unsigned mArcDefsToEnd   : 1 ;
    unsigned mArcDefsWithJ   : 1 ;
    unsigned mBase62Labels   : 1 ;
    unsigned mAproxAccuracy  : 1 ;
  
    //Have no effect yet
    unsigned mNoAccLikes     : 1 ;
    unsigned mStripTriphones : 1 ;
    unsigned mLinNodeSeqs    : 1 ;
  };
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Network expansion options definitions
   */
  class ExpansionOptions 
  {
  public:
    unsigned mNoOptimization    : 1;
    unsigned mNoWordExpansion   : 1;
    unsigned mRespectPronunVar  : 1;
    unsigned mRemoveWordsNodes  : 1;
    unsigned mCDPhoneExpansion  : 1;
    unsigned mStrictTiming      : 1;
    unsigned mTraceFlag;
  };
  
  
  // GLOBAL FUNCTIONS
  //
  
  Node *MakeNetworkFromLabels(Label *labels, enum NodeType node_type);
  
  void ExpandWordNetworkByDictionary(
    Node *first,
    MyHSearchData *dict,
    int keep_word_nodes,
    int multiple_pronun);
  
  void ExpandMonophoneNetworkToTriphones(
    Node *first,
    MyHSearchData *nonCDphones,
    MyHSearchData *CDphones);
  
  void LatticeLocalOptimization(
    Node *first,
    int strictTiming,
    int trace_flag);
  
  Node *DiscardUnwantedInfoInNetwork(
    Node *first,
    STKNetworkOutputFormat format);
  
  void WriteSTKNetwork(
    FILE *flp,
    Node *node,
    STKNetworkOutputFormat format,
    long sampPeriod,
    const char *label_file,
    const char *out_MNF);
  
  void WriteSTKNetworkInOldFormat(
    FILE *flp,
    Node *node,
    LabelFormat labelFormat,
    long sampPeriod,
    const char *label_file,
    const char *out_MNF);
  
  void FreeNetwork(Node *node);
  
  Node *ReadSTKNetwork(
    FILE *lfp,
    MyHSearchData *word_hash,
    MyHSearchData *phone_hash,
    int notInDict,
    LabelFormat labelFormat,
    long sampPeriod,
    const char *file_name,
    const char *in_MLF);
  
  Node *ReadSTKNetworkInOldFormat(
    FILE *lfp,
    MyHSearchData *word_hash,
    MyHSearchData *phone_hash,
    LabelFormat labelFormat,
    long sampPeriod,
    const char *file_name,
    const char *in_MLF);
  
  Node *ReadHTKLattice(
    FILE *lfp,
    MyHSearchData *word_hash,
    MyHSearchData *phone_hash,
    LabelFormat labelFormat,
    long sampPeriod,
    const char *file_name);
  
  void ComputeAproximatePhoneAccuracy(
    Node *first,
    int type);
  
  void SelfLinksToNullNodes(Node *first);
  int RemoveRedundantNullNodes(Node *first);
  
  void NetworkExpansionsAndOptimizations(
    Node *node,
    ExpansionOptions expOptions,
    STKNetworkOutputFormat out_net_fmt,
    MyHSearchData *dictHash,
    MyHSearchData *nonCDphHash,
    MyHSearchData *triphHash);

}; // namespace STK

#endif // STK_Net_h
