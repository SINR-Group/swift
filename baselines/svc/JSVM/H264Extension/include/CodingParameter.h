
#if !defined(AFX_CODINGPARAMETER_H__8403A680_A65D_466E_A411_05C3A7C0D59F__INCLUDED_)
#define AFX_CODINGPARAMETER_H__8403A680_A65D_466E_A411_05C3A7C0D59F__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResizeParameters.h"

H264AVC_NAMESPACE_BEGIN

// JVT-AD021 {
#define MULTILAYER_LAMBDA_OFF		0
#define MULTILAYER_LAMBDA_C1		1
#define MULTILAYER_LAMBDA_C08		2
// JVT-AD021 }


#if defined( MSYS_WIN32 )
# pragma warning( disable: 4275 )
#endif

#define MAX_CONFIG_PARAMS 256

class H264AVCENCODERLIB_API EncoderConfigLineBase
{
protected:
  EncoderConfigLineBase( const Char* pcTag, UInt uiType ) : m_cTag( pcTag ), m_uiType( uiType ) {}
  EncoderConfigLineBase() {}
public:
  virtual ~EncoderConfigLineBase() {}
  std::string&  getTag () { return m_cTag; }
  virtual Void  setVar ( std::string& rcValue ) = 0;
protected:
  std::string m_cTag;
  UInt m_uiType;
};

class H264AVCENCODERLIB_API MotionVectorSearchParams
{
public:
  MotionVectorSearchParams()
    : m_uiSearchMode      ( FAST_SEARCH )
    , m_uiFullPelDFunc    ( DF_SAD )
    , m_uiSubPelDFunc     ( DF_SAD )
    , m_uiSearchRange     ( 64 )
    , m_uiDirectMode      ( 0 )
    , m_uiELSearchRange   ( 0 )
    , m_uiFastBiSearch    ( 0 )
    , m_bELSearch         ( false )
  {}

  ErrVal check();

  SearchMode  getSearchMode     ()  const { return (SearchMode)m_uiSearchMode; }
  DFunc       getFullPelDFunc   ()  const { return (DFunc)m_uiFullPelDFunc; }
  DFunc       getSubPelDFunc    ()  const { return (DFunc)m_uiSubPelDFunc; }
  UInt        getSearchRange    ()  const { return m_uiSearchRange; }
  UInt        getNumMaxIter     ()  const { return m_uiNumMaxIter; }
  UInt        getIterSearchRange()  const { return m_uiIterSearchRange; }
  UInt        getDirectMode     ()  const { return m_uiDirectMode; }
  UInt        getELSearchRange  ()  const { return m_uiELSearchRange; }
  UInt        getFastBiSearch   ()  const { return m_uiFastBiSearch; }
  Bool        getELSearch       ()  const { return m_bELSearch; }

  Void setSearchMode      ( UInt uiSearchMode )       { m_uiSearchMode  = uiSearchMode; }
  Void setFullPelDFunc    ( UInt uiFullPelDFunc )     { m_uiFullPelDFunc = uiFullPelDFunc; }
  Void setSubPelDFunc     ( UInt uiSubPelDFunc )      { m_uiSubPelDFunc = uiSubPelDFunc; }
  Void setSearchRange     ( UInt uiSearchRange )      { m_uiSearchRange = uiSearchRange; }
  Void setNumMaxIter      ( UInt uiNumMaxIter )       { m_uiNumMaxIter = uiNumMaxIter; }
  Void setIterSearchRange ( UInt uiIterSearchRange )  { m_uiIterSearchRange = uiIterSearchRange; }
  Void setDirectMode      ( UInt uiDirectMode)        { m_uiDirectMode = uiDirectMode; }
  Void setELSearchRange   ( UInt ui )                 { m_uiELSearchRange = ui; }
  Void setFastBiSearch    ( UInt ui )                 { m_uiFastBiSearch = ui; }

public:
  UInt        m_uiSearchMode;
  UInt        m_uiFullPelDFunc;
  UInt        m_uiSubPelDFunc;
  UInt        m_uiSearchRange;   // no limit
  UInt        m_uiNumMaxIter;
  UInt        m_uiIterSearchRange;
  UInt        m_uiDirectMode;    // 0 temporal, 1 spatial
  UInt        m_uiELSearchRange;
  UInt        m_uiFastBiSearch;
  Bool        m_bELSearch;
};



class H264AVCENCODERLIB_API LoopFilterParams
{
public:
  LoopFilterParams() : m_uiFilterIdc( 0 ),  m_iAlphaOffset( 0 ), m_iBetaOffset( 0 ) {}

  ErrVal check() const ;

  Bool  isDefault()                     const { return m_uiFilterIdc == 0 && m_iAlphaOffset == 0 && m_iBetaOffset == 0;}
  const UInt getFilterIdc()             const { return m_uiFilterIdc; }
  const Int getAlphaOffset()            const { return m_iAlphaOffset; }
  const Int getBetaOffset()             const { return m_iBetaOffset; }
  Void setAlphaOffset( Int iAlphaOffset )     { m_iAlphaOffset = iAlphaOffset; }
  Void setBetaOffset( Int iBetaOffset )       { m_iBetaOffset = iBetaOffset; }
  Void setFilterIdc( UInt uiFilterIdc)        { m_uiFilterIdc = uiFilterIdc; }

public:
  UInt m_uiFilterIdc;   // 0: Filter All Edges, 1: Filter No Edges, 2: Filter All Edges But Slice Boundaries
  Int  m_iAlphaOffset;
  Int  m_iBetaOffset;
};


const unsigned CodParMAXNumSliceGroupsMinus1 =8; // the same as MAXNumSliceGroupsMinus1 in pictureParameter.h



class CodingParameter;


class H264AVCENCODERLIB_API LayerParameters
{
public:
  LayerParameters()
    : m_uiLayerId                         (MSYS_UINT_MAX)
    , m_uiDependencyId                    (0)
    , m_uiFrameWidthInSamples             (352)
    , m_uiFrameHeightInSamples            (288)
    , m_dInputFrameRate                   (7.5)
    , m_dOutputFrameRate                  (7.5)
    , m_cInputFilename                    ("none")
    , m_cOutputFilename                   ("none")
    , m_uiEntropyCodingModeFlag           (1)
    , m_uiEnable8x8Trafo                  (0)
    , m_uiScalingMatricesPresent          (0)
    , m_uiMaxAbsDeltaQP                   (1)
    , m_dBaseQpResidual                   (26.0)
    , m_uiInterLayerPredictionMode        (0)
    , m_uiILPredMode                      (MSYS_UINT_MAX)
    , m_uiILPredMotion                    (MSYS_UINT_MAX)
    , m_uiILPredResidual                  (MSYS_UINT_MAX)
    , m_uiDecompositionStages             (0)
    , m_uiNotCodedStages                  (0)
    , m_uiTemporalResolution              (0)
    , m_uiFrameDelay                      (0)
    , m_iChromaQPIndexOffset              ( 0 )
    , m_i2ndChromaQPIndexOffset           ( 0 )
    , m_uiConstrainedIntraPred            ( 0 )
    , m_uiForceReorderingCommands         (0)
    , m_uiBaseLayerId                     (MSYS_UINT_MAX)
    , m_uiMbAff                           ( 0 )
    , m_uiPAff                            ( 0 )
    , m_uiUseRedundantSlice               (0)   //JVT-Q054 Red. Picture
		, m_uiUseRedundantKeySlice            (0)   //JVT-W049
// JVT-Q065 EIDR{
	  , m_iIDRPeriod						  (0)
  // JVT-Q065 EIDR}
    , m_iLayerIntraPeriod ( 0 )
    , m_uiMGSVectorMode                      ( 0 )
    , m_dQpModeDecisionLP ( 0.00 )
    , m_uiNumSliceGroupMapUnitsMinus1 ( 0 )
    // JVT-S054 (ADD) ->
    , m_uiNumSliceMinus1 (0)
    , m_bSliceDivisionFlag (false)
    //, m_uiSliceDivisionType (0)//SEI changes update
		, m_bGridFlag (0)//SEI changes update
    , m_puiGridSliceWidthInMbsMinus1 (0)
    , m_puiGridSliceHeightInMbsMinus1 (0)
    , m_puiFirstMbInSlice (0)
    , m_puiLastMbInSlice (0)
    , m_puiSliceId (0)
    // JVT-S054 (ADD) <-
    , m_uiSliceGroupIdArraySize(0)
    , m_pauiSliceGroupId(0)
    , m_bAVCRewriteFlag                  ( 0 )   // JVT-V035
    , m_bAVCAdaptiveRewriteFlag          ( 0 )   // JVT-V035
    , m_uiSliceSkip                      ( 0 )
    , m_uiSliceSkipTLevelStart           ( 0 )
    , m_uiLowComplexMbEnable             ( 0 )   // JVT-V079

