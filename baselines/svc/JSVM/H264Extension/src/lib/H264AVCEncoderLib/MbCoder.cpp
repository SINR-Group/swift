
#include "H264AVCEncoderLib.h"
#include "MbCoder.h"
#include "H264AVCCommonLib/Tables.h"
#include "H264AVCCommonLib/TraceFile.h"

// JVT-W043 {
#include "RateCtlBase.h"
#include "RateCtlQuadratic.h"
// JVT-W043 }
//JVT-X046 {
#include "CabacWriter.h"
#include "UvlcWriter.h"
//JVT-X046 }

H264AVC_NAMESPACE_BEGIN


MbCoder::MbCoder():
  m_pcMbSymbolWriteIf( NULL ),
  m_pcRateDistortionIf( NULL ),
  m_bInitDone( false ),
  m_bCabac( false ),
  m_bPrevIsSkipped( false )
{
  CabacWriter*  pcCabacWriter = 0;
  UvlcWriter*   pcUvlcWriter  = 0;
  ANOK( BitWriteBuffer::create( m_pcBitWriteBufferCabac ) );
  ANOK( BitWriteBuffer::create( m_pcBitWriteBufferUvlc  ) );
	ANOK( CabacWriter::create   ( pcCabacWriter  ) );
	ANOK( UvlcWriter::create    ( pcUvlcWriter   ) );
  ANOK( pcCabacWriter ->init( m_pcBitWriteBufferCabac ) );
  ANOK( pcUvlcWriter  ->init( m_pcBitWriteBufferUvlc  ) );
  m_pcCabacSymbolWriteIf  = pcCabacWriter;
  m_pcUvlcSymbolWriteIf   = pcUvlcWriter;
}


MbCoder::~MbCoder()
{
  ANOK( m_pcBitWriteBufferCabac->destroy() );
  ANOK( m_pcBitWriteBufferUvlc ->destroy() );
	//JVT-X046 {
	delete m_pcCabacSymbolWriteIf;
	delete m_pcUvlcSymbolWriteIf;
	//JVT-X046 }
}


ErrVal MbCoder::create( MbCoder*& rpcMbCoder )
{
  rpcMbCoder = new MbCoder;

  ROT( NULL == rpcMbCoder );

  return Err::m_nOK;
}

ErrVal MbCoder::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal MbCoder::initSlice(  const SliceHeader& rcSH,
                            MbSymbolWriteIf*   pcMbSymbolWriteIf,
                            RateDistortionIf*  pcRateDistortionIf )
{
  ROT( NULL == pcMbSymbolWriteIf );
  ROT( NULL == pcRateDistortionIf );

  m_pcMbSymbolWriteIf = pcMbSymbolWriteIf;
  m_pcRateDistortionIf = pcRateDistortionIf;

  m_bCabac          = rcSH.getPPS().getEntropyCodingModeFlag();
  m_bPrevIsSkipped  = false;

  m_bInitDone = true;

  return Err::m_nOK;
}


ErrVal MbCoder::uninit()
{
  m_pcMbSymbolWriteIf = NULL;
  m_pcRateDistortionIf = NULL;

  m_bInitDone = false;
  return Err::m_nOK;
}






