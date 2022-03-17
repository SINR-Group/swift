
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/SequenceParameterSet.h"
#include "H264AVCCommonLib/TraceFile.h"
#include "ResizeParameters.h"
#include <cmath>


H264AVC_NAMESPACE_BEGIN


const SequenceParameterSet::LevelLimit SequenceParameterSet::m_aLevelLimit[52] =
{
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //0
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //1
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //2
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //3
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //4
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //5
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //6
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //7
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //8
  { 1,   1485,    99,    297*1024,    128,    350,  256, 2, 64 },                 //9 (1b)
  { 1,   1485,    99,    297*1024,     64,    175,  256, 2, 64 },                 //10
  { 1,   3000,   396,    675*1024,    192,    500,  512, 2, 64 },                 //11
  { 1,   6000,   396,   1782*1024,    384,   1000,  512, 2, 64 },                 //12
  { 1,  11880,   396,   1782*1024,    768,   2000,  512, 2, 64 },                 //13
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //14
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //15
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //16
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //17
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //18
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //19
  { 1,  11880,   396,   1782*1024,   2000,   2000,  512, 2, 64 },                 //20
  { 1,  19800,   792,   3564*1024,   4000,   4000, 1024, 2, 64 },                 //21
  { 1,  20250,  1620,   6075*1024,   4000,   4000, 1024, 2, 64 },                 //22
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //23
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //24
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //25
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //26
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //27
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //28
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //29
  { 1,  40500,  1620,   6075*1024,  10000,  10000, 1024, 2, 32 },                 //30
  { 1, 108000,  3600,  13500*1024,  14000,  14000, 2048, 4, 16 },                 //31
  { 1, 216000,  5120,  15360*1024,  20000,  20000, 2048, 4, 16 },                 //32
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //33
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //34
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //35
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //36
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //37
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //38
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //39
  { 1, 245760,  8192,  24576*1024,  20000,  25000, 2048, 4, 16 },                 //40
  { 1, 245760,  8192,  24576*1024,  50000,  62500, 2048, 2, 16 },                 //41
  { 1, 491520,  8192,  24576*1024,  50000,  62500, 2048, 2, 16 },                 //42
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //43
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //44
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //45
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //46
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //47
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //48
  { 0,      0,     0,           0,      0,      0,    0, 0,  0 },                 //49
  { 1, 589824, 22080,  82620*1024, 135000, 135000, 2048, 2, 16 },                 //50
  { 1, 983040, 36864, 138240*1024, 240000, 240000, 2048, 2, 16 }                  //51
};





SequenceParameterSet::SequenceParameterSet  ()
: m_eNalUnitType                            ( NAL_UNIT_UNSPECIFIED_0 )
, m_uiDependencyId                          ( 0 )
, m_eProfileIdc                             ( SCALABLE_HIGH_PROFILE )
, m_bConstrainedSet0Flag                    ( false )
, m_bConstrainedSet1Flag                    ( false )
, m_bConstrainedSet2Flag                    ( false )
, m_bConstrainedSet3Flag                    ( false )
, m_uiLevelIdc                              ( 0 )
, m_uiSeqParameterSetId                     ( MSYS_UINT_MAX )
, m_uiChromaFormatIdc                       ( 1 ) //JVT-W046
, m_bSeparateColourPlaneFlag                ( false )
, m_uiBitDepthLumaMinus8                    ( 0 )
, m_uiBitDepthChromaMinus8                  ( 0 )
, m_bTransformBypassFlag                    ( false )
, m_bSeqScalingMatrixPresentFlag            ( false )
, m_uiLog2MaxFrameNum                       ( 0 )
, m_uiPicOrderCntType                       ( 0 )
, m_uiLog2MaxPicOrderCntLsb                 ( 4 )
, m_bDeltaPicOrderAlwaysZeroFlag            ( false )
, m_iOffsetForNonRefPic                     ( 0 )
, m_iOffsetForTopToBottomField              ( 0 )
, m_uiNumRefFramesInPicOrderCntCycle        ( 0 )
, m_uiNumRefFrames                          ( 0 )
, m_bGapsInFrameNumValueAllowedFlag         ( false )
, m_uiFrameWidthInMbs                       ( 0 )
, m_uiFrameHeightInMbs                      ( 0 )
, m_bDirect8x8InferenceFlag                 ( false )
, m_uiExtendedSpatialScalability            ( ESS_NONE ) // TMM_ESS
, m_uiChromaPhaseXPlus1                     ( 0 ) // TMM_ESS
, m_uiChromaPhaseYPlus1                     ( 1 )// TMM_ESS
, m_uiBaseChromaPhaseXPlus1                 ( 0 ) //JVT-W046
, m_uiBaseChromaPhaseYPlus1                 ( 1 ) //JVT-W046
, m_bInterlayerDeblockingPresent            ( 0 )
, m_bFrameMbsOnlyFlag                       ( true )
, m_bMbAdaptiveFrameFieldFlag               ( false )
, m_bAVCRewriteFlag                         ( false )   // V035
, m_bAVCAdaptiveRewriteFlag                 ( false )
, m_bAVCHeaderRewriteFlag                   ( false ) //JVT-W046
//SSPS {
, m_bSVCVUIParametersPresentFlag            ( false )
, m_bAdditionalExtension2Flag               ( false )
//SSPS }
, m_uiFrameCropLeftOffset                   ( 0 )
, m_uiFrameCropRightOffset                  ( 0 )
, m_uiFrameCropTopOffset                    ( 0 )
, m_uiFrameCropBottomOffset                 ( 0 )
{
  m_auiNumRefIdxUpdateActiveDefault[LIST_0]=1;// VW
  m_auiNumRefIdxUpdateActiveDefault[LIST_1]=1;// VW
  ::memset( m_uiMGSVect,           0x00, 16*sizeof(UInt) );
  m_uiMGSVect[0] = 16;
}

SequenceParameterSet::~SequenceParameterSet()
{
}



ErrVal
SequenceParameterSet::create( SequenceParameterSet*& rpcSPS )
{
  rpcSPS = new SequenceParameterSet;
  ROT( NULL == rpcSPS);
  return Err::m_nOK;
}


ErrVal
SequenceParameterSet::destroy()
{
  // JVT-V068 {
  if (m_pcVUI) delete m_pcVUI;
  // JVT-V068 }

  delete this;
  return Err::m_nOK;
}