	//S051{
	, m_cOutSIPFileName					("none")
	, m_cInSIPFileName					("none")
	, m_uiAnaSIP						(0)
	, m_bEncSIP							(false)
	//S051}
//JVT-T054{
    , m_uiLayerCGSSNR         ( 0 )
    , m_uiQualityLevelCGSSNR  ( 0 )
    , m_uiBaseLayerCGSSNR     ( 0 )
    , m_uiBaseQualityLevelCGSSNR ( 0 )
//DS_FIX_FT_09_2007
    , m_bDiscardable          ( true )
//~DS_FIX_FT_09_2007
//JVT-T054}
    , m_uiExplicitQPCascading ( 0 )
    , m_uiIPCMRate            ( 0 )
    , m_uiBiPred8x8Disable    ( 0 )
    , m_uiMCBlks8x8Disable    ( 0 )
    , m_uiBotFieldFirst       ( 0 )
    , m_uiUseLongTerm         ( 0 )
    , m_uiPicCodingType       ( 0 )
    , m_uiNumDependentDId     ( 0 )
    , m_uiProfileIdc          ( 0 )
    , m_uiLevelIdc            ( 0 )
    , m_bIntraOnly            ( false )
    , m_bConstrainedSetFlag0  ( false )
    , m_bConstrainedSetFlag1  ( false )
    , m_bConstrainedSetFlag2  ( false )
    , m_bConstrainedSetFlag3  ( false )
    , m_uiMMCOBaseEnable      ( 0 )  //JVT-S036 lsj
    , m_uiMMCOEnable          ( 0 )
  {
    for( UInt ui = 0; ui < MAX_DSTAGES; ui++ ) m_adQpModeDecision[ui] = 0.00;
    ::memset( m_uiMGSVect, 0x00, 16*sizeof(UInt) );

    for( UInt uiTTL = 0; uiTTL < MAX_TEMP_LEVELS; uiTTL++ )
    {
      m_adDeltaQPTLevel[uiTTL] = 0.0;
    }
    for( UInt uiSM = 0; uiSM < 8; uiSM++ )
    {
      ::memset( &(m_aaucScalingMatrices[uiSM][0]), 0x00, 64*sizeof(UChar) );
    }
  }

  virtual ~LayerParameters()
  {
    // JVT-S054 (ADD) ->
    if (m_puiGridSliceWidthInMbsMinus1 != NULL)
    {
      free(m_puiGridSliceWidthInMbsMinus1);
      m_puiGridSliceWidthInMbsMinus1 = NULL;
    }
    if (m_puiGridSliceHeightInMbsMinus1 != NULL)
    {
      free(m_puiGridSliceHeightInMbsMinus1);
      m_puiGridSliceHeightInMbsMinus1 = NULL;
    }
    if (m_puiFirstMbInSlice != NULL)
    {
      free(m_puiFirstMbInSlice);
      m_puiFirstMbInSlice = NULL;
    }
    if (m_puiLastMbInSlice != NULL)
    {
      free(m_puiLastMbInSlice);
      m_puiLastMbInSlice = NULL;
    }
    if (m_puiSliceId != NULL)
    {
      free(m_puiSliceId);
      m_puiSliceId = NULL;
    }
    // JVT-S054 (ADD) <-
    delete [] m_pauiSliceGroupId;
  }

  ErrVal  setAndCheckProfile( CodingParameter* pcCodingParameter );
  ErrVal  updateWithLevel   ( CodingParameter* pcCodingParameter, UInt& ruiLevelIdc );

  //===== get =====
  UInt                            getLayerId                        () const {return m_uiLayerId; }
  UInt                            getDependencyId                   () const {return m_uiDependencyId; }
  Bool                            isInterlaced                      () const {return ( m_uiMbAff != 0 || m_uiPAff != 0 ); }
  UInt                            getFrameWidthInSamples            () const {return m_uiFrameWidthInSamples; }
  UInt                            getFrameHeightInSamples           () const {return m_uiFrameHeightInSamples; }
  UInt                            getFrameWidthInMbs                () const {return ( m_uiFrameWidthInSamples + 15 ) >> 4; }
  UInt                            getFrameHeightInMbs               () const {return ( isInterlaced() ? ( ( m_uiFrameHeightInSamples + 31 ) >> 5 ) << 1 : ( m_uiFrameHeightInSamples + 15 ) >> 4 ); }
  UInt                            getFrameHeightInMapUnits          () const {return ( isInterlaced() ?   ( m_uiFrameHeightInSamples + 31 ) >> 5        : ( m_uiFrameHeightInSamples + 15 ) >> 4 ); }
  UInt                            getHorPadding                     () const {return 16*getFrameWidthInMbs () - getFrameWidthInSamples (); }
  UInt                            getVerPadding                     () const {return 16*getFrameHeightInMbs() - getFrameHeightInSamples(); }
  Double                          getInputFrameRate                 () const {return m_dInputFrameRate; }
  Double                          getOutputFrameRate                () const {return m_dOutputFrameRate; }
  const std::string&              getInputFilename                  () const {return m_cInputFilename; }
  const std::string&              getOutputFilename                 () const {return m_cOutputFilename; }
  Bool                            getEntropyCodingModeFlag          () const {return m_uiEntropyCodingModeFlag == 1; }
  UInt                            getEnable8x8Trafo                 () const {return m_uiEnable8x8Trafo; }
  UInt                            getScalingMatricesPresent         () const {return m_uiScalingMatricesPresent; }
  UInt                            getMaxAbsDeltaQP                  () const {return m_uiMaxAbsDeltaQP; }
  Double                          getBaseQpResidual                 () const {return m_dBaseQpResidual; }
  Double                          getQpModeDecision          (UInt ui) const {return m_adQpModeDecision[ui]; }
  Double                          getQpModeDecisionLP               () const {return m_dQpModeDecisionLP; }
  UInt                            getInterLayerPredictionMode       () const {return m_uiInterLayerPredictionMode; }
  UInt                            getILPredMode                     () const {return m_uiILPredMode; }
  UInt                            getILPredMotion                   () const {return m_uiILPredMotion; }
  UInt                            getILPredResidual                 () const {return m_uiILPredResidual; }
  Int                             getChromaQPIndexOffset            () const { return m_iChromaQPIndexOffset; }
  Int                             get2ndChromaQPIndexOffset         () const { return m_i2ndChromaQPIndexOffset; }

//JVT-V079 Low-complexity MB mode decision {
  UInt                            getLowComplexMbEnable             () const   { return m_uiLowComplexMbEnable; }
//JVT-V079 Low-complexity MB mode decision }