ErrVal MbCoder::encode( MbDataAccess& rcMbDataAccess,
                        MbDataAccess* pcMbDataAccessBase,
                        Bool          bTerminateSlice,
                        Bool          bSendTerminateSlice )
{
  ROF( m_bInitDone );
  ROTRS( rcMbDataAccess.getSH().getSliceSkipFlag(), Err::m_nOK );

  ETRACE_DECLARE( Bool m_bTraceEnable = true );
  ETRACE_DECLARE( Int  iDQIdBase      = Int( 16 * rcMbDataAccess.getSH().getLayerCGSSNR() + rcMbDataAccess.getSH().getQualityLevelCGSSNR() ) );
  ETRACE_LAYER  ( iDQIdBase );
  ETRACE_NEWMB  ( rcMbDataAccess.getMbAddress() );

  rcMbDataAccess.getMbData().setBCBP(0);

  // JVT-W043 {
  if ( bRateControlEnable )
  {
    pcGenericRC->m_iRCTotalBits = getBitCount();
    pcGenericRC->m_iRCTextureBits = 0;
  }
  // JVT-W043 }

  //===== skip flag =====
  Bool  bIsCoded  = ! rcMbDataAccess.isSkippedMb();

  RNOK( m_pcMbSymbolWriteIf->skipFlag( rcMbDataAccess ) );

  MbSymbolWriteIf *pcCurrentWriter = m_pcMbSymbolWriteIf;
  UInt uiMGSFragment = 0;
  for( uiMGSFragment = 0;
       rcMbDataAccess.getSH().getSPS().getMGSCoeffStop( uiMGSFragment ) < 16;
       uiMGSFragment++ )
  {
    ETRACE_LAYER( iDQIdBase + Int( uiMGSFragment + 1 ) );
    ETRACE_NEWMB( rcMbDataAccess.getMbAddress() );
    pcCurrentWriter = pcCurrentWriter->getSymbolWriteIfNextSlice();
    RNOK( pcCurrentWriter->skipFlag( rcMbDataAccess ) );
    if( bIsCoded  && rcMbDataAccess.getSH().isMbaffFrame() && ( rcMbDataAccess.isTopMb() || m_bPrevIsSkipped ) )
    {
      RNOK( pcCurrentWriter->fieldFlag( rcMbDataAccess ) );
    }
  }
  ETRACE_LAYER( iDQIdBase );

  if( bIsCoded )
  {
    Bool bFieldFlagCoded = true;
    if( bFieldFlagCoded && rcMbDataAccess.getSH().isMbaffFrame() && ( rcMbDataAccess.isTopMb() || m_bPrevIsSkipped ) )
    {
      RNOK( m_pcMbSymbolWriteIf->fieldFlag( rcMbDataAccess) );
    }

    Bool bBaseLayerAvailable = (NULL != pcMbDataAccessBase) && !rcMbDataAccess.getSH().getNoInterLayerPredFlag();

    //===== base layer mode flag and base layer refinement flag =====
    if( bBaseLayerAvailable )
    {
      if ( pcMbDataAccessBase->getMbData().getInCropWindowFlag() == true )// TMM_ESS
      {
				if( rcMbDataAccess.getSH().getAdaptiveBaseModeFlag() )
				{
					RNOK  ( m_pcMbSymbolWriteIf->BLSkipFlag( rcMbDataAccess ) );
				}
      }
      else
      {
          ROT  ( rcMbDataAccess.getMbData().getBLSkipFlag () );
      }
    }
    else
    {
      ROT  ( rcMbDataAccess.getMbData().getBLSkipFlag () );
    }

    //===== macroblock mode =====
    if( ! rcMbDataAccess.getMbData().getBLSkipFlag() )
    {
      MbMode  eMbModeOrg = rcMbDataAccess.getMbData().getMbMode();
      MbMode  eMbModeSet = ( eMbModeOrg == INTRA_BL ? INTRA_4X4 : eMbModeOrg );
      rcMbDataAccess.getMbData().setMbMode( eMbModeSet );
      RNOK( m_pcMbSymbolWriteIf->mbMode( rcMbDataAccess ) );
      rcMbDataAccess.getMbData().setMbMode( eMbModeOrg );
    }

    //--- reset motion pred flags ---
    if( rcMbDataAccess.getMbData().getBLSkipFlag() || rcMbDataAccess.getMbData().isIntra() || rcMbDataAccess.getMbData().getMbMode() == MODE_SKIP )
    {
      rcMbDataAccess.getMbMotionData( LIST_0 ).setMotPredFlag( false );
      rcMbDataAccess.getMbMotionData( LIST_1 ).setMotPredFlag( false );
    }
    else if( rcMbDataAccess.getSH().isBSlice() )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        if( BLK_SKIP == rcMbDataAccess.getMbData().getBlkMode   ( c8x8Idx.b8x8Index() ) ||
            !rcMbDataAccess            .getMbData().isBlockFwdBwd( c8x8Idx.b8x8Index(), LIST_0 ) )
        {
          rcMbDataAccess.getMbMotionData( LIST_0 ).setMotPredFlag( false, c8x8Idx );
        }
        if( BLK_SKIP == rcMbDataAccess.getMbData().getBlkMode   ( c8x8Idx.b8x8Index() ) ||
            !rcMbDataAccess            .getMbData().isBlockFwdBwd( c8x8Idx.b8x8Index(), LIST_1 ) )
        {
          rcMbDataAccess.getMbMotionData( LIST_1 ).setMotPredFlag( false, c8x8Idx );
        }
      }
    }

    //===== prediction info =====
    if( ! rcMbDataAccess.getMbData().getBLSkipFlag() )
    {
      //===== BLOCK MODES =====
      if( rcMbDataAccess.getMbData().isInter8x8() )
      {
        RNOK( m_pcMbSymbolWriteIf->blockModes( rcMbDataAccess ) );
      }

      if( rcMbDataAccess.getMbData().isPCM() )
      {
        //===== PCM SAMPLES =====
        RNOK( m_pcMbSymbolWriteIf->samplesPCM( rcMbDataAccess ) );
      }
      else if( rcMbDataAccess.getMbData().isIntra() )
      {
        //===== INTRA PREDICTION MODES =====
        RNOK( xWriteIntraPredModes( rcMbDataAccess ) );
      }
      else
      {
        //===== MOTION INFORMATION =====
        MbMode eMbMode = rcMbDataAccess.getMbData().getMbMode();
        if( rcMbDataAccess.getSH().isBSlice() )
        {
          RNOK( xWriteMotionPredFlags( rcMbDataAccess, eMbMode, LIST_0 ) );
          RNOK( xWriteMotionPredFlags( rcMbDataAccess, eMbMode, LIST_1 ) );
          RNOK( xWriteReferenceFrames( rcMbDataAccess, eMbMode, LIST_0 ) );
          RNOK( xWriteReferenceFrames( rcMbDataAccess, eMbMode, LIST_1 ) );
          RNOK( xWriteMotionVectors  ( rcMbDataAccess, eMbMode, LIST_0 ) );
          RNOK( xWriteMotionVectors  ( rcMbDataAccess, eMbMode, LIST_1 ) );
        }
        else
        {
          RNOK( xWriteMotionPredFlags( rcMbDataAccess, eMbMode, LIST_0 ) );
          RNOK( xWriteReferenceFrames( rcMbDataAccess, eMbMode, LIST_0 ) );
          RNOK( xWriteMotionVectors  ( rcMbDataAccess, eMbMode, LIST_0 ) );
        }
      }
    }

    // JVT-W043 {
    if ( bRateControlEnable )
    {
      pcGenericRC->m_iRCTextureBits = getBitCount();
    }
    // JVT-W043 }

    //===== TEXTURE =====
    if( ! rcMbDataAccess.getMbData().isPCM() )
    {
      Bool bTrafo8x8Flag = ( rcMbDataAccess.getSH().getPPS().getTransform8x8ModeFlag() &&
                           ( rcMbDataAccess.getMbData().getBLSkipFlag() ||
                           ( rcMbDataAccess.getMbData().is8x8TrafoFlagPresent( rcMbDataAccess.getSH().getSPS().getDirect8x8InferenceFlag() ) &&
                             !rcMbDataAccess.getMbData().isIntra4x4() ) ) );
 			//-- JVT-R091
      MbSymbolWriteIf *pcMasterWriter = m_pcMbSymbolWriteIf;
      uiMGSFragment = 0;
      while( true )
      {
        ETRACE_LAYER( iDQIdBase + Int( uiMGSFragment ) );
        RNOK( xWriteTextureInfo( rcMbDataAccess,
                                 pcMbDataAccessBase,
                                 rcMbDataAccess.getMbTCoeffs(),
                                 bTrafo8x8Flag,
                                 rcMbDataAccess.getSH().getSPS().getMGSCoeffStart( uiMGSFragment ),
                                 rcMbDataAccess.getSH().getSPS().getMGSCoeffStop( uiMGSFragment ),
                                 uiMGSFragment ) );

        if( rcMbDataAccess.getSH().getSPS().getMGSCoeffStop( uiMGSFragment ) >= 16 )
        {
          break;
        }
        uiMGSFragment++;

        // update bTrafo8x8Flag according to following slices
        bTrafo8x8Flag = rcMbDataAccess.getSH().getPPS().getTransform8x8ModeFlag();

        m_pcMbSymbolWriteIf = m_pcMbSymbolWriteIf->getSymbolWriteIfNextSlice();
      }
      m_pcMbSymbolWriteIf = pcMasterWriter;
      ETRACE_LAYER( iDQIdBase );
    }

    // JVT-W043 {
    if ( bRateControlEnable )
    {
      pcGenericRC->m_iRCTextureBits = getBitCount() - pcGenericRC->m_iRCTextureBits;
    }
    // JVT-W043 }
  }
  m_bPrevIsSkipped = !bIsCoded;

  ROTRS( ! bSendTerminateSlice, Err::m_nOK );

	//JVT-X046 {
  if (m_uiSliceMode==2)
  {
		if (m_bCabac)
		{
			if ((getBitsWritten() >= m_uiSliceArgument))
			{
				m_pcMbSymbolWriteIf->loadCabacWrite(m_pcCabacSymbolWriteIf);
        ETRACE_RESET;
				RNOK( m_pcMbSymbolWriteIf->terminatingBit ( true ? 1:0 ) );
				RNOK( m_pcMbSymbolWriteIf->finishSlice() );
				bSliceCodedDone=true;
				return Err::m_nOK;
			}
			m_pcCabacSymbolWriteIf->loadCabacWrite(m_pcMbSymbolWriteIf);
		}
		else
		{
			if ((getBitsWritten() >= m_uiSliceArgument))
			{
				m_pcMbSymbolWriteIf->loadUvlcWrite(m_pcUvlcSymbolWriteIf);
        ETRACE_RESET;
				RNOK( m_pcMbSymbolWriteIf->terminatingBit ( true ? 1:0 ) );
				RNOK( m_pcMbSymbolWriteIf->finishSlice() );
				bSliceCodedDone=true;
				return Err::m_nOK;
			}
			m_pcUvlcSymbolWriteIf->loadUvlcWrite(m_pcMbSymbolWriteIf);
		}
    ETRACE_STORE;
	}
	//JVT-X046 }


  //--- write terminating bit ---
  RNOK( m_pcMbSymbolWriteIf->terminatingBit ( bTerminateSlice ? 1:0 ) );

  if( bTerminateSlice )
  {
    RNOK( m_pcMbSymbolWriteIf->finishSlice() );
  }

  // JVT-W043 {
  if ( bRateControlEnable )
  {
    pcGenericRC->m_iRCTotalBits = getBitCount() - pcGenericRC->m_iRCTotalBits;
    pcGenericRC->m_iRCHeaderBits = pcGenericRC->m_iRCTotalBits - pcGenericRC->m_iRCTextureBits;
  }
  // JVT-W043 }

  MbSymbolWriteIf *pcMasterWriter = m_pcMbSymbolWriteIf;
  for( uiMGSFragment = 0; rcMbDataAccess.getSH().getSPS().getMGSCoeffStop( uiMGSFragment ) < 16; uiMGSFragment++ )
  {
    ETRACE_LAYER( iDQIdBase + Int( uiMGSFragment + 1 ) );
    m_pcMbSymbolWriteIf = m_pcMbSymbolWriteIf->getSymbolWriteIfNextSlice();
    RNOK( m_pcMbSymbolWriteIf->terminatingBit ( bTerminateSlice ? 1:0 ) );
    if( bTerminateSlice )
    {
      RNOK( m_pcMbSymbolWriteIf->finishSlice() );
    }
  }
  m_pcMbSymbolWriteIf = pcMasterWriter;
  ETRACE_LAYER( iDQIdBase );
  return Err::m_nOK;
}