SequenceParameterSet& SequenceParameterSet::operator = ( const SequenceParameterSet& rcSPS )
{
  m_eNalUnitType                      = rcSPS.m_eNalUnitType;
  m_uiDependencyId                    = rcSPS.m_uiDependencyId;
  m_eProfileIdc                       = rcSPS.m_eProfileIdc;
  m_bConstrainedSet0Flag              = rcSPS.m_bConstrainedSet0Flag;
  m_bConstrainedSet1Flag              = rcSPS.m_bConstrainedSet1Flag;
  m_bConstrainedSet2Flag              = rcSPS.m_bConstrainedSet2Flag;
  m_bConstrainedSet3Flag              = rcSPS.m_bConstrainedSet3Flag;
  m_uiLevelIdc                        = rcSPS.m_uiLevelIdc;
  m_uiSeqParameterSetId               = rcSPS.m_uiSeqParameterSetId;
  m_uiLog2MaxFrameNum                 = rcSPS.m_uiLog2MaxFrameNum;
  m_uiPicOrderCntType                 = rcSPS.m_uiPicOrderCntType;
  m_uiLog2MaxPicOrderCntLsb           = rcSPS.m_uiLog2MaxPicOrderCntLsb;
  m_bDeltaPicOrderAlwaysZeroFlag      = rcSPS.m_bDeltaPicOrderAlwaysZeroFlag;
  m_iOffsetForNonRefPic               = rcSPS.m_iOffsetForNonRefPic;
  m_iOffsetForTopToBottomField        = rcSPS.m_iOffsetForTopToBottomField;
  m_uiNumRefFramesInPicOrderCntCycle  = rcSPS.m_uiNumRefFramesInPicOrderCntCycle;
  m_piOffsetForRefFrame               = rcSPS.m_piOffsetForRefFrame;
  m_uiNumRefFrames                    = rcSPS.m_uiNumRefFrames;
  m_bGapsInFrameNumValueAllowedFlag   = rcSPS.m_bGapsInFrameNumValueAllowedFlag;
  m_uiFrameWidthInMbs                 = rcSPS.m_uiFrameWidthInMbs;
  m_uiFrameHeightInMbs                = rcSPS.m_uiFrameHeightInMbs;
  m_bFrameMbsOnlyFlag                 = rcSPS.m_bFrameMbsOnlyFlag;
  m_bMbAdaptiveFrameFieldFlag         = rcSPS.m_bMbAdaptiveFrameFieldFlag;
  m_bDirect8x8InferenceFlag           = rcSPS.m_bDirect8x8InferenceFlag;
  m_bSeqScalingMatrixPresentFlag      = rcSPS.m_bSeqScalingMatrixPresentFlag;
  m_uiExtendedSpatialScalability      = rcSPS.m_uiExtendedSpatialScalability;
  m_uiChromaPhaseXPlus1               = rcSPS.m_uiChromaPhaseXPlus1;
  m_uiChromaPhaseYPlus1               = rcSPS.m_uiChromaPhaseYPlus1;
  m_uiBaseChromaPhaseXPlus1           = rcSPS.m_uiBaseChromaPhaseXPlus1;
  m_uiBaseChromaPhaseYPlus1           = rcSPS.m_uiBaseChromaPhaseYPlus1;
  m_iScaledBaseLeftOffset             = rcSPS.m_iScaledBaseLeftOffset;
  m_iScaledBaseTopOffset              = rcSPS.m_iScaledBaseTopOffset;
  m_iScaledBaseRightOffset            = rcSPS.m_iScaledBaseRightOffset;
  m_iScaledBaseBottomOffset           = rcSPS.m_iScaledBaseBottomOffset;
  return *this;
}



Bool
SequenceParameterSet::doesFulfillMGSConstraint( const SequenceParameterSet& rcSPS )  const
{
  ROFRS( m_uiChromaFormatIdc                  == rcSPS.m_uiChromaFormatIdc,                 false );
  ROFRS( m_bSeparateColourPlaneFlag           == rcSPS.m_bSeparateColourPlaneFlag,          false );
  ROFRS( m_uiBitDepthLumaMinus8               == rcSPS.m_uiBitDepthLumaMinus8,              false );
  ROFRS( m_uiBitDepthChromaMinus8             == rcSPS.m_uiBitDepthChromaMinus8,            false );
  ROFRS( m_bTransformBypassFlag               == rcSPS.m_bTransformBypassFlag,              false );
  ROFRS( m_bSeqScalingMatrixPresentFlag       == rcSPS.m_bSeqScalingMatrixPresentFlag,      false );
  if( m_bSeqScalingMatrixPresentFlag )
  {
    ROFRS( m_cSeqScalingMatrix          .isSame( rcSPS.m_cSeqScalingMatrix ),               false );

  }
  ROFRS( m_uiLog2MaxFrameNum                  ==  rcSPS.m_uiLog2MaxFrameNum,                false );
  ROFRS( m_uiPicOrderCntType                  ==  rcSPS.m_uiPicOrderCntType,                false );
  if( m_uiPicOrderCntType == 0 )
  {
    ROFRS( m_uiLog2MaxPicOrderCntLsb          ==  rcSPS.m_uiLog2MaxPicOrderCntLsb,          false );
  }
  else if( m_uiPicOrderCntType == 1 )
  {
    ROFRS( m_bDeltaPicOrderAlwaysZeroFlag     ==  rcSPS.m_bDeltaPicOrderAlwaysZeroFlag,     false );
    ROFRS( m_iOffsetForNonRefPic              ==  rcSPS.m_iOffsetForNonRefPic,              false );
    ROFRS( m_iOffsetForTopToBottomField       ==  rcSPS.m_iOffsetForTopToBottomField,       false );
    ROFRS( m_uiNumRefFramesInPicOrderCntCycle ==  rcSPS.m_uiNumRefFramesInPicOrderCntCycle, false );
    for( UInt ui = 0; ui < m_uiNumRefFramesInPicOrderCntCycle; ui++ )
    {
      ROFRS( m_piOffsetForRefFrame[ ui ]      ==  rcSPS.m_piOffsetForRefFrame[ ui ],        false );
    }
  }
  ROFRS( m_uiNumRefFrames                     ==  rcSPS.m_uiNumRefFrames,                   false );
  ROFRS( m_bGapsInFrameNumValueAllowedFlag    ==  rcSPS.m_bGapsInFrameNumValueAllowedFlag,  false );
  ROFRS( m_uiFrameWidthInMbs                  ==  rcSPS.m_uiFrameWidthInMbs,                false );
  ROFRS( m_uiFrameHeightInMbs                 ==  rcSPS.m_uiFrameHeightInMbs,               false );
  ROFRS( m_bFrameMbsOnlyFlag                  ==  rcSPS.m_bFrameMbsOnlyFlag,                false );
  if( !m_bFrameMbsOnlyFlag )
  {
    ROFRS( m_bMbAdaptiveFrameFieldFlag        ==  rcSPS.m_bMbAdaptiveFrameFieldFlag,        false );
  }
  ROFRS( m_bDirect8x8InferenceFlag            ==  rcSPS.m_bDirect8x8InferenceFlag,          false );
  ROFRS( m_uiFrameCropLeftOffset              ==  rcSPS.m_uiFrameCropLeftOffset,            false );
  ROFRS( m_uiFrameCropRightOffset             ==  rcSPS.m_uiFrameCropRightOffset,           false );
  ROFRS( m_uiFrameCropTopOffset               ==  rcSPS.m_uiFrameCropTopOffset,             false );
  ROFRS( m_uiFrameCropBottomOffset            ==  rcSPS.m_uiFrameCropBottomOffset,          false );

  AOF  ( m_pcVUI );
  AOF  ( rcSPS.m_pcVUI );
  ROFRS( m_pcVUI->isSameExceptHRDParametersAndSVCExt( *rcSPS.m_pcVUI ),                     false );

  if( isSubSetSPS() && rcSPS.isSubSetSPS() )
  {
    ROFRS( m_bInterlayerDeblockingPresent     ==  rcSPS.m_bInterlayerDeblockingPresent,     false );
    ROFRS( m_uiExtendedSpatialScalability     ==  rcSPS.m_uiExtendedSpatialScalability,     false );
    if( m_uiChromaFormatIdc == 1 || m_uiChromaFormatIdc == 2 )
    {
      ROFRS( m_uiChromaPhaseXPlus1            ==  rcSPS.m_uiChromaPhaseXPlus1,              false );
    }
    if( m_uiChromaFormatIdc == 1 )
    {
      ROFRS( m_uiChromaPhaseYPlus1            ==  rcSPS.m_uiChromaPhaseYPlus1,              false );
    }
    if( m_uiExtendedSpatialScalability == 1 )
    {
      if( m_uiChromaFormatIdc > 0 )
      {
        ROFRS( m_uiBaseChromaPhaseXPlus1      ==  rcSPS.m_uiBaseChromaPhaseXPlus1,          false );
        ROFRS( m_uiBaseChromaPhaseYPlus1      ==  rcSPS.m_uiBaseChromaPhaseYPlus1,          false );
      }
      ROFRS( m_iScaledBaseLeftOffset          ==  rcSPS.m_iScaledBaseLeftOffset,            false );
      ROFRS( m_iScaledBaseTopOffset           ==  rcSPS.m_iScaledBaseTopOffset,             false );
      ROFRS( m_iScaledBaseRightOffset         ==  rcSPS.m_iScaledBaseRightOffset,           false );
      ROFRS( m_iScaledBaseBottomOffset        ==  rcSPS.m_iScaledBaseBottomOffset,          false );
    }
    ROFRS( m_bAVCRewriteFlag                  ==  rcSPS.m_bAVCRewriteFlag,                  false );
    if( m_bAVCRewriteFlag )
    {
      ROFRS( m_bAVCAdaptiveRewriteFlag        ==  rcSPS.m_bAVCAdaptiveRewriteFlag,          false );
    }
    ROFRS( m_bAVCHeaderRewriteFlag            ==  rcSPS.m_bAVCHeaderRewriteFlag,            false );
  }
  return true;
}