  UInt                            getDecompositionStages            () const {return m_uiDecompositionStages; }
  UInt                            getNotCodedStages                 () const {return m_uiNotCodedStages    ; }
  UInt                            getTemporalResolution             () const {return m_uiTemporalResolution; }
  UInt                            getFrameDelay                     () const {return m_uiFrameDelay; }

  UInt                            getConstrainedIntraPred           () const {return m_uiConstrainedIntraPred; }
  UInt                            getForceReorderingCommands        () const {return m_uiForceReorderingCommands; }
  UInt                            getBaseLayerId                    () const {return m_uiBaseLayerId; }
  UInt                            getMbAff                          () const {return m_uiMbAff;}
  UInt                            getPAff                           () const {return m_uiPAff;}
  Bool                            getUseRedundantSliceFlag          () const {return m_uiUseRedundantSlice == 1; }  //JVT-Q054 Red. Picture
  Bool                            getUseRedundantKeySliceFlag       () const {return m_uiUseRedundantKeySlice == 1; }   //JVT-W049
  //--ICU/ETRI FMO Implementation :  FMO start
  UInt          getNumSliceGroupsMinus1() const {return m_uiNumSliceGroupsMinus1;}  //for test
  UInt          getSliceGroupMapType() const {return  m_uiSliceGroupMapType;  }
  Bool          getSliceGroupChangeDirection_flag () const {return m_bSliceGroupChangeDirection_flag;}
  UInt          getSliceGroupChangeRateMinus1 () const {return m_uiSliceGroupChangeRateMinus1;}
  UInt          getNumSliceGroupMapUnitsMinus1() const {return m_uiNumSliceGroupMapUnitsMinus1;}
  UInt          getSliceGroupId(Int i) const { AOF(i<(Int)m_uiSliceGroupIdArraySize); return m_pauiSliceGroupId[i];}
  UInt          getSliceMode() const {return m_uiSliceMode;}
  UInt          getSliceArgument() const { return m_uiSliceArgument ;}
  const std::string&   getSliceGroupConfigFileName() const{ return m_cSliceGroupConfigFileName;}
  UInt          getUseRedundantSlice() const { return m_uiUseRedundantSlice;}
  UInt*         getArrayRunLengthMinus1 () const {return (UInt*)m_uiRunLengthMinus1;}
  UInt*         getArrayTopLeft () const {return (UInt*)m_uiTopLeft;}
  UInt*         getArrayBottomRight () const {return (UInt*)m_uiBottomRight;}
  UInt*         getArraySliceGroupId() const {return m_pauiSliceGroupId;}
  //--ICU/ETRI FMO Implementation : FMO end

  //<-- consider ROI Extraction ICU/ETRI DS
  const std::string&   getROIConfigFileName() const{ return m_cROIConfigFileName;}
  UInt          getNumROI() const {return m_uiNumROI;}  //for test

  UInt*         getROIID () const {return (UInt*)m_uiROIID;}
  UInt*         getSGID () const {return (UInt*)m_uiSGID;}
  UInt*         getSLID () const {return (UInt*)m_uiSLID;}
  //--> consider ROI Extraction ICU/ETRI DS

  UInt  getMMCOBaseEnable ()  const	  { return m_uiMMCOBaseEnable; } //JVT-S036 lsj
  UInt  getMMCOEnable     ()  const   { return m_uiMMCOEnable; }

  UInt getMGSVect                        (UInt uiNum) const { return m_uiMGSVectorMode ? m_uiMGSVect[uiNum] : (uiNum == 0 ? 16 : 0); }
  Bool getTCoeffLevelPredictionFlag ()               const { return m_bAVCRewriteFlag==1; }
  Bool getAVCAdaptiveRewriteFlag ()       const { return m_bAVCAdaptiveRewriteFlag==1; }
  Void setAVCRewrite( UInt ui ) { m_bAVCRewriteFlag = ui; }

  UInt getSliceSkip() const { return m_uiSliceSkip; }
  UInt getSliceSkipTLevelStart()  const { return m_uiSliceSkipTLevelStart; }
  Void setSliceSkip( UInt uiSliceSkip ) { m_uiSliceSkip = uiSliceSkip; }
  Void setSliceSkipTLevelStart( UInt ui )  { m_uiSliceSkipTLevelStart = ui; }

  //===== set =====
  Void setLayerId                         (UInt   p) { m_uiLayerId                        = p; }
  Void setDependencyId                    (UInt   p) { m_uiDependencyId                   = p; }
  Void setFrameWidthInSamples             (UInt   p) { m_uiFrameWidthInSamples            = p; }
  Void setFrameHeightInSamples            (UInt   p) { m_uiFrameHeightInSamples           = p; }
  Void setInputFrameRate                  (Double p) { m_dInputFrameRate                  = p; }
  Void setOutputFrameRate                 (Double p) { m_dOutputFrameRate                 = p; }
  Void setInputFilename                   (Char*  p) { m_cInputFilename                   = p; }
  Void setOutputFilename                  (Char*  p) { m_cOutputFilename                  = p; }
  Void setEntropyCodingModeFlag           (Bool   p) { m_uiEntropyCodingModeFlag          = p; }
  Void setEnable8x8Trafo                  (UInt   p) { m_uiEnable8x8Trafo                 = p; }
  Void setScalingMatricesPresent          (UInt   p) { m_uiScalingMatricesPresent         = p; }
  Void setMaxAbsDeltaQP                   (UInt   p) { m_uiMaxAbsDeltaQP                  = p; }
  Void setBaseQpResidual                  (Double p) { m_dBaseQpResidual                  = p; }
  Void setQpModeDecision                  (UInt   n,
                                           Double p) { m_adQpModeDecision             [n] = p; }
  Void setQpModeDecisionLP                (Double p) { m_dQpModeDecisionLP                = p; }
  Void setInterLayerPredictionMode        (UInt   p) { m_uiInterLayerPredictionMode       = p; }
  Void setILPredMode                      (UInt   p) { m_uiILPredMode                     = p; }
  Void setILPredMotion                    (UInt   p) { m_uiILPredMotion                   = p; }
  Void setILPredResidual                  (UInt   p) { m_uiILPredResidual                 = p; }

  Void setDecompositionStages             (UInt   p) { m_uiDecompositionStages            = p; }
  Void setNotCodedStages                  (UInt   p) { m_uiNotCodedStages                 = p; }
  Void setTemporalResolution              (UInt   p) { m_uiTemporalResolution             = p; }
  Void setFrameDelay                      (UInt   p) { m_uiFrameDelay                     = p; }