ErrVal MbCoder::xWriteIntraPredModes( MbDataAccess& rcMbDataAccess )
{
  ROFRS( rcMbDataAccess.getMbData().isIntra(), Err::m_nOK );

  if( rcMbDataAccess.getMbData().isIntra4x4() )
  {
    if( rcMbDataAccess.getSH().getPPS().getTransform8x8ModeFlag() )
    {
      RNOK( m_pcMbSymbolWriteIf->transformSize8x8Flag( rcMbDataAccess ) );
    }

    if( rcMbDataAccess.getMbData().isTransformSize8x8() )
    {
      for( B8x8Idx cIdx; cIdx.isLegal(); cIdx++ )
      {
        RNOK( m_pcMbSymbolWriteIf->intraPredModeLuma( rcMbDataAccess, cIdx ) );
      }
    }
    else
    {
      for( S4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
      {
        RNOK( m_pcMbSymbolWriteIf->intraPredModeLuma( rcMbDataAccess, cIdx ) );
      }
    }
  }

  if( rcMbDataAccess.getMbData().isIntra4x4() || rcMbDataAccess.getMbData().isIntra16x16() )
  {
    RNOK( m_pcMbSymbolWriteIf->intraPredModeChroma( rcMbDataAccess ) );
  }

  return Err::m_nOK;
}




ErrVal MbCoder::xWriteBlockMv( MbDataAccess& rcMbDataAccess, B8x8Idx c8x8Idx, ListIdx eLstIdx )
{
  BlkMode eBlkMode = rcMbDataAccess.getMbData().getBlkMode( c8x8Idx.b8x8Index() );

  ParIdx8x8 eParIdx = c8x8Idx.b8x8();
  switch( eBlkMode )
  {
    case BLK_8x8:
    {
      RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, eParIdx ) );
      break;
    }
    case BLK_8x4:
    {
      RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_8x4_0 ) );
      RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_8x4_1 ) );
      break;
    }
    case BLK_4x8:
    {
      RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x8_0 ) );
      RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x8_1 ) );
      break;
    }
    case BLK_4x4:
    {
      RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x4_0 ) );
      RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x4_1 ) );
      RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x4_2 ) );
      RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x4_3 ) );
      break;
    }
    case BLK_SKIP:
    {
      break;
    }
    default:
    {
      AF();
      return Err::m_nERR;
    }
  }
  return Err::m_nOK;
}