ErrVal
SequenceParameterSet::copySPSDataForMGSEnhancement( const SequenceParameterSet& rcSPS, UInt CurrQId ) //zhangxd_20101220
{
  m_uiChromaFormatIdc                 = rcSPS.m_uiChromaFormatIdc;
  m_bSeparateColourPlaneFlag          = rcSPS.m_bSeparateColourPlaneFlag;
  m_uiBitDepthLumaMinus8              = rcSPS.m_uiBitDepthLumaMinus8;
  m_uiBitDepthChromaMinus8            = rcSPS.m_uiBitDepthChromaMinus8;
  m_bTransformBypassFlag              = rcSPS.m_bTransformBypassFlag;
  m_bSeqScalingMatrixPresentFlag      = rcSPS.m_bSeqScalingMatrixPresentFlag;
  m_cSeqScalingMatrix                 = rcSPS.m_cSeqScalingMatrix;
  m_uiLog2MaxFrameNum                 = rcSPS.m_uiLog2MaxFrameNum;
  m_uiPicOrderCntType                 = rcSPS.m_uiPicOrderCntType;
  m_uiLog2MaxPicOrderCntLsb           = rcSPS.m_uiLog2MaxPicOrderCntLsb;
  m_bDeltaPicOrderAlwaysZeroFlag      = rcSPS.m_bDeltaPicOrderAlwaysZeroFlag;
  m_iOffsetForNonRefPic               = rcSPS.m_iOffsetForNonRefPic;
  m_iOffsetForTopToBottomField        = rcSPS.m_iOffsetForTopToBottomField;
  m_uiNumRefFramesInPicOrderCntCycle  = rcSPS.m_uiNumRefFramesInPicOrderCntCycle;
  m_piOffsetForRefFrame               = rcSPS.m_piOffsetForRefFrame;
  m_uiNumRefFrames                    = rcSPS.m_uiNumRefFrames;
  m_bGapsInFrameNumValueAllowedFlag   = rcSPS.m_bGapsInFrameNumValueAllowedFlag;
  m_uiFrameWidthInMbs                 = rcSPS.m_uiFrameWidthInMbs;
  m_uiFrameHeightInMbs                = rcSPS.m_uiFrameHeightInMbs;
  m_bFrameMbsOnlyFlag                 = rcSPS.m_bFrameMbsOnlyFlag;
  m_bMbAdaptiveFrameFieldFlag         = rcSPS.m_bMbAdaptiveFrameFieldFlag;
  m_bDirect8x8InferenceFlag           = rcSPS.m_bDirect8x8InferenceFlag;
  m_uiFrameCropLeftOffset             = rcSPS.m_uiFrameCropLeftOffset;
  m_uiFrameCropRightOffset            = rcSPS.m_uiFrameCropRightOffset;
  m_uiFrameCropTopOffset              = rcSPS.m_uiFrameCropTopOffset;
  m_uiFrameCropBottomOffset           = rcSPS.m_uiFrameCropBottomOffset;
  ROF ( m_pcVUI );
  ROF ( rcSPS.m_pcVUI );
  RNOK( m_pcVUI->copyExceptHRDParametersAndSVCExt( *rcSPS.m_pcVUI ) );
  if( rcSPS.isSubSetSPS() && CurrQId != 1 ) //zhangxd_20101220
  {
    m_bInterlayerDeblockingPresent    = rcSPS.m_bInterlayerDeblockingPresent;
    m_uiExtendedSpatialScalability    = rcSPS.m_uiExtendedSpatialScalability;
    m_uiChromaPhaseXPlus1             = rcSPS.m_uiChromaPhaseXPlus1;
    m_uiChromaPhaseYPlus1             = rcSPS.m_uiChromaPhaseYPlus1;
    m_uiBaseChromaPhaseXPlus1         = rcSPS.m_uiBaseChromaPhaseXPlus1;
    m_uiBaseChromaPhaseYPlus1         = rcSPS.m_uiBaseChromaPhaseYPlus1;
    m_iScaledBaseLeftOffset           = rcSPS.m_iScaledBaseLeftOffset;
    m_iScaledBaseTopOffset            = rcSPS.m_iScaledBaseTopOffset;
    m_iScaledBaseRightOffset          = rcSPS.m_iScaledBaseRightOffset;
    m_iScaledBaseBottomOffset         = rcSPS.m_iScaledBaseBottomOffset;
    m_bAVCRewriteFlag                 = rcSPS.m_bAVCRewriteFlag;
    m_bAVCAdaptiveRewriteFlag         = rcSPS.m_bAVCAdaptiveRewriteFlag;
    m_bAVCHeaderRewriteFlag           = rcSPS.m_bAVCHeaderRewriteFlag;
  }
  return Err::m_nOK;
}


