
#include "H264AVCEncoderLib.h"
#include "H264AVCCommonLib.h"

#include "H264AVCCommonLib/PocCalculator.h"
#include "H264AVCCommonLib/ControlMngIf.h"
#include "H264AVCCommonLib/CFMO.h"
#include "H264AVCCommonLib/LoopFilter.h"
#include "CodingParameter.h"
#include "PicEncoder.h"
#include "RecPicBuffer.h"
#include "NalUnitEncoder.h"
#include "SliceEncoder.h"


H264AVC_NAMESPACE_BEGIN


PicEncoder::PicEncoder()
: m_bInit                   ( false )
, m_bInitParameterSets      ( false )
, m_pcSequenceStructure     ( NULL )
, m_pcInputPicBuffer        ( NULL )
, m_pcSPS                   ( NULL )
, m_pcPPS                   ( NULL )
, m_pcRecPicBuffer          ( NULL )
, m_pcCodingParameter       ( NULL )
, m_pcControlMng            ( NULL )
, m_pcSliceEncoder          ( NULL )
, m_pcLoopFilter            ( NULL )
, m_pcPocCalculator         ( NULL )
, m_pcNalUnitEncoder        ( NULL )
, m_pcYuvBufferCtrlFullPel  ( NULL )
, m_pcYuvBufferCtrlHalfPel  ( NULL )
, m_pcQuarterPelFilter      ( NULL )
, m_pcMotionEstimation      ( NULL )
, m_uiFrameWidthInMb        ( 0 )
, m_uiFrameHeightInMb       ( 0 )
, m_uiMbNumber              ( 0 )
, m_uiFrameNum              ( 0 )
, m_uiIdrPicId              ( 0 )
, m_uiCodedFrames           ( 0 )
, m_dSumYPSNR               ( 0.0 )
, m_dSumUPSNR               ( 0.0 )
, m_dSumVPSNR               ( 0.0 )
, m_uiWrittenBytes          ( 0 )
, m_uiWriteBufferSize       ( 0 )
, m_pucWriteBuffer          ( NULL )
, m_bTraceEnable            ( true )
{
}


PicEncoder::~PicEncoder()
{
  uninit();
}


ErrVal
PicEncoder::create( PicEncoder*& rpcPicEncoder )
{
  rpcPicEncoder = new PicEncoder;
  ROF( rpcPicEncoder );
  return Err::m_nOK;
}

ErrVal
PicEncoder::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal
PicEncoder::init( CodingParameter*    pcCodingParameter,
                  ControlMngIf*       pcControlMng,
                  SliceEncoder*       pcSliceEncoder,
                  LoopFilter*         pcLoopFilter,
                  PocCalculator*      pcPocCalculator,
                  NalUnitEncoder*     pcNalUnitEncoder,
                  YuvBufferCtrl*      pcYuvBufferCtrlFullPel,
                  YuvBufferCtrl*      pcYuvBufferCtrlHalfPel,
                  QuarterPelFilter*   pcQuarterPelFilter,
                  MotionEstimation*   pcMotionEstimation )
{
  ROF( pcCodingParameter      );
  ROF( pcControlMng           );
  ROF( pcSliceEncoder         );
  ROF( pcLoopFilter           );
  ROF( pcPocCalculator        );
  ROF( pcNalUnitEncoder       );
  ROF( pcYuvBufferCtrlFullPel );
  ROF( pcYuvBufferCtrlHalfPel );
  ROF( pcQuarterPelFilter     );
  ROF( pcMotionEstimation     );

  m_pcCodingParameter       = pcCodingParameter;
  m_pcControlMng            = pcControlMng;
  m_pcSliceEncoder          = pcSliceEncoder;
  m_pcLoopFilter            = pcLoopFilter;
  m_pcPocCalculator         = pcPocCalculator;
  m_pcNalUnitEncoder        = pcNalUnitEncoder;
  m_pcYuvBufferCtrlFullPel  = pcYuvBufferCtrlFullPel;
  m_pcYuvBufferCtrlHalfPel  = pcYuvBufferCtrlHalfPel;
  m_pcQuarterPelFilter      = pcQuarterPelFilter;
  m_pcMotionEstimation      = pcMotionEstimation;

  //----- create objects -----
  RNOK( RecPicBuffer      ::create( m_pcRecPicBuffer ) );
  RNOK( SequenceStructure ::create( m_pcSequenceStructure ) );
  RNOK( InputPicBuffer    ::create( m_pcInputPicBuffer ) );

  //----- init objects -----
  RNOK( m_pcRecPicBuffer  ->init( m_pcYuvBufferCtrlFullPel, m_pcYuvBufferCtrlHalfPel ) );
  RNOK( m_pcInputPicBuffer->init() );

  m_pcSliceEncoder->getMbEncoder()->setLowComplexMbEnable( 0, false );

  //----- init parameters -----
  m_uiWrittenBytes          = 0;
  m_uiCodedFrames           = 0;
  m_dSumYPSNR               = 0.0;
  m_dSumVPSNR               = 0.0;
  m_dSumUPSNR               = 0.0;
  m_bInit                   = true;

  return Err::m_nOK;
}


