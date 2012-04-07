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

#ifndef STK_Models_h
#define STK_Models_h

#include "Matrix.h"
#include "common.h"
#include "stkstream.h"


#define SQR(x) ((x) * (x))

#define WARN_FEW_EXAMPLES(type, name, exs) \
  Warning(type" %s is not updated (%s%ld example%s)", \
          name, exs == 0 ? "" : "only ", exs, exs == 1 ? "" : "s")
  
#define DEFAULT_XFORM_NAME "defaultInputXform"


namespace STK
{  
  class ModelSetIOBase;
  class ModelSet;
  class MacroHash;  
  class MacroData;
  class Macro;
  class Hmm;  
  class State;
  class Mixture;
  class MixtureLink;
  class Mean;
  class Variance;
  class Transition;
  class XformInstance;
  class Xform;
  class CompositeXform;
  class CopyXform;
  class LinearXform;
  class BiasXform;
  class FuncXform;
  class StackingXform;
  class XformStatCache;
  class XformStatAccum;

  enum MacroType 
  {
    mt_hmm             = 'h',
    mt_state           = 's',
    mt_mixture         = 'm',
    mt_mean            = 'u',
    mt_variance        = 'v',
  //mt_variance        = 'i',
    mt_transition      = 't',
    mt_XformInstance   = 'j',
    mt_Xform           = 'x',
  };

  
  /**
   * @name MacroTypeMask
   * These constants define the flags used by Scan(...) methods. The method 
   * uses these flags to decide which macros are processed by the
   * action procedure.
   */
  //@{
  const int MTM_HMM             = 0x0001;
  const int MTM_STATE           = 0x0002;
  const int MTM_MIXTURE         = 0x0004;
  const int MTM_MEAN            = 0x0008;
  const int MTM_VARIANCE        = 0x0010;
  const int MTM_TRANSITION      = 0x0020;
  const int MTM_XFORM_INSTANCE  = 0x0040;
  const int MTM_XFORM           = 0x0080;
  const int MTM_ALL             = 0x00ff;
  const int MTM_REVERSE_PASS    = 0x4000; ///< Process last macro first
  const int MTM_PRESCAN         = 0x8000; ///< Process HMMs then states then mixtures, ...
  //@}

  
  typedef char HMMSetNodeName[128];

