
#include "H264AVCDecoderLib.h"
#include "H264AVCCommonLib/Tables.h"


#include "H264AVCCommonLib/Transform.h"
#include "H264AVCCommonLib/IntraPrediction.h"
#include "H264AVCCommonLib/MotionCompensation.h"
#include "MbDecoder.h"



H264AVC_NAMESPACE_BEGIN



MbDecoder::MbDecoder()
: m_pcTransform         ( 0 )
, m_pcIntraPrediction   ( 0 )
, m_pcMotionCompensation( 0 )
, m_bInitDone           ( false )
{
}

MbDecoder::~MbDecoder()
{
}

ErrVal
MbDecoder::create( MbDecoder*& rpcMbDecoder )
{
  rpcMbDecoder = new MbDecoder;
  ROT( NULL == rpcMbDecoder );
  return Err::m_nOK;
}

ErrVal
MbDecoder::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal
MbDecoder::init( Transform*          pcTransform,
                 IntraPrediction*    pcIntraPrediction,
                 MotionCompensation* pcMotionCompensation )
{
  ROF( pcTransform );
  ROF( pcIntraPrediction );
  ROF( pcMotionCompensation );

  m_pcTransform           = pcTransform;
  m_pcIntraPrediction     = pcIntraPrediction;
  m_pcMotionCompensation  = pcMotionCompensation;
  m_bInitDone             = true;

  return Err::m_nOK;
}


ErrVal
MbDecoder::uninit()
{
  m_pcTransform           = 0;
  m_pcIntraPrediction     = 0;
  m_pcMotionCompensation  = 0;
  m_bInitDone             = false;
  return Err::m_nOK;
}

ErrVal
MbDecoder::xPredictionFromBaseLayer( MbDataAccess&  rcMbDataAccess,
                                     MbDataAccess*  pcMbDataAccessBase )
{
  MbData& rcMbData        = rcMbDataAccess.getMbData();
  Bool    bSNR            = pcMbDataAccessBase && ( rcMbDataAccess.getSH().getSCoeffResidualPredFlag() || rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag() );
  Bool    bFieldMismatch  = bSNR && ( rcMbDataAccess.getMbData().getFieldFlag() != pcMbDataAccessBase->getMbData().getFieldFlag() );

  if( rcMbData.getBLSkipFlag() )
  {
    ROF( pcMbDataAccessBase );
    ROT( bFieldMismatch );
    Bool bBLSkipFlag  = rcMbData.getBLSkipFlag();
    rcMbData.copyMotion( pcMbDataAccessBase->getMbData() );
    rcMbData.setBLSkipFlag( bBLSkipFlag );
    if( rcMbData.isIntra() )
    {
      rcMbData.setMbMode( INTRA_BL );
    }
  }
  else
  {
    for( ListIdx eListIdx = LIST_0; eListIdx <= LIST_1; eListIdx = ListIdx( eListIdx + 1 ) )
    {
      MbMotionData& rcMbMotionData = rcMbData.getMbMotionData( eListIdx );

      switch( rcMbData.getMbMode() )
      {
      case MODE_16x16:
        {
          if( rcMbData.isBlockFwdBwd( B_8x8_0, eListIdx ) && rcMbMotionData.getMotPredFlag() )
          {
            ROT( bFieldMismatch );
            ROF( pcMbDataAccessBase );
            rcMbMotionData.setRefIdx( pcMbDataAccessBase->getMbMotionData( eListIdx ).getRefIdx() );
          }
        }
        break;
      case MODE_16x8:
        {
          if( rcMbData.isBlockFwdBwd( B_8x8_0, eListIdx ) && rcMbMotionData.getMotPredFlag( PART_16x8_0 ) )
          {
            ROT( bFieldMismatch );
            ROF( pcMbDataAccessBase );
            rcMbMotionData.setRefIdx( pcMbDataAccessBase->getMbMotionData( eListIdx ).getRefIdx( PART_16x8_0 ), PART_16x8_0 );
          }
          if( rcMbData.isBlockFwdBwd( B_8x8_2, eListIdx ) && rcMbMotionData.getMotPredFlag( PART_16x8_1 ) )
          {
            ROT( bFieldMismatch );
            ROF( pcMbDataAccessBase );
            rcMbMotionData.setRefIdx( pcMbDataAccessBase->getMbMotionData( eListIdx ).getRefIdx( PART_16x8_1 ), PART_16x8_1 );
          }
        }
        break;
      case MODE_8x16:
        {
          if( rcMbData.isBlockFwdBwd( B_8x8_0, eListIdx ) && rcMbMotionData.getMotPredFlag( PART_8x16_0 ) )
          {
            ROT( bFieldMismatch );
            ROF( pcMbDataAccessBase );
            rcMbMotionData.setRefIdx( pcMbDataAccessBase->getMbMotionData( eListIdx ).getRefIdx( PART_8x16_0 ), PART_8x16_0 );
          }
          if( rcMbData.isBlockFwdBwd( B_8x8_1, eListIdx ) && rcMbMotionData.getMotPredFlag( PART_8x16_1 ) )
          {
            ROT( bFieldMismatch );
            ROF( pcMbDataAccessBase );
            rcMbMotionData.setRefIdx( pcMbDataAccessBase->getMbMotionData( eListIdx ).getRefIdx( PART_8x16_1 ), PART_8x16_1 );
          }
        }
        break;
      case MODE_8x8:
      case MODE_8x8ref0:
        {
          for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
          {
            if( rcMbData.getBlkMode( c8x8Idx.b8x8Index() ) != BLK_SKIP  &&
                rcMbData.isBlockFwdBwd( c8x8Idx.b8x8Index(), eListIdx ) && rcMbMotionData.getMotPredFlag( c8x8Idx.b8x8() ) )
            {
              ROT( bFieldMismatch );
              ROF( pcMbDataAccessBase );
              rcMbMotionData.setRefIdx( pcMbDataAccessBase->getMbMotionData( eListIdx ).getRefIdx( c8x8Idx.b8x8() ), c8x8Idx.b8x8() );
            }
          }
        }
        break;
      default:
        break;
      }
    }
  }

  //----- check coefficients for residual prediction ----
  if( bFieldMismatch && rcMbDataAccess.getMbData().getResidualPredFlag() )
  {
    if( rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag() )
    {
      ROF( pcMbDataAccessBase->getMbTCoeffs().allCoeffsZero() );
    }
    else
    {
      ROF( pcMbDataAccessBase->getMbTCoeffs().allLevelsZero() );
    }
  }

  return Err::m_nOK;
}