ErrVal
MbCoder::xWriteMotionPredFlags( MbDataAccess&  rcMbDataAccess,
                                MbMode         eMbMode,
                                ListIdx        eLstIdx )
{
  AOT_DBG( rcMbDataAccess.getMbData().isIntra() );

  MbDataAccess* pcMbDataAccessBase = rcMbDataAccess.getMbDataAccessBase();
  ROFRS ( pcMbDataAccessBase,                                       Err::m_nOK );
  ROFRS ( pcMbDataAccessBase->getMbData().getInCropWindowFlag(),    Err::m_nOK );
// JVT-U160 LMI
  ROFRS ( rcMbDataAccess.getSH().getAdaptiveMotionPredictionFlag(), Err::m_nOK );
  switch( eMbMode )
  {
  case MODE_SKIP:
    {
      break;
    }

  case MODE_16x16:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx) )
      {
        RNOK( m_pcMbSymbolWriteIf->motionPredFlag( rcMbDataAccess, eLstIdx ) );
      }
      break;
    }

  case MODE_16x8:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx) )
      {
        RNOK( m_pcMbSymbolWriteIf->motionPredFlag( rcMbDataAccess, eLstIdx, PART_16x8_0 ) );
      }

      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_2, eLstIdx) )
      {
        RNOK( m_pcMbSymbolWriteIf->motionPredFlag( rcMbDataAccess, eLstIdx, PART_16x8_1 ) );
      }
      break;
    }

  case MODE_8x16:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx) )
      {
        RNOK( m_pcMbSymbolWriteIf->motionPredFlag( rcMbDataAccess, eLstIdx, PART_8x16_0 ) );
      }

      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_1, eLstIdx) )
      {
        RNOK( m_pcMbSymbolWriteIf->motionPredFlag( rcMbDataAccess, eLstIdx, PART_8x16_1 ) );
      }
      break;
    }

  case MODE_8x8:
  case MODE_8x8ref0:
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        if( BLK_SKIP != rcMbDataAccess.getMbData().getBlkMode   ( c8x8Idx.b8x8Index() ) &&
           rcMbDataAccess             .getMbData().isBlockFwdBwd( c8x8Idx.b8x8Index(), eLstIdx) )
        {
          RNOK( m_pcMbSymbolWriteIf->motionPredFlag( rcMbDataAccess, eLstIdx, c8x8Idx.b8x8() ) );
        }
      }
      break;
    }

  default:
    {
      AF();
      return Err::m_nERR;
    }
  }

  return Err::m_nOK;
}