ErrVal
PicEncoder::uninit()
{
  //----- free allocated memory -----
  RNOK( xDeleteData() );

  //----- free allocated member -----
  if( m_pcRecPicBuffer )
  {
    RNOK( m_pcRecPicBuffer->uninit  () );
    RNOK( m_pcRecPicBuffer->destroy () );
  }
  if( m_pcSPS )
  {
    RNOK( m_pcSPS->destroy() );
  }
  if( m_pcPPS )
  {
    RNOK( m_pcPPS->destroy() );
  }
  if( m_pcSequenceStructure )
  {
    RNOK( m_pcSequenceStructure->uninit () );
    RNOK( m_pcSequenceStructure->destroy() );
  }
  if( m_pcInputPicBuffer )
  {
    RNOK( m_pcInputPicBuffer->uninit  () );
    RNOK( m_pcInputPicBuffer->destroy () );
  }

  m_pcRecPicBuffer      = NULL;
  m_pcSequenceStructure = NULL;
  m_pcInputPicBuffer    = NULL;
  m_pcSPS               = NULL;
  m_pcPPS               = NULL;
  m_bInitParameterSets  = false;
  m_bInit               = false;

  return Err::m_nOK;
}


ErrVal
PicEncoder::writeAndInitParameterSets( ExtBinDataAccessor* pcExtBinDataAccessor,
                                       Bool&               rbMoreSets )
{
  if( ! m_pcSPS )
  {
    //===== create SPS =====
    RNOK( xInitSPS() );

    //===== write SPS =====
    UInt uiSPSBits = 0;
    RNOK( m_pcNalUnitEncoder->initNalUnit ( pcExtBinDataAccessor ) );
    RNOK( m_pcNalUnitEncoder->write       ( *m_pcSPS ) );
    RNOK( m_pcNalUnitEncoder->closeNalUnit( uiSPSBits ) );
    m_uiWrittenBytes += ( ( uiSPSBits >> 3 ) + 4 );
  }
  else if( ! m_pcPPS )
  {
    //===== create PPS =====
    RNOK( xInitPPS() );

    //===== write PPS =====
    UInt uiPPSBits = 0;
    RNOK( m_pcNalUnitEncoder->initNalUnit ( pcExtBinDataAccessor ) );
    RNOK( m_pcNalUnitEncoder->write       ( *m_pcPPS ) );
    RNOK( m_pcNalUnitEncoder->closeNalUnit( uiPPSBits ) );
    m_uiWrittenBytes += ( ( uiPPSBits >> 3 ) + 4 );

    //===== init pic encoder with parameter sets =====
    RNOK( xInitParameterSets() );
  }

  //===== set status =====
  rbMoreSets = ! m_bInitParameterSets;

  return Err::m_nOK;
}


ErrVal
PicEncoder::process( PicBuffer*               pcInputPicBuffer,
                     PicBufferList&           rcOutputList,
                     PicBufferList&           rcUnusedList,
                     ExtBinDataAccessorList&  rcExtBinDataAccessorList )
{
  ROF( m_bInitParameterSets );

  //===== add picture to input picture buffer =====
  RNOK( m_pcInputPicBuffer->add( pcInputPicBuffer ) );

  //===== encode following access units that are stored in input picture buffer =====
  while( true )
  {
    InputAccessUnit* pcInputAccessUnit = NULL;

    //----- get next frame specification and input access unit -----
    if( ! m_pcInputPicBuffer->empty() )
    {
      if( ! m_cFrameSpecification.isInitialized() )
      {
        m_cFrameSpecification = m_pcSequenceStructure->getNextFrameSpec();
      }
      pcInputAccessUnit       = m_pcInputPicBuffer->remove( m_cFrameSpecification.getContFrameNumber() );
    }
    if( ! pcInputAccessUnit )
    {
      break;
    }

    //----- initialize picture -----
    Double          dLambda         = 0;
    UInt            uiPictureBits   = 0;
    SliceHeader*    pcSliceHeader   = 0;
    RecPicBufUnit*  pcRecPicBufUnit = 0;
    PicBuffer*      pcOrigPicBuffer = pcInputAccessUnit->getInputPicBuffer();
    RNOK( xInitSliceHeader( pcSliceHeader, m_cFrameSpecification, dLambda ) );
    RNOK( m_pcRecPicBuffer->initCurrRecPicBufUnit( pcRecPicBufUnit, pcOrigPicBuffer, pcSliceHeader, rcOutputList, rcUnusedList ) );

    //----- encoding -----
    RNOK( xEncodePicture( rcExtBinDataAccessorList, *pcRecPicBufUnit, *pcSliceHeader, dLambda, uiPictureBits ) );
    m_uiWrittenBytes += ( uiPictureBits >> 3 );

    //----- store picture -----
    RNOK( m_pcRecPicBuffer->store( pcRecPicBufUnit, pcSliceHeader, rcOutputList, rcUnusedList ) );

    //----- reset -----
    delete pcInputAccessUnit;
    delete pcSliceHeader;
    m_cFrameSpecification.uninit();
  }

  return Err::m_nOK;
}