UInt
SequenceParameterSet::getMaxDPBSize() const
{
  const LevelLimit* pcLevelLimit = 0;
  UInt              uiFrameSize = 384*getMbInFrame();
  ANOK( xGetLevelLimit( pcLevelLimit, getConvertedLevelIdc() ) );
  UInt uiNumDPBEntries  = pcLevelLimit->uiMaxDPBSizeX2 / ( 2*uiFrameSize );
  uiNumDPBEntries       = gMin( uiNumDPBEntries, 16 );
  return uiNumDPBEntries;
}

// JVT-V068 HRD {
Void SequenceParameterSet::setVUI(SequenceParameterSet* pcSPS)
{
  m_pcVUI = new VUI(pcSPS);
}

UInt
SequenceParameterSet::getMaxCPBSize() const
{
  const LevelLimit* pcLevelLimit = 0;
  ANOK( xGetLevelLimit( pcLevelLimit, getConvertedLevelIdc() ) );
  return pcLevelLimit->uiMaxCPBSize;
}

UInt
SequenceParameterSet::getMaxBitRate() const
{
  const LevelLimit* pcLevelLimit = 0;
  ANOK( xGetLevelLimit( pcLevelLimit, getConvertedLevelIdc() ) );
  return pcLevelLimit->uiMaxBitRate;
}
// JVT-V068 HRD }

UInt
SequenceParameterSet::getMaxSliceSize( Bool bFieldPic ) const
{
  ROTRS( m_uiFrameWidthInMbs * m_uiFrameHeightInMbs <= UInt( bFieldPic ? 3240 : 1620 ), MSYS_UINT_MAX );
  const LevelLimit* pcLevelLimit = 0;
  ANOK( xGetLevelLimit( pcLevelLimit, getConvertedLevelIdc() ) );
  return ( pcLevelLimit->uiMaxFrameSize >> 2 );
}

UInt
SequenceParameterSet::getMaxMVsPer2Mb()  const
{
  const LevelLimit* pcLevelLimit = 0;
  ANOK( xGetLevelLimit( pcLevelLimit, getConvertedLevelIdc() ) );
  return pcLevelLimit->uiMaxMvsPer2Mb;
}

Bool
SequenceParameterSet::getBiPred8x8Disabled()  const
{
  if( m_eProfileIdc == MAIN_PROFILE || m_eProfileIdc == HIGH_PROFILE || ( ( m_eProfileIdc == BASELINE_PROFILE || m_eProfileIdc == EXTENDED_PROFILE ) && m_bConstrainedSet1Flag ) )
  {
    return ( m_uiLevelIdc >= 31 );
  }
  if( m_eProfileIdc == SCALABLE_BASELINE_PROFILE )
  {
    return true;
  }
  if( m_eProfileIdc == SCALABLE_HIGH_PROFILE )
  {
    return ( m_bConstrainedSet0Flag || m_uiLevelIdc >= 31 );
  }
  return false;
}

ErrVal
SequenceParameterSet::xGetLevelLimit( const LevelLimit*& rpcLevelLimit, Int iLevelIdc )
{
  ROT ( iLevelIdc > 51 )
  rpcLevelLimit = &m_aLevelLimit[iLevelIdc];
  ROFS( rpcLevelLimit->bValid )
  return Err::m_nOK;
}

UInt
SequenceParameterSet::getLevelIdc( UInt uiMbY, UInt uiMbX, UInt uiOutFreq, UInt uiMvRange, UInt uiNumRefPic, UInt uiRefLayerMbs )
{
  const UInt  uiLevelOrder[16] = { 10,9,11,12,13,  20,21,22,  30,31,32,  40,41,42,  50,51 };

  UInt        uiFrameSize      = uiMbY * uiMbX;
  UInt        uiMbPerSec       = ( uiFrameSize + ( ( uiRefLayerMbs + 1 ) >> 1 ) ) * uiOutFreq;
  UInt        uiDPBSizeX2      = (uiFrameSize*16*16*3/2) * uiNumRefPic * 2;

  for( Int n = 0; n < 16; n++ )
  {
    UInt              uiLevel = uiLevelOrder[n];
    const LevelLimit* pcLevelLimit;

    if( Err::m_nOK == xGetLevelLimit( pcLevelLimit, uiLevel ) )
    {
      UInt  uiMbPerLine  = (UInt)sqrt( (Double) pcLevelLimit->uiMaxFrameSize * 8 );
      if( ( uiMbPerLine                   >= uiMbX        ) &&
          ( uiMbPerLine                   >= uiMbY        ) &&
          ( pcLevelLimit->uiMaxMbPerSec   >= uiMbPerSec   ) &&
          ( pcLevelLimit->uiMaxFrameSize  >= uiFrameSize  ) &&
          ( pcLevelLimit->uiMaxDPBSizeX2  >= uiDPBSizeX2  ) &&
          ( pcLevelLimit->uiMaxVMvRange   >= uiMvRange    )    )
      {
        return uiLevel;
      }
    }
  }
  return MSYS_UINT_MAX;
}