  Void setChromaQPIndexOffset             (Int    p) { m_iChromaQPIndexOffset             = p; }
  Void set2ndChromaQPIndexOffset          (Int    p) { m_i2ndChromaQPIndexOffset          = p; }
  Void setConstrainedIntraPred            (UInt   p) { m_uiConstrainedIntraPred           = p; }
  Void setForceReorderingCommands         (UInt   p) { m_uiForceReorderingCommands        = p; }
  Void setBaseLayerId                     (UInt   p) { m_uiBaseLayerId                    = p; }
  Void setMbAff                           (UInt   p) { m_uiMbAff                          = p; }
  Void setPAff                            (UInt   p) { m_uiPAff                           = p; }

  ResizeParameters&   getResizeParameters ()  { return m_cResizeParameters; }
  const std::string&  getESSFilename      ()  { return m_cESSFilename; }

// JVT-Q065 EIDR{
  Int				  getIDRPeriod			   () { return m_iIDRPeriod; }
// JVT-Q065 EIDR}
  Int         getLayerIntraPeriod  () { return m_iLayerIntraPeriod; }

  UInt                getPLR                   () { return m_uiPLR; } //JVT-R057 LA-RDO
  Void                setPLR                   (UInt ui){m_uiPLR=ui;} //JVT-W049
  //===== check =====
  ErrVal  check();

//--ICU/ETRI FMO Implementation
  Void  setSliceGroupId(int i, UInt value) { AOF(i<(Int)m_uiSliceGroupIdArraySize); m_pauiSliceGroupId[i] = value;}
  Void  initSliceGroupIdArray( UInt uiNumMapUnits )
  {
    if( m_uiSliceGroupIdArraySize < uiNumMapUnits )
    {
      delete [] m_pauiSliceGroupId;
      m_uiSliceGroupIdArraySize     = uiNumMapUnits;
      m_pauiSliceGroupId            = new UInt [m_uiSliceGroupIdArraySize];
    }
    m_uiNumSliceGroupMapUnitsMinus1 = uiNumMapUnits - 1;
  }

  Void                            setUseRedundantSliceFlag(Bool   b) { m_uiUseRedundantSlice = b; }  // JVT-Q054 Red. Picture
  Void                            setUseRedundantKeySliceFlag(UInt   b) { m_uiUseRedundantKeySlice = b; }  //JVT-W049
  //S051{
  const std::string&              getInSIPFileName             () const { return m_cInSIPFileName; }
  const std::string&              getOutSIPFileName            () const { return m_cOutSIPFileName; }
  Void							  setInSIPFileName			   (Char* p) { m_cInSIPFileName=p; }
  Void							  setOutSIPFileName			   (Char* p) { m_cOutSIPFileName=p; }
  Void							  setAnaSIP					   (UInt	uiAnaSIP){ m_uiAnaSIP = uiAnaSIP;}
  Void						      setEncSIP					   (Bool	bEncSIP){ m_bEncSIP = bEncSIP;}
  UInt							  getAnaSIP					   (){ return m_uiAnaSIP; }
  Bool							  getEncSIP					   (){ return m_bEncSIP; }
  //S051}
//JVT-T054{
  UInt getLayerCGSSNR                    ()    { return m_uiLayerCGSSNR;}
  UInt getQualityLevelCGSSNR             ()    { return m_uiQualityLevelCGSSNR;}
  UInt getNumberOfQualityLevelsCGSSNR    ()   const;
  UInt getBaseLayerCGSSNR                    ()    { return m_uiBaseLayerCGSSNR;}
  UInt getBaseQualityLevelCGSSNR             ()    { return m_uiBaseQualityLevelCGSSNR;}
  Void setLayerCGSSNR                    (UInt ui)    { m_uiLayerCGSSNR                   = ui;}
  Void setQualityLevelCGSSNR             (UInt ui)    { m_uiQualityLevelCGSSNR            = ui;}
  Void setBaseLayerCGSSNR                    (UInt ui)    { m_uiBaseLayerCGSSNR                   = ui;}
  Void setBaseQualityLevelCGSSNR             (UInt ui)    { m_uiBaseQualityLevelCGSSNR            = ui;}
//DS_FIX_FT_09_2007
  Void setNonDiscardable                  ()       { m_bDiscardable = false;}
//~DS_FIX_FT_09_2007
  Void setDiscardable                     ()       { m_bDiscardable = true;}  //zhangxd_20101220
  Bool isDiscardable                      ()          { return m_bDiscardable;}
//JVT-T054}

  UInt    getExplicitQPCascading  ()               const   { return m_uiExplicitQPCascading; }
  Double  getDeltaQPTLevel        ( UInt    ui )   const   { return m_adDeltaQPTLevel[ui]; }

  Void    setExplicitQPCascading  ( UInt    ui )           { m_uiExplicitQPCascading = ui; }
  Void    setDeltaQPTLevel        ( UInt    tl,
                                    Double  d  )           { m_adDeltaQPTLevel[tl] = d; }

  Void    setIPCMRate( UInt ui ) { m_uiIPCMRate = ui; }
  UInt    getIPCMRate() const    { return m_uiIPCMRate; }
  const UChar* getScalMatrixBuffer() const { return m_aaucScalingMatrices[0]; }
  UInt  getBiPred8x8Disable() const     { return m_uiBiPred8x8Disable; }
  Void  setBiPred8x8Disable( UInt ui )  { m_uiBiPred8x8Disable = ui; }
  UInt  getMCBlks8x8Disable() const     { return m_uiMCBlks8x8Disable; }
  Void  setMCBlks8x8Disable( UInt ui )  { m_uiMCBlks8x8Disable = ui; }

  UInt  getBotFieldFirst()    const     { return m_uiBotFieldFirst; }
  Void  setBotFieldFirst( UInt ui )     { m_uiBotFieldFirst = ui; }
  UInt  getPicCodingType()    const     { return m_uiPicCodingType; }
  Void  setPicCodingType( UInt ui )     { m_uiPicCodingType = ui; }

  Void  setProfileIdc( UInt ui )          { m_uiProfileIdc = ui; }
  UInt  getProfileIdc         ()  const   { return m_uiProfileIdc; }
  UInt  getLevelIdc           ()  const   { return m_uiLevelIdc; }
  Bool  isIntraOnly           ()  const   { return m_bIntraOnly; }
  Bool  getConstrainedSet0Flag()  const   { return m_bConstrainedSetFlag0; }
  Bool  getConstrainedSet1Flag()  const   { return m_bConstrainedSetFlag1; }
  Bool  getConstrainedSet2Flag()  const   { return m_bConstrainedSetFlag2; }
  Bool  getConstrainedSet3Flag()  const   { return m_bConstrainedSetFlag3; }

  Void  setUseLongTerm  ( UInt ui )       { m_uiUseLongTerm = ui; }
  UInt  getUseLongTerm  ()          const { return m_uiUseLongTerm; }

  protected:
    Bool    xIsBaselineProfile            ( CodingParameter*  pcCodingParameter );
    ErrVal  xForceBaselineProfile         ( CodingParameter*  pcCodingParameter );
    Bool    xIsMainProfile                ( CodingParameter*  pcCodingParameter );
    ErrVal  xForceMainProfile             ( CodingParameter*  pcCodingParameter );
    Bool    xIsExtendedProfile            ( CodingParameter*  pcCodingParameter );
    ErrVal  xForceExtendedProfile         ( CodingParameter*  pcCodingParameter );
    Bool    xIsHighProfile                ( CodingParameter*  pcCodingParameter );
    ErrVal  xForceHighProfile             ( CodingParameter*  pcCodingParameter );
    Bool    xIsIntraOnly                  ( CodingParameter*  pcCodingParameter );