ErrVal
PicEncoder::finish( PicBufferList&  rcOutputList,
                    PicBufferList&  rcUnusedList )
{
  RNOK( m_pcRecPicBuffer->clear( rcOutputList, rcUnusedList ) );

  //===== output summary =====
  printf("\n %5d frames encoded:  Y %7.4lf dB  U %7.4lf dB  V %7.4lf dB\n"
       "\n     average bit rate:  %.4lf kbit/s  [%d byte for %.3lf sec]\n\n",
    m_uiCodedFrames,
    m_dSumYPSNR / (Double)m_uiCodedFrames,
    m_dSumUPSNR / (Double)m_uiCodedFrames,
    m_dSumVPSNR / (Double)m_uiCodedFrames,
    0.008*(Double)m_uiWrittenBytes/(Double)m_uiCodedFrames*m_pcCodingParameter->getMaximumFrameRate(),
    m_uiWrittenBytes,
    (Double)m_uiCodedFrames/m_pcCodingParameter->getMaximumFrameRate()
    );

  return Err::m_nOK;
}


ErrVal
PicEncoder::xInitSPS()
{
  ROF( m_bInit );
  ROT( m_pcSPS );

  //===== determine parameters =====
  UInt  uiSPSId     = 0;
  UInt  uiMbX       = ( m_pcCodingParameter->getFrameWidth () + 15 ) >> 4;
  UInt  uiMbY       = ( m_pcCodingParameter->getFrameHeight() + 15 ) >> 4;
  UInt  uiOutFreq   = (UInt)ceil( m_pcCodingParameter->getMaximumFrameRate() );
  UInt  uiMvRange   = m_pcCodingParameter->getMotionVectorSearchParams().getSearchRange();
  UInt  uiDPBSize   = m_pcCodingParameter->getDPBSize();
  UInt  uiLevelIdc  = SequenceParameterSet::getLevelIdc( uiMbY, uiMbX, uiOutFreq, uiMvRange, uiDPBSize, 0 );

  //===== create parameter sets =====
  RNOK( SequenceParameterSet::create( m_pcSPS ) );

  //===== set SPS parameters =====
  m_pcSPS->setNalUnitType                           ( NAL_UNIT_SPS );
  m_pcSPS->setAVCHeaderRewriteFlag( false );
  m_pcSPS->setDependencyId                               ( 0 );
  m_pcSPS->setProfileIdc                            ( HIGH_PROFILE );
  m_pcSPS->setConstrainedSet0Flag                   ( false );
  m_pcSPS->setConstrainedSet1Flag                   ( false );
  m_pcSPS->setConstrainedSet2Flag                   ( false );
  m_pcSPS->setConstrainedSet3Flag                   ( false );
  m_pcSPS->setConvertedLevelIdc                     ( uiLevelIdc );
  m_pcSPS->setSeqParameterSetId                     ( uiSPSId );
  m_pcSPS->setSeqScalingMatrixPresentFlag           ( m_pcCodingParameter->getScalingMatricesPresent() > 0 );
  m_pcSPS->setLog2MaxFrameNum                       ( m_pcCodingParameter->getLog2MaxFrameNum() );
  m_pcSPS->setLog2MaxPicOrderCntLsb                 ( m_pcCodingParameter->getLog2MaxPocLsb() );
  m_pcSPS->setNumRefFrames                          ( m_pcCodingParameter->getNumDPBRefFrames() );
  m_pcSPS->setGapsInFrameNumValueAllowedFlag   ( true );
  m_pcSPS->setFrameWidthInMbs                       ( uiMbX );
  m_pcSPS->setFrameHeightInMbs                      ( uiMbY );
  m_pcSPS->setDirect8x8InferenceFlag                ( true );

// JVT-V068 {
  m_pcSPS->setVUI                                   (m_pcSPS);
// JVT-V068 }
  return Err::m_nOK;
}