ErrVal
MbCoder::xWriteReferenceFrames( MbDataAccess& rcMbDataAccess,
                                MbMode        eMbMode,
                                ListIdx       eLstIdx )
{
  AOT_DBG( rcMbDataAccess.getMbData().isIntra() );

  if( 1 == rcMbDataAccess.getNumActiveRef( eLstIdx ) )
  {
    return Err::m_nOK;
  }

  MbMotionData& rcMot = rcMbDataAccess.getMbMotionData( eLstIdx );

  switch( eMbMode )
  {
  case MODE_SKIP:
    {
      break;
    }

  case MODE_16x16:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx) && !rcMot.getMotPredFlag() )
      {
        RNOK( m_pcMbSymbolWriteIf->refFrame( rcMbDataAccess, eLstIdx ) );
      }
      break;
    }

  case MODE_16x8:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx) && !rcMot.getMotPredFlag( PART_16x8_0 ) )
      {
        RNOK( m_pcMbSymbolWriteIf->refFrame( rcMbDataAccess, eLstIdx, PART_16x8_0 ) );
      }
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_2, eLstIdx) && !rcMot.getMotPredFlag( PART_16x8_1 ) )
      {
        RNOK( m_pcMbSymbolWriteIf->refFrame( rcMbDataAccess, eLstIdx, PART_16x8_1 ) );
      }
      break;
    }

  case MODE_8x16:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx) && !rcMot.getMotPredFlag( PART_8x16_0 ) ) 
      {
        RNOK( m_pcMbSymbolWriteIf->refFrame( rcMbDataAccess, eLstIdx, PART_8x16_0 ) );
      }
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_1, eLstIdx) && !rcMot.getMotPredFlag( PART_8x16_1 ) )
      {
        RNOK( m_pcMbSymbolWriteIf->refFrame( rcMbDataAccess, eLstIdx, PART_8x16_1 ) );
      }
      break;
    }

  case MODE_8x8:
  case MODE_8x8ref0:
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        if( BLK_SKIP != rcMbDataAccess.getMbData().getBlkMode( c8x8Idx.b8x8Index() ) &&
            rcMbDataAccess.getMbData().isBlockFwdBwd( c8x8Idx.b8x8Index(), eLstIdx ) && !rcMot.getMotPredFlag( c8x8Idx.b8x8() ) )
        {
          RNOK( m_pcMbSymbolWriteIf->refFrame( rcMbDataAccess, eLstIdx, c8x8Idx.b8x8() ) );
        }
      }
      break;
    }

  default:
    {
      AF();
      return Err::m_nERR;
    }
  }

  return Err::m_nOK;
}