ErrVal
MbDecoder::decode( MbDataAccess&  rcMbDataAccess,
                   MbDataAccess*  pcMbDataAccessBase,
                   Frame*         pcFrame,
                   Frame*         pcResidualLF,
                   Frame*         pcResidualILPred,
                   Frame*         pcBaseLayer,
                   Frame*         pcBaseLayerResidual,
                   RefFrameList*  pcRefFrameList0,
                   RefFrameList*  pcRefFrameList1,
                   Bool           bReconstructAll )
{
  ROF( m_bInitDone );

  if( !rcMbDataAccess.getMbData().getInCropWindowFlag() )
  {
    pcMbDataAccessBase = 0;
  }

  RNOK( xPredictionFromBaseLayer( rcMbDataAccess, pcMbDataAccessBase ) );

  rcMbDataAccess.setMbDataAccessBase(pcMbDataAccessBase);

  YuvMbBuffer  cPredBuffer;  cPredBuffer.setAllSamplesToZero();

  //===== modify QP values (as specified in G.8.1.5.1.2) =====
  if( pcMbDataAccessBase && rcMbDataAccess.getMbData().getMbCbp() == 0
      && ( rcMbDataAccess.getSH().getSCoeffResidualPredFlag() || rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag() ) // SpatialResolutionChangeFlag == 0
      && ( rcMbDataAccess.getMbData().getMbMode() == INTRA_BL || rcMbDataAccess.getMbData().getResidualPredFlag() ) )
  {
    rcMbDataAccess.getMbData().setQp( pcMbDataAccessBase->getMbData().getQp() );
  }

  //===== infer 8x8 transform flag =====
  if( rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag() || rcMbDataAccess.getSH().getSCoeffResidualPredFlag() ) // SpatialResolutionChangeFlag == 0
  {
    if( ( rcMbDataAccess.getMbData().getResidualPredFlag() && ! rcMbDataAccess.getMbData().isIntra() && ! pcMbDataAccessBase->getMbData().isIntra() ) || ( rcMbDataAccess.getMbData().getMbMode() == INTRA_BL ) )
    { // must have same trafo8x8flag in this case
      if( ( rcMbDataAccess.getMbData().getMbCbp() & 0x0F ) == 0 )
      {
        rcMbDataAccess.getMbData().setTransformSize8x8( pcMbDataAccessBase->getMbData().isTransformSize8x8() );
      }
    }
  }

  //===== infer intra prediction for tcoeff_level_prediction_flag equal to 1 =====
  if( rcMbDataAccess.isTCoeffPred() && rcMbDataAccess.getMbData().getMbMode() == INTRA_BL )
  {
	  // Inherit the mode of the base block
	  rcMbDataAccess.getMbData().setMbMode( pcMbDataAccessBase->getMbData().getMbMode() );
	  // Inherit intra prediction modes
	  for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
		  rcMbDataAccess.getMbData().intraPredMode(cIdx) = pcMbDataAccessBase->getMbData().intraPredMode(cIdx);
    }
	  rcMbDataAccess.getMbData().setChromaPredMode( pcMbDataAccessBase->getMbData().getChromaPredMode() );
  }

  //==== infer IPCM mode ===
  if( rcMbDataAccess.getSH().getSCoeffResidualPredFlag() && rcMbDataAccess.getMbData().isIntraBL() && rcMbDataAccess.getMbData().getMbCbp() == 0 && pcMbDataAccessBase->getMbData().isPCM() )
  {
    rcMbDataAccess.getMbData().setMbMode( MODE_PCM );
    //--- copy levels ---
    TCoeff* pBase = pcMbDataAccessBase->getMbTCoeffs().getTCoeffBuffer();
    TCoeff* pCurr = rcMbDataAccess     .getMbTCoeffs().getTCoeffBuffer();
    for( UInt ui = 0; ui < 384; ui++, pBase++, pCurr++ )
    {
      pCurr->setLevel( pBase->getLevel() );
    }
  }

  //===== update transform coefficient levels =====
  if( rcMbDataAccess.isTCoeffPred() )
  {
    RNOK( xAddTCoeffs( rcMbDataAccess, *pcMbDataAccessBase ) );
  }

  //===== clear base residual =====
  if( pcBaseLayerResidual && ( rcMbDataAccess.getMbData().isIntra() || !rcMbDataAccess.getMbData().getResidualPredFlag() || rcMbDataAccess.isTCoeffPred() ) )
  {
    YuvPicBuffer* pcBaseResidualBuffer = pcBaseLayerResidual->getFullPelYuvBuffer();
    pcBaseResidualBuffer->clearCurrMb();
  }

  //===== scale coefficients =====
  RNOK( xScaleTCoeffs( rcMbDataAccess ) );

  if( rcMbDataAccess.getMbData().isIntra() )
  {
    //===== clear residual signal for intra macroblocks =====
    if( pcResidualLF || pcResidualILPred )
    {
      YuvMbBuffer  cYuvMbBuffer; 
      cYuvMbBuffer.setAllSamplesToZero();
      if( pcResidualLF )
      {
        RNOK( pcResidualLF    ->getFullPelYuvBuffer()->loadBuffer( &cYuvMbBuffer ) );
      }
      if( pcResidualILPred )
      {
        RNOK( pcResidualILPred->getFullPelYuvBuffer()->loadBuffer( &cYuvMbBuffer ) );
      }
    }

    if( rcMbDataAccess.getMbData().isPCM() )
    {
      //===== I_PCM mode =====
      YuvMbBuffer cRecBuffer;
      RNOK( xDecodeMbPCM( rcMbDataAccess, cRecBuffer ) );
      cPredBuffer.loadLuma  ( cRecBuffer );
      cPredBuffer.loadChroma( cRecBuffer );
      RNOK( pcFrame->getFullPelYuvBuffer()->loadBuffer( &cRecBuffer ) );
      rcMbDataAccess.getMbTCoeffs().copyPredictionFrom( cRecBuffer );
    }
    else if( rcMbDataAccess.getMbData().getMbMode() == INTRA_BL )
    {
      //===== I_BL mode =====
      RNOK( xDecodeMbIntraBL( rcMbDataAccess,  pcFrame->getFullPelYuvBuffer(),
                              cPredBuffer, pcBaseLayer->getFullPelYuvBuffer() ) );
    }
    else
    {
      m_pcIntraPrediction->setAvailableMaskMb( rcMbDataAccess.getAvailableMask() );
      YuvMbBuffer cRecBuffer;  cRecBuffer.loadIntraPredictors( pcFrame->getFullPelYuvBuffer() );

      if( rcMbDataAccess.getMbData().isIntra16x16() )
      {
        //===== I_16x16 mode ====
        RNOK( xDecodeMbIntra16x16( rcMbDataAccess, cRecBuffer, cPredBuffer ) );
      }
      else if( rcMbDataAccess.getMbData().isTransformSize8x8() )
      {
        //===== I_8x8 mode =====
        RNOK( xDecodeMbIntra8x8( rcMbDataAccess, cRecBuffer, cPredBuffer ) );
      }
      else
      {
        //===== I_4x4 mode =====
        RNOK( xDecodeMbIntra4x4( rcMbDataAccess, cRecBuffer, cPredBuffer ) );
      }
      RNOK( pcFrame->getFullPelYuvBuffer()->loadBuffer( &cRecBuffer ) );
      rcMbDataAccess.getMbTCoeffs().copyPredictionFrom( cRecBuffer );
    }
  }
  else
  {
    //===== motion-compensated modes =====
    RNOK( xDecodeMbInter( rcMbDataAccess, pcMbDataAccessBase,
                          cPredBuffer, pcFrame->getFullPelYuvBuffer(),
                          pcResidualLF, pcResidualILPred, pcBaseLayerResidual,
                          *pcRefFrameList0, *pcRefFrameList1, bReconstructAll, pcBaseLayer  ) );
  }

  RNOK( rcMbDataAccess.getMbMotionData( LIST_0 ).setRefPicIdcs( rcMbDataAccess.getSH().getRefFrameList( rcMbDataAccess.getMbPicType(), LIST_0 ) ) );
  RNOK( rcMbDataAccess.getMbMotionData( LIST_1 ).setRefPicIdcs( rcMbDataAccess.getSH().getRefFrameList( rcMbDataAccess.getMbPicType(), LIST_1 ) ) );

  return Err::m_nOK;
}