ErrVal
SequenceParameterSet::write( HeaderSymbolWriteIf* pcWriteIf ) const
{
  //===== NAL unit header =====
  ETRACE_DECLARE( Bool m_bTraceEnable = true );
  ETRACE_LAYER  ( 0 );
  if( m_eNalUnitType == NAL_UNIT_SUBSET_SPS )
  {
    ETRACE_HEADER( "SUBSET SEQUENCE PARAMETER SET" );
  }
  else
  {
    ETRACE_HEADER( "SEQUENCE PARAMETER SET" );
  }
  RNOK  ( pcWriteIf->writeFlag( 0,                                        "NAL unit header: forbidden_zero_bit" ) );
  RNOK  ( pcWriteIf->writeCode( 3, 2,                                     "NAL unit header: nal_ref_idc" ) );
  RNOK  ( pcWriteIf->writeCode( m_eNalUnitType, 5,                        "NAL unit header: nal_unit_type" ) );

  //===== Sequence parameter set =====
  RNOK  ( pcWriteIf->writeCode( getProfileIdc(),                  8,      "SPS: profile_idc" ) );
  RNOK  ( pcWriteIf->writeFlag( m_bConstrainedSet0Flag,                   "SPS: constraint_set0_flag" ) ); //VB-JV 04/08
  RNOK  ( pcWriteIf->writeFlag( m_bConstrainedSet1Flag,                   "SPS: constraint_set1_flag" ) );  //VB-JV 04/08
  RNOK  ( pcWriteIf->writeFlag( m_bConstrainedSet2Flag,                   "SPS: constraint_set2_flag" ) );  //VB-JV 04/08
  RNOK  ( pcWriteIf->writeFlag( m_bConstrainedSet3Flag,                   "SPS: constraint_set3_flag" ) );   //VB-JV 04/08
  RNOK  ( pcWriteIf->writeCode( 0,                                4,      "SPS: reserved_zero_4bits" ) );
  RNOK  ( pcWriteIf->writeCode( getLevelIdc(),                    8,      "SPS: level_idc" ) );
  RNOK  ( pcWriteIf->writeUvlc( getSeqParameterSetId(),                   "SPS: seq_parameter_set_id" ) );

  //--- fidelity range extension syntax ---
  RNOK  ( xWriteFrext( pcWriteIf ) );

  UInt    uiTmp = getLog2MaxFrameNum();
  ROF   ( uiTmp >= 4 );
  RNOK  ( pcWriteIf->writeUvlc( uiTmp - 4,                                "SPS: log2_max_frame_num_minus_4" ) );
  RNOK  ( pcWriteIf->writeUvlc( getPicOrderCntType(),                     "SPS: pic_order_cnt_type" ) );
  if( getPicOrderCntType() == 0 )
  {
    RNOK  ( pcWriteIf->writeUvlc( getLog2MaxPicOrderCntLsb() - 4,         "SPS: log2_max_pic_order_cnt_lsb_minus4" ) );
  }
  else if( getPicOrderCntType() == 1 )
  {
    RNOK( pcWriteIf->writeFlag( getDeltaPicOrderAlwaysZeroFlag(),         "SPS: delta_pic_order_always_zero_flag" ) );
    RNOK( pcWriteIf->writeSvlc( getOffsetForNonRefPic(),                  "SPS: offset_for_non_ref_pic" ) );
    RNOK( pcWriteIf->writeSvlc( getOffsetForTopToBottomField(),           "SPS: offset_for_top_to_bottom_field" ) );
    RNOK( pcWriteIf->writeUvlc( getNumRefFramesInPicOrderCntCycle(),      "SPS: num_ref_frames_in_pic_order_cnt_cycle" ) );
    for( UInt uiIndex = 0; uiIndex < getNumRefFramesInPicOrderCntCycle(); uiIndex++ )
    {
      RNOK( pcWriteIf->writeSvlc( getOffsetForRefFrame( uiIndex ),        "SPS: offset_for_ref_frame" ) );
    }
  }
  RNOK  ( pcWriteIf->writeUvlc( getNumRefFrames(),                        "SPS: num_ref_frames" ) );
  RNOK  ( pcWriteIf->writeFlag( getGapsInFrameNumValueAllowedFlag(),      "SPS: gaps_in_frame_num_value_allowed_flag" ) );

  RNOK  ( pcWriteIf->writeUvlc( getFrameWidthInMbs  () - 1,               "SPS: pic_width_in_mbs_minus1" ) );
  UInt uiHeight = ( getFrameHeightInMbs()-1) >> (1- getFrameMbsOnlyFlag() );

  RNOK  ( pcWriteIf->writeUvlc( uiHeight,                                 "SPS: pic_height_in_map_units_minus1" ) );
  RNOK  ( pcWriteIf->writeFlag( getFrameMbsOnlyFlag(),                    "SPS: frame_mbs_only_flag" ) );

  if( !getFrameMbsOnlyFlag() )
  {
    RNOK( pcWriteIf->writeFlag( getMbAdaptiveFrameFieldFlag(),            "SPS: mb_adaptive_frame_field_flag" ) );
  }


  RNOK  ( pcWriteIf->writeFlag( getDirect8x8InferenceFlag(),              "SPS: direct_8x8_inference_flag" ) );

  if( m_uiFrameCropLeftOffset || m_uiFrameCropRightOffset || m_uiFrameCropTopOffset || m_uiFrameCropBottomOffset )
  {
    RNOK( pcWriteIf->writeFlag( true,                                     "SPS: frame_cropping_flag"      ) );
    RNOK( pcWriteIf->writeUvlc( m_uiFrameCropLeftOffset,                  "SPS: frame_crop_left_offset"   ) );
    RNOK( pcWriteIf->writeUvlc( m_uiFrameCropRightOffset,                 "SPS: frame_crop_right_offset"  ) );
    RNOK( pcWriteIf->writeUvlc( m_uiFrameCropTopOffset,                   "SPS: frame_crop_top_offset"    ) );
    RNOK( pcWriteIf->writeUvlc( m_uiFrameCropBottomOffset,                "SPS: frame_crop_bottom_offset" ) );
  }
  else
  {
    RNOK( pcWriteIf->writeFlag( false,                                    "SPS: frame_cropping_flag" ) );
  }

  RNOK( m_pcVUI->write( pcWriteIf ) );

  ROFRS( m_eNalUnitType == NAL_UNIT_SUBSET_SPS, Err::m_nOK );


  //===== start of subset sequence parameter set extension ======
  if( m_eProfileIdc == SCALABLE_BASELINE_PROFILE || m_eProfileIdc == SCALABLE_HIGH_PROFILE )
  {
    RNOK( pcWriteIf->writeFlag( getInterlayerDeblockingPresent(),       "SPS: inter_layer_deblocking_filter_control_present_flag" ) ); //VB-JV 04/08
    RNOK( pcWriteIf->writeCode( getExtendedSpatialScalability(), 2,     "SPS: extended_spatial_scalability" ) );

    if (getChromaFormatIdc() == 1 || getChromaFormatIdc() == 2 )
    {
      RNOK( pcWriteIf->writeCode( m_uiChromaPhaseXPlus1, 1,             "SPS: chroma_phase_x_plus1_flag" ) ); //VB-JV 04/08
    }
    if (getChromaFormatIdc() == 1 )
    {
      RNOK( pcWriteIf->writeCode( m_uiChromaPhaseYPlus1, 2,             "SPS: chroma_phase_y_plus1" ) );
    }
    if( getExtendedSpatialScalability() == ESS_SEQ )
    {
      if (getChromaFormatIdc() > 0 )
      {
        RNOK( pcWriteIf->writeCode( m_uiBaseChromaPhaseXPlus1, 1,       "SPS: seq_ref_layer_chroma_phase_x_plus1_flag" ) );  //VB-JV 04/08
        RNOK( pcWriteIf->writeCode( m_uiBaseChromaPhaseYPlus1, 2,       "SPS: seq_ref_layer_chroma_phase_y_plus1" ) );       //VB-JV 04/08
      }
      RNOK( pcWriteIf->writeSvlc( m_iScaledBaseLeftOffset,              "SPS: seq_scaled_ref_layer_left_offset" ) );         //VB-JV 04/08
      RNOK( pcWriteIf->writeSvlc( m_iScaledBaseTopOffset,               "SPS: seq_scaled_ref_layer_top_offset" ) );          //VB-JV 04/08
      RNOK( pcWriteIf->writeSvlc( m_iScaledBaseRightOffset,             "SPS: seq_scaled_ref_layer_right_offset" ) );        //VB-JV 04/08
      RNOK( pcWriteIf->writeSvlc( m_iScaledBaseBottomOffset,            "SPS: seq_scaled_ref_layer_bottom_offset" ) );       //VB-JV 04/08
    }
    RNOK( pcWriteIf->writeFlag( m_bAVCRewriteFlag,                      "SPS: seq_tcoeff_level_prediction_flag" ) );
    if( m_bAVCRewriteFlag )
    {
      RNOK( pcWriteIf->writeFlag( m_bAVCAdaptiveRewriteFlag,            "SPS: adaptive_tcoeff_level_prediction_flag" ) );
    }
    RNOK( pcWriteIf->writeFlag( m_bAVCHeaderRewriteFlag,                "SPS: slice_header_restriction_flag" ) );

    //===== SVC VUI extension ====
    RNOK( pcWriteIf->writeFlag( getSVCVUIParametersPresentFlag(),       "SPS: svc_vui_parameters_present_flag" ) );
    if( getSVCVUIParametersPresentFlag() )
    {
      RNOK( m_pcVUI->writeSVCExtension( pcWriteIf ) );
    }
  }

  RNOK( pcWriteIf->writeFlag( getAdditionalExtension2Flag(),            "SPS: additional_extension2_flag" ) );
  ROT ( getAdditionalExtension2Flag() ); // not supported

  return Err::m_nOK;
}