    Bool    xHasRestrictedESS             ( CodingParameter*  pcCodingParameter );
    Bool    xIsScalableBaselineProfile    ( CodingParameter*  pcCodingParameter );
    ErrVal  xForceScalableBaselineProfile ( CodingParameter*  pcCodingParameter );
    Bool    xIsScalableHighProfile        ( CodingParameter*  pcCodingParameter );
    ErrVal  xForceScalableHighProfile     ( CodingParameter*  pcCodingParameter );

public:
  UInt                      m_uiLayerId;
  UInt                      m_uiDependencyId;
  UInt                      m_uiFrameWidthInSamples;
  UInt                      m_uiFrameHeightInSamples;
  Double                    m_dInputFrameRate;
  Double                    m_dOutputFrameRate;
  std::string               m_cInputFilename;
  std::string               m_cOutputFilename;

  UInt                      m_uiEntropyCodingModeFlag;
  UInt                      m_uiEnable8x8Trafo;
  UInt                      m_uiScalingMatricesPresent;

  UInt                      m_uiMaxAbsDeltaQP;
  Double                    m_dBaseQpResidual;

  Double                    m_adQpModeDecision[MAX_DSTAGES];
  Double                    m_dQpModeDecisionLP;
  UInt                      m_uiInterLayerPredictionMode;
  UInt                      m_uiILPredMode;
  UInt                      m_uiILPredMotion;
  UInt                      m_uiILPredResidual;
  UInt                      m_uiConstrainedIntraPred;
  UInt                      m_uiForceReorderingCommands;
  UInt                      m_uiBaseLayerId;
  Int                       m_iChromaQPIndexOffset;
  Int                       m_i2ndChromaQPIndexOffset;

  //JVT-V079 Low-complexity MB mode decision {
  Int                     m_uiLowComplexMbEnable;
  //JVT-V079 Low-complexity MB mode decision }

  //----- derived parameters -----
  UInt                      m_uiDecompositionStages;
  UInt                      m_uiNotCodedStages    ;
  UInt                      m_uiTemporalResolution;
  UInt                      m_uiFrameDelay;

  //----- ESS ----
  ResizeParameters          m_cResizeParameters;
  std::string               m_cESSFilename;

  UInt                      m_uiMbAff;
  UInt                      m_uiPAff;

  //--ICU/ETRI FMO Implementation : FMO start
  UInt         m_uiNumSliceGroupsMinus1;
  UInt         m_uiSliceGroupMapType;
  UInt         m_uiRunLengthMinus1[CodParMAXNumSliceGroupsMinus1];
  UInt         m_uiTopLeft[CodParMAXNumSliceGroupsMinus1];
  UInt         m_uiBottomRight[CodParMAXNumSliceGroupsMinus1];
  Bool         m_bSliceGroupChangeDirection_flag;
  UInt         m_uiSliceGroupChangeRateMinus1;
  UInt         m_uiNumSliceGroupMapUnitsMinus1;
  UInt         m_uiSliceGroupIdArraySize;
  UInt*        m_pauiSliceGroupId;
  UInt         m_uiSliceMode;
  UInt         m_uiSliceArgument;
  std::string  m_cSliceGroupConfigFileName;

  std::string  m_cROIConfigFileName;
  UInt		   m_uiNumROI;
  UInt		   m_uiROIID[CodParMAXNumSliceGroupsMinus1];
  UInt		   m_uiSGID[CodParMAXNumSliceGroupsMinus1];
  UInt		   m_uiSLID[CodParMAXNumSliceGroupsMinus1];

  //--ICU/ETRI FMO Implementation : FMO end
  UInt         m_uiUseRedundantSlice;   // JVT-Q054 Red. Picture
  UInt         m_uiUseRedundantKeySlice;   //JVT-W049
  // JVT-S054 (ADD) ->
  Bool         m_bSliceDivisionFlag;
  UInt         m_uiNumSliceMinus1;
  //UInt         m_uiSliceDivisionType;//SEI changes update
	Bool         m_bGridFlag;//SEI changes update
  UInt*        m_puiGridSliceWidthInMbsMinus1;
  UInt*        m_puiGridSliceHeightInMbsMinus1;
  UInt*        m_puiFirstMbInSlice;
  UInt*        m_puiLastMbInSlice;
  UInt*        m_puiSliceId;
  // JVT-S054 (ADD) <-

  // JVT-V035
  UInt         m_bAVCRewriteFlag;
  UInt         m_bAVCAdaptiveRewriteFlag;

  UInt    m_uiSliceSkip;
  UInt    m_uiSliceSkipTLevelStart;

// JVT-Q065 EIDR{
  Int						m_iIDRPeriod;
// JVT-Q065 EIDR}
  Int       m_iLayerIntraPeriod;

  UInt               m_uiPLR; //JVT-R057 LA-RDO
  UInt       m_uiMGSVectorMode;
  UInt       m_uiMGSVect[16];

  //S051{
  std::string    m_cOutSIPFileName;
  std::string	 m_cInSIPFileName;
  UInt			 m_uiAnaSIP;
  Bool			 m_bEncSIP;
  //S051}
//JVT-T054{
  UInt                      m_uiLayerCGSSNR;
  UInt                      m_uiQualityLevelCGSSNR;
  UInt                      m_uiBaseLayerCGSSNR;
  UInt                      m_uiBaseQualityLevelCGSSNR;
  Bool                      m_bDiscardable;
//JVT-T054}

  UInt    m_uiExplicitQPCascading;
  Double  m_adDeltaQPTLevel[MAX_TEMP_LEVELS];

  UInt    m_uiIPCMRate;
  std::string m_acScalMatFiles      [8];
  UChar       m_aaucScalingMatrices [8][64];
  UInt  m_uiBiPred8x8Disable;
  UInt  m_uiMCBlks8x8Disable;
  UInt  m_uiBotFieldFirst;
  UInt  m_uiUseLongTerm;
  UInt  m_uiPicCodingType;
  UInt  m_uiNumDependentDId;
  UInt  m_uiProfileIdc;
  UInt  m_uiLevelIdc;
  Bool  m_bIntraOnly;
  Bool  m_bConstrainedSetFlag0;
  Bool  m_bConstrainedSetFlag1;
  Bool  m_bConstrainedSetFlag2;
  Bool  m_bConstrainedSetFlag3;

  UInt	m_uiMMCOBaseEnable;  //JVT-S036 lsj
  UInt  m_uiMMCOEnable;
};


//TMM_WP
class H264AVCENCODERLIB_API SampleWeightingParams
{
  public:
    SampleWeightingParams() : m_uiIPMode(0), m_uiBMode(0), m_uiLumaDenom(5), m_uiChromaDenom(5), m_fDiscardThr(1) { }
        ErrVal check() const ;
        ErrVal checkForValidChanges( const SampleWeightingParams& rcSW )const;

        Bool operator == ( const SampleWeightingParams& rcSWP ) const ;
        Bool operator != ( const SampleWeightingParams& rcSWP ) const { return !((*this) == rcSWP); }
        ErrVal writeBinary( BinDataAccessor& rcBinDataAccessor )  const;
        ErrVal readBinary( BinDataAccessor& rcBinDataAccessor );