ErrVal
MbCoder::xWriteMotionVectors( MbDataAccess& rcMbDataAccess,
                              MbMode        eMbMode,
                              ListIdx       eLstIdx )
{
  AOT_DBG( rcMbDataAccess.getMbData().isIntra() );

  switch( eMbMode )
  {
  case MODE_SKIP:
    {
      return Err::m_nOK;
    }

  case MODE_16x16:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx) )
      {
        RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx ) );
      }
      return Err::m_nOK;
    }

  case MODE_16x8:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx) )
      {
        RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, PART_16x8_0 ) );
      }

      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_2, eLstIdx) )
      {
        RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, PART_16x8_1 ) );
      }
      return Err::m_nOK;
    }

  case MODE_8x16:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx) )
      {
        RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, PART_8x16_0 ) );
      }

      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_1, eLstIdx) )
      {
        RNOK( m_pcMbSymbolWriteIf->mvd( rcMbDataAccess, eLstIdx, PART_8x16_1 ) );
      }
      return Err::m_nOK;
    }

  case MODE_8x8:
  case MODE_8x8ref0:
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        if( rcMbDataAccess.getMbData().isBlockFwdBwd( c8x8Idx.b8x8Index(), eLstIdx ) )
        {
          RNOK( xWriteBlockMv( rcMbDataAccess, c8x8Idx, eLstIdx ) );
        }
      }
      return Err::m_nOK;
    }

  default:
    {
      AF();
      return Err::m_nERR;
    }
  }

  return Err::m_nERR;
}





