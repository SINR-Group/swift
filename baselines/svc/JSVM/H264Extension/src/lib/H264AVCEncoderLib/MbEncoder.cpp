
#include "H264AVCEncoderLib.h"
#include "MbEncoder.h"
#include "H264AVCCommonLib/Tables.h"


#include "H264AVCCommonLib/Transform.h"

#include "IntraPredictionSearch.h"
#include "MotionEstimation.h"
#include "CodingParameter.h"

#include "RateDistortionIf.h"

// JVT-W043 {
#include "RateCtlBase.h"
#include "RateCtlQuadratic.h"
// JVT-W043 }

H264AVC_NAMESPACE_BEGIN

typedef MotionEstimation::MEBiSearchParameters  BSParams;


const UChar g_aucFrameBits[32] =
{
  0,
  1,
  3, 3,
  5, 5, 5, 5,
  7, 7, 7, 7, 7, 7, 7, 7,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
};

//JVT-V079 Low-complexity MB mode decision {
#define SHIFT_QP 12

static const Int g_iQP2quant[40] = {
 1, 1, 1, 1, 2, 2, 2, 2,
 3, 3, 3, 4, 4, 4, 5, 6,
 6, 7, 8, 9, 10, 11, 13, 14,
 16, 18, 20, 23, 25, 29, 32, 36,
 40, 45, 51, 57, 64, 72, 81, 91
};
//JVT-V079 Low-complexity MB mode decision }


MbEncoder::MbEncoder():
  m_pcCodingParameter( NULL ),
  m_pcTransform( NULL ),
  m_pcIntraPrediction( NULL ),
  m_pcMotionEstimation( NULL ),
  m_pcRateDistortionIf( NULL ),
  m_pcXDistortion( 0 ),
  m_bInitDone( false ),
  m_pcIntMbBestData( NULL ),
  m_pcIntMbTempData( NULL ),
  m_pcIntMbBest8x8Data( NULL ),
  m_pcIntMbTemp8x8Data( NULL ),
  m_pcIntMbBestIntraChroma( NULL ),
  m_pcIntOrgMbPelData( NULL ),
#if PROPOSED_DEBLOCKING_APRIL2010
  m_pcRefLayerResidual( NULL ),
#endif
  m_pcIntPicBuffer( NULL ),
  m_pcIntraPredPicBuffer( NULL ),
  m_BitCounter( NULL )
  ,m_bLARDOEnable( false ), //JVT-R057 LA-RDO
  m_uiMBSSD( 0 ),           //JVT-R057 LA-RDO
  m_pcFrameEcEp ( NULL ),   //JVT-R057 LA-RDO
  m_iEpRef ( 0 ),           //JVT-R057 LA-RDO
  m_dWr0 ( 0.5 ),           //JVT-R057 LA-RDO
  m_dWr1 ( 0.5 )            //JVT-R057 LA-RDO
  //S051{
  ,m_bUseBDir(true)
  //S051}
  ,m_bBaseModeAllowedFlag( true )
  , m_uiIPCMRate( 0 )
{
}





MbEncoder::~MbEncoder()
{
}



ErrVal
MbEncoder::create( MbEncoder*& rpcMbEncoder )
{
  rpcMbEncoder = new MbEncoder;
  ROT( NULL == rpcMbEncoder );

  return Err::m_nOK;
}


ErrVal
MbEncoder::destroy()
{
  delete this;

  return Err::m_nOK;
}


ErrVal
MbEncoder::initSlice( const SliceHeader& rcSH )
{
  RNOK( MbCoder::initSlice( rcSH, this, MbEncoder::m_pcRateDistortionIf ) );

  m_BitCounter            = (BitWriteBufferIf*)this;
  RNOK( UvlcWriter::init( m_BitCounter ) );
  RNOK( UvlcWriter::startSlice( rcSH ) );

  m_pcIntMbBestData       = &m_acIntMbTempData[0];
  m_pcIntMbTempData       = &m_acIntMbTempData[1];
  m_pcIntMbBest8x8Data    = &m_acIntMbTempData[2];
  m_pcIntMbTemp8x8Data    = &m_acIntMbTempData[3];

  return Err::m_nOK;
}



ErrVal
MbEncoder::init( Transform*             pcTransform,
                 IntraPredictionSearch* pcIntraPrediction,
                 MotionEstimation*      pcMotionEstimation,
                 CodingParameter*       pcCodingParameter,
                 RateDistortionIf*      pcRateDistortionIf,
                 XDistortion*           pcXDistortion )
{
  ROT( NULL == pcTransform );
  ROT( NULL == pcIntraPrediction );
  ROT( NULL == pcMotionEstimation );
  ROT( NULL == pcCodingParameter );
  ROT( NULL == pcRateDistortionIf );
  ROT( NULL == pcXDistortion );

  m_pcRateDistortionIf    = pcRateDistortionIf;
  m_pcCodingParameter     = pcCodingParameter;
  m_pcXDistortion         = pcXDistortion;

  m_pcTransform           = pcTransform;
  m_pcIntraPrediction     = pcIntraPrediction;
  m_pcMotionEstimation    = pcMotionEstimation;
  m_bInitDone             = true;

  return Err::m_nOK;
}


ErrVal
MbEncoder::uninit()
{
  RNOK( MbCoder::uninit() );

  m_pcTransform = NULL;
  m_pcIntraPrediction = NULL;
  m_pcMotionEstimation = NULL;
  m_bInitDone = false;
  return Err::m_nOK;
}



ErrVal
MbEncoder::xCheckInterMbMode8x8( IntMbTempData*&    rpcMbTempData,
                                 IntMbTempData*&    rpcMbBestData,
                                 IntMbTempData*     pcMbRefData,
                                 RefListStruct&     rcRefListStruct,
                                 UInt               uiMinQP,
                                 UInt               uiMaxQP,
                                 Bool               bBLSkip,
                                 MbDataAccess*      pcMbDataAccessBaseMotion,
                                 Frame*             pcBaseLayerRec,
                                 const YuvMbBuffer* pcBaseLayerResidual )
{
  ROTRS( ! rpcMbTempData->getSH().getPPS().getTransform8x8ModeFlag(), Err::m_nOK );
  ROTRS( ! pcMbRefData->is8x8TrafoFlagPresent( rpcMbTempData->getSH().getSPS().getDirect8x8InferenceFlag() ), Err::m_nOK );

  if( pcMbRefData == rpcMbTempData )
  {
    rpcMbTempData->clearCost          ();
    rpcMbTempData->getMbTCoeffs       ().clear();
    rpcMbTempData->setTransformSize8x8( true );
  }
  else
  {
    rpcMbTempData->clear                ();
    rpcMbTempData->setMbMode            (           pcMbRefData->getMbMode            () );
    rpcMbTempData->setBLSkipFlag        (           pcMbRefData->getBLSkipFlag        () );
    rpcMbTempData->setResidualPredFlag  (           pcMbRefData->getResidualPredFlag  () );
    rpcMbTempData->setBlkMode           ( B_8x8_0,  pcMbRefData->getBlkMode           ( B_8x8_0 ) );
    rpcMbTempData->setBlkMode           ( B_8x8_1,  pcMbRefData->getBlkMode           ( B_8x8_1 ) );
    rpcMbTempData->setBlkMode           ( B_8x8_2,  pcMbRefData->getBlkMode           ( B_8x8_2 ) );
    rpcMbTempData->setBlkMode           ( B_8x8_3,  pcMbRefData->getBlkMode           ( B_8x8_3 ) );
    rpcMbTempData->setTransformSize8x8  ( true );

    rpcMbTempData->getMbMotionData( LIST_0 ).copyFrom( pcMbRefData->getMbMotionData( LIST_0 ) );
    rpcMbTempData->getMbMotionData( LIST_1 ).copyFrom( pcMbRefData->getMbMotionData( LIST_1 ) );
    rpcMbTempData->getMbMvdData   ( LIST_0 ).copyFrom( pcMbRefData->getMbMvdData   ( LIST_0 ) );
    rpcMbTempData->getMbMvdData   ( LIST_1 ).copyFrom( pcMbRefData->getMbMvdData   ( LIST_1 ) );
  }

  RNOK( xSetRdCost8x8InterMb( *rpcMbTempData, pcMbDataAccessBaseMotion, rcRefListStruct,
                              uiMinQP, uiMaxQP, bBLSkip, 0, pcBaseLayerRec, pcBaseLayerResidual ) );

  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
	  rpcMbTempData->rdCost()+=getEpRef();
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}



ErrVal
MbEncoder::encodeMacroblock( MbDataAccess&  rcMbDataAccess,
                             Frame*         pcFrame,
                             RefListStruct& rcRefListStruct,
                             UInt           uiMaxNumMv,
                             Bool           bMCBlks8x8Disable,
                             Bool           bBiPred8x8Disable,
                             UInt           uiNumMaxIter,
                             UInt           uiIterSearchRange,
                             Double         dLambda )
{
  ROF( m_bInitDone );
  ROF( uiMaxNumMv );

  bBiPred8x8Disable = ( bBiPred8x8Disable || rcMbDataAccess.getSH().getSPS().getBiPred8x8Disabled() );

  UInt  uiQp = rcMbDataAccess.getMbData().getQp();
  RNOK( m_pcRateDistortionIf->setMbQpLambda( rcMbDataAccess, uiQp, dLambda ) );

  m_pcIntMbBestIntraChroma  = NULL;
  m_pcIntMbBestData   ->init( rcMbDataAccess );
  m_pcIntMbTempData   ->init( rcMbDataAccess );
  m_pcIntMbBest8x8Data->init( rcMbDataAccess );
  m_pcIntMbTemp8x8Data->init( rcMbDataAccess );

  m_pcIntPicBuffer = pcFrame->getFullPelYuvBuffer();
  m_pcXDistortion->loadOrgMbPelData( m_pcIntPicBuffer, m_pcIntOrgMbPelData );
  m_pcTransform->setQp( rcMbDataAccess, false );

  //====== evaluate macroblock modes ======
  if( rcMbDataAccess.getSH().isPSlice() )
  {
    RNOK( xEstimateMbSkip     ( m_pcIntMbTempData,  m_pcIntMbBestData,  rcRefListStruct, false, true ) );
  }
  if( rcMbDataAccess.getSH().isBSlice() )
  {
    RNOK( xEstimateMbDirect   ( m_pcIntMbTempData,  m_pcIntMbBestData,  rcRefListStruct, uiQp, uiQp, NULL, uiMaxNumMv, false,  rcMbDataAccess.getMbData().getQp() ) );
  }
  if( rcMbDataAccess.getSH().isPSlice() || rcMbDataAccess.getSH().isBSlice() )
  {
    RNOK( xEstimateMb16x16    ( m_pcIntMbTempData,  m_pcIntMbBestData,  rcRefListStruct, uiQp, uiQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange,  NULL, false ) );
    RNOK( xEstimateMb16x8     ( m_pcIntMbTempData,  m_pcIntMbBestData,  rcRefListStruct, uiQp, uiQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange,  NULL, false ) );
    RNOK( xEstimateMb8x16     ( m_pcIntMbTempData,  m_pcIntMbBestData,  rcRefListStruct, uiQp, uiQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange,  NULL, false ) );
    RNOK( xEstimateMb8x8      ( m_pcIntMbTempData,  m_pcIntMbBestData,  rcRefListStruct, uiQp, uiQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange,  NULL, false, bMCBlks8x8Disable, bBiPred8x8Disable ) );
    RNOK( xEstimateMb8x8Frext ( m_pcIntMbTempData,  m_pcIntMbBestData,  rcRefListStruct, uiQp, uiQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange,  NULL, false ) );
  }
  RNOK(   xEstimateMbIntra16  ( m_pcIntMbTempData,  m_pcIntMbBestData,  uiQp, rcMbDataAccess.getSH().isBSlice() ) );
  RNOK(   xEstimateMbIntra8   ( m_pcIntMbTempData,  m_pcIntMbBestData,  uiQp, rcMbDataAccess.getSH().isBSlice() ) );
  RNOK(   xEstimateMbIntra4   ( m_pcIntMbTempData,  m_pcIntMbBestData,  uiQp, rcMbDataAccess.getSH().isBSlice() ) );
  RNOK(   xEstimateMbPCM      ( m_pcIntMbTempData,  m_pcIntMbBestData,        rcMbDataAccess.getSH().isBSlice() ) );

  //===== fix estimation =====
  RNOK( m_pcRateDistortionIf->fixMacroblockQP( *m_pcIntMbBestData ) );
  RNOK( xStoreEstimation( rcMbDataAccess, *m_pcIntMbBestData, NULL, NULL, NULL, rcRefListStruct, NULL ) );

  //===== uninit =====
  m_pcIntMbBestData   ->uninit();
  m_pcIntMbTempData   ->uninit();
  m_pcIntMbBest8x8Data->uninit();
  m_pcIntMbTemp8x8Data->uninit();

  return Err::m_nOK;
}






ErrVal
MbEncoder::xCheckSkipSliceMb( IntMbTempData& rcMbTempData )
{
  ROFRS( rcMbTempData.getSH().getSliceSkipFlag(), Err::m_nOK );

  //===== re-load prediction signal =====
  rcMbTempData.loadLuma   ( rcMbTempData.getTempYuvMbBuffer() );
  rcMbTempData.loadChroma ( rcMbTempData.getTempYuvMbBuffer() );

  //===== clear transform coefficients =====
  rcMbTempData.MbTransformCoeffs::clear();

  //===== adjust coeffs =====
  Bool bBaseTransSize8x8 = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().isTransformSize8x8();
  if( rcMbTempData.getMbDataAccess().isSCoeffPred() )
  {
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      reCalcBlock8x8( rcMbTempData, c8x8Idx, ! bBaseTransSize8x8 );
    }
    reCalcChroma( rcMbTempData );
  }
  else if( rcMbTempData.getMbDataAccess().isTCoeffPred() )
  {
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      RNOK( reCalcBlock8x8Rewrite( rcMbTempData, c8x8Idx, ! bBaseTransSize8x8 ) );
    }
    RNOK( reCalcChromaRewrite( rcMbTempData ) );
  }

  //===== clear parameters =====
  rcMbTempData.cbp      () = 0;
  rcMbTempData.bits     () = 0;
  rcMbTempData.coeffCost() = 0;
  return Err::m_nOK;
}

ErrVal
MbEncoder::xCheckSkipSliceMbIntra4( IntMbTempData& rcMbTempData, LumaIdx c4x4Idx, UInt& ruiAbsSum )
{
  ROFRS( rcMbTempData.getSH().getSliceSkipFlag(), Err::m_nOK );

  //===== re-load prediction signal of 4x4 block =====
  rcMbTempData.loadLuma( rcMbTempData.getTempYuvMbBuffer(), c4x4Idx );

  //===== clear transform coefficients =====
  rcMbTempData.MbTransformCoeffs::clearLumaLevels4x4( c4x4Idx );

  //===== set base layer QP (see G.8.1.5.1.2) =====
  UChar ucQp     = rcMbTempData.getQp();
  UChar ucBaseQp = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp();
  rcMbTempData  .setQp( ucBaseQp );
  m_pcTransform->setQp( rcMbTempData, true );

  //===== adjust coeffs =====
  RNOK( reCalcBlock4x4Rewrite( rcMbTempData, c4x4Idx ) );

  //===== restore QP (will be adjusted later) =====
  rcMbTempData  .setQp( ucQp );
  m_pcTransform->setQp( rcMbTempData, true );

  //===== clear parameters =====
  ruiAbsSum = 0;
  return Err::m_nOK;
}

ErrVal
MbEncoder::xCheckSkipSliceMbIntra8( IntMbTempData& rcMbTempData, B8x8Idx c8x8Idx, UInt& ruiAbsSum )
{
  ROFRS( rcMbTempData.getSH().getSliceSkipFlag(), Err::m_nOK );

  //===== re-load prediction signal of 8x8 block =====
  rcMbTempData.loadLuma( rcMbTempData.getTempYuvMbBuffer(), c8x8Idx );

  //===== clear transform coefficients =====
  rcMbTempData.MbTransformCoeffs::clearLumaLevels8x8Block( c8x8Idx );

  //===== set base layer QP (see G.8.1.5.1.2) =====
  UChar ucQp     = rcMbTempData.getQp();
  UChar ucBaseQp = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp();
  rcMbTempData  .setQp( ucBaseQp );
  m_pcTransform->setQp( rcMbTempData, true );

  //===== adjust coeffs =====
  RNOK( reCalcBlock8x8Rewrite( rcMbTempData, c8x8Idx, false ) );

  //===== restore QP (will be adjusted later) =====
  rcMbTempData  .setQp( ucQp );
  m_pcTransform->setQp( rcMbTempData, true );

  //===== clear parameters =====
  ruiAbsSum = 0;
  return Err::m_nOK;
}

ErrVal
MbEncoder::xCheckSkipSliceMbIntra16( IntMbTempData& rcMbTempData, UInt& ruiAcAbs )
{
  ROFRS( rcMbTempData.getSH().getSliceSkipFlag(), Err::m_nOK );

  //===== re-load luma prediction signal =====
  rcMbTempData.loadLuma( rcMbTempData.getTempYuvMbBuffer() );

  //===== clear transform coefficients =====
  rcMbTempData.MbTransformCoeffs::clearLumaLevels();

  //===== set base layer QP (see G.8.1.5.1.2) =====
  UChar ucQp     = rcMbTempData.getQp();
  UChar ucBaseQp = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp();
  rcMbTempData  .setQp( ucBaseQp );
  m_pcTransform->setQp( rcMbTempData, true );

  //===== adjust coeffs =====
  RNOK( reCalcBlock16x16Rewrite( rcMbTempData ) );

  //===== restore QP (will be adjusted later) =====
  rcMbTempData  .setQp( ucQp );
  m_pcTransform->setQp( rcMbTempData, true );

  //===== clear parameters =====
  ruiAcAbs = 0;
  return Err::m_nOK;
}

ErrVal
MbEncoder::xCheckSkipSliceMbIntraChroma( IntMbTempData& rcMbTempData, UInt& ruiChromaCbp )
{
  ROFRS( rcMbTempData.getSH().getSliceSkipFlag(),         Err::m_nOK );
  ROF  ( rcMbTempData.getMbDataAccess().isTCoeffPred() );
  ROF  ( rcMbTempData.isIntraBL() );

  //===== re-load prediction signal of chroma block =====
  rcMbTempData.loadChroma( rcMbTempData.getTempYuvMbBuffer() );

  //===== clear transform coefficients =====
  rcMbTempData.MbTransformCoeffs::clearChromaLevels();

  //===== set base layer QP (see G.8.1.5.1.2) =====
  UChar ucQp     = rcMbTempData.getQp();
  UChar ucBaseQp = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp();
  rcMbTempData  .setQp( ucBaseQp );
  m_pcTransform->setQp( rcMbTempData, true );

  //===== adjust coeffs =====
  RNOK( reCalcChromaRewrite( rcMbTempData ) );

  //===== restore QP (will be adjusted later) =====
  rcMbTempData  .setQp( ucQp );
  m_pcTransform->setQp( rcMbTempData, true );

  //===== clear parameters =====
  ruiChromaCbp = 0;
  return Err::m_nOK;

}


ErrVal
MbEncoder::xAdjustRewriteReconstruction( IntMbTempData& rcMbTempData )
{
  ROT   ( rcMbTempData.isIntra() ); // do not call this function for intra blocks !!!
  ROTRS ( rcMbTempData.getSH().getNoInterLayerPredFlag(),       Err::m_nOK );
  ROFRS ( rcMbTempData.getSH().getTCoeffLevelPredictionFlag(),  Err::m_nOK );
  ROTRS ( rcMbTempData.getMbCbp(),                              Err::m_nOK );
  ROFRS ( rcMbTempData.getResidualPredFlag(),                   Err::m_nOK );

  //===== reload prediction signal =====
  rcMbTempData.loadLuma   ( rcMbTempData.getTempYuvMbBuffer() );
  rcMbTempData.loadChroma ( rcMbTempData.getTempYuvMbBuffer() );

  //===== set base layer QP (see G.8.1.5.1.2) =====
  UChar ucQp     = rcMbTempData.getQp();
  UChar ucBaseQp = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp();
  Bool  bBase8x8 = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().isTransformSize8x8();
  rcMbTempData  .setQp( ucBaseQp );
  m_pcTransform->setQp( rcMbTempData, false );

  //===== recalc signal =====
  for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
  {
    RNOK( reCalcBlock8x8Rewrite( rcMbTempData, c8x8Idx, !bBase8x8 ) );
  }
  RNOK( reCalcChromaRewrite( rcMbTempData ) );

  //===== restore QP (will be adjusted later) =====
  rcMbTempData  .setQp( ucQp );
  m_pcTransform->setQp( rcMbTempData, false );

  return Err::m_nOK;
}

ErrVal
MbEncoder::encodeMacroblockSVC( MbDataAccess&   rcMbDataAccess,       // current macroblock data
                                MbDataAccess*   pcMbDataAccessBase,   // inferred macroblock data (from base layer)
                                const Frame&    rcOrgFrame,           // original frame
                                Frame&          rcFrame,              // reconstructed frame
                                Frame*          pcResidualLF,         // reconstructed residual for loop filter
                                Frame*          pcResidualILPred,     // reconstructed residual for inter-layer prediction
                                Frame*          pcPredSignal,         // prediction signal
                                const Frame*    pcBaseLayerIntraRec,  // base layer intra reconstruction
                                const Frame*    pcBaseLayerResidual,  // base layer residual reconstruction
                                RefListStruct&  rcRefListStruct,      // reference picture lists
                                UInt            uiMaxNumMv,           // maximum number of MVs for current macroblock
                                UInt            uiNumMaxIter,         // number of iteration for bi-predictive search
                                UInt            uiIterSearchRange,    // search range for iterative search
                                Bool            bBiPred8x8Disable,    // if bi-prediction for blocks smaller than 8x8 is allowed
                                Bool            bMCBlks8x8Disable,    // if blocks smaller than 8x8 are disabled
                                Bool            bSkipModeAllowed,     // if skip mode is allowed
                                UInt            uiMaxDeltaQp,         // maximum delta QP
                                Double          dLambda,              // Langrangian multiplier
                                Double&         rdCost                // r-d cost for coded macroblock
                                )
{
  ROF( m_bInitDone );
  Bool  bDisableIntraDQP  = false; // optional
  uiMaxDeltaQp            = ( m_bLowComplexMbEnable[rcMbDataAccess.getSH().getDependencyId()] ? 0 : uiMaxDeltaQp );
  UInt  uiCurrQP          = rcMbDataAccess.getMbData().getQp();
  UInt  uiMinQp           = (UInt)gMax( MIN_QP, Int( uiCurrQP - uiMaxDeltaQp ) );
  UInt  uiMaxQp           = (UInt)gMin( MAX_QP, Int( uiCurrQP + uiMaxDeltaQp ) );
  Bool  bISlice           = ( rcMbDataAccess.getSH().getSliceType () == I_SLICE );
  Bool  bPSlice           = ( rcMbDataAccess.getSH().getSliceType () == P_SLICE );
  Bool  bBSlice           = ( rcMbDataAccess.getSH().getSliceType () == B_SLICE );
  bBiPred8x8Disable       = ( bBiPred8x8Disable || rcMbDataAccess.getSH().getSPS().getBiPred8x8Disabled() );
  ROT( ( bPSlice || bBSlice ) && ( uiMaxNumMv                                      == 0 ) );
  ROT( ( bPSlice || bBSlice ) && ( rcRefListStruct.acRefFrameListRC[0].getActive() == 0 ) );
  ROT( (            bBSlice ) && ( rcRefListStruct.acRefFrameListRC[1].getActive() == 0 ) );
  ROT( ( bPSlice            ) && ( rcRefListStruct.acRefFrameListRC[1].getActive() != 0 ) );
  ROT( ( bISlice            ) && ( rcRefListStruct.acRefFrameListRC[0].getActive() != 0 ) );
  ROT( ( bISlice            ) && ( rcRefListStruct.acRefFrameListRC[0].getActive() != 0 ) );

  //===== set QP and Lambda =====
  if( rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag() )
  {
    UInt uiLastQP  = rcMbDataAccess.getLastQp();
    ROF( uiLastQP >= uiMinQp && uiLastQP <= uiMaxQp );
    ROF( ! bDisableIntraDQP  || uiLastQP == uiCurrQP || !bISlice );
  }
  RNOK( m_pcRateDistortionIf->setMbQpLambda( rcMbDataAccess, uiCurrQP, dLambda ) );
  m_pcTransform->setClipMode( false );

  //===== set reference layer =====
  rcMbDataAccess.setMbDataAccessBase( pcMbDataAccessBase );
  YuvMbBuffer cBaseLayerBuffer;

  //===== init temporary data =====
  m_pcIntMbBestIntraChroma  = NULL;
  m_pcIntMbBestData   ->init( rcMbDataAccess );
  m_pcIntMbTempData   ->init( rcMbDataAccess );
  m_pcIntMbBest8x8Data->init( rcMbDataAccess );
  m_pcIntMbTemp8x8Data->init( rcMbDataAccess );

  //===== init picture data ====
  m_pcIntPicBuffer = rcFrame.getFullPelYuvBuffer();
  m_pcXDistortion->loadOrgMbPelData( rcOrgFrame.getFullPelYuvBuffer(), m_pcIntOrgMbPelData );

  //===== set estimation parameters =====
  ROT( !rcMbDataAccess.getSH().getNoInterLayerPredFlag() && ( !pcMbDataAccessBase || !pcBaseLayerIntraRec || !pcBaseLayerResidual ) );
  Bool  bIsEnhLayer           = !rcMbDataAccess.getSH().getNoInterLayerPredFlag();
  Bool  bInCropWindow         = ( bIsEnhLayer && pcMbDataAccessBase->getMbData().getInCropWindowFlag() );
  pcMbDataAccessBase          = ( bInCropWindow  ? pcMbDataAccessBase : 0 );
  Bool  bSNRMode              = bInCropWindow && ( rcMbDataAccess.getSH().getSCoeffResidualPredFlag() || rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag() );
  Bool  bMismatchedFieldFlag  = bSNRMode && rcMbDataAccess.getMbData().getFieldFlag() != pcMbDataAccessBase->getMbData().getFieldFlag();
  Bool  bIntraEnable          = true;
  Bool  bInterEnable          = !bISlice;
  Bool  bTCoeffPredFlag       = rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag();
  Bool  bSCoeffPredFlag       = rcMbDataAccess.getCoeffResidualPredFlag();
  Bool  bARP                  = rcMbDataAccess.getSH().getAdaptiveResidualPredictionFlag();
  Bool  bDRP                  = rcMbDataAccess.getSH().getDefaultResidualPredictionFlag ();
  Bool  bELMbRefInter         = ( bInCropWindow && bInterEnable && !pcMbDataAccessBase->getMbData().isIntra() );
  Bool  bCheckWithResPred     = ( bInterEnable &&    bInCropWindow && ( ( bARP && bELMbRefInter ) ||  bDRP ) && !bMismatchedFieldFlag );
  Bool  bCheckWithoutResPred  = ( bInterEnable && ( !bInCropWindow ||     bARP                    || !bDRP ) );
  Bool  bWithAndWithoutRP     = ( bCheckWithResPred && bCheckWithoutResPred );
  Bool  bPreferResPred        = rcMbDataAccess.getSH().getPPS().getEntropyCodingModeFlag();
  Bool  bZeroBaseLayerResFlag = false;
  if( bCheckWithResPred )
  {
    if( bTCoeffPredFlag )
    {
      cBaseLayerBuffer.setAllSamplesToZero();
      bZeroBaseLayerResFlag   = pcMbDataAccessBase->getMbTCoeffs().allCoeffsZero();
    }
    else if( bSCoeffPredFlag )
    {
      pcMbDataAccessBase->getMbTCoeffs().copyPredictionTo( cBaseLayerBuffer );
      bZeroBaseLayerResFlag   = pcMbDataAccessBase->getMbTCoeffs().allLevelsAndPredictionsZero();
    }
    else
    {
      cBaseLayerBuffer.loadBuffer( pcBaseLayerResidual->getFullPelYuvBuffer() );
      bZeroBaseLayerResFlag   = cBaseLayerBuffer.isZero();
    }
  }
  if( bWithAndWithoutRP && bZeroBaseLayerResFlag ) // inter enabled, in crop window, adap. res. pred, inter base layer block, and zero base layer residual
  { // avoiding to check both
    bCheckWithoutResPred      = !bPreferResPred;
    bCheckWithResPred         =  bPreferResPred;
  }
  Bool  bABM                  = rcMbDataAccess.getSH().getAdaptiveBaseModeFlag();
  Bool  bDBM                  = rcMbDataAccess.getSH().getDefaultBaseModeFlag ();
  Bool  bCheckBaseMode        =  bInCropWindow && ( bABM ||  bDBM ) && !bMismatchedFieldFlag;
  Bool  bNonBaseModeOk        = !bInCropWindow ||   bABM || !bDBM;
  Bool  bCheckStdInter        = !bInCropWindow || rcMbDataAccess.getSH().getAdaptiveILPred() || ( !bABM && !bDBM );
  Bool  bCheckIntraBL         = ( bIntraEnable &&    bInCropWindow && ( bABM ||  bDBM ) && pcMbDataAccessBase->getMbData().isIntra() && !bMismatchedFieldFlag );
  Bool  bCheckSpatialIntra    = ( bIntraEnable && ( !bInCropWindow ||   bABM || !bDBM ) && rcMbDataAccess.getSH().getSPS().getNumberOfQualityLevelsCGSSNR() == 1 );
  Bool  bLowComplexMbEnable   = m_bLowComplexMbEnable[rcMbDataAccess.getSH().getDependencyId()];
  m_bBaseMotionPredAllowed    = ( rcMbDataAccess.getSH().getAdaptiveMotionPredictionFlag() || rcMbDataAccess.getSH().getDefaultMotionPredictionFlag() ) && !bMismatchedFieldFlag;


  //===== CHECK: inter modes with residual prediction =====
  if( bCheckWithResPred )
  {
    m_pcIntOrgMbPelData->subtract( cBaseLayerBuffer );
#if PROPOSED_DEBLOCKING_APRIL2010
    m_pcRefLayerResidual = &cBaseLayerBuffer;
#endif

    if( bCheckBaseMode && !pcMbDataAccessBase->getMbData().isIntra() )
    {
      RNOK  ( xEstimateMbBLSkip   ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, pcBaseLayerIntraRec, uiMaxNumMv, bBiPred8x8Disable, bBSlice, pcMbDataAccessBase, rcMbDataAccess, true, &cBaseLayerBuffer ) );
    }
    if( bCheckStdInter )
    {
      RNOK  ( xEstimateMbDirect   ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, pcMbDataAccessBase,  uiMaxNumMv, true, rcMbDataAccess.getMbData().getQp(), bSkipModeAllowed ) );
      RNOK  ( xEstimateMb16x16    ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, true ) );
      RNOK  ( xEstimateMb16x8     ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, true ) );
      RNOK  ( xEstimateMb8x16     ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, true ) );
      RNOK  ( xEstimateMb8x8      ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, true, bMCBlks8x8Disable, bBiPred8x8Disable ) );
      RNOK  ( xEstimateMb8x8Frext ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, true ) );
      if( bWithAndWithoutRP && !bCheckWithoutResPred )
      { // potential B_SKIP without residual prediction
        RNOK( xEstimateMbDirect   ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, pcMbDataAccessBase,  uiMaxNumMv, false, rcMbDataAccess.getMbData().getQp(), bSkipModeAllowed ) );
      }
      if( bPSlice && rcMbDataAccess.getSH().getDefaultResidualPredictionFlag() && !bLowComplexMbEnable )
      { // P_SKIP with residual prediction
        RNOK( xEstimateMbSkip( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, true,  bSkipModeAllowed ) );
      }
      if( bPSlice && !bCheckWithoutResPred && !rcMbDataAccess.getSH().getDefaultResidualPredictionFlag() && !bLowComplexMbEnable )
      { // P_SKIP without residual prediction
        RNOK( xEstimateMbSkip( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, false, bSkipModeAllowed ) );
      }
    }
    else if( bNonBaseModeOk && !pcMbDataAccessBase->getMbData().isIntra() && m_pcIntMbBestData->getCostData().rdCost() == DOUBLE_MAX ) // fall-back
    {
      ROF   ( rcMbDataAccess.getSH().getAdaptiveBaseModeFlag() );
      RNOK  ( xEstimateMb16x16    ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, true ) );
    }

    m_pcIntOrgMbPelData->add( cBaseLayerBuffer );
#if PROPOSED_DEBLOCKING_APRIL2010
    m_pcRefLayerResidual = 0;
#endif
  }

  //===== CHECK: inter modes without residual prediction =====
  if( bCheckWithoutResPred )
  {
    if( bCheckBaseMode && !pcMbDataAccessBase->getMbData().isIntra() )
    {
      RNOK  ( xEstimateMbBLSkip   ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, pcBaseLayerIntraRec, uiMaxNumMv, bBiPred8x8Disable, bBSlice, pcMbDataAccessBase, rcMbDataAccess, false ) );
    }
    if( bCheckStdInter )
    {
      if( bPSlice && !bLowComplexMbEnable )
      { // P_SKIP without residual prediction
        RNOK( xEstimateMbSkip     ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, false, bSkipModeAllowed ) );
      }
      RNOK  ( xEstimateMbDirect   ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, pcMbDataAccessBase, uiMaxNumMv, false,  rcMbDataAccess.getMbData().getQp(), bSkipModeAllowed ) );
      RNOK  ( xEstimateMb16x16    ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, false ) );
      RNOK  ( xEstimateMb16x8     ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, false ) );
      RNOK  ( xEstimateMb8x16     ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, false ) );
      RNOK  ( xEstimateMb8x8      ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, false, bMCBlks8x8Disable, bBiPred8x8Disable ) );
      RNOK  ( xEstimateMb8x8Frext ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, false ) );
    }
    else if( bNonBaseModeOk && !pcMbDataAccessBase->getMbData().isIntra() && m_pcIntMbBestData->getCostData().rdCost() == DOUBLE_MAX ) // fall-back
    {
      ROF   ( rcMbDataAccess.getSH().getAdaptiveBaseModeFlag() );
      RNOK  ( xEstimateMb16x16    ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiMinQp, uiMaxQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, pcMbDataAccessBase, false ) );
    }
  }

  //===== CHECK: intra base layer mode =====
  if( bCheckIntraBL )
  {
    RNOK( xEstimateMbIntraBL( m_pcIntMbTempData, m_pcIntMbBestData, uiMinQp, uiMaxQp, pcBaseLayerIntraRec, bBSlice, pcMbDataAccessBase ) );
  }

  //===== CHECK: spatial intra modes ====
  m_pcTransform->setClipMode( true );
  if( bCheckSpatialIntra )
  {
    UInt  uiMinIntraQP  = ( bDisableIntraDQP ? uiCurrQP : uiMinQp );
    UInt  uiMaxIntraQP  = ( bDisableIntraDQP ? uiCurrQP : uiMaxQp );
    for( UInt uiQp = uiMinIntraQP; uiQp <= uiMaxIntraQP; uiQp++ )
    {
      m_pcIntMbBestIntraChroma  = NULL;
      RNOK( xEstimateMbIntra16 ( m_pcIntMbTempData, m_pcIntMbBestData, uiQp, bBSlice ) );
      RNOK( xEstimateMbIntra8  ( m_pcIntMbTempData, m_pcIntMbBestData, uiQp, bBSlice ) );
      RNOK( xEstimateMbIntra4  ( m_pcIntMbTempData, m_pcIntMbBestData, uiQp, bBSlice ) );
    }
    RNOK  ( xEstimateMbPCM     ( m_pcIntMbTempData, m_pcIntMbBestData,       bBSlice ) );
  }

  //---- check whether a mode was selected ---
  if( m_pcIntMbBestData->getCostData().rdCost() == DOUBLE_MAX )
  {
    ROF( bMismatchedFieldFlag );
    rdCost = DOUBLE_MAX;
    return Err::m_nOK;
  }

  //JVT-W043 {
  if( bRateControlEnable && !pcJSVMParams->m_uiLayerId )
  {
    pcGenericRC->update_rc( jsvmCalcMAD( m_pcIntMbBestData, rcMbDataAccess ) );
  }
  // JVT-W043 }

  // JVT-V079 Low-complexity MB mode decision {
  if( bLowComplexMbEnable && !m_pcIntMbBestData->isIntra() )
  {
    if( bPSlice && m_pcIntMbBestData->getMbMode() == MODE_16x16 && m_pcIntMbBestData->getMbMotionData( LIST_0 ).getRefIdx() == 1 )
    {
      RNOK( xEstimateMbSkip( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, rcMbDataAccess.getSH().getDefaultResidualPredictionFlag(), bSkipModeAllowed ) );
    }
    /* Inter mode needs motion compensation */
    RNOK( xSetRdCostInterMb( *m_pcIntMbBestData, pcMbDataAccessBase, rcRefListStruct, uiMinQp, uiMaxQp ) );
  }
  //JVT-V079 Low-complexity MB mode decision }

  //===== store best estimation =====
  RNOK( m_pcRateDistortionIf->fixMacroblockQP( *m_pcIntMbBestData ) );
  RNOK( xStoreEstimation( rcMbDataAccess, *m_pcIntMbBestData, pcResidualLF, pcResidualILPred, pcPredSignal, rcRefListStruct, &cBaseLayerBuffer ) );


  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  Int x,y;
	  MbMode mode=rcMbDataAccess.getMbData().getMbMode();
	  Bool bInter=!rcMbDataAccess.getSH().getNoInterLayerPredFlag();
	  Int KBlock = m_pcIntPicBuffer->getLWidth()/4;

	  Int blockX=rcMbDataAccess.getMbX()*4;
	  Int blockY=rcMbDataAccess.getMbY()*4;
	  Int ep_ref,ec_rec,ec_ep;
	  m_pcIntPicBuffer->getYuvBufferCtrl().initMb();

	  UInt p=1;
	  UInt q=100;
	  if(bInter)
	  {
		  for(UInt i=0;i<=m_uiLayerID;i++)
		  {
			  //Bug_Fix JVT-R057 0806{
			  //p=p*(100-m_auiPLR[m_uiLayerID]);
			  p=p*(100-m_auiPLR[i]);
			  //Bug_Fix JVT-R057 0806}
		  }
		  //q=(UInt)pow(100,(m_uiLayerID+1));
      q=(UInt)pow(100.0,(int)(m_uiLayerID+1));
	  }
	  else
	  {
		  p=100-m_auiPLR[m_uiLayerID];
	  }

	  if(mode==INTRA_BL)
	  {
		  Int ep_base;
		  Int blockIndex;
		  Int xx,yy;
		  for(y=blockY;y<(blockY+4);y++)
			  for(x=blockX;x<(blockX+4);x++)
			  {
				  if(m_pcFrameEcEp)
					  ec_ep=m_pcFrameEcEp->getChannelDistortion()[y*KBlock+x];
				  else
					  ec_ep=0;
				  m_pcIntPicBuffer->getYuvBufferCtrl().initMb();
				  if(m_pcFrameEcEp)
					  ec_rec=GetEC_REC(m_pcIntPicBuffer,m_pcFrameEcEp->getFullPelYuvBuffer(),x,y);
				  else
					  ec_rec=0;

				  xx=(Int)(x/m_aadRatio[m_uiLayerID][0]);
				  yy=(Int)(y/m_aadRatio[m_uiLayerID][1]);
				  blockIndex=yy*(Int)(KBlock/m_aadRatio[m_uiLayerID][0])+xx;

				  ep_base=pcBaseLayerIntraRec->getChannelDistortion()[blockIndex];
				  rcFrame.getChannelDistortion()[y*KBlock+x]=(p*ep_base+(q-p)*(ec_rec+ec_ep))/q;
			  }
	  }
	  else if(mode==MODE_SKIP||mode==MODE_16x16||mode==MODE_16x8||mode==MODE_8x16||mode==MODE_8x8||mode==MODE_8x8ref0)
	  {
		  for( Int n = 0; n <16; n++)
		  {
			  Int iRefIdx[2];
			  iRefIdx [0]=rcMbDataAccess.getMbMotionData(LIST_0).getRefIdx(B4x4Idx(n));
			  iRefIdx [1]=rcMbDataAccess.getMbMotionData(LIST_1).getRefIdx(B4x4Idx(n));
			  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
			  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
			  Int iMvX;
			  Int iMvY;
			  Int iDLIST0=0,iDLIST1=0;
			  if(pcRefFrame0)
			  {
				  iMvX=rcMbDataAccess.getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getHor();
				  iMvY=rcMbDataAccess.getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getVer();
				  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&iDLIST0,iMvX,iMvY,n%4,n/4,1,1);
			  }
			  if(pcRefFrame1)
			  {
				  iMvX=rcMbDataAccess.getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getHor();
				  iMvY=rcMbDataAccess.getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getVer();
				  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&iDLIST1,iMvX,iMvY,n%4,n/4,1,1);
				  iDLIST0=(iDLIST0+iDLIST1)/2;
			  }
			  ep_ref=iDLIST0;

			  x=blockX+n%4;
			  y=blockY+n/4;
			  if(m_pcFrameEcEp)
				  ec_ep=m_pcFrameEcEp->getChannelDistortion()[y*KBlock+x];
			  else
				  ec_ep=0;
			  m_pcIntPicBuffer->getYuvBufferCtrl().initMb();
			  if(m_pcFrameEcEp)
				  ec_rec=GetEC_REC(m_pcIntPicBuffer,m_pcFrameEcEp->getFullPelYuvBuffer(),x,y);
			  else
				  ec_rec=0;

			  rcFrame.getChannelDistortion()[y*KBlock+x]=(p*ep_ref+(q-p)*(ec_rec+ec_ep))/q;
		  }
	  }
	  else
	  {
		  for(y=blockY;y<(blockY+4);y++)
			  for(x=blockX;x<(blockX+4);x++)
			  {
				  if(m_pcFrameEcEp)
					  ec_ep=m_pcFrameEcEp->getChannelDistortion()[y*KBlock+x];
				  else
					  ec_ep=0;
				  m_pcIntPicBuffer->getYuvBufferCtrl().initMb();
				  if(m_pcFrameEcEp)
					  ec_rec=GetEC_REC(m_pcIntPicBuffer,m_pcFrameEcEp->getFullPelYuvBuffer(),x,y);
				  else
					  ec_rec=0;
				  rcFrame.getChannelDistortion()[y*KBlock+x]=(q-p)*(ec_rec+ec_ep)/q;
			  }
	  }
  }
  //JVT-R057 LA-RDO}

  rdCost = m_pcIntMbBestData->rdCost();

  m_pcIntMbBestData   ->uninit();
  m_pcIntMbTempData   ->uninit();
  m_pcIntMbBest8x8Data->uninit();
  m_pcIntMbTemp8x8Data->uninit();

  return Err::m_nOK;
}


ErrVal
MbEncoder::estimatePrediction( MbDataAccess&  rcMbDataAccess,
                               RefListStruct& rcRefListStruct,
                               const Frame&   rcOrigFrame,
                               Frame&         rcIntraRecFrame,
                               UInt           uiMaxNumMv,
                               Bool           bMCBlks8x8Disable,
                               Bool           bBiPred8x8Disable,
                               UInt           uiNumMaxIter,
                               UInt           uiIterSearchRange,
                               Double         dLambda,
                               Double&        rdCost )
{
  ROF( m_bInitDone );
  ROF( uiMaxNumMv );

  bBiPred8x8Disable = ( bBiPred8x8Disable || rcMbDataAccess.getSH().getSPS().getBiPred8x8Disabled() );
  rcMbDataAccess.setMbDataAccessBase( NULL );

  Bool  bBSlice = rcMbDataAccess.getSH().getSliceType () == B_SLICE;
  UInt  uiQp    = rcMbDataAccess.getSH().getSliceQp   ();
  RNOK( m_pcRateDistortionIf->setMbQpLambda( rcMbDataAccess, uiQp, dLambda ) )

  m_pcIntMbBestIntraChroma  = NULL;
  m_pcIntMbBestData   ->init( rcMbDataAccess );
  m_pcIntMbTempData   ->init( rcMbDataAccess );
  m_pcIntMbBest8x8Data->init( rcMbDataAccess );
  m_pcIntMbTemp8x8Data->init( rcMbDataAccess );

  m_pcIntPicBuffer               = rcIntraRecFrame.getFullPelYuvBuffer();
  YuvPicBuffer*  pcOrgPicBuffer  = const_cast<Frame&>( rcOrigFrame ).getFullPelYuvBuffer();
  m_pcXDistortion->loadOrgMbPelData ( pcOrgPicBuffer, m_pcIntOrgMbPelData );
  m_pcTransform  ->setQp            ( rcMbDataAccess, false );

  Bool  bIntraEnable  = ! rcMbDataAccess.isFieldMbInMbaffFrame() || rcMbDataAccess.isTopMb() ||   rcMbDataAccess.getMbDataComplementary().isIntra();
  Bool  bInterEnable  = ! rcMbDataAccess.isFieldMbInMbaffFrame() || rcMbDataAccess.isTopMb() || ! rcMbDataAccess.getMbDataComplementary().isIntra();

  if( bInterEnable )
  {
    if( ! bBSlice )
    {
      RNOK( xEstimateMbSkip     ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, false, true ) );
    }
    RNOK  ( xEstimateMbDirect   ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiQp, uiQp, 0, uiMaxNumMv, false, rcMbDataAccess.getMbData().getQp(), true ) );
    RNOK  ( xEstimateMb16x16    ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiQp, uiQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, 0, false ) );
    RNOK  ( xEstimateMb16x8     ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiQp, uiQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, 0, false ) );
    RNOK  ( xEstimateMb8x16     ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiQp, uiQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, 0, false ) );
    RNOK  ( xEstimateMb8x8      ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiQp, uiQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, 0, false, bMCBlks8x8Disable, bBiPred8x8Disable ) );
    RNOK  ( xEstimateMb8x8Frext ( m_pcIntMbTempData, m_pcIntMbBestData, rcRefListStruct, uiQp, uiQp, uiMaxNumMv, uiNumMaxIter, uiIterSearchRange, 0, false ) );
  }
  if( bIntraEnable )
  {
    RNOK  ( xEstimateMbIntra16  ( m_pcIntMbTempData, m_pcIntMbBestData, uiQp, bBSlice ) );
    RNOK  ( xEstimateMbIntra8   ( m_pcIntMbTempData, m_pcIntMbBestData, uiQp, bBSlice ) );
    RNOK  ( xEstimateMbIntra4   ( m_pcIntMbTempData, m_pcIntMbBestData, uiQp, bBSlice ) );
    RNOK  ( xEstimateMbPCM      ( m_pcIntMbTempData, m_pcIntMbBestData,       bBSlice ) );
  }
  RNOK( xStoreEstimation( rcMbDataAccess, *m_pcIntMbBestData, NULL, NULL, NULL, rcRefListStruct, 0 ) );

  rdCost = m_pcIntMbBestData->rdCost();

  m_pcIntMbBestData   ->uninit();
  m_pcIntMbTempData   ->uninit();
  m_pcIntMbBest8x8Data->uninit();
  m_pcIntMbTemp8x8Data->uninit();

  return Err::m_nOK;
}



ErrVal
MbEncoder::compensatePrediction( MbDataAccess&  rcMbDataAccess,
                                 Frame*         pcMCFrame,
                                 RefListStruct& rcRefListStruct,
                                 Bool           bCalcMv,
                                 Bool           bFaultTolerant)
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListRC[0];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListRC[1];
  YuvMbBuffer   cYuvMbBuffer;
  if( rcMbDataAccess.getMbData().isIntra() )
  {
    cYuvMbBuffer.setAllSamplesToZero();
  }
  else
  {
    if( rcMbDataAccess.getMbData().getMbMode() == MODE_8x8 || rcMbDataAccess.getMbData().getMbMode() == MODE_8x8ref0 )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        RNOK( m_pcMotionEstimation->compensateSubMb( c8x8Idx, rcMbDataAccess, rcRefFrameList0, rcRefFrameList1, &cYuvMbBuffer, bCalcMv, bFaultTolerant ) );
      }
    }
    else
    {
      //----- motion compensated prediction -----
      RNOK( m_pcMotionEstimation->compensateMb( rcMbDataAccess, rcRefFrameList0, rcRefFrameList1, &cYuvMbBuffer, bCalcMv ) );
    }
    if( getBaseLayerRec() )
    {
      RNOK( m_pcMotionEstimation->compensateMbBLSkipIntra( rcMbDataAccess, &cYuvMbBuffer, getBaseLayerRec() ) );
    }
  }

  //===== insert into frame =====
  RNOK( pcMCFrame->getFullPelYuvBuffer()->loadBuffer( &cYuvMbBuffer ) );

  return Err::m_nOK;
}


ErrVal
MbEncoder::compensateUpdate(  MbDataAccess&   rcMbDataAccess,
                              Frame*       pcMCFrame,
                              Int             iRefIdx,
                              ListIdx         eListPrd,
                              Frame*       pcPrdFrame)
{

  if( rcMbDataAccess.getMbData().isIntra() )
  {
    return Err::m_nOK;
  }
  else
  {
    if( rcMbDataAccess.getMbData().getMbMode() == MODE_8x8 || rcMbDataAccess.getMbData().getMbMode() == MODE_8x8ref0 )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        if( rcMbDataAccess.getMbData().getMbMotionData(eListPrd).getRefIdx( c8x8Idx.b8x8() ) == iRefIdx )
          RNOK( m_pcMotionEstimation->updateSubMb( c8x8Idx, rcMbDataAccess, pcMCFrame, pcPrdFrame, eListPrd ) );
      }
    }
    else
    {
      RNOK( m_pcMotionEstimation->updateMb( rcMbDataAccess, pcMCFrame, pcPrdFrame, eListPrd, iRefIdx ) );
    }
  }

  return Err::m_nOK;
}




ErrVal
MbEncoder::xCheckBestEstimation( IntMbTempData*& rpcMbTempData,
                                 IntMbTempData*& rpcMbBestData )
{
  ROFRS( rpcMbTempData->rdCost() < rpcMbBestData->rdCost(), Err::m_nOK );

  //----- switch data objects -----
  IntMbTempData*  pcTempData  = rpcMbTempData;
  rpcMbTempData               = rpcMbBestData;
  rpcMbBestData               = pcTempData;

  // Maintain state between encoder/decoder
  if( rpcMbBestData->getMbDataAccess().getSH().getTCoeffLevelPredictionFlag() || rpcMbBestData->getMbDataAccess().getSH().getSCoeffResidualPredFlag() ) // SpatialResolutionChangeFlag == 0
  {
    if( ( rpcMbBestData->getMbDataAccess().getMbData().getResidualPredFlag() && ! rpcMbBestData->getMbDataAccess().getSH().getSliceSkipFlag() ) || ( rpcMbBestData->getMbDataAccess().getMbData().getMbMode() == INTRA_BL ) )
    {
      if( ( rpcMbBestData->getMbDataAccess().getMbData().getMbCbp() & 0x0F ) == 0 )
      {
        rpcMbBestData->getMbDataAccess().getMbData().setTransformSize8x8( false );
      }
    }
  }

  return Err::m_nOK;
}



ErrVal
MbEncoder::xEstimateMbIntraBL( IntMbTempData*&  rpcMbTempData,
                               IntMbTempData*&  rpcMbBestData,
                               UInt             uiMinQP,
                               UInt             uiMaxQP,
                               const Frame*     pcBaseLayerRec,
                               Bool             bBSlice,
                               MbDataAccess*    pcMbDataAccessBase )
{
  ROF  ( pcBaseLayerRec );
  ROFRS( m_bBaseModeAllowedFlag, Err::m_nOK );

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  Bool          bBLSkip           = pcMbDataAccessBase->getMbData().isIntra();
  YuvMbBuffer&  rcYuvMbBuffer     = *rpcMbTempData;
  YuvMbBuffer&  rcTempYuvMbBuffer = rpcMbTempData->getTempYuvMbBuffer();
  MbDataAccess& rcMbDataAccess    = rpcMbTempData->getMbDataAccess();

  rpcMbTempData->clear();
  rpcMbTempData->setMbMode( INTRA_BL );
  rpcMbTempData->setBLSkipFlag( bBLSkip );
  rpcMbTempData->setTransformSize8x8( false );

  if( rpcMbTempData->getSH().getTCoeffLevelPredictionFlag() )
  {
    Bool bClipValue = m_pcTransform->getClipMode();
    m_pcTransform->setClipMode( true );

    if( pcMbDataAccessBase->getMbData().getMbMode() == MODE_PCM )
    {
      RNOK( xEstimateMbPCMRewrite( rpcMbTempData, rpcMbBestData ) );
    }
    else
    {
      for( UInt uiQp = uiMinQP; uiQp <= uiMaxQP; uiQp++ )
      {
        switch( pcMbDataAccessBase->getMbData().getMbMode() )
        {
        case INTRA_4X4:
          if( pcMbDataAccessBase->getMbData().isTransformSize8x8() == false )
          {
            RNOK( xEstimateMbIntra4   ( rpcMbTempData, rpcMbBestData, uiQp, bBSlice, bBLSkip ) );
          }
          else
          {
            RNOK( xEstimateMbIntra8   ( rpcMbTempData, rpcMbBestData, uiQp, bBSlice, bBLSkip ) );
          }
          break;
        case INTRA_BL:
        case MODE_PCM:
          ROT(1);
          break;
        default:
          RNOK( xEstimateMbIntra16    ( rpcMbTempData, rpcMbBestData, uiQp, bBSlice, bBLSkip ) );
          break;
        }
      }
    }
    m_pcTransform->setClipMode( bClipValue );
    m_pcTransform->setQp      ( rcMbDataAccess, true );
    return Err::m_nOK;
  }

  rpcMbTempData->setResidualPredFlag( false );

  if( rcMbDataAccess.getSH().getSCoeffResidualPredFlag() )
  {
    pcMbDataAccessBase->getMbTCoeffs().copyPredictionTo( rcYuvMbBuffer );
  }
  else
  {
    rcYuvMbBuffer.loadBuffer( ((Frame*)pcBaseLayerRec)->getFullPelYuvBuffer() );
  }
  rcTempYuvMbBuffer.loadLuma  ( rcYuvMbBuffer );
  rcTempYuvMbBuffer.loadChroma( rcYuvMbBuffer );


  //===== LOOP OVER QP values =====
  Double            dMinCost  = DOUBLE_MAX;
  UInt              uiBestQP  = MSYS_UINT_MAX;
  UInt              uiBestCBP = MSYS_UINT_MAX;
  MbTransformCoeffs cBestCoeffs;
  YuvMbBuffer       cBestRec;
  for( UInt uiQP = uiMinQP; uiQP <= uiMaxQP; uiQP++ )
  {
    rpcMbTempData->setQp( uiQP );
    m_pcTransform->setQp( *rpcMbTempData, true );

    //===== encode residual and get rate for coefficients =====
    UInt  uiCoeffBits = 0;
    UInt  uiCoeffCost = 0;
    UInt  uiExtCbp    = 0;
    //--- LUMA ---
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      xSetCoeffCost( 0 );
      UInt  uiBits = 0;
      UInt  uiCbp  = 0;

      for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
      {
        RNOK( xEncode4x4InterBlock( *rpcMbTempData, cIdx, uiBits, uiCbp ) );
      }
      if( uiCbp )
      {
        uiCoeffCost += xGetCoeffCost();
        uiExtCbp    += uiCbp;
        uiCoeffBits += uiBits;
      }
    }
    //--- CHROMA ---
    RNOK( xEncodeChromaTexture( *rpcMbTempData, uiExtCbp, uiCoeffBits ) );
    //--- get CBP ---
    rpcMbTempData->cbp() = xCalcMbCbp( uiExtCbp );

    RNOK( xCheckSkipSliceMb( *rpcMbTempData ) );

    rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( false );
    rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( false );

    //JVT-V079 Low-complexity MB mode decision {
    if ( !bLowComplexMbEnable )
    {
      RNOK( xSetRdCostIntraMb     ( *rpcMbTempData, uiCoeffBits, bBLSkip ) );
    }
    else
    {
      RNOK( xSetRdCostIntraMb     ( *rpcMbTempData, uiCoeffBits, bBLSkip ) );
      XPel*         pPel        = rpcMbTempData->getMbLumAddr();
      Int           iStride     = rpcMbTempData->getLStride();
      UInt uiDist  = m_pcXDistortion->getLum16x16( pPel, iStride, DF_HADAMARD );

      //--- store estimated parameters ---
      rpcMbTempData->rdCost() = uiDist;
    }
    //JVT-V079 Low-complexity MB mode decision }

    //===== check rd-cost =====
    if( rpcMbTempData->rdCost() < dMinCost )
    {
      dMinCost  = rpcMbTempData->rdCost();
      uiBestQP  = rpcMbTempData->getQp      ();
      uiBestCBP = rpcMbTempData->getMbExtCbp();
      cBestCoeffs .copyFrom   (  rpcMbTempData->getMbTCoeffs() );
      cBestRec    .loadLuma   ( *rpcMbTempData );
      cBestRec    .loadChroma ( *rpcMbTempData );
    }

    //===== reset prediction signal =====
    rpcMbTempData->loadLuma   ( rcTempYuvMbBuffer );
    rpcMbTempData->loadChroma ( rcTempYuvMbBuffer );
    if( rcMbDataAccess.getSH().getSCoeffResidualPredFlag() )
    {
      pcMbDataAccessBase->getMbTCoeffs().copyPredictionTo( rcYuvMbBuffer );
    }
  }

  //===== set best data ====
  rpcMbTempData->rdCost() = dMinCost;
  rpcMbTempData->setQp        ( uiBestQP );
  rpcMbTempData->setMbExtCbp  ( uiBestCBP );
  rpcMbTempData->getMbTCoeffs ().copyFrom( cBestCoeffs );
  rpcMbTempData->loadLuma     ( cBestRec );
  rpcMbTempData->loadChroma   ( cBestRec );

  //===== reset transform status =====
  m_pcTransform->setQp( rcMbDataAccess, true );

  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  Int x,y,blockX,blockY;
	  blockX=rcMbDataAccess.getMbX()*4;
	  blockY=rcMbDataAccess.getMbY()*4;
	  Int blockIndex;
	  Int ep_ref=0;
	  Int KBlock=m_pcIntPicBuffer->getLWidth()/4;
	  Int xx,yy;
	  for(y=blockY;y<(blockY+4);y++)
	  {
		  for(x=blockX;x<(blockX+4);x++)
		  {
			  xx=(Int)(x/m_aadRatio[m_uiLayerID][0]);
			  yy=(Int)(y/m_aadRatio[m_uiLayerID][1]);
			  blockIndex=yy*(Int)(KBlock/m_aadRatio[m_uiLayerID][0])+xx;
			  ep_ref+=const_cast<Frame*>(pcBaseLayerRec)->getChannelDistortion()[blockIndex];
		  }
	  }
	  setEpRef(ep_ref);
	  rpcMbTempData->rdCost()+=ep_ref;
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation  (  rpcMbTempData, rpcMbBestData ) );
  RNOK( xEstimateMbIntraBL8x8 (  rpcMbTempData, pcMbDataAccessBase, rpcMbBestData, uiMinQP, uiMaxQP, pcBaseLayerRec, bBSlice, bBLSkip ) );

  return Err::m_nOK;
}


ErrVal
MbEncoder::xEstimateMbIntraBL8x8( IntMbTempData*&  rpcMbTempData,
                                  MbDataAccess*    pcMbDataAccessBase,
                                  IntMbTempData*&  rpcMbBestData,
                                  UInt             uiMinQP,
                                  UInt             uiMaxQP,
                                  const Frame*     pcBaseLayerRec,
                                  Bool             bBSlice,
                                  Bool             bBLSkip )
{
  ROFRS( pcBaseLayerRec,          Err::m_nOK  );
  ROFRS( m_bBaseModeAllowedFlag,  Err::m_nOK );
  ROTRS( ! rpcMbTempData->getSH().getPPS().getTransform8x8ModeFlag(), Err::m_nOK );

  rpcMbTempData->clear();
  rpcMbTempData->setMbMode( INTRA_BL );
  rpcMbTempData->setBLSkipFlag( bBLSkip );
  rpcMbTempData->setTransformSize8x8( true );

  YuvMbBuffer& rcYuvMbBuffer     = *rpcMbTempData;
  YuvMbBuffer& rcTempYuvMbBuffer =  rpcMbTempData->getTempYuvMbBuffer();

  rpcMbTempData->setResidualPredFlag( false );
  if( rpcMbTempData->getSH().getSCoeffResidualPredFlag() )
  {
    pcMbDataAccessBase->getMbTCoeffs().copyPredictionTo( rcYuvMbBuffer );
  }
  else
  {
    rcYuvMbBuffer    .loadBuffer( ((Frame*)pcBaseLayerRec)->getFullPelYuvBuffer() );
  }

  rcTempYuvMbBuffer.loadLuma  ( rcYuvMbBuffer );
  rcTempYuvMbBuffer.loadChroma( rcYuvMbBuffer );


  //===== LOOP OVER QP values =====
  Double            dMinCost  = DOUBLE_MAX;
  UInt              uiBestQP  = MSYS_UINT_MAX;
  UInt              uiBestCBP = MSYS_UINT_MAX;
  MbTransformCoeffs cBestCoeffs;
  YuvMbBuffer       cBestRec;
  for( UInt uiQP = uiMinQP; uiQP <= uiMaxQP; uiQP++ )
  {
    rpcMbTempData->setQp( uiQP );
    m_pcTransform->setQp( *rpcMbTempData, true );

    //===== encode residual and get rate for coefficients =====
    UInt  uiCoeffBits = 0;
    UInt  uiCoeffCost = 0;
    UInt  uiExtCbp    = 0;
    //--- LUMA ---
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      xSetCoeffCost( 0 );
      UInt  uiBits = 0;
      UInt  uiCbp  = 0;

      RNOK( xEncode8x8InterBlock( *rpcMbTempData, c8x8Idx, uiBits, uiCbp ) );
      if( uiCbp )
      {
        uiCoeffCost += xGetCoeffCost();
        uiExtCbp    += uiCbp;
        uiCoeffBits += uiBits;
      }
    }
    //--- CHROMA ---
    RNOK( xEncodeChromaTexture( *rpcMbTempData, uiExtCbp, uiCoeffBits ) );
    //--- get CBP ---
    rpcMbTempData->cbp() = xCalcMbCbp( uiExtCbp );

    RNOK( xCheckSkipSliceMb( *rpcMbTempData ) );

    rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( false );
    rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( false );

    RNOK( xSetRdCostIntraMb( *rpcMbTempData, uiCoeffBits, bBLSkip ) );

    //===== check rd-cost =====
    if( rpcMbTempData->rdCost() < dMinCost )
    {
      dMinCost  = rpcMbTempData->rdCost();
      uiBestQP  = rpcMbTempData->getQp      ();
      uiBestCBP = rpcMbTempData->getMbExtCbp();
      cBestCoeffs .copyFrom   (  rpcMbTempData->getMbTCoeffs() );
      cBestRec    .loadLuma   ( *rpcMbTempData );
      cBestRec    .loadChroma ( *rpcMbTempData );
    }

    //===== reset prediction signal =====
    rpcMbTempData->loadLuma   ( rcTempYuvMbBuffer );
    rpcMbTempData->loadChroma ( rcTempYuvMbBuffer );
    if( rpcMbTempData->getSH().getSCoeffResidualPredFlag() )
    {
      pcMbDataAccessBase->getMbTCoeffs().copyPredictionTo( rcYuvMbBuffer );
    }
  }

  //===== set best data ====
  rpcMbTempData->rdCost() = dMinCost;
  rpcMbTempData->setQp        ( uiBestQP );
  rpcMbTempData->setMbExtCbp  ( uiBestCBP );
  rpcMbTempData->getMbTCoeffs ().copyFrom( cBestCoeffs );
  rpcMbTempData->loadLuma     ( cBestRec );
  rpcMbTempData->loadChroma   ( cBestRec );

  //===== reset transform status =====
  m_pcTransform->setQp( rpcMbTempData->getMbDataAccess(), true );

  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  rpcMbTempData->rdCost()+=getEpRef();
  }
  //JVT-R057 LA-RDO}

  ROT( rpcMbTempData->getSH().getTCoeffLevelPredictionFlag() );
  if( !rpcMbTempData->getMbDataAccess().isSCoeffPred() )
  {
    RNOK( xCheckBestEstimation( rpcMbTempData, rpcMbBestData ) );
  }
  else if( pcMbDataAccessBase->getMbData().isTransformSize8x8() )
  {
    IntMbTempData *pcSwitchPtr = rpcMbTempData;
    rpcMbTempData = rpcMbBestData;
    rpcMbBestData = pcSwitchPtr;
  }
  return Err::m_nOK;
}





ErrVal
MbEncoder::xEstimateMbIntra16( IntMbTempData*&  rpcMbTempData,
                               IntMbTempData*&  rpcMbBestData,
                               UInt             uiQp,
                               Bool             bBSlice,
							                 Bool             bBLSkip
                               )
{
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag() && !bBLSkip, Err::m_nOK );

  rpcMbTempData->clear();
  rpcMbTempData->setBLSkipFlag( bBLSkip );
  rpcMbTempData->setQp( uiQp );
  m_pcTransform->setQp( *rpcMbTempData, true );

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  //----- init intra prediction -----
  rpcMbTempData->loadIntraPredictors( m_pcIntPicBuffer );
  rpcMbTempData->clearIntraPredictionModes( false );
  m_pcIntraPrediction->setAvailableMaskMb( rpcMbTempData->getMbDataAccess().getAvailableMask() );

  Int           iScalMat    = 0;
  const UChar*  pucScale    = ( rpcMbTempData->getSH().isScalingMatrixPresent(iScalMat) ? rpcMbTempData->getSH().getScalingMatrix(iScalMat) : NULL );
  XPel*         pPel        = rpcMbTempData->getMbLumAddr();
  Int           iStride     = rpcMbTempData->getLStride();
  UInt          uiPredMode  = 0;
  UInt          uiBestRd    = MSYS_UINT_MAX;
  UInt          uiBestBits  = 0;
  UInt          uiDist, uiBits = 0, uiRd, uiCost;

  // Make sure the uiPredMode is equal to the baselayer when AVC rewriting is enabled.
  Bool avcRewriteFlag = rpcMbTempData->getMbDataAccess().getSH().getTCoeffLevelPredictionFlag();
  if( avcRewriteFlag && bBLSkip )
    uiPredMode = rpcMbTempData->getMbDataAccess().getMbDataAccessBase()->getMbData().intraPredMode();

  for( Int n = 0; n < 4; n++ )
  {
    Bool bValid = true;
    RNOK( m_pcIntraPrediction->predictSLumaMb( rpcMbTempData, n, bValid ) );

    if( avcRewriteFlag && bBLSkip )
    {
      UInt ui_BLintraPredMode = rpcMbTempData->getMbDataAccess().getMbDataAccessBase()->getMbData().intraPredMode();
      bValid = ( n==ui_BLintraPredMode ) ? bValid : false;
    }

    if( ! bValid )
    {
      continue;
    }

    UInt  uiDcAbs = 0;
    UInt  uiAcAbs = 0;
    if ( !bLowComplexMbEnable )
    {

      RNOK( m_pcTransform->transformMb16x16( m_pcIntOrgMbPelData, rpcMbTempData, rpcMbTempData->get( B4x4Idx(0) ), pucScale, uiDcAbs, uiAcAbs ) );
      uiDist = m_pcXDistortion->getLum16x16( pPel, iStride );

      if( avcRewriteFlag && bBLSkip )
      {
        // Note: This currently makes uiDcAbs and uiAcAbs meaningless
        m_pcTransform->predictMb16x16( rpcMbTempData->get( B4x4Idx(0) ),
          rpcMbTempData->getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( B4x4Idx(0) ),
          rpcMbTempData->getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
          uiDcAbs, uiAcAbs );
      }

      BitCounter::init();

      ROF( rpcMbTempData->getMbDataAccess().getMbData().getMbMode() == MODE_SKIP );
      rpcMbTempData->getMbDataAccess().getMbData().setMbMode( MbMode(INTRA_4X4 + 1) ); // one arbitrary Intra 16x16 mode

      if( avcRewriteFlag && bBLSkip )
      {
        for( B4x4Idx b4x4Idx; b4x4Idx.isLegal(); b4x4Idx++ )
          RNOK( MbCoder::xScanLumaBlock( *rpcMbTempData, *rpcMbTempData, b4x4Idx ) );
      }
      else
      {
        RNOK( MbCoder::xScanLumaIntra16x16( *rpcMbTempData, *rpcMbTempData, uiAcAbs != 0 ) );
      }

      rpcMbTempData->getMbDataAccess().getMbData().setMbMode( MODE_SKIP );

      uiBits = BitCounter::getNumberOfWrittenBits();

      uiRd = (UInt)floor(m_pcRateDistortionIf->getCost( uiBits, uiDist ));
    }
    else
     uiRd = uiDist = m_pcXDistortion->getLum16x16( pPel, iStride, DF_HADAMARD );


    if( uiRd < uiBestRd )
    {
      uiBestRd   = uiRd;
      uiPredMode = n;
      uiBestBits = uiBits;
    }
  }

  Bool  bValid  = true;
  UInt  uiDcAbs = 0;
  UInt  uiAcAbs = 0;
  RNOK( m_pcIntraPrediction->predictSLumaMb( rpcMbTempData, uiPredMode, bValid ) );

  if (avcRewriteFlag && !bValid && bBLSkip)
  {
    return Err::m_nOK;    // no valid mode is found
  }

  rpcMbTempData->getTempYuvMbBuffer().loadLuma( *rpcMbTempData );

  RNOK( m_pcTransform->transformMb16x16( m_pcIntOrgMbPelData, rpcMbTempData, rpcMbTempData->get( B4x4Idx(0) ), pucScale, uiDcAbs, uiAcAbs ) );

  UInt uiExtCbp = 0;
  if( avcRewriteFlag && bBLSkip )
  {
	  m_pcTransform->predictMb16x16( rpcMbTempData->get( B4x4Idx(0) ),
		  rpcMbTempData->getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( B4x4Idx(0) ),
		  rpcMbTempData->getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
		  uiDcAbs, uiAcAbs );
    RNOK( xCheckSkipSliceMbIntra16( *rpcMbTempData, uiAcAbs ) );

	  // Note: uiDcAbs and uiAcAbs are now meaningless

	  // Creat a new ExtCbp
	  for( B4x4Idx b4x4Idx; b4x4Idx.isLegal(); b4x4Idx++ )
	  {
		  TCoeff* pcTCoeff = rpcMbTempData->get( b4x4Idx );
		  for( UInt n=0; n<16; n++ )
			  if( pcTCoeff[n]!=0 )
			  {
				  uiExtCbp |= 1 << b4x4Idx;
				  break;
			  }
	  }
  }

  if ( !bLowComplexMbEnable )
  {
   UInt    uiMBits = ( bBSlice ? 9 : 5 ) + 2;
   const DFunc&  rcDFunc = m_pcCodingParameter->getMotionVectorSearchParams().getSubPelDFunc();
   uiDist  = m_pcXDistortion->getLum16x16( pPel, iStride, rcDFunc );
   uiCost  = uiDist + m_pcMotionEstimation->getRateCost( uiBestBits + uiMBits, rcDFunc == DF_SAD );
  }
  else
  {
   uiCost = uiBestRd;
  }

  UInt uiChromaCbp = 0;
  if( avcRewriteFlag && bBLSkip )
	  rpcMbTempData->setMbMode( INTRA_BL );
  else
  rpcMbTempData->setMbMode( INTRA_4X4 ); // for right scaling matrix
  RNOK( xEncodeChromaIntra( *rpcMbTempData, uiChromaCbp, uiBestBits, bLowComplexMbEnable ) );

  if( avcRewriteFlag && (uiBestBits == MSYS_UINT_MAX ))
    return Err::m_nOK;    // no valid mode is found

  UInt  uiMbType  = INTRA_4X4 + 1;
        uiMbType += uiPredMode;
        uiMbType += ( uiAcAbs != 0 ) ? 12 : 0;
        uiMbType += uiChromaCbp >> 14;

  // needed for CABAC
  if (avcRewriteFlag)
  {
    if( bBLSkip )
      rpcMbTempData->cbp()  =   xCalcMbCbp( uiExtCbp + uiChromaCbp );
    else
      rpcMbTempData->cbp()  =   xCalcMbCbp( ( uiAcAbs != 0 ? 0xFFFF : 0 ) + uiChromaCbp );

    rpcMbTempData->setMbMode( bBLSkip ? INTRA_BL : MbMode( uiMbType ) );
    rpcMbTempData->setBLSkipFlag( bBLSkip );
  }
  else
  {
    rpcMbTempData->cbp()  =   xCalcMbCbp( ( uiAcAbs != 0 ? 0xFFFF : 0 ) + uiChromaCbp );
    rpcMbTempData->setMbMode( MbMode( uiMbType ) );
    rpcMbTempData->setBLSkipFlag( false );
  }

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( false );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( false );

  //--- store estimated parameters ---
  rpcMbTempData->rdCost() = uiCost;

  if ( !bLowComplexMbEnable )
   RNOK( xSetRdCostIntraMb   ( *rpcMbTempData, uiBestBits, false ) );

  if( avcRewriteFlag && bBLSkip && rpcMbTempData->getMbCbp() == 0 && ! rpcMbTempData->getSH().getSliceSkipFlag() )
  {
    // we have to set the QP to the BL QP (see G.8.1.5.1.2). Since it is very unlikely to happen,
    // we simply forbid this mode
    // !!! this is not optimal !!!
    rpcMbTempData->rdCost() = rpcMbBestData->rdCost();
  }

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}



ErrVal
MbEncoder::xEstimateMbIntra4( IntMbTempData*&  rpcMbTempData,
                              IntMbTempData*&  rpcMbBestData,
                              UInt             uiQp,
                              Bool             bBSlice,
                              Bool             bBLSkip )
{
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag() && !bBLSkip, Err::m_nOK );

  /* JVT-V079: parameter rcMbDataAccess used to catch most probable intra pred mode */
  rpcMbTempData->clear();
  rpcMbTempData->setQp( uiQp );
  m_pcTransform->setQp( *rpcMbTempData, true );

  Bool avcRewriteFlag = rpcMbTempData->getSH().getTCoeffLevelPredictionFlag();
  if (avcRewriteFlag)
  {
    rpcMbTempData->setMbMode( bBLSkip?INTRA_BL:INTRA_4X4 );
    rpcMbTempData->setBLSkipFlag( bBLSkip );
  }
  else
  {
    rpcMbTempData->setMbMode( INTRA_4X4 );
    rpcMbTempData->setBLSkipFlag( false );
  }

  MbDataAccess& rcMbDataAccess      = rpcMbTempData->getMbDataAccess();
  UInt          uiCost, uiTempCost  = 0;

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  //----- init intra prediction -----
  rpcMbTempData->loadIntraPredictors( m_pcIntPicBuffer );
  rpcMbTempData->clearIntraPredictionModes( false );
  m_pcIntraPrediction->setAvailableMaskMb( rpcMbTempData->getMbDataAccess().getAvailableMask() );


  UInt uiExtCbp = 0;
  UInt uiMbBits = 0;

  Int tmp_max_qp  = ( uiQp - SHIFT_QP );
  if ( tmp_max_qp<0) tmp_max_qp=0;
  UInt lambda_val = 4 * g_iQP2quant[tmp_max_qp];

  if ( bLowComplexMbEnable )
  {
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      UInt uiBits = 0;
      UInt uiCbp = 0;
      for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
      {
        Int mpMode = rcMbDataAccess.mostProbableIntraPredMode(LumaIdx(cIdx));
        RNOK( xEncode4x4IntraBlock( *rpcMbTempData, cIdx, uiBits, uiCbp,
              mpMode, lambda_val, bLowComplexMbEnable ) );

        if( avcRewriteFlag && (uiBits == MSYS_UINT_MAX ))
          return Err::m_nOK;    // no valid mode is found

        rpcMbTempData->copyTo     ( rcMbDataAccess );
        uiTempCost += uiBits;
      }
      uiTempCost += 6 * g_iQP2quant[tmp_max_qp];

      if( uiCbp != 0 )
      {
        uiMbBits += uiBits;
        uiExtCbp += uiCbp;
      }
      else
      {
        uiMbBits += uiBits-4;
      }
    }
  }
  else
  {
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      UInt uiBits = 0;
      UInt uiCbp = 0;
      for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
      {
        RNOK( xEncode4x4IntraBlock( *rpcMbTempData, cIdx, uiBits, uiCbp, 0, 0, false ) );
        if( avcRewriteFlag && (uiBits == MSYS_UINT_MAX ))
          return Err::m_nOK;    // no valid mode is found
      }

      if( uiCbp != 0 )
      {
        uiMbBits += uiBits;
        uiExtCbp += uiCbp;
      }
      else
      {
        uiMbBits += uiBits-4;
      }
    }
  }

  if ( !bLowComplexMbEnable )
  {
    XPel*         pPel    = rpcMbTempData->getMbLumAddr();
    Int           iStride = rpcMbTempData->getLStride();
    const DFunc&  rcDFunc = m_pcCodingParameter->getMotionVectorSearchParams().getSubPelDFunc();
    UInt          uiDist  = m_pcXDistortion->getLum16x16( pPel, iStride, rcDFunc );
    UInt          uiMBits = ( bBSlice ? 9 : 5 );
                  uiCost  = uiDist + m_pcMotionEstimation->getRateCost( uiMbBits + uiMBits, rcDFunc == DF_SAD );
  }
  else
    uiCost = uiTempCost;

  RNOK( xEncodeChromaIntra( *rpcMbTempData, uiExtCbp, uiMbBits, bLowComplexMbEnable ) );

  if( avcRewriteFlag && (uiMbBits == MSYS_UINT_MAX ))
    return Err::m_nOK;    	// no valid mode is found

  //--- store estimated parameters ---
  rpcMbTempData->rdCost() = uiCost;
  rpcMbTempData->cbp()    = xCalcMbCbp( uiExtCbp );

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( false );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( false );

  if ( !bLowComplexMbEnable )
    RNOK( xSetRdCostIntraMb( *rpcMbTempData, uiMbBits, false ) );

  if( avcRewriteFlag && bBLSkip && rpcMbTempData->getMbCbp() == 0 && ! rpcMbTempData->getSH().getSliceSkipFlag() )
  {
    // we have to set the QP to the BL QP (see G.8.1.5.1.2). Since it is very unlikely to happen,
    // we simply forbid this mode
    // !!! this is not optimal !!!
    rpcMbTempData->rdCost() = rpcMbBestData->rdCost();
  }

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}



ErrVal
MbEncoder::xEstimateMbIntra8( IntMbTempData*& rpcMbTempData,
                              IntMbTempData*& rpcMbBestData,
                              UInt            uiQp,
                              Bool            bBSlice,
                              Bool            bBLSkip
                              )
{
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag() && !bBLSkip, Err::m_nOK );

  ROTRS( ! rpcMbTempData->getSH().getPPS().getTransform8x8ModeFlag(), Err::m_nOK );

  rpcMbTempData->clear();
  rpcMbTempData->setQp( uiQp );
  m_pcTransform->setQp( *rpcMbTempData, true );

  Bool avcRewriteFlag = rpcMbTempData->getSH().getTCoeffLevelPredictionFlag();
  if (avcRewriteFlag)
  {
    rpcMbTempData->setMbMode( bBLSkip?INTRA_BL:INTRA_4X4 );
    rpcMbTempData->setBLSkipFlag( bBLSkip );
  }
  else
  {
    rpcMbTempData->setMbMode( INTRA_4X4 );
    rpcMbTempData->setBLSkipFlag( false );
  }

  rpcMbTempData->setTransformSize8x8( true );

  //----- init intra prediction -----
  rpcMbTempData->loadIntraPredictors( m_pcIntPicBuffer );
  rpcMbTempData->clearIntraPredictionModes( true );
  m_pcIntraPrediction->setAvailableMaskMb( rpcMbTempData->getMbDataAccess().getAvailableMask() );

  UInt uiExtCbp = 0;
  UInt uiMbBits = 0;

  for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
  {
    RNOK( xEncode8x8IntraBlock( *rpcMbTempData, c8x8Idx, uiMbBits, uiExtCbp ) );
    if( avcRewriteFlag && (uiMbBits == MSYS_UINT_MAX ))
      return Err::m_nOK;  // no valid mode is found
  }

  XPel*         pPel      = rpcMbTempData->getMbLumAddr();
  Int           iStride   = rpcMbTempData->getLStride();
  const DFunc&  rcDFunc   = m_pcCodingParameter->getMotionVectorSearchParams().getSubPelDFunc();
  UInt          uiDist    = m_pcXDistortion->getLum16x16( pPel, iStride, rcDFunc );
  UInt          uiMBits   = ( bBSlice ? 9 : 5 );
  UInt          uiCost    = uiDist + m_pcMotionEstimation->getRateCost( uiMbBits + uiMBits, rcDFunc == DF_SAD );

  RNOK( xEncodeChromaIntra( *rpcMbTempData, uiExtCbp, uiMbBits ) );

  if( avcRewriteFlag && (uiMbBits == MSYS_UINT_MAX ))
    return Err::m_nOK;    // no valid mode is found

  //----- store estimated parameters -----
  rpcMbTempData->rdCost() = uiCost;
  rpcMbTempData->cbp()    = xCalcMbCbp( uiExtCbp );

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( false );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( false );

  RNOK( xSetRdCostIntraMb( *rpcMbTempData, uiMbBits, false ) );

  if( avcRewriteFlag && bBLSkip && rpcMbTempData->getMbCbp() == 0 && ! rpcMbTempData->getSH().getSliceSkipFlag() )
  {
    // we have to set the QP to the BL QP (see G.8.1.5.1.2). Since it is very unlikely to happen,
    // we simply forbid this mode
    // !!! this is not optimal !!!
    rpcMbTempData->rdCost() = rpcMbBestData->rdCost();
  }

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}


ErrVal
MbEncoder::xEstimateMbPCM( IntMbTempData*&   rpcMbTempData,
                           IntMbTempData*&   rpcMbBestData,
                           Bool              bBSlice )
{
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag(), Err::m_nOK );

  rpcMbTempData->clear        ();
  rpcMbTempData->setMbMode    ( MODE_PCM );
  rpcMbTempData->setQp        ( rpcMbTempData->getMbDataAccess().getLastQp() );
  rpcMbTempData->setBLSkipFlag( false );
  rpcMbTempData->loadLuma     ( *m_pcIntOrgMbPelData );
  rpcMbTempData->loadChroma   ( *m_pcIntOrgMbPelData );

  TCoeff* pCoeff    = rpcMbTempData->getTCoeffBuffer();
  XPel*   pucSrc    = rpcMbTempData->getMbLumAddr();
  Int     iStride   = rpcMbTempData->getLStride();
  UInt    uiDist    = 0;
  UInt    uiMbBits  = 8*8*8*6+(bBSlice?11:9)+4;
  Int     n, m, dest, diff;

  for( n = 0; n < 16; n++ )
  {
    for( m = 0; m < 16; m++ )
    {
      dest       = gMin( 255, gMax( 1, pucSrc[m] ) );
      pCoeff->setLevel( dest );
      *pCoeff++;
      diff       = pucSrc[m] - dest;
      pucSrc[m]  = dest;
      uiDist    += diff * diff;
    }
    pucSrc  += iStride;
  }

  pucSrc  = rpcMbTempData->getMbCbAddr();
  iStride = rpcMbTempData->getCStride();

  for( n = 0; n < 8; n++ )
  {
    for( m = 0; m < 8; m++ )
    {
      dest       = gMin( 255, gMax( 1, pucSrc[m] ) );
      pCoeff->setLevel( dest );
      *pCoeff++;
      diff       = pucSrc[m] - dest;
      pucSrc[m]  = dest;
      uiDist    += diff * diff;
    }
    pucSrc  += iStride;
  }

  pucSrc  = rpcMbTempData->getMbCrAddr();

  for( n = 0; n < 8; n++ )
  {
    for( m = 0; m < 8; m++ )
    {
      dest       = gMin( 255, gMax( 1, pucSrc[m] ) );
      pCoeff->setLevel( dest );
      *pCoeff++;
      diff       = pucSrc[m] - dest;
      pucSrc[m]  = dest;
      uiDist    += diff * diff;
    }
    pucSrc  += iStride;
  }

  rpcMbTempData->getTempYuvMbBuffer().setAllSamplesToZero();

  const DFunc&  rcDFunc   = m_pcCodingParameter->getMotionVectorSearchParams().getSubPelDFunc();
  rpcMbTempData->rdCost() = uiDist + m_pcMotionEstimation->getRateCost( uiMbBits, rcDFunc == DF_SAD );
  rpcMbTempData->cbp()    = 0;

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( false );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( false );

  rpcMbTempData->rdCost() = m_pcRateDistortionIf->getCost( uiMbBits, uiDist );

  if( ( rand() % 100 ) < (Int)m_uiIPCMRate ) // force IPCM rate
  {
    rpcMbTempData->rdCost() = 0;
  }

  RNOK( xCheckBestEstimation( rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}


ErrVal
MbEncoder::xEstimateMbPCMRewrite( IntMbTempData*&   rpcMbTempData,
                                  IntMbTempData*&   rpcMbBestData )
{
  ROF( rpcMbTempData->getSH().getTCoeffLevelPredictionFlag() );

  rpcMbTempData->clear        ();
  rpcMbTempData->setMbMode    ( INTRA_BL );
  rpcMbTempData->setQp        ( rpcMbTempData->getMbDataAccess().getLastQp() );
  rpcMbTempData->setBLSkipFlag( true );
  rpcMbTempData->loadLuma     ( *m_pcIntOrgMbPelData );
  rpcMbTempData->loadChroma   ( *m_pcIntOrgMbPelData );

  TCoeff* pRefCoeff = rpcMbTempData->getMbDataAccess().getMbDataAccessBase()->getMbData().getMbTCoeffs().getTCoeffBuffer();
  XPel*   pucSrc    = rpcMbTempData->getMbLumAddr();
  Int     iStride   = rpcMbTempData->getLStride();
  UInt    uiDist    = 0;
  UInt    uiMbBits  = 4;
  Int     n, m, diff;

  for( n = 0; n < 16; n++ )
  {
    for( m = 0; m < 16; m++ )
    {
      diff       = pucSrc[m] - pRefCoeff->getLevel();
      pucSrc[m]  = pRefCoeff->getLevel();
      uiDist    += diff * diff;
      *pRefCoeff++;
    }
    pucSrc  += iStride;
  }

  pucSrc  = rpcMbTempData->getMbCbAddr();
  iStride = rpcMbTempData->getCStride();

  for( n = 0; n < 8; n++ )
  {
    for( m = 0; m < 8; m++ )
    {
      diff       = pucSrc[m] - pRefCoeff->getLevel();
      pucSrc[m]  = pRefCoeff->getLevel();
      uiDist    += diff * diff;
      *pRefCoeff++;
    }
    pucSrc  += iStride;
  }

  pucSrc  = rpcMbTempData->getMbCrAddr();

  for( n = 0; n < 8; n++ )
  {
    for( m = 0; m < 8; m++ )
    {
      diff       = pucSrc[m] - pRefCoeff->getLevel();
      pucSrc[m]  = pRefCoeff->getLevel();
      uiDist    += diff * diff;
      *pRefCoeff++;
    }
    pucSrc  += iStride;
  }

  rpcMbTempData->getTempYuvMbBuffer().setAllSamplesToZero();

  const DFunc&  rcDFunc   = m_pcCodingParameter->getMotionVectorSearchParams().getSubPelDFunc();
  rpcMbTempData->rdCost() = uiDist + m_pcMotionEstimation->getRateCost( uiMbBits, rcDFunc == DF_SAD );
  rpcMbTempData->cbp()    = 0;

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( false );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( false );

  rpcMbTempData->rdCost() = m_pcRateDistortionIf->getCost( uiMbBits, uiDist );

  RNOK( xCheckBestEstimation( rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}




UInt MbEncoder::xCalcMbCbp( UInt uiExtCbp )
{
  UInt uiMbCbp;
  {
    UInt uiCbp = uiExtCbp;
    uiMbCbp  = (0 != (uiCbp & 0x33)) ? 1 : 0;
    uiMbCbp += (0 != (uiCbp & 0xcc)) ? 2 : 0;
    uiCbp >>= 8;
    uiMbCbp += (0 != (uiCbp & 0x33)) ? 4 : 0;
    uiMbCbp += (0 != (uiCbp & 0xcc)) ? 8 : 0;
  }
  uiMbCbp += (uiExtCbp >> 16) << 4;

  return (uiMbCbp<<24) | uiExtCbp;
}





ErrVal
MbEncoder::xEncode4x4InterBlock( IntMbTempData& rcMbTempData,
                                 LumaIdx        cIdx,
                                 UInt&          ruiBits,
                                 UInt&          ruiExtCbp )
{
  rcMbTempData.set4x4Block( cIdx );
  m_pcIntOrgMbPelData->set4x4Block( cIdx );

  UInt uiBits = 0;
  UInt uiAbsSum = 0;

  Int           iScalMat  = ( rcMbTempData.isIntra() ? 0 : 3 );
  const UChar*  pucScale  = ( rcMbTempData.getSH().isScalingMatrixPresent(iScalMat) ? rcMbTempData.getSH().getScalingMatrix(iScalMat) : NULL );

  if( rcMbTempData.getMbDataAccess().isSCoeffPred() )
  {
		TCoeff* piCoeffBase = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( cIdx );
    RNOK( m_pcTransform->transform4x4BlkCGS( m_pcIntOrgMbPelData, rcMbTempData, rcMbTempData.get( cIdx ), piCoeffBase, pucScale, uiAbsSum ) );
  }
  else
    RNOK( m_pcTransform->transform4x4Blk( m_pcIntOrgMbPelData, rcMbTempData, rcMbTempData.get( cIdx ), pucScale, uiAbsSum ) );

  if( rcMbTempData.getMbDataAccess().isTCoeffPred() )
  {
	  RNOK( m_pcTransform->predict4x4Blk( rcMbTempData.get( cIdx ),
		  rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( cIdx ),
		  rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
		  uiAbsSum ) );
  }

  if( 0 == uiAbsSum )
  {
    ruiBits += 1;
    return Err::m_nOK;
  }

  BitCounter::init();
  RNOK( MbCoder::xScanLumaBlock( rcMbTempData, rcMbTempData, cIdx ) );
  uiBits = BitCounter::getNumberOfWrittenBits();
  AOT_DBG( uiBits == 0);

  ruiExtCbp |= 1 << cIdx;
  ruiBits   += uiBits;

  return Err::m_nOK;
}



ErrVal
MbEncoder::xEncode8x8InterBlock( IntMbTempData& rcMbTempData,
                                 B8x8Idx        c8x8Idx,
                                 UInt&          ruiBits,
                                 UInt&          ruiExtCbp )
{
  rcMbTempData.set4x4Block( c8x8Idx );
  m_pcIntOrgMbPelData->set4x4Block( c8x8Idx );

  UInt uiBits     = 0;
  UInt uiAbsSum   = 0;

  Int           iScalMat  = ( rcMbTempData.isIntra() ? 6 : 7 );
  const UChar*  pucScale  = ( rcMbTempData.getSH().isScalingMatrixPresent(iScalMat) ? rcMbTempData.getSH().getScalingMatrix(iScalMat) : NULL );

  if ( rcMbTempData.getMbDataAccess().isSCoeffPred() )
  {
		TCoeff* piCoeffBase = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get8x8( c8x8Idx );
    RNOK( m_pcTransform->transform8x8BlkCGS( m_pcIntOrgMbPelData, rcMbTempData, rcMbTempData.get8x8( c8x8Idx ), piCoeffBase, pucScale, uiAbsSum ) );
  }
  else
    RNOK( m_pcTransform->transform8x8Blk( m_pcIntOrgMbPelData, rcMbTempData, rcMbTempData.get8x8( c8x8Idx ), pucScale, uiAbsSum ) );

  if( rcMbTempData.getMbDataAccess().isTCoeffPred() )
  {
	  RNOK( m_pcTransform->predict8x8Blk( rcMbTempData.get8x8( c8x8Idx ),
		  rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get8x8( c8x8Idx ),
		  rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
		  uiAbsSum ) );
  }

  if( 0 == uiAbsSum )
  {
    ruiBits += 1;
    return Err::m_nOK;
  }

  {
    BitCounter::init();
    RNOK( m_pcMbSymbolWriteIf->residualBlock8x8( rcMbTempData, c8x8Idx, LUMA_SCAN ) )
    uiBits = ( BitCounter::getNumberOfWrittenBits() );
    AOT_DBG( uiBits == 0);
  }

  ruiExtCbp |= 0x33 << c8x8Idx.b4x4();
  ruiBits   += uiBits;

  return Err::m_nOK;
}



ErrVal
MbEncoder::xEncode8x8IntraBlock( IntMbTempData& rcMbTempData,
                                 B8x8Idx        c8x8Idx,
                                 UInt&          ruiBits,
                                 UInt&          ruiExtCbp )
{
  rcMbTempData.        set4x4Block( c8x8Idx );
  m_pcIntOrgMbPelData->set4x4Block( c8x8Idx );

  UInt   uiAbsSum;
  Double fBestRd     = DOUBLE_MAX;
  UInt   uiBestMode  = 2;
  UInt   uiBestBits  = 0;
  Int    iPredMode   = 0;
  Bool   bValid      = true;

  Bool avcRewriteFlag = rcMbTempData.getSH().getTCoeffLevelPredictionFlag();
  if( avcRewriteFlag && rcMbTempData.isIntraBL() )
	  rcMbTempData.intraPredMode( c8x8Idx ) = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().intraPredMode( c8x8Idx );

  Int           iScalMat  = 6;
  const UChar*  pucScale  = ( rcMbTempData.getSH().isScalingMatrixPresent(iScalMat) ? rcMbTempData.getSH().getScalingMatrix(iScalMat) : NULL );

  for( UInt n = 0; n < 9; n++)
  {
    bValid    = true;
    iPredMode = n;

    if( avcRewriteFlag && rcMbTempData.isIntraBL() && bValid )
    {
      bValid = (n==rcMbTempData.intraPredMode( c8x8Idx ) );
    }

    if( bValid )
    {
      RNOK( m_pcIntraPrediction->predictSLumaBlock8x8( rcMbTempData, iPredMode, c8x8Idx, bValid ) );
    }

    if( bValid )
    {
      RNOK( m_pcTransform->transform8x8Blk( m_pcIntOrgMbPelData, rcMbTempData, rcMbTempData.get8x8( c8x8Idx ), pucScale, uiAbsSum = 0 ) );
      UInt uiDist = m_pcXDistortion->getLum8x8( rcMbTempData.getLumBlk(), rcMbTempData.getLStride() );

      Int uiBits = 0;

      if( avcRewriteFlag && rcMbTempData.isIntraBL() )
      {
        RNOK( m_pcTransform->predict8x8Blk( rcMbTempData.get8x8( c8x8Idx ),
          rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get8x8( c8x8Idx ),
          rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
          uiAbsSum ) );
      }
      else
      {
        rcMbTempData.intraPredMode(c8x8Idx) = iPredMode;

        BitCounter::init();
        m_pcMbSymbolWriteIf->intraPredModeLuma( rcMbTempData, c8x8Idx );
        uiBits = BitCounter::getNumberOfWrittenBits() + 1;
      }

      if( 0 != uiAbsSum )
      {
        BitCounter::init();
        RNOK( m_pcMbSymbolWriteIf->residualBlock8x8( rcMbTempData, c8x8Idx, LUMA_SCAN ) )
        uiBits += ( BitCounter::getNumberOfWrittenBits() ) - 1;
      }

      Double fCost = m_pcRateDistortionIf->getFCost( uiBits, uiDist );

      if( fCost < fBestRd )
      {
        fBestRd     = fCost;
        uiBestBits  = uiBits;
        uiBestMode  = iPredMode;
      }
    }
  }

  RNOK( m_pcIntraPrediction->predictSLumaBlock8x8( rcMbTempData, uiBestMode, c8x8Idx, bValid ) );
  {
    S4x4Idx cIdx4x4(c8x8Idx);
    for( Int n = 0; n < 4; n++, cIdx4x4++ )
    {
      rcMbTempData.intraPredMode( cIdx4x4 ) = uiBestMode;
    }
  }

  rcMbTempData.getTempYuvMbBuffer().loadLuma( rcMbTempData, c8x8Idx );

  RNOK( m_pcTransform->transform8x8Blk( m_pcIntOrgMbPelData, rcMbTempData, rcMbTempData.get8x8( c8x8Idx ), pucScale, uiAbsSum = 0 ) );

  if( avcRewriteFlag && rcMbTempData.isIntraBL() )
  {
	  RNOK( m_pcTransform->predict8x8Blk( rcMbTempData.get8x8( c8x8Idx ),
		  rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get8x8( c8x8Idx ),
		  rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
		  uiAbsSum ) );
    RNOK( xCheckSkipSliceMbIntra8( rcMbTempData, c8x8Idx, uiAbsSum ) );
  }

  if( 0 != uiAbsSum )
  {
    ruiExtCbp |= 0x33 << c8x8Idx.b4x4();
  }

  if ( avcRewriteFlag && rcMbTempData.isIntraBL() && rcMbTempData.getMbDataAccess().getMbDataAccessBase() )
  {
    if( uiBestMode != rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().intraPredMode( c8x8Idx ) )
      ruiBits = MSYS_UINT_MAX;  // mode is different from base layer, this is not allowed when avc rewriting is enabled
  }

  if( ruiBits != MSYS_UINT_MAX )
    ruiBits += uiBestBits;

  return Err::m_nOK;
}





ErrVal
MbEncoder::xEncode4x4IntraBlock( IntMbTempData& rcMbTempData,
                                 LumaIdx        cIdx,
                                 UInt&          ruiBits,
                                 UInt&          ruiExtCbp,
                                 UInt           mpMode,
                                 UInt           lambda_val,
                                 Bool           bLowComplexity )
{
  rcMbTempData.        set4x4Block( cIdx );
  m_pcIntOrgMbPelData->set4x4Block( cIdx );
  UInt uiAbsSum;
  Double fCost;
  UInt uiDist, uiBits=0;

  Double fBestRd = DOUBLE_MAX;
  UInt uiBestMode = 2;
  UInt uiBestBits = 0;
  Int iPredMode = 0;
  Bool bValid = true;

  Bool avcRewriteFlag = rcMbTempData.getSH().getTCoeffLevelPredictionFlag();
  if( avcRewriteFlag && rcMbTempData.isIntraBL() )
	  rcMbTempData.intraPredMode(cIdx) = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().intraPredMode(cIdx);

  Int           iScalMat  = 0;
  const UChar*  pucScale  = ( rcMbTempData.getSH().isScalingMatrixPresent(iScalMat) ? rcMbTempData.getSH().getScalingMatrix(iScalMat) : NULL );

  for( UInt n = 0; n < 9; n++)
  {
    bValid    = true;
    iPredMode = n;

    if( avcRewriteFlag && rcMbTempData.isIntraBL() && bValid )
    {
      bValid = (n==rcMbTempData.intraPredMode(cIdx));
    }

    if( bValid )
    {
      RNOK( m_pcIntraPrediction->predictSLumaBlock( rcMbTempData, iPredMode, cIdx, bValid ) );
    }

    if( bValid )
    {
      if ( !bLowComplexity )
      {
        RNOK( m_pcTransform->transform4x4Blk( m_pcIntOrgMbPelData, rcMbTempData, rcMbTempData.get( cIdx ), pucScale, uiAbsSum = 0 ) );

        uiDist = m_pcXDistortion->getLum4x4( rcMbTempData.getLumBlk(), rcMbTempData.getLStride() );

        uiBits = 0;
        if( avcRewriteFlag && rcMbTempData.isIntraBL() )
        {
          RNOK( m_pcTransform->predict4x4Blk( rcMbTempData.get( cIdx ),
          rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( cIdx ),
          rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
          uiAbsSum ) );
        }
        else
        {
          BitCounter::init();
          rcMbTempData.intraPredMode(cIdx) = iPredMode;

          m_pcMbSymbolWriteIf->intraPredModeLuma( rcMbTempData, cIdx );
          uiBits = BitCounter::getNumberOfWrittenBits() + 1;
        }

        if( 0 != uiAbsSum )
        {
          BitCounter::init();
          RNOK( MbCoder::xScanLumaBlock( rcMbTempData, rcMbTempData, cIdx ) );
          uiBits += BitCounter::getNumberOfWrittenBits() - 1;
        }

        fCost = m_pcRateDistortionIf->getFCost( uiBits, uiDist );
      }
      else
      {
        fCost = uiDist = m_pcXDistortion->getLum4x4( rcMbTempData.getLumBlk(), rcMbTempData.getLStride(), DF_SAD );
        fCost += ( iPredMode == mpMode ) ? 0 : lambda_val;
      }


      if( fCost < fBestRd )
      {
        fBestRd = fCost;
        uiBestBits = uiBits;
        uiBestMode = iPredMode;
      }
    }
  }

  RNOK( m_pcIntraPrediction->predictSLumaBlock( rcMbTempData, uiBestMode, cIdx, bValid ) );
  rcMbTempData.intraPredMode( cIdx ) = uiBestMode;

  rcMbTempData.getTempYuvMbBuffer().loadLuma( rcMbTempData, cIdx );

  RNOK( m_pcTransform->transform4x4Blk( m_pcIntOrgMbPelData, rcMbTempData, rcMbTempData.get( cIdx ), pucScale, uiAbsSum = 0 ) );

  if( avcRewriteFlag && rcMbTempData.isIntraBL() )
  {
	  RNOK( m_pcTransform->predict4x4Blk( rcMbTempData.get( cIdx ),
		  rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( cIdx ),
		  rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
		  uiAbsSum ) );
    RNOK( xCheckSkipSliceMbIntra4( rcMbTempData, cIdx, uiAbsSum ) );
  }

  if( 0 != uiAbsSum )
  {
    ruiExtCbp |= 1 << cIdx;
  }

  if ( avcRewriteFlag && rcMbTempData.isIntraBL() && rcMbTempData.getMbDataAccess().getMbDataAccessBase() )
  {
    if( uiBestMode != rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().intraPredMode( cIdx ) )
      ruiBits = MSYS_UINT_MAX;
  }
  if( ruiBits != MSYS_UINT_MAX )
    ruiBits += uiBestBits;

  if ( bLowComplexity )
    ruiBits = (UInt) fBestRd; //warning

  return Err::m_nOK;
}




Void
MbEncoder::xReStoreParameter( MbDataAccess&   rcMbDataAccess,
                              IntMbTempData&  rcMbBestData )
{
  if( rcMbBestData.isIntra() )
  {
    rcMbBestData.getMbMotionData( LIST_0 ).clear( BLOCK_NOT_PREDICTED );
    rcMbBestData.getMbMvdData   ( LIST_0 ).clear();

    if( rcMbDataAccess.getSH().isBSlice() )
    {
      rcMbBestData.getMbMotionData( LIST_1 ).clear( BLOCK_NOT_PREDICTED );
      rcMbBestData.getMbMvdData   ( LIST_1 ).clear();
    }
  }
  else if( rcMbBestData.isSkiped() )
  {
    rcMbBestData.getMbMvdData( LIST_0 ).clear();

    if( rcMbDataAccess.getSH().isBSlice() )
    {
      rcMbBestData.getMbMvdData( LIST_1 ).clear();
    }
  }

  UInt uiFwdBwd = 0;
  if( rcMbDataAccess.getSH().isBSlice() )
  {
    for( Int n = 3; n >= 0; n--)
    {
      uiFwdBwd <<= 4;
      uiFwdBwd += (0 < rcMbBestData.getMbMotionData( LIST_0 ).getRefIdx( Par8x8(n) )) ? 1:0;
      uiFwdBwd += (0 < rcMbBestData.getMbMotionData( LIST_1 ).getRefIdx( Par8x8(n) )) ? 2:0;
    }
  }

  if( rcMbDataAccess.getSH().isPSlice() )
  {
    for( Int n = 3; n >= 0; n--)
    {
      uiFwdBwd <<= 4;
      uiFwdBwd +=  (0 < rcMbBestData.getMbMotionData( LIST_0 ).getRefIdx( Par8x8(n) )) ? 1:0;
    }
  }
  rcMbBestData.setFwdBwd  ( uiFwdBwd );
  rcMbBestData.copyTo     ( rcMbDataAccess );
  m_pcIntPicBuffer->loadBuffer( &rcMbBestData );
}



ErrVal
MbEncoder::xStoreEstimation( MbDataAccess&  rcMbDataAccess,
                             IntMbTempData& rcMbBestData,
                             Frame*         pcResidualLF,
                             Frame*         pcResidualILPred,
                             Frame*         pcPredSignal,
                             RefListStruct& rcRefListStruct,
                             YuvMbBuffer*   pcBaseLayerBuffer )
{
  if( rcMbBestData.isIntra() )
  {
    rcMbBestData.getMbMotionData( LIST_0 ).clear( BLOCK_NOT_PREDICTED );
    rcMbBestData.getMbMvdData   ( LIST_0 ).clear();

    if( rcMbDataAccess.getSH().isBSlice() )
    {
      rcMbBestData.getMbMotionData( LIST_1 ).clear( BLOCK_NOT_PREDICTED );
      rcMbBestData.getMbMvdData   ( LIST_1 ).clear();
    }
  }
  else if( rcMbBestData.isSkiped() )
  {
    rcMbBestData.getMbMvdData( LIST_0 ).clear();

    if( rcMbDataAccess.getSH().isBSlice() )
    {
      rcMbBestData.getMbMvdData( LIST_1 ).clear();
    }
  }

  UInt uiFwdBwd = 0;
  if( rcMbDataAccess.getSH().isBSlice() )
  {
    for( Int n = 3; n >= 0; n--)
    {
      uiFwdBwd <<= 4;
      uiFwdBwd += (0 < rcMbBestData.getMbMotionData( LIST_0 ).getRefIdx( Par8x8(n) )) ? 1:0;
      uiFwdBwd += (0 < rcMbBestData.getMbMotionData( LIST_1 ).getRefIdx( Par8x8(n) )) ? 2:0;
    }
  }

  if( rcMbDataAccess.getSH().isPSlice() )
  {
    for( Int n = 3; n >= 0; n--)
    {
      uiFwdBwd <<= 4;
      uiFwdBwd +=  (0 < rcMbBestData.getMbMotionData( LIST_0 ).getRefIdx( Par8x8(n) )) ? 1:0;
    }
  }
  rcMbBestData.setFwdBwd  ( uiFwdBwd );

  if( rcMbBestData.isIntraBL() && ! rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag() )
  {
    rcMbBestData.getMbTCoeffs().copyPredictionFrom( rcMbBestData.getTempYuvMbBuffer() );
  }
  else if( rcMbBestData.isIntra() )
  {
    rcMbBestData.getMbTCoeffs().copyPredictionFrom( rcMbBestData );
  }
  else if( rcMbBestData.getResidualPredFlag() )
  {
    rcMbBestData.getMbTCoeffs().copyPredictionFrom( *pcBaseLayerBuffer );
  }
  else
  {
    rcMbBestData.getMbTCoeffs().clearPrediction();
  }

  rcMbBestData.copyTo     ( rcMbDataAccess );
  Bool avcRewriteFlag = rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag();

  if( ! rcMbDataAccess.getMbData().isIntra4x4() && ( rcMbDataAccess.getMbData().getMbCbp() & 0x0F ) == 0 )
  {
    if( !avcRewriteFlag || ( avcRewriteFlag && !rcMbDataAccess.getMbData().isIntraBL() ) )
    {
      rcMbDataAccess.getMbData().setTransformSize8x8( false );
    }
  }

  rcMbDataAccess.getMbData().setQp4LF( rcMbDataAccess.getMbData().getQp() );

  RefFrameList* pcRefList0 = &rcRefListStruct.acRefFrameListRC[0];
  RefFrameList* pcRefList1 = &rcRefListStruct.acRefFrameListRC[1];
  RNOK( rcMbDataAccess.getMbMotionData( LIST_0 ).setRefPicIdcs( pcRefList0 ) );
  RNOK( rcMbDataAccess.getMbMotionData( LIST_1 ).setRefPicIdcs( pcRefList1 ) );

  YuvMbBuffer cResidual;
  if( rcMbBestData.isIntra() )
  {
    if( pcPredSignal )
    {
      RNOK( pcPredSignal->getFullPelYuvBuffer()->loadBuffer( &rcMbBestData.getTempYuvMbBuffer() ) );
    }
    if( pcResidualLF || pcResidualILPred )
    {
      cResidual.setAllSamplesToZero();
      if( pcResidualLF )
      {
        RNOK( pcResidualLF    ->getFullPelYuvBuffer()->loadBuffer( &cResidual ) );
      }
      if( pcResidualILPred )
      {
        RNOK( pcResidualILPred->getFullPelYuvBuffer()->loadBuffer( &cResidual ) );
      }
    }
    rcMbBestData.clip();
  }
  else if( rcMbDataAccess.getMbData().getResidualPredFlag() )
  {
    ROF( pcBaseLayerBuffer );
    cResidual.loadLuma  ( rcMbBestData );
    cResidual.loadChroma( rcMbBestData );
    cResidual.subtract  ( rcMbBestData.getTempYuvMbBuffer() ); // coded residual
    YuvMbBuffer   cTmpPred, cTmpResILPred;
    YuvMbBuffer*  pcMbPrediction      = 0;
    YuvMbBuffer*  pcMbResidualILPred  = 0;
    if( rcMbBestData.getBLSkipFlag() && rcMbBestData.getResidualPredFlag() )
    {
      //----- set residual for inter-layer prediction -----
      cTmpResILPred.loadLuma  ( cResidual );
      cTmpResILPred.loadChroma( cResidual );
      cTmpResILPred.addRes    ( *pcBaseLayerBuffer );
      //----- set residual for loopfilter/reconstruction -----
      cResidual    .addRes    ( rcMbBestData.getTempBLSkipResBuffer () );
      //----- set correct prediction signal -----
      cTmpPred     .loadLuma  ( rcMbBestData.getTempYuvMbBuffer     () );
      cTmpPred     .loadChroma( rcMbBestData.getTempYuvMbBuffer     () );
      cTmpPred     .subtract  ( rcMbBestData.getTempBLSkipResBuffer () );
      cTmpPred     .add       ( *pcBaseLayerBuffer );
      //----- set pointers -----
      pcMbResidualILPred  = &cTmpResILPred;
      pcMbPrediction      = &cTmpPred;
    }
    else
    {
      cResidual.addRes( *pcBaseLayerBuffer );
      pcMbResidualILPred  = &cResidual;
      pcMbPrediction      = &rcMbBestData.getTempYuvMbBuffer();
    }
    rcMbBestData.loadLuma  ( *pcMbPrediction );
    rcMbBestData.loadChroma( *pcMbPrediction );
    rcMbBestData.addClip   ( cResidual );
    if( pcPredSignal )
    {
      RNOK( pcPredSignal    ->getFullPelYuvBuffer()->loadBuffer( pcMbPrediction ) );
    }
    if( pcResidualLF )
    {
      RNOK( pcResidualLF    ->getFullPelYuvBuffer()->loadBuffer( &cResidual ) );
    }
    if( pcResidualILPred )
    {
      RNOK( pcResidualILPred->getFullPelYuvBuffer()->loadBuffer( pcMbResidualILPred ) );
    }
  }
  else
  {
    if( pcPredSignal )
    {
      RNOK( pcPredSignal->getFullPelYuvBuffer()->loadBuffer( &rcMbBestData.getTempYuvMbBuffer() ) );
    }
    if( pcResidualLF || pcResidualILPred )
    {
      cResidual.loadLuma   ( rcMbBestData );
      cResidual.loadChroma ( rcMbBestData );
      cResidual.subtract   ( rcMbBestData.getTempYuvMbBuffer() );
      if( pcResidualLF )
      {
        RNOK( pcResidualLF    ->getFullPelYuvBuffer()->loadBuffer( &cResidual ) );
      }
      if( pcResidualILPred )
      {
        RNOK( pcResidualILPred->getFullPelYuvBuffer()->loadBuffer( &cResidual ) );
      }
    }
    rcMbBestData.clip();
  }

  if( m_pcIntPicBuffer )
  {
    m_pcIntPicBuffer->loadBuffer( &rcMbBestData );
  }

  return Err::m_nOK;
}





ErrVal
MbEncoder::xEncodeChromaIntra( IntMbTempData& rcMbTempData,
                               UInt&          ruiExtCbp,
                               UInt&          ruiBits,
                               Bool           bLowComplexity )   // JVT-V079
{
  Double fBestRd = DOUBLE_MAX;        // moved from below
  Bool avcRewriteFlag = rcMbTempData.getSH().getTCoeffLevelPredictionFlag();

  bLowComplexity = false;

  // do it once
  if (avcRewriteFlag ||  ( m_pcIntMbBestIntraChroma == NULL))
  {
    m_pcIntMbBestIntraChroma = &m_acIntMbTempData[4];

	  if( avcRewriteFlag && rcMbTempData.isIntraBL() )
		  rcMbTempData.setChromaPredMode( rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getChromaPredMode() );

    for( UInt uiPM = 0; uiPM < 4; uiPM++ )
    {
      Bool bValid = false;
      RNOK( m_pcIntraPrediction->predictSChromaBlock( rcMbTempData, uiPM, bValid ) );
      rcMbTempData.getTempYuvMbBuffer().loadChroma( rcMbTempData );

      if( avcRewriteFlag && rcMbTempData.isIntraBL() )
        bValid = ( rcMbTempData.getChromaPredMode() == uiPM ) ? bValid : false;

      if( bValid )
      {
        UInt uiBits = 0;
        UInt uiCbp  = 0;
        RNOK( xEncodeChromaTexture( rcMbTempData, uiCbp, uiBits, bLowComplexity ) );
        UInt uiDist = rcMbTempData.distU() + rcMbTempData.distV();
        rcMbTempData.setChromaPredMode( uiPM );

        if( !avcRewriteFlag || !rcMbTempData.isIntraBL() )
        {
          BitCounter::init();
          RNOK( UvlcWriter::intraPredModeChroma( rcMbTempData ) );
          uiBits += BitCounter::getNumberOfWrittenBits();
        }

        Double fNewRd = m_pcRateDistortionIf->getCost( uiBits, uiDist );

        if( fNewRd < fBestRd )
        {
          m_pcIntMbBestIntraChroma->loadChromaData( rcMbTempData );
          m_pcIntMbBestIntraChroma->cbp()    = uiCbp;
          m_pcIntMbBestIntraChroma->bits()   = uiBits;
          fBestRd = fNewRd;
        }
      }
    }
  }

  if (avcRewriteFlag && (fBestRd == DOUBLE_MAX))        // no valid mode is found
  {
    ruiBits = MSYS_UINT_MAX;
    return Err::m_nOK;
  }

  rcMbTempData.loadChromaData( *m_pcIntMbBestIntraChroma );
  ruiExtCbp += m_pcIntMbBestIntraChroma->cbp();
  ruiBits   += m_pcIntMbBestIntraChroma->bits();

  return Err::m_nOK;
}





ErrVal
MbEncoder::xEncodeChromaTexture( IntMbTempData& rcMbTempData,
                                 UInt&          ruiExtCbp,
                                 UInt&          ruiBits,
                                 Bool           bLowComplexity )   // JVT-V079
{
  TCoeff aiCoeff[128];

  bLowComplexity = false;

  XPel* pucCb   = rcMbTempData.getMbCbAddr();
  XPel* pucCr   = rcMbTempData.getMbCrAddr();
  Int   iStride = rcMbTempData.getCStride();

  Int           iScalMatCb  = ( rcMbTempData.isIntra() ? 1 : 4 );
  Int           iScalMatCr  = ( rcMbTempData.isIntra() ? 2 : 5 );
  const UChar*  pucScaleCb  = ( rcMbTempData.getSH().isScalingMatrixPresent(iScalMatCb) ? rcMbTempData.getSH().getScalingMatrix(iScalMatCb) : NULL );
  const UChar*  pucScaleCr  = ( rcMbTempData.getSH().isScalingMatrixPresent(iScalMatCr) ? rcMbTempData.getSH().getScalingMatrix(iScalMatCr) : NULL );

  UInt uiDcCb = 0;
  UInt uiAcCb = 0;
  UInt uiDcCr = 0;
  UInt uiAcCr = 0;

  if ( rcMbTempData.getMbDataAccess().isSCoeffPred() )
  {
	  TCoeff* piCoeffBase = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( CIdx(0)) ; // U
    RNOK( m_pcTransform->transformChromaBlocksCGS( m_pcIntOrgMbPelData->getMbCbAddr(), pucCb, CIdx(0), iStride, rcMbTempData.get( CIdx(0) ), aiCoeff+0x00, piCoeffBase, pucScaleCb, uiDcCb, uiAcCb ) );

    piCoeffBase = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( CIdx(4) ); // V
    RNOK( m_pcTransform->transformChromaBlocksCGS( m_pcIntOrgMbPelData->getMbCrAddr(), pucCr, CIdx(4), iStride, rcMbTempData.get( CIdx(4) ), aiCoeff+0x40, piCoeffBase, pucScaleCr, uiDcCr, uiAcCr ) );
  }
  else
  {
    RNOK( m_pcTransform->transformChromaBlocks( m_pcIntOrgMbPelData->getMbCbAddr(), pucCb, CIdx(0), iStride, rcMbTempData.get( CIdx(0) ), aiCoeff+0x00, pucScaleCb, uiDcCb, uiAcCb ) );
    RNOK( m_pcTransform->transformChromaBlocks( m_pcIntOrgMbPelData->getMbCrAddr(), pucCr, CIdx(4), iStride, rcMbTempData.get( CIdx(4) ), aiCoeff+0x40, pucScaleCr, uiDcCr, uiAcCr ) );
  }

  UInt uiChromaCbp = 0;
  UInt uiDcBits = 0;
  UInt uiDcAbs = uiDcCb + uiDcCr;

  if( rcMbTempData.getMbDataAccess().isTCoeffPred() )
  {
    UInt uiCbBaseQp = rcMbTempData.getMbDataAccess().getSH().getBaseSliceHeader()->getCbQp( rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp() );
    UInt uiCrBaseQp = rcMbTempData.getMbDataAccess().getSH().getBaseSliceHeader()->getCrQp( rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp() );
    RNOK( m_pcTransform->predictChromaBlocks( 0, rcMbTempData.get( CIdx(0) ),
                                              rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( CIdx(0) ),
                                              uiCbBaseQp, uiDcCb, uiAcCb ) );
    RNOK( m_pcTransform->predictChromaBlocks( 1, rcMbTempData.get( CIdx(4) ),
                                              rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( CIdx(4) ),
                                              uiCrBaseQp, uiDcCr, uiAcCr ) );
	  uiDcAbs = uiDcCb + uiDcCr;
  }

  if( uiDcAbs )
  {
    BitCounter::init();
    MbCoder::xScanChromaDc( rcMbTempData, rcMbTempData );
    uiDcBits = BitCounter::getNumberOfWrittenBits();
    uiChromaCbp = 1;
  }

  UInt uiCbp1 = uiChromaCbp;
  UInt uiCbp2 = uiChromaCbp;
  UInt uiAcBits1 = 4;
  UInt uiAcBits2 = 4;

  if( uiAcCr )
  {
    uiCbp2 = 2;
    xSetCoeffCost( 0 );
    BitCounter::init();
    MbCoder::xScanChromaAcV( rcMbTempData, rcMbTempData );
    uiAcBits2 = BitCounter::getNumberOfWrittenBits();

    if( 4 > xGetCoeffCost() )
    {
      if( uiAcBits2 > 4 )
      {
        for( CIdx cCIdx(4); cCIdx.isLegal(8); cCIdx++)
        {
          rcMbTempData.clearAcBlk( cCIdx );
        }
        uiAcBits2 = 4;

        if( m_pcIntMbTempData->getMbDataAccess().isSCoeffPred() )
        {
          // add the base layer coeff back
          TCoeff* piCoeff = &aiCoeff[0x40];
          TCoeff* ptCoeff = rcMbTempData.getMbDataAccess().getMbTCoeffs().get( CIdx(4) );
          TCoeff* piCoeffBase = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( CIdx(4) );
          for( UInt x=0x00; x<0x40; x+=0x10 )
            for( UInt n=1; n<16; n++ )
            {
              piCoeff[x+n] = piCoeffBase[x+n].getLevel();
              ptCoeff[x+n].setLevel(piCoeffBase[x+n].getLevel()); // store the coefficients in TCoeff.level field
            }
        }
        else if( m_pcIntMbTempData->getMbDataAccess().isTCoeffPred() )
        {
          // Predict AC Scaled Coefficients
          UInt uiBaseQp = rcMbTempData.getMbDataAccess().getSH().getBaseSliceHeader()->getCrQp( rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp() );
          m_pcTransform->predictScaledACCoeffs( 1, &aiCoeff[0x40],
                                                rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( CIdx(4) ),
                                                uiBaseQp, pucScaleCr );

          TCoeff* ptCoeff = rcMbTempData.getMbDataAccess().getMbTCoeffs().get( CIdx(4) );
          for( UInt x=0x00; x<0x40; x+=0x10 )
          {
            for( UInt n=1; n<16; n++ )
            {
              ptCoeff[x+n].setLevel(aiCoeff[0x40+x+n].getCoeff()); // store the coefficients in TCoeff.level field
            }
          }
        }
        else
        {
          ::memset( &aiCoeff[0x41], 0x00, 15*sizeof(TCoeff) );
          ::memset( &aiCoeff[0x51], 0x00, 15*sizeof(TCoeff) );
          ::memset( &aiCoeff[0x61], 0x00, 15*sizeof(TCoeff) );
          ::memset( &aiCoeff[0x71], 0x00, 15*sizeof(TCoeff) );
        }
      }

      uiCbp2 = uiChromaCbp;
    }
  }


  if( uiAcCb )
  {
    uiCbp1 = 2;
    xSetCoeffCost( 0 );
    BitCounter::init();
    MbCoder::xScanChromaAcU( rcMbTempData, rcMbTempData );
    uiAcBits1 = BitCounter::getNumberOfWrittenBits();

    if( 4 > xGetCoeffCost() )
    {
      if( uiAcBits1 > 4 )
      {
        for( CIdx cCIdx(0); cCIdx.isLegal(4); cCIdx++)
        {
          rcMbTempData.clearAcBlk( cCIdx );
        }
        uiAcBits1 = 4;

        if( m_pcIntMbTempData->getMbDataAccess().isSCoeffPred() )
        {
          // add the base layer coeff back and also store the dequantized coeffs
          TCoeff* piCoeff = &aiCoeff[0x00];
          TCoeff* ptCoeff = rcMbTempData.getMbDataAccess().getMbTCoeffs().get( CIdx(0) );
          TCoeff* piCoeffBase = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( CIdx(0) );
          for( UInt x=0x00; x<0x40; x+=0x10 )
            for( UInt n=1; n<16; n++ )
            {
              piCoeff[x+n] = piCoeffBase[x+n].getLevel();
              ptCoeff[x+n].setLevel(piCoeffBase[x+n].getLevel()); // store the coefficients in TCoeff.level field
            }
        }
        else if( m_pcIntMbTempData->getMbDataAccess().isTCoeffPred() )
        {
          // Predict AC Scaled Coefficients
          UInt uiBaseQp = rcMbTempData.getMbDataAccess().getSH().getBaseSliceHeader()->getCbQp( rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp() );
          m_pcTransform->predictScaledACCoeffs( 0, &aiCoeff[0x00],
                                                rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( CIdx(0) ),
                                                uiBaseQp, pucScaleCb );

          TCoeff* ptCoeff = rcMbTempData.getMbDataAccess().getMbTCoeffs().get( CIdx(0) );
          for( UInt x=0x00; x<0x40; x+=0x10 )
            for( UInt n=1; n<16; n++ )
            {
              ptCoeff[x+n].setLevel(aiCoeff[x+n].getCoeff()); // store the coefficients in TCoeff.level field
            }
        }
        else
        {
          ::memset( &aiCoeff[0x01], 0x00, 15*sizeof(TCoeff) );
          ::memset( &aiCoeff[0x11], 0x00, 15*sizeof(TCoeff) );
          ::memset( &aiCoeff[0x21], 0x00, 15*sizeof(TCoeff) );
          ::memset( &aiCoeff[0x31], 0x00, 15*sizeof(TCoeff) );
        }
      }
      uiCbp1 = uiChromaCbp;
    }
  }

  if( (uiAcBits1 + uiAcBits2) > 8)
  {
    ruiBits += uiAcBits1 + uiAcBits2;
    uiChromaCbp = gMax( uiCbp1, uiCbp2 );
  }

  if( rcMbTempData.getMbDataAccess().isSCoeffPred() || rcMbTempData.getSH().getTCoeffLevelPredictionFlag() || uiCbp1)
  {
    m_pcTransform->invTransformChromaDc( &aiCoeff[0x00] );
    RNOK( m_pcTransform->invTransformChromaBlocks( pucCb, iStride, &aiCoeff[0x00] ) );
  }

  if( rcMbTempData.getMbDataAccess().isSCoeffPred() || rcMbTempData.getSH().getTCoeffLevelPredictionFlag() || uiCbp2)
  {
    m_pcTransform->invTransformChromaDc( &aiCoeff[0x40] );
    RNOK( m_pcTransform->invTransformChromaBlocks( pucCr, iStride, &aiCoeff[0x40] ) );
  }

  if ( bLowComplexity )
  {
    m_pcIntMbTempData->distU() = m_pcXDistortion->get8x8Cr( pucCr, iStride, DF_SAD );
    m_pcIntMbTempData->distV() = m_pcXDistortion->get8x8Cb( pucCb, iStride, DF_SAD );
  }
  else
  {
    m_pcIntMbTempData->distU() = m_pcXDistortion->get8x8Cr( pucCr, iStride );
    m_pcIntMbTempData->distV() = m_pcXDistortion->get8x8Cb( pucCb, iStride );
  }

  if( uiChromaCbp )
  {
    ruiBits += uiDcBits;
  }

  if( m_pcIntMbTempData->getMbDataAccess().isTCoeffPred() && m_pcIntMbTempData->isIntraBL() )
  {
    RNOK( xCheckSkipSliceMbIntraChroma( *m_pcIntMbTempData, uiChromaCbp ) );
  }

  ruiExtCbp |= uiChromaCbp << 16;

  return Err::m_nOK;
}


Void
MbEncoder::reCalcChroma( IntMbTempData& rcMbTempData )
{
  TCoeff* pcCoeff     = rcMbTempData.get( CIdx(0) );
  TCoeff* pcBaseCoeff = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( CIdx(0) );

  //=== add the base layer dequant. coeff ====
  UInt ui = 0;
  for( ui = 0; ui < 128; ui++ )
  {
    pcCoeff[ui] = pcBaseCoeff[ui].getLevel();
    pcCoeff[ui].setLevel( pcBaseCoeff[ui].getLevel() );
  }

  //=== inverse trafo ===
  XPel*         pucCb       = rcMbTempData.getMbCbAddr();
  XPel*         pucCr       = rcMbTempData.getMbCrAddr();
  Int           iStride     = rcMbTempData.getCStride();
  m_pcTransform->invTransformChromaDc( &pcCoeff[0x00] );
  m_pcTransform->invTransformChromaDc( &pcCoeff[0x40] );
  m_pcTransform->invTransformChromaBlocks( pucCb, iStride, &pcCoeff[0x00] );
  m_pcTransform->invTransformChromaBlocks( pucCr, iStride, &pcCoeff[0x40] );

  //=== only clear coefficients ===
  for( ui = 0; ui < 128; ui++ )
  {
    pcCoeff[ui].setCoeff( 0 );
  }
}


ErrVal
MbEncoder::reCalcChromaRewrite( IntMbTempData& rcMbTempData )
{
  TCoeff aiCoeff[128];

  XPel* pucCb   = rcMbTempData.getMbCbAddr();
  XPel* pucCr   = rcMbTempData.getMbCrAddr();
  Int   iStride = rcMbTempData.getCStride();

  Int           iScalMatCb  = ( rcMbTempData.isIntra() ? 1 : 4 );
  Int           iScalMatCr  = ( rcMbTempData.isIntra() ? 2 : 5 );
  const UChar*  pucScaleCb  = ( rcMbTempData.getSH().isScalingMatrixPresent(iScalMatCb) ? rcMbTempData.getSH().getScalingMatrix(iScalMatCb) : NULL );
  const UChar*  pucScaleCr  = ( rcMbTempData.getSH().isScalingMatrixPresent(iScalMatCr) ? rcMbTempData.getSH().getScalingMatrix(iScalMatCr) : NULL );

  UInt    uiBaseCbQp  = rcMbTempData.getMbDataAccess().getSH().getBaseSliceHeader()->getCbQp( rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp() );
  UInt    uiBaseCrQp  = rcMbTempData.getMbDataAccess().getSH().getBaseSliceHeader()->getCrQp( rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp() );
  TCoeff* piBaseCoeff = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( CIdx(0) );
  TCoeff* piCoeff     = rcMbTempData.get( CIdx(0) );

  m_pcTransform->predictScaledChromaCoeffs( aiCoeff, piBaseCoeff, uiBaseCbQp, uiBaseCrQp, pucScaleCb, pucScaleCr );
  for( UInt x = 0x00; x < 0x80; x++ )
  {
    piCoeff[x].setLevel( aiCoeff[x].getCoeff() ); // store the coefficients in TCoeff.level field
  }
  {
    m_pcTransform->invTransformChromaDc( &aiCoeff[0x00] );
    RNOK( m_pcTransform->invTransformChromaBlocks( pucCb, iStride, &aiCoeff[0x00] ) );
  }
  {
    m_pcTransform->invTransformChromaDc( &aiCoeff[0x40] );
    RNOK( m_pcTransform->invTransformChromaBlocks( pucCr, iStride, &aiCoeff[0x40] ) );
  }
  {
    m_pcIntMbTempData->distU() = m_pcXDistortion->get8x8Cr( pucCr, iStride );
    m_pcIntMbTempData->distV() = m_pcXDistortion->get8x8Cb( pucCb, iStride );
  }

  return Err::m_nOK;
}



ErrVal
MbEncoder::xSetRdCostIntraMb( IntMbTempData&  rcMbTempData,
                              UInt            uiCoeffBits,
                              Bool            bBLSkip )
{
  if( rcMbTempData.getSH().getTCoeffLevelPredictionFlag() && 
      rcMbTempData.getMbCbp() == 0 &&
      rcMbTempData.getQp   () != rcMbTempData.getMbDataAccess().getLastQp() )
  {
    rcMbTempData.rdCost() = DOUBLE_MAX;
    return Err::m_nOK;
  }

  YuvMbBuffer&  rcYuvMbBuffer = rcMbTempData;
  UInt          uiMbDist      = 0;
  UInt          uiMbBits      = 0;

  //===== get distortion =====
  uiMbDist  += rcMbTempData.distY() = m_pcXDistortion->getLum16x16 ( rcYuvMbBuffer.getMbLumAddr(), rcYuvMbBuffer.getLStride() );
  uiMbDist  += rcMbTempData.distU() = m_pcXDistortion->get8x8Cb    ( rcYuvMbBuffer.getMbCbAddr (), rcYuvMbBuffer.getCStride() );
  uiMbDist  += rcMbTempData.distV() = m_pcXDistortion->get8x8Cr    ( rcYuvMbBuffer.getMbCrAddr (), rcYuvMbBuffer.getCStride() );

  //===== get rate =====
  RNOK(   BitCounter::init() );
  if( ! bBLSkip )
  {
    RNOK( MbCoder::m_pcMbSymbolWriteIf->mbMode    ( rcMbTempData ) );
  }
  RNOK(   MbCoder::m_pcMbSymbolWriteIf->cbp       ( rcMbTempData ) );
  if( rcMbTempData.cbp() )
  {
    RNOK( MbCoder::m_pcMbSymbolWriteIf->deltaQp   ( rcMbTempData ) );
  }
  uiMbBits  += BitCounter::getNumberOfWrittenBits();
  uiMbBits  += uiCoeffBits + 1; // 1 for chroma pred mode

  rcMbTempData.bits() = uiMbBits;

  //===== set rd-cost =====
  rcMbTempData.rdCost() = m_pcRateDistortionIf->getCost( uiMbBits, uiMbDist );

  return Err::m_nOK;
}



ErrVal
MbEncoder::xSetRdCostInterMb( IntMbTempData&      rcMbTempData,
                              MbDataAccess*       pcMbDataAccessBase,
                              RefListStruct&      rcRefListStruct,
                              UInt                uiMinQP,
                              UInt                uiMaxQP,
                              Bool                bLowComplexity,   // JVT-V079
                              Bool                bBLSkip,
                              UInt                uiAdditionalBits,
                              Frame*              pcBaseLayerRec,
                              const YuvMbBuffer*  pcBaseLayerResidual )
{
  YuvMbBuffer   cTempPredBuffer;
  YuvMbBuffer&  rcYuvMbBuffer       = rcMbTempData;
  YuvMbBuffer&  rcTempYuvMbBuffer   = rcMbTempData  .getTempYuvMbBuffer     ();
  YuvMbBuffer&  rcTempBLSkipBaseRes = rcMbTempData  .getTempBLSkipResBuffer ();
  MbDataAccess& rcMbDataAccess      = rcMbTempData  .getMbDataAccess        ();
  MbData&       rcMbData            = rcMbDataAccess.getMbData              ();
  MbMode        eMbMode             = rcMbData      .getMbMode              ();
  Bool          b8x8Mode            = ( eMbMode == MODE_8x8 || eMbMode == MODE_8x8ref0 );
  UInt          uiFwdBwd            = 0;
  Bool          bBLSkipResPred      = ( bBLSkip && rcMbTempData.getResidualPredFlag() );

  if( bLowComplexity && ( rcMbTempData.rdCost() != DOUBLE_MAX ) )
  {
    return Err::m_nOK;
  }

  // enforce to use the same transform size as base
  if( pcMbDataAccessBase && pcMbDataAccessBase->getMbData().isTransformSize8x8()
      && ( rcMbDataAccess.isSCoeffPred() || rcMbDataAccess.isTCoeffPred() ) )
  {
    // 4x4-trafo not allowed here
    // Encoders can consider 8x8 transform though...
    rcMbTempData.rdCost() = DOUBLE_MAX;
    return Err::m_nOK;
  }

  //===== set forward / backward indication =====
  if( rcMbDataAccess.getSH().isBSlice() )
  {
    for( Int n = 3; n >= 0; n--)
    {
      uiFwdBwd <<= 4;
      uiFwdBwd += (0 < rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( Par8x8(n) )) ? 1:0;
      uiFwdBwd += (0 < rcMbDataAccess.getMbMotionData( LIST_1 ).getRefIdx( Par8x8(n) )) ? 2:0;
    }
  }
  if( rcMbDataAccess.getSH().isPSlice() )
  {
    for( Int n = 3; n >= 0; n--)
    {
      uiFwdBwd <<= 4;
      uiFwdBwd +=  (0 < rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( Par8x8(n) )) ? 1:0;
    }
  }
  rcMbTempData.setFwdBwd( uiFwdBwd );

  //===== get prediction signal (for residual coding) =====
  {
    RefFrameList& rcMCList0 = rcRefListStruct.acRefFrameListMC[ 0 ];
    RefFrameList& rcMCList1 = rcRefListStruct.acRefFrameListMC[ 1 ];
    if( b8x8Mode )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        RNOK( m_pcMotionEstimation->compensateSubMb( c8x8Idx, rcMbDataAccess, rcMCList0, rcMCList1,
                                                     &rcYuvMbBuffer, false, false ) );
      }
    }
    else
    {
      RNOK( m_pcMotionEstimation->compensateMb( rcMbDataAccess, rcMCList0, rcMCList1,
                                                &rcYuvMbBuffer, false ) );
    }
    if( bBLSkip )
    {
      ROF ( pcBaseLayerRec );
      RNOK( m_pcMotionEstimation->compensateMbBLSkipIntra( rcMbDataAccess, &rcYuvMbBuffer, pcBaseLayerRec ) );
    }
    cTempPredBuffer.loadLuma  ( rcYuvMbBuffer );
    cTempPredBuffer.loadChroma( rcYuvMbBuffer );
  }

  //===== get prediction signal (for reconstruction) =====
  if( rcRefListStruct.bMCandRClistsDiffer )
  {
    RefFrameList& rcMCList0 = rcRefListStruct.acRefFrameListRC[ 0 ];
    RefFrameList& rcMCList1 = rcRefListStruct.acRefFrameListRC[ 1 ];
    if( b8x8Mode )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        RNOK( m_pcMotionEstimation->compensateSubMb( c8x8Idx, rcMbDataAccess, rcMCList0, rcMCList1,
                                                     &rcTempYuvMbBuffer, false, false ) );
      }
    }
    else
    {
      RNOK( m_pcMotionEstimation->compensateMb( rcMbDataAccess, rcMCList0, rcMCList1,
                                                &rcTempYuvMbBuffer, false ) );
    }
    if( bBLSkip )
    {
      ROF ( pcBaseLayerRec );
      RNOK( m_pcMotionEstimation->compensateMbBLSkipIntra( rcMbDataAccess, &rcTempYuvMbBuffer, pcBaseLayerRec ) );
    }
  }
  else
  {
    rcTempYuvMbBuffer.loadLuma  ( rcYuvMbBuffer );
    rcTempYuvMbBuffer.loadChroma( rcYuvMbBuffer );
  }

  if( bLowComplexity )
  {
    rcMbTempData.rdCost() = m_pcXDistortion->getLum16x16( rcYuvMbBuffer.getMbLumAddr(), rcYuvMbBuffer.getLStride(), DF_SAD );
    return Err::m_nOK;
  }

  if( bBLSkipResPred )
  {
    ROF( pcBaseLayerResidual );
    //===== get actual base layer residual signal and modify prediction signal accordingly =====
    rcTempBLSkipBaseRes.loadLuma  ( *pcBaseLayerResidual );
    rcTempBLSkipBaseRes.loadChroma( *pcBaseLayerResidual );
    RNOK( m_pcMotionEstimation->updateMbBLSkipResidual( rcMbDataAccess, rcTempBLSkipBaseRes ) ); // this line makes the difference
    rcYuvMbBuffer      .subtract  ( *pcBaseLayerResidual );
    rcYuvMbBuffer      .add       (  rcTempBLSkipBaseRes );
    rcTempYuvMbBuffer  .loadLuma  (  rcYuvMbBuffer );
    rcTempYuvMbBuffer  .loadChroma(  rcYuvMbBuffer );
    cTempPredBuffer    .loadLuma  (  rcYuvMbBuffer );
    cTempPredBuffer    .loadChroma(  rcYuvMbBuffer );
  }

  //===== LOOP OVER QP VALUES ====
  Bool              bSkipMode = ( eMbMode == MODE_SKIP && rcMbDataAccess.getSH().isPSlice() );
  Double            dMinCost  = DOUBLE_MAX;
  UInt              uiBestQP  = MSYS_UINT_MAX;
  UInt              uiBestCBP = MSYS_UINT_MAX;
  MbTransformCoeffs cBestCoeffs;
  YuvMbBuffer       cBestRec;
  if( bSkipMode )
  {
    uiMinQP = uiMaxQP = rcMbDataAccess.getMbData().getQp();
  }
  for( UInt uiQP = uiMinQP; uiQP <= uiMaxQP; uiQP++ )
  {
    rcMbTempData  .setQp( uiQP );
    m_pcTransform->setQp( rcMbTempData, false );
    
    //===== encode residual and get rate for coefficients =====
    UInt  uiMbDist    = 0;
    UInt  uiMbBits    = 0;
    UInt  uiCoeffCost = 0;
    UInt  uiExtCbp    = 0;
    if( ! bSkipMode )
    {
      //--- LUMA ---
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        xSetCoeffCost( 0 );
        UInt  uiBits = 0;
        UInt  uiCbp  = 0;
        for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
        {
          RNOK( xEncode4x4InterBlock( rcMbTempData, cIdx, uiBits, uiCbp ) );
        }
        if( uiCbp )
        {
          if( xGetCoeffCost() <= 4 )
          {
            rcYuvMbBuffer.loadLuma( cTempPredBuffer, c8x8Idx );
            rcMbTempData.clearLumaLevels8x8( c8x8Idx );
            if( rcMbTempData.getMbDataAccess().isSCoeffPred() )
            {
              reCalcBlock8x8(rcMbTempData, c8x8Idx, 1);
            }
            else if( rcMbTempData.getMbDataAccess().isTCoeffPred() )
            {
              reCalcBlock8x8Rewrite(rcMbTempData, c8x8Idx, 1);
            }
          }
          else
          {
            uiCoeffCost += xGetCoeffCost();
            uiExtCbp    += uiCbp;
            uiMbBits    += uiBits;
          }
        }
      }
      if( uiExtCbp && uiCoeffCost <= 5 )
      {
        rcYuvMbBuffer.loadLuma( cTempPredBuffer );
        uiExtCbp  = 0;
        uiMbBits  = 0;
        rcMbTempData.clearLumaLevels();
        if( rcMbTempData.getMbDataAccess().isSCoeffPred() )
        {
          for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
          {
            reCalcBlock8x8(rcMbTempData, c8x8Idx, 1);
          }
        }
        else if( rcMbTempData.getMbDataAccess().isTCoeffPred() )
        {
          for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
          {
            reCalcBlock8x8Rewrite(rcMbTempData, c8x8Idx, 1);
          }
        }
      }
      //--- CHROMA ---
      RNOK( xEncodeChromaTexture( rcMbTempData, uiExtCbp, uiMbBits ) );
    }
    rcMbTempData.cbp() = xCalcMbCbp( uiExtCbp );
    RNOK( xCheckSkipSliceMb( rcMbTempData ) );
    RNOK( xAdjustRewriteReconstruction( rcMbTempData ) );

    //===== get distortion =====
    if( !pcMbDataAccessBase || pcMbDataAccessBase->getMbData().getSafeResPred() || !rcMbTempData.getResidualPredFlag() )
    {
      uiMbDist += m_pcXDistortion->getLum16x16  ( rcYuvMbBuffer.getMbLumAddr(), rcYuvMbBuffer.getLStride() );
    }
    else
    {
      uiMbDist += m_pcXDistortion->getLum16x16RP( rcYuvMbBuffer.getMbLumAddr(), rcYuvMbBuffer.getLStride() );
    }
    uiMbDist   += m_pcXDistortion->get8x8Cb     ( rcYuvMbBuffer.getMbCbAddr (), rcYuvMbBuffer.getCStride() );
    uiMbDist   += m_pcXDistortion->get8x8Cr     ( rcYuvMbBuffer.getMbCrAddr (), rcYuvMbBuffer.getCStride() );

    if( rcMbDataAccess.getSH().getSliceType()  == B_SLICE   &&
        rcMbTempData.getMbMode() == MODE_SKIP && uiExtCbp == 0 &&
        rcMbTempData.getResidualPredFlag() == rcMbDataAccess.getSH().getDefaultResidualPredictionFlag() )
    {
      bSkipMode = true;
      if( rcMbDataAccess.getSH().getNoInterLayerPredFlag() )
      {
        uiMbBits += 1;
      }
    }

    //===== get rate =====
    if( ! bSkipMode )
    {
      RNOK( BitCounter::init() );
      if( ! bBLSkip )
      {
        RNOK(   MbCoder::m_pcMbSymbolWriteIf->mbMode    ( rcMbTempData ) );
        if( b8x8Mode )
        {
          RNOK( MbCoder::m_pcMbSymbolWriteIf->blockModes( rcMbTempData ) );
        }
      }
      RNOK(     MbCoder::m_pcMbSymbolWriteIf->cbp       ( rcMbTempData ) );
      if( rcMbTempData.cbp() )
      {
        RNOK(   MbCoder::m_pcMbSymbolWriteIf->deltaQp   ( rcMbTempData ) );
      }
      if( rcRefListStruct.acRefFrameListRC[ 0 ].getActive() && !bBLSkip )
      {
        RNOK(   MbCoder::xWriteMotionPredFlags          ( rcMbTempData, eMbMode, LIST_0 ) );
        RNOK(   MbCoder::xWriteReferenceFrames          ( rcMbTempData, eMbMode, LIST_0 ) );
        RNOK(   MbCoder::xWriteMotionVectors            ( rcMbTempData, eMbMode, LIST_0 ) );
      }
      if( rcRefListStruct.acRefFrameListRC[ 1 ].getActive() && !bBLSkip )
      {
        RNOK(   MbCoder::xWriteMotionPredFlags          ( rcMbTempData, eMbMode, LIST_1 ) );
        RNOK(   MbCoder::xWriteReferenceFrames          ( rcMbTempData, eMbMode, LIST_1 ) );
        RNOK(   MbCoder::xWriteMotionVectors            ( rcMbTempData, eMbMode, LIST_1 ) );
      }
      uiMbBits  += BitCounter::getNumberOfWrittenBits();
    }

    //===== set rd-cost =====
    if( rcMbTempData.getSH().getTCoeffLevelPredictionFlag() && 
        rcMbTempData.getMbCbp() == 0 &&
        rcMbTempData.getQp   () != rcMbDataAccess.getLastQp() )
    {
      rcMbTempData.rdCost() = DOUBLE_MAX;
    }
    else
    {
      rcMbTempData.rdCost() = m_pcRateDistortionIf->getCost( uiMbBits + uiAdditionalBits, uiMbDist );
    }

#if PROPOSED_DEBLOCKING_APRIL2010
    RNOK( xCheckInterProfileCompatibility( rcMbTempData, &rcTempYuvMbBuffer, ( bBLSkipResPred ? &rcTempBLSkipBaseRes : m_pcRefLayerResidual ), false ) );
#endif

    //===== check r-d cost =====
    if( rcMbTempData.rdCost() < dMinCost )
    {
      dMinCost  = rcMbTempData.rdCost();
      uiBestQP  = rcMbTempData.getQp      ();
      uiBestCBP = rcMbTempData.getMbExtCbp();
      cBestCoeffs .copyFrom   ( rcMbTempData.getMbTCoeffs() );
      cBestRec    .loadLuma   ( rcMbTempData );
      cBestRec    .loadChroma ( rcMbTempData );
    }

    //===== reset prediction signal =====
    rcMbTempData.loadLuma   ( cTempPredBuffer );
    rcMbTempData.loadChroma ( cTempPredBuffer );
  }

  //===== set best data ====
  rcMbTempData.rdCost() = dMinCost;
  rcMbTempData.setQp        ( uiBestQP );
  rcMbTempData.setMbExtCbp  ( uiBestCBP );
  rcMbTempData.getMbTCoeffs ().copyFrom( cBestCoeffs );
  rcMbTempData.loadLuma     ( cBestRec );
  rcMbTempData.loadChroma   ( cBestRec );

  //===== reset transform status =====
  m_pcTransform->setQp( rcMbDataAccess, false );

  //---- correct prediction signal ----
  if( rcRefListStruct.bMCandRClistsDiffer )
  {
    rcYuvMbBuffer.subtract( cTempPredBuffer );
    rcYuvMbBuffer.add     ( rcTempYuvMbBuffer );
  }

  return Err::m_nOK;
}




ErrVal
MbEncoder::xScale4x4Block( TCoeff*            piCoeff,
                           const UChar*       pucScale,
                           UInt               uiStart,
                           const QpParameter& rcQP )
{
  if( pucScale )
  {
    Int iAdd = ( rcQP.per() <= 3 ? ( 1 << ( 3 - rcQP.per() ) ) : 0 );

    for( UInt ui = uiStart; ui < 16; ui++ )
    {
      piCoeff[ui] = ( ( piCoeff[ui] * g_aaiDequantCoef[rcQP.rem()][ui] * pucScale[ui] + iAdd ) << rcQP.per() ) >> 4;
    }
  }
  else
  {
    for( UInt ui = uiStart; ui < 16; ui++ )
    {
      piCoeff[ui] *= ( g_aaiDequantCoef[rcQP.rem()][ui] << rcQP.per() );
    }
  }

  return Err::m_nOK;
}


ErrVal
MbEncoder::xScale8x8Block( TCoeff*            piCoeff,
                           const UChar*       pucScale,
                           const QpParameter& rcQP )
{
  Int iAdd = ( rcQP.per() <= 5 ? ( 1 << ( 5 - rcQP.per() ) ) : 0 );

  if( pucScale )
  {
    for( UInt ui = 0; ui < 64; ui++ )
    {
      piCoeff[ui] = ( ( piCoeff[ui] * g_aaiDequantCoef64[rcQP.rem()][ui] * pucScale[ui] + iAdd ) << rcQP.per() ) >> 6;
    }
  }
  else
  {
    for( UInt ui = 0; ui < 64; ui++ )
    {
      piCoeff[ui] = ( ( piCoeff[ui] * g_aaiDequantCoef64[rcQP.rem()][ui] * 16 + iAdd ) << rcQP.per() ) >> 6;
    }
  }

  return Err::m_nOK;
}


ErrVal
MbEncoder::xScaleTCoeffs( MbDataAccess& rcMbDataAccess, MbTransformCoeffs& rcTCoeffs )
{
  const Int aaiDequantDcCoef[6] = {  10, 11, 13, 14, 16, 18 };
  Quantizer cQuantizer;
  cQuantizer.setQp( rcMbDataAccess, false );

  const QpParameter&  cLQp      = cQuantizer.getLumaQp();
  const QpParameter&  cUQp      = cQuantizer.getCbQp  ();
  const QpParameter&  cVQp      = cQuantizer.getCrQp  ();
  Bool                bIntra    = rcMbDataAccess.getMbData().isIntra();
  Bool                b8x8      = rcMbDataAccess.getMbData().isTransformSize8x8();
  Bool                b16x16    = rcMbDataAccess.getMbData().isIntra16x16();
  UInt                uiYScalId = ( bIntra ? ( b8x8 && !b16x16 ? 6 : 0 ) : ( b8x8 ? 7 : 3 ) );
  UInt                uiUScalId = ( bIntra ? 1 : 4 );
  UInt                uiVScalId = ( bIntra ? 2 : 5 );
  const UChar*        pucScaleY = rcMbDataAccess.getSH().getScalingMatrix( uiYScalId );
  const UChar*        pucScaleU = rcMbDataAccess.getSH().getScalingMatrix( uiUScalId );
  const UChar*        pucScaleV = rcMbDataAccess.getSH().getScalingMatrix( uiVScalId );

  //===== copy all coefficients =====
  rcTCoeffs.copyFrom( rcMbDataAccess.getMbTCoeffs() );

  //===== luma =====
  if( b16x16 )
  {
    //===== INTRA_16x16 =====
    Int iScaleY  = aaiDequantDcCoef[cLQp.rem()] << cLQp.per();
    if( pucScaleY )
    {
      iScaleY  *= pucScaleY[0];
      iScaleY >>= 4;
    }
    for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( xScale4x4Block( rcTCoeffs.get( cIdx ), pucScaleY, 1, cLQp ) );
    }
  }
  else if( b8x8 )
  {
    //===== 8x8 BLOCKS =====
    for( B8x8Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( xScale8x8Block( rcTCoeffs.get8x8( cIdx ), pucScaleY, cLQp ) );
    }
  }
  else
  {
    //===== 4x4 BLOCKS =====
    for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( xScale4x4Block( rcTCoeffs.get( cIdx ), pucScaleY, 0, cLQp ) );
    }
  }

  //===== chroma =====
  Int iScaleU  = ( g_aaiDequantCoef[cUQp.rem()][0] << cUQp.per() ) * ( pucScaleU ? pucScaleU[0] : 16 );
  Int iScaleV  = ( g_aaiDequantCoef[cVQp.rem()][0] << cVQp.per() ) * ( pucScaleU ? pucScaleU[0] : 16 );
  for( CIdx cIdx; cIdx.isLegal(); cIdx++ )
  {
    const UChar*        pucScale  = ( cIdx.plane() ? pucScaleV : pucScaleU );
    const QpParameter&  rcCQp     = ( cIdx.plane() ? cVQp      : cUQp      );
    const Int           iScale    = ( cIdx.plane() ? iScaleV   : iScaleU   );
    RNOK( xScale4x4Block( rcTCoeffs.get( cIdx ), pucScale, 1, rcCQp ) );
    rcTCoeffs.get( cIdx )[0] *= iScale;
  }

  return Err::m_nOK;
}



ErrVal
MbEncoder::xSetRdCost8x8InterMb ( IntMbTempData&      rcMbTempData,
                                  MbDataAccess*       pcMbDataAccessBaseMotion,
                                  RefListStruct&      rcRefListStruct,
                                  UInt                uiMinQP,
                                  UInt                uiMaxQP,
                                  Bool                bBLSkip,
                                  UInt                uiAdditionalBits,
                                  Frame*              pcBaseLayerRec,
                                  const YuvMbBuffer*  pcBaseLayerResidual )
{
  YuvMbBuffer   cTempPredBuffer;
  YuvMbBuffer&  rcYuvMbBuffer       = rcMbTempData;
  YuvMbBuffer&  rcTempYuvMbBuffer   = rcMbTempData.getTempYuvMbBuffer();
  YuvMbBuffer&  rcTempBLSkipBaseRes = rcMbTempData.getTempBLSkipResBuffer();
  MbDataAccess& rcMbDataAccess      = rcMbTempData.getMbDataAccess();
  MbData&       rcMbData            = rcMbDataAccess.getMbData();
  MbMode        eMbMode             = rcMbData.getMbMode();
  Bool          b8x8Mode            = ( eMbMode == MODE_8x8 || eMbMode == MODE_8x8ref0 );
  UInt          uiFwdBwd            = 0;
  Bool          bBLSkipResPred      = ( bBLSkip && rcMbTempData.getResidualPredFlag() );

  if( pcMbDataAccessBaseMotion && !pcMbDataAccessBaseMotion->getMbData().isTransformSize8x8()
      && ( rcMbDataAccess.isSCoeffPred() || rcMbDataAccess.isTCoeffPred() ) )
  {
    //8x8-trafo not allowed here...
    rcMbTempData.rdCost() = DOUBLE_MAX;
    return Err::m_nOK;
  }

  //=== check ===
  ROT(   eMbMode == MODE_SKIP && rcMbDataAccess.getSH().isPSlice() ); // not for skip mode
  ROT( ( eMbMode == MODE_8x8 || eMbMode == MODE_8x8ref0 ) && rcMbData.getBlkMode( B_8x8_0 ) != BLK_SKIP && rcMbData.getBlkMode( B_8x8_0 ) != BLK_8x8 );
  ROT( ( eMbMode == MODE_8x8 || eMbMode == MODE_8x8ref0 ) && rcMbData.getBlkMode( B_8x8_1 ) != BLK_SKIP && rcMbData.getBlkMode( B_8x8_1 ) != BLK_8x8 );
  ROT( ( eMbMode == MODE_8x8 || eMbMode == MODE_8x8ref0 ) && rcMbData.getBlkMode( B_8x8_2 ) != BLK_SKIP && rcMbData.getBlkMode( B_8x8_2 ) != BLK_8x8 );
  ROT( ( eMbMode == MODE_8x8 || eMbMode == MODE_8x8ref0 ) && rcMbData.getBlkMode( B_8x8_3 ) != BLK_SKIP && rcMbData.getBlkMode( B_8x8_3 ) != BLK_8x8 );

  //===== set forward / backward indication =====
  if( rcMbDataAccess.getSH().isBSlice() )
  {
    for( Int n = 3; n >= 0; n--)
    {
      uiFwdBwd <<= 4;
      uiFwdBwd += (0 < rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( Par8x8(n) )) ? 1:0;
      uiFwdBwd += (0 < rcMbDataAccess.getMbMotionData( LIST_1 ).getRefIdx( Par8x8(n) )) ? 2:0;
    }
  }
  if( rcMbDataAccess.getSH().isPSlice() )
  {
    for( Int n = 3; n >= 0; n--)
    {
      uiFwdBwd <<= 4;
      uiFwdBwd +=  (0 < rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( Par8x8(n) )) ? 1:0;
    }
  }
  rcMbTempData.setFwdBwd( uiFwdBwd );


  //===== get prediction signal (for residual coding) =====
  {
    RefFrameList& rcMCList0 = rcRefListStruct.acRefFrameListMC[ 0 ];
    RefFrameList& rcMCList1 = rcRefListStruct.acRefFrameListMC[ 1 ];
    if( b8x8Mode )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        RNOK( m_pcMotionEstimation->compensateSubMb( c8x8Idx, rcMbDataAccess, rcMCList0, rcMCList1,
                                                     &rcYuvMbBuffer, false, false ) );
      }
    }
    else
    {
      RNOK( m_pcMotionEstimation->compensateMb( rcMbDataAccess, rcMCList0, rcMCList1,
                                                &rcYuvMbBuffer, false ) );
    }
    if( bBLSkip )
    {
      ROF( pcBaseLayerRec );
      RNOK( m_pcMotionEstimation->compensateMbBLSkipIntra( rcMbDataAccess, &rcYuvMbBuffer, pcBaseLayerRec ) );
    }
    cTempPredBuffer.loadLuma  ( rcYuvMbBuffer );
    cTempPredBuffer.loadChroma( rcYuvMbBuffer );
  }

  //===== get prediction signal (for reconstruction) =====
  if( rcRefListStruct.bMCandRClistsDiffer )
  {
    RefFrameList& rcMCList0 = rcRefListStruct.acRefFrameListRC[ 0 ];
    RefFrameList& rcMCList1 = rcRefListStruct.acRefFrameListRC[ 1 ];
    if( b8x8Mode )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        RNOK( m_pcMotionEstimation->compensateSubMb( c8x8Idx, rcMbDataAccess, rcMCList0, rcMCList1,
                                                     &rcTempYuvMbBuffer, false, false ) );
      }
    }
    else
    {
      RNOK( m_pcMotionEstimation->compensateMb( rcMbDataAccess, rcMCList0, rcMCList1,
                                                &rcTempYuvMbBuffer, false ) );
    }
    if( bBLSkip )
    {
      ROF( pcBaseLayerRec );
      RNOK( m_pcMotionEstimation->compensateMbBLSkipIntra( rcMbDataAccess, &rcTempYuvMbBuffer, pcBaseLayerRec ) );
    }
  }
  else
  {
    rcTempYuvMbBuffer.loadLuma  ( rcYuvMbBuffer );
    rcTempYuvMbBuffer.loadChroma( rcYuvMbBuffer );
  }

  if( bBLSkipResPred )
  {
    ROF( pcBaseLayerResidual );
    //===== get actual base layer residual signal and modify prediction signal accordingly =====
    rcTempBLSkipBaseRes.loadLuma  ( *pcBaseLayerResidual );
    rcTempBLSkipBaseRes.loadChroma( *pcBaseLayerResidual );
    RNOK( m_pcMotionEstimation->updateMbBLSkipResidual( rcMbDataAccess, rcTempBLSkipBaseRes ) ); // this line makes the difference
    rcYuvMbBuffer      .subtract  ( *pcBaseLayerResidual );
    rcYuvMbBuffer      .add       (  rcTempBLSkipBaseRes );
    rcTempYuvMbBuffer  .loadLuma  (  rcYuvMbBuffer );
    rcTempYuvMbBuffer  .loadChroma(  rcYuvMbBuffer );
    cTempPredBuffer    .loadLuma  (  rcYuvMbBuffer );
    cTempPredBuffer    .loadChroma(  rcYuvMbBuffer );
  }

  //===== LOOP OVER QP VALUES =====
  Double            dMinCost  = DOUBLE_MAX;
  UInt              uiBestQP  = MSYS_UINT_MAX;
  UInt              uiBestCBP = MSYS_UINT_MAX;
  MbTransformCoeffs cBestCoeffs;
  YuvMbBuffer       cBestRec;
  for( UInt uiQP = uiMinQP; uiQP <= uiMaxQP; uiQP++ )
  {
    rcMbTempData  .setQp( uiQP );
    m_pcTransform->setQp( rcMbTempData, false );

    //===== encode residual and get rate for coefficients =====
    UInt  uiMbDist    = 0;
    UInt  uiMbBits    = 0;
    UInt  uiCoeffCost = 0;
    UInt  uiExtCbp    = 0;
    rcMbTempData.setTransformSize8x8( true );

    //--- LUMA ---
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      xSetCoeffCost( 0 );
      UInt  uiBits = 0;
      UInt  uiCbp  = 0;
      RNOK( xEncode8x8InterBlock( rcMbTempData, c8x8Idx, uiBits, uiCbp ) );
      if( uiCbp )
      {
        if( xGetCoeffCost() <= 4 )
        {
          rcYuvMbBuffer.loadLuma( cTempPredBuffer, c8x8Idx );
          rcMbTempData.clearLumaLevels8x8Block( c8x8Idx );
          if( rcMbTempData.getMbDataAccess().isSCoeffPred() )
          {
            reCalcBlock8x8(rcMbTempData, c8x8Idx, 0);
          }
          else if( rcMbTempData.getMbDataAccess().isTCoeffPred() )
          {
            reCalcBlock8x8Rewrite(rcMbTempData, c8x8Idx, 0);
          }
        }
        else
        {
          uiCoeffCost += xGetCoeffCost();
          uiExtCbp    += uiCbp;
          uiMbBits    += uiBits;
        }
      }
    }
    if( uiExtCbp && uiCoeffCost <= 5 )
    {
      rcYuvMbBuffer.loadLuma( cTempPredBuffer );
      uiExtCbp  = 0;
      uiMbBits  = 0;
      rcMbTempData.clearLumaLevels();
      if( rcMbTempData.getMbDataAccess().isSCoeffPred() )
      {
        for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
        {
          reCalcBlock8x8(rcMbTempData, c8x8Idx, 0);
        }
      }
      else if ( rcMbTempData.getMbDataAccess().isTCoeffPred() )
      {
        for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
        {
          reCalcBlock8x8Rewrite(rcMbTempData, c8x8Idx, 0);
        }
      }
    }
    //--- CHROMA ---
    RNOK( xEncodeChromaTexture( rcMbTempData, uiExtCbp, uiMbBits ) );
    rcMbTempData.cbp() = xCalcMbCbp( uiExtCbp );
    RNOK( xCheckSkipSliceMb( rcMbTempData ) );
    RNOK( xAdjustRewriteReconstruction( rcMbTempData ) );

    //===== get distortion =====
    uiMbDist  += m_pcXDistortion->getLum16x16 ( rcYuvMbBuffer.getMbLumAddr(), rcYuvMbBuffer.getLStride() );
    uiMbDist  += m_pcXDistortion->get8x8Cb    ( rcYuvMbBuffer.getMbCbAddr (), rcYuvMbBuffer.getCStride() );
    uiMbDist  += m_pcXDistortion->get8x8Cr    ( rcYuvMbBuffer.getMbCrAddr (), rcYuvMbBuffer.getCStride() );

    //===== get rate =====
    RNOK(   BitCounter::init() );
    if( !bBLSkip )
    {
      RNOK(   MbCoder::m_pcMbSymbolWriteIf->mbMode    ( rcMbTempData ) );
      if( b8x8Mode )
      {
        RNOK( MbCoder::m_pcMbSymbolWriteIf->blockModes( rcMbTempData ) );
      }
    }
    RNOK(     MbCoder::m_pcMbSymbolWriteIf->cbp       ( rcMbTempData ) );
    if( rcMbTempData.cbp() )
    {
      RNOK(   MbCoder::m_pcMbSymbolWriteIf->deltaQp   ( rcMbTempData ) );
    }
    if( rcRefListStruct.acRefFrameListRC[ 0 ].getActive() && !bBLSkip )
    {
      RNOK(   MbCoder::xWriteMotionPredFlags          ( rcMbTempData, eMbMode, LIST_0 ) );
      RNOK(   MbCoder::xWriteReferenceFrames          ( rcMbTempData, eMbMode, LIST_0 ) );
      RNOK(   MbCoder::xWriteMotionVectors            ( rcMbTempData, eMbMode, LIST_0 ) );
    }
    if( rcRefListStruct.acRefFrameListRC[ 1 ].getActive() && !bBLSkip )
    {
      RNOK(   MbCoder::xWriteMotionPredFlags          ( rcMbTempData, eMbMode, LIST_1 ) );
      RNOK(   MbCoder::xWriteReferenceFrames          ( rcMbTempData, eMbMode, LIST_1 ) );
      RNOK(   MbCoder::xWriteMotionVectors            ( rcMbTempData, eMbMode, LIST_1 ) );
    }

    uiMbBits  += BitCounter::getNumberOfWrittenBits();

    //===== set rd-cost =====
    if( rcMbTempData.getSH().getTCoeffLevelPredictionFlag() &&
        rcMbTempData.getMbCbp() == 0 &&
        rcMbTempData.getQp   () != rcMbDataAccess.getLastQp() )
    {
      rcMbTempData.rdCost() = DOUBLE_MAX;
    }
    else
    {
      rcMbTempData.rdCost() = m_pcRateDistortionIf->getCost( uiMbBits + uiAdditionalBits, uiMbDist );
    }

#if PROPOSED_DEBLOCKING_APRIL2010
    RNOK( xCheckInterProfileCompatibility( rcMbTempData, &rcTempYuvMbBuffer, ( bBLSkipResPred ? &rcTempBLSkipBaseRes : m_pcRefLayerResidual ), true ) );
#endif

    //===== check r-d cost =====
    if( rcMbTempData.rdCost() < dMinCost )
    {
      dMinCost  = rcMbTempData.rdCost     ();
      uiBestQP  = rcMbTempData.getQp      ();
      uiBestCBP = rcMbTempData.getMbExtCbp();
      cBestCoeffs .copyFrom   ( rcMbTempData.getMbTCoeffs() );
      cBestRec    .loadLuma   ( rcMbTempData );
      cBestRec    .loadChroma ( rcMbTempData );
    }

    //===== reset prediction signal =====
    rcMbTempData.loadLuma   ( cTempPredBuffer );
    rcMbTempData.loadChroma ( cTempPredBuffer );
  }

  //===== set best data =====
  rcMbTempData.rdCost() = dMinCost;
  rcMbTempData.setQp        ( uiBestQP );
  rcMbTempData.setMbExtCbp  ( uiBestCBP );
  rcMbTempData.getMbTCoeffs ().copyFrom( cBestCoeffs );
  rcMbTempData.loadLuma     ( cBestRec );
  rcMbTempData.loadChroma   ( cBestRec );

  //===== reset transform status =====
  m_pcTransform->setQp( rcMbDataAccess, false );

  //---- correct prediction signal ----
  if( rcRefListStruct.bMCandRClistsDiffer )
  {
    rcYuvMbBuffer.subtract( cTempPredBuffer );
    rcYuvMbBuffer.add     ( rcTempYuvMbBuffer );
  }

  return Err::m_nOK;
}


#if PROPOSED_DEBLOCKING_APRIL2010
ErrVal
MbEncoder::xCheckInterProfileCompatibility( IntMbTempData&      rcMbTempData,
                                            const YuvMbBuffer*  pcPredSignal,
                                            const YuvMbBuffer*  pcRefLayerResidual,
                                            Bool                b8x8 )
{
  const SequenceParameterSet& rcSPS = rcMbTempData.getSH().getSPS();
  ROFRS( rcMbTempData.getResidualPredFlag(),                                                    Err::m_nOK );
  ROTRS( rcSPS.getProfileIdc() == SCALABLE_BASELINE_PROFILE && !rcSPS.getConstrainedSet1Flag(), Err::m_nOK );
  ROTRS( rcSPS.getProfileIdc() == SCALABLE_HIGH_PROFILE     && !rcSPS.getConstrainedSet0Flag(), Err::m_nOK );
  ROF  ( pcPredSignal );
  ROF  ( pcRefLayerResidual );
  YuvMbBuffer cRecRes;
  cRecRes.loadLuma( rcMbTempData );
  cRecRes.subtract( *pcPredSignal );
  cRecRes.addRes  ( *pcRefLayerResidual );
  if( b8x8 )
  {
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      Bool    bNonZeroCoeff = false;
      TCoeff* pcCoeff       = rcMbTempData.get8x8( c8x8Idx );
      for( UInt ui = 0; ui < 64; ui++ )
      {
        if( pcCoeff[ ui ] != 0 )
        {
          bNonZeroCoeff = true;
          break;
        }
      }
      if( bNonZeroCoeff )
      {
        Bool  bNonZeroSample  = false;
        XPel* pBlk            = cRecRes.getYBlk( c8x8Idx );
        UInt  uiStride        = cRecRes.getLStride();
        for( UInt uiY = 0; uiY < 8 && !bNonZeroSample; uiY++, pBlk += uiStride )
        {
          for( UInt uiX = 0; uiX < 8; uiX++ )
          {
            if( pBlk[ uiX ] != 0 )
            {
              bNonZeroSample = true;
              break;
            }
          }
        }
        if( !bNonZeroSample )
        {
          rcMbTempData.rdCost() = DOUBLE_MAX;
          break;
        }
      }
    }
  }
  else
  {
    for( B4x4Idx c4x4Idx; c4x4Idx.isLegal(); c4x4Idx++ )
    {
      Bool    bNonZeroCoeff = false;
      TCoeff* pcCoeff       = rcMbTempData.get( c4x4Idx );
      for( UInt ui = 0; ui < 16; ui++ )
      {
        if( pcCoeff[ ui ] != 0 )
        {
          bNonZeroCoeff = true;
          break;
        }
      }
      if( bNonZeroCoeff )
      {
        Bool  bNonZeroSample  = false;
        XPel* pBlk            = cRecRes.getYBlk( c4x4Idx );
        UInt  uiStride        = cRecRes.getLStride();
        for( UInt uiY = 0; uiY < 4 && !bNonZeroSample; uiY++, pBlk += uiStride )
        {
          for( UInt uiX = 0; uiX < 4; uiX++ )
          {
            if( pBlk[ uiX ] != 0 )
            {
              bNonZeroSample = true;
              break;
            }
          }
        }
        if( !bNonZeroSample )
        {
          rcMbTempData.rdCost() = DOUBLE_MAX;
          break;
        }
      }
    }
  }
  return Err::m_nOK;
}
#endif



ErrVal
MbEncoder::xSetRdCostInterSubMb( IntMbTempData&  rcMbTempData,
                                 RefListStruct&  rcRefListStruct,
                                 B8x8Idx         c8x8Idx,
                                 Bool            bTrafo8x8,
                                 UInt            uiAddBits,
                                 Bool            bLowComplexity )   // JVT-V079
{
  YuvMbBuffer&  rcYuvMbBuffer     = rcMbTempData;
  YuvMbBuffer&  rcTempYuvMbBuffer = rcMbTempData.getTempYuvMbBuffer();
  MbDataAccess& rcMbDataAccess    = rcMbTempData.getMbDataAccess();
  UInt          uiSubMbDist       = 0;
  UInt          uiSubMbBits       = 0;
  RefFrameList& rcRefFrameList0   = rcRefListStruct.acRefFrameListMC[ 0 ];
  RefFrameList& rcRefFrameList1   = rcRefListStruct.acRefFrameListMC[ 1 ];

  if( bLowComplexity )
  {
    if( rcMbTempData.rdCost() == DOUBLE_MAX )
    {
      //===== get prediction and copy to temp buffer =====
      RNOK( m_pcMotionEstimation->compensateSubMb( c8x8Idx, rcMbDataAccess,
                                                   rcRefFrameList0, rcRefFrameList1,
                                                   &rcYuvMbBuffer, false, false ) );
      rcTempYuvMbBuffer.loadLuma( rcYuvMbBuffer, c8x8Idx );

      //===== get distortion =====
      m_pcIntOrgMbPelData->set4x4Block( c8x8Idx );
      rcYuvMbBuffer       .set4x4Block( c8x8Idx );
      uiSubMbDist += m_pcXDistortion->getLum8x8 ( rcYuvMbBuffer.getLumBlk(), rcYuvMbBuffer.getLStride(), DF_SAD );

      //===== set rd-cost =====
      rcMbTempData.rdCost() = uiSubMbDist;
    }
  }
  else
  {
    //===== get prediction and copy to temp buffer =====
    RNOK( m_pcMotionEstimation->compensateSubMb( c8x8Idx, rcMbDataAccess, rcRefFrameList0, rcRefFrameList1,
                                                 &rcYuvMbBuffer, false, false ) );
    rcTempYuvMbBuffer.loadLuma( rcYuvMbBuffer, c8x8Idx );


    //===== encode residual and get rate for coefficients =====
    xSetCoeffCost( 0 );
    UInt  uiBits = 0;
    UInt  uiCbp  = 0;
    if( bTrafo8x8 )
    {
      RNOK( xEncode8x8InterBlock( rcMbTempData, c8x8Idx, uiBits, uiCbp ) );
      if( uiCbp )
      {
        if( xGetCoeffCost() <= 4 )
        {
          rcYuvMbBuffer.loadLuma( rcTempYuvMbBuffer, c8x8Idx );

          rcMbTempData.clearLumaLevels8x8Block( c8x8Idx );

          if( rcMbTempData.getMbDataAccess().isSCoeffPred() )
          {
            reCalcBlock8x8(rcMbTempData, c8x8Idx, 0);
          }
          else if ( rcMbTempData.getMbDataAccess().isTCoeffPred() )
          {
            reCalcBlock8x8Rewrite(rcMbTempData, c8x8Idx, 0);
          }
        }
        else
        {
          uiSubMbBits += uiBits;
        }
      }
    }
    else
    {
      for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
      {
        RNOK( xEncode4x4InterBlock( rcMbTempData, cIdx, uiBits, uiCbp ) );
      }
      if( uiCbp )
      {
        if( xGetCoeffCost() <= 4 )
        {
          rcYuvMbBuffer.loadLuma( rcTempYuvMbBuffer, c8x8Idx );

          rcMbTempData.clearLumaLevels8x8( c8x8Idx );

          if( rcMbTempData.getMbDataAccess().isSCoeffPred() )
          {
            reCalcBlock8x8(rcMbTempData, c8x8Idx, 1);
          }
          else if( rcMbTempData.getMbDataAccess().isTCoeffPred() )
          {
            reCalcBlock8x8Rewrite(rcMbTempData, c8x8Idx, 1);
          }
        }
        else
        {
          uiSubMbBits += uiBits;
        }
      }
    }
    //===== get distortion =====
    m_pcIntOrgMbPelData->set4x4Block( c8x8Idx );
    rcYuvMbBuffer       .set4x4Block( c8x8Idx );
    uiSubMbDist += m_pcXDistortion->getLum8x8 ( rcYuvMbBuffer.getLumBlk(), rcYuvMbBuffer.getLStride() );

    //===== get rate =====
    uiSubMbBits += uiAddBits;

    //===== set rd-cost =====
    rcMbTempData.rdCost() = m_pcRateDistortionIf->getCost( uiSubMbBits, uiSubMbDist );
  }

  return Err::m_nOK;
}






ErrVal
MbEncoder::xEstimateMbDirect( IntMbTempData*& rpcMbTempData,
                              IntMbTempData*& rpcMbBestData,
                              RefListStruct&  rcRefListStruct,
                              UInt            uiMinQP,
                              UInt            uiMaxQP,
                              MbDataAccess*   pcMbDataAccessBaseMotion,
                              UInt            uiMaxNumMv,
                              Bool            bResidualPred,
                              UInt            uiQp,
                              Bool            bSkipModeAllowed)
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListME[ 1 ];
  ROF  ( uiMaxNumMv );
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag(),                   Err::m_nOK );
  ROFRS( rcRefFrameList0.getActive() && rcRefFrameList1.getActive(),  Err::m_nOK );
  ROFRS( m_bUseBDir,                                                  Err::m_nOK );

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  rpcMbTempData->clear              ();
  rpcMbTempData->setMbMode          ( MODE_SKIP );
  rpcMbTempData->setBLSkipFlag      ( false );
  rpcMbTempData->setResidualPredFlag( bResidualPred );
  rpcMbTempData->getMbMvdData       ( LIST_0 ).setAllMv       ( Mv::ZeroMv() );
  rpcMbTempData->getMbMvdData       ( LIST_1 ).setAllMv       ( Mv::ZeroMv() );
  rpcMbTempData->getMbMotionData    ( LIST_0 ).setMotPredFlag ( false );
  rpcMbTempData->getMbMotionData    ( LIST_1 ).setMotPredFlag ( false );
  RNOK( rpcMbTempData->getMbDataAccess().setSVCDirectModeMvAndRef( rcRefFrameList0, rcRefFrameList1 ) );

  UInt uiNumMv = ( rpcMbTempData->getMbMotionData( LIST_0 ).getRefIdx() > 0 ? 1 : 0 ) + ( rpcMbTempData->getMbMotionData( LIST_1 ).getRefIdx() > 0 ? 1 : 0 );

  if( rpcMbTempData->getSH().isH264AVCCompatible() )
  {
    //===== H.264/AVC compatible direct mode =====
    Bool            bOneMv          = false;
    Bool            bFaultTolerant  = false;
    MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
    B8x8Idx         c8x8Idx;
    ROFRS( rcMbDataAccess.getMvPredictorDirect( c8x8Idx.b8x8(), bOneMv, bFaultTolerant, &rcRefFrameList0, &rcRefFrameList1 ), Err::m_nOK ); c8x8Idx++;
    ROFRS( rcMbDataAccess.getMvPredictorDirect( c8x8Idx.b8x8(), bOneMv, bFaultTolerant, &rcRefFrameList0, &rcRefFrameList1 ), Err::m_nOK ); c8x8Idx++;
    ROFRS( rcMbDataAccess.getMvPredictorDirect( c8x8Idx.b8x8(), bOneMv, bFaultTolerant, &rcRefFrameList0, &rcRefFrameList1 ), Err::m_nOK ); c8x8Idx++;
    ROFRS( rcMbDataAccess.getMvPredictorDirect( c8x8Idx.b8x8(), bOneMv, bFaultTolerant, &rcRefFrameList0, &rcRefFrameList1 ), Err::m_nOK );
    uiNumMv = 0;
    for( B8x8Idx c8x8; c8x8.isLegal(); c8x8++ )
    {
      uiNumMv += ( rpcMbTempData->getMbMotionData( LIST_0 ).getRefIdx( c8x8.b8x8Index() ) > 0 ? 1 : 0 );
      uiNumMv += ( rpcMbTempData->getMbMotionData( LIST_1 ).getRefIdx( c8x8.b8x8Index() ) > 0 ? 1 : 0 );
    }
	}
  ROFRS( uiNumMv <= uiMaxNumMv, Err::m_nOK );

  IntMbTempData* pcMbRefData = rpcMbTempData;
  RNOK ( xSetRdCostInterMb( *rpcMbTempData, pcMbDataAccessBaseMotion, rcRefListStruct, uiMinQP, uiMaxQP, bLowComplexMbEnable ) );
  ROFRS( bSkipModeAllowed || rpcMbTempData->getMbCbp(), Err::m_nOK );

  if ( bLowComplexMbEnable )
  {
    Int tmp_max_qp = ( uiQp - SHIFT_QP );
    if ( tmp_max_qp<0) tmp_max_qp=0;
    rpcMbTempData->rdCost() -= 8 * g_iQP2quant[tmp_max_qp];
  }

  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2=0,distortion=0;
	  for( Int n = 0; n <1; n++)
	  {
		  Int iRefIdx[2];
		  iRefIdx [0]=rpcMbTempData->getMbMotionData(LIST_0).getRefIdx(B4x4Idx(n));
		  iRefIdx [1]=rpcMbTempData->getMbMotionData(LIST_1).getRefIdx(B4x4Idx(n));
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,0,0,4,4);
		  }
		  if(pcRefFrame1)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,0,0,4,4);
			  if(pcRefFrame0)
				  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
			  else
				  distortion1=distortion2;
		  }
		  distortion+=distortion1;
	  }
	  setEpRef(distortion);
	  rpcMbTempData->rdCost()+=distortion;
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  RNOK( xCheckInterMbMode8x8(  rpcMbTempData, rpcMbBestData, pcMbRefData, rcRefListStruct, uiMinQP, uiMaxQP, false, pcMbDataAccessBaseMotion ) );

  return Err::m_nOK;
}



ErrVal
MbEncoder::xEstimateMbBLSkip( IntMbTempData*&     rpcMbTempData,
                              IntMbTempData*&     rpcMbBestData,
                              RefListStruct&      rcRefListStruct,
                              UInt                uiMinQP,
                              UInt                uiMaxQP,
                              const Frame*        pcBaseLayerRec,
                              UInt                uiMaxNumMv,
                              Bool                bBiPred8x8Disable,
                              Bool                bBSlice,
                              MbDataAccess*       pcMbDataAccessBase,
                              MbDataAccess&       rcMbDataAccess,
                              Bool                bResidualPred,
                              const YuvMbBuffer*  pcBLResidual )
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListME[ 1 ];
  ROF  ( uiMaxNumMv );
  ROF  ( pcMbDataAccessBase );
  ROFRS( m_bBaseModeAllowedFlag, Err::m_nOK );

  PicType eMbPicType = rcMbDataAccess.getMbPicType();

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  if( eMbPicType != FRAME && pcMbDataAccessBase->getMbPicType() == FRAME )
  {
    // check inter intra mixed modes
    if( pcMbDataAccessBase->getMbData().isIntra() != pcMbDataAccessBase->getMbDataComplementary().isIntra() )
    {
      printf( "  %d %d ", pcMbDataAccessBase->getMbData().getMbMode(), pcMbDataAccessBase->getMbDataComplementary().getMbMode());
      return Err::m_nOK;
    }
    if( ! pcMbDataAccessBase->getMbData().isIntra() && pcMbDataAccessBase->getMbData().getSliceId() != pcMbDataAccessBase->getMbDataComplementary().getSliceId() )
    {
      printf( "  %d %d ", pcMbDataAccessBase->getMbData().getSliceId(), pcMbDataAccessBase->getMbDataComplementary().getSliceId());
      return Err::m_nOK;
    }
  }

  if( ! pcMbDataAccessBase->getMbData().isIntra() )
  {
    ROTRS( rpcMbTempData->getSH().getSliceSkipFlag() && !bResidualPred, Err::m_nOK );

    Bool bRefIdxNotAvailable = false;
    for( UInt uiListIdx = 0; uiListIdx < 2 && !bRefIdxNotAvailable; uiListIdx++ )
    {
      RefFrameList& rcRefList = ( uiListIdx ? rcRefFrameList1 : rcRefFrameList0 );
      MbMotionData& rcMotData = pcMbDataAccessBase->getMbMotionData( ListIdx( uiListIdx ) );
      for( UInt uiBlk = 0; uiBlk < 4 && !bRefIdxNotAvailable; uiBlk++ )
      {
        Int iRefIdx         = rcMotData.getRefIdx( Par8x8( uiBlk ) );
        bRefIdxNotAvailable = ( iRefIdx > (Int)rcRefList.getActive() );
      }
    }
    ROTRS( bRefIdxNotAvailable, Err::m_nOK );
    if( rpcMbTempData->getSH().getSliceType() == P_SLICE )
    {
      UInt   uiFwdBwd   = pcMbDataAccessBase->getMbData().getFwdBwd();
      Bool   bAvailable = ( ( uiFwdBwd & 0x1111 ) == 0x1111 );
      ROFRS( bAvailable, Err::m_nOK );
    }

    //===== BASE LAYER MODE IS INTER =====
    rpcMbTempData->clear               ();
    rpcMbTempData->copyMotion          ( pcMbDataAccessBase->getMbData() );
    rpcMbTempData->setBLSkipFlag       ( true  );
    rpcMbTempData->getMbMvdData        ( LIST_0 ).setAllMv( Mv::ZeroMv() );
    rpcMbTempData->getMbMvdData        ( LIST_1 ).setAllMv( Mv::ZeroMv() );
    rpcMbTempData->setResidualPredFlag ( bResidualPred );

    UInt  uiNumMv                         = 0;
    Bool  bBiPredForBlocksSmallerThan8x8  = false;
    switch( rpcMbTempData->getMbDataAccess().getMbData().getMbMode() )
    {
    case MODE_16x16:
      uiNumMv += ( rpcMbTempData->getMbMotionData( LIST_0 ).getRefIdx() > 0 ? 1 : 0 );
      uiNumMv += ( rpcMbTempData->getMbMotionData( LIST_1 ).getRefIdx() > 0 ? 1 : 0 );
      break;
    case MODE_16x8:
    case MODE_8x16:
      uiNumMv += ( rpcMbTempData->getMbMotionData( LIST_0 ).getRefIdx( PART_8x8_0 ) > 0 ? 1 : 0 );
      uiNumMv += ( rpcMbTempData->getMbMotionData( LIST_0 ).getRefIdx( PART_8x8_3 ) > 0 ? 1 : 0 );
      uiNumMv += ( rpcMbTempData->getMbMotionData( LIST_1 ).getRefIdx( PART_8x8_0 ) > 0 ? 1 : 0 );
      uiNumMv += ( rpcMbTempData->getMbMotionData( LIST_1 ).getRefIdx( PART_8x8_3 ) > 0 ? 1 : 0 );
      break;
    case MODE_8x8:
    case MODE_8x8ref0:
      {
        for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
        {
          UInt uiSListMv = 0;
          switch( rpcMbTempData->getMbDataAccess().getMbData().getBlkMode( c8x8Idx.b8x8Index() ) )
          {
          case BLK_8x8: uiSListMv = 1;  break;
          case BLK_8x4:
          case BLK_4x8: uiSListMv = 2;  break;
          case BLK_4x4: uiSListMv = 4;  break;
          default:      ROT(1);
          }
          UInt uiNumList = ( rpcMbTempData->getMbMotionData( LIST_0 ).getRefIdx( c8x8Idx.b8x8Index() ) > 0 ? 1 : 0 );
          uiNumList     += ( rpcMbTempData->getMbMotionData( LIST_1 ).getRefIdx( c8x8Idx.b8x8Index() ) > 0 ? 1 : 0 );
          if( uiNumList == 2 && uiSListMv > 1 )
          {
            bBiPredForBlocksSmallerThan8x8 = true;
          }
          uiNumMv += ( uiNumList * uiSListMv );
        }
      }
      break;
    default:
      ROT(1);
    }
    ROTRS( bBiPred8x8Disable && bBiPredForBlocksSmallerThan8x8, Err::m_nOK );
    ROFRS( uiNumMv <= uiMaxNumMv,                               Err::m_nOK );

    IntMbTempData* pcMbRefData = rpcMbTempData;

    RNOK( xSetRdCostInterMb( *rpcMbTempData, pcMbDataAccessBase, rcRefListStruct, uiMinQP, uiMaxQP, bLowComplexMbEnable, true, 0, (Frame*)pcBaseLayerRec, pcBLResidual ) ) ;

    //JVT-R057 LA-RDO{
    if(m_bLARDOEnable)
    {
      MbDataAccess&  rcMbDataAccess1  = rpcMbTempData->getMbDataAccess();
      Int distortion1=0,distortion2=0,distortion=0;
      //Bug_Fix JVT-R057 0806{

      for( Int n = 0; n <16; n++)
      {
        Int iRefIdx[2];
        iRefIdx [0]=rcMbDataAccess1.getMbMotionData(LIST_0).getRefIdx(B4x4Idx(n));
        iRefIdx [1]=rcMbDataAccess1.getMbMotionData(LIST_1).getRefIdx(B4x4Idx(n));
        Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
        Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
        Int iMvX;
        Int iMvY;

        if(pcRefFrame0)
        {
          iMvX=rcMbDataAccess1.getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getHor();
          iMvY=rcMbDataAccess1.getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getVer();
          getChannelDistortion(rcMbDataAccess1,*pcRefFrame0,&distortion1,iMvX,iMvY,n%4,n/4,1,1);
        }
        if(pcRefFrame1)
        {
          iMvX=rcMbDataAccess1.getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getHor();
          iMvY=rcMbDataAccess1.getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getVer();
          getChannelDistortion(rcMbDataAccess1,*pcRefFrame1,&distortion2,iMvX,iMvY,n%4,n/4,1,1);
          distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
        }
        distortion+=distortion1;
      }
      //Bug_Fix JVT-R057 0806}

      setEpRef(distortion);
      rpcMbTempData->rdCost()+=distortion;
    }
    //JVT-R057 LA-RDO}

    RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

    RNOK( xCheckInterMbMode8x8( rpcMbTempData, rpcMbBestData, pcMbRefData, rcRefListStruct, uiMinQP, uiMaxQP, true, pcMbDataAccessBase, (Frame*)pcBaseLayerRec, pcBLResidual ) );
  }
  else
  {
    //===== INTRA MODE =====
    if ( pcMbDataAccessBase->getMbData().getInCropWindowFlag() ) // TMM_ESS
	    RNOK( xEstimateMbIntraBL  ( rpcMbTempData, rpcMbBestData, uiMinQP, uiMaxQP, pcBaseLayerRec, bBSlice, pcMbDataAccessBase ) );
  }

  return Err::m_nOK;
}


ErrVal
MbEncoder::xEstimateMbSkip( IntMbTempData*&  rpcMbTempData,
                            IntMbTempData*&  rpcMbBestData,
                            RefListStruct&   rcRefListStruct,
                            Bool             bResidualPred,
                            Bool             bSkipModeAllowed )
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  ROFRS( bSkipModeAllowed,                                  Err::m_nOK );
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag(),         Err::m_nOK );
  ROFRS( rpcMbTempData->getSH().getSliceType() == P_SLICE,  Err::m_nOK );
  ROF( rcRefFrameList0.getActive() );

  Int iRefIdxL0 = 1;
  Int iRefIdxL1 = BLOCK_NOT_PREDICTED;
  Mv  cMvPredL0;
  Mv  cMvPredL1;

  //JVT-V079 Low-complexity MB mode decision
  Bool   bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  rpcMbTempData->clear();
  rpcMbTempData->setMbMode( MODE_SKIP );
  rpcMbTempData->setBLSkipFlag( false );
  rpcMbTempData->getMbDataAccess().getMvPredictorSkipMode( cMvPredL0 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx( iRefIdxL0 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMvPredL0 );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( Mv::ZeroMv() );
  rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx( iRefIdxL1 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMvPredL1 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( Mv::ZeroMv() );
  rpcMbTempData->setResidualPredFlag( bResidualPred );

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( false );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( false );

  UInt  uiQP = rpcMbTempData->getMbDataAccess().getMbData().getQp();
  RNOK( xSetRdCostInterMb( *rpcMbTempData, NULL, rcRefListStruct, uiQP, uiQP, bLowComplexMbEnable ) );


  //JVT-R057 LA-RDO}
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2=0,distortion=0;
	  for( Int n = 0; n <1; n++)
	  {
		  Int iRefIdx[2];
		  iRefIdx [0]=rpcMbTempData->getMbMotionData(LIST_0).getRefIdx(B4x4Idx(n));
		  iRefIdx [1]=rpcMbTempData->getMbMotionData(LIST_1).getRefIdx(B4x4Idx(n));
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,0,0,4,4);
		  }
		  if(pcRefFrame1)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,0,0,4,4);
			  if(pcRefFrame0)
				  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
			  else
				  distortion1=distortion2;
		  }
		  distortion+=distortion1;
	  }
	  setEpRef(distortion);
	  rpcMbTempData->rdCost()+=distortion;
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}


__inline
UInt
getRefIdxBits( Int iRefIdx, RefFrameList& rcRefFrameList )
{
  AOT  ( rcRefFrameList.getActive() == 0    );
  ROTRS( rcRefFrameList.getActive() == 1, 0 );
  ROTRS( rcRefFrameList.getActive() == 2, 1 );

  return g_aucFrameBits[ iRefIdx ];
}

ErrVal
MbEncoder::xEstimateMb16x16( IntMbTempData*&  rpcMbTempData,
                             IntMbTempData*&  rpcMbBestData,
                             RefListStruct&   rcRefListStruct,
                             UInt             uiMinQP,
                             UInt             uiMaxQP,
                             UInt             uiMaxNumMv,
                             UInt             uiNumMaxIter,
                             UInt             uiIterSearchRange,
                             MbDataAccess*    pcMbDataAccessBase,
                             Bool             bResidualPred )
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListME[ 1 ];
  ROF  ( uiMaxNumMv );
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag(), Err::m_nOK );
  ROF  ( rcRefFrameList0.getActive() <= 32 );
  ROF  ( rcRefFrameList1.getActive() <= 32 );
  ROF  ( rcRefFrameList0.getActive() );

  Bool        bPSlice       = rpcMbTempData->getMbDataAccess().getSH().isPSlice();
  Double      fRdCost       = 0;
  UInt        uiCost  [3]   = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX }, uiCostTest;
  UInt        uiMbBits[3]   = { ( ! bPSlice ? 3 : 1 ), 3, 5 };
  Int         iRefIdx [2]   = { 0, 0 }, iRefIdxBi[2], iRefIdxTest;
  UInt        uiBits  [3]   = { 0, 0, 0 }, uiBitsTest;
  Mv          cMv[2], cMvBi[2], cMvPred[2][33], cMvLastEst[2][33], cMvd[2];
  YuvMbBuffer cYuvMbBuffer[2];
  Frame*      pcRefFrame    = 0;

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  Bool        bBLPred   [2] = { false, false };
  Bool        bBLPredBi [2] = { false, false };
  Int         iBLRefIdx [2] = { -1, -1 };
  Mv          cBLMvPred [2], cBLMvLastEst[2];

  if( pcMbDataAccessBase && m_bBaseMotionPredAllowed )
  {
    if( pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx() >  0                           &&
        pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx() <= (Int)rcRefFrameList0.getActive()   )
    {
      iBLRefIdx [0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx ();
      cBLMvPred [0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ();
    }
    if( pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx() >  0                           &&
        pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx() <= (Int)rcRefFrameList1.getActive()   )
    {
      iBLRefIdx [1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx ();
      cBLMvPred [1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ();
    }
  }

  UInt  uiBasePredType = MSYS_UINT_MAX;
  rpcMbTempData->clear();
  rpcMbTempData->setMbMode( MODE_16x16 );
  rpcMbTempData->setBLSkipFlag( false );
  rpcMbTempData->setResidualPredFlag( bResidualPred );

  //===== LIST 0 PREDICTION ======
  for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList0.getActive(); iRefIdxTest++ )
  {
    pcRefFrame = rcRefFrameList0[iRefIdxTest];

    if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
    {
      rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[0][iRefIdxTest], iRefIdxTest, LIST_0 );
      uiBitsTest                  = ( uiBasePredType == 0 ? 0 : uiMbBits[0] ) + getRefIdxBits( iRefIdxTest, rcRefFrameList0 );
      cMvLastEst[0][iRefIdxTest]  = cMvPred [0][iRefIdxTest];
      if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[0] )
      {
        cMvLastEst[0][iRefIdxTest] = cBLMvPred[0];
        m_pcMotionEstimation->setEL( true );
      }
      RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                          cMvLastEst[0][iRefIdxTest],
                                                          cMvPred   [0][iRefIdxTest],
                                                          uiBitsTest, uiCostTest,
                                                          PART_16x16, MODE_16x16, 0,
                                                          &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
      if( uiCostTest < uiCost[0] )
      {
        bBLPred   [0] = false;
        iRefIdx   [0] = iRefIdxTest;
        cMv       [0] = cMvLastEst[0][iRefIdxTest];
        uiBits    [0] = uiBitsTest;
        uiCost    [0] = uiCostTest;
      }
    }

    if( iRefIdxTest == iBLRefIdx[0] )
    {
      uiBitsTest      = ( uiBasePredType == 0 ? 0 : uiMbBits[0] );
      cBLMvLastEst[0] = cBLMvPred [0];
      m_pcMotionEstimation->setEL( true );

      rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[0], LIST_0 );
      RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                          cBLMvLastEst[0],
                                                          cBLMvPred   [0],
                                                          uiBitsTest, uiCostTest,
                                                          PART_16x16, MODE_16x16, 0,
                                                          &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
      if( uiCostTest < uiCost[0] )
      {
        bBLPred [0] = true;
        iRefIdx [0] = iRefIdxTest;
        cMv     [0] = cBLMvLastEst[0];
        uiBits  [0] = uiBitsTest;
        uiCost  [0] = uiCostTest;
      }
    }
  }


  //===== LIST 1 PREDICTION =====
  for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList1.getActive(); iRefIdxTest++ )
  {
    pcRefFrame = rcRefFrameList1[iRefIdxTest];

    if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
    {
      rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[1][iRefIdxTest], iRefIdxTest, LIST_1 );
      uiBitsTest                  = ( uiBasePredType == 1 ? 0 : uiMbBits[1] ) + getRefIdxBits( iRefIdxTest, rcRefFrameList1 );
      cMvLastEst[1][iRefIdxTest]  = cMvPred [1][iRefIdxTest];
      if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[1] )
      {
        cMvLastEst[1][iRefIdxTest] = cBLMvPred[1];
        m_pcMotionEstimation->setEL( true );
      }
      RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                          cMvLastEst[1][iRefIdxTest],
                                                          cMvPred   [1][iRefIdxTest],
                                                          uiBitsTest, uiCostTest,
                                                          PART_16x16, MODE_16x16, 0,
                                                          &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
      if( uiCostTest < uiCost[1] )
      {
        bBLPred   [1] = false;
        iRefIdx   [1] = iRefIdxTest;
        cMv       [1] = cMvLastEst[1][iRefIdxTest];
        uiBits    [1] = uiBitsTest;
        uiCost    [1] = uiCostTest;

        RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[1],
          PART_16x16, MODE_16x16 ) );
      }
    }

    if( iRefIdxTest == iBLRefIdx[1] )
    {
      uiBitsTest      = ( uiBasePredType == 1 ? 0 : uiMbBits[1] );
      cBLMvLastEst[1] = cBLMvPred [1];
      m_pcMotionEstimation->setEL( true );

      rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[1], LIST_1 );
      RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                          cBLMvLastEst[1],
                                                          cBLMvPred   [1],
                                                          uiBitsTest, uiCostTest,
                                                          PART_16x16, MODE_16x16, 0,
                                                          &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
      if( uiCostTest < uiCost[1] )
      {
        bBLPred [1] = true;
        iRefIdx [1] = iRefIdxTest;
        cMv     [1] = cBLMvLastEst[1];
        uiBits  [1] = uiBitsTest;
        uiCost  [1] = uiCostTest;

        RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[1],
                                                          PART_16x16, MODE_16x16 ) );
      }
    }
  }

  ROTRS( uiCost[0] == MSYS_UINT_MAX && uiCost[1] == MSYS_UINT_MAX, Err::m_nOK );
  Bool bBiPredOk = ( uiCost[0] != MSYS_UINT_MAX && uiCost[1] != MSYS_UINT_MAX );

  //===== BI PREDICTION =====
  if( bBiPredOk && rcRefFrameList0.getActive() && rcRefFrameList1.getActive() && uiMaxNumMv >= 2 )
  {
    //----- initialize with forward and backward estimation -----
    cMvBi           [0] = cMv     [0];
    cMvBi           [1] = cMv     [1];
    iRefIdxBi       [0] = iRefIdx [0];
    iRefIdxBi       [1] = iRefIdx [1];
    bBLPredBi       [0] = bBLPred [0];
    bBLPredBi       [1] = bBLPred [1];
    UInt  uiMotBits [2] = { uiBits[0] - uiMbBits[0], uiBits[1] - uiMbBits[1] };
    uiBits          [2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];

    if( ! uiNumMaxIter )
    {
      uiNumMaxIter      = 1;
      uiIterSearchRange = 0;
    }

    //----- iterative search -----
    for( UInt uiIter = 0; uiIter < uiNumMaxIter; uiIter++ )
    {
      Bool          bChanged        = false;
      UInt          uiDir           = uiIter % 2;
      RefFrameList& rcRefFrameList  = ( uiDir ? rcRefFrameList1 : rcRefFrameList0 );
      BSParams      cBSParams;
      cBSParams.pcAltRefFrame       = ( uiDir ? rcRefFrameList0 : rcRefFrameList1 )[ iRefIdxBi[ 1 - uiDir ] ];
      cBSParams.pcAltRefPelData     = &cYuvMbBuffer[1-uiDir];
      cBSParams.uiL1Search          = uiDir;
      cBSParams.apcWeight[LIST_0]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxBi[LIST_0], rpcMbTempData->getFieldFlag() );
      cBSParams.apcWeight[LIST_1]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxBi[LIST_1], rpcMbTempData->getFieldFlag() );

      for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList.getActive(); iRefIdxTest++ )
      {
        if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
        {
          uiBitsTest  = ( uiBasePredType == 2 ? 0 : uiMbBits[2] ) + uiMotBits[1-uiDir] + getRefIdxBits( iRefIdxTest, rcRefFrameList );
          pcRefFrame  = rcRefFrameList[iRefIdxTest];
          RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                              cMvLastEst[uiDir][iRefIdxTest],
                                                              cMvPred   [uiDir][iRefIdxTest],
                                                              uiBitsTest, uiCostTest,
                                                              PART_16x16, MODE_16x16, 
                                                              uiIterSearchRange, 0, &cBSParams ) );

          if( uiCostTest < uiCost[2] )
          {
            bChanged          = true;
            bBLPredBi [uiDir] = false;
            iRefIdxBi [uiDir] = iRefIdxTest;
            cMvBi     [uiDir] = cMvLastEst[uiDir][iRefIdxTest];
            uiMotBits [uiDir] = uiBitsTest - uiMbBits[2] - uiMotBits[1-uiDir];
            uiBits    [2]     = uiBitsTest;
            uiCost    [2]     = uiCostTest;

            RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[uiDir],
                                                              PART_16x16, MODE_16x16 ) );
          }
        }

        if( iRefIdxTest == iBLRefIdx[uiDir] )
        {
          uiBitsTest      = ( uiBasePredType == 2 ? 0 : uiMbBits[2] ) + uiMotBits[1-uiDir];
          RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                              cBLMvLastEst[uiDir],
                                                              cBLMvPred   [uiDir],
                                                              uiBitsTest, uiCostTest,
                                                              PART_16x16, MODE_16x16, 
                                                              uiIterSearchRange, 0, &cBSParams ) );
          if( uiCostTest < uiCost[2] )
          {
            bChanged          = true;
            bBLPredBi [uiDir] = true;
            iRefIdxBi [uiDir] = iRefIdxTest;
            cMvBi     [uiDir] = cBLMvLastEst[uiDir];
            uiMotBits [uiDir] = uiBitsTest - uiMbBits[2] - uiMotBits[1-uiDir];
            uiBits    [2]     = uiBitsTest;
            uiCost    [2]     = uiCostTest;

            RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[uiDir],
                                                              PART_16x16, MODE_16x16 ) );
          }
        }
      }

      if( ! bChanged )
      {
        break;
      }
    }
  }


  //===== chose parameters =====
  if( uiCost[2] <= uiCost[0] && uiCost[2] <= uiCost[1] )
  {
    //----- bi-directional prediction -----
    fRdCost     = uiCost    [2];
    iRefIdx [0] = iRefIdxBi [0];
    iRefIdx [1] = iRefIdxBi [1];
    bBLPred [0] = bBLPredBi [0];
    bBLPred [1] = bBLPredBi [1];
    if( bBLPred[0] )  cMvPred[0][iRefIdx[0]] = cBLMvPred[0];
    if( bBLPred[1] )  cMvPred[1][iRefIdx[1]] = cBLMvPred[1];
    cMv     [0] = cMvBi     [0];
    cMv     [1] = cMvBi     [1];
    cMvd    [0] = cMv       [0] - cMvPred[0][iRefIdx[0]];
    cMvd    [1] = cMv       [1] - cMvPred[1][iRefIdx[1]];
  }
  else if( uiCost[0] <= uiCost[1] )
  {
    //----- list 0 prediction -----
    fRdCost     = uiCost[0];
    iRefIdx [1] = BLOCK_NOT_PREDICTED;
    bBLPred [1] = false;
    if( bBLPred[0] )  cMvPred[0][iRefIdx[0]] = cBLMvPred[0];
    cMv     [1] = Mv::ZeroMv();
    cMvd    [0] = cMv[0] - cMvPred[0][iRefIdx[0]];
  }
  else
  {
    //----- list 1 prediction -----
    fRdCost     = uiCost[1];
    iRefIdx [0] = BLOCK_NOT_PREDICTED;
    bBLPred [0] = false;
    if( bBLPred[1] )  cMvPred[1][iRefIdx[1]] = cBLMvPred[1];
    cMv     [0] = Mv::ZeroMv();
    cMvd    [1] = cMv[1] - cMvPred[1][iRefIdx[1]];
  }


  //===== set parameters and compare =====
  rpcMbTempData->rdCost() = fRdCost;
  rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx    ( iRefIdx [0] );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv     ( cMv     [0] );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv     ( cMvd    [0] );

  rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx    ( iRefIdx [1] );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv     ( cMv     [1] );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv     ( cMvd    [1] );

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( bBLPred [0] );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( bBLPred [1] );
  ROT( bBLPred[0] && iRefIdx[0] != iBLRefIdx[0] );
  ROT( bBLPred[1] && iRefIdx[1] != iBLRefIdx[1] );

  IntMbTempData* pcMbRefData = rpcMbTempData;

  RNOK( xSetRdCostInterMb( *rpcMbTempData, pcMbDataAccessBase, rcRefListStruct, uiMinQP, uiMaxQP, bLowComplexMbEnable ) );


  //JVT-R057 LA-RDO}
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2=0,distortion=0;
	  for( Int n = 0; n <1; n++)
	  {
		  iRefIdx [0]=rpcMbTempData->getMbMotionData(LIST_0).getRefIdx(B4x4Idx(n));
		  iRefIdx [1]=rpcMbTempData->getMbMotionData(LIST_1).getRefIdx(B4x4Idx(n));
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,0,0,4,4);
		  }
		  if(pcRefFrame1)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,0,0,4,4);
			  if(pcRefFrame0)
				  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
			  else
				  distortion1=distortion2;
		  }
		  distortion+=distortion1;
	  }
	  setEpRef(distortion);
	  rpcMbTempData->rdCost()+=distortion;
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  RNOK( xCheckInterMbMode8x8(  rpcMbTempData, rpcMbBestData, pcMbRefData, rcRefListStruct, uiMinQP, uiMaxQP, false, pcMbDataAccessBase ) );

  return Err::m_nOK;
}




ErrVal
MbEncoder::xEstimateMb16x8 ( IntMbTempData*&  rpcMbTempData,
                             IntMbTempData*&  rpcMbBestData,
                             RefListStruct&   rcRefListStruct,
                             UInt             uiMinQP,
                             UInt             uiMaxQP,
                             UInt             uiMaxNumMv,
                             UInt             uiNumMaxIter,
                             UInt             uiIterSearchRange,
                             MbDataAccess*    pcMbDataAccessBase,
                             Bool             bResidualPred )
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListME[ 1 ];
  ROF  ( uiMaxNumMv );
  ROFRS( uiMaxNumMv >= 2,                           Err::m_nOK );
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag(), Err::m_nOK );
  ROF  ( rcRefFrameList0.getActive() <= 32 );
  ROF  ( rcRefFrameList1.getActive() <= 32 );
  ROF  ( rcRefFrameList0.getActive() );

  const UInt aauiMbBits[2][3][3] = { { {0,0,3}, {0,0,0}, {0,0,0} } , { {5,7,7}, {7,5,7}, {9-3,9-3,9-3} } };

  rpcMbTempData->clear();
  rpcMbTempData->setMbMode( MODE_16x8 );
  rpcMbTempData->setBLSkipFlag( false );
  rpcMbTempData->setResidualPredFlag( bResidualPred );

  Bool   bPSlice       = rpcMbTempData->getMbDataAccess().getSH().isPSlice();
  Double fRdCost       = 0;
  UInt   uiLastMode    = 0;

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
  {
    ParIdx16x8      eParIdx     = ( uiBlk ? PART_16x8_1 : PART_16x8_0 );
    UInt            uiCost  [3] = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX }, uiCostTest;
    Int             iRefIdx [2] = { 0, 0 }, iRefIdxBi[2], iRefIdxTest;
    UInt            uiMbBits[3] = { 3, 0, 0 };
    UInt            uiBits  [3] = { 0, 0, 0 }, uiBitsTest;
    Mv              cMv[2], cMvBi[2], cMvPred[2][33], cMvLastEst[2][33], cMvd[2];
    YuvMbBuffer     cYuvMbBuffer[2];
    Frame*          pcRefFrame    = 0;
    Bool            bBLPred   [2] = { false, false };
    Bool            bBLPredBi [2] = { false, false };
    Int             iBLRefIdx [2] = { -1, -1 };
    Mv              cBLMvPred [2], cBLMvLastEst[2];

    if( pcMbDataAccessBase && m_bBaseMotionPredAllowed )
    {
      if( pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx ) >  0                           &&
          pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx ) <= (Int)rcRefFrameList0.getActive()   )
      {
        iBLRefIdx [0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx ( eParIdx );
        cBLMvPred [0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx );
      }
      if( pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx ) >  0                           &&
          pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx ) <= (Int)rcRefFrameList1.getActive()   )
      {
        iBLRefIdx [1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx ( eParIdx );
        cBLMvPred [1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx );
      }
    }

    UInt  uiBasePredType = MSYS_UINT_MAX;

    //----- set macroblock bits -----
    if( ! bPSlice )
    {
      memcpy( uiMbBits, aauiMbBits[uiBlk][uiLastMode], 3*sizeof(UInt) );
    }

    //===== LIST 0 PREDICTION ======
    for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList0.getActive(); iRefIdxTest++ )
    {
      pcRefFrame = rcRefFrameList0[iRefIdxTest];

      if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
      {
        rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[0][iRefIdxTest], iRefIdxTest,
                                                            LIST_0, eParIdx );
        uiBitsTest                  = ( uiBasePredType == 0 ? 0 : uiMbBits[0] ) + getRefIdxBits( iRefIdxTest, rcRefFrameList0 );
        cMvLastEst[0][iRefIdxTest]  = cMvPred [0][iRefIdxTest];
        if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[0] )
        {
          cMvLastEst[0][iRefIdxTest] = cBLMvPred[0];
          m_pcMotionEstimation->setEL( true );
        }
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cMvLastEst[0][iRefIdxTest],
                                                            cMvPred   [0][iRefIdxTest],
                                                            uiBitsTest, uiCostTest,
                                                            eParIdx, MODE_16x8, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        if( uiCostTest < uiCost[0] )
        {
          bBLPred [0] = false;
          iRefIdx [0] = iRefIdxTest;
          cMv     [0] = cMvLastEst[0][iRefIdxTest];
          uiBits  [0] = uiBitsTest;
          uiCost  [0] = uiCostTest;
        }
      }

      if( iRefIdxTest == iBLRefIdx[0] )
      {
        uiBitsTest      = ( uiBasePredType == 0 ? 0 : uiMbBits[0] );
        cBLMvLastEst[0] = cBLMvPred [0];
        m_pcMotionEstimation->setEL( true );

        rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[0], LIST_0, eParIdx );
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cBLMvLastEst[0],
                                                            cBLMvPred   [0],
                                                            uiBitsTest, uiCostTest,
                                                            eParIdx, MODE_16x8, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        if( uiCostTest < uiCost[0] )
        {
          bBLPred [0] = true;
          iRefIdx [0] = iRefIdxTest;
          cMv     [0] = cBLMvLastEst[0];
          uiBits  [0] = uiBitsTest;
          uiCost  [0] = uiCostTest;
        }
      }
    }


    //===== LIST 1 PREDICTION =====
    for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList1.getActive(); iRefIdxTest++ )
    {
      pcRefFrame = rcRefFrameList1[iRefIdxTest];

      if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
      {
        rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[1][iRefIdxTest], iRefIdxTest,
                                                            LIST_1, eParIdx );
        uiBitsTest                  = ( uiBasePredType == 1 ? 0 : uiMbBits[1] ) + getRefIdxBits( iRefIdxTest, rcRefFrameList1 );
        cMvLastEst[1][iRefIdxTest]  = cMvPred [1][iRefIdxTest];
        if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[1] )
        {
          cMvLastEst[1][iRefIdxTest] = cBLMvPred[1];
          m_pcMotionEstimation->setEL( true );
        }
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cMvLastEst[1][iRefIdxTest],
                                                            cMvPred   [1][iRefIdxTest],
                                                            uiBitsTest, uiCostTest,
                                                            eParIdx, MODE_16x8, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        if( uiCostTest < uiCost[1] )
        {
          bBLPred [1] = false;
          iRefIdx [1] = iRefIdxTest;
          cMv     [1] = cMvLastEst[1][iRefIdxTest];
          uiBits  [1] = uiBitsTest;
          uiCost  [1] = uiCostTest;

          RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[1],
                                                            eParIdx, MODE_16x8 ) );
        }
      }

      if( iRefIdxTest == iBLRefIdx[1] )
      {
        uiBitsTest      = ( uiBasePredType == 1 ? 0 : uiMbBits[1] );
        cBLMvLastEst[1] = cBLMvPred [1];
        m_pcMotionEstimation->setEL( true );

        rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[1], LIST_1, eParIdx );
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cBLMvLastEst[1],
                                                            cBLMvPred   [1],
                                                            uiBitsTest, uiCostTest,
                                                            eParIdx, MODE_16x8, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        if( uiCostTest < uiCost[1] )
        {
          bBLPred [1] = true;
          iRefIdx [1] = iRefIdxTest;
          cMv     [1] = cBLMvLastEst[1];
          uiBits  [1] = uiBitsTest;
          uiCost  [1] = uiCostTest;

          RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[1],
                                                            eParIdx, MODE_16x8 ) );
        }
      }
    }

    ROTRS( uiCost[0] == MSYS_UINT_MAX && uiCost[1] == MSYS_UINT_MAX, Err::m_nOK );
    Bool bBiPredOk = ( uiCost[0] != MSYS_UINT_MAX && uiCost[1] != MSYS_UINT_MAX );

    //===== BI PREDICTION =====
    Bool bBiPredPossible = bBiPredOk && ( uiBlk ? uiMaxNumMv >= 2 : uiMaxNumMv >= 3 );
    if( rcRefFrameList0.getActive() && rcRefFrameList1.getActive() && bBiPredPossible )
    {
      //----- initialize with forward and backward estimation -----
      cMvBi           [0] = cMv[0];
      cMvBi           [1] = cMv[1];
      iRefIdxBi       [0] = iRefIdx [0];
      iRefIdxBi       [1] = iRefIdx [1];
      bBLPredBi       [0] = bBLPred [0];
      bBLPredBi       [1] = bBLPred [1];
      UInt  uiMotBits[2]  = { uiBits[0] - uiMbBits[0], uiBits[1] - uiMbBits[1] };
      uiBits[2]           = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];

      if( ! uiNumMaxIter )
      {
        uiNumMaxIter      = 1;
        uiIterSearchRange = 0;
      }

      //----- iterative search -----
      for( UInt uiIter = 0; uiIter < uiNumMaxIter; uiIter++ )
      {
        Bool  bChanged                = false;
        UInt  uiDir                   = uiIter % 2;
        RefFrameList& rcRefFrameList  = ( uiDir ? rcRefFrameList1 : rcRefFrameList0 );
        BSParams      cBSParams;
        cBSParams.pcAltRefFrame       = ( uiDir ? rcRefFrameList0 : rcRefFrameList1 )[ iRefIdxBi[ 1 - uiDir ] ];
        cBSParams.pcAltRefPelData     = &cYuvMbBuffer[1-uiDir];
        cBSParams.uiL1Search          = uiDir;
        cBSParams.apcWeight[LIST_0]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxBi[LIST_0], rpcMbTempData->getFieldFlag() );
        cBSParams.apcWeight[LIST_1]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxBi[LIST_1], rpcMbTempData->getFieldFlag() );

        for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList.getActive(); iRefIdxTest++ )
        {
          if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
          {
            uiBitsTest        = ( uiBasePredType == 2 ? 0 : uiMbBits[2] ) + uiMotBits[1-uiDir] + getRefIdxBits( iRefIdxTest, rcRefFrameList );
            pcRefFrame        = rcRefFrameList[iRefIdxTest];

            RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                                cMvLastEst[uiDir][iRefIdxTest],
                                                                cMvPred   [uiDir][iRefIdxTest],
                                                                uiBitsTest, uiCostTest,
                                                                eParIdx, MODE_16x8, 
                                                                uiIterSearchRange, 0, &cBSParams ) );
            if( uiCostTest < uiCost[2] )
            {
              bChanged          = true;
              bBLPredBi [uiDir] = false;
              iRefIdxBi [uiDir] = iRefIdxTest;
              cMvBi     [uiDir] = cMvLastEst[uiDir][iRefIdxTest];
              uiMotBits [uiDir] = uiBitsTest - uiMbBits[2] - uiMotBits[1-uiDir];
              uiBits    [2]     = uiBitsTest;
              uiCost    [2]     = uiCostTest;

              RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[uiDir],
                                                                eParIdx, MODE_16x8 ) );
            }
          }

          if( iRefIdxTest == iBLRefIdx[uiDir] )
          {
            uiBitsTest  = ( uiBasePredType == 2 ? 0 : uiMbBits[2] ) + uiMotBits[1-uiDir];
            RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                                cBLMvLastEst[uiDir],
                                                                cBLMvPred   [uiDir],
                                                                uiBitsTest, uiCostTest,
                                                                eParIdx, MODE_16x8, 
                                                                uiIterSearchRange, 0, &cBSParams ) );
            if( uiCostTest < uiCost[2] )
            {
              bChanged          = true;
              bBLPredBi [uiDir] = true;
              iRefIdxBi [uiDir] = iRefIdxTest;
              cMvBi     [uiDir] = cBLMvLastEst[uiDir];
              uiMotBits [uiDir] = uiBitsTest - uiMbBits[2] - uiMotBits[1-uiDir];
              uiBits    [2]     = uiBitsTest;
              uiCost    [2]     = uiCostTest;

              RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[uiDir],
                                                                eParIdx, MODE_16x8 ) );
            }
          }
        }

        if( ! bChanged )
        {
          break;
        }
      }
    }


    //===== chose parameters =====
    if( uiCost[2] <= uiCost[0] && uiCost[2] <= uiCost[1] )
    {
      //----- bi-directional prediction -----
      uiLastMode  = 2;
      fRdCost    += uiCost    [2];
      iRefIdx [0] = iRefIdxBi [0];
      iRefIdx [1] = iRefIdxBi [1];
      bBLPred [0] = bBLPredBi [0];
      bBLPred [1] = bBLPredBi [1];
      if( bBLPred[0] )  cMvPred[0][iRefIdx[0]] = cBLMvPred[0];
      if( bBLPred[1] )  cMvPred[1][iRefIdx[1]] = cBLMvPred[1];
      cMv     [0] = cMvBi     [0];
      cMv     [1] = cMvBi     [1];
      cMvd    [0] = cMv       [0] - cMvPred[0][iRefIdx[0]];
      cMvd    [1] = cMv       [1] - cMvPred[1][iRefIdx[1]];
      uiMaxNumMv-= 2;
    }
    else if( uiCost[0] <= uiCost[1] )
    {
      //----- list 0 prediction -----
      uiLastMode  = 0;
      fRdCost    += uiCost[0];
      iRefIdx [1] = BLOCK_NOT_PREDICTED;
      bBLPred [1] = false;
      if( bBLPred[0] )  cMvPred[0][iRefIdx[0]] = cBLMvPred[0];
      cMv     [1] = Mv::ZeroMv();
      cMvd    [0] = cMv[0] - cMvPred[0][iRefIdx[0]];
      uiMaxNumMv--;
    }
    else
    {
      //----- list 1 prediction -----
      uiLastMode  = 1;
      fRdCost    += uiCost[1];
      iRefIdx [0] = BLOCK_NOT_PREDICTED;
      bBLPred [0] = false;
      if( bBLPred[1] )  cMvPred[1][iRefIdx[1]] = cBLMvPred[1];
      cMv     [0] = Mv::ZeroMv();
      cMvd    [1] = cMv[1] - cMvPred[1][iRefIdx[1]];
      uiMaxNumMv--;
    }


    //===== set parameters and compare =====
    rpcMbTempData->rdCost() = fRdCost;
    rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx( iRefIdx [0],  eParIdx );
    rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0],  eParIdx );
    rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0],  eParIdx );
    rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx( iRefIdx [1],  eParIdx );
    rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1],  eParIdx );
    rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1],  eParIdx );

    rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( bBLPred[0], eParIdx );
    rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( bBLPred[1], eParIdx );
    ROT( bBLPred[0] && iRefIdx[0] != iBLRefIdx[0] );
    ROT( bBLPred[1] && iRefIdx[1] != iBLRefIdx[1] );
  }

  IntMbTempData* pcMbRefData = rpcMbTempData;

  RNOK( xSetRdCostInterMb( *rpcMbTempData, pcMbDataAccessBase, rcRefListStruct, uiMinQP, uiMaxQP, bLowComplexMbEnable ) );


  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2=0,distortion=0;
	  for( Int n = 0; n <2; n++)
	  {
		  Int iRefIdx[2];
		  Int Tab[2]={0,8};
		  iRefIdx [0]=rpcMbTempData->getMbMotionData(LIST_0).getRefIdx(B4x4Idx(Tab[n]));
		  iRefIdx [1]=rpcMbTempData->getMbMotionData(LIST_1).getRefIdx(B4x4Idx(Tab[n]));
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_0).getMv(B4x4Idx(Tab[n])).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_0).getMv(B4x4Idx(Tab[n])).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,Tab[n]%4,Tab[n]/4,4,2);
		  }
		  if(pcRefFrame1)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_1).getMv(B4x4Idx(Tab[n])).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_1).getMv(B4x4Idx(Tab[n])).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,Tab[n]%4,Tab[n]/4,4,2);
			  if(pcRefFrame0)
				  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
			  else
				  distortion1=distortion2;
		  }
		  distortion+=distortion1;
	  }
	  setEpRef(distortion);
	  rpcMbTempData->rdCost()+=distortion;
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  RNOK( xCheckInterMbMode8x8(  rpcMbTempData, rpcMbBestData, pcMbRefData, rcRefListStruct, uiMinQP, uiMaxQP, false, pcMbDataAccessBase ) );

  return Err::m_nOK;
}





ErrVal
MbEncoder::xEstimateMb8x16 ( IntMbTempData*&  rpcMbTempData,
                             IntMbTempData*&  rpcMbBestData,
                             RefListStruct&   rcRefListStruct,
                             UInt             uiMinQP,
                             UInt             uiMaxQP,
                             UInt             uiMaxNumMv,
                             UInt             uiNumMaxIter,
                             UInt             uiIterSearchRange,
                             MbDataAccess*    pcMbDataAccessBase,
                             Bool             bResidualPred )
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListME[ 1 ];
  ROF  ( uiMaxNumMv );
  ROFRS( uiMaxNumMv >= 2,                           Err::m_nOK );
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag(), Err::m_nOK );
  ROF  ( rcRefFrameList0.getActive() <= 32 );
  ROF  ( rcRefFrameList1.getActive() <= 32 );
  ROF  ( rcRefFrameList0.getActive() );

  const UInt aauiMbBits[2][3][3] = { { {0,2,3}, {0,0,0}, {0,0,0} } , { {5,7,7}, {7-2,7-2,9-2}, {9-3,9-3,9-3} } };

  rpcMbTempData->clear();
  rpcMbTempData->setMbMode( MODE_8x16 );
  rpcMbTempData->setBLSkipFlag( false );
  rpcMbTempData->setResidualPredFlag( bResidualPred );

  Bool   bPSlice       = rpcMbTempData->getMbDataAccess().getSH().isPSlice();
  Double fRdCost       = 0;
  UInt   uiLastMode    = 0;

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  for( UInt   uiBlk = 0; uiBlk < 2; uiBlk++ )
  {
    ParIdx8x16      eParIdx     = ( uiBlk ? PART_8x16_1 : PART_8x16_0 );
    UInt            uiCost  [3] = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX }, uiCostTest;
    Int             iRefIdx [2] = { 0, 0 }, iRefIdxBi[2], iRefIdxTest;
    UInt            uiMbBits[3] = { 3, 0, 0 };
    UInt            uiBits  [3] = { 0, 0, 0 }, uiBitsTest;
    Mv              cMv[2], cMvBi[2], cMvPred[2][33], cMvLastEst[2][33], cMvd[2];
    YuvMbBuffer     cYuvMbBuffer[2];
    Frame*          pcRefFrame    = 0;
    Bool            bBLPred   [2] = { false, false };
    Bool            bBLPredBi [2] = { false, false };
    Int             iBLRefIdx [2] = { -1, -1 };
    Mv              cBLMvPred [2], cBLMvLastEst[2];

    if( pcMbDataAccessBase && m_bBaseMotionPredAllowed )
    {
      if( pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx ) >  0                           &&
          pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx ) <= (Int)rcRefFrameList0.getActive()   )
      {
        iBLRefIdx [0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx ( eParIdx );
        cBLMvPred [0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx );
      }
      if( pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx ) >  0                           &&
          pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx ) <= (Int)rcRefFrameList1.getActive()   )
      {
        iBLRefIdx [1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx ( eParIdx );
        cBLMvPred [1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx );
      }
    }

    UInt  uiBasePredType = MSYS_UINT_MAX;

    //----- set macroblock bits -----
    if( ! bPSlice )
    {
      memcpy( uiMbBits, aauiMbBits[uiBlk][uiLastMode], 3*sizeof(UInt) );
    }

    //===== LIST 0 PREDICTION ======
    for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList0.getActive(); iRefIdxTest++ )
    {
      pcRefFrame = rcRefFrameList0[iRefIdxTest];

      if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
      {
        rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[0][iRefIdxTest], iRefIdxTest,
                                                            LIST_0, eParIdx );
        uiBitsTest                  = ( uiBasePredType == 0 ? 0 : uiMbBits[0] ) + getRefIdxBits( iRefIdxTest, rcRefFrameList0 );
        cMvLastEst[0][iRefIdxTest]  = cMvPred [0][iRefIdxTest];
        if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[0] )
        {
          cMvLastEst[0][iRefIdxTest] = cBLMvPred[0];
          m_pcMotionEstimation->setEL( true );
        }
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cMvLastEst[0][iRefIdxTest],
                                                            cMvPred   [0][iRefIdxTest],
                                                            uiBitsTest, uiCostTest,
                                                            eParIdx, MODE_8x16, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        if( uiCostTest < uiCost[0] )
        {
          bBLPred [0] = false;
          iRefIdx [0] = iRefIdxTest;
          cMv     [0] = cMvLastEst[0][iRefIdxTest];
          uiBits  [0] = uiBitsTest;
          uiCost  [0] = uiCostTest;
        }
      }

      if( iRefIdxTest == iBLRefIdx[0] )
      {
        uiBitsTest      = ( uiBasePredType == 0 ? 0 : uiMbBits[0] );
        cBLMvLastEst[0] = cBLMvPred [0];
        m_pcMotionEstimation->setEL( true );

        rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[0], LIST_0, eParIdx );
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cBLMvLastEst[0],
                                                            cBLMvPred   [0],
                                                            uiBitsTest, uiCostTest,
                                                            eParIdx, MODE_8x16, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        if( uiCostTest < uiCost[0] )
        {
          bBLPred [0] = true;
          iRefIdx [0] = iRefIdxTest;
          cMv     [0] = cBLMvLastEst[0];
          uiBits  [0] = uiBitsTest;
          uiCost  [0] = uiCostTest;
        }
      }
    }


    //===== LIST 1 PREDICTION =====
    for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList1.getActive(); iRefIdxTest++ )
    {
      pcRefFrame = rcRefFrameList1[iRefIdxTest];

      if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
      {
        rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[1][iRefIdxTest], iRefIdxTest,
                                                            LIST_1, eParIdx );
        uiBitsTest                  = ( uiBasePredType == 1 ? 0 : uiMbBits[1] ) + getRefIdxBits( iRefIdxTest, rcRefFrameList1 );
        cMvLastEst[1][iRefIdxTest]  = cMvPred [1][iRefIdxTest];
        if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[1] )
        {
          cMvLastEst[1][iRefIdxTest] = cBLMvPred[1];
          m_pcMotionEstimation->setEL( true );
        }
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cMvLastEst[1][iRefIdxTest],
                                                            cMvPred   [1][iRefIdxTest],
                                                            uiBitsTest, uiCostTest,
                                                            eParIdx, MODE_8x16, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        if( uiCostTest < uiCost[1] )
        {
          bBLPred [1] = false;
          iRefIdx [1] = iRefIdxTest;
          cMv     [1] = cMvLastEst[1][iRefIdxTest];
          uiBits  [1] = uiBitsTest;
          uiCost  [1] = uiCostTest;

          RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[1],
                                                            eParIdx, MODE_8x16 ) );
        }
      }

      if( iRefIdxTest == iBLRefIdx[1] )
      {
        uiBitsTest      = ( uiBasePredType == 1 ? 0 : uiMbBits[1] );
        cBLMvLastEst[1] = cBLMvPred [1];
        m_pcMotionEstimation->setEL( true );

        rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[1], LIST_1, eParIdx );
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cBLMvLastEst[1],
                                                            cBLMvPred   [1],
                                                            uiBitsTest, uiCostTest,
                                                            eParIdx, MODE_8x16, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        if( uiCostTest < uiCost[1] )
        {
          bBLPred [1] = true;
          iRefIdx [1] = iRefIdxTest;
          cMv     [1] = cBLMvLastEst[1];
          uiBits  [1] = uiBitsTest;
          uiCost  [1] = uiCostTest;

          RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[1],
                                                            eParIdx, MODE_8x16 ) );
        }
      }
    }

    ROTRS( uiCost[0] == MSYS_UINT_MAX && uiCost[1] == MSYS_UINT_MAX, Err::m_nOK );
    Bool bBiPredOk = ( uiCost[0] != MSYS_UINT_MAX && uiCost[1] != MSYS_UINT_MAX );

    //===== BI PREDICTION =====
    Bool bBiPredPossible = bBiPredOk && ( uiBlk ? uiMaxNumMv >= 2 : uiMaxNumMv >= 3 );
    if( rcRefFrameList0.getActive() && rcRefFrameList1.getActive() && bBiPredPossible )
    {
      //----- initialize with forward and backward estimation -----
      iRefIdxBi       [0] = iRefIdx [0];
      iRefIdxBi       [1] = iRefIdx [1];
      bBLPredBi       [0] = bBLPred [0];
      bBLPredBi       [1] = bBLPred [1];
      cMvBi           [0] = cMv     [0];
      cMvBi           [1] = cMv     [1];
      UInt  uiMotBits[2]  = { uiBits[0] - uiMbBits[0], uiBits[1] - uiMbBits[1] };
      uiBits[2]           = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];

      if( ! uiNumMaxIter )
      {
        uiNumMaxIter      = 1;
        uiIterSearchRange = 0;
      }

      //----- iterative search -----
      for( UInt uiIter = 0; uiIter < uiNumMaxIter; uiIter++ )
      {
        Bool          bChanged        = false;
        UInt          uiDir           = uiIter % 2;
        RefFrameList& rcRefFrameList  = ( uiDir ? rcRefFrameList1 : rcRefFrameList0 );
        BSParams      cBSParams;
        cBSParams.pcAltRefFrame       = ( uiDir ? rcRefFrameList0 : rcRefFrameList1 )[ iRefIdxBi[ 1 - uiDir ] ];
        cBSParams.pcAltRefPelData     = &cYuvMbBuffer[1-uiDir];
        cBSParams.uiL1Search          = uiDir;
        cBSParams.apcWeight[LIST_0]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxBi[LIST_0], rpcMbTempData->getFieldFlag() );
        cBSParams.apcWeight[LIST_1]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxBi[LIST_1], rpcMbTempData->getFieldFlag() );

        for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList.getActive(); iRefIdxTest++ )
        {
          if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
          {
            uiBitsTest  = ( uiBasePredType == 2 ? 0 : uiMbBits[2] ) + uiMotBits[1-uiDir] + getRefIdxBits( iRefIdxTest, rcRefFrameList );
            pcRefFrame  = rcRefFrameList[iRefIdxTest];

            RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                                cMvLastEst[uiDir][iRefIdxTest],
                                                                cMvPred   [uiDir][iRefIdxTest],
                                                                uiBitsTest, uiCostTest,
                                                                eParIdx, MODE_8x16, 
                                                                uiIterSearchRange, 0, &cBSParams ) );
            if( uiCostTest < uiCost[2] )
            {
              bChanged          = true;
              bBLPredBi [uiDir] = false;
              iRefIdxBi [uiDir] = iRefIdxTest;
              cMvBi     [uiDir] = cMvLastEst[uiDir][iRefIdxTest];
              uiMotBits [uiDir] = uiBitsTest - uiMbBits[2] - uiMotBits[1-uiDir];
              uiBits    [2]     = uiBitsTest;
              uiCost    [2]     = uiCostTest;

              RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[uiDir],
                                                                eParIdx, MODE_8x16 ) );
            }
          }

          if( iRefIdxTest == iBLRefIdx[uiDir] )
          {
            uiBitsTest  = ( uiBasePredType == 2 ? 0 : uiMbBits[2] ) + uiMotBits[1-uiDir];
            RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                                cBLMvLastEst[uiDir],
                                                                cBLMvPred   [uiDir],
                                                                uiBitsTest, uiCostTest,
                                                                eParIdx, MODE_8x16, 
                                                                uiIterSearchRange, 0, &cBSParams ) );
            if( uiCostTest < uiCost[2] )
            {
              bChanged          = true;
              bBLPredBi [uiDir] = true;
              iRefIdxBi [uiDir] = iRefIdxTest;
              cMvBi     [uiDir] = cBLMvLastEst[uiDir];
              uiMotBits [uiDir] = uiBitsTest - uiMbBits[2] - uiMotBits[1-uiDir];
              uiBits    [2]     = uiBitsTest;
              uiCost    [2]     = uiCostTest;

              RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[uiDir],
                                                                eParIdx, MODE_8x16 ) );
            }
          }
        }

        if( ! bChanged )
        {
          break;
        }
      }
    }


    //===== chose parameters =====
    if( uiCost[2] <= uiCost[0] && uiCost[2] <= uiCost[1] )
    {
      //----- bi-directional prediction -----
      uiLastMode  = 2;
      fRdCost    += uiCost    [2];
      iRefIdx [0] = iRefIdxBi [0];
      iRefIdx [1] = iRefIdxBi [1];
      bBLPred [0] = bBLPredBi [0];
      bBLPred [1] = bBLPredBi [1];
      if( bBLPred[0] )  cMvPred[0][iRefIdx[0]] = cBLMvPred[0];
      if( bBLPred[1] )  cMvPred[1][iRefIdx[1]] = cBLMvPred[1];
      cMv     [0] = cMvBi     [0];
      cMv     [1] = cMvBi     [1];
      cMvd    [0] = cMv       [0] - cMvPred[0][iRefIdx[0]];
      cMvd    [1] = cMv       [1] - cMvPred[1][iRefIdx[1]];
      uiMaxNumMv-= 2;
    }
    else if( uiCost[0] <= uiCost[1] )
    {
      //----- list 0 prediction -----
      uiLastMode  = 0;
      fRdCost    += uiCost[0];
      iRefIdx [1] = BLOCK_NOT_PREDICTED;
      bBLPred [1] = false;
      if( bBLPred[0] )  cMvPred[0][iRefIdx[0]] = cBLMvPred[0];
      cMv     [1] = Mv::ZeroMv();
      cMvd    [0] = cMv[0] - cMvPred[0][iRefIdx[0]];
      uiMaxNumMv--;
    }
    else
    {
      //----- list 1 prediction -----
      uiLastMode  = 1;
      fRdCost    += uiCost[1];
      iRefIdx [0] = BLOCK_NOT_PREDICTED;
      bBLPred [0] = false;
      if( bBLPred[1] )  cMvPred[1][iRefIdx[1]] = cBLMvPred[1];
      cMv     [0] = Mv::ZeroMv();
      cMvd    [1] = cMv[1] - cMvPred[1][iRefIdx[1]];
      uiMaxNumMv--;
    }


    //===== set parameters and compare =====
    rpcMbTempData->rdCost() = fRdCost;
    rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx( iRefIdx [0],  eParIdx );
    rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0],  eParIdx );
    rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0],  eParIdx );
    rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx( iRefIdx [1],  eParIdx );
    rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1],  eParIdx );
    rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1],  eParIdx );

    rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( bBLPred[0], eParIdx );
    rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( bBLPred[1], eParIdx );
    ROT( bBLPred[0] && iRefIdx[0] != iBLRefIdx[0] );
    ROT( bBLPred[1] && iRefIdx[1] != iBLRefIdx[1] );
  }

  IntMbTempData* pcMbRefData = rpcMbTempData;

  RNOK( xSetRdCostInterMb( *rpcMbTempData, pcMbDataAccessBase, rcRefListStruct, uiMinQP, uiMaxQP, bLowComplexMbEnable ) );


  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2,distortion=0;
	  for( Int n = 0; n <2; n++)
	  {
		  Int iRefIdx[2];
		  Int Tab[2]={0,2};
		  iRefIdx [0]=rpcMbTempData->getMbMotionData(LIST_0).getRefIdx(B4x4Idx(Tab[n]));
		  iRefIdx [1]=rpcMbTempData->getMbMotionData(LIST_1).getRefIdx(B4x4Idx(Tab[n]));
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_0).getMv(B4x4Idx(Tab[n])).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_0).getMv(B4x4Idx(Tab[n])).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,Tab[n]%4,Tab[n]/4,2,4);
		  }
		  if(pcRefFrame1)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_1).getMv(B4x4Idx(Tab[n])).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_1).getMv(B4x4Idx(Tab[n])).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,Tab[n]%4,Tab[n]/4,2,4);
			  if(pcRefFrame0)
				  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
			  else
				  distortion1=distortion2;
		  }
		  distortion+=distortion1;
	  }
	  setEpRef(distortion);
	  rpcMbTempData->rdCost()+=distortion;
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  RNOK( xCheckInterMbMode8x8(  rpcMbTempData, rpcMbBestData, pcMbRefData, rcRefListStruct, uiMinQP, uiMaxQP, false, pcMbDataAccessBase ) );

  return Err::m_nOK;
}




ErrVal
MbEncoder::xEstimateMb8x8 ( IntMbTempData*&   rpcMbTempData,
                            IntMbTempData*&   rpcMbBestData,
                            RefListStruct&    rcRefListStruct,
                            UInt              uiMinQP,
                            UInt              uiMaxQP,
                            UInt              uiMaxNumMv,
                            UInt              uiNumMaxIter,
                            UInt              uiIterSearchRange,
                            MbDataAccess*     pcMbDataAccessBase,
                            Bool              bResidualPred,
                            Bool              bMCBlks8x8Disable,
                            Bool              bBiPred8x8Disable )
{
  ROF  ( uiMaxNumMv );
  ROFRS( uiMaxNumMv >= 4,                           Err::m_nOK );
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag(), Err::m_nOK );

  Bool  bPSlice  = rpcMbTempData->getMbDataAccess().getSH().isPSlice();
  UInt  uiBits   = ( ! bPSlice ? 9 : 5 ); // for signalling macroblock mode

  rpcMbTempData->clear    ();
  rpcMbTempData->setMbMode( MODE_8x8 );
  rpcMbTempData->setBLSkipFlag( false );
  rpcMbTempData->setResidualPredFlag( bResidualPred );
  rpcMbTempData->rdCost   () = 0;

  m_pcTransform->setQp( rpcMbTempData->getMbDataAccess(), false );

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  for( Par8x8 ePar8x8 = B_8x8_0; ePar8x8 < 4; ePar8x8 = Par8x8( ePar8x8 + 1 ), uiBits = 0 )
  {
    UInt      uiRemBlocks     = 3 - (UInt)ePar8x8;
    UInt      uiMaxBlkMvMax   = uiMaxNumMv -   uiRemBlocks;         // don't care about following blocks
    UInt      uiMaxBlkMvMin   = uiMaxNumMv / ( uiRemBlocks + 1 );   // don't use more than available for other blocks
    UInt      uiMaxBlkMvs     = ( uiMaxBlkMvMax + uiMaxBlkMvMin + 1 ) >> 1;
    ParIdx8x8 aeParIdx8x8[4]  = { PART_8x8_0, PART_8x8_1, PART_8x8_2, PART_8x8_3 };
    ParIdx8x8 eParIdx8x8      = aeParIdx8x8[ ePar8x8 ];

    m_pcIntMbBest8x8Data->clear ();
    RNOK  ( xEstimateSubMbDirect( ePar8x8, m_pcIntMbTemp8x8Data, m_pcIntMbBest8x8Data, rcRefListStruct, uiMaxBlkMvs, false,                                              uiBits ) );
    RNOK  ( xEstimateSubMb8x8   ( ePar8x8, m_pcIntMbTemp8x8Data, m_pcIntMbBest8x8Data, rcRefListStruct, uiMaxBlkMvs, false,             uiNumMaxIter, uiIterSearchRange, uiBits, pcMbDataAccessBase ) );
    if( !bMCBlks8x8Disable )
    {
      RNOK( xEstimateSubMb8x4   ( ePar8x8, m_pcIntMbTemp8x8Data, m_pcIntMbBest8x8Data, rcRefListStruct, uiMaxBlkMvs, bBiPred8x8Disable, uiNumMaxIter, uiIterSearchRange, uiBits, pcMbDataAccessBase ) );
      RNOK( xEstimateSubMb4x8   ( ePar8x8, m_pcIntMbTemp8x8Data, m_pcIntMbBest8x8Data, rcRefListStruct, uiMaxBlkMvs, bBiPred8x8Disable, uiNumMaxIter, uiIterSearchRange, uiBits, pcMbDataAccessBase ) );
      RNOK( xEstimateSubMb4x4   ( ePar8x8, m_pcIntMbTemp8x8Data, m_pcIntMbBest8x8Data, rcRefListStruct, uiMaxBlkMvs, bBiPred8x8Disable, uiNumMaxIter, uiIterSearchRange, uiBits, pcMbDataAccessBase ) );
    }
    ROTRS( m_pcIntMbBest8x8Data->rdCost() == DOUBLE_MAX, Err::m_nOK );

    //----- store parameters in MbTempData -----
    rpcMbTempData->rdCost()  += m_pcIntMbBest8x8Data->rdCost    ();
    BlkMode eBlkMode          = m_pcIntMbBest8x8Data->getBlkMode( ePar8x8 );
    rpcMbTempData->setBlkMode ( ePar8x8, eBlkMode );

    rpcMbTempData->getMbMotionData( LIST_0 ).copyFrom( m_pcIntMbBest8x8Data->getMbMotionData( LIST_0 ), eParIdx8x8 );
    rpcMbTempData->getMbMotionData( LIST_1 ).copyFrom( m_pcIntMbBest8x8Data->getMbMotionData( LIST_1 ), eParIdx8x8 );
    rpcMbTempData->getMbMvdData   ( LIST_0 ).copyFrom( m_pcIntMbBest8x8Data->getMbMvdData   ( LIST_0 ), eParIdx8x8 );
    rpcMbTempData->getMbMvdData   ( LIST_1 ).copyFrom( m_pcIntMbBest8x8Data->getMbMvdData   ( LIST_1 ), eParIdx8x8 );

    //----- set parameters in MbTemp8x8Data for prediction of next block -----
    m_pcIntMbTemp8x8Data->getMbMotionData( LIST_0 ).copyFrom( rpcMbTempData->getMbMotionData( LIST_0 ), eParIdx8x8 );
    m_pcIntMbTemp8x8Data->getMbMotionData( LIST_1 ).copyFrom( rpcMbTempData->getMbMotionData( LIST_1 ), eParIdx8x8 );
    m_pcIntMbBest8x8Data->getMbMotionData( LIST_0 ).copyFrom( rpcMbTempData->getMbMotionData( LIST_0 ), eParIdx8x8 );
    m_pcIntMbBest8x8Data->getMbMotionData( LIST_1 ).copyFrom( rpcMbTempData->getMbMotionData( LIST_1 ), eParIdx8x8 );

    //----- update motion vector count -----
    UInt  uiMvsPerList = ( eBlkMode == BLK_4x4 ? 4 : ( eBlkMode == BLK_4x8 || eBlkMode == BLK_8x4 ) ? 2 : 1 );
    uiMaxNumMv        -= ( rpcMbTempData->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) > 0 ? uiMvsPerList : 0 );
    uiMaxNumMv        -= ( rpcMbTempData->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) > 0 ? uiMvsPerList : 0 );
  }

  RNOK( xSetRdCostInterMb( *rpcMbTempData, pcMbDataAccessBase, rcRefListStruct, uiMinQP, uiMaxQP, bLowComplexMbEnable ) );



  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2=0,distortion=0;
	  for( Int n = 0; n <16; n++)
	  {
		  Int iRefIdx[2];
		  iRefIdx [0]=rcMbDataAccess.getMbMotionData(LIST_0).getRefIdx(B4x4Idx(n));
		  iRefIdx [1]=rcMbDataAccess.getMbMotionData(LIST_1).getRefIdx(B4x4Idx(n));
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=rcMbDataAccess.getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getHor();
			  iMvY=rcMbDataAccess.getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,n%4,n/4,1,1);
		  }
		  if(pcRefFrame1)
		  {
			  iMvX=rcMbDataAccess.getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getHor();
			  iMvY=rcMbDataAccess.getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,n%4,n/4,1,1);
			  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
		  }
		  distortion+=distortion1;
	  }
	  rpcMbTempData->rdCost()+=distortion;
  }

  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}




ErrVal
MbEncoder::xEstimateMb8x8Frext( IntMbTempData*&   rpcMbTempData,
                                IntMbTempData*&   rpcMbBestData,
                                RefListStruct&    rcRefListStruct,
                                UInt              uiMinQP,
                                UInt              uiMaxQP,
                                UInt              uiMaxNumMv,
                                UInt              uiNumMaxIter,
                                UInt              uiIterSearchRange,
                                MbDataAccess*     pcMbDataAccessBase,
                                Bool              bResidualPred )
{
  ROF  ( uiMaxNumMv );
  ROFRS( uiMaxNumMv >= 4,                           Err::m_nOK );
  ROTRS( rpcMbTempData->getSH().getSliceSkipFlag(), Err::m_nOK );
  ROTRS( !rpcMbTempData->getSH().getPPS().getTransform8x8ModeFlag(), Err::m_nOK );

  Bool  bPSlice  = rpcMbTempData->getMbDataAccess().getSH().isPSlice();
  UInt  uiBits   = ( ! bPSlice ? 9 : 5 ); // for signalling macroblock mode

  rpcMbTempData->clear    ();
  rpcMbTempData->setMbMode( MODE_8x8 );
  rpcMbTempData->setBLSkipFlag( false );
  rpcMbTempData->setResidualPredFlag( bResidualPred );
  rpcMbTempData->rdCost   () = 0;

  m_pcTransform->setQp( rpcMbTempData->getMbDataAccess(), false );

  for( Par8x8 ePar8x8 = B_8x8_0; ePar8x8 < 4; ePar8x8 = Par8x8( ePar8x8 + 1 ), uiBits = 0 )
  {
    UInt      uiRemBlocks     = 3 - (UInt)ePar8x8;
    UInt      uiMaxBlkMvMax   = uiMaxNumMv -   uiRemBlocks;         // don't care about following blocks
    UInt      uiMaxBlkMvMin   = uiMaxNumMv / ( uiRemBlocks + 1 );   // don't use more than available for other blocks
    UInt      uiMaxBlkMvs     = ( uiMaxBlkMvMax + uiMaxBlkMvMin + 1 ) >> 1;
    ParIdx8x8 aeParIdx8x8[4]  = { PART_8x8_0, PART_8x8_1, PART_8x8_2, PART_8x8_3 };
    ParIdx8x8 eParIdx8x8      = aeParIdx8x8[ ePar8x8 ];

    m_pcIntMbBest8x8Data->clear();
    RNOK( xEstimateSubMbDirect( ePar8x8, m_pcIntMbTemp8x8Data, m_pcIntMbBest8x8Data, rcRefListStruct, uiMaxBlkMvs, true,                                  uiBits ) );
    RNOK( xEstimateSubMb8x8   ( ePar8x8, m_pcIntMbTemp8x8Data, m_pcIntMbBest8x8Data, rcRefListStruct, uiMaxBlkMvs, true, uiNumMaxIter, uiIterSearchRange, uiBits, pcMbDataAccessBase ) );
    ROTRS( m_pcIntMbBest8x8Data->rdCost() == DOUBLE_MAX, Err::m_nOK );

    //----- store parameters in MbTempData -----
    rpcMbTempData->rdCost()  += m_pcIntMbBest8x8Data->rdCost    ();
    BlkMode eBlkMode          = m_pcIntMbBest8x8Data->getBlkMode( ePar8x8 );
    rpcMbTempData->setBlkMode ( ePar8x8, eBlkMode );

    rpcMbTempData->getMbMotionData( LIST_0 ).copyFrom( m_pcIntMbBest8x8Data->getMbMotionData( LIST_0 ), eParIdx8x8 );
    rpcMbTempData->getMbMotionData( LIST_1 ).copyFrom( m_pcIntMbBest8x8Data->getMbMotionData( LIST_1 ), eParIdx8x8 );
    rpcMbTempData->getMbMvdData   ( LIST_0 ).copyFrom( m_pcIntMbBest8x8Data->getMbMvdData   ( LIST_0 ), eParIdx8x8 );
    rpcMbTempData->getMbMvdData   ( LIST_1 ).copyFrom( m_pcIntMbBest8x8Data->getMbMvdData   ( LIST_1 ), eParIdx8x8 );

    //----- set parameters in MbTemp8x8Data for prediction of next block -----
    m_pcIntMbTemp8x8Data->getMbMotionData( LIST_0 ).copyFrom( rpcMbTempData->getMbMotionData( LIST_0 ), eParIdx8x8 );
    m_pcIntMbTemp8x8Data->getMbMotionData( LIST_1 ).copyFrom( rpcMbTempData->getMbMotionData( LIST_1 ), eParIdx8x8 );
    m_pcIntMbBest8x8Data->getMbMotionData( LIST_0 ).copyFrom( rpcMbTempData->getMbMotionData( LIST_0 ), eParIdx8x8 );
    m_pcIntMbBest8x8Data->getMbMotionData( LIST_1 ).copyFrom( rpcMbTempData->getMbMotionData( LIST_1 ), eParIdx8x8 );

    //----- update motion vector count -----
    UInt  uiMvsPerList = ( eBlkMode == BLK_4x4 ? 4 : ( eBlkMode == BLK_4x8 || eBlkMode == BLK_8x4 ) ? 2 : 1 );
    uiMaxNumMv        -= ( rpcMbTempData->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) > 0 ? uiMvsPerList : 0 );
    uiMaxNumMv        -= ( rpcMbTempData->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) > 0 ? uiMvsPerList : 0 );
  }

  IntMbTempData* pcMbRefData = rpcMbTempData;


  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2=0,distortion=0;
	  for( Int n = 0; n <16; n++)
	  {
		  Int iRefIdx[2];
		  iRefIdx [0]=rcMbDataAccess.getMbMotionData(LIST_0).getRefIdx(B4x4Idx(n));
		  iRefIdx [1]=rcMbDataAccess.getMbMotionData(LIST_1).getRefIdx(B4x4Idx(n));
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=rcMbDataAccess.getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getHor();
			  iMvY=rcMbDataAccess.getMbMotionData(LIST_0).getMv(B4x4Idx(n)).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,n%4,n/4,1,1);
		  }
		  if(pcRefFrame1)
		  {
			  iMvX=rcMbDataAccess.getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getHor();
			  iMvY=rcMbDataAccess.getMbMotionData(LIST_1).getMv(B4x4Idx(n)).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,n%4,n/4,1,1);
			  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
		  }
		  distortion+=distortion1;
	  }
	  setEpRef(distortion);
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckInterMbMode8x8(  rpcMbTempData, rpcMbBestData, pcMbRefData, rcRefListStruct, uiMinQP, uiMaxQP, false, pcMbDataAccessBase ) );

  return Err::m_nOK;
}



ErrVal
MbEncoder::xEstimateSubMbDirect( Par8x8            ePar8x8,
                                 IntMbTempData*&   rpcMbTempData,
                                 IntMbTempData*&   rpcMbBestData,
                                 RefListStruct&    rcRefListStruct,
                                 UInt              uiMaxNumMv,
                                 Bool              bTrafo8x8,
                                 UInt              uiAddBits )
{
  ROFRS( m_bUseBDir, Err::m_nOK );
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListME[ 1 ];
  ROF  ( uiMaxNumMv );
  ROFRS( rcRefFrameList0.getActive() && rcRefFrameList1.getActive(),  Err::m_nOK );

  ParIdx8x8 aeParIdx8x8 [4] = { PART_8x8_0, PART_8x8_1, PART_8x8_2, PART_8x8_3 };
  ParIdx8x8 eParIdx8x8      = aeParIdx8x8[ ePar8x8 ];

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  rpcMbTempData->clear();
  rpcMbTempData->setBlkMode( ePar8x8, BLK_SKIP );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv       ( Mv::ZeroMv(), eParIdx8x8 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv       ( Mv::ZeroMv(), eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag ( false,        eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag ( false,        eParIdx8x8 );
  RNOK( rpcMbTempData->getMbDataAccess().setSVCDirectModeMvAndRef( rcRefFrameList0, rcRefFrameList1, (Int)ePar8x8 ) );

  if( rpcMbTempData->getSH().isH264AVCCompatible() )
  {
    Bool bOneMv = false;
    Bool bFaultTolerant = false;
    MbDataAccess&  rcMbDataAccess = rpcMbTempData->getMbDataAccess();
    ROFRS( rcMbDataAccess.getMvPredictorDirect( eParIdx8x8, bOneMv, bFaultTolerant, &rcRefFrameList0, &rcRefFrameList1 ), Err::m_nOK );
  }

  UInt   uiNumMv = ( rpcMbTempData->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) > 0 ? 1 : 0 ) + ( rpcMbTempData->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) > 0 ? 1 : 0 );
  ROFRS( uiNumMv <= uiMaxNumMv, Err::m_nOK );

  RNOK( xSetRdCostInterSubMb( *rpcMbTempData, rcRefListStruct, B8x8Idx( ePar8x8 ), bTrafo8x8, 1+uiAddBits, bLowComplexMbEnable ) );


  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2=0,distortion=0;
	  Int iRefIdx[2];
	  iRefIdx [0]=rpcMbTempData->getMbMotionData(LIST_0).getRefIdx(eParIdx8x8);
	  iRefIdx [1]=rpcMbTempData->getMbMotionData(LIST_1).getRefIdx(eParIdx8x8);
	  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
	  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
	  Int iMvX;
	  Int iMvY;

	  if(pcRefFrame0)
	  {
		  iMvX=rpcMbTempData->getMbMotionData(LIST_0).getMv(eParIdx8x8).getHor();
		  iMvY=rpcMbTempData->getMbMotionData(LIST_0).getMv(eParIdx8x8).getVer();
		  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,eParIdx8x8%4,eParIdx8x8/4,2,2);
	  }

	  if(pcRefFrame1)
	  {
		  iMvX=rpcMbTempData->getMbMotionData(LIST_1).getMv(eParIdx8x8).getHor();
		  iMvY=rpcMbTempData->getMbMotionData(LIST_1).getMv(eParIdx8x8).getVer();
		  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,eParIdx8x8%4,eParIdx8x8/4,2,2);
		  if(pcRefFrame0)
			  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
		  else
			  distortion1=distortion2;
	  }
    distortion+=distortion1;
	  rpcMbTempData->rdCost()+=distortion;

  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}

ErrVal
MbEncoder::xEstimateSubMb8x8( Par8x8            ePar8x8,
                              IntMbTempData*&   rpcMbTempData,
                              IntMbTempData*&   rpcMbBestData,
                              RefListStruct&    rcRefListStruct,
                              UInt              uiMaxNumMv,
                              Bool              bTrafo8x8,
                              UInt              uiNumMaxIter,
                              UInt              uiIterSearchRange,
                              UInt              uiAddBits,
                              MbDataAccess*     pcMbDataAccessBase )
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListME[ 1 ];
  ROF( uiMaxNumMv );
  ROF( rcRefFrameList0.getActive() <= 32 );
  ROF( rcRefFrameList1.getActive() <= 32 );
  ROF( rcRefFrameList0.getActive() );

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  Bool            bPSlice         = rpcMbTempData->getMbDataAccess().getSH().isPSlice();
  Double          fRdCost         = 0;
  UInt            uiSubMbBits     = 0;
  ParIdx8x8       aeParIdx8x8 [4] = { PART_8x8_0, PART_8x8_1, PART_8x8_2, PART_8x8_3 };
  ParIdx8x8       eParIdx8x8      = aeParIdx8x8[ ePar8x8 ];
  UInt            uiCost      [3] = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX }, uiCostTest;
  UInt            uiBlkBits   [3] = { ( ! bPSlice ? 3 : 1 ) + uiAddBits, 3 + uiAddBits, 5 + uiAddBits };
  Int             iRefIdx     [2] = { 0, 0 }, iRefIdxBi[2], iRefIdxTest;
  UInt            uiBits      [3] = { 0, 0, 0 }, uiBitsTest;
  Mv              cMv[2], cMvBi[2], cMvPred[2][33], cMvLastEst[2][33], cMvd[2];
  YuvMbBuffer     cYuvMbBuffer[2];
  Frame*          pcRefFrame    = 0;
  Bool            bBLPred   [2] = { false, false };
  Bool            bBLPredBi [2] = { false, false };
  Int             iBLRefIdx [2] = { -1, -1 };
  Mv              cBLMvPred [2], cBLMvLastEst[2];

  if( pcMbDataAccessBase && m_bBaseMotionPredAllowed )
  {
    if( pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) >  0 &&
        pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) <= (Int)rcRefFrameList0.getActive() )
    {
      iBLRefIdx [0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx ( eParIdx8x8 );
      cBLMvPred [0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx8x8 );
    }
    if( pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) >  0 &&
        pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) <= (Int)rcRefFrameList1.getActive() )
    {
      iBLRefIdx [1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx ( eParIdx8x8 );
      cBLMvPred [1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx8x8 );
    }
  }

  UInt  uiBasePredType = MSYS_UINT_MAX;
  rpcMbTempData->clear();
  rpcMbTempData->setBlkMode( ePar8x8, BLK_8x8 );

  //===== LIST 0 PREDICTION ======
  for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList0.getActive(); iRefIdxTest++ )
  {
    pcRefFrame = rcRefFrameList0[iRefIdxTest];

    if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
    {
      rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[0][iRefIdxTest], iRefIdxTest,
                                                          LIST_0, eParIdx8x8);
      uiBitsTest                  = ( uiBasePredType == 0 ? 0 : uiBlkBits[0] ) + getRefIdxBits( iRefIdxTest, rcRefFrameList0 );
      cMvLastEst[0][iRefIdxTest]  = cMvPred   [0][iRefIdxTest];
      if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[0] )
      {
        cMvLastEst[0][iRefIdxTest] = cBLMvPred[0];
        m_pcMotionEstimation->setEL( true );
      }
      RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                          cMvLastEst[0][iRefIdxTest],
                                                          cMvPred   [0][iRefIdxTest],
                                                          uiBitsTest, uiCostTest,
                                                          eParIdx8x8+SPART_8x8, BLK_8x8, 0,
                                                          &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
      if( uiCostTest < uiCost[0] )
      {
        bBLPred [0] = false;
        iRefIdx [0] = iRefIdxTest;
        cMv     [0] = cMvLastEst[0][iRefIdxTest];
        uiBits  [0] = uiBitsTest;
        uiCost  [0] = uiCostTest;
      }
    }

    if( iRefIdxTest == iBLRefIdx[0] )
    {
      uiBitsTest      = ( uiBasePredType == 0 ? 0 : uiBlkBits[0] );
      cBLMvLastEst[0] = cBLMvPred [0];
      m_pcMotionEstimation->setEL( true );

      rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[0], LIST_0, eParIdx8x8 );
      RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                          cBLMvLastEst[0],
                                                          cBLMvPred   [0],
                                                          uiBitsTest, uiCostTest,
                                                          eParIdx8x8+SPART_8x8, BLK_8x8, 0,
                                                          &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
      if( uiCostTest < uiCost[0] )
      {
        bBLPred [0] = true;
        iRefIdx [0] = iRefIdxTest;
        cMv     [0] = cBLMvLastEst[0];
        uiBits  [0] = uiBitsTest;
        uiCost  [0] = uiCostTest;
      }
    }
  }


  //===== LIST 1 PREDICTION =====
  for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList1.getActive(); iRefIdxTest++ )
  {
    pcRefFrame = rcRefFrameList1[iRefIdxTest];

    if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
    {
      rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[1][iRefIdxTest], iRefIdxTest,
                                                          LIST_1, eParIdx8x8 );
      uiBitsTest                  = ( uiBasePredType == 1 ? 0 : uiBlkBits[1] ) + getRefIdxBits( iRefIdxTest, rcRefFrameList1 );
      cMvLastEst[1][iRefIdxTest]  = cMvPred   [1][iRefIdxTest];
      if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[1] )
      {
        cMvLastEst[1][iRefIdxTest] = cBLMvPred[1];
        m_pcMotionEstimation->setEL( true );
      }
      RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                          cMvLastEst[1][iRefIdxTest],
                                                          cMvPred   [1][iRefIdxTest],
                                                          uiBitsTest, uiCostTest,
                                                          eParIdx8x8+SPART_8x8, BLK_8x8, 0,
                                                          &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
      if( uiCostTest < uiCost[1] )
      {
        bBLPred [1] = false;
        iRefIdx [1] = iRefIdxTest;
        cMv     [1] = cMvLastEst[1][iRefIdxTest];
        uiBits  [1] = uiBitsTest;
        uiCost  [1] = uiCostTest;

        RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[1],
                                                          eParIdx8x8+SPART_8x8, BLK_8x8 ) );
      }
    }

    if( iRefIdxTest == iBLRefIdx[1] )
    {
      uiBitsTest      = ( uiBasePredType == 1 ? 0 : uiBlkBits[1] );
      cBLMvLastEst[1] = cBLMvPred [1];
      m_pcMotionEstimation->setEL( true );

      rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[1], LIST_1, eParIdx8x8 );
      RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                          cBLMvLastEst[1],
                                                          cBLMvPred   [1],
                                                          uiBitsTest, uiCostTest,
                                                          eParIdx8x8+SPART_8x8, BLK_8x8, 0,
                                                          &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
      if( uiCostTest < uiCost[1] )
      {
        bBLPred [1] = true;
        iRefIdx [1] = iRefIdxTest;
        cMv     [1] = cBLMvLastEst[1];
        uiBits  [1] = uiBitsTest;
        uiCost  [1] = uiCostTest;

        RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[1],
                                                          eParIdx8x8+SPART_8x8, BLK_8x8 ) );
      }
    }
  }

  ROTRS( uiCost[0] == MSYS_UINT_MAX && uiCost[1] == MSYS_UINT_MAX, Err::m_nOK );
  Bool bBiPredOk = ( uiCost[0] != MSYS_UINT_MAX && uiCost[1] != MSYS_UINT_MAX );

  //===== BI PREDICTION =====
  if( bBiPredOk && rcRefFrameList0.getActive() && rcRefFrameList1.getActive() && uiMaxNumMv >= 2 )
  {
    //----- initialize with forward and backward estimation -----
    iRefIdxBi [0]       = iRefIdx [0];
    iRefIdxBi [1]       = iRefIdx [1];
    bBLPredBi [0]       = bBLPred [0];
    bBLPredBi [1]       = bBLPred [1];
    cMvBi     [0]       = cMv     [0];
    cMvBi     [1]       = cMv     [1];
    UInt  uiMotBits[2]  = { uiBits[0] - uiBlkBits[0], uiBits[1] - uiBlkBits[1] };
    uiBits[2]           = uiBlkBits[2] + uiMotBits[0] + uiMotBits[1];

    if( ! uiNumMaxIter )
    {
      uiNumMaxIter      = 1;
      uiIterSearchRange = 0;
    }

    //----- iterative search -----
    for( UInt uiIter = 0; uiIter < uiNumMaxIter; uiIter++ )
    {
      Bool          bChanged        = false;
      UInt          uiDir           = uiIter % 2;
      RefFrameList& rcRefFrameList  = ( uiDir ? rcRefFrameList1 : rcRefFrameList0 );
      BSParams      cBSParams;
      cBSParams.pcAltRefFrame       = ( uiDir ? rcRefFrameList0 : rcRefFrameList1 )[ iRefIdxBi[ 1 - uiDir ] ];
      cBSParams.pcAltRefPelData     = &cYuvMbBuffer[1-uiDir];
      cBSParams.uiL1Search          = uiDir;
      cBSParams.apcWeight[LIST_0]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxBi[LIST_0], rpcMbTempData->getFieldFlag() );
      cBSParams.apcWeight[LIST_1]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxBi[LIST_1], rpcMbTempData->getFieldFlag());

      for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList.getActive(); iRefIdxTest++ )
      {
        if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
        {
          uiBitsTest  = ( uiBasePredType == 2 ? 0 : uiBlkBits[2] ) + uiMotBits[1-uiDir] + getRefIdxBits( iRefIdxTest, rcRefFrameList );
          pcRefFrame  = rcRefFrameList[iRefIdxTest];

          RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                              cMvLastEst[uiDir][iRefIdxTest],
                                                              cMvPred   [uiDir][iRefIdxTest],
                                                              uiBitsTest, uiCostTest,
                                                              eParIdx8x8+SPART_8x8, BLK_8x8, 
                                                              uiIterSearchRange, 0, &cBSParams ) );
          if( uiCostTest < uiCost[2] )
          {
            bChanged          = true;
            bBLPredBi [uiDir] = false;
            iRefIdxBi [uiDir] = iRefIdxTest;
            cMvBi     [uiDir] = cMvLastEst[uiDir][iRefIdxTest];
            uiMotBits [uiDir] = uiBitsTest - uiBlkBits[2] - uiMotBits[1-uiDir];
            uiBits    [2]     = uiBitsTest;
            uiCost    [2]     = uiCostTest;

            RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[uiDir],
                                                              eParIdx8x8+SPART_8x8, BLK_8x8 ) );
          }
        }

        if( iRefIdxTest == iBLRefIdx[uiDir] )
        {
          uiBitsTest  = ( uiBasePredType == 2 ? 0 : uiBlkBits[2] ) + uiMotBits[1-uiDir];
          RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                              cBLMvLastEst[uiDir],
                                                              cBLMvPred   [uiDir],
                                                              uiBitsTest, uiCostTest,
                                                              eParIdx8x8+SPART_8x8, BLK_8x8, 
                                                              uiIterSearchRange, 0, &cBSParams ) );
          if( uiCostTest < uiCost[2] )
          {
            bChanged          = true;
            bBLPredBi [uiDir] = true;
            iRefIdxBi [uiDir] = iRefIdxTest;
            cMvBi     [uiDir] = cBLMvLastEst[uiDir];
            uiMotBits [uiDir] = uiBitsTest - uiBlkBits[2] - uiMotBits[1-uiDir];
            uiBits    [2]     = uiBitsTest;
            uiCost    [2]     = uiCostTest;

            RNOK( m_pcMotionEstimation->compensateBlock     ( &cYuvMbBuffer[uiDir],
                                                              eParIdx8x8+SPART_8x8, BLK_8x8 ) );
          }
        }
      }

      if( ! bChanged )
      {
        break;
      }
    }
  }


  //===== chose parameters =====
  if( uiCost[2] <= uiCost[0] && uiCost[2] <= uiCost[1] )
  {
    //----- bi-directional prediction -----
    fRdCost     = uiCost    [2];
    uiSubMbBits = uiBits    [2];
    iRefIdx [0] = iRefIdxBi [0];
    iRefIdx [1] = iRefIdxBi [1];
    bBLPred [0] = bBLPredBi [0];
    bBLPred [1] = bBLPredBi [1];
    if( bBLPred[0] )  cMvPred[0][iRefIdx[0]] = cBLMvPred[0];
    if( bBLPred[1] )  cMvPred[1][iRefIdx[1]] = cBLMvPred[1];
    cMv     [0] = cMvBi     [0];
    cMv     [1] = cMvBi     [1];
    cMvd    [0] = cMv       [0] - cMvPred[0][iRefIdx[0]];
    cMvd    [1] = cMv       [1] - cMvPred[1][iRefIdx[1]];
  }
  else if( uiCost[0] <= uiCost[1] )
  {
    //----- list 0 prediction -----
    fRdCost     = uiCost[0];
    uiSubMbBits = uiBits[0];
    iRefIdx [1] = BLOCK_NOT_PREDICTED;
    bBLPred [1] = false;
    if( bBLPred[0] )  cMvPred[0][iRefIdx[0]] = cBLMvPred[0];
    cMv     [1] = Mv::ZeroMv();
    cMvd    [0] = cMv[0] - cMvPred[0][iRefIdx[0]];
  }
  else
  {
    //----- list 1 prediction -----
    fRdCost     = uiCost[1];
    uiSubMbBits = uiBits[1];
    iRefIdx [0] = BLOCK_NOT_PREDICTED;
    bBLPred [0] = false;
    if( bBLPred[1] )  cMvPred[1][iRefIdx[1]] = cBLMvPred[1];
    cMv     [0] = Mv::ZeroMv();
    cMvd    [1] = cMv[1] - cMvPred[1][iRefIdx[1]];
  }


  //===== set parameters and compare =====
  rpcMbTempData->rdCost() = fRdCost;
  rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx( iRefIdx [0],  eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0],  eParIdx8x8 );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0],  eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx( iRefIdx [1],  eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1],  eParIdx8x8 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1],  eParIdx8x8 );

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( bBLPred[0], eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( bBLPred[1], eParIdx8x8 );
  ROT( bBLPred[0] && iRefIdx[0] != iBLRefIdx[0] );
  ROT( bBLPred[1] && iRefIdx[1] != iBLRefIdx[1] );

  RNOK( xSetRdCostInterSubMb( *rpcMbTempData, rcRefListStruct, B8x8Idx( ePar8x8 ), bTrafo8x8, uiSubMbBits, bLowComplexMbEnable ) );


  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2=0,distortion=0;
	  for( Int n = 0; n <1; n++)
	  {
		  iRefIdx [0]=rpcMbTempData->getMbMotionData(LIST_0).getRefIdx(eParIdx8x8);
		  iRefIdx [1]=rpcMbTempData->getMbMotionData(LIST_1).getRefIdx(eParIdx8x8);
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_0).getMv(eParIdx8x8).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_0).getMv(eParIdx8x8).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,eParIdx8x8%4,eParIdx8x8/4,2,2);
		  }

		  if(pcRefFrame1)
		  {
			  iMvX=rpcMbTempData->getMbMotionData(LIST_1).getMv(eParIdx8x8).getHor();
			  iMvY=rpcMbTempData->getMbMotionData(LIST_1).getMv(eParIdx8x8).getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,eParIdx8x8%4,eParIdx8x8/4,2,2);
			  if(pcRefFrame0)
				  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
			  else
				  distortion1=distortion2;
		  }
		  distortion+=distortion1;
	  }
	  rpcMbTempData->rdCost()+=distortion;
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}

ErrVal
MbEncoder::xEstimateSubMb8x4( Par8x8            ePar8x8,
                              IntMbTempData*&   rpcMbTempData,
                              IntMbTempData*&   rpcMbBestData,
                              RefListStruct&    rcRefListStruct,
                              UInt              uiMaxNumMv,
                              Bool              bBiPred8x8Disable,
                              UInt              uiNumMaxIter,
                              UInt              uiIterSearchRange,
                              UInt              uiAddBits,
                              MbDataAccess*     pcMbDataAccessBase )
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListME[ 1 ];
  ROF  ( uiMaxNumMv );
  ROFRS( uiMaxNumMv >= 2, Err::m_nOK );
  ROF  ( rcRefFrameList0.getActive() <= 32 );
  ROF  ( rcRefFrameList1.getActive() <= 32 );
  ROF  ( rcRefFrameList0.getActive() );

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  Bool            bPSlice         = rpcMbTempData->getMbDataAccess().getSH().isPSlice();
  Double          fRdCost         = 0;
  UInt            uiSubMbBits     = 0;
  SParIdx8x4      aeParIdx8x4 [2] = { SPART_8x4_0, SPART_8x4_1 };
  ParIdx8x8       aeParIdx8x8 [4] = { PART_8x8_0, PART_8x8_1, PART_8x8_2, PART_8x8_3 };
  ParIdx8x8       eParIdx8x8      = aeParIdx8x8[ ePar8x8 ];
  UInt            uiCost      [3] = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX }, uiCostTest;
  UInt            uiBlkBits   [3] = { ( ! bPSlice ? 5 : 3 ) + uiAddBits, 5 + uiAddBits, 7 + uiAddBits };
  Int             iRefIdx     [2] = { 0, 0 }, iRefIdxBi[2], iRefIdxTest;
  UInt            uiBits      [3] = { 0, 0, 0 }, uiBitsTest;
  Mv              cMv[2][2], cMvBi[2][2], cMvd[2][2], cMvLastEst[2][33][2], cMvPred[2][33][2], cMvPredBi[2][2];
  YuvMbBuffer     cYuvMbBuffer[2], cTmpYuvMbBuffer;
  Frame*          pcRefFrame;
  Bool            bBLPred   [2] = { false, false };
  Bool            bBLPredBi [2] = { false, false };
  Int             iBLRefIdx [2] = { -1, -1 };
  Mv              cBLMvPred [2][2], cBLMvLastEst[2][2];

  if( pcMbDataAccessBase && m_bBaseMotionPredAllowed )
  {
    if( pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) >  0 &&
        pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) <= (Int)rcRefFrameList0.getActive() )
    {
      iBLRefIdx [0]    = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx ( eParIdx8x8 );
      cBLMvPred [0][0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx8x8, SPART_8x4_0 );
      cBLMvPred [0][1] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx8x8, SPART_8x4_1 );
    }
    if( pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) >  0 &&
        pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) <= (Int)rcRefFrameList1.getActive() )
    {
      iBLRefIdx [1]    = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx ( eParIdx8x8 );
      cBLMvPred [1][0] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx8x8, SPART_8x4_0 );
      cBLMvPred [1][1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx8x8, SPART_8x4_1 );
    }
  }

  UInt  uiBasePredType = MSYS_UINT_MAX;

  rpcMbTempData->clear();
  rpcMbTempData->setBlkMode( ePar8x8, BLK_8x4 );


  //===== LIST 0 PREDICTION ======
  for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList0.getActive(); iRefIdxTest++ )
  {
    rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx( iRefIdxTest, eParIdx8x8 );
    pcRefFrame  = rcRefFrameList0[iRefIdxTest];

    if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;

      for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
      {
        SParIdx8x4  eSubParIdx  = aeParIdx8x4[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 0 ? 0 : uiBlkBits[0] + getRefIdxBits( iRefIdxTest, rcRefFrameList0 ) );
        UInt        uiTmpCost;

        rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[0][iRefIdxTest][uiBlk], iRefIdxTest,
                                                            LIST_0, eParIdx8x8, eSubParIdx );
        cMvLastEst[0][iRefIdxTest][uiBlk] = cMvPred[0][iRefIdxTest][uiBlk];

        if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[0] )
        {
          cMvLastEst[0][iRefIdxTest][uiBlk] = cBLMvPred[0][uiBlk];
          m_pcMotionEstimation->setEL( true );
        }
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cMvLastEst[0][iRefIdxTest][uiBlk],
                                                            cMvPred   [0][iRefIdxTest][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_8x4, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMvLastEst[0][iRefIdxTest][uiBlk], eParIdx8x8, eSubParIdx );

        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[0] )
      {
        bBLPred [0]     = false;
        iRefIdx [0]     = iRefIdxTest;
        cMv     [0][0]  = cMvLastEst[0][iRefIdxTest][0];
        cMv     [0][1]  = cMvLastEst[0][iRefIdxTest][1];
        uiBits  [0]     = uiBitsTest;
        uiCost  [0]     = uiCostTest;
      }
    }

    if( iRefIdxTest == iBLRefIdx[0] )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;
      for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
      {
        SParIdx8x4  eSubParIdx  = aeParIdx8x4[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 0 ? 0 : uiBlkBits[0] );
        UInt        uiTmpCost;
        cBLMvLastEst[0][uiBlk]  = cBLMvPred[0][uiBlk];
        m_pcMotionEstimation->setEL( true );

        rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[0][uiBlk], LIST_0, eParIdx8x8, eSubParIdx );
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cBLMvLastEst[0][uiBlk],
                                                            cBLMvPred   [0][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_8x4, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cBLMvLastEst[0][uiBlk], eParIdx8x8, eSubParIdx );
        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[0] )
      {
        bBLPred [0]     = true;
        iRefIdx [0]     = iRefIdxTest;
        cMv     [0][0]  = cBLMvLastEst[0][0];
        cMv     [0][1]  = cBLMvLastEst[0][1];
        uiBits  [0]     = uiBitsTest;
        uiCost  [0]     = uiCostTest;
      }
    }
  }


  //===== LIST 1 PREDICTION =====
  for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList1.getActive(); iRefIdxTest++ )
  {
    rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx( iRefIdxTest, eParIdx8x8 );
    pcRefFrame  = rcRefFrameList1[iRefIdxTest];
 
    if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;

      for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
      {
        SParIdx8x4  eSubParIdx  = aeParIdx8x4[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 1 ? 0 : uiBlkBits[1] + getRefIdxBits( iRefIdxTest, rcRefFrameList1 ) );
        UInt        uiTmpCost;

        rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[1][iRefIdxTest][uiBlk], iRefIdxTest,
                                                            LIST_1, eParIdx8x8, eSubParIdx );
        cMvLastEst[1][iRefIdxTest][uiBlk] = cMvPred[1][iRefIdxTest][uiBlk];

        if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[1] )
        {
          cMvLastEst[1][iRefIdxTest][uiBlk] = cBLMvPred[1][uiBlk];
          m_pcMotionEstimation->setEL( true );
        }
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cMvLastEst[1][iRefIdxTest][uiBlk],
                                                            cMvPred   [1][iRefIdxTest][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_8x4, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        RNOK( m_pcMotionEstimation->compensateBlock       ( &cTmpYuvMbBuffer,
                                                            eParIdx8x8+eSubParIdx, BLK_8x4 ) );
        rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMvLastEst[1][iRefIdxTest][uiBlk], eParIdx8x8, eSubParIdx );

        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[1] )
      {
        bBLPred [1]     = false;
        iRefIdx [1]     = iRefIdxTest;
        cMv     [1][0]  = cMvLastEst[1][iRefIdxTest][0];
        cMv     [1][1]  = cMvLastEst[1][iRefIdxTest][1];
        uiBits  [1]     = uiBitsTest;
        uiCost  [1]     = uiCostTest;

        cYuvMbBuffer[1].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
      }
    }

    if( iRefIdxTest == iBLRefIdx[1] )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;
      for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
      {
        SParIdx8x4  eSubParIdx  = aeParIdx8x4[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 1 ? 0 : uiBlkBits[1] );
        UInt        uiTmpCost;
        cBLMvLastEst[1][uiBlk]  = cBLMvPred[1][uiBlk];
        m_pcMotionEstimation->setEL( true );

        rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[1][uiBlk], LIST_1, eParIdx8x8, eSubParIdx );
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cBLMvLastEst[1][uiBlk],
                                                            cBLMvPred   [1][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_8x4, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag() ) ) );
        RNOK( m_pcMotionEstimation->compensateBlock       ( &cTmpYuvMbBuffer,
                                                            eParIdx8x8+eSubParIdx, BLK_8x4 ) );
        rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cBLMvLastEst[1][uiBlk], eParIdx8x8, eSubParIdx );
        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[1] )
      {
        bBLPred [1]     = true;
        iRefIdx [1]     = iRefIdxTest;
        cMv     [1][0]  = cBLMvLastEst[1][0];
        cMv     [1][1]  = cBLMvLastEst[1][1];
        uiBits  [1]     = uiBitsTest;
        uiCost  [1]     = uiCostTest;
        cYuvMbBuffer[1].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
      }
    }
  }

  ROTRS( uiCost[0] == MSYS_UINT_MAX && uiCost[1] == MSYS_UINT_MAX, Err::m_nOK );
  Bool bBiPredOk = ( uiCost[0] != MSYS_UINT_MAX && uiCost[1] != MSYS_UINT_MAX );

  //===== BI PREDICTION =====
  if( bBiPredOk && !bBiPred8x8Disable && rcRefFrameList0.getActive() && rcRefFrameList1.getActive() && uiMaxNumMv >= 4 )
  {
    //----- initialize with forward and backward estimation -----
    iRefIdxBi [0] = iRefIdx [0];
    iRefIdxBi [1] = iRefIdx [1];
    bBLPredBi [0] = bBLPred [0];
    bBLPredBi [1] = bBLPred [1];

    memcpy( cMvBi,      cMv,      2*2*sizeof(Mv ) );

    cMvPredBi[0][0]     = cMvPred[0][iRefIdx[0]][0];
    cMvPredBi[0][1]     = cMvPred[0][iRefIdx[0]][1];
    cMvPredBi[1][0]     = cMvPred[1][iRefIdx[1]][0];
    cMvPredBi[1][1]     = cMvPred[1][iRefIdx[1]][1];
    UInt  uiMotBits[2]  = { uiBits[0] - uiBlkBits[0], uiBits[1] - uiBlkBits[1] };
    uiBits[2]           = uiBlkBits[2] + uiMotBits[0] + uiMotBits[1];

    if( ! uiNumMaxIter )
    {
      uiNumMaxIter      = 1;
      uiIterSearchRange = 0;
    }

    //----- iterative search -----
    for( UInt uiIter = 0; uiIter < uiNumMaxIter; uiIter++ )
    {
      Bool          bChanged        = false;
      UInt          uiDir           = uiIter % 2;
      ListIdx       eListIdx        = ListIdx( uiDir );
      RefFrameList& rcRefFrameList  = ( uiDir ? rcRefFrameList1 : rcRefFrameList0 );
      BSParams      cBSParams;
      cBSParams.pcAltRefFrame       = ( uiDir ? rcRefFrameList0 : rcRefFrameList1 )[ iRefIdxBi[ 1 - uiDir ] ];
      cBSParams.pcAltRefPelData     = &cYuvMbBuffer[1-uiDir];
      cBSParams.uiL1Search          = uiDir;
      cBSParams.apcWeight[LIST_0]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxBi[LIST_0], rpcMbTempData->getFieldFlag() );
      cBSParams.apcWeight[LIST_1]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxBi[LIST_1], rpcMbTempData->getFieldFlag() );

      for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList.getActive(); iRefIdxTest++ )
      {
        Mv  cMvPredTest[2];
        rpcMbTempData->getMbMotionData( eListIdx ).setRefIdx( iRefIdxTest, eParIdx8x8 );
        pcRefFrame  = rcRefFrameList[iRefIdxTest];

        if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
        {
          uiBitsTest  = 0;
          uiCostTest  = 0;

          for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
          {
            SParIdx8x4  eSubParIdx  = aeParIdx8x4[ uiBlk ];
            UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 2 ? 0 : uiBlkBits[2] + uiMotBits[1-uiDir] + getRefIdxBits( iRefIdxTest, rcRefFrameList ) );
            UInt        uiTmpCost;

            rpcMbTempData->getMbDataAccess().getMvPredictor     ( cMvPredTest[uiBlk], iRefIdxTest,
                                                                  eListIdx, eParIdx8x8, eSubParIdx );
            RNOK( m_pcMotionEstimation->estimateBlockWithStart  ( *rpcMbTempData, *pcRefFrame,
                                                                  cMvLastEst[uiDir][iRefIdxTest][uiBlk],
                                                                  cMvPredTest                   [uiBlk],
                                                                  uiTmpBits, uiTmpCost,
                                                                  eParIdx8x8+eSubParIdx, BLK_8x4, 
                                                                  uiIterSearchRange, 0, &cBSParams ) );
            RNOK( m_pcMotionEstimation->compensateBlock         ( &cTmpYuvMbBuffer,
                                                                  eParIdx8x8+eSubParIdx, BLK_8x4 ) );
            rpcMbTempData->getMbMotionData( eListIdx ).setAllMv ( cMvLastEst[uiDir][iRefIdxTest][uiBlk], eParIdx8x8, eSubParIdx );

            uiBitsTest  += uiTmpBits;
            uiCostTest  += uiTmpCost;
          }

          if( uiCostTest < uiCost[2] )
          {
            bChanged              = true;
            bBLPredBi [uiDir]     = false;
            iRefIdxBi [uiDir]     = iRefIdxTest;
            cMvBi     [uiDir][0]  = cMvLastEst[uiDir][iRefIdxTest][0];
            cMvBi     [uiDir][1]  = cMvLastEst[uiDir][iRefIdxTest][1];
            cMvPredBi [uiDir][0]  = cMvPredTest                   [0];
            cMvPredBi [uiDir][1]  = cMvPredTest                   [1];
            uiMotBits [uiDir]     = uiBitsTest - uiBlkBits[2] - uiMotBits[1-uiDir];
            uiBits    [2]         = uiBitsTest;
            uiCost    [2]         = uiCostTest;

            cYuvMbBuffer[uiDir].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
          }
        }

        if( iRefIdxTest == iBLRefIdx[uiDir] )
        {
          uiBitsTest  = 0;
          uiCostTest  = 0;
          for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
          {
            SParIdx8x4  eSubParIdx  = aeParIdx8x4[ uiBlk ];
            UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 2 ? 0 : uiBlkBits[2] + uiMotBits[1-uiDir] );
            UInt        uiTmpCost;
            RNOK( m_pcMotionEstimation->estimateBlockWithStart  ( *rpcMbTempData, *pcRefFrame,
                                                                  cBLMvLastEst[uiDir][uiBlk],
                                                                  cBLMvPred   [uiDir][uiBlk],
                                                                  uiTmpBits, uiTmpCost,
                                                                  eParIdx8x8+eSubParIdx, BLK_8x4, 
                                                                  uiIterSearchRange, 0, &cBSParams ) );
            RNOK( m_pcMotionEstimation->compensateBlock         ( &cTmpYuvMbBuffer,
                                                                  eParIdx8x8+eSubParIdx, BLK_8x4 ) );
            rpcMbTempData->getMbMotionData( eListIdx ).setAllMv ( cBLMvLastEst[uiDir][uiBlk], eParIdx8x8, eSubParIdx );
            uiBitsTest  += uiTmpBits;
            uiCostTest  += uiTmpCost;
          }

          if( uiCostTest < uiCost[2] )
          {
            bChanged              = true;
            bBLPredBi [uiDir]     = true;
            iRefIdxBi [uiDir]     = iRefIdxTest;
            cMvBi     [uiDir][0]  = cBLMvLastEst[uiDir][0];
            cMvBi     [uiDir][1]  = cBLMvLastEst[uiDir][1];
            uiMotBits [uiDir]     = uiBitsTest - uiBlkBits[2] - uiMotBits[1-uiDir];
            uiBits    [2]         = uiBitsTest;
            uiCost    [2]         = uiCostTest;

            cYuvMbBuffer[uiDir].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
          }
        }
      }

      if( ! bChanged )
      {
        break;
      }
    }
  }


  //===== chose parameters =====
  if( uiCost[2] <= uiCost[0] && uiCost[2] <= uiCost[1] )
  {
    //----- bi-directional prediction -----
    fRdCost         = uiCost    [2];
    uiSubMbBits     = uiBits    [2];
    iRefIdx [0]     = iRefIdxBi [0];
    iRefIdx [1]     = iRefIdxBi [1];
    bBLPred [0]     = bBLPredBi [0];
    bBLPred [1]     = bBLPredBi [1];
    if( bBLPred[0] )  { cMvPredBi[0][0] = cBLMvPred[0][0]; cMvPredBi[0][1] = cBLMvPred[0][1];  }
    if( bBLPred[1] )  { cMvPredBi[1][0] = cBLMvPred[1][0]; cMvPredBi[1][1] = cBLMvPred[1][1];  }
    cMv     [0][0]  = cMvBi     [0][0];
    cMv     [0][1]  = cMvBi     [0][1];
    cMv     [1][0]  = cMvBi     [1][0];
    cMv     [1][1]  = cMvBi     [1][1];
    cMvd    [0][0]  = cMv       [0][0]  - cMvPredBi[0][0];
    cMvd    [0][1]  = cMv       [0][1]  - cMvPredBi[0][1];
    cMvd    [1][0]  = cMv       [1][0]  - cMvPredBi[1][0];
    cMvd    [1][1]  = cMv       [1][1]  - cMvPredBi[1][1];
  }
  else if( uiCost[0] <= uiCost[1] )
  {
    //----- list 0 prediction -----
    fRdCost         = uiCost    [0];
    uiSubMbBits     = uiBits    [0];
    iRefIdx [1]     = BLOCK_NOT_PREDICTED;
    bBLPred [1]     = false;
    if( bBLPred[0] )  { cMvPred[0][iRefIdx[0]][0] = cBLMvPred[0][0]; cMvPred[0][iRefIdx[0]][1] = cBLMvPred[0][1];  }
    cMv     [1][0]  = Mv::ZeroMv();
    cMv     [1][1]  = Mv::ZeroMv();
    cMvd    [0][0]  = cMv[0][0] - cMvPred[0][iRefIdx[0]][0];
    cMvd    [0][1]  = cMv[0][1] - cMvPred[0][iRefIdx[0]][1];
  }
  else
  {
    //----- list 1 prediction -----
    fRdCost         = uiCost    [1];
    uiSubMbBits     = uiBits    [1];
    iRefIdx [0]     = BLOCK_NOT_PREDICTED;
    bBLPred [0]     = false;
    if( bBLPred[1] )  { cMvPred[1][iRefIdx[1]][0] = cBLMvPred[1][0]; cMvPred[1][iRefIdx[1]][1] = cBLMvPred[1][1];  }
    cMv     [0][0]  = Mv::ZeroMv();
    cMv     [0][1]  = Mv::ZeroMv();
    cMvd    [1][0]  = cMv[1][0] - cMvPred[1][iRefIdx[1]][0];
    cMvd    [1][1]  = cMv[1][1] - cMvPred[1][iRefIdx[1]][1];
  }


  //===== set parameters and compare =====
  rpcMbTempData->rdCost() = fRdCost;
  rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx( iRefIdx [0],    eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0][0], eParIdx8x8, SPART_8x4_0 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0][1], eParIdx8x8, SPART_8x4_1 );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0][0], eParIdx8x8, SPART_8x4_0 );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0][1], eParIdx8x8, SPART_8x4_1 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx( iRefIdx [1],    eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1][0], eParIdx8x8, SPART_8x4_0 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1][1], eParIdx8x8, SPART_8x4_1 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1][0], eParIdx8x8, SPART_8x4_0 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1][1], eParIdx8x8, SPART_8x4_1 );

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( bBLPred[0], eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( bBLPred[1], eParIdx8x8 );
  ROT( bBLPred[0] && iRefIdx[0] != iBLRefIdx[0] );
  ROT( bBLPred[1] && iRefIdx[1] != iBLRefIdx[1] );

  RNOK( xSetRdCostInterSubMb( *rpcMbTempData, rcRefListStruct, B8x8Idx( ePar8x8 ), false, uiSubMbBits, bLowComplexMbEnable ) );


  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2=0,distortion=0;
	  for( Int n = 0; n <2; n++)
	  {
		  iRefIdx [0]=rpcMbTempData->getMbMotionData(LIST_0).getRefIdx(eParIdx8x8);
		  iRefIdx [1]=rpcMbTempData->getMbMotionData(LIST_1).getRefIdx(eParIdx8x8);
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=cMv[LIST_0][n].getHor();
			  iMvY=cMv[LIST_0][n].getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,(eParIdx8x8+aeParIdx8x4[n])%4,(eParIdx8x8+aeParIdx8x4 [n])/4,2,1);
		  }
		  if(pcRefFrame1)
		  {
			  iMvX=cMv[LIST_1][n].getHor();
			  iMvY=cMv[LIST_1][n].getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,(eParIdx8x8+aeParIdx8x4[n])%4,(eParIdx8x8+aeParIdx8x4 [n])/4,2,1);
			  if(pcRefFrame0)
				  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
			  else
				  distortion1=distortion2;
		  }
		  distortion+=distortion1;
	  }
	  rpcMbTempData->rdCost()+=distortion;
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}

ErrVal
MbEncoder::xEstimateSubMb4x8( Par8x8            ePar8x8,
                              IntMbTempData*&   rpcMbTempData,
                              IntMbTempData*&   rpcMbBestData,
                              RefListStruct&    rcRefListStruct,
                              UInt              uiMaxNumMv,
                              Bool              bBiPred8x8Disable,
                              UInt              uiNumMaxIter,
                              UInt              uiIterSearchRange,
                              UInt              uiAddBits,
                              MbDataAccess*     pcMbDataAccessBase )
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListME[ 1 ];
  ROF   ( uiMaxNumMv );
  ROFRS ( uiMaxNumMv >= 2, Err::m_nOK );
  ROF   ( rcRefFrameList0.getActive() <= 32 );
  ROF   ( rcRefFrameList1.getActive() <= 32 );
  ROF   ( rcRefFrameList0.getActive() );

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  Bool            bPSlice         = rpcMbTempData->getMbDataAccess().getSH().isPSlice();
  Double          fRdCost         = 0;
  UInt            uiSubMbBits     = 0;
  SParIdx4x8      aeParIdx4x8 [2] = { SPART_4x8_0, SPART_4x8_1 };
  ParIdx8x8       aeParIdx8x8 [4] = { PART_8x8_0, PART_8x8_1, PART_8x8_2, PART_8x8_3 };
  ParIdx8x8       eParIdx8x8      = aeParIdx8x8[ ePar8x8 ];
  UInt            uiCost      [3] = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX }, uiCostTest;
  UInt            uiBlkBits   [3] = { ( ! bPSlice ? 5 : 3 ) + uiAddBits, 7 + uiAddBits, 7 + uiAddBits };
  Int             iRefIdx     [2] = { 0, 0 }, iRefIdxBi[2], iRefIdxTest;
  UInt            uiBits      [3] = { 0, 0, 0 }, uiBitsTest;
  Mv              cMv[2][2], cMvBi[2][2], cMvd[2][2], cMvLastEst[2][33][2], cMvPred[2][33][2], cMvPredBi[2][2];
  YuvMbBuffer     cYuvMbBuffer[2], cTmpYuvMbBuffer;
  Frame*          pcRefFrame;
  Bool            bBLPred   [2] = { false, false };
  Bool            bBLPredBi [2] = { false, false };
  Int             iBLRefIdx [2] = { -1, -1 };
  Mv              cBLMvPred [2][2], cBLMvLastEst[2][2];

  if( pcMbDataAccessBase && m_bBaseMotionPredAllowed )
  {
    if( pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) >  0 &&
        pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) <= (Int)rcRefFrameList0.getActive() )
    {
      iBLRefIdx [0]    = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx ( eParIdx8x8 );
      cBLMvPred [0][0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx8x8, SPART_4x8_0 );
      cBLMvPred [0][1] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx8x8, SPART_4x8_1 );
    }
    if( pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) >  0 &&
        pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) <= (Int)rcRefFrameList1.getActive() )
    {
      iBLRefIdx [1]    = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx ( eParIdx8x8 );
      cBLMvPred [1][0] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx8x8, SPART_4x8_0 );
      cBLMvPred [1][1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx8x8, SPART_4x8_1 );
    }
  }

  UInt  uiBasePredType = MSYS_UINT_MAX;

  rpcMbTempData->clear();
  rpcMbTempData->setBlkMode( ePar8x8, BLK_4x8 );


  //===== LIST 0 PREDICTION ======
  for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList0.getActive(); iRefIdxTest++ )
  {
    rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx( iRefIdxTest, eParIdx8x8 );
    pcRefFrame  = rcRefFrameList0[iRefIdxTest];

    if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;

      for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
      {
        SParIdx4x8  eSubParIdx  = aeParIdx4x8[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 0 ? 0 : uiBlkBits[0] + getRefIdxBits( iRefIdxTest, rcRefFrameList0 ) );
        UInt        uiTmpCost;

        rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[0][iRefIdxTest][uiBlk], iRefIdxTest,
                                                            LIST_0, eParIdx8x8, eSubParIdx );
        cMvLastEst[0][iRefIdxTest][uiBlk] = cMvPred[0][iRefIdxTest][uiBlk];

        if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[0] )
        {
          cMvLastEst[0][iRefIdxTest][uiBlk] = cBLMvPred[0][uiBlk];
          m_pcMotionEstimation->setEL( true );
        }
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cMvLastEst[0][iRefIdxTest][uiBlk],
                                                            cMvPred   [0][iRefIdxTest][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_4x8, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMvLastEst[0][iRefIdxTest][uiBlk], eParIdx8x8, eSubParIdx );

        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[0] )
      {
        bBLPred [0]     = false;
        iRefIdx [0]     = iRefIdxTest;
        cMv     [0][0]  = cMvLastEst[0][iRefIdxTest][0];
        cMv     [0][1]  = cMvLastEst[0][iRefIdxTest][1];
        uiBits  [0]     = uiBitsTest;
        uiCost  [0]     = uiCostTest;
      }
    }

    if( iRefIdxTest == iBLRefIdx[0] )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;
      for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
      {
        SParIdx4x8  eSubParIdx  = aeParIdx4x8[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 0 ? 0 : uiBlkBits[0] );
        UInt        uiTmpCost;
        cBLMvLastEst[0][uiBlk]  = cBLMvPred[0][uiBlk];
        m_pcMotionEstimation->setEL( true );

        rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[0][uiBlk], LIST_0, eParIdx8x8, eSubParIdx );
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cBLMvLastEst[0][uiBlk],
                                                            cBLMvPred   [0][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_4x8, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cBLMvLastEst[0][uiBlk], eParIdx8x8, eSubParIdx );
        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[0] )
      {
        bBLPred [0]     = true;
        iRefIdx [0]     = iRefIdxTest;
        cMv     [0][0]  = cBLMvLastEst[0][0];
        cMv     [0][1]  = cBLMvLastEst[0][1];
        uiBits  [0]     = uiBitsTest;
        uiCost  [0]     = uiCostTest;
      }
    }
  }


  //===== LIST 1 PREDICTION =====
  for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList1.getActive(); iRefIdxTest++ )
  {
    rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx( iRefIdxTest, eParIdx8x8 );
    pcRefFrame  = rcRefFrameList1[iRefIdxTest];

    if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;

      for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
      {
        SParIdx4x8  eSubParIdx  = aeParIdx4x8[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 1 ? 0 : uiBlkBits[1] + getRefIdxBits( iRefIdxTest, rcRefFrameList1 ) );
        UInt        uiTmpCost;

        rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[1][iRefIdxTest][uiBlk], iRefIdxTest,
                                                            LIST_1, eParIdx8x8, eSubParIdx );
        cMvLastEst[1][iRefIdxTest][uiBlk] = cMvPred[1][iRefIdxTest][uiBlk];

        if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[1] )
        {
          cMvLastEst[1][iRefIdxTest][uiBlk] = cBLMvPred[1][uiBlk];
          m_pcMotionEstimation->setEL( true );
        }
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cMvLastEst[1][iRefIdxTest][uiBlk],
                                                            cMvPred   [1][iRefIdxTest][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_4x8, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        RNOK( m_pcMotionEstimation->compensateBlock       ( &cTmpYuvMbBuffer,
                                                            eParIdx8x8+eSubParIdx, BLK_4x8 ) );
        rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMvLastEst[1][iRefIdxTest][uiBlk], eParIdx8x8, eSubParIdx );

        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[1] )
      {
        bBLPred [1]     = false;
        iRefIdx [1]     = iRefIdxTest;
        cMv     [1][0]  = cMvLastEst[1][iRefIdxTest][0];
        cMv     [1][1]  = cMvLastEst[1][iRefIdxTest][1];
        uiBits  [1]     = uiBitsTest;
        uiCost  [1]     = uiCostTest;

        cYuvMbBuffer[1].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
      }
    }

    if( iRefIdxTest == iBLRefIdx[1] )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;
      for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
      {
        SParIdx4x8  eSubParIdx  = aeParIdx4x8[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 1 ? 0 : uiBlkBits[1] );
        UInt        uiTmpCost;
        cBLMvLastEst[1][uiBlk] = cBLMvPred[1][uiBlk];
        m_pcMotionEstimation->setEL( true );

        rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[1][uiBlk], LIST_1, eParIdx8x8, eSubParIdx );
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cBLMvLastEst[1][uiBlk],
                                                            cBLMvPred   [1][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_4x8, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        RNOK( m_pcMotionEstimation->compensateBlock       ( &cTmpYuvMbBuffer,
                                                            eParIdx8x8+eSubParIdx, BLK_4x8 ) );
        rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cBLMvLastEst[1][uiBlk], eParIdx8x8, eSubParIdx );
        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[1] )
      {
        bBLPred [1]     = true;
        iRefIdx [1]     = iRefIdxTest;
        cMv     [1][0]  = cBLMvLastEst[1][0];
        cMv     [1][1]  = cBLMvLastEst[1][1];
        uiBits  [1]     = uiBitsTest;
        uiCost  [1]     = uiCostTest;

        cYuvMbBuffer[1].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
      }
    }
  }

  ROTRS( uiCost[0] == MSYS_UINT_MAX && uiCost[1] == MSYS_UINT_MAX, Err::m_nOK );
  Bool bBiPredOk = ( uiCost[0] != MSYS_UINT_MAX && uiCost[1] != MSYS_UINT_MAX );

  //===== BI PREDICTION =====
  if( bBiPredOk && !bBiPred8x8Disable && rcRefFrameList0.getActive() && rcRefFrameList1.getActive() && uiMaxNumMv >= 4 )
  {
    //----- initialize with forward and backward estimation -----
    iRefIdxBi [0] = iRefIdx [0];
    iRefIdxBi [1] = iRefIdx [1];
    bBLPredBi [0] = bBLPred [0];
    bBLPredBi [1] = bBLPred [1];

    memcpy( cMvBi,      cMv,      2*2*sizeof(Mv ) );

    cMvPredBi [0][0]    = cMvPred [0][iRefIdx[0]][0];
    cMvPredBi [0][1]    = cMvPred [0][iRefIdx[0]][1];
    cMvPredBi [1][0]    = cMvPred [1][iRefIdx[1]][0];
    cMvPredBi [1][1]    = cMvPred [1][iRefIdx[1]][1];
    UInt  uiMotBits[2]  = { uiBits[0] - uiBlkBits[0], uiBits[1] - uiBlkBits[1] };
    uiBits[2]           = uiBlkBits[2] + uiMotBits[0] + uiMotBits[1];

    if( ! uiNumMaxIter )
    {
      uiNumMaxIter      = 1;
      uiIterSearchRange = 0;
    }

    //----- iterative search -----
    for( UInt uiIter = 0; uiIter < uiNumMaxIter; uiIter++ )
    {
      Bool          bChanged        = false;
      UInt          uiDir           = uiIter % 2;
      ListIdx       eListIdx        = ListIdx( uiDir );
      RefFrameList& rcRefFrameList  = ( uiDir ? rcRefFrameList1 : rcRefFrameList0 );
      BSParams      cBSParams;
      cBSParams.pcAltRefFrame       = ( uiDir ? rcRefFrameList0 : rcRefFrameList1 )[ iRefIdxBi[ 1 - uiDir ] ];
      cBSParams.pcAltRefPelData     = &cYuvMbBuffer[1-uiDir];
      cBSParams.uiL1Search          = uiDir;
      cBSParams.apcWeight[LIST_0]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxBi[LIST_0], rpcMbTempData->getFieldFlag() );
      cBSParams.apcWeight[LIST_1]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxBi[LIST_1], rpcMbTempData->getFieldFlag() );

      for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList.getActive(); iRefIdxTest++ )
      {
        Mv  cMvPredTest[2];
        rpcMbTempData->getMbMotionData( eListIdx ).setRefIdx( iRefIdxTest, eParIdx8x8 );
        pcRefFrame  = rcRefFrameList[iRefIdxTest];

        if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
        {
          uiBitsTest  = 0;
          uiCostTest  = 0;

          for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
          {
            SParIdx4x8  eSubParIdx  = aeParIdx4x8[ uiBlk ];
            UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 2 ? 0 : uiBlkBits[2] + uiMotBits[1-uiDir] + getRefIdxBits( iRefIdxTest, rcRefFrameList ) );
            UInt        uiTmpCost;

            rpcMbTempData->getMbDataAccess().getMvPredictor     ( cMvPredTest[uiBlk], iRefIdxTest,
                                                                  eListIdx, eParIdx8x8, eSubParIdx );
            RNOK( m_pcMotionEstimation->estimateBlockWithStart  ( *rpcMbTempData, *pcRefFrame,
                                                                  cMvLastEst[uiDir][iRefIdxTest][uiBlk],
                                                                  cMvPredTest                   [uiBlk],
                                                                  uiTmpBits, uiTmpCost,
                                                                  eParIdx8x8+eSubParIdx, BLK_4x8, 
                                                                  uiIterSearchRange, 0, &cBSParams ) );
            RNOK( m_pcMotionEstimation->compensateBlock         ( &cTmpYuvMbBuffer,
                                                                  eParIdx8x8+eSubParIdx, BLK_4x8 ) );
            rpcMbTempData->getMbMotionData( eListIdx ).setAllMv ( cMvLastEst[uiDir][iRefIdxTest][uiBlk], eParIdx8x8, eSubParIdx );

            uiBitsTest += uiTmpBits;
            uiCostTest += uiTmpCost;
          }

          if( uiCostTest < uiCost[2] )
          {
            bChanged              = true;
            bBLPredBi [uiDir]     = false;
            iRefIdxBi [uiDir]     = iRefIdxTest;
            cMvBi     [uiDir][0]  = cMvLastEst[uiDir][iRefIdxTest][0];
            cMvBi     [uiDir][1]  = cMvLastEst[uiDir][iRefIdxTest][1];
            cMvPredBi [uiDir][0]  = cMvPredTest                   [0];
            cMvPredBi [uiDir][1]  = cMvPredTest                   [1];
            uiMotBits [uiDir]     = uiBitsTest - uiBlkBits[2] - uiMotBits[1-uiDir];
            uiBits    [2]         = uiBitsTest;
            uiCost    [2]         = uiCostTest;

            cYuvMbBuffer[uiDir].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
          }
        }

        if( iRefIdxTest == iBLRefIdx[uiDir] )
        {
          uiBitsTest  = 0;
          uiCostTest  = 0;
          for( UInt uiBlk = 0; uiBlk < 2; uiBlk++ )
          {
            SParIdx4x8  eSubParIdx  = aeParIdx4x8[ uiBlk ];
            UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 2 ? 0 : uiBlkBits[2] + uiMotBits[1-uiDir] );
            UInt        uiTmpCost;
            RNOK( m_pcMotionEstimation->estimateBlockWithStart  ( *rpcMbTempData, *pcRefFrame,
                                                                  cBLMvLastEst[uiDir][uiBlk],
                                                                  cBLMvPred   [uiDir][uiBlk],
                                                                  uiTmpBits, uiTmpCost,
                                                                  eParIdx8x8+eSubParIdx, BLK_4x8, 
                                                                  uiIterSearchRange, 0, &cBSParams ) );
            RNOK( m_pcMotionEstimation->compensateBlock         ( &cTmpYuvMbBuffer,
                                                                  eParIdx8x8+eSubParIdx, BLK_4x8 ) );
            rpcMbTempData->getMbMotionData( eListIdx ).setAllMv ( cBLMvLastEst[uiDir][uiBlk], eParIdx8x8, eSubParIdx );
            uiBitsTest += uiTmpBits;
            uiCostTest += uiTmpCost;
          }

          if( uiCostTest < uiCost[2] )
          {
            bChanged              = true;
            bBLPredBi [uiDir]     = true;
            iRefIdxBi [uiDir]     = iRefIdxTest;
            cMvBi     [uiDir][0]  = cBLMvLastEst[uiDir][0];
            cMvBi     [uiDir][1]  = cBLMvLastEst[uiDir][1];
            uiMotBits [uiDir]     = uiBitsTest - uiBlkBits[2] - uiMotBits[1-uiDir];
            uiBits    [2]         = uiBitsTest;
            uiCost    [2]         = uiCostTest;

            cYuvMbBuffer[uiDir].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
          }
        }
      }

      if( ! bChanged )
      {
        break;
      }
    }
  }


  //===== chose parameters =====
  if( uiCost[2] <= uiCost[0] && uiCost[2] <= uiCost[1] )
  {
    //----- bi-directional prediction -----
    fRdCost         = uiCost    [2];
    uiSubMbBits     = uiBits    [2];
    iRefIdx [0]     = iRefIdxBi [0];
    iRefIdx [1]     = iRefIdxBi [1];
    bBLPred [0]     = bBLPredBi [0];
    bBLPred [1]     = bBLPredBi [1];
    if( bBLPred[0] )  { cMvPredBi[0][0] = cBLMvPred[0][0]; cMvPredBi[0][1] = cBLMvPred[0][1];  }
    if( bBLPred[1] )  { cMvPredBi[1][0] = cBLMvPred[1][0]; cMvPredBi[1][1] = cBLMvPred[1][1];  }
    cMv     [0][0]  = cMvBi     [0][0];
    cMv     [0][1]  = cMvBi     [0][1];
    cMv     [1][0]  = cMvBi     [1][0];
    cMv     [1][1]  = cMvBi     [1][1];
    cMvd    [0][0]  = cMv       [0][0]  - cMvPredBi[0][0];
    cMvd    [0][1]  = cMv       [0][1]  - cMvPredBi[0][1];
    cMvd    [1][0]  = cMv       [1][0]  - cMvPredBi[1][0];
    cMvd    [1][1]  = cMv       [1][1]  - cMvPredBi[1][1];
  }
  else if( uiCost[0] <= uiCost[1] )
  {
    //----- list 0 prediction -----
    fRdCost         = uiCost    [0];
    uiSubMbBits     = uiBits    [0];
    iRefIdx [1]     = BLOCK_NOT_PREDICTED;
    bBLPred [1]     = false;
    if( bBLPred[0] )  { cMvPred[0][iRefIdx[0]][0] = cBLMvPred[0][0]; cMvPred[0][iRefIdx[0]][1] = cBLMvPred[0][1];  }
    cMv     [1][0]  = Mv::ZeroMv();
    cMv     [1][1]  = Mv::ZeroMv();
    cMvd    [0][0]  = cMv[0][0] - cMvPred[0][iRefIdx[0]][0];
    cMvd    [0][1]  = cMv[0][1] - cMvPred[0][iRefIdx[0]][1];
  }
  else
  {
    //----- list 1 prediction -----
    fRdCost         = uiCost    [1];
    uiSubMbBits     = uiBits    [1];
    iRefIdx [0]     = BLOCK_NOT_PREDICTED;
    bBLPred [0]     = false;
    if( bBLPred[1] )  { cMvPred[1][iRefIdx[1]][0] = cBLMvPred[1][0]; cMvPred[1][iRefIdx[1]][1] = cBLMvPred[1][1];  }
    cMv     [0][0]  = Mv::ZeroMv();
    cMv     [0][1]  = Mv::ZeroMv();
    cMvd    [1][0]  = cMv[1][0] - cMvPred[1][iRefIdx[1]][0];
    cMvd    [1][1]  = cMv[1][1] - cMvPred[1][iRefIdx[1]][1];
  }


  //===== set parameters and compare =====
  rpcMbTempData->rdCost() = fRdCost;
  rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx( iRefIdx [0],    eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0][0], eParIdx8x8, SPART_4x8_0 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0][1], eParIdx8x8, SPART_4x8_1 );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0][0], eParIdx8x8, SPART_4x8_0 );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0][1], eParIdx8x8, SPART_4x8_1 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx( iRefIdx [1],    eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1][0], eParIdx8x8, SPART_4x8_0 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1][1], eParIdx8x8, SPART_4x8_1 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1][0], eParIdx8x8, SPART_4x8_0 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1][1], eParIdx8x8, SPART_4x8_1 );

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( bBLPred[0], eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( bBLPred[1], eParIdx8x8 );
  ROT( bBLPred[0] && iRefIdx[0] != iBLRefIdx[0] );
  ROT( bBLPred[1] && iRefIdx[1] != iBLRefIdx[1] );

  RNOK( xSetRdCostInterSubMb( *rpcMbTempData, rcRefListStruct, B8x8Idx( ePar8x8 ), false, uiSubMbBits, bLowComplexMbEnable ) );


  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2,distortion=0;
	  for( Int n = 0; n <2; n++)
	  {
		  iRefIdx [0]=rpcMbTempData->getMbMotionData(LIST_0).getRefIdx(eParIdx8x8);
		  iRefIdx [1]=rpcMbTempData->getMbMotionData(LIST_1).getRefIdx(eParIdx8x8);
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=cMv[LIST_0][n].getHor();
			  iMvY=cMv[LIST_0][n].getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,(eParIdx8x8+aeParIdx4x8[n])%4,(eParIdx8x8+aeParIdx4x8 [n])/4,1,2);
		  }
		  if(pcRefFrame1)
		  {
			  iMvX=cMv[LIST_1][n].getHor();
			  iMvY=cMv[LIST_1][n].getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,(eParIdx8x8+aeParIdx4x8[n])%4,(eParIdx8x8+aeParIdx4x8 [n])/4,1,2);
			  if(pcRefFrame0)
				  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
			  else
				  distortion1=distortion2;
		  }
		  distortion+=distortion1;
	  }
	  rpcMbTempData->rdCost()+=distortion;
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}





ErrVal
MbEncoder::xEstimateSubMb4x4( Par8x8            ePar8x8,
                              IntMbTempData*&   rpcMbTempData,
                              IntMbTempData*&   rpcMbBestData,
                              RefListStruct&    rcRefListStruct,
                              UInt              uiMaxNumMv,
                              Bool              bBiPred8x8Disable,
                              UInt              uiNumMaxIter,
                              UInt              uiIterSearchRange,
                              UInt              uiAddBits,
                              MbDataAccess*     pcMbDataAccessBase )
{
  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListME[ 0 ];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListME[ 1 ];
  ROF  ( uiMaxNumMv );
  ROFRS( uiMaxNumMv >= 4, Err::m_nOK );
  ROF  ( rcRefFrameList0.getActive() <= 32 );
  ROF  ( rcRefFrameList1.getActive() <= 32 );
  ROF  ( rcRefFrameList0.getActive() );

  //JVT-V079 Low-complexity MB mode decision
  Bool bLowComplexMbEnable = m_bLowComplexMbEnable[rpcMbTempData->getSH().getDependencyId()];

  Bool            bPSlice         = rpcMbTempData->getMbDataAccess().getSH().isPSlice();
  Double          fRdCost         = 0;
  UInt            uiSubMbBits     = 0;
  SParIdx4x4      aeParIdx4x4 [4] = { SPART_4x4_0, SPART_4x4_1, SPART_4x4_2, SPART_4x4_3 };
  ParIdx8x8       aeParIdx8x8 [4] = { PART_8x8_0, PART_8x8_1, PART_8x8_2, PART_8x8_3 };
  ParIdx8x8       eParIdx8x8      = aeParIdx8x8[ ePar8x8 ];
  UInt            uiCost      [3] = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX }, uiCostTest;
  UInt            uiBlkBits   [3] = { ( ! bPSlice ? 7 : 5 ) + uiAddBits, 7 + uiAddBits, 7 + uiAddBits };
  Int             iRefIdx     [2] = { 0, 0 }, iRefIdxBi[2], iRefIdxTest;
  UInt            uiBits      [3] = { 0, 0, 0 }, uiBitsTest;
  Mv              cMv[2][4], cMvBi[2][4], cMvd[2][4], cMvLastEst[2][33][4], cMvPred[2][33][4], cMvPredBi[2][4];
  YuvMbBuffer     cYuvMbBuffer[2], cTmpYuvMbBuffer;
  Frame*          pcRefFrame;
  Bool            bBLPred   [2] = { false, false };
  Bool            bBLPredBi [2] = { false, false };
  Int             iBLRefIdx [2] = { -1, -1 };
  Mv              cBLMvPred [2][4], cBLMvLastEst[2][4];

  if( pcMbDataAccessBase && m_bBaseMotionPredAllowed )
  {
    if( pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) >  0 &&
        pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx( eParIdx8x8 ) <= (Int)rcRefFrameList0.getActive() )
    {
      iBLRefIdx [0]    = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getRefIdx ( eParIdx8x8 );
      cBLMvPred [0][0] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx8x8, SPART_4x4_0 );
      cBLMvPred [0][1] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx8x8, SPART_4x4_1 );
      cBLMvPred [0][2] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx8x8, SPART_4x4_2 );
      cBLMvPred [0][3] = pcMbDataAccessBase->getMbMotionData( LIST_0 ).getMv     ( eParIdx8x8, SPART_4x4_3 );
    }
    if( pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) >  0 &&
        pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx( eParIdx8x8 ) <= (Int)rcRefFrameList1.getActive() )
    {
      iBLRefIdx [1]    = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getRefIdx ( eParIdx8x8 );
      cBLMvPred [1][0] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx8x8, SPART_4x4_0 );
      cBLMvPred [1][1] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx8x8, SPART_4x4_1 );
      cBLMvPred [1][2] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx8x8, SPART_4x4_2 );
      cBLMvPred [1][3] = pcMbDataAccessBase->getMbMotionData( LIST_1 ).getMv     ( eParIdx8x8, SPART_4x4_3 );
    }
  }

  UInt  uiBasePredType = MSYS_UINT_MAX;

  rpcMbTempData->clear();
  rpcMbTempData->setBlkMode( ePar8x8, BLK_4x4 );


  //===== LIST 0 PREDICTION ======
  for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList0.getActive(); iRefIdxTest++ )
  {
    rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx( iRefIdxTest, eParIdx8x8 );
    pcRefFrame  = rcRefFrameList0[iRefIdxTest];

    if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;

      for( UInt uiBlk = 0; uiBlk < 4; uiBlk++ )
      {
        SParIdx4x4  eSubParIdx  = aeParIdx4x4[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 0 ? 0 : uiBlkBits[0] + getRefIdxBits( iRefIdxTest, rcRefFrameList0 ) );
        UInt        uiTmpCost;

        rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[0][iRefIdxTest][uiBlk], iRefIdxTest,
                                                            LIST_0, eParIdx8x8, eSubParIdx );
        cMvLastEst[0][iRefIdxTest][uiBlk] = cMvPred[0][iRefIdxTest][uiBlk];

        if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[0] )
        {
          cMvLastEst[0][iRefIdxTest][uiBlk] = cBLMvPred[0][uiBlk];
          m_pcMotionEstimation->setEL( true );
        }
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cMvLastEst[0][iRefIdxTest][uiBlk],
                                                            cMvPred   [0][iRefIdxTest][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_4x4, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMvLastEst[0][iRefIdxTest][uiBlk], eParIdx8x8, eSubParIdx );

        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[0] )
      {
        bBLPred [0]     = false;
        iRefIdx [0]     = iRefIdxTest;
        cMv     [0][0]  = cMvLastEst[0][iRefIdxTest][0];
        cMv     [0][1]  = cMvLastEst[0][iRefIdxTest][1];
        cMv     [0][2]  = cMvLastEst[0][iRefIdxTest][2];
        cMv     [0][3]  = cMvLastEst[0][iRefIdxTest][3];
        uiBits  [0]     = uiBitsTest;
        uiCost  [0]     = uiCostTest;
      }
    }

    if( iRefIdxTest == iBLRefIdx[0] )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;
      for( UInt uiBlk = 0; uiBlk < 4; uiBlk++ )
      {
        SParIdx4x4  eSubParIdx  = aeParIdx4x4[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 0 ? 0 : uiBlkBits[0] );
        UInt        uiTmpCost;
        cBLMvLastEst[0][uiBlk]  = cBLMvPred[0][uiBlk];
        m_pcMotionEstimation->setEL( true );

        rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[0][uiBlk], LIST_0, eParIdx8x8, eSubParIdx );
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cBLMvLastEst[0][uiBlk],
                                                            cBLMvPred   [0][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_4x4, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cBLMvLastEst[0][uiBlk], eParIdx8x8, eSubParIdx );
        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[0] )
      {
        bBLPred [0]     = true;
        iRefIdx [0]     = iRefIdxTest;
        cMv     [0][0]  = cBLMvLastEst[0][0];
        cMv     [0][1]  = cBLMvLastEst[0][1];
        cMv     [0][2]  = cBLMvLastEst[0][2];
        cMv     [0][3]  = cBLMvLastEst[0][3];
        uiBits  [0]     = uiBitsTest;
        uiCost  [0]     = uiCostTest;
      }
    }
  }


  //===== LIST 1 PREDICTION =====
  for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList1.getActive(); iRefIdxTest++ )
  {
    rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx( iRefIdxTest, eParIdx8x8 );
    pcRefFrame  = rcRefFrameList1[iRefIdxTest];

    if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;

      for( UInt uiBlk = 0; uiBlk < 4; uiBlk++ )
      {
        SParIdx4x4  eSubParIdx  = aeParIdx4x4[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 1 ? 0 : uiBlkBits[1] + getRefIdxBits( iRefIdxTest, rcRefFrameList1 ) );
        UInt        uiTmpCost;

        rpcMbTempData->getMbDataAccess().getMvPredictor   ( cMvPred[1][iRefIdxTest][uiBlk], iRefIdxTest,
                                                            LIST_1, eParIdx8x8, eSubParIdx );
        cMvLastEst[1][iRefIdxTest][uiBlk] = cMvPred[1][iRefIdxTest][uiBlk];

        if( m_pcMotionEstimation->getELSearch() && iRefIdxTest == iBLRefIdx[1] )
        {
          cMvLastEst[1][iRefIdxTest][uiBlk] = cBLMvPred[1][uiBlk];
          m_pcMotionEstimation->setEL( true );
        }
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cMvLastEst[1][iRefIdxTest][uiBlk],
                                                            cMvPred   [1][iRefIdxTest][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_4x4, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        RNOK( m_pcMotionEstimation->compensateBlock       ( &cTmpYuvMbBuffer,
                                                            eParIdx8x8+eSubParIdx, BLK_4x4 ) );
        rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMvLastEst[1][iRefIdxTest][uiBlk], eParIdx8x8, eSubParIdx );

        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[1] )
      {
        bBLPred [1]     = false;
        iRefIdx [1]     = iRefIdxTest;
        cMv     [1][0]  = cMvLastEst[1][iRefIdxTest][0];
        cMv     [1][1]  = cMvLastEst[1][iRefIdxTest][1];
        cMv     [1][2]  = cMvLastEst[1][iRefIdxTest][2];
        cMv     [1][3]  = cMvLastEst[1][iRefIdxTest][3];
        uiBits  [1]     = uiBitsTest;
        uiCost  [1]     = uiCostTest;

        cYuvMbBuffer[1].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
      }
    }

    if( iRefIdxTest == iBLRefIdx[1] )
    {
      uiBitsTest  = 0;
      uiCostTest  = 0;
      for( UInt uiBlk = 0; uiBlk < 4; uiBlk++ )
      {
        SParIdx4x4  eSubParIdx  = aeParIdx4x4[ uiBlk ];
        UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 1 ? 0 : uiBlkBits[1] );
        UInt        uiTmpCost;
        cBLMvLastEst[1][uiBlk]  = cBLMvPred[1][uiBlk];
        m_pcMotionEstimation->setEL( true );

        rpcMbTempData->getMbDataAccess().setMvPredictorsBL( cBLMvPred[1][uiBlk], LIST_1, eParIdx8x8, eSubParIdx );
        RNOK( m_pcMotionEstimation->estimateBlockWithStart( *rpcMbTempData, *pcRefFrame,
                                                            cBLMvLastEst[1][uiBlk],
                                                            cBLMvPred   [1][uiBlk],
                                                            uiTmpBits, uiTmpCost,
                                                            eParIdx8x8+eSubParIdx, BLK_4x4, 0,
                                                            &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxTest, rpcMbTempData->getFieldFlag()) ) );
        RNOK( m_pcMotionEstimation->compensateBlock       ( &cTmpYuvMbBuffer,
                                                            eParIdx8x8+eSubParIdx, BLK_4x4 ) );
        rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cBLMvLastEst[1][uiBlk], eParIdx8x8, eSubParIdx );
        uiBitsTest  += uiTmpBits;
        uiCostTest  += uiTmpCost;
      }

      if( uiCostTest < uiCost[1] )
      {
        bBLPred [1]     = true;
        iRefIdx [1]     = iRefIdxTest;
        cMv     [1][0]  = cBLMvLastEst[1][0];
        cMv     [1][1]  = cBLMvLastEst[1][1];
        cMv     [1][2]  = cBLMvLastEst[1][2];
        cMv     [1][3]  = cBLMvLastEst[1][3];
        uiBits  [1]     = uiBitsTest;
        uiCost  [1]     = uiCostTest;

        cYuvMbBuffer[1].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
      }
    }
  }

  ROTRS( uiCost[0] == MSYS_UINT_MAX && uiCost[1] == MSYS_UINT_MAX, Err::m_nOK );
  Bool bBiPredOk = ( uiCost[0] != MSYS_UINT_MAX && uiCost[1] != MSYS_UINT_MAX );

  //===== BI PREDICTION =====
  if( bBiPredOk && !bBiPred8x8Disable && rcRefFrameList0.getActive() && rcRefFrameList1.getActive() && uiMaxNumMv >= 8 )
  {
    //----- initialize with forward and backward estimation -----
    iRefIdxBi [0] = iRefIdx [0];
    iRefIdxBi [1] = iRefIdx [1];
    bBLPredBi [0] = bBLPred [0];
    bBLPredBi [1] = bBLPred [1];

    memcpy( cMvBi,      cMv,      2*4*sizeof(Mv ) );

    cMvPredBi [0][0]    = cMvPred [0][iRefIdx[0]][0];
    cMvPredBi [0][1]    = cMvPred [0][iRefIdx[0]][1];
    cMvPredBi [0][2]    = cMvPred [0][iRefIdx[0]][2];
    cMvPredBi [0][3]    = cMvPred [0][iRefIdx[0]][3];
    cMvPredBi [1][0]    = cMvPred [1][iRefIdx[1]][0];
    cMvPredBi [1][1]    = cMvPred [1][iRefIdx[1]][1];
    cMvPredBi [1][2]    = cMvPred [1][iRefIdx[1]][2];
    cMvPredBi [1][3]    = cMvPred [1][iRefIdx[1]][3];
    UInt  uiMotBits[2]  = { uiBits[0] - uiBlkBits[0], uiBits[1] - uiBlkBits[1] };
    uiBits[2]           = uiBlkBits[2] + uiMotBits[0] + uiMotBits[1];

    if( ! uiNumMaxIter )
    {
      uiNumMaxIter      = 1;
      uiIterSearchRange = 0;
    }

    //----- iterative search -----
    for( UInt uiIter = 0; uiIter < uiNumMaxIter; uiIter++ )
    {
      Bool          bChanged        = false;
      UInt          uiDir           = uiIter % 2;
      ListIdx       eListIdx        = ListIdx( uiDir );
      RefFrameList& rcRefFrameList  = ( uiDir ? rcRefFrameList1 : rcRefFrameList0 );
      BSParams      cBSParams;
      cBSParams.pcAltRefFrame       = ( uiDir ? rcRefFrameList0 : rcRefFrameList1 )[ iRefIdxBi[ 1 - uiDir ] ];
      cBSParams.pcAltRefPelData     = &cYuvMbBuffer[1-uiDir];
      cBSParams.uiL1Search          = uiDir;
      cBSParams.apcWeight[LIST_0]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_0, iRefIdxBi[LIST_0], rpcMbTempData->getFieldFlag() );
      cBSParams.apcWeight[LIST_1]   = &rpcMbTempData->getMbDataAccess().getSH().getPredWeight( LIST_1, iRefIdxBi[LIST_1], rpcMbTempData->getFieldFlag() );

      for( iRefIdxTest = 1; iRefIdxTest <= (Int)rcRefFrameList.getActive(); iRefIdxTest++ )
      {
        Mv  cMvPredTest[4];
        rpcMbTempData->getMbMotionData( eListIdx ).setRefIdx( iRefIdxTest, eParIdx8x8 );
        pcRefFrame  = rcRefFrameList[iRefIdxTest];

        if( !pcMbDataAccessBase || !rpcMbTempData->getMbDataAccess().getSH().getDefaultMotionPredictionFlag() )
        {
          uiBitsTest  = 0;
          uiCostTest  = 0;

          for( UInt uiBlk = 0; uiBlk < 4; uiBlk++ )
          {
            SParIdx4x4  eSubParIdx  = aeParIdx4x4[ uiBlk ];
            UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 2 ? 0 : uiBlkBits[2] + uiMotBits[1-uiDir] + getRefIdxBits( iRefIdxTest, rcRefFrameList ) );
            UInt        uiTmpCost;

            rpcMbTempData->getMbDataAccess().getMvPredictor     ( cMvPredTest[uiBlk], iRefIdxTest,
                                                                  eListIdx, eParIdx8x8, eSubParIdx );
            RNOK( m_pcMotionEstimation->estimateBlockWithStart  ( *rpcMbTempData, *pcRefFrame,
                                                                  cMvLastEst[uiDir][iRefIdxTest][uiBlk],
                                                                  cMvPredTest                   [uiBlk],
                                                                  uiTmpBits, uiTmpCost,
                                                                  eParIdx8x8+eSubParIdx, BLK_4x4, 
                                                                  uiIterSearchRange, 0, &cBSParams ) );
            RNOK( m_pcMotionEstimation->compensateBlock         ( &cTmpYuvMbBuffer,
                                                                  eParIdx8x8+eSubParIdx, BLK_4x4 ) );
            rpcMbTempData->getMbMotionData( eListIdx ).setAllMv ( cMvLastEst[uiDir][iRefIdxTest][uiBlk], eParIdx8x8, eSubParIdx );

            uiBitsTest += uiTmpBits;
            uiCostTest += uiTmpCost;
          }

          if( uiCostTest < uiCost[2] )
          {
            bChanged              = true;
            bBLPredBi [uiDir]     = false;
            iRefIdxBi [uiDir]     = iRefIdxTest;
            cMvBi     [uiDir][0]  = cMvLastEst[uiDir][iRefIdxTest][0];
            cMvBi     [uiDir][1]  = cMvLastEst[uiDir][iRefIdxTest][1];
            cMvBi     [uiDir][2]  = cMvLastEst[uiDir][iRefIdxTest][2];
            cMvBi     [uiDir][3]  = cMvLastEst[uiDir][iRefIdxTest][3];
            cMvPredBi [uiDir][0]  = cMvPredTest                   [0];
            cMvPredBi [uiDir][1]  = cMvPredTest                   [1];
            cMvPredBi [uiDir][2]  = cMvPredTest                   [2];
            cMvPredBi [uiDir][3]  = cMvPredTest                   [3];
            uiMotBits [uiDir]     = uiBitsTest - uiBlkBits[2] - uiMotBits[1-uiDir];
            uiBits    [2]         = uiBitsTest;
            uiCost    [2]         = uiCostTest;

            cYuvMbBuffer[uiDir].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
          }
        }

        if( iRefIdxTest == iBLRefIdx[uiDir] )
        {
          uiBitsTest  = 0;
          uiCostTest  = 0;
          for( UInt uiBlk = 0; uiBlk < 4; uiBlk++ )
          {
            SParIdx4x4  eSubParIdx  = aeParIdx4x4[ uiBlk ];
            UInt        uiTmpBits   = ( uiBlk || uiBasePredType == 2 ? 0 : uiBlkBits[2] + uiMotBits[1-uiDir] );
            UInt        uiTmpCost;
            RNOK( m_pcMotionEstimation->estimateBlockWithStart  ( *rpcMbTempData, *pcRefFrame,
                                                                  cBLMvLastEst[uiDir][uiBlk],
                                                                  cBLMvPred   [uiDir][uiBlk],
                                                                  uiTmpBits, uiTmpCost,
                                                                  eParIdx8x8+eSubParIdx, BLK_4x4, 
                                                                  uiIterSearchRange, 0, &cBSParams ) );
            RNOK( m_pcMotionEstimation->compensateBlock         ( &cTmpYuvMbBuffer,
                                                                  eParIdx8x8+eSubParIdx, BLK_4x4 ) );
            rpcMbTempData->getMbMotionData( eListIdx ).setAllMv ( cBLMvLastEst[uiDir][uiBlk], eParIdx8x8, eSubParIdx );
            uiBitsTest += uiTmpBits;
            uiCostTest += uiTmpCost;
          }

          if( uiCostTest < uiCost[2] )
          {
            bChanged              = true;
            bBLPredBi [uiDir]     = true;
            iRefIdxBi [uiDir]     = iRefIdxTest;
            cMvBi     [uiDir][0]  = cBLMvLastEst[uiDir][0];
            cMvBi     [uiDir][1]  = cBLMvLastEst[uiDir][1];
            cMvBi     [uiDir][2]  = cBLMvLastEst[uiDir][2];
            cMvBi     [uiDir][3]  = cBLMvLastEst[uiDir][3];
            uiMotBits [uiDir]     = uiBitsTest - uiBlkBits[2] - uiMotBits[1-uiDir];
            uiBits    [2]         = uiBitsTest;
            uiCost    [2]         = uiCostTest;

            cYuvMbBuffer[uiDir].loadLuma( cTmpYuvMbBuffer, B8x8Idx( ePar8x8 ) );
          }
        }
      }

      if( ! bChanged )
      {
        break;
      }
    }
  }


  //===== chose parameters =====
  if( uiCost[2] <= uiCost[0] && uiCost[2] <= uiCost[1] )
  {
    //----- bi-directional prediction -----
    fRdCost         = uiCost    [2];
    uiSubMbBits     = uiBits    [2];
    iRefIdx [0]     = iRefIdxBi [0];
    iRefIdx [1]     = iRefIdxBi [1];
    bBLPred [0]     = bBLPredBi [0];
    bBLPred [1]     = bBLPredBi [1];
    if( bBLPred[0] )
    {
      cMvPredBi[0][0] = cBLMvPred[0][0];
      cMvPredBi[0][1] = cBLMvPred[0][1];
      cMvPredBi[0][2] = cBLMvPred[0][2];
      cMvPredBi[0][3] = cBLMvPred[0][3];
    }
    if( bBLPred[1] )
    {
      cMvPredBi[1][0] = cBLMvPred[1][0];
      cMvPredBi[1][1] = cBLMvPred[1][1];
      cMvPredBi[1][2] = cBLMvPred[1][2];
      cMvPredBi[1][3] = cBLMvPred[1][3];
    }
    cMv     [0][0]  = cMvBi     [0][0];
    cMv     [0][1]  = cMvBi     [0][1];
    cMv     [0][2]  = cMvBi     [0][2];
    cMv     [0][3]  = cMvBi     [0][3];
    cMv     [1][0]  = cMvBi     [1][0];
    cMv     [1][1]  = cMvBi     [1][1];
    cMv     [1][2]  = cMvBi     [1][2];
    cMv     [1][3]  = cMvBi     [1][3];
    cMvd    [0][0]  = cMv       [0][0]  - cMvPredBi[0][0];
    cMvd    [0][1]  = cMv       [0][1]  - cMvPredBi[0][1];
    cMvd    [0][2]  = cMv       [0][2]  - cMvPredBi[0][2];
    cMvd    [0][3]  = cMv       [0][3]  - cMvPredBi[0][3];
    cMvd    [1][0]  = cMv       [1][0]  - cMvPredBi[1][0];
    cMvd    [1][1]  = cMv       [1][1]  - cMvPredBi[1][1];
    cMvd    [1][2]  = cMv       [1][2]  - cMvPredBi[1][2];
    cMvd    [1][3]  = cMv       [1][3]  - cMvPredBi[1][3];
  }
  else if( uiCost[0] <= uiCost[1] )
  {
    //----- list 0 prediction -----
    fRdCost         = uiCost    [0];
    uiSubMbBits     = uiBits    [0];
    iRefIdx [1]     = BLOCK_NOT_PREDICTED;
    bBLPred [1]     = false;
    if( bBLPred[0] )
    {
      cMvPred[0][iRefIdx[0]][0] = cBLMvPred[0][0];
      cMvPred[0][iRefIdx[0]][1] = cBLMvPred[0][1];
      cMvPred[0][iRefIdx[0]][2] = cBLMvPred[0][2];
      cMvPred[0][iRefIdx[0]][3] = cBLMvPred[0][3];
    }
    cMv     [1][0]  = Mv::ZeroMv();
    cMv     [1][1]  = Mv::ZeroMv();
    cMv     [1][2]  = Mv::ZeroMv();
    cMv     [1][3]  = Mv::ZeroMv();
    cMvd    [0][0]  = cMv[0][0] - cMvPred[0][iRefIdx[0]][0];
    cMvd    [0][1]  = cMv[0][1] - cMvPred[0][iRefIdx[0]][1];
    cMvd    [0][2]  = cMv[0][2] - cMvPred[0][iRefIdx[0]][2];
    cMvd    [0][3]  = cMv[0][3] - cMvPred[0][iRefIdx[0]][3];
  }
  else
  {
    //----- list 1 prediction -----
    fRdCost         = uiCost    [1];
    uiSubMbBits     = uiBits    [1];
    iRefIdx [0]     = BLOCK_NOT_PREDICTED;
    bBLPred [0]     = false;
    if( bBLPred[1] )
    {
      cMvPred[1][iRefIdx[1]][0] = cBLMvPred[1][0];
      cMvPred[1][iRefIdx[1]][1] = cBLMvPred[1][1];
      cMvPred[1][iRefIdx[1]][2] = cBLMvPred[1][2];
      cMvPred[1][iRefIdx[1]][3] = cBLMvPred[1][3];
    }
    cMv     [0][0]  = Mv::ZeroMv();
    cMv     [0][1]  = Mv::ZeroMv();
    cMv     [0][2]  = Mv::ZeroMv();
    cMv     [0][3]  = Mv::ZeroMv();
    cMvd    [1][0]  = cMv[1][0] - cMvPred[1][iRefIdx[1]][0];
    cMvd    [1][1]  = cMv[1][1] - cMvPred[1][iRefIdx[1]][1];
    cMvd    [1][2]  = cMv[1][2] - cMvPred[1][iRefIdx[1]][2];
    cMvd    [1][3]  = cMv[1][3] - cMvPred[1][iRefIdx[1]][3];
  }


  //===== set parameters and compare =====
  rpcMbTempData->rdCost() = fRdCost;
  rpcMbTempData->getMbMotionData( LIST_0 ).setRefIdx( iRefIdx [0],    eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0][0], eParIdx8x8, SPART_4x4_0 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0][1], eParIdx8x8, SPART_4x4_1 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0][2], eParIdx8x8, SPART_4x4_2 );
  rpcMbTempData->getMbMotionData( LIST_0 ).setAllMv ( cMv     [0][3], eParIdx8x8, SPART_4x4_3 );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0][0], eParIdx8x8, SPART_4x4_0 );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0][1], eParIdx8x8, SPART_4x4_1 );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0][2], eParIdx8x8, SPART_4x4_2 );
  rpcMbTempData->getMbMvdData   ( LIST_0 ).setAllMv ( cMvd    [0][3], eParIdx8x8, SPART_4x4_3 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setRefIdx( iRefIdx [1],    eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1][0], eParIdx8x8, SPART_4x4_0 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1][1], eParIdx8x8, SPART_4x4_1 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1][2], eParIdx8x8, SPART_4x4_2 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setAllMv ( cMv     [1][3], eParIdx8x8, SPART_4x4_3 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1][0], eParIdx8x8, SPART_4x4_0 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1][1], eParIdx8x8, SPART_4x4_1 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1][2], eParIdx8x8, SPART_4x4_2 );
  rpcMbTempData->getMbMvdData   ( LIST_1 ).setAllMv ( cMvd    [1][3], eParIdx8x8, SPART_4x4_3 );

  rpcMbTempData->getMbMotionData( LIST_0 ).setMotPredFlag( bBLPred[0], eParIdx8x8 );
  rpcMbTempData->getMbMotionData( LIST_1 ).setMotPredFlag( bBLPred[1], eParIdx8x8 );
  ROT( bBLPred[0] && iRefIdx[0] != iBLRefIdx[0] );
  ROT( bBLPred[1] && iRefIdx[1] != iBLRefIdx[1] );

  RNOK( xSetRdCostInterSubMb( *rpcMbTempData, rcRefListStruct, B8x8Idx( ePar8x8 ), false, uiSubMbBits, bLowComplexMbEnable ) );


  //JVT-R057 LA-RDO{
  if(m_bLARDOEnable)
  {
	  MbDataAccess&   rcMbDataAccess  = rpcMbTempData->getMbDataAccess();
	  Int distortion1=0,distortion2=0,distortion=0;
	  for( Int n = 0; n <4; n++)
	  {
		  iRefIdx [0]=rpcMbTempData->getMbMotionData(LIST_0).getRefIdx(eParIdx8x8);
		  iRefIdx [1]=rpcMbTempData->getMbMotionData(LIST_1).getRefIdx(eParIdx8x8);
		  Frame* pcRefFrame0 = ( iRefIdx [0] > 0 ? rcRefListStruct.acRefFrameListMC[0][ iRefIdx [0] ] : NULL );
		  Frame* pcRefFrame1 = ( iRefIdx [1] > 0 ? rcRefListStruct.acRefFrameListMC[1][ iRefIdx [1] ] : NULL );
		  Int iMvX;
		  Int iMvY;

		  if(pcRefFrame0)
		  {
			  iMvX=cMv[LIST_0][n].getHor();
			  iMvY=cMv[LIST_0][n].getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame0,&distortion1,iMvX,iMvY,(eParIdx8x8+aeParIdx4x4[n])%4,(eParIdx8x8+aeParIdx4x4 [n])/4,1,1);
		  }
		  if(pcRefFrame1)
		  {
			  iMvX=cMv[LIST_1][n].getHor();
			  iMvY=cMv[LIST_1][n].getVer();
			  getChannelDistortion(rcMbDataAccess,*pcRefFrame1,&distortion2,iMvX,iMvY,(eParIdx8x8+aeParIdx4x4[n])%4,(eParIdx8x8+aeParIdx4x4 [n])/4,1,1);
			  if(pcRefFrame0)
				  distortion1=(Int)(m_dWr0*distortion1+m_dWr0*distortion2);
			  else
				  distortion1=distortion2;
		  }
		  distortion+=distortion1;
	  }
	  rpcMbTempData->rdCost()+=distortion;
  }
  //JVT-R057 LA-RDO}

  RNOK( xCheckBestEstimation(  rpcMbTempData, rpcMbBestData ) );

  return Err::m_nOK;
}




//TMM_WP
ErrVal MbEncoder::getPredWeights( SliceHeader& rcSH, ListIdx eLstIdx,
                                  Double(*pafWeight)[3], Frame* pOrgFrame,
                                  RefFrameList& rcRefFrameListX)
{
  YuvPicBuffer *pcOrgPicBuffer;
  YuvPicBuffer *pcRefPicBuffer;
  Frame* pRefFrame;

  pcOrgPicBuffer = pOrgFrame->getFullPelYuvBuffer();

  Int iRefPic = 0;

  for( iRefPic = 0; iRefPic < (Int)rcRefFrameListX.getActive(); iRefPic++ )
  {
      pRefFrame = rcRefFrameListX.getEntry(iRefPic);
      pcRefPicBuffer = pRefFrame->getFullPelYuvBuffer();

      m_pcXDistortion->getLumaWeight(  pcOrgPicBuffer, pcRefPicBuffer, pafWeight[iRefPic][0], rcSH.getLumaLog2WeightDenom() );
      m_pcXDistortion->getChromaWeight(pcOrgPicBuffer, pcRefPicBuffer, pafWeight[iRefPic][1], rcSH.getChromaLog2WeightDenom(), true );
      m_pcXDistortion->getChromaWeight(pcOrgPicBuffer, pcRefPicBuffer, pafWeight[iRefPic][2], rcSH.getChromaLog2WeightDenom(), false );

//      printf( "\n%2.4f  %2.4f %2.4f", pafWeight[iRefPic][0],pafWeight[iRefPic][1],pafWeight[iRefPic][2]);
  }

  return Err::m_nOK;
}

ErrVal MbEncoder::getPredOffsets( SliceHeader& rcSH, ListIdx eLstIdx,
                                  Double(*pafOffsets)[3], Frame* pOrgFrame,
                                  RefFrameList& rcRefFrameListX)
{
  YuvPicBuffer *pcOrgPicBuffer;
  YuvPicBuffer *pcRefPicBuffer;
  Frame* pRefFrame;

  pcOrgPicBuffer = pOrgFrame->getFullPelYuvBuffer();

  Int iRefPic = 0;

  for( iRefPic = 0; iRefPic < (Int)rcRefFrameListX.getActive(); iRefPic++ )
  {
      pRefFrame = rcRefFrameListX.getEntry(iRefPic);
      pcRefPicBuffer = pRefFrame->getFullPelYuvBuffer();

      m_pcXDistortion->getLumaOffsets(  pcOrgPicBuffer, pcRefPicBuffer, pafOffsets[iRefPic][0] );
      m_pcXDistortion->getChromaOffsets(pcOrgPicBuffer, pcRefPicBuffer, pafOffsets[iRefPic][1], true );
      m_pcXDistortion->getChromaOffsets(pcOrgPicBuffer, pcRefPicBuffer, pafOffsets[iRefPic][2], false );

//      printf( "\nOffsets:%2.4f  %2.4f %2.4f\n", pafOffsets[iRefPic][0],pafOffsets[iRefPic][1],pafOffsets[iRefPic][2]);
  }

  return Err::m_nOK;
}
//TMM_WP



//JVT-R057 LA-RDO{
Int
MbEncoder::GetEC_REC(YuvPicBuffer* pPic1,
                     YuvPicBuffer* pPic2,
                     Int              blockX,
                     Int              blockY)
{

	XPel* pS1,*pS2;
	Int   iStride = pPic1->getLStride();
	Int uiDiff;
	UInt uiSSD;
  Int j, i;

	uiSSD=0;
	pS1=pPic1->getMbLumAddr();
	pS2=pPic2->getMbLumAddr();
	for(j=blockY*4;j<blockY*4+4;j++)
	{
		for( i=blockX*4;i<blockX*4+4;i++)
		{
			uiDiff=pS1[j*iStride+i]-pS2[j*iStride+i];
			uiSSD=uiSSD+uiDiff*uiDiff;
		}
	}
	pS1=pPic1->getMbCbAddr();
	pS2=pPic2->getMbCbAddr();
	for( j=blockY*2;j<blockY*2+2;j++)
	{
		for( i=blockX*2;i<blockX*2+2;i++)
		{
			uiDiff=pS1[j*(iStride/2)+i]-pS2[j*(iStride/2)+i];
			uiSSD=uiSSD+uiDiff*uiDiff;
		}
	}
	pS1=pPic1->getMbCrAddr();
	pS2=pPic2->getMbCrAddr();
	for( j=blockY*2;j<blockY*2+2;j++)
	{
		for( i=blockX*2;i<blockX*2+2;i++)
		{
			uiDiff=pS1[j*(iStride/2)+i]-pS2[j*(iStride/2)+i];
			uiSSD=uiSSD+uiDiff*uiDiff;
		}
	}
	return uiSSD;
}

Void
MbEncoder::getChannelDistortion(MbDataAccess&   rcMbDataAccess,
								Frame&       rcRefFrame,
								                Int             *distortion,
								                Int             iMvX,
								                Int             iMvY,
								                Int             startX,
								                Int             startY,
								                Int             blockX,
								                Int             blockY,
								                Bool            bSpatial)
{
#define MBK_SIZE 16
#define BLK_PER_MB 4
#define BLK_SIZE 4

	Int blkIdxX, blkIdxY;
	Int i0, j0;
	Int i1, j1;
	Int i2, j2;
	Int k0, l0;
	Int picWidth;
	Int picHeight;
	Int mbIdxRef;
	Int mbkPerLine;
	UInt *pDistortion;

	distortion[0] = 0;


	YuvPicBuffer* pTemp;
	pTemp=rcRefFrame.getFullPelYuvBuffer();

	picWidth   = pTemp->getLWidth();
	mbkPerLine = picWidth / MBK_SIZE;


	picHeight=pTemp->getLHeight();



	// 1:    (1-p) * Dc(n-1, j)
	for (blkIdxY = startY; blkIdxY <startY+blockY; blkIdxY += 1)
	{
		for (blkIdxX = startX; blkIdxX < startX+blockX; blkIdxX += 1)
		{

			// the starting position of current block in pixel: k0, l0
			k0 = (rcMbDataAccess.getMbX() * BLK_PER_MB + blkIdxX) * BLK_SIZE;
			l0 = (rcMbDataAccess.getMbY() * BLK_PER_MB + blkIdxY) * BLK_SIZE;

			// Absolute motion vector coordinates of the macroblock
			pDistortion = rcRefFrame.getChannelDistortion();

			i0 = k0 * 4+iMvX ;
			j0 = l0 * 4+iMvY ;

			// the starting position of the ref block in pixel: i0, j0
			i0 = i0 / 4;
			j0 = j0 / 4;
			if (i0 < 0) i0 = 0;
			if (j0 < 0) j0 = 0;
			if (i0 >= picWidth) i0 = picWidth - 1;
			if (j0 >= picHeight) j0 = picHeight - 1;

			// calculate the distortion here:
			for (j1 = j0; j1 < j0 + 4; j1++) {
				for (i1 = i0; i1 < i0 + 4; i1++) {
					i2 = i1;
					j2 = j1;

					if (i2 >= picWidth) i2 = picWidth - 1;
					if (j2 >= picHeight) j2 = picHeight - 1;
					//Bug_Fix JVT-R057 0806{
					//if(bSpatial)
					//	mbIdxRef = (j2 / MBK_SIZE/2*4) * (mbkPerLine/2*4) + i2 / MBK_SIZE/2*4;
					//else
					//	mbIdxRef  = (j2 / MBK_SIZE*4) * (mbkPerLine*4) + i2 / MBK_SIZE*4;
					mbIdxRef  = (j2 / 4) * (mbkPerLine*4) + i2 / 4;
					//Bug_Fix JVT-R057 0806}
					distortion[0] += pDistortion[mbIdxRef];  //  / 256.0
				}
			}

		}
	}
	distortion[0] = distortion[0] >> 4; //  / 256
}

//JVT-R057 LA-RDO}

// JVT-W043 {
unsigned int MbEncoder::jsvmCalcMAD( IntMbTempData*&   rpcMbBestData, MbDataAccess&  rcMbDataAccess )
{
  UInt  uiDist   = 0;
  UInt  uiDelta  = 1;
  Int   n, m;

  IntMbTempData *rpcMbTempData = new IntMbTempData;
  rpcMbTempData->init( rcMbDataAccess );
  rpcMbTempData->loadLuma   ( *m_pcIntOrgMbPelData );
  rpcMbTempData->loadChroma ( *m_pcIntOrgMbPelData );

  XPel* pucDst        = rpcMbBestData->getMbLumAddr();
  XPel* pucSrc        = rpcMbTempData->getMbLumAddr();
  Int   iStride       = rpcMbTempData->getLStride();
  Int   iDeltaXStride = uiDelta * iStride;
  AOF( iStride == rpcMbBestData->getLStride() );

  for( n = 0; n < 16; n += uiDelta )
  {
    for( m = 0; m < 16; m += uiDelta )
    {
      uiDist += abs( pucSrc[m] - pucDst[m] );
    }
    pucSrc += iDeltaXStride;
    pucDst += iDeltaXStride;
  }
  delete rpcMbTempData;
  return uiDist;
}
// JVT-W043 }

Void
MbEncoder::reCalcBlock4x4( IntMbTempData& rcMbTempData, LumaIdx c4x4Idx )
{
  TCoeff* pcCoeff     = rcMbTempData.get( c4x4Idx );
  TCoeff* piCoeffBase = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( c4x4Idx );

  // add the base layer dequantized coeff
  UInt ui=0;
  for( ui=0; ui<16; ui++ )
  {
    pcCoeff[ui] = piCoeffBase[ui].getLevel();
    pcCoeff[ui].setLevel(piCoeffBase[ui].getLevel());
  }

  m_pcTransform->invTransform4x4Blk( rcMbTempData.getYBlk( c4x4Idx ), rcMbTempData.getLStride(), pcCoeff );

  // only clear the coefficients
  for( ui=0; ui<16; ui++ )
  {
    pcCoeff[ui].setCoeff(0);
  }
}

Void
MbEncoder::reCalcBlock8x8(IntMbTempData& rcMbTempData, B8x8Idx c8x8Idx, Int is4x4)
{
  if( !is4x4 ) // 8x8 mode
  {
    TCoeff* pcCoeff     = rcMbTempData.get8x8( c8x8Idx );
    TCoeff* piCoeffBase = rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get8x8( c8x8Idx );

    UInt ui=0;
	  for( ui=0; ui<64; ui++ )
    {
      pcCoeff[ui] = piCoeffBase[ui].getLevel();
      pcCoeff[ui].setLevel(piCoeffBase[ui].getLevel());
    }

    m_pcTransform->invTransform8x8Blk( rcMbTempData.getYBlk( c8x8Idx ),  rcMbTempData.getLStride(), pcCoeff );

    // only clear the coefficients
    for( ui=0; ui<64; ui++ )
    {
      pcCoeff[ui].setCoeff(0);
    }
  }
  else // 4x4 mode
  {
    for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
    {
      reCalcBlock4x4( rcMbTempData, cIdx );
    }
  }
}

ErrVal
MbEncoder::reCalcBlock4x4Rewrite( IntMbTempData& rcMbTempData, LumaIdx c4x4Idx )
{
  TCoeff* pcCoeff   = rcMbTempData.get( c4x4Idx );
  UInt    uiAbsSum  = 0;

  RNOK( m_pcTransform->predict4x4Blk( pcCoeff,
                                      rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( c4x4Idx ),
                                      rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
                                      uiAbsSum ) );

  for( unsigned int i=0; i<16; i++ )
    pcCoeff[i] = -pcCoeff[i];

  Bool          bIntra    = rcMbTempData.getMbDataAccess().getMbData().isIntra();
  UInt          uiScaleId = ( bIntra ? 0 : 3 );
  const UChar*  pucScale  = rcMbTempData.getMbDataAccess().getSH().getScalingMatrix( uiScaleId );
  xScale4x4Block( pcCoeff, pucScale, 0, m_pcTransform->getLumaQp() );

  m_pcTransform->invTransform4x4Blk( rcMbTempData.getYBlk( c4x4Idx ), rcMbTempData.getLStride(), pcCoeff );

  // only clear the coefficients
  for( UInt ui=0; ui<16; ui++ )
  {
    pcCoeff[ui].setLevel(pcCoeff[ui].getCoeff());
    pcCoeff[ui].setCoeff(0);
  }
  return Err::m_nOK;
}

ErrVal
MbEncoder::reCalcBlock8x8Rewrite(IntMbTempData& rcMbTempData, B8x8Idx c8x8Idx, Int is4x4)
{
  if( !is4x4 ) // 8x8 mode
  {
    TCoeff* pcCoeff   = rcMbTempData.get8x8( c8x8Idx );
    UInt    uiAbsSum  = 0;

    RNOK( m_pcTransform->predict8x8Blk( pcCoeff,
      rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get8x8( c8x8Idx ),
      rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
      uiAbsSum ) );

    for( unsigned int i=0; i<64; i++ )
      pcCoeff[i] = -pcCoeff[i];

    Bool          bIntra    = rcMbTempData.getMbDataAccess().getMbData().isIntra();
    UInt          uiScaleId = ( bIntra ? 6 : 7 );
    const UChar*  pucScale  = rcMbTempData.getMbDataAccess().getSH().getScalingMatrix( uiScaleId );
    xScale8x8Block( pcCoeff, pucScale, m_pcTransform->getLumaQp() );

    m_pcTransform->invTransform8x8Blk( rcMbTempData.getYBlk( c8x8Idx ), rcMbTempData.getLStride(), pcCoeff );

    // only clear the coefficients
    for( UInt ui=0; ui<64; ui++ )
    {
      pcCoeff[ui].setLevel(pcCoeff[ui].getCoeff());
      pcCoeff[ui].setCoeff(0);
    }
  }
  else  // 4x4 mode
  {
    for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
    {
      RNOK( reCalcBlock4x4Rewrite( rcMbTempData, cIdx ) );
    }
  }

  return Err::m_nOK;
}

ErrVal
MbEncoder::reCalcBlock16x16Rewrite( IntMbTempData& rcMbTempData )
{
  TCoeff* pcCoeff   = rcMbTempData.get( B4x4Idx(0) );
  UInt    uiDcAbs   = 0;
  UInt    uiAcAbs   = 0;

  RNOK( m_pcTransform->predictMb16x16( pcCoeff,
        rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbTCoeffs().get( B4x4Idx(0) ),
        rcMbTempData.getMbDataAccess().getMbDataAccessBase()->getMbData().getQp(),
        uiDcAbs, uiAcAbs ) );

  for( UInt i=0; i<256; i++ )
    pcCoeff[i] = -pcCoeff[i];

  // inverse 16x16 trafo
  {
    const Int aaiDequantDcCoef[6] = { 10, 11, 13, 14, 16, 18 };
    const Int iQp = rcMbTempData.getQp();
    Int iQpScale = aaiDequantDcCoef[iQp%6] * 16;
    RNOK( m_pcTransform->invTransformDcCoeff( pcCoeff, iQpScale, iQp/6) );
  }

  const UChar*  pucScale  = rcMbTempData.getMbDataAccess().getSH().getScalingMatrix( 0 );

  for( B4x4Idx c4x4Idx; c4x4Idx.isLegal(); c4x4Idx++ )
  {
    TCoeff* pcBlkCoeff = rcMbTempData.get( c4x4Idx );
    xScale4x4Block( pcBlkCoeff, pucScale, 1, m_pcTransform->getLumaQp() );
    RNOK( m_pcTransform->invTransform4x4Blk( rcMbTempData.getYBlk( c4x4Idx ), rcMbTempData.getLStride(), pcBlkCoeff ) );
  }

  // only clear the coefficients
  for( UInt ui=0; ui<256; ui++ )
  {
    pcCoeff[ui].setLevel(pcCoeff[ui].getCoeff());
    pcCoeff[ui].setCoeff(0);
  }
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