        UInt getIPMode()                  const { return m_uiIPMode; }
        UInt getBMode()                   const { return m_uiBMode; }
        UInt getLumaDenom()               const { return m_uiLumaDenom; }
        UInt getChromaDenom()             const { return m_uiChromaDenom; }
        Float getDiscardThr()             const { return m_fDiscardThr; }
        Void setIPMode( UInt uiIPMode )         { m_uiIPMode      = uiIPMode; }
        Void setBMode( UInt uiBMode )           { m_uiBMode       = uiBMode; }
        Void setLumaDenom( UInt uiDenom )       { m_uiLumaDenom   = uiDenom; }
        Void setChromaDenom( UInt uiDenom )     { m_uiChromaDenom = uiDenom; }
        Void setDiscardThr( Float fDiscardThr ) { m_fDiscardThr = fDiscardThr; }

  protected:
        UInt m_uiIPMode;      // 0 off, 1 on, 2 random
        UInt m_uiBMode;       // 0 off, 1 explicit, 2 implicit, 3 random
        UInt m_uiLumaDenom;   // 0-7
        UInt m_uiChromaDenom; // 0-7
        Float m_fDiscardThr;
};
//TMM_WP



class H264AVCENCODERLIB_API CodingParameter
{
  friend class LayerParameters;

public:
  CodingParameter()
    : m_dMaximumFrameRate                 ( 0.0 )
    , m_dMaximumDelay                     ( 1e6 )
    , m_uiTotalFrames                     ( 0 )
    , m_uiGOPSize                         ( 0 )
    , m_uiDecompositionStages             ( 0 )
    , m_uiIntraPeriod                     ( 0 )
    , m_uiIntraPeriodLowPass              ( 0 )
    , m_uiNumRefFrames                    ( 0 )
    , m_uiBaseLayerMode                   ( 0 )
    , m_uiNumberOfLayers                  ( 0 )
    , m_uiAVCmode                         ( 0 )
    , m_uiFrameWidth                      ( 0 )
    , m_uiFrameHeight                     ( 0 )
    , m_uiSymbolMode                      ( 0 )
    , m_uiEnable8x8Trafo                  ( 0 )
    , m_uiConstrainedIntraPred            ( 0 )
    , m_uiScalingMatricesPresent          ( 0 )
    , m_dBasisQp                          ( 0 )
    , m_uiDPBSize                         ( 0 )
    , m_uiNumDPBRefFrames                 ( 0 )
    , m_uiLog2MaxFrameNum                 ( 0 )
    , m_uiLog2MaxPocLsb                   ( 0 )
    , m_cSequenceFormatString             ()
    , m_uiMaxRefIdxActiveBL0              ( 0 )
    , m_uiMaxRefIdxActiveBL1              ( 0 )
    , m_uiMaxRefIdxActiveP                ( 0 )
//TMM_WP
      , m_uiIPMode                        ( 0 )
      , m_uiBMode                         ( 0 )
//TMM_WP
	  , m_bNonRequiredEnable		      ( 0 ) //NonRequired JVT-Q066
	  , m_uiLARDOEnable                   ( 0 )      //JVT-R057 LA-RDO
		, m_uiEssRPChkEnable									( 0 )
		, m_uiMVThres													( 20 )
	  , m_uiPreAndSuffixUnitEnable		      ( 0 )  //JVT-S036 lsj
//JVT-T073 {
	  , m_uiNestingSEIEnable              ( 0 )
	  , m_uiSceneInfoEnable               ( 0 )
//JVT-T073 }
		, m_uiIntegrityCheckSEIEnable       ( 0 )//JVT-W052 wxwan
//JVT-T054{
    , m_uiCGSSNRRefinementFlag            ( 0 )
//JVT-T054}
// JVT-U085 LMI
    , m_uiTlevelNestingFlag               ( 1 )
// JVT-U116 W062 LMI
    , m_uiTl0DepRepIdxSeiEnable           ( 0 )
    , m_uiCIUFlag                         ( 0 ) //JV
    , m_uiEncodeKeyPictures               ( 0 )
    , m_uiMGSKeyPictureControl            ( 0 )
// JVT-W043 {
    , m_uiRCMinQP                         ( 12 )
    , m_uiRCMaxQP                         ( 40 )
    , m_uiMaxQpChange                     ( 2 )
    , m_uiInitialQp                       ( 30 )
    , m_uiBasicUnit                       ( 99 )
    , m_uiBitRate                         ( 64000 )
    , m_uiRateControlEnable               ( 0 )
    , m_uiAdaptInitialQP                  ( 0 )
// JVT-W043 }
    , m_uiBiPred8x8Disable                ( 0 )
    , m_uiMCBlks8x8Disable                ( 0 )
	// JVT-AD021 {
	, m_uiMultiLayerLambda				  ( 0 )
	// JVT-AD021 }
  {
    for( UInt uiLayerId = 0; uiLayerId < MAX_LAYERS; uiLayerId++ )
    {
      m_acLayerParameters[uiLayerId].setLayerId( uiLayerId );
    }
    for( UInt uiLayer = 0; uiLayer < 6; uiLayer++ )
    {
      m_adDeltaQpLayer[uiLayer] = 0;
    }
  }
	virtual ~CodingParameter()
  {
  }

public:
  const MotionVectorSearchParams& getMotionVectorSearchParams       () const {return m_cMotionVectorSearchParams; }
  const LoopFilterParams&         getLoopFilterParams               () const {return m_cLoopFilterParams; }
  const LoopFilterParams&         getInterLayerLoopFilterParams     () const {return m_cInterLayerLoopFilterParams; }

  MotionVectorSearchParams&       getMotionVectorSearchParams       ()       {return m_cMotionVectorSearchParams; }
  LoopFilterParams&               getLoopFilterParams               ()       {return m_cLoopFilterParams; }
  LoopFilterParams&               getInterLayerLoopFilterParams     ()       {return m_cInterLayerLoopFilterParams; }

  const LayerParameters&          getLayerParameters  ( UInt    n )   const   { return m_acLayerParameters[n]; }
  LayerParameters&                getLayerParameters  ( UInt    n )           { return m_acLayerParameters[n]; }

//TMM_WP
  SampleWeightingParams&           getSampleWeightingParams(UInt uiLayerId)  {return m_cSampleWeightingParams[uiLayerId];}
//TMM_WP

  const std::string&              getInputFile            ()              const   { return m_cInputFile; }
  Double                          getMaximumFrameRate     ()              const   { return m_dMaximumFrameRate; }
  Double                          getMaximumDelay         ()              const   { return m_dMaximumDelay; }
  UInt                            getTotalFrames          ()              const   { return m_uiTotalFrames; }
  UInt                            getGOPSize              ()              const   { return m_uiGOPSize; }
  UInt                            getDecompositionStages  ()              const   { return m_uiDecompositionStages; }
  UInt                            getIntraPeriod          ()              const   { return m_uiIntraPeriod; }
  UInt                            getIntraPeriodLowPass   ()              const   { return m_uiIntraPeriodLowPass; }
  UInt                            getNumRefFrames         ()              const   { return m_uiNumRefFrames; }
  UInt                            getBaseLayerMode        ()              const   { return m_uiBaseLayerMode; }
  UInt                            getNumberOfLayers       ()              const   { return m_uiNumberOfLayers; }
/*  Void                            getSimplePriorityMap    ( UInt uiSimplePri, UInt& uiTemporalLevel, UInt& uiLayer, UInt& uiQualityLevel )
                                                                          { uiTemporalLevel = m_uiTemporalLevelList[uiSimplePri];
                                                                            uiLayer         = m_uiDependencyIdList [uiSimplePri];
                                                                            uiQualityLevel  = m_uiQualityLevelList [uiSimplePri];
                                                                          }
 JVT-S036 lsj */
//TMM_WP
  UInt getIPMode()                  const { return m_uiIPMode; }
  UInt getBMode()                   const { return m_uiBMode; }
//TMM_WP