ErrVal
SequenceParameterSet::read( HeaderSymbolReadIf* pcReadIf,
                            NalUnitType         eNalUnitType,
                            Bool&               rbCompletelyParsed )
{
  rbCompletelyParsed = true;

  //===== NAL unit header =====
  setNalUnitType          ( eNalUnitType );
  setAVCHeaderRewriteFlag ( eNalUnitType == NAL_UNIT_SPS ); // for non-SVC SPS
  m_uiExtendedSpatialScalability = ESS_NONE; // for non-SVC SPS

  Bool  bTmp;
  UInt  uiTmp;

  //===== Sequence parameter set =====
  RNOK  ( pcReadIf->getCode( uiTmp,                               8,      "SPS: profile_idc" ) );
  m_eProfileIdc  = Profile ( uiTmp );
  RNOK  ( pcReadIf->getFlag( m_bConstrainedSet0Flag,                      "SPS: constraint_set0_flag" ) );  //VB-JV 04/08
  RNOK  ( pcReadIf->getFlag( m_bConstrainedSet1Flag,                      "SPS: constraint_set1_flag" ) );  //VB-JV 04/08
  RNOK  ( pcReadIf->getFlag( m_bConstrainedSet2Flag,                      "SPS: constraint_set2_flag" ) );  //VB-JV 04/08
  RNOK  ( pcReadIf->getFlag( m_bConstrainedSet3Flag,                      "SPS: constraint_set3_flag" ) );  //VB-JV 04/08
  RNOK  ( pcReadIf->getCode( uiTmp,                               4,      "SPS: reserved_zero_4bits" ) );
  RNOK  ( pcReadIf->getCode( m_uiLevelIdc,                        8,      "SPS: level_idc" ) );
  RNOK  ( pcReadIf->getUvlc( m_uiSeqParameterSetId,                       "SPS: seq_parameter_set_id" ) );

  //--- check profile idc ---
  if( m_eProfileIdc != BASELINE_PROFILE           &&
      m_eProfileIdc != MAIN_PROFILE               &&
      m_eProfileIdc != EXTENDED_PROFILE           &&
      m_eProfileIdc != HIGH_PROFILE               &&
      m_eProfileIdc != HIGH_10_PROFILE            &&
      m_eProfileIdc != HIGH_422_PROFILE           &&
      m_eProfileIdc != HIGH_444_PROFILE           &&
      m_eProfileIdc != CAVLC_444_PROFILE          &&
      m_eProfileIdc != SCALABLE_BASELINE_PROFILE  &&
      m_eProfileIdc != SCALABLE_HIGH_PROFILE        )
  { // unkown profile
    rbCompletelyParsed = false;
    return Err::m_nOK;
  }

  //--- fidelity range extension syntax ---
  RNOK  ( xReadFrext( pcReadIf ) );

  RNOK  ( pcReadIf->getUvlc( uiTmp,                                       "SPS: log2_max_frame_num_minus_4" ) );
  ROT   ( uiTmp > 12 );
  setLog2MaxFrameNum( uiTmp + 4 );
  RNOK  ( xReadPicOrderCntInfo( pcReadIf ) );
  RNOK( pcReadIf->getUvlc( m_uiNumRefFrames,                              "SPS: num_ref_frames" ) );
  RNOK( pcReadIf->getFlag( m_bGapsInFrameNumValueAllowedFlag,             "SPS: gaps_in_frame_num_value_allowed_flag" ) );

  RNOK( pcReadIf->getUvlc( uiTmp,                                         "SPS: pic_width_in_mbs_minus1" ) );
  setFrameWidthInMbs ( 1 + uiTmp );
  RNOK( pcReadIf->getUvlc( uiTmp,                                         "SPS: pic_height_in_map_units_minus1" ) );
  RNOK( pcReadIf->getFlag( m_bFrameMbsOnlyFlag,                           "SPS: frame_mbs_only_flag" ) );
  if( getFrameMbsOnlyFlag() )
  {
    setFrameHeightInMbs( uiTmp+1 );
    setMbAdaptiveFrameFieldFlag( false );
  }
  else
  {
    setFrameHeightInMbs( (uiTmp+1)<<1 );
    RNOK( pcReadIf->getFlag( m_bMbAdaptiveFrameFieldFlag,                 "SPS: mb_adaptive_frame_field_flag"));
  }
  RNOK( pcReadIf->getFlag( m_bDirect8x8InferenceFlag,                     "SPS: direct_8x8_inference_flag" ) );

  Bool  bFrameCroppingFlag;
  RNOK( pcReadIf->getFlag( bFrameCroppingFlag,                            "SPS: frame_cropping_flag"      ) );
  if( bFrameCroppingFlag )
  {
    RNOK( pcReadIf->getUvlc( m_uiFrameCropLeftOffset,                     "SPS: frame_crop_left_offset"   ) );
    RNOK( pcReadIf->getUvlc( m_uiFrameCropRightOffset,                    "SPS: frame_crop_right_offset"  ) );
    RNOK( pcReadIf->getUvlc( m_uiFrameCropTopOffset,                      "SPS: frame_crop_top_offset"    ) );
    RNOK( pcReadIf->getUvlc( m_uiFrameCropBottomOffset,                   "SPS: frame_crop_bottom_offset" ) );
  }
  else
  {
    m_uiFrameCropLeftOffset   = 0;
    m_uiFrameCropRightOffset  = 0;
    m_uiFrameCropTopOffset    = 0;
    m_uiFrameCropBottomOffset = 0;
  }

  RNOK( pcReadIf->getFlag( bTmp,                                          "SPS: vui_parameters_present_flag" ) );
  m_pcVUI = new VUI(this);
  m_pcVUI->setVuiParametersPresentFlag( bTmp );
  if( bTmp )
  {
    RNOK( m_pcVUI->read( pcReadIf ) );
  }

  ROFRS( m_eNalUnitType == NAL_UNIT_SUBSET_SPS, Err::m_nOK );


  //===== start of subset parameter extension =====
  if( m_eProfileIdc == SCALABLE_BASELINE_PROFILE || m_eProfileIdc == SCALABLE_HIGH_PROFILE )
  {
    RNOK( pcReadIf->getFlag( m_bInterlayerDeblockingPresent,              "SPS: inter_layer_deblocking_filter_control_present_flag" ) ); //VB-JV 04/08

    RNOK( pcReadIf->getCode( m_uiExtendedSpatialScalability, 2,           "SPS: extended_spatial_scalability" ) );
    if (getChromaFormatIdc() == 1 || getChromaFormatIdc() == 2 )
    {
      RNOK( pcReadIf->getCode( m_uiChromaPhaseXPlus1, 1,                  "SPS: chroma_phase_x_plus1_flag" ) );  //VB-JV 04/08
      m_uiBaseChromaPhaseXPlus1 = m_uiChromaPhaseXPlus1;
    }
    if (getChromaFormatIdc() == 1 )
    {
      RNOK( pcReadIf->getCode( m_uiChromaPhaseYPlus1, 2,                  "SPS: chroma_phase_y_plus1" ) );
      m_uiBaseChromaPhaseYPlus1 = m_uiChromaPhaseYPlus1;
    }
    if( m_uiExtendedSpatialScalability == ESS_SEQ )
    {
      if( getChromaFormatIdc() > 0 )
      {
        RNOK( pcReadIf->getCode( m_uiBaseChromaPhaseXPlus1, 1,            "SPS: seq_ref_layer_chroma_phase_x_plus1_flag" ) ); //VB-JV 04/08
        RNOK( pcReadIf->getCode( m_uiBaseChromaPhaseYPlus1, 2,            "SPS: seq_ref_layer_chroma_phase_y_plus1" ) );      //VB-JV 04/08
      }
      RNOK( pcReadIf->getSvlc( m_iScaledBaseLeftOffset,                   "SPS: seq_scaled_ref_layer_left_offset" ) );        //VB-JV 04/08
      RNOK( pcReadIf->getSvlc( m_iScaledBaseTopOffset,                    "SPS: seq_scaled_ref_layer_top_offset" ) );		  //VB-JV 04/08
      RNOK( pcReadIf->getSvlc( m_iScaledBaseRightOffset,                  "SPS: seq_scaled_ref_layer_right_offset" ) );		  //VB-JV 04/08
      RNOK( pcReadIf->getSvlc( m_iScaledBaseBottomOffset,                 "SPS: seq_scaled_ref_layer_bottom_offset" ) );	  //VB-JV 04/08
    }
    else
    {
      m_uiBaseChromaPhaseXPlus1 = m_uiChromaPhaseXPlus1;
      m_uiBaseChromaPhaseYPlus1 = m_uiChromaPhaseYPlus1;
      m_iScaledBaseLeftOffset   = 0;
      m_iScaledBaseTopOffset    = 0;
      m_iScaledBaseRightOffset  = 0;
      m_iScaledBaseBottomOffset = 0;
    }
    RNOK( pcReadIf->getFlag( m_bAVCRewriteFlag,                           "SPS: seq_tcoeff_level_prediction_flag" ) );
    if( m_bAVCRewriteFlag )
    {
      RNOK( pcReadIf->getFlag( m_bAVCAdaptiveRewriteFlag,                 "SPS: adaptive_tcoeff_level_prediction_flag" ) );
    }
    RNOK( pcReadIf->getFlag( m_bAVCHeaderRewriteFlag,                     "SPS: slice_header_restriction_flag" ) );

    //===== svc VUI extension =====
    RNOK ( pcReadIf->getFlag( m_bSVCVUIParametersPresentFlag,             "SPS: svc_vui_parameters_present_flag" ) );
    if( m_bSVCVUIParametersPresentFlag )
    {
      RNOK( m_pcVUI->readSVCExtension( pcReadIf ) );
    }
  }

  RNOK( pcReadIf->getFlag ( m_bAdditionalExtension2Flag,                  "SPS: additional_extension2_flag" ) );
  rbCompletelyParsed = !m_bAdditionalExtension2Flag;

  return Err::m_nOK;
}



