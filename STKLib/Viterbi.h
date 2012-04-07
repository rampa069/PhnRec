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

#ifndef STK_Viterbi_h
#define STK_Viterbi_h

#define bordel_staff

#include "Models.h"
#include "labels.h"
#include "dict.h"
#include "Net.h"

//#define DEBUG_MSGS
//#define TRACE_TOKENS

#define IS_ACTIVE(token) ((token).mLike > LOG_MIN)


namespace STK
{

  //###########################################################################
  //###########################################################################
  // CLASS OVERVIEW
  //###########################################################################
  //###########################################################################
  class Cache;
  class WordLinkRecord;
  class Network;
  
  
  //###########################################################################
  //###########################################################################
  // GENERAL ENUMS
  //###########################################################################
  //###########################################################################
  typedef enum 
  {
    SP_ALL, 
    SP_TRUE_ONLY
  } SearchPathsType;
  
  
  typedef enum 
  {
    NO_ALIGNMENT    = 0,
    WORD_ALIGNMENT  = 1,
    MODEL_ALIGNMENT = 2,
    STATE_ALIGNMENT = 4,
    FRAME_ALIGNMENT = 8
  } AlignmentType;
  
  
  typedef enum 
  {
    AT_ML=0,
    AT_MPE,
    AT_MFE,
    AT_MCE,
    AT_MMI
  } AccumType;
  
  
  //###########################################################################
  //###########################################################################
  // CLASS DEFINITIONS
  //###########################################################################
  //###########################################################################
  
  
  class Cache 
  {
  public:
    FLOAT mValue;
    long  mTime;
  };
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Network representation
   */
  class Network 
  {
  private:
    /**
     * @brief Forward-Backward method return type
     */
    struct FWBWRet 
    {
      double          totLike;            ///< total likelihood
      FloatInLog      avgAccuracy;        ///< average accuracy
    }; // struct FWBWRet

    
    /**
     * @brief Sorts nodes in this network
     */
    void 
    SortNodes();
    
    /**
     * @brief Checks for a cyclic structure in network
     */
    int 
    HasCycle();
    
    void 
    ReestState(Node *    pNode,
               int       stateIndex, 
               FLOAT     logPriorProb, 
               FLOAT     updateDir,
               FLOAT *   obs, 
               FLOAT *   obs2);
      
    /**
     * @brief  Computes likelihood using the forward/backward algorithm
     * @param  pObsMx 
     * @param  nFrames number of frames to use
     * @return @c FWBWRet structure containing total likelihood and average
     *         accuracy
     */
    Network::FWBWRet
    ForwardBackward(FLOAT * pObsMx, int nFrames);
    
    /**
     * @brief Frees 
     */
    void 
    FreeFWBWRecords();
  
    /**
     * @brief Returns true if propagation is forward
     */
    const bool
    InForwardPass() const {return mPropagDir == FORWARD;}
    
    Node *
    pActivateWordNodesLeadingFrom(Node * pNode);
    
    void
    ActivateModel(Node * pNode);

    void
    DeactivateModel(Node *pNode);
    
    void
    DeactivateWordNodesLeadingFrom(Node *pNode);
    
    void 
    MarkWordNodesLeadingFrom(Node *node);
    
    bool
    AllWordSuccessorsAreActive();
    
    void 
    TokenPropagationInit();
    
    void 
    TokenPropagationInNetwork();

    void
    TokenPropagationInModels(FLOAT *observation);
        
    void 
    TokenPropagationDone();

  public:
    //Subnet part
    Node  *                 mpFirst;
    Node  *                 mpLast;
  
    Node  *                 mpActiveModels;
    Node  *                 mpActiveNodes;
    int                     mActiveTokens;
                            
    int                     mNumberOfNetStates;
    Token *                 mpAuxTokens;
    Cache *                 mpOutPCache;
    Cache *                 mpMixPCache;
  
    long                    mTime;
    Token *                 mpBestToken;
    Node  *                 mpBestNode;
    
    // Passing parameters
    State *                 mpThreshState;
    FLOAT                   mBeamThresh;
    FLOAT                   mWordThresh;
                            
    FLOAT                   mWPenalty;  
    FLOAT                   mMPenalty;
    FLOAT                   mPronScale;
    FLOAT                   mLmScale;
    FLOAT                   mTranScale;
    FLOAT                   mOutpScale;
    FLOAT                   mOcpScale;
    FLOAT                   mPruningThresh;
    SearchPathsType         mSearchPaths;
                              
    FLOAT   (*OutputProbability) (State *state, FLOAT *observation, Network *network);
    int     (*PassTokenInNetwork)(Token *from, Token *to, FLOAT addLogLike);
    int     (*PassTokenInModel)  (Token *from, Token *to, FLOAT addLogLike);
    