  /**
   * @brief Scan action procedure type
   * 
   * This function type is used by the scan functions to perform the desired
   * action on them.
   */  
  typedef void (*ScanAction)( int             type,           ///< Type of data to perform the action on
                              HMMSetNodeName  nodeName,       ///< node name
                              MacroData *     pData,          ///< Macro data class 
                              void *          pUserData);     ///< User data to process

  
  /// model set flags
  //@{  
  const FlagType MODEL_SET_WITH_ACCUM   = 1; ///< The set should allocate space for
                                             ///< accumulators 
  //@}
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Model IO interface (abstract class
   */
  class ModelSetIOBase
  {
    int x;
  public:
    ModelSetIOBase() {};
    
    virtual
    ~ModelSetIOBase() {};
    
    /**
     * @brief Loads a model set from stream
     * @param pModelSet ModelSet structure to load
     * @param rFileName file name 
     * @return true on success, false on failure
     */
    virtual bool
    Load(ModelSet & rModelSet, const std::string & rFileName, char * expectHMM) = 0;
    
    /**
     * @brief Writes a model set to stream
     * @param ModelSet ModelSet structure to load
     * @param rFileName file name 
     * @return true on success, false on failure
     */
    virtual bool
    Save(ModelSet & rModelSet, const std::string & rFileName) = 0;
  }; // class ModelIO
  
    
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Macro data base class
   *  
   *  Any class, that can be defined as a macro in the MMF file
   */
  class MacroData
  {
  public:
    Macro *                     mpMacro;
  
  public:  
    /// Default constructor
    MacroData() : mpMacro(NULL) {}
    
    virtual
    ~MacroData() {};
    
    // we'll make the Macro class a frien as it will increment the mLinksCount
    friend class Macro;
    
    virtual void
    Scan(int mask, HMMSetNodeName nodeName, ScanAction action, void *userData)
    { }
  }; // class MacroData

  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Macro representation
   */
  class Macro 
  {
  public:
    char*         mpName;
    char*         mpFileName;
    MacroData*    mpData;
    long          mOccurances;
    int           mType;
    Macro*        nextAll;
    Macro*        prevAll;
  };
  
          
  enum UpdateMask
  {
    UM_TRANSITION = 1,
    UM_MEAN       = 2,
    UM_VARIANCE   = 4,
    UM_WEIGHT     = 8,
    UM_OLDMEANVAR = 16,
    UM_XFSTATS    = 32,
    UM_XFORM      = 64
  };
  
  enum KeywordID
  {
    KID_UNSET=-1,
    KID_BeginHMM,    KID_Use,        KID_EndHMM,    KID_NumMixes,  KID_NumStates,
    KID_StreamInfo,  KID_VecSize,    KID_NullD,     KID_PoissonD,  KID_GammaD,
    KID_RelD,        KID_GenD,       KID_DiagC,     KID_FullC,     KID_XformC,
    KID_State,       KID_TMix,       KID_Mixture,   KID_Stream,    KID_SWeights,
    KID_Mean,        KID_Variance,   KID_InvCovar,  KID_Xform,     KID_GConst,
    KID_Duration,    KID_InvDiagC,   KID_TransP,    KID_DProb,     KID_LLTC,
    KID_LLTCovar,
    KID_XformKind=90,KID_ParentXform,KID_NumXforms, KID_XformSet,  KID_LinXform,
    KID_Offset,      KID_Bias,       KID_BlockInfo, KID_Block,     KID_BaseClass,
    KID_Class,       KID_XformWgtSet,KID_ClassXform,KID_MMFIDMask, KID_Parameters,
    KID_NumClasses,  KID_AdaptKind,  KID_Prequal,   KID_InputXform,
    KID_RClass  =110,KID_RegTree,    KID_Node,      KID_TNode,
    KID_HMMSetID=119,KID_ParmKind,
  
    /* Non-HTK keywords */
    KID_FrmExt  =200,KID_PDFObsVec,  KID_ObsCoef,    KID_Input,    KID_NumLayers,
    KID_NumBlocks,   KID_Layer,      KID_Copy,       KID_Stacking,
  
    /* Numeric functions - FuncXform*/
    KID_Sigmoid,     KID_Log,        KID_Exp,        KID_Sqrt,     KID_SoftMax,
    
    KID_Weights = 253,
  
    KID_MaxKwdID
  };
 
  
  class MakeXformCommand 
  {
  public:
    Xform *mpXform;
    char  *mpShellCommand;
  };
  
  struct FunctionTable
  {
    void (*funcPtr)(FLOAT *, FLOAT*, int);
    KeywordID KID;
  };
  


  /** *************************************************************************
   ** *************************************************************************
   *  @brief Master model class
   * 
   *  This class encapsulates STK model manipulation
   */
  class ModelSet 
  {
  public:
    /// :TODO:
    /// This atribute is a temporary flag that tells whether to use new 
    /// Matrix in IO operations
    bool            mUseNewMatrix;
    
  private:
    ModelSetIOBase  * mpIOBase;
    
    /// This group of methods does exactly what their names state
    /// @{ 
    Hmm *           ReadHMM           (FILE * fp, Macro * macro);
    State *         ReadState         (FILE * fp, Macro * macro);
    Mixture *       ReadMixture       (FILE * fp, Macro * macro);
    Mean *          ReadMean          (FILE * fp, Macro * macro);
    Variance *      ReadVariance      (FILE * fp, Macro * macro);
    Transition *    ReadTransition    (FILE * fp, Macro * macro);
    XformInstance * ReadXformInstance (FILE * fp, Macro * macro);
    Xform *         ReadXform         (FILE * fp, Macro * macro);
    CompositeXform *ReadCompositeXform(FILE * fp, Macro * macro);
    LinearXform *   ReadLinearXform   (FILE * fp, Macro * macro);
    CopyXform *     ReadCopyXform     (FILE * fp, Macro * macro);
    BiasXform *     ReadBiasXform     (FILE * fp, Macro * macro);
    FuncXform *     ReadFuncXform     (FILE * fp, Macro * macro, int funcId);
    StackingXform * ReadStackingXform (FILE * fp, Macro * macro);
    int             ReadGlobalOptions (FILE * fp);
    
    void   WriteHMM          (FILE * fp, bool binary, Hmm * hmm);
    void   WriteState        (FILE * fp, bool binary, State * state);
    void   WriteMixture      (FILE * fp, bool binary, Mixture * mixture);
    void   WriteMean         (FILE * fp, bool binary, Mean * mean);
    void   WriteVariance     (FILE * fp, bool binary, Variance * variance);
    void   WriteTransition   (FILE * fp, bool binary, Transition * transition);
    void   WriteXformInstance(FILE * fp, bool binary, XformInstance * xformInstance);
    void   WriteXform        (FILE * fp, bool binary,          Xform * xform);
    void   WriteCompositeXform(FILE *fp, bool binary, CompositeXform * xform);
    void   WriteLinearXform  (FILE * fp, bool binary,    LinearXform * xform);
    void   WriteCopyXform    (FILE * fp, bool binary,      CopyXform * xform);
    void   WriteFuncXform    (FILE * fp, bool binary,      FuncXform * xform);
    void   WriteBiasXform    (FILE * fp, bool binary,      BiasXform * xform);
    void   WriteStackingXform(FILE * fp, bool binary,  StackingXform * xform);
    void   WriteGlobalOptions(FILE * fp, bool binary);  
    /// @}
    
    
  public:
    Macro *                   mpFirstMacro;
    Macro *                   mpLastMacro;
    
    MyHSearchData             mHmmHash;
    MyHSearchData             mStateHash;
    MyHSearchData             mMixtureHash;
    MyHSearchData             mMeanHash;
    MyHSearchData             mVarianceHash;
    MyHSearchData             mTransitionHash;
    MyHSearchData             mXformInstanceHash;
    MyHSearchData             mXformHash;
    
    int                       mInputVectorSize;
    int                       mParamKind;
    size_t                    mNMixtures;
    size_t                    mNStates;
    bool                      mAllocAccums;
    int                       mTotalDelay;
    bool                      mAllMixuresUpdatableFromStatAccums;
    bool                      mIsHTKCopatible;         ///< Models use no extension with respecto to HTK
    
    KeywordID                 mOutPdfKind;
    KeywordID                 mDurKind;
    XformInstance *           mpInputXform;
    XformInstance *           mpXformInstances;
    
    //Reestimation params
    int                       mUpdateMask;
    FLOAT                     mMinMixWeight;
    Variance *                mpVarFloor;
    double                    mMinVariance;            ///< global minimum variance floor
    long                      mMinOccurances;
    MakeXformCommand *        mpXformToUpdate;
    size_t                    mNumberOfXformsToUpdate;
    int                       mGaussLvl2ModelReest;
    int                       mMmiUpdate;
    FLOAT                     MMI_E;
    FLOAT                     MMI_h;
    FLOAT                     MMI_tauI;
    
    bool                      mClusterWeightUpdate;
    BiasXform**               mpClusterWeights;
    int                       mNClusterWeights;
    Matrix<FLOAT>*            mpGw;                                             ///< Accumulator G_w for cluster vector update
    Matrix<FLOAT>*            mpKw;                                             ///< Accumulator k_w for cluster vector update
    OStkStream                mClusterWeightStream;
    std::string               mClusterWeightOutPath;
    
    /**
     * @name MMF output functions
     */
    //@{
    void 
    /**
     * @brief Writes the complete Model set to a file
     * @param rFileName file name to write to
     * @param rOutputDir output path
     * @param rOutputExt output file's implicit extension
     * @param binary write in binary mode if true
     */
    WriteMmf(const char * pFileName, 
             const char * pOutputDir,
             const char * pOutputExt, bool binary);
    
    void 
    /**
     * @brief Writes HMM statistics to a file
     * @param rFileName file to write to
     */
    WriteHMMStats(const char * pFileName);             
    
    
    /**
     * @brief Writes accumulators to a file
     * @param rFileName 
     * @param rOutputDir 
     * @param totFrames 
     * @param totLogLike 
     */
    void 
    WriteAccums(const char * pFileName, 
                const char * pOutputDir,
                long         totFrames, 
                FLOAT        totLogLike);
    
    void 
    /**
     * 
     * @param rOutDir 
     * @param binary 
     */
    WriteXformStatsAndRunCommands(const char * pOutDir, bool binary);
    //@}
    
    
    /**
     *  @brief Initializes the Model set
     *  @param modelSetType tells whether the models should allocate space for accumulators
     */  
    void
    Init(FlagType flags = 0);
    
    /**
     *  @brief Releases memory occupied by this object's structures
     */
    void
    Release();
      
    
    /**
     * @name MMF input functions
     */
    //@{
    void
    /**
     *  @brief Reads definition from the stream and parses
     *  @param rName filename to parse
     *  @return @c true on success
     *  
     *  The entire stream is read and propriate structures are constructed.
     */  
    ParseMmf(const char * pName, char * expectHMM);
    
    void 
    /**
     * @brief Loads an HMM list from file
     * @param rFileName  file to open
     *
     */
    ReadHMMList(const char * pFileName, 
                const char * pInMmfDir, 
                const char * pInMmfExt);
    
    /**
     * @brief Reads accumulators from file
     * @param rFileName file to read from
     * @param weight 
     * @param totFrames 
     * @param totLogLike 
     * @param MMI_denominator_accums 
     */
    void
    ReadAccums(const char * pFileName, 
               float        weight,
               long *       totFrames, 
               FLOAT *      totLogLike, 
               int          mmiDenominatorAccums);
    
    
    void
    ReadXformStats(const char * pOutDir, bool binary);
    
    void
    ReadXformList(const char * pFileName);
    //@}
    
    
    void
    AllocateAccumulatorsForXformStats();
                    
    void 
    NormalizeAccums();
    
    /**
     * @brief Resets the accumulators to 0
     */
    void 
    ResetAccums();
    
    void 
    ComputeGlobalStats(FLOAT* observation, int time);
    
    void 
    UpdateFromAccums(const char* pOutputDir);
    
    void 
    DistributeMacroOccurances();
    
    void 
    ResetXformInstances();
  
    void
    UpdateStacks(FLOAT* obs, int time,  PropagDirectionType dir);
    
    Macro*
    pAddMacro(const char type, const std::string & rNewName);    
    
    MyHSearchData 
    MakeCIPhoneHash();
    
    
    void
    ComputeClusterWeightVectorCAT(size_t i);
    
    void
    ComputeClusterWeightVectorsCAT();
    
    void 
    ResetClusterWeightAccumsCAT(size_t i);
    
    void 
    /**
     * @brief Performs desired @c action on the set's data which are chosen by @mask
     * @param mask bit mask of flags that tells which data to process by @c action
     * @param nodeNameBuffer 
     * @param action 
     * @param pUserData 
     */
    Scan( int             mask,
          HMMSetNodeName  nodeNameBuffer,
          ScanAction      action, 
          void *          pUserData);
  }; // ModelSet

  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Basic class defining HMM 
   * 
   *  This class represents the Hidden Markov Model structure and defines
   *  supporting procedures.
   */
  class Hmm : public MacroData 
  {
  public:
    size_t                    mNStates;
    Transition*               mpTransition;
    State**                   mpState;
    
  public:
    /// Constructor
    Hmm(size_t nStates);
    
    /// Destructor
    virtual ~Hmm();
    
    /**
     * @brief Updates the object from the accumulators
     * @param rModelSet ModelSet object which holds the accumulator configuration
     */
    void
    UpdateFromAccums(const ModelSet * pModelSet);
    
    
    /**
     * @brief Performs desired @c action on the HMM's data which are chosen by @mask
     * @param mask bit mask of flags that tells which data to process by @c action
     * @param nodeNameBuffer 
     * @param action routine to apply on pUserData
     * @param pUserData the data to process by @c action
     */
    virtual void
    Scan( int             mask,
          HMMSetNodeName  nodeNameBuffer,
          ScanAction      action, 
          void *          pUserData);
          
    /**
     * @brief Returns number of states
     * @return integer with number of states
     */
    size_t
    NStates() const
    {
      return mNStates;
    }    
  };
  
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Defines HMM State representation
   */
  class State : public MacroData  
  {
  public:
    
    /**
     * @brief The constructor
     * @param numMixes Number of gaussian mixtures to use
     */
    State(size_t numMixes);
    
    /**
     * @brief The destructor
     */     
    virtual 
    ~State();
    
    
    long                      mID;
  
    KeywordID                 mOutPdfKind;
    union 
    {
      size_t                  mNumberOfMixtures;
      int                     PDF_obs_coef;
    };
  
    struct MixtureLink
    {
    public:
      Mixture*                mpEstimates;
      FLOAT                   mWeight;
      FLOAT                   mWeightAccum; //used for reestimation
      FLOAT                   mWeightAccumDen;
    }*                       mpMixture;
    
    void
    /**
     * @brief Updates the object from the accumulators
     * @param rModelSet ModelSet object which holds the accumulator configuration
     * @param rHmm parrent Hmm of this state
     */
    UpdateFromAccums(const ModelSet * pModelSet, const Hmm * pHmm);
  
  
    void
    /**
     * @brief Performs desired @c action on the HMM's data which are chosen by @mask
     * @param mask bit mask of flags that tells which data to process by @c action
     * @param nodeNameBuffer 
     * @param action routine to apply on pUserData
     * @param pUserData the data to process by @c action
     */
    Scan( int             mask,
          HMMSetNodeName  nodeNameBuffer,
          ScanAction      action, 
          void *          pUserData);
  
  };

  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Defines HMM Mixture
   *
   */
  class Mixture : public MacroData 
  {
  private:
    FLOAT                     mGConst;
    
  public:
    /// The (empty) constructor
    Mixture(): mpMean(NULL), mpVariance(NULL), mpInputXform(NULL)
    {};
    
    /// The (empty) destructor
    virtual
    ~Mixture() {};
    
    
    long                      mID;
    Mean*                     mpMean;
    Variance*                 mpVariance;
    XformInstance*            mpInputXform;
  
    /// Returns the GConst constant
    const FLOAT &
    GConst() const { return mGConst; }
    
    /// Gives full access to the GConst
    FLOAT &
    rGConst() { return mGConst; }
    
    /// Computes the GConst constant
    void
    ComputeGConst();
    
    void
    /**
     * @brief Updates the object from the accumulators
     * @param rModelSet ModelSet object which holds the accumulator configuration
     */
    UpdateFromAccums(const ModelSet * pModelSet);
    
    /**
     * @brief Performs variance flooring on the mixture's variance
     * @param rModelSet parrent ModelSet containing variance floor vector
     * @return pointer to the mixture variance object
     */
    Variance*
    FloorVariance(const ModelSet * pModelSet);

    
    Mixture&
    AddToAccumCAT(Matrix<FLOAT>* pGw, Matrix<FLOAT>* pKw);

    Mixture&
    ResetAccumCAT();
        
    void
    /**
     * @brief Performs desired @c action on the HMM's data which are chosen by @mask
     * @param mask bit mask of flags that tells which data to process by @c action
     * @param nodeNameBuffer 
     * @param action routine to apply on pUserData
     * @param pUserData the data to process by @c action
     */
    Scan( int             mask,
          HMMSetNodeName  nodeNameBuffer,
          ScanAction      action, 
          void *          pUserData);
  };

  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Xform statistics accumulator
   */
  class XformStatAccum
  {
  public:
    Xform*                  mpXform;
    FLOAT                   mNorm;
    FLOAT*                  mpStats;
  };
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Mean representation
   */
  class Mean : public MacroData 
  {
  public:
    /**
     * @brief The consturctor
     * @param vectorSize Mean vector size
     * @param allocateAccums if true, memory for accumulators is allocated
     */
    Mean(size_t vectorSize, bool allocateAccums);
    
    /// The destructor
    virtual
    ~Mean();
    
    
    XformStatAccum*         mpXformStatAccum;
    size_t                  mNumberOfXformStatAccums;
    bool                    mUpdatableFromStatAccums;
    
    BiasXform**             mpWeights;                 ///< Specifies cluser weight vectors defined by Bias Xform macros (if CAT), otherwise NULL
    size_t                  mNWeights;                 ///< Number of cluset weight vectors to use
    Matrix<FLOAT>           mClusterMatrix;            ///< Cluster mean vectors in matrix (stored in transposed form = mean vectors in rows)
    FLOAT*                  mpOccProbAccums;           ///< Occupation probability accumulators
    Matrix<FLOAT>           mCwvAccum;                 ///< Cluster weight vector accumulators (\sum \gamma_m(\tau) o(\tau))
    
    FLOAT*                  mpVectorO;                 ///< Matrix of cluster mean vectors (with CAT)
#ifdef STK_MEMALIGN_MANUAL
    FLOAT*                  mpVectorOFree;
#endif
    
    /**
     * @brief Returns mean vector size
     */
    const size_t
    VectorSize() const 
    {return mVectorSize;}
    
    /**
     * @brief Accumulator array accessor
     * @return Pointer to the array of accumulators
     */
    FLOAT*
    Accumulators()
    { return mpAccumulators; }
    
    /**
     * @brief Updates the object from the accumulators
     * @param rModelSet ModelSet object which holds the accumulator configuration
     */
    void
    UpdateFromAccums(const ModelSet * pModelSet);
    
    /** @brief Recalculates mean vector for cluster adaptive training
     * 
     * mClusterMatrix holds the matrix of cluster mean vectors, mpWeights is the 
     * cluster weight vector. This method implements their scalar multiplication
     */
    void
    RecalculateCAT();
  
  private:
    size_t                  mVectorSize;
    FLOAT*                  mpAccumulators;
#ifdef STK_MEMALIGN_MANUAL
    FLOAT*                  mpAccumulatorsFree;
#endif
  };

  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Variance representation
   */
  class Variance : public MacroData 
  {
  public:
    /**
     * @brief The consturctor
     * @param vectorSize Mean vector size
     * @param allocateAccums if true, memory for accumulators is allocated
     */
    Variance(size_t vectorSize, bool allocateAccums);
    
    /// The destructor
    virtual
    ~Variance();
  
    //  BOOL         diagonal;
    XformStatAccum *        mpXformStatAccum;
    size_t                  mNumberOfXformStatAccums;
    bool                    mUpdatableFromStatAccums;
    FLOAT*                  mpVectorO;    
#ifdef STK_MEMALIGN_MANUAL
    FLOAT*                  mpVectorOFree;
#endif
    
    /**
     * @brief Returns mean vector size
     */
    const size_t
    VectorSize() const 
    {return mVectorSize;}
    
    void
    /**
     * @brief Updates the object from the accumulators
     * @param rModelSet ModelSet object which holds the accumulator configuration
     */
    UpdateFromAccums(const ModelSet * pModelSet);
  
  private:
    size_t                  mVectorSize;
  };
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Defines HMM Transition representation (matrix)
   *
   *
   */
  class Transition : public MacroData 
  {
  public:
    /**
     * @brief The constructor
     * @param nStates Number of states
     * @param allocateAccums if true, memory for accumulators is allocated
     */
    Transition(size_t nStates, bool allocateAccums);
    
    /// The destructor
    virtual
    ~Transition();
  
    size_t                  mNStates;
    //Matrix<FLOAT>           mMatrix;
    FLOAT *                 mpMatrixO;
  
    void
    /**
     * @brief Updates the object from the accumulators
     * @param rModelSet ModelSet object which holds the accumulator configuration
     */
    UpdateFromAccums(const ModelSet * pModelSet);
  };


  /** *************************************************************************
   ** *************************************************************************
   *  @brief Xform statisctics cache
   */
  class XformStatCache 
  {
  public:
    XformStatCache *        mpUpperLevelStats;
    Xform *                 mpXform;
    int                     mNorm;
    FLOAT *                 mpStats;
  };

  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Xform Instance 
   */
  class XformInstance : public MacroData 
  {
  public:
    /**
     * @brief The constructor
     * @param vectorSize size of output vector
     */
    XformInstance(size_t vectorSize);

    /**
     * @brief A constructor
     * @param pXform Xform to use in the instance
     * @param vectorSize size of the output vector
     * @return 
     */
    XformInstance(Xform * pXform, size_t vectorSize);
        
    
    /// the destructor
    virtual
    ~XformInstance();
    
  
    XformInstance*        mpInput;
    Xform*                mpXform;
    int                   mTime;
    XformInstance*        mpNext; // Chain of all instances
    XformStatCache*       mpXformStatCache;
    size_t                mNumberOfXformStatCaches;
    size_t                mOutSize;
    int                   mStatCacheTime;
    char*                 mpMemory;
    int                   mTotalDelay;

    Variance*             mpVarFloor;
        
    FLOAT*                mpOutputVector; 
    
    FLOAT*
    XformPass(FLOAT *in_vec, int time, PropagDirectionType dir);
  
  
    void
    /**
     * @brief Performs desired @c action on the HMM's data which are chosen by @mask
     * @param mask bit mask of flags that tells which data to process by @c action
     * @param nodeNameBuffer 
     * @param action routine to apply on pUserData
     * @param pUserData the data to process by @c action
     */
    Scan( int             mask,
          HMMSetNodeName  nodeNameBuffer,
          ScanAction      action, 
          void*           pUserData);
  
  };

  
  typedef enum 
  {
    XT_LINEAR,
    XT_COPY,
    XT_BIAS,
    XT_FUNC,
    XT_STACKING,
    XT_COMPOSITE,
  } XformType;

  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Xform interface (abstract class) to all kinds of transforms
   */
  class Xform : public MacroData 
  {
  public:
    XformType           mXformType;
    size_t              mInSize;
    size_t              mOutSize;
    size_t              mMemorySize;
    int                 mDelay;
    
    /**
     * @brief Interface to the evaluation procedure of a concrete Xform
     * @param pInputVector pointer to the input vector
     * @param pOutputVector pointer to the output vector
     * @param pMemory pointer to the extra memory needed by the operation
     * @param direction propagation direction (forward/backward)
     */
    virtual FLOAT * 
    Evaluate(FLOAT *    pInputVector, 
             FLOAT *    pOutputVector,
             char *     pMemory,
             PropagDirectionType  direction) = 0;
  
    void
    /**
     * @brief Performs desired @c action on the HMM's data which are chosen by @mask
     * @param mask bit mask of flags that tells which data to process by @c action
     * @param nodeNameBuffer 
     * @param action routine to apply on pUserData
     * @param pUserData the data to process by @c action
     */
    Scan( int             mask,
          HMMSetNodeName  nodeNameBuffer,
          ScanAction      action, 
          void *          pUserData);
  };

  
  /** *************************************************************************
   ** *************************************************************************
   * @brief Composite Xform Layer represenation
   *
   * This class represents a layer for Composite Xform. It holds separate 
   * blocks. See also CompositeXform class.
   */
  class XformLayer
  {
  public:
    FLOAT *             mpOutputVector;
    size_t              mNBlocks;
    Xform **            mpBlock;
    
    /// The (empty) constructor
    XformLayer();
    
    /// The destructor
    ~XformLayer();
    
    Xform **
    /**
     * @brief Inits (creates) the blocks
     * @param nBlocks number of blocks in the layer
     * @return pointer to the newly created array of pointers to Xforms
     */
    InitBlocks(size_t nBlocks);
  }; // class XformLayer
  
  
  /** *************************************************************************
   ** *************************************************************************
   * @brief Composite Xform 
   * 
   * The base composite Xform holds elementary layers of the form
   */
  class CompositeXform : public Xform 
  {
  public:
    /**
     * @brief The constructor
     * @param nLayers number of layers the composite should have
     */
    CompositeXform(size_t nLayers);
    
    /// The destructor
    virtual
    ~CompositeXform();    
    
    size_t              mNLayers;    
    XformLayer *        mpLayer;
    
    /**
     * @brief Composite Xform evaluation 
     * @param pInputVector pointer to the input vector
     * @param pOutputVector pointer to the output vector
     * @param pMemory pointer to the extra memory needed by the operation
     * @param direction propagation direction (forward/backward)
     */
    virtual FLOAT * 
    Evaluate(FLOAT *    pInputVector, 
             FLOAT *    pOutputVector,
             char *     pMemory,
             PropagDirectionType  direction);
  };
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Linear Xform representation
   */
  class LinearXform : public Xform 
  {
  public:
    /**
     * @brief The constructor
     * @param inSize input vector size
     * @param outSize output vector size
     */
    LinearXform(size_t inSize, size_t outSize);
    
    /// The destructor
    virtual
    ~LinearXform();
  
    
    Matrix<FLOAT>       mMatrix;
    FLOAT *             mpMatrixO;
    
    /**
     * @brief Linear Xform evaluation 
     * @param pInputVector pointer to the input vector
     * @param pOutputVector pointer to the output vector
     * @param pMemory pointer to the extra memory needed by the operation
     * @param direction propagation direction (forward/backward)
     */
    virtual FLOAT * 
    Evaluate(FLOAT *    pInputVector, 
             FLOAT *    pOutputVector,
             char *     pMemory,
             PropagDirectionType  direction);
  };
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Bias Xform representation
   */
  class BiasXform : public Xform 
  {
  public:
    /**
     * @brief The constructor
     * @param vectorSize size of
     */
    BiasXform(size_t vectorSize);
    
    virtual
    ~BiasXform();
    
    Matrix<FLOAT>       mVector;
    //FLOAT *           mpVectorO;
    
    
    /**
     * @brief Bias Xform evaluation 
     * @param pInputVector pointer to the input vector
     * @param pOutputVector pointer to the output vector
     * @param pMemory pointer to the extra memory needed by the operation
     * @param direction propagation direction (forward/backward)
     */
    virtual FLOAT * 
    Evaluate(FLOAT *    pInputVector, 
             FLOAT *    pOutputVector,
             char *     pMemory,
             PropagDirectionType  direction);
  };
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Function Xform representation
   */
  class FuncXform : public Xform 
  {
  public:
    /**
     * @brief The constructor
     * @param size parameter vector size
     * @param funcId function ID
     */
    FuncXform(size_t size, int funcId);
    
    virtual
    ~FuncXform();
    
    int                 mFuncId;  
  
    /**
     * @brief Function Xform evaluation 
     * @param pInputVector pointer to the input vector
     * @param pOutputVector pointer to the output vector
     * @param pMemory pointer to the extra memory needed by the operation
     * @param direction propagation direction (forward/backward)
     */
    virtual FLOAT * 
    Evaluate(FLOAT *    pInputVector, 
             FLOAT *    pOutputVector,
             char *     pMemory,
             PropagDirectionType  direction);
  };
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Copy Xform representation
   */
  class CopyXform : public Xform 
  {
  public:
    /**
     * @brief The constructor
     * @param inSize size of input vector
     * @param outSize size of output vector
     */
    CopyXform(size_t inSize, size_t outSize);
    
    virtual
    ~CopyXform();
    
    int *               mpIndices;  
  
    /**
     * @brief Copy Xform evaluation 
     * @param pInputVector pointer to the input vector
     * @param pOutputVector pointer to the output vector
     * @param pMemory pointer to the extra memory needed by the operation
     * @param direction propagation direction (forward/backward)
     */
    virtual FLOAT * 
    Evaluate(FLOAT *    pInputVector, 
             FLOAT *    pOutputVector,
             char *     pMemory,
             PropagDirectionType  direction);  
  };
  
  
  /** *************************************************************************
   ** *************************************************************************
   *  @brief Stacking Xform representation
   */
  class StackingXform : public Xform 
  {
  public:
    /**
     * @brief The Constructor
     * @param stackSize size of the stack
     * @param inSize size of the input vector
     */
    StackingXform(size_t stackSize, size_t inSize);
    
    /// The destructor
    virtual
    ~StackingXform();
    
    int                 mHorizStack;
  
    /**
     * @brief Stacking Xform evaluation 
     * @param pInputVector pointer to the input vector
     * @param pOutputVector pointer to the output vector
     * @param pMemory pointer to the extra memory needed by the operation
     * @param direction propagation direction (forward/backward)
     */
    virtual FLOAT * 
    Evaluate(FLOAT *    pInputVector, 
             FLOAT *    pOutputVector,
             char *     pMemory,
             PropagDirectionType  direction);
  };
  
  
  class XformStatsFileNames
  {
  public:
    FILE *      mpStatsP;
    char *      mpStatsN;
    FILE *      mpOccupP;
    char *      mpOccupN;
  };
  
  ///
  /// @{
  class ReadAccumUserData 
  {
  public:
    FILE   *      mpFp;
    const char*   mpFileName;
    ModelSet *    mpModelSet;
    float         mWeight;
    int           mMmi;
  };
    
  class WriteAccumUserData
  {
  public:
    FILE *        mpFp;
    char *        mpFileName;
    int           mMmi;
  };

  class ClusterWeightAccums
  {
  public:
    int            mNClusterWeights;
    Matrix<FLOAT>* mpGw;
    Matrix<FLOAT>* mpKw;
  };
  
  class ReplaceItemUserData
  {
  public:
    MacroData *         mpOldData;
    MacroData *         mpNewData;
    int                 mType;
  };

  struct GlobalStatsUserData
  {
    FLOAT *             observation;
    int                 mTime;
  } ;
  
  class WriteStatsForXformUserData 
  {
  public:
    LinearXform *             mpXform;
    XformStatsFileNames       mMeanFile;
    XformStatsFileNames       mCovFile;
    bool                      mBinary;
  };
  /// @}

    
  
  extern const char *   gpCurrentMmfName;
                            
  extern bool           gHmmsIgnoreMacroRedefinition;         ///< Controls macro redefinition behavior
  extern const char *   gpHListFilter;                        ///< HMM list Filter command
  extern FunctionTable  gFuncTable[];                         ///< FuncXform table (keyword Id to function mapping)
  extern size_t         gFuncTableSize;                       ///< Number of records in gFuncTable
  extern char *         gpKwds[KID_MaxKwdID];                 ///< MMF keyword table
                                                           

                  
  /**
   * @brief Passes the vector through XformInstance
   */
  FLOAT *     XformPass(XformInstance *xformInstance, FLOAT *in_vec, int time, PropagDirectionType dir);

  void        ReleaseMacroHash(MyHSearchData *macro_hash);
  Macro *     FindMacro(MyHSearchData *macro_hash, const char *name);
  
  void        PutFlt(FILE *fp, bool binary, FLOAT f);
  void        PutInt(FILE *fp, bool binary, int i);
  void        PutKwd(FILE *fp, bool binary, KeywordID kwdID);
  void        PutNLn(FILE *fp, bool binary);
  
  FLOAT       GetFloat(FILE *fp);
  int         GetInt(FILE *fp);
  char *      GetString(FILE *fp, int eofNotExpected);
  void        RemoveSpaces(FILE *fp);
  void        UngetString(void);
  
  
  int         CheckKwd(const char *str, KeywordID kwdID);
  void        InitKwdTable();
  KeywordID   ReadDurKind(char *str);
  KeywordID   ReadOutPDFKind(char *str);
  
  /// Action procedures
  /// @{
  void        AllocateXformStatCachesAndAccums(int, HMMSetNodeName, MacroData * pData, void *);
  void        GlobalStats(int macro_type, HMMSetNodeName, MacroData * pData, void *userData);
  void        NormalizeAccum(int macro_type, HMMSetNodeName, MacroData * pData, void *userData);
  void        NormalizeStatsForXform(int macro_type, HMMSetNodeName, MacroData * pData, void *userData);
  void        ReadAccum  (int macro_type, HMMSetNodeName, MacroData * pData, void *userData);
  void        ReadStatsForXform(int macro_type, HMMSetNodeName, void *data, void *userData);
  void        ReleaseItem(int macro_type, HMMSetNodeName, MacroData * pData, void *userData);
  void        ReplaceItem(int macro_type, HMMSetNodeName, MacroData * pData, void *userData);
  void        ResetAccum (int macro_type, HMMSetNodeName, MacroData * pData, void *userData);
  void        WriteAccum (int macro_type, HMMSetNodeName, MacroData * pData, void *userData);
  void        WriteStatsForXform(int macro_type, HMMSetNodeName, MacroData * pData, void *userData);
  void        ComputeClusterWeightVectorAccums(int macro_type, HMMSetNodeName nodeName, MacroData* pData, void* pUserData);
  /// @}  
  
}; //namespace STK

#endif // STK_Models_h