ErrVal
MbDecoder::calcMv( MbDataAccess& rcMbDataAccess,
                   MbDataAccess* pcMbDataAccessBaseMotion )
{
  if( rcMbDataAccess.getMbData().getBLSkipFlag() )
  {
    return Err::m_nOK;
  }

  if( rcMbDataAccess.getMbData().getMbMode() == INTRA_4X4 )
  {
    //----- intra prediction -----
    rcMbDataAccess.getMbMotionData( LIST_0 ).setRefIdx( BLOCK_NOT_PREDICTED );
    rcMbDataAccess.getMbMotionData( LIST_0 ).setAllMv ( Mv::ZeroMv() );
    rcMbDataAccess.getMbMotionData( LIST_1 ).setRefIdx( BLOCK_NOT_PREDICTED );
    rcMbDataAccess.getMbMotionData( LIST_1 ).setAllMv ( Mv::ZeroMv() );
  }
  else
  {
    if( rcMbDataAccess.getMbData().getMbMode() == MODE_8x8 || rcMbDataAccess.getMbData().getMbMode() == MODE_8x8ref0 )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        //----- motion compensated prediction -----
        RNOK( m_pcMotionCompensation->calcMvSubMb( c8x8Idx, rcMbDataAccess, pcMbDataAccessBaseMotion ) );
      }
    }
    else
    {
      //----- motion compensated prediction -----
      RNOK( m_pcMotionCompensation->calcMvMb( rcMbDataAccess, pcMbDataAccessBaseMotion ) );
    }
  }

  return Err::m_nOK;
}