    PropagDirectionType     mPropagDir;
    int                     mAlignment;
    int                     mCollectAlphaBeta;
    
  //  int                     mmi_den_pass;
    AccumType               mAccumType;
    ModelSet *              mpModelSet;
    ModelSet *              mpModelSetToUpdate;
    
    
    //*************************************************************************
    void 
    /**
     * @brief Initializes the network
     * @param net 
     * @param first 
     * @param hmms 
     * @param hmmsToUptade 
     */
    Init(Node * pFirst, ModelSet * pHmms, ModelSet * pHmmsToUptade);
    
    void
    /**
     * @brief Releases memory occupied by the network resources
     */
    Release();
  
    
    void              
    ViterbiInit();  
    
    void              
    ViterbiStep(FLOAT * pObservation);
    
    FLOAT             
    ViterbiDone(Label ** pLabels);
    
    FLOAT 
    MCEReest(FLOAT * pObsMx, FLOAT * pObsMx2, int nFrames, FLOAT weight, FLOAT sigSlope);
    
    FLOAT 
    ViterbiReest(FLOAT * pObsMx, FLOAT * pObsMx2, int nFrames, FLOAT weight);
    
    FLOAT
    BaumWelchReest(FLOAT * pObsMx, FLOAT * pObsMx2, int nFrames, FLOAT weight);

  }; // class Network
  //***************************************************************************
  //***************************************************************************
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Token representation
   */
  class Token 
  {
  public:
    double                  mLike;            ///< Likelihood
    WordLinkRecord   *      mpWlr;            ///< Associated word link record
    FloatInLog              mAccuracy;        ///< Accuracy
    
#   ifdef bordel_staff
    WordLinkRecord   *      mpTWlr;
    FLOAT                   mBestLike;
#   endif
  
    /**
     * @brief Returns true if token is active
     */
    bool
    IsActive() const {return mLike > LOG_MIN;}
    
    /**
     * @brief Returns pointer to an array of this token's labels
     */
    Label *
    pGetLabels();
    
    void
    AddWordLinkRecord(Node *node, int state_idx, int time);
  };
    
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Alpha and Beta pair for forward/backward propagation
   */
  struct AlphaBeta 
  {
    FLOAT             mAlpha;
    FLOAT             mBeta;
    FloatInLog        mAlphaAccuracy;
    FloatInLog        mBetaAccuracy;
  };
  
  
  class FWBWR 
  {
  public:
    FWBWR *           mpNext;
    int               mTime;
    AlphaBeta         mpState[1];
  };
  
  
  class WordLinkRecord 
  {
  public:
    Node  *           mpNode;
    int               mStateIdx;
    FLOAT             mLike;
    long              mTime;
    WordLinkRecord *  mpNext;
    int               mNReferences;
  #ifdef DEBUG_MSGS
    WordLinkRecord *  mpTmpNext;
    bool              mIsFreed;
  #endif
  };
  
  
  
  void              KillToken(Token *token);
  
  FLOAT             DiagCGaussianMixtureDensity(State *state, FLOAT *obs, Network *network);
  FLOAT             FromObservationAtStateId(State *state, FLOAT *obs, Network *network);
  int               PassTokenMax(Token *from, Token *to, FLOAT addLogLike);
  int               PassTokenSum(Token *from, Token *to, FLOAT addLogLike);
  int               PassTokenSumUnlogLikes(Token *from, Token *to, FLOAT addLogLike);
  
  WordLinkRecord *  TimePruning(Network *network, int frame_delay);
  
  void LoadRecognitionNetwork(
    char *        netFileName,
    ModelSet *    hmms,
    Network *     net);
  
  void ReadRecognitionNetwork(
    FILE *        fp,
    ModelSet *    hmms,
    Network *     network,
    LabelFormat   labelFormat,
    long          sampPeriod,
    const char *  file_name,
    const char *  in_MLF);
  
  FLOAT *StateOccupationProbability(
    Network *     network,
    FLOAT *       observationMx,
    ModelSet *    hmmset,
    int           nFrames,
    FLOAT **      outProbOrMahDist,
    int           getMahalDist);
    
  /*
  FLOAT MCEReest(
    Network *     net,
    FLOAT *       obsMx,
    FLOAT *       obsMx2,
    int           nFrames,
    FLOAT         weight,
    FLOAT         sigSlope);
  
  FLOAT BaumWelchReest(
    Network *     net,
    FLOAT *       obsMx,
    FLOAT *       obsMx2,
    int           nFrames,
    FLOAT         weight);
  
  FLOAT ViterbiReest(
    Network *     net,
    FLOAT *       observationMx,
    FLOAT *       observationMx2,
    int           nFrames,
    FLOAT         weight);
  */
}; // namespace STK

#endif  // #ifndef STK_Viterbi_h
  