ErrVal
SequenceParameterSet::xWriteFrext( HeaderSymbolWriteIf* pcWriteIf ) const
{
  ROTRS( m_eProfileIdc != HIGH_PROFILE              &&
         m_eProfileIdc != HIGH_10_PROFILE           &&
         m_eProfileIdc != HIGH_422_PROFILE          &&
         m_eProfileIdc != HIGH_444_PROFILE          &&
         m_eProfileIdc != CAVLC_444_PROFILE         &&
         m_eProfileIdc != SCALABLE_BASELINE_PROFILE &&
         m_eProfileIdc != SCALABLE_HIGH_PROFILE,      Err::m_nOK );

  RNOK  ( pcWriteIf->writeUvlc( m_uiChromaFormatIdc,            "SPS: chroma_format_idc" ) );
  if( m_uiChromaFormatIdc == 3 )
  {
    RNOK( pcWriteIf->writeFlag( m_bSeparateColourPlaneFlag,     "SPS: separate_colour_plane_flag" ) );
  }
  RNOK  ( pcWriteIf->writeUvlc( m_uiBitDepthLumaMinus8,         "SPS: bit_depth_luma_minus8" ) );
  RNOK  ( pcWriteIf->writeUvlc( m_uiBitDepthChromaMinus8,       "SPS: bit_depth_chroma_minus8" ) );
  RNOK  ( pcWriteIf->writeFlag( m_bTransformBypassFlag,         "SPS: qpprime_y_zero_transform_bypass_flag" ) );
  RNOK  ( pcWriteIf->writeFlag( m_bSeqScalingMatrixPresentFlag, "SPS: seq_scaling_matrix_present_flag"  ) );
  ROFRS ( m_bSeqScalingMatrixPresentFlag, Err::m_nOK );
  RNOK  ( m_cSeqScalingMatrix.write( pcWriteIf, true ) );

  return Err::m_nOK;
}