ErrVal MbCoder::xWriteTextureInfo( MbDataAccess&            rcMbDataAccess,
																	 MbDataAccess*						pcMbDataAccessBase,	// JVT-R091
                                   const MbTransformCoeffs& rcMbTCoeff,
                                   Bool											bTrafo8x8Flag
                                  ,UInt                     uiStart,
                                   UInt                     uiStop,
                                   UInt                     uiMGSFragment
                                   )
{
  Bool bWriteDQp = ( uiStart < uiStop );

  if( uiMGSFragment == 0 ) // required, since we don't have the correct slice header
  if( rcMbDataAccess.getMbData().getBLSkipFlag() ||
     !rcMbDataAccess.getMbData().isIntra() )
  {
    if( rcMbDataAccess.getSH().getAdaptiveResidualPredictionFlag() && pcMbDataAccessBase->getMbData().getInCropWindowFlag() )
    {
      if( ! rcMbDataAccess.getSH().isIntraSlice() )
      {
        RNOK( m_pcMbSymbolWriteIf->resPredFlag( rcMbDataAccess ) );
      }
    }
  }

  if( uiStart != 0 || uiStop != 16 )
  {
    ROT( rcMbDataAccess.getMbData().isIntraButnotIBL() );
  }
  const UInt uiCbp = rcMbDataAccess.getMbData().calcMbCbp( uiStart, uiStop );

  if( uiStart < uiStop && ( rcMbDataAccess.getMbData().getBLSkipFlag() || !rcMbDataAccess.getMbData().isIntra16x16() ) )
  {
    RNOK( m_pcMbSymbolWriteIf->cbp( rcMbDataAccess, uiStart, uiStop ) );
    bWriteDQp = ( 0 != uiCbp );
  }

  if( uiStart < uiStop && bTrafo8x8Flag && ( uiCbp & 0x0F ) )
  {
    ROT( rcMbDataAccess.getMbData().isIntra16x16() );
    ROT( rcMbDataAccess.getMbData().isIntra4x4  () );
    RNOK( m_pcMbSymbolWriteIf->transformSize8x8Flag( rcMbDataAccess, uiStart, uiStop ) );
  }

  if( bWriteDQp )
  {
    RNOK( m_pcMbSymbolWriteIf->deltaQp( rcMbDataAccess ) );
  }

  if( uiStart < uiStop && rcMbDataAccess.getMbData().isIntra16x16() )
  {
    ROT( uiStart != 0 || uiStop != 16 ); // not allowed in this encoder
    RNOK( xScanLumaIntra16x16( rcMbDataAccess, rcMbTCoeff, rcMbDataAccess.getMbData().isAcCoded(), uiStart, uiStop ) );
    RNOK( xScanChromaBlocks  ( rcMbDataAccess, rcMbTCoeff, rcMbDataAccess.getMbData().getCbpChroma16x16(), uiStart, uiStop ) );
    return Err::m_nOK;
  }

  if( uiStart < uiStop )
  {
    if( rcMbDataAccess.getMbData().isTransformSize8x8() )
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        if( (uiCbp >> c8x8Idx.b8x8Index()) & 1 )
        {
          RNOK( m_pcMbSymbolWriteIf->residualBlock8x8( rcMbDataAccess, c8x8Idx, LUMA_SCAN, uiStart, uiStop ) );
        }
      }
    }
    else
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        if( (uiCbp >> c8x8Idx.b8x8Index()) & 1 )
        {
          for( S4x4Idx cIdx(c8x8Idx); cIdx.isLegal(c8x8Idx); cIdx++ )
          {
            RNOK( xScanLumaBlock( rcMbDataAccess, rcMbTCoeff, cIdx, uiStart, uiStop ) );
          }
        }
      }
    }

    RNOK( xScanChromaBlocks( rcMbDataAccess, rcMbTCoeff, uiCbp >> 4, uiStart, uiStop ) );
  }

  return Err::m_nOK;
}