ErrVal
MbDecoder::xDecodeMbPCM( MbDataAccess& rcMbDataAccess, YuvMbBuffer& rcYuvMbBuffer )
{
  TCoeff* pCoeff  = rcMbDataAccess.getMbTCoeffs().getTCoeffBuffer();
  XPel*   pucDest = rcYuvMbBuffer.getMbLumAddr();
  Int     iStride = rcYuvMbBuffer.getLStride();
  Int     n, m;

  for( n = 0; n < 16; n++ )
  {
    for( m = 0; m < 16; m++ )
    {
      pucDest[m] = pCoeff->getLevel();
      pCoeff++;
    }
    pucDest += iStride;
  }

  pucDest = rcYuvMbBuffer.getMbCbAddr();
  iStride = rcYuvMbBuffer.getCStride();

  for( n = 0; n < 8; n++ )
  {
    for( m = 0; m < 8; m++ )
    {
      pucDest[m] = pCoeff->getLevel();
      pCoeff++;
    }
    pucDest += iStride;
  }

  pucDest = rcYuvMbBuffer.getMbCrAddr();

  for( n = 0; n < 8; n++ )
  {
    for( m = 0; m < 8; m++ )
    {
      pucDest[m] = pCoeff->getLevel();
      pCoeff++;
    }
    pucDest += iStride;
  }

  return Err::m_nOK;
}


ErrVal
MbDecoder::xDecodeMbInter( MbDataAccess&  rcMbDataAccess,
                           MbDataAccess*  pcMbDataAccessBase,
                           YuvMbBuffer&   rcPredBuffer,
                           YuvPicBuffer*  pcRecYuvBuffer,
                           Frame*         pcResidualLF,
                           Frame*         pcResidualILPred,
                           Frame*         pcBaseResidual,
                           RefFrameList&  rcRefFrameList0,
                           RefFrameList&  rcRefFrameList1,
                           Bool           bReconstruct,
                           Frame*         pcBaseLayerRec )
{
  YuvMbBuffer      cYuvMbBuffer;         cYuvMbBuffer        .setAllSamplesToZero();
  YuvMbBuffer      cYuvMbBufferResidual; cYuvMbBufferResidual.setAllSamplesToZero();
  MbTransformCoeffs&  rcCoeffs        = m_cTCoeffs;
  Bool                bCalcMv         = false;
  Bool                bFaultTolerant  = true;

  //===== derive motion vectors =====
  calcMv( rcMbDataAccess, pcMbDataAccessBase );

#ifdef SHARP_AVC_REWRITE_OUTPUT
  return Err::m_nOK;
#endif

  //===== get prediction signal when full reconstruction is requested =====
  if( bReconstruct )
  {
    if( rcMbDataAccess.getMbData().getMbMode() == MODE_8x8 || rcMbDataAccess.getMbData().getMbMode() == MODE_8x8ref0 )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        //----- motion compensated prediction -----
        RNOK( m_pcMotionCompensation->compensateSubMb ( c8x8Idx,
                                                        rcMbDataAccess, rcRefFrameList0, rcRefFrameList1,
                                                        &cYuvMbBuffer, bCalcMv, bFaultTolerant ) );
      }
    }
    else
    {
      //----- motion compensated prediction -----
      RNOK(   m_pcMotionCompensation->compensateMb    ( rcMbDataAccess,
                                                        rcRefFrameList0,
                                                        rcRefFrameList1,
                                                        &cYuvMbBuffer,
                                                        bCalcMv ) );
    }
    if(pcBaseLayerRec)
    {
      RNOK(m_pcMotionCompensation->compensateMbBLSkipIntra(rcMbDataAccess, &cYuvMbBuffer, pcBaseLayerRec));
    }
    rcPredBuffer.loadLuma   ( cYuvMbBuffer );
    rcPredBuffer.loadChroma ( cYuvMbBuffer );
  }

  //===== add base layer residual =====
  if( rcMbDataAccess.isSCoeffPred() )
  {
    rcCoeffs.add( &pcMbDataAccessBase->getMbData().getMbTCoeffs() );
    rcMbDataAccess.getMbTCoeffs().copyFrom(rcCoeffs);     // store the sum of the coefficients and base layer coefficients
  }
  else if( rcMbDataAccess.isTCoeffPred() )
  {
    pcMbDataAccessBase->getMbData().getMbTCoeffs().clearLumaLevels();
  }

  //===== reconstruct residual signal by using transform coefficients ======
  m_pcTransform->setClipMode( false );
  if( rcMbDataAccess.getMbData().isTransformSize8x8() )
  {
    for( B8x8Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( m_pcTransform->invTransform8x8Blk( cYuvMbBufferResidual.getYBlk( cIdx ),
                                               cYuvMbBufferResidual.getLStride(),
                                               rcCoeffs.get8x8(cIdx) ) );
    }
  }
  else
  {
    for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( m_pcTransform->invTransform4x4Blk( cYuvMbBufferResidual.getYBlk( cIdx ),
                                               cYuvMbBufferResidual.getLStride(),
                                               rcCoeffs.get(cIdx) ) );
    }
  }

  UInt uiChromaCbp = rcMbDataAccess.getMbData().getCbpChroma4x4();
  RNOK( xDecodeChroma( rcMbDataAccess, cYuvMbBufferResidual, rcPredBuffer, uiChromaCbp, false ) );
  m_pcTransform->setClipMode( true );

  //===== get base layer residual =====
  YuvMbBuffer cResidual, cResidualILPred;
  if( rcMbDataAccess.getMbData().getResidualPredFlag() )
  {
    if( rcMbDataAccess.isSCoeffPred() )
    {
      rcMbDataAccess.getMbDataAccessBase()->getMbTCoeffs().copyPredictionTo( cResidual );
    }
    else
    {
      cResidual.loadBuffer( pcBaseResidual->getFullPelYuvBuffer() );
    }
  }
  else
  {
    cResidual.setAllSamplesToZero();
  }
  cResidualILPred.loadLuma   ( cResidual );
  cResidualILPred.loadChroma ( cResidual );
  if( bReconstruct && pcBaseLayerRec )
  {
    RNOK( m_pcMotionCompensation->updateMbBLSkipResidual( rcMbDataAccess, cResidual ) );
  }

  //===== store spatially predicted residual =====
  rcMbDataAccess.getMbTCoeffs().copyPredictionFrom( cResidualILPred );

  //===== add current residual =====
  cResidual      .addRes( cYuvMbBufferResidual );
  cResidualILPred.addRes( cYuvMbBufferResidual );

  //===== store residual =====
  if( pcResidualLF )
  {
    RNOK( pcResidualLF    ->getFullPelYuvBuffer()->loadBuffer( &cResidual ) );
  }
  if( pcResidualILPred )
  {
    RNOK( pcResidualILPred->getFullPelYuvBuffer()->loadBuffer( &cResidualILPred ) );
  }

  //===== reconstruct signal =====
  if( bReconstruct )
  {
    cYuvMbBuffer.addClip( cResidual );
  }

  //===== store reconstructed signal =====
  if( pcRecYuvBuffer )
  {
    RNOK( pcRecYuvBuffer->loadBuffer( &cYuvMbBuffer ) );
  }

  return Err::m_nOK;
}