ErrVal
SequenceParameterSet::xReadFrext( HeaderSymbolReadIf* pcReadIf )
{
  ROTRS( m_eProfileIdc != HIGH_PROFILE              &&
         m_eProfileIdc != HIGH_10_PROFILE           &&
         m_eProfileIdc != HIGH_422_PROFILE          &&
         m_eProfileIdc != HIGH_444_PROFILE          &&
         m_eProfileIdc != CAVLC_444_PROFILE         &&
         m_eProfileIdc != SCALABLE_BASELINE_PROFILE &&
         m_eProfileIdc != SCALABLE_HIGH_PROFILE,      Err::m_nOK );

  RNOK  ( pcReadIf->getUvlc( m_uiChromaFormatIdc,            "SPS: chroma_format_idc" ) );
  if( m_uiChromaFormatIdc == 3 )
  {
    RNOK( pcReadIf->getFlag( m_bSeparateColourPlaneFlag,     "SPS: separate_colour_plane_flag" ) );
  }
  RNOK  ( pcReadIf->getUvlc( m_uiBitDepthLumaMinus8,         "SPS: bit_depth_luma_minus8" ) );
  RNOK  ( pcReadIf->getUvlc( m_uiBitDepthChromaMinus8,       "SPS: bit_depth_chroma_minus8" ) );
  RNOK  ( pcReadIf->getFlag( m_bTransformBypassFlag,         "SPS: qpprime_y_zero_transform_bypass_flag" ) );
  if( m_eProfileIdc == SCALABLE_BASELINE_PROFILE || m_eProfileIdc == SCALABLE_HIGH_PROFILE )
  {
    ROF( m_uiChromaFormatIdc      == 1  );
    ROF( m_uiBitDepthLumaMinus8   == 0  );
    ROF( m_uiBitDepthChromaMinus8 == 0  );
    ROT( m_bTransformBypassFlag         );
  }
  else if( m_eProfileIdc == HIGH_PROFILE )
  {
    ROF( m_uiChromaFormatIdc      <= 1  );
    ROF( m_uiBitDepthLumaMinus8   == 0  );
    ROF( m_uiBitDepthChromaMinus8 == 0  );
    ROT( m_bTransformBypassFlag         );
  }
  else if( m_eProfileIdc == HIGH_10_PROFILE )
  {
    ROF( m_uiChromaFormatIdc      <= 1  );
    ROF( m_uiBitDepthLumaMinus8   <= 2  );
    ROF( m_uiBitDepthChromaMinus8 <= 2  );
    ROT( m_bTransformBypassFlag         );
  }
  else if( m_eProfileIdc == HIGH_422_PROFILE )
  {
    ROF( m_uiChromaFormatIdc      <= 2  );
    ROF( m_uiBitDepthLumaMinus8   <= 2  );
    ROF( m_uiBitDepthChromaMinus8 <= 2  );
    ROT( m_bTransformBypassFlag         );
  }
  else
  {
    ROF( m_uiChromaFormatIdc      <= 3  );
    ROF( m_uiBitDepthLumaMinus8   <= 6  );
    ROF( m_uiBitDepthChromaMinus8 <= 6  );
  }

  RNOK( pcReadIf->getFlag( m_bSeqScalingMatrixPresentFlag,  "SPS: seq_scaling_matrix_present_flag") );
  ROFRS ( m_bSeqScalingMatrixPresentFlag, Err::m_nOK );
  RNOK  ( m_cSeqScalingMatrix.read( pcReadIf, true ) );

  return Err::m_nOK;
}


Void SequenceParameterSet::setResizeParameters( const ResizeParameters& rcResizeParameters )
{
  Int iVer = ( m_bFrameMbsOnlyFlag ? 2 : 4 ); // m_bFrameMbsOnlyFlag must be set !!!!!

  m_uiExtendedSpatialScalability  = (UInt)rcResizeParameters.m_iExtendedSpatialScalability;
  m_uiChromaPhaseXPlus1           = (UInt)( rcResizeParameters.m_iChromaPhaseX + 1 );
  m_uiChromaPhaseYPlus1           = (UInt)( rcResizeParameters.m_iChromaPhaseY + 1 );

  if( m_uiExtendedSpatialScalability == ESS_SEQ )
  {
    m_uiBaseChromaPhaseXPlus1 = (UInt)(rcResizeParameters.m_iRefLayerChromaPhaseX+1);
    m_uiBaseChromaPhaseYPlus1 = (UInt)(rcResizeParameters.m_iRefLayerChromaPhaseY+1);
    m_iScaledBaseLeftOffset   = rcResizeParameters.m_iLeftFrmOffset / 2;
    m_iScaledBaseTopOffset    = rcResizeParameters.m_iTopFrmOffset  / iVer;
    m_iScaledBaseRightOffset  = ( rcResizeParameters.m_iFrameWidth  - rcResizeParameters.m_iLeftFrmOffset - rcResizeParameters.m_iScaledRefFrmWidth  ) / 2;
    m_iScaledBaseBottomOffset = ( rcResizeParameters.m_iFrameHeight - rcResizeParameters.m_iTopFrmOffset  - rcResizeParameters.m_iScaledRefFrmHeight ) / iVer;
  }
  else
  {
    m_uiBaseChromaPhaseXPlus1 = m_uiChromaPhaseXPlus1;
    m_uiBaseChromaPhaseYPlus1 = m_uiChromaPhaseYPlus1;
    m_iScaledBaseBottomOffset = 0;
    m_iScaledBaseLeftOffset   = 0;
    m_iScaledBaseRightOffset  = 0;
    m_iScaledBaseTopOffset    = 0;
  }
}


ErrVal SequenceParameterSet::xReadPicOrderCntInfo( HeaderSymbolReadIf* pcReadIf )
{
  RNOK( pcReadIf->getUvlc( m_uiPicOrderCntType,                  "SPS: pic_order_cnt_type" ) );
  ROT( m_uiPicOrderCntType>2 );

  ROTRS( 2 == m_uiPicOrderCntType, Err::m_nOK );

  if( 0 == m_uiPicOrderCntType )
  {
    UInt uiTmp;
    RNOK( pcReadIf->getUvlc( uiTmp,                              "SPS: log2_max_pic_order_cnt_lsb_minus4" ));
    setLog2MaxPicOrderCntLsb( 4+uiTmp );
  }
  else if( 1 == m_uiPicOrderCntType )
  {
    RNOK( pcReadIf->getFlag( m_bDeltaPicOrderAlwaysZeroFlag,     "SPS: delta_pic_order_always_zero_flag" ));
    RNOK( pcReadIf->getSvlc( m_iOffsetForNonRefPic,              "SPS: offset_for_non_ref_pic" ));
    RNOK( pcReadIf->getSvlc( m_iOffsetForTopToBottomField,       "SPS: offset_for_top_to_bottom_field" ));
    RNOK( pcReadIf->getUvlc( m_uiNumRefFramesInPicOrderCntCycle, "SPS: num_ref_frames_in_pic_order_cnt_cycle" ));
    RNOK( initOffsetForRefFrame( m_uiNumRefFramesInPicOrderCntCycle ) );

    for( UInt i = 0; i < m_uiNumRefFramesInPicOrderCntCycle; i++)
    {
      Int  iTmp;
      RNOK( pcReadIf->getSvlc( iTmp,                             "SPS: offset_for_ref_frame" ) );
      setOffsetForRefFrame( i, iTmp );
    }
  }

  return Err::m_nOK;
}

H264AVC_NAMESPACE_END