ErrVal
PicEncoder::xInitPPS()
{
  ROF( m_bInit );
  ROF( m_pcSPS );
  ROT( m_pcPPS );

  //===== determine parameters =====
  UInt  uiPPSId = 0;

  //===== create PPS =====
  RNOK( PictureParameterSet ::create( m_pcPPS ) );

  //===== set PPS parameters =====
  m_pcPPS->setNalUnitType                           ( NAL_UNIT_PPS );
  m_pcPPS->setPicParameterSetId                     ( uiPPSId );
  m_pcPPS->setSeqParameterSetId                     ( m_pcSPS->getSeqParameterSetId() );
  m_pcPPS->setEntropyCodingModeFlag                 ( m_pcCodingParameter->getSymbolMode() != 0 );
  m_pcPPS->setPicOrderPresentFlag                   ( true );
  m_pcPPS->setNumRefIdxActive                       ( LIST_0, m_pcCodingParameter->getMaxRefIdxActiveBL0() );
  m_pcPPS->setNumRefIdxActive                       ( LIST_1, m_pcCodingParameter->getMaxRefIdxActiveBL1() );
  m_pcPPS->setPicInitQp                             ( gMin( 51, gMax( 0, (Int)m_pcCodingParameter->getBasisQp() ) ) );
  m_pcPPS->setChromaQpIndexOffset                   ( 0 );
  m_pcPPS->setDeblockingFilterParametersPresentFlag ( ! m_pcCodingParameter->getLoopFilterParams().isDefault() );
  m_pcPPS->setConstrainedIntraPredFlag              ( m_pcCodingParameter->getConstrainedIntraPred() > 0 );
  m_pcPPS->setRedundantPicCntPresentFlag            ( false );  //JVT-Q054 Red. Picture
  m_pcPPS->setTransform8x8ModeFlag                  ( m_pcCodingParameter->getEnable8x8Trafo() > 0 );
  m_pcPPS->setPicScalingMatrixPresentFlag           ( false );
  m_pcPPS->set2ndChromaQpIndexOffset                ( 0 );
  m_pcPPS->setNumSliceGroupsMinus1                  ( 0 );

  //===== prediction weights =====
//  m_pcPPS->setWeightedPredFlag                      ( WEIGHTED_PRED_FLAG );
//  m_pcPPS->setWeightedBiPredIdc                     ( WEIGHTED_BIPRED_IDC );

//TMM_WP
    m_pcPPS->setWeightedPredFlag                   (m_pcCodingParameter->getIPMode()!=0);
    m_pcPPS->setWeightedBiPredIdc                  (m_pcCodingParameter->getBMode());
//TMM_WP

  return Err::m_nOK;
}


ErrVal
PicEncoder::xInitParameterSets()
{
  //===== init control manager =====
  m_pcSPS->setAllocFrameMbsX( m_pcSPS->getFrameWidthInMbs () );
  m_pcSPS->setAllocFrameMbsY( m_pcSPS->getFrameHeightInMbs() );
  RNOK( m_pcControlMng->initParameterSets( *m_pcSPS, *m_pcPPS ) );

  //===== set fixed parameters =====
  m_uiFrameWidthInMb      = m_pcSPS->getFrameWidthInMbs  ();
  m_uiFrameHeightInMb     = m_pcSPS->getFrameHeightInMbs ();
  m_uiMbNumber            = m_uiFrameWidthInMb * m_uiFrameHeightInMb;

  //===== re-allocate dynamic memory =====
  RNOK( xDeleteData() );
  RNOK( xCreateData() );

  //===== init objects =====
  RNOK( m_pcRecPicBuffer      ->initSPS ( *m_pcSPS ) );
  RNOK( m_pcSequenceStructure ->init    ( m_pcCodingParameter->getSequenceFormatString(),
                                          m_pcCodingParameter->getTotalFrames() ) );

  //==== initialize variable parameters =====
  m_uiFrameNum            = 0;
  m_uiIdrPicId            = 0;
  m_bInitParameterSets    = true;

  return Err::m_nOK;
}


ErrVal
PicEncoder::xCreateData()
{
  //===== write buffer =====
  UInt  uiNum4x4Blocks        = m_uiFrameWidthInMb * m_uiFrameHeightInMb * 4 * 4;
  m_uiWriteBufferSize         = 3 * ( uiNum4x4Blocks * 4 * 4 );
  ROFS( ( m_pucWriteBuffer   = new UChar [ m_uiWriteBufferSize ] ) );

  return Err::m_nOK;
}

ErrVal
PicEncoder::xDeleteData()
{

  //===== write buffer =====
  delete [] m_pucWriteBuffer;
  m_pucWriteBuffer    = 0;
  m_uiWriteBufferSize = 0;

  return Err::m_nOK;
}