ErrVal
MbDecoder::xDecodeMbIntra4x4( MbDataAccess&  rcMbDataAccess,
                              YuvMbBuffer&   cYuvMbBuffer,
                              YuvMbBuffer&   rcPredBuffer )
{
#ifndef SHARP_AVC_REWRITE_OUTPUT //JV: not clean at all -> to remove compilation warnings
	Int  iStride = cYuvMbBuffer.getLStride();
  MbTransformCoeffs& rcCoeffs = m_cTCoeffs;
#endif //JV: not clean at all -> to remove compilation warnings

  for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
  {
    if( !rcMbDataAccess.getMbData().getBLSkipFlag() || !rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag() )
      rcMbDataAccess.getMbData().intraPredMode( cIdx ) = rcMbDataAccess.decodeIntraPredMode( cIdx );
#ifndef SHARP_AVC_REWRITE_OUTPUT
	XPel* puc = cYuvMbBuffer.getYBlk( cIdx );

    UInt uiPredMode = rcMbDataAccess.getMbData().intraPredMode( cIdx );
    RNOK( m_pcIntraPrediction->predictLumaBlock( puc, iStride, uiPredMode, cIdx ) );

    rcPredBuffer.loadLuma( cYuvMbBuffer, cIdx );

    if( rcMbDataAccess.getMbData().is4x4BlkCoded( cIdx ) )
    {
      RNOK( m_pcTransform->invTransform4x4Blk( puc, iStride, rcCoeffs.get( cIdx ) ) );
    }
#endif
  }

  UInt uiChromaCbp = rcMbDataAccess.getMbData().getCbpChroma4x4();
  RNOK( xDecodeChroma( rcMbDataAccess, cYuvMbBuffer, rcPredBuffer, uiChromaCbp, true ) );

  return Err::m_nOK;
}