  UInt                            getAVCmode                ()              const   { return m_uiAVCmode; }
  UInt                            getFrameWidth             ()              const   { return m_uiFrameWidth; }
  UInt                            getFrameHeight            ()              const   { return m_uiFrameHeight; }
  UInt                            getSymbolMode             ()              const   { return m_uiSymbolMode; }
  UInt                            getEnable8x8Trafo         ()              const   { return m_uiEnable8x8Trafo; }
  UInt                            getScalingMatricesPresent ()              const   { return m_uiScalingMatricesPresent; }
  Double                          getBasisQp                ()              const   { return m_dBasisQp; }
  UInt                            getDPBSize                ()              const   { return m_uiDPBSize; }
  UInt                            getNumDPBRefFrames        ()              const   { return m_uiNumDPBRefFrames; }
  UInt                            getLog2MaxFrameNum        ()              const   { return m_uiLog2MaxFrameNum; }
  UInt                            getLog2MaxPocLsb          ()              const   { return m_uiLog2MaxPocLsb; }
  std::string                     getSequenceFormatString   ()              const   { return m_cSequenceFormatString; }
  Double                          getDeltaQpLayer           ( UInt ui )     const   { return m_adDeltaQpLayer[ui]; }
  UInt                            getMaxRefIdxActiveBL0     ()              const   { return m_uiMaxRefIdxActiveBL0; }
  UInt                            getMaxRefIdxActiveBL1     ()              const   { return m_uiMaxRefIdxActiveBL1; }
  UInt                            getMaxRefIdxActiveP       ()              const   { return m_uiMaxRefIdxActiveP; }

  Void                            setInputFile              ( Char*   p )   { m_cInputFile            = p; }

  UInt                            getLARDOEnable            ()              const   { return m_uiLARDOEnable;} //JVT-R057 LA-RDO
  UInt														getEssRPChkEnable				  ()							const		{	return m_uiEssRPChkEnable;}
  UInt														getMVThres							  ()							const		{	return m_uiMVThres;}

  UInt							              getPreAndSuffixUnitEnable	()		          const	  { return m_uiPreAndSuffixUnitEnable;} //prefix unit
  // JVT-T073 {
  UInt                            getNestingSEIEnable       ()              const   { return m_uiNestingSEIEnable; }
  UInt                            getSceneInfoEnable        ()              const   { return m_uiSceneInfoEnable; }
  // JVT-T073 }

	UInt														getIntegrityCheckSEIEnable()						const   { return m_uiIntegrityCheckSEIEnable; }//JVT-W052
  Void                            setIntegrityCheckSEIEnable( UInt  ui )  { m_uiIntegrityCheckSEIEnable = ui;}           //JVT-W052 bug_fixed
  Void                            setMaximumFrameRate     ( Double  d )   { m_dMaximumFrameRate     = d; }
  Void                            setMaximumDelay         ( Double  d )   { m_dMaximumDelay         = d; }
  Void                            setTotalFrames          ( UInt    n )   { m_uiTotalFrames         = n; }
  Void                            setGOPSize              ( UInt    n )   { m_uiGOPSize             = n; }
  Void                            setDecompositionStages  ( UInt    n )   { m_uiDecompositionStages = n; }
  Void                            setIntraPeriod          ( UInt    n )   { m_uiIntraPeriod         = n; }
  Void                            setIntraPeriodLowPass   ( UInt    n )   { m_uiIntraPeriodLowPass  = n; }
  Void                            setNumRefFrames         ( UInt    n )   { m_uiNumRefFrames        = n; }
  Void                            setBaseLayerMode        ( UInt    n )   { m_uiBaseLayerMode       = n; }
  Void                            setNumberOfLayers       ( UInt    n )   { m_uiNumberOfLayers      = n; }
 /* Void                            setSimplePriorityMap ( UInt uiSimplePri, UInt uiTemporalLevel, UInt uiLayer, UInt uiQualityLevel )
                                                                          { m_uiTemporalLevelList[uiSimplePri] = uiTemporalLevel;
                                                                            m_uiDependencyIdList [uiSimplePri] = uiLayer;
                                                                            m_uiQualityLevelList [uiSimplePri] = uiQualityLevel;
                                                                          }
 JVT-S036 lsj */
  Void                            setFrameWidth             ( UInt    p )   { m_uiFrameWidth              = p; }
  Void                            setFrameHeight            ( UInt    p )   { m_uiFrameHeight             = p; }
  Void                            setSymbolMode             ( UInt    p )   { m_uiSymbolMode              = p; }
  Void                            setEnable8x8Trafo         ( UInt    p )   { m_uiEnable8x8Trafo          = p; }
  Void                            setScalingMatricesPresent ( UInt    p )   { m_uiScalingMatricesPresent  = p; }
  Void                            setBasisQp                ( Double  p )   { m_dBasisQp                  = p; }
  Void                            setDPBSize                ( UInt    p )   { m_uiDPBSize                 = p; }
  Void                            setNumDPBRefFrames        ( UInt    p )   { m_uiNumDPBRefFrames         = p; }
  Void                            setLog2MaxFrameNum        ( UInt    p )   { m_uiLog2MaxFrameNum         = p; }
  Void                            setLog2MaxPocLsb          ( UInt    p )   { m_uiLog2MaxPocLsb           = p; }
  Void                            setSequenceFormatString   ( Char*   p )   { m_cSequenceFormatString     = p; }
  Void                            setDeltaQpLayer           ( UInt    n,
                                                              Double  p )   { m_adDeltaQpLayer[n]         = p; }
  Void                            setMaxRefIdxActiveBL0     ( UInt    p )   { m_uiMaxRefIdxActiveBL0      = p; }
  Void                            setMaxRefIdxActiveBL1     ( UInt    p )   { m_uiMaxRefIdxActiveBL1      = p; }
  Void                            setMaxRefIdxActiveP       ( UInt    p )   { m_uiMaxRefIdxActiveP        = p; }

  ErrVal                          check                     ();

  Int					              		  getNonRequiredEnable      ()			{ return m_bNonRequiredEnable; }  //NonRequired JVT-Q066 (06-04-08)

//JVT-T054{
  UInt                            getCGSSNRRefinement       ()              const   { return m_uiCGSSNRRefinementFlag;}
  Void                            setCGSSNRRefinement       ( UInt    b )   { m_uiCGSSNRRefinementFlag = b; }
//JVT-T054}
// JVT-U085 LMI {
  Bool                            getTlevelNestingFlag      ()              const   { return m_uiTlevelNestingFlag > 0 ? true : false; }
  Void                            setTlevelNestingFlag      ( UInt  ui )    { m_uiTlevelNestingFlag = ui; }
// JVT-U085 LMI }
// JVT-U116 W062 LMI {
  Bool                            getTl0DepRepIdxSeiEnable  ()              const   { return m_uiTl0DepRepIdxSeiEnable > 0 ? true : false; }
  Void                            setTl0DepRepIdxSeiEnable  ( UInt  ui )    { m_uiTl0DepRepIdxSeiEnable = ui; }
// JVT-U116 W062 LMI }