ErrVal
PicEncoder::xInitSliceHeader( SliceHeader*&     rpcSliceHeader,
                              const FrameSpec&  rcFrameSpec,
                              Double&           rdLambda )
{
  ROF( m_bInitParameterSets );

  //===== create new slice header =====
  rpcSliceHeader = new SliceHeader( *m_pcSPS, *m_pcPPS );
  ROF( rpcSliceHeader );

  //===== determine parameters =====
  Double dQp      = m_pcCodingParameter->getBasisQp() + m_pcCodingParameter->getDeltaQpLayer( rcFrameSpec.getTemporalLayer() );
  Int    iQp      = gMin( 51, gMax( 0, (Int)dQp ) );
  rdLambda        = 0.85 * pow( 2.0, gMin( 52.0, dQp ) / 3.0 - 4.0 );
  UInt   uiSizeL0 = ( rcFrameSpec.getSliceType() == I_SLICE ? 0 :
                      rcFrameSpec.getSliceType() == P_SLICE ? m_pcCodingParameter->getMaxRefIdxActiveP  () :
                      rcFrameSpec.getSliceType() == B_SLICE ? m_pcCodingParameter->getMaxRefIdxActiveBL0() : MSYS_UINT_MAX );
  UInt   uiSizeL1 = ( rcFrameSpec.getSliceType() == I_SLICE ? 0 :
                      rcFrameSpec.getSliceType() == P_SLICE ? 0 :
                      rcFrameSpec.getSliceType() == B_SLICE ? m_pcCodingParameter->getMaxRefIdxActiveBL1() : MSYS_UINT_MAX );

  //===== set NAL unit header =====
  rpcSliceHeader->setNalRefIdc                          ( rcFrameSpec.getNalRefIdc    () );
  rpcSliceHeader->setNalUnitType                        ( rcFrameSpec.getNalUnitType  () );
  rpcSliceHeader->setDependencyId                       ( 0 );
  rpcSliceHeader->setTemporalId                         ( rcFrameSpec.getTemporalLayer() );
  rpcSliceHeader->setUseRefBasePicFlag                  ( false );
  rpcSliceHeader->setStoreRefBasePicFlag                ( false );
  rpcSliceHeader->setPriorityId                         ( 0 );
  rpcSliceHeader->setDiscardableFlag                    ( false );
  rpcSliceHeader->setQualityId                          ( 0 );
  rpcSliceHeader->setIdrFlag                            ( rcFrameSpec.getNalUnitType  () == NAL_UNIT_CODED_SLICE_IDR );


  //===== set general parameters =====
  rpcSliceHeader->setFirstMbInSlice                     ( 0 );
  rpcSliceHeader->setLastMbInSlice                      ( m_uiMbNumber - 1 );
  rpcSliceHeader->setSliceType                          ( rcFrameSpec.getSliceType    () );
  rpcSliceHeader->setFrameNum                           ( m_uiFrameNum );
  rpcSliceHeader->setNumMbsInSlice                      ( m_uiMbNumber );
  rpcSliceHeader->setIdrPicId                           ( m_uiIdrPicId );
  rpcSliceHeader->setDirectSpatialMvPredFlag            ( true );
  rpcSliceHeader->setRefLayer                           ( MSYS_UINT_MAX, 15 );
  rpcSliceHeader->setNoInterLayerPredFlag               ( true );
  rpcSliceHeader->setNoOutputOfPriorPicsFlag            ( false );
  rpcSliceHeader->setCabacInitIdc                       ( 0 );
  rpcSliceHeader->setSliceHeaderQp                      ( iQp );

  //===== reference picture list ===== (init with default data, later updated)
  rpcSliceHeader->setNumRefIdxActiveOverrideFlag        ( false );
  rpcSliceHeader->setNumRefIdxActive                    ( LIST_0, uiSizeL0 );
  rpcSliceHeader->setNumRefIdxActive                    ( LIST_1, uiSizeL1 );

  //===== set deblocking filter parameters =====
  if( rpcSliceHeader->getPPS().getDeblockingFilterParametersPresentFlag() )
  {
    rpcSliceHeader->getDeblockingFilterParameter().setDisableDeblockingFilterIdc(   m_pcCodingParameter->getLoopFilterParams().getFilterIdc   () );
    rpcSliceHeader->getDeblockingFilterParameter().setSliceAlphaC0Offset        ( 2*m_pcCodingParameter->getLoopFilterParams().getAlphaOffset () );
    rpcSliceHeader->getDeblockingFilterParameter().setSliceBetaOffset           ( 2*m_pcCodingParameter->getLoopFilterParams().getBetaOffset  () );
  }

  //===== set picture order count =====
  RNOK( m_pcPocCalculator->setPoc( *rpcSliceHeader, rcFrameSpec.getContFrameNumber() ) );

  //===== set MMCO commands =====
  if( rcFrameSpec.getMmcoBuf() )
  {
    rpcSliceHeader->getDecRefPicMarking().copy( *rcFrameSpec.getMmcoBuf() );
    rpcSliceHeader->getDecRefPicMarking().setAdaptiveRefPicMarkingModeFlag( true );
  }

  //===== set RPRL commands =====
  if( rcFrameSpec.getRplrBuf( LIST_0 ) )
  {
    rpcSliceHeader->getRefPicListReordering( LIST_0 ).copy( *rcFrameSpec.getRplrBuf( LIST_0 ) );
  }
  if( rcFrameSpec.getRplrBuf( LIST_1 ) )
  {
    rpcSliceHeader->getRefPicListReordering( LIST_1 ).copy( *rcFrameSpec.getRplrBuf( LIST_1 ) );
  }

  //===== flexible macroblock ordering =====
  rpcSliceHeader->setSliceGroupChangeCycle( 1 );
  rpcSliceHeader->FMOInit();


  //===== update parameters =====
  if( rpcSliceHeader->getNalRefIdc() )
  {
    m_uiFrameNum  = ( m_uiFrameNum + 1 ) % ( 1 << rpcSliceHeader->getSPS().getLog2MaxFrameNum() );
  }
  if( rpcSliceHeader->getIdrFlag() )
  {
    m_uiIdrPicId  = ( m_uiIdrPicId + 1 ) % 3;
  }

  return Err::m_nOK;
}