ErrVal
MbDecoder::xDecodeMbIntra8x8( MbDataAccess& rcMbDataAccess,
                              YuvMbBuffer&  cYuvMbBuffer,
                              YuvMbBuffer&  rcPredBuffer )
{
#ifndef SHARP_AVC_REWRITE_OUTPUT //JV : Not clean at all -> to remove compilation warnings
	Int  iStride = cYuvMbBuffer.getLStride();
    MbTransformCoeffs& rcCoeffs = m_cTCoeffs;
#endif //JV : Not clean at all

  for( B8x8Idx cIdx; cIdx.isLegal(); cIdx++ )
  {
    if( !rcMbDataAccess.getMbData().getBLSkipFlag() || !rcMbDataAccess.getSH().getTCoeffLevelPredictionFlag() )
    {
      Int iPredMode = rcMbDataAccess.decodeIntraPredMode( cIdx );
      for( S4x4Idx cIdx4x4( cIdx ); cIdx4x4.isLegal( cIdx ); cIdx4x4++ )
      {
        rcMbDataAccess.getMbData().intraPredMode( cIdx4x4 ) = iPredMode;
      }
    }
#ifndef SHARP_AVC_REWRITE_OUTPUT
    XPel* puc = cYuvMbBuffer.getYBlk( cIdx );

    const UInt uiPredMode = rcMbDataAccess.getMbData().intraPredMode( cIdx );

    RNOK( m_pcIntraPrediction->predictLuma8x8Block( puc, iStride, uiPredMode, cIdx ) );

    rcPredBuffer.loadLuma( cYuvMbBuffer, cIdx );

    if( rcMbDataAccess.getMbData().is4x4BlkCoded( cIdx ) )
    {
      RNOK( m_pcTransform->invTransform8x8Blk( puc, iStride, rcCoeffs.get8x8( cIdx ) ) );
    }
#endif
  }

  UInt uiChromaCbp = rcMbDataAccess.getMbData().getCbpChroma4x4();
  RNOK( xDecodeChroma( rcMbDataAccess, cYuvMbBuffer, rcPredBuffer, uiChromaCbp, true ) );

  return Err::m_nOK;
}



ErrVal
MbDecoder::xDecodeMbIntraBL( MbDataAccess&  rcMbDataAccess,
                             YuvPicBuffer*  pcRecYuvBuffer,
                             YuvMbBuffer&   rcPredBuffer,
                             YuvPicBuffer*  pcBaseYuvBuffer )
{
#ifdef SHARP_AVC_REWRITE_OUTPUT
  return Err::m_nOK;
#endif
  YuvMbBuffer      cYuvMbBuffer;
  MbTransformCoeffs&  rcCoeffs = m_cTCoeffs;

  Bool bAddBaseCoeffs = false;
  if( rcMbDataAccess.getSH().getSCoeffResidualPredFlag() )
  {
    rcMbDataAccess.getMbDataAccessBase()->getMbTCoeffs().copyPredictionTo( cYuvMbBuffer );
    bAddBaseCoeffs = rcMbDataAccess.getMbDataAccessBase()->getMbData().isIntraBL();
    if( bAddBaseCoeffs )
    {
      rcCoeffs.add( &rcMbDataAccess.getMbDataAccessBase()->getMbTCoeffs(), true, false );
    }
  }
  else
  {
    cYuvMbBuffer.loadBuffer ( pcBaseYuvBuffer );
  }
  rcPredBuffer.loadLuma   ( cYuvMbBuffer );
  rcPredBuffer.loadChroma ( cYuvMbBuffer );

  if( rcMbDataAccess.getMbData().isTransformSize8x8() )
  {
    for( B8x8Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( m_pcTransform->invTransform8x8Blk( cYuvMbBuffer.getYBlk( cIdx ),
                                               cYuvMbBuffer.getLStride(),
                                               rcCoeffs.get8x8(cIdx) ) );
    }
  }
  else
  {
    for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( m_pcTransform->invTransform4x4Blk( cYuvMbBuffer.getYBlk( cIdx ),
                                               cYuvMbBuffer.getLStride(),
                                               rcCoeffs.get(cIdx) ) );
    }
  }

  UInt uiChromaCbp = rcMbDataAccess.getMbData().getCbpChroma4x4();
  RNOK( xDecodeChroma( rcMbDataAccess, cYuvMbBuffer, rcPredBuffer, uiChromaCbp, false, bAddBaseCoeffs ) );
  // Note that the following also copies pred buffer inside of MbTransformCoeffs
  rcMbDataAccess.getMbTCoeffs().copyFrom( rcCoeffs );
  rcMbDataAccess.getMbTCoeffs().copyPredictionFrom( rcPredBuffer );
  pcRecYuvBuffer->loadBuffer( &cYuvMbBuffer );

  return Err::m_nOK;
}