ErrVal MbCoder::xScanLumaIntra16x16( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, Bool bAC, UInt uiStart, UInt uiStop )
{
  if( uiStart == 0 && uiStop != 0 )
  {
    RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, B4x4Idx(0), LUMA_I16_DC, uiStart, uiStop ) );
  }
  ROFRS( bAC && uiStop > 1, Err::m_nOK );

  for( S4x4Idx cIdx; cIdx.isLegal(); cIdx++)
  {
    RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, cIdx, LUMA_I16_AC, uiStart, uiStop ) );
  }

  return Err::m_nOK;
}

ErrVal MbCoder::xScanLumaBlock( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, LumaIdx cIdx, UInt uiStart, UInt uiStop )
{
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, cIdx, LUMA_SCAN, uiStart, uiStop ) );
  return Err::m_nOK;
}


ErrVal MbCoder::xScanChromaDc( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, UInt uiStart, UInt uiStop )
{
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, CIdx(0), CHROMA_DC, uiStart, uiStop ) );
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, CIdx(4), CHROMA_DC, uiStart, uiStop ) );
  return Err::m_nOK;
}

ErrVal MbCoder::xScanChromaAcU( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, UInt uiStart, UInt uiStop )
{
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, CIdx(0), CHROMA_AC, uiStart, uiStop ) );
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, CIdx(1), CHROMA_AC, uiStart, uiStop ) );
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, CIdx(2), CHROMA_AC, uiStart, uiStop ) );
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, CIdx(3), CHROMA_AC, uiStart, uiStop ) );
  return Err::m_nOK;
}

ErrVal MbCoder::xScanChromaAcV( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, UInt uiStart, UInt uiStop )
{
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, CIdx(4), CHROMA_AC, uiStart, uiStop ) );
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, CIdx(5), CHROMA_AC, uiStart, uiStop ) );
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, CIdx(6), CHROMA_AC, uiStart, uiStop ) );
  RNOK( m_pcMbSymbolWriteIf->residualBlock( rcMbDataAccess, CIdx(7), CHROMA_AC, uiStart, uiStop ) );
  return Err::m_nOK;
}

ErrVal MbCoder::xScanChromaBlocks( MbDataAccess& rcMbDataAccess, const MbTransformCoeffs& rcTCoeff, UInt uiChromCbp, UInt uiStart, UInt uiStop )
{
  ROTRS( 1 > uiChromCbp, Err::m_nOK );

  if( uiStart == 0 )
  {
    RNOK( xScanChromaDc ( rcMbDataAccess, rcTCoeff, uiStart, uiStop ) );
  }

  ROTRS( 2 > uiChromCbp, Err::m_nOK );

  if( uiStop > 1 )
  {
    RNOK( xScanChromaAcU( rcMbDataAccess, rcTCoeff, uiStart, uiStop ) );
    RNOK( xScanChromaAcV( rcMbDataAccess, rcTCoeff, uiStart, uiStop ) );
  }
  return Err::m_nOK;
}



H264AVC_NAMESPACE_END