ErrVal
PicEncoder::xInitPredWeights( SliceHeader& rcSliceHeader )
{
  if( rcSliceHeader.isPSlice() )
  {
    if( rcSliceHeader.getPPS().getWeightedPredFlag() )
    {
      rcSliceHeader.setLumaLog2WeightDenom  ( 6 );
      rcSliceHeader.setChromaLog2WeightDenom( 6 );
      RNOK( rcSliceHeader.getPredWeightTableL0().initDefaults( rcSliceHeader.getLumaLog2WeightDenom(), rcSliceHeader.getChromaLog2WeightDenom() ) );
      RNOK( rcSliceHeader.getPredWeightTableL0().initRandomly() );
    }
  }
  else if( rcSliceHeader.isBSlice() )
  {
    if( rcSliceHeader.getPPS().getWeightedBiPredIdc() == 1 )
    {
      rcSliceHeader.setLumaLog2WeightDenom  ( 6 );
      rcSliceHeader.setChromaLog2WeightDenom( 6 );
      RNOK( rcSliceHeader.getPredWeightTableL0().initDefaults( rcSliceHeader.getLumaLog2WeightDenom(), rcSliceHeader.getChromaLog2WeightDenom() ) );
      RNOK( rcSliceHeader.getPredWeightTableL1().initDefaults( rcSliceHeader.getLumaLog2WeightDenom(), rcSliceHeader.getChromaLog2WeightDenom() ) );
      RNOK( rcSliceHeader.getPredWeightTableL0().initRandomly() );
      RNOK( rcSliceHeader.getPredWeightTableL1().initRandomly() );
    }
  }
  return Err::m_nOK;
}


ErrVal
PicEncoder::xInitExtBinDataAccessor( ExtBinDataAccessor& rcExtBinDataAccessor )
{
  ROF( m_pucWriteBuffer );
  m_cBinData.reset          ();
  m_cBinData.set            ( m_pucWriteBuffer, m_uiWriteBufferSize );
  m_cBinData.setMemAccessor ( rcExtBinDataAccessor );

  return Err::m_nOK;
}


ErrVal
PicEncoder::xAppendNewExtBinDataAccessor( ExtBinDataAccessorList& rcExtBinDataAccessorList,
                                          ExtBinDataAccessor*     pcExtBinDataAccessor )
{
  ROF( pcExtBinDataAccessor );
  ROF( pcExtBinDataAccessor->data() );

  UInt    uiNewSize     = pcExtBinDataAccessor->size();
  UChar*  pucNewBuffer  = new UChar [ uiNewSize ];
  ROF( pucNewBuffer );
  memcpy( pucNewBuffer, pcExtBinDataAccessor->data(), uiNewSize * sizeof( UChar ) );

  ExtBinDataAccessor* pcNewExtBinDataAccessor = new ExtBinDataAccessor;
  ROF( pcNewExtBinDataAccessor );

  m_cBinData              .reset          ();
  m_cBinData              .set            (  pucNewBuffer, uiNewSize );
  m_cBinData              .setMemAccessor ( *pcNewExtBinDataAccessor );
  rcExtBinDataAccessorList.push_back      (  pcNewExtBinDataAccessor );

  m_cBinData              .reset          ();
  m_cBinData              .setMemAccessor ( *pcExtBinDataAccessor );

  return Err::m_nOK;
}