ErrVal
MbDecoder::xDecodeMbIntra16x16( MbDataAccess& rcMbDataAccess,
                                YuvMbBuffer&  cYuvMbBuffer,
                                YuvMbBuffer&  rcPredBuffer )
{
#ifdef SHARP_AVC_REWRITE_OUTPUT
  return Err::m_nOK;
#endif

  Int  iStride = cYuvMbBuffer.getLStride();

  RNOK( m_pcIntraPrediction->predictLumaMb( cYuvMbBuffer.getMbLumAddr(), iStride, rcMbDataAccess.getMbData().intraPredMode() ) );

  rcPredBuffer.loadLuma( cYuvMbBuffer );

  MbTransformCoeffs& rcCoeffs = m_cTCoeffs;


  Bool                bIntra    = rcMbDataAccess.getMbData().isIntra();
  Bool                b8x8      = rcMbDataAccess.getMbData().isTransformSize8x8();
  Bool                b16x16    = rcMbDataAccess.getMbData().isIntra16x16();
  UInt                uiYScalId = ( bIntra ? ( b8x8 && !b16x16 ? 6 : 0 ) : ( b8x8 ? 7 : 3 ) );
  const UChar*        pucScaleY = rcMbDataAccess.getSH().getScalingMatrix( uiYScalId );
	const Int aaiDequantDcCoef[6] = { 10, 11, 13, 14, 16, 18 };
	const Int iQp = rcMbDataAccess.getMbData().getQp();
	Int iQpScale = aaiDequantDcCoef[iQp%6];
  if( pucScaleY )
  {
    iQpScale  *= pucScaleY[0];
  }
  else
  {
    iQpScale *= 16;
  }
  RNOK( m_pcTransform->invTransformDcCoeff( rcCoeffs.get( B4x4Idx(0) ), iQpScale, iQp/6) );

  for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
  {
    RNOK( m_pcTransform->invTransform4x4Blk( cYuvMbBuffer.getYBlk( cIdx ), iStride, rcCoeffs.get( cIdx ) ) );
  }

  UInt uiChromaCbp = rcMbDataAccess.getMbData().getCbpChroma16x16();
  RNOK( xDecodeChroma( rcMbDataAccess, cYuvMbBuffer, rcPredBuffer, uiChromaCbp, true ) );

  return Err::m_nOK;
}


ErrVal
MbDecoder::xDecodeChroma( MbDataAccess& rcMbDataAccess,
                          YuvMbBuffer&  rcRecYuvBuffer,
                          YuvMbBuffer&  rcPredMbBuffer,
                          UInt          uiChromaCbp,
                          Bool          bPredChroma,
                          Bool          bAddBaseCoeffsChroma )
{
#ifdef SHARP_AVC_REWRITE_OUTPUT
  return Err::m_nOK;
#endif
  MbTransformCoeffs& rcCoeffs = m_cTCoeffs;

  XPel* pucCb   = rcRecYuvBuffer.getMbCbAddr();
  XPel* pucCr   = rcRecYuvBuffer.getMbCrAddr();
  Int   iStride = rcRecYuvBuffer.getCStride();

  if( bPredChroma )
  {
    RNOK( m_pcIntraPrediction->predictChromaBlock( pucCb, pucCr, iStride, rcMbDataAccess.getMbData().getChromaPredMode() ) );
    rcPredMbBuffer.loadChroma( rcRecYuvBuffer );
  }

  if( bAddBaseCoeffsChroma )
  {
    rcCoeffs.add( &rcMbDataAccess.getMbDataAccessBase()->getMbTCoeffs(), false, true );
  }
  TCoeff aDC[8];
  //=== store DC coeff ===
  {
    for( Int i = 0; i < 8; i++ )
    {
      aDC[i] = rcCoeffs.get( CIdx(i) )[0];
    }
  }
  m_pcTransform->invTransformChromaDc( rcCoeffs.get( CIdx(0) ) );
  m_pcTransform->invTransformChromaDc( rcCoeffs.get( CIdx(4) ) );

  RNOK( m_pcTransform->invTransformChromaBlocks( pucCb, iStride, rcCoeffs.get( CIdx(0) ) ) );
  RNOK( m_pcTransform->invTransformChromaBlocks( pucCr, iStride, rcCoeffs.get( CIdx(4) ) ) );

  //=== reset DC coeff ===
  {
    for( Int i = 0; i < 8; i++ )
    {
      rcCoeffs.get( CIdx(i) )[0] = aDC[i];
    }
  }

  return Err::m_nOK;
}