  //JVT-U106 Behaviour at slice boundaries{
  void   setCIUFlag(UInt  flag)
  {
	  m_uiCIUFlag=flag;
  }
  UInt   getCIUFlag()
  {
	  return m_uiCIUFlag;
  }
  //JVT-U106 Behaviour at slice boundaries}

  Void  setConstrainedIntraPred   ( UInt ui )          { m_uiConstrainedIntraPred = ui; }
  UInt  getConstrainedIntraPred   ()           const   { return m_uiConstrainedIntraPred; }

  Void  setEncodeKeyPictures      ( UInt ui )          { m_uiEncodeKeyPictures = ui; }
  UInt  getEncodeKeyPictures      ()           const   { return m_uiEncodeKeyPictures; }

  Void  setMGSKeyPictureControl   ( UInt ui )          { m_uiMGSKeyPictureControl = ui; }
  UInt  getMGSKeyPictureControl   ()           const   { return m_uiMGSKeyPictureControl; }

  // JVT-V068 HRD {
  UInt getEnableVUI()    { return getEnableNalHRD() || getEnableVclHRD(); }
  Bool getEnableNalHRD() { return m_uiNalHRD == 0 ? false: true; }
  Bool getEnableVclHRD() { return m_uiVclHRD == 0 ? false: true; }
  UInt getEnableVUITimingInfo() { return getEnableVUI(); }
  UInt getEnableSEIBufferingPeriod() { return getEnableNalHRD() || getEnableVclHRD(); }
  UInt getEnableSEIPicTiming() { return getEnableNalHRD() || getEnableVclHRD(); }
  // JVT-V068 HRD }
  //JVT-W049 {
  Bool getEnableRedundantKeyPic() {return m_uiRedundantKeyPic == 0 ? false:true; }
  //JVT-W049 }
  UInt  getBiPred8x8Disable       ()  const           { return m_uiBiPred8x8Disable; }
  Void  setBiPred8x8Disable       ( UInt ui )         { m_uiBiPred8x8Disable = ui; }
  UInt  getMCBlks8x8Disable       () const            { return m_uiMCBlks8x8Disable; }
  Void  setMCBlks8x8Disable       ( UInt ui )         { m_uiMCBlks8x8Disable = ui; }

  // JVT-AD021 {
  Void  setMultiLayerLambda		  ( UInt ui )		  { m_uiMultiLayerLambda = ui; }
  UInt  getMultiLayerLambda		  ()				  { return m_uiMultiLayerLambda; }
  // JVT-AD021 }

private:
  UInt    getLogFactor( Double  r0, Double  r1 );

  ErrVal  xCheckAndSetProfiles();


protected:
  std::string               m_cInputFile;
  Double                    m_dMaximumFrameRate;
  Double                    m_dMaximumDelay;
  UInt                      m_uiTotalFrames;

  UInt                      m_uiGOPSize;
  UInt                      m_uiDecompositionStages;
  UInt                      m_uiIntraPeriod;
  UInt                      m_uiIntraPeriodLowPass;
  UInt                      m_uiNumRefFrames;
  UInt                      m_uiBaseLayerMode;

  MotionVectorSearchParams  m_cMotionVectorSearchParams;
  LoopFilterParams          m_cLoopFilterParams;
  LoopFilterParams          m_cInterLayerLoopFilterParams;

  UInt                      m_uiNumberOfLayers;
  LayerParameters           m_acLayerParameters[MAX_LAYERS];

  EncoderConfigLineBase*    m_pEncoderLines[MAX_CONFIG_PARAMS];
  EncoderConfigLineBase*    m_pLayerLines  [MAX_CONFIG_PARAMS];

  UInt                      m_uiAVCmode;
  UInt                      m_uiFrameWidth;
  UInt                      m_uiFrameHeight;
  UInt                      m_uiSymbolMode;
  UInt                      m_uiEnable8x8Trafo;
  UInt                      m_uiConstrainedIntraPred;
  UInt                      m_uiScalingMatricesPresent;
  Double                    m_dBasisQp;
  UInt                      m_uiDPBSize;
  UInt                      m_uiNumDPBRefFrames;
  UInt                      m_uiLog2MaxFrameNum;
  UInt                      m_uiLog2MaxPocLsb;
  std::string               m_cSequenceFormatString;
  Double                    m_adDeltaQpLayer[6];
  UInt                      m_uiMaxRefIdxActiveBL0;
  UInt                      m_uiMaxRefIdxActiveBL1;
  UInt                      m_uiMaxRefIdxActiveP;

//TMM_WP
  UInt m_uiIPMode;
  UInt m_uiBMode;

  SampleWeightingParams m_cSampleWeightingParams[MAX_LAYERS];
//TMM_WP

  Int						m_bNonRequiredEnable; //NonRequired JVT-Q066
  UInt                       m_uiLARDOEnable; //JVT-R057 LA-RDO

	UInt						m_uiEssRPChkEnable;
	UInt						m_uiMVThres;

  UInt						m_uiPreAndSuffixUnitEnable; //JVT-S036 lsj

//JVT-T054{
  UInt                      m_uiCGSSNRRefinementFlag;
//JVT-T054}
//JVT-T073 {
  UInt                      m_uiNestingSEIEnable;
  UInt                      m_uiSceneInfoEnable;
//JVT-T073 }

	//JVT-W052
	UInt											m_uiIntegrityCheckSEIEnable;
	//JVT-W052

// JVT-U085 LMI
  UInt                      m_uiTlevelNestingFlag;
// JVT-U116,W062 LMI
  //UInt                      m_uiTl0PicIdxPresentFlag;
  UInt                      m_uiTl0DepRepIdxSeiEnable;
  //JVT-U106 Behaviour at slice boundaries{
  UInt                      m_uiCIUFlag;
  //JVT-U106 Behaviour at slice boundaries}

  UInt    m_uiEncodeKeyPictures;  // 0:only FGS[default], 1:FGS&MGS, 2:always[useless]
  UInt    m_uiMGSKeyPictureControl;

  // JVT-V068 HRD {
  UInt    m_uiNalHRD;
  UInt    m_uiVclHRD;
  // JVT-V068 HRD }
	//JVT-W049 {
  UInt    m_uiRedundantKeyPic;
  //JVT-W049 }
  UInt    m_uiBiPred8x8Disable;
  UInt    m_uiMCBlks8x8Disable;

  // JVT-AD021 {
  UInt	  m_uiMultiLayerLambda;
  // JVT-AD021 }

  // JVT-W043
  public:
    UInt                      m_uiRCMinQP;
    UInt                      m_uiRCMaxQP;
    UInt                      m_uiMaxQpChange;
    UInt                      m_uiInitialQp;
    UInt                      m_uiBasicUnit;
    UInt                      m_uiBitRate;
    UInt                      m_uiRateControlEnable;
    UInt                      m_uiAdaptInitialQP;
};

#if defined( MSYS_WIN32 )
# pragma warning( default: 4275 )
#endif


H264AVC_NAMESPACE_END


#endif // !defined(AFX_CODINGPARAMETER_H__8403A680_A65D_466E_A411_05C3A7C0D59F__INCLUDED_)