ErrVal
PicEncoder::xEncodePicture( ExtBinDataAccessorList& rcExtBinDataAccessorList,
                            RecPicBufUnit&          rcRecPicBufUnit,
                            SliceHeader&            rcSliceHeader,
                            Double                  dLambda,
                            UInt&                   ruiBits  )
{
  UInt  uiBits = 0;

  //===== start picture =====
  RefFrameList  cList0, cList1;
  RNOK( xStartPicture( rcRecPicBufUnit, rcSliceHeader, cList0, cList1 ) );
  RefListStruct cRefListStruct;
  cRefListStruct.acRefFrameListME[0].copy( cList0 );
  cRefListStruct.acRefFrameListMC[0].copy( cList0 );
  cRefListStruct.acRefFrameListRC[0].copy( cList0 );
  cRefListStruct.acRefFrameListME[1].copy( cList1 );
  cRefListStruct.acRefFrameListMC[1].copy( cList1 );
  cRefListStruct.acRefFrameListRC[1].copy( cList1 );
  cRefListStruct.bMCandRClistsDiffer = false;
  cRefListStruct.uiFrameIdCol        = MSYS_UINT_MAX;

//TMM_WP
  if(rcSliceHeader.getSliceType() == P_SLICE)
      m_pcSliceEncoder->xSetPredWeights( rcSliceHeader,
                                         rcRecPicBufUnit.getRecFrame(),
                                         cRefListStruct );
  else if(rcSliceHeader.getSliceType() == B_SLICE)
      m_pcSliceEncoder->xSetPredWeights( rcSliceHeader,
                                         rcRecPicBufUnit.getRecFrame(),
                                         cRefListStruct );

//TMM_WP

  //===== encoding of slice groups =====
  for( Int iSliceGroupID = rcSliceHeader.getFMO()->getFirstSliceGroupId(); ! rcSliceHeader.getFMO()->SliceGroupCompletelyCoded( iSliceGroupID ); iSliceGroupID = rcSliceHeader.getFMO()->getNextSliceGroupId( iSliceGroupID ) )
  {
    UInt  uiBitsSlice = 0;

    //----- init slice size -----
    rcSliceHeader.setFirstMbInSlice( rcSliceHeader.getFMO()->getFirstMacroblockInSlice( iSliceGroupID ) );
    rcSliceHeader.setLastMbInSlice ( rcSliceHeader.getFMO()->getLastMBInSliceGroup    ( iSliceGroupID ) );

    //----- init NAL unit -----
    RNOK( xInitExtBinDataAccessor        (  m_cExtBinDataAccessor ) );
    RNOK( m_pcNalUnitEncoder->initNalUnit( &m_cExtBinDataAccessor ) );

    //----- write slice header -----
    RNOK( m_pcNalUnitEncoder->write ( rcSliceHeader ) );

    //----- real coding -----
    RNOK( m_pcSliceEncoder->encodeSlice( rcSliceHeader,
                                         rcRecPicBufUnit.getRecFrame  (),
                                         rcRecPicBufUnit.getMbDataCtrl(),
                                         cRefListStruct,
                                         m_pcCodingParameter->getMCBlks8x8Disable() > 0,
                                         m_uiFrameWidthInMb,
                                         dLambda ) );

    //----- close NAL unit -----
    RNOK( m_pcNalUnitEncoder->closeNalUnit( uiBitsSlice ) );
    RNOK( xAppendNewExtBinDataAccessor( rcExtBinDataAccessorList, &m_cExtBinDataAccessor ) );
    uiBitsSlice += 4*8;
    uiBits      += uiBitsSlice;
  }


  //===== finish =====
  RNOK( xFinishPicture( rcRecPicBufUnit, rcSliceHeader, cList0, cList1, uiBits ) );
  ruiBits += uiBits;

  return Err::m_nOK;
}


ErrVal
PicEncoder::xStartPicture( RecPicBufUnit& rcRecPicBufUnit,
                           SliceHeader&   rcSliceHeader,
                           RefFrameList&  rcList0,
                           RefFrameList&  rcList1 )
{
  //===== initialize reference picture lists and update slice header =====
  RNOK( m_pcRecPicBuffer->getRefLists( rcList0, rcList1, rcSliceHeader ) );
  rcSliceHeader.setRefFrameList( &rcList0, FRAME, LIST_0 );
  rcSliceHeader.setRefFrameList( &rcList1, FRAME, LIST_1 );

  //===== init half-pel buffers =====
  UInt uiPos;
  for( uiPos = 0; uiPos < rcList0.getActive(); uiPos++ )
  {
    Frame* pcRefFrame = rcList0.getEntry( uiPos );
    if( ! pcRefFrame->isHalfPel() )
    {
      RNOK( pcRefFrame->initHalfPel() );
      RNOK( pcRefFrame->extendFrame( m_pcQuarterPelFilter ) );
    }
    else
    if( ! pcRefFrame->isExtended() )
    {
      RNOK( pcRefFrame->extendFrame( m_pcQuarterPelFilter ) );
    }
  }
  for( uiPos = 0; uiPos < rcList1.getActive(); uiPos++ )
  {
    Frame* pcRefFrame = rcList1.getEntry( uiPos );
    if( ! pcRefFrame->isHalfPel() )
    {
      RNOK( pcRefFrame->initHalfPel() );
    }
    if( ! pcRefFrame->isExtended() )
    {
      RNOK( pcRefFrame->extendFrame( m_pcQuarterPelFilter ) );
    }
  }

  //===== reset macroblock data =====
  RNOK( rcRecPicBufUnit.getMbDataCtrl()->reset() );
  RNOK( rcRecPicBufUnit.getMbDataCtrl()->clear() );

  return Err::m_nOK;
}