ErrVal
MbDecoder::xScale4x4Block( TCoeff*            piCoeff,
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
MbDecoder::xScale8x8Block( TCoeff*            piCoeff,
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
MbDecoder::xScaleTCoeffs( MbDataAccess& rcMbDataAccess )
{
  ROTRS( rcMbDataAccess.getMbData().isPCM(), Err::m_nOK );
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

  //===== store coefficient level values ==
  rcMbDataAccess.getMbTCoeffs().storeLevelData();

  //===== copy all coefficients =====
  MbTransformCoeffs& rcTCoeffs = m_cTCoeffs;
  rcTCoeffs.copyFrom( rcMbDataAccess.getMbTCoeffs() );

  //===== luma =====
  if( b16x16 )
  {
    //===== INTRA_16x16 =====
    const Int aaiDequantDcCoef[6] = { 10, 11, 13, 14, 16, 18 };

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
  Int iScaleV  = ( g_aaiDequantCoef[cVQp.rem()][0] << cVQp.per() ) * ( pucScaleV ? pucScaleV[0] : 16 );
  for( CIdx cIdx; cIdx.isLegal(); cIdx++ )
  {
    const UChar*        pucScale  = ( cIdx.plane() ? pucScaleV : pucScaleU );
    const QpParameter&  rcQp      = ( cIdx.plane() ? cVQp      : cUQp      );
    const Int           iScale    = ( cIdx.plane() ? iScaleV   : iScaleU   );
    RNOK( xScale4x4Block( rcTCoeffs.get( cIdx ), pucScale, 1, rcQp ) );
    rcTCoeffs.get( cIdx )[0] *= iScale;
  }

  // store the coefficient for non intra 16x16 block
  if( !rcMbDataAccess.getMbData().isIntra16x16() )
  {
    rcMbDataAccess.getMbTCoeffs().copyFrom( rcTCoeffs );
  }

  return Err::m_nOK;
}


ErrVal
MbDecoder::xAddTCoeffs( MbDataAccess& rcMbDataAccess, MbDataAccess& rcMbDataAccessBase )
{
  if( rcMbDataAccess.getMbData().isPCM() )
  {
    TCoeff* pBaseCoeff = rcMbDataAccessBase.getMbTCoeffs().getTCoeffBuffer();
    TCoeff* pCurrCoeff = rcMbDataAccess    .getMbTCoeffs().getTCoeffBuffer();
    for( UInt ui = 0; ui < 384; ui++, pBaseCoeff++, pCurrCoeff++ )
    {
      ROT( pCurrCoeff->getLevel() );
      pCurrCoeff->setLevel( pBaseCoeff->getLevel() );
    }
    return Err::m_nOK;
  }

  UInt uiBCBP = 0;
	UInt uiCoded = 0;
	Bool bCoded = false;
	Bool bChromaAC = false;
	Bool bChromaDC = false;

	rcMbDataAccessBase.getMbTCoeffs().switchLevelCoeffData();

	// Add the luma coefficients and track the new BCBP
	if( rcMbDataAccess.getMbData().isTransformSize8x8() )
	{
		for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
		{
			bCoded = false;

			m_pcTransform->addPrediction8x8Blk( rcMbDataAccess.getMbTCoeffs().get8x8( c8x8Idx ),
				rcMbDataAccessBase.getMbTCoeffs().get8x8( c8x8Idx ),
				rcMbDataAccess.getMbData().getQp(),
				rcMbDataAccessBase.getMbData().getQp(), bCoded );

			if( rcMbDataAccess.getMbData().isIntra16x16() )
				AOT(1);

			if( bCoded )
				uiBCBP |= (0x33 << c8x8Idx.b4x4());
		}

	}
	else
	{

		for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
		{
			uiCoded = 0;

			m_pcTransform->addPrediction4x4Blk( rcMbDataAccess.getMbTCoeffs().get( cIdx ),
				rcMbDataAccessBase.getMbTCoeffs().get( cIdx ),
				rcMbDataAccess.getMbData().getQp(),
				rcMbDataAccessBase.getMbData().getQp(), uiCoded );

			if( rcMbDataAccess.getMbData().isIntra16x16() )
			{
				if( *(rcMbDataAccess.getMbTCoeffs().get( cIdx )) )
					uiCoded--;
			}

			if( uiCoded )
				uiBCBP |= (1<<cIdx);
		}

		if( rcMbDataAccess.getMbData().isIntra16x16() )
		{
			uiBCBP = uiBCBP?((1<<16)-1):0;
		}
	}

	// Add the chroma coefficients and update the BCBP
  m_pcTransform->addPredictionChromaBlocks( rcMbDataAccess.getMbTCoeffs().get( CIdx(0) ),
                                            rcMbDataAccessBase.getMbTCoeffs().get( CIdx(0) ),
                                            rcMbDataAccess.getSH().getCbQp( rcMbDataAccess.getMbData().getQp() ),
                                            rcMbDataAccess.getSH().getBaseSliceHeader()->getCbQp( rcMbDataAccessBase.getMbData().getQp() ),
                                            bChromaDC, bChromaAC );
  m_pcTransform->addPredictionChromaBlocks( rcMbDataAccess.getMbTCoeffs().get( CIdx(4) ),
                                            rcMbDataAccessBase.getMbTCoeffs().get( CIdx(4) ),
                                            rcMbDataAccess.getSH().getCrQp( rcMbDataAccess.getMbData().getQp() ),
                                            rcMbDataAccess.getSH().getBaseSliceHeader()->getCrQp( rcMbDataAccessBase.getMbData().getQp() ),
                                            bChromaDC, bChromaAC );

	uiBCBP |= (bChromaAC?2:(bChromaDC?1:0))<<16;

	// Update the CBP
	rcMbDataAccess.getMbData().setAndConvertMbExtCbp( uiBCBP );

	// Update the Intra16x16 mode
	if( rcMbDataAccess.getMbData().isIntra16x16() )
	{
		UInt uiMbType = INTRA_4X4 + 1;
		UInt uiPredMode = rcMbDataAccess.getMbData().intraPredMode();
		UInt uiChromaCbp = uiBCBP>>16;
		Bool bACcoded = (uiBCBP && ((1<<16)-1));

		uiMbType += uiPredMode;
        uiMbType += ( bACcoded ) ? 12 : 0;
        uiMbType += uiChromaCbp << 2;

		rcMbDataAccess.getMbData().setMbMode( MbMode(uiMbType) );

		// Sanity checks
		if( rcMbDataAccess.getMbData().intraPredMode() != uiPredMode )
			AOT(1);
		if( rcMbDataAccess.getMbData().getCbpChroma16x16() != uiChromaCbp )
			AOT(1);
		if( rcMbDataAccess.getMbData().isAcCoded() != bACcoded )
			AOT(1);
	}

	rcMbDataAccessBase.getMbTCoeffs().switchLevelCoeffData();

	return Err::m_nOK;

}

H264AVC_NAMESPACE_END