ErrVal
PicEncoder::xFinishPicture( RecPicBufUnit&  rcRecPicBufUnit,
                            SliceHeader&    rcSliceHeader,
                            RefFrameList&   rcList0,
                            RefFrameList&   rcList1,
                            UInt            uiBits )
{
  //===== uninit half-pel data =====
  UInt uiPos;
  for( uiPos = 0; uiPos < rcList0.getActive(); uiPos++ )
  {
    Frame* pcRefFrame = rcList0.getEntry( uiPos );
    if( pcRefFrame->isExtended() )
    {
      pcRefFrame->clearExtended();
    }
    if( pcRefFrame->isHalfPel() )
    {
      pcRefFrame->uninitHalfPel();
    }
  }
  for( uiPos = 0; uiPos < rcList1.getActive(); uiPos++ )
  {
    Frame* pcRefFrame = rcList1.getEntry( uiPos );
    if( pcRefFrame->isExtended() )
    {
      pcRefFrame->clearExtended();
    }
    if( pcRefFrame->isHalfPel() )
    {
      pcRefFrame->uninitHalfPel();
    }
  }

  //===== deblocking =====
  RNOK( m_pcLoopFilter->process( rcSliceHeader, rcRecPicBufUnit.getRecFrame(), NULL, rcRecPicBufUnit.getMbDataCtrl(), 0, false ) );

  //===== get PSNR =====
  Double dPSNR[3];
  RNOK( xGetPSNR( rcRecPicBufUnit, dPSNR ) );

  //===== output =====
  printf( "%4d %c %s %4d  QP%3d  Y %7.4lf dB  U %7.4lf dB  V %7.4lf dB   bits%8d\n",
    m_uiCodedFrames,
    rcSliceHeader.getSliceType  ()==I_SLICE ? 'I' :
    rcSliceHeader.getSliceType  ()==P_SLICE ? 'P' : 'B',
    rcSliceHeader.getNalUnitType()==NAL_UNIT_CODED_SLICE_IDR ? "IDR" :
    rcSliceHeader.getNalRefIdc  ()==NAL_REF_IDC_PRIORITY_LOWEST ? "   " : "REF",
    rcSliceHeader.getPoc(),
    rcSliceHeader.getSliceQp(),
    dPSNR[0],
    dPSNR[1],
    dPSNR[2],
    uiBits
    );

  //===== update parameters =====
  m_uiCodedFrames++;
  ETRACE_NEWPIC;

  return Err::m_nOK;
}


ErrVal
PicEncoder::xGetPSNR( RecPicBufUnit&  rcRecPicBufUnit,
                      Double*         adPSNR )
{
  //===== reset buffer control =====
  RNOK( m_pcYuvBufferCtrlFullPel->initMb() );

  //===== set parameters =====
  const YuvBufferCtrl::YuvBufferParameter&  cBufferParam  = m_pcYuvBufferCtrlFullPel->getBufferParameter();
  Frame*                                 pcFrame       = rcRecPicBufUnit.getRecFrame  ();
  PicBuffer*                                pcPicBuffer   = rcRecPicBufUnit.getPicBuffer ();

  //===== calculate PSNR =====
  Pel*    pPelOrig  = pcPicBuffer->getBuffer() + cBufferParam.getMbLum();
  XPel*   pPelRec   = pcFrame->getFullPelYuvBuffer()->getMbLumAddr();
  Int     iStride   = cBufferParam.getStride();
  Int     iWidth    = cBufferParam.getWidth ();
  Int     iHeight   = cBufferParam.getHeight();
  UInt    uiSSDY    = 0;
  UInt    uiSSDU    = 0;
  UInt    uiSSDV    = 0;
  Int     x, y;

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)pPelOrig[x] - (Int)pPelRec[x];
      uiSSDY   += iDiff * iDiff;
    }
    pPelOrig += iStride;
    pPelRec  += iStride;
  }

  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  pPelOrig  = pcPicBuffer->getBuffer() + cBufferParam.getMbCb();
  pPelRec   = pcFrame->getFullPelYuvBuffer()->getMbCbAddr();

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)pPelOrig[x] - (Int)pPelRec[x];
      uiSSDU   += iDiff * iDiff;
    }
    pPelOrig += iStride;
    pPelRec  += iStride;
  }

  pPelOrig  = pcPicBuffer->getBuffer() + cBufferParam.getMbCr();
  pPelRec   = pcFrame->getFullPelYuvBuffer()->getMbCrAddr();

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)pPelOrig[x] - (Int)pPelRec[x];
      uiSSDV   += iDiff * iDiff;
    }
    pPelOrig += iStride;
    pPelRec  += iStride;
  }

  Double fRefValueY = 255.0 * 255.0 * 16.0 * 16.0 * (Double)m_uiMbNumber;
  Double fRefValueC = fRefValueY / 4.0;
  adPSNR[0]         = ( uiSSDY ? 10.0 * log10( fRefValueY / (Double)uiSSDY ) : 99.99 );
  adPSNR[1]         = ( uiSSDU ? 10.0 * log10( fRefValueC / (Double)uiSSDU ) : 99.99 );
  adPSNR[2]         = ( uiSSDV ? 10.0 * log10( fRefValueC / (Double)uiSSDV ) : 99.99 );
  m_dSumYPSNR      += adPSNR[0];
  m_dSumUPSNR      += adPSNR[1];
  m_dSumVPSNR      += adPSNR[2];

  return Err::m_nOK;
}

H264AVC_NAMESPACE_END

