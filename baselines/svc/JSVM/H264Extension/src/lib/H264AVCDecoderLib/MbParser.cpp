
#include "H264AVCDecoderLib.h"

#include "MbSymbolReadIf.h"
#include "MbParser.h"
#include "DecError.h"


H264AVC_NAMESPACE_BEGIN


MbParser::MbParser()
: m_bInitDone         ( false )
, m_pcMbSymbolReadIf  ( 0 )
, m_bPrevIsSkipped    ( false )
{
}

MbParser::~MbParser()
{
}

ErrVal
MbParser::create( MbParser*& rpcMbParser )
{
  rpcMbParser = new MbParser;
  ROF( rpcMbParser );
  return Err::m_nOK;
}

ErrVal
MbParser::init()
{
  return Err::m_nOK;
}

ErrVal
MbParser::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal
MbParser::initSlice( MbSymbolReadIf* pcMbSymbolReadIf )
{
  ROF( pcMbSymbolReadIf );
  m_pcMbSymbolReadIf  = pcMbSymbolReadIf;
  m_bPrevIsSkipped    = false;
  m_bInitDone         = true;
  return Err::m_nOK;
}

ErrVal
MbParser::uninit()
{
  m_pcMbSymbolReadIf  = 0;
  m_bInitDone         = false;
  return Err::m_nOK;
}


ErrVal
MbParser::read( MbDataAccess&  rcMbDataAccess,
                UInt           uiNumMbRead,
                Bool&          rbEndOfSlice,
                UInt&          ruiNextSkippedVLC )
{
  ROF( m_bInitDone );

  ROTRS( xCheckSkipSliceMb( rcMbDataAccess, uiNumMbRead, rbEndOfSlice ), Err::m_nOK );

  Bool bIsCoded = true;
  if( m_pcMbSymbolReadIf->isMbSkipped( rcMbDataAccess, ruiNextSkippedVLC ) )
  {
    bIsCoded = false;
    rcMbDataAccess.getMbTCoeffs().clear();
    rcMbDataAccess.getMbData().clearIntraPredictionModes( true );
    RNOK( xSkipMb( rcMbDataAccess ) );
    rcMbDataAccess.getMbData().setBLSkipFlag( false );
    rcMbDataAccess.getMbData().setResidualPredFlag( rcMbDataAccess.getMbData().getInCropWindowFlag() ? rcMbDataAccess.getSH().getDefaultResidualPredictionFlag() : false );
    if( rcMbDataAccess.getSH().isBSlice() )
    {
      rcMbDataAccess.getMbData().setFwdBwd( 0x3333 );
      rcMbDataAccess.getMbMotionData( LIST_0 ).clear( RefIdxValues(1) );
      rcMbDataAccess.getMbMvdData   ( LIST_0 ).clear();
      rcMbDataAccess.getMbMotionData( LIST_1 ).clear( RefIdxValues(1) );
      rcMbDataAccess.getMbMvdData   ( LIST_1 ).clear();
    }
    else
    {
      rcMbDataAccess.getMbData().setFwdBwd( 0x1111 );
      rcMbDataAccess.getMbMotionData( LIST_0 ).clear( RefIdxValues(1) );
      rcMbDataAccess.getMbMvdData   ( LIST_0 ).clear();
    }
    rcMbDataAccess.resetQp(); // set QP to that of the last macroblock
  }

  if( bIsCoded )
  {
    if( rcMbDataAccess.getSH().isMbaffFrame() && ( rcMbDataAccess.isTopMb() || m_bPrevIsSkipped ) )
    {
      RNOK( m_pcMbSymbolReadIf->fieldFlag( rcMbDataAccess) );
    }

    Bool bBaseLayerAvailable = ! rcMbDataAccess.getSH().getNoInterLayerPredFlag();

      //===== base layer mode flag and base layer refinement flag =====
    if( bBaseLayerAvailable )
    {
      if ( rcMbDataAccess.getMbData().getInCropWindowFlag() == true )
      {
			  if( rcMbDataAccess.getSH().getAdaptiveBaseModeFlag() )
			  {
          m_pcMbSymbolReadIf->isBLSkipped( rcMbDataAccess );
			  }
			  else
			  {
          rcMbDataAccess.getMbData().setBLSkipFlag( rcMbDataAccess.getSH().getDefaultBaseModeFlag() );
			  }
      }
      else
      {
          rcMbDataAccess.getMbData().setBLSkipFlag( false );
      }
    }
    else
    {
        rcMbDataAccess.getMbData().setBLSkipFlag( false );
    }

    if( rcMbDataAccess.getSH().getStoreRefBasePicFlag() && rcMbDataAccess.getSH().getQualityId() > 0 && rcMbDataAccess.getMbData().getBLSkipFlag() == false )
    {
      printf("Conformance Issue: base_mode_flag = 0 in enhancement layer MGS key picture\n");
    }

    //===== macroblock mode =====
    if( ! rcMbDataAccess.getMbData().getBLSkipFlag() )
    {
      DECRNOK( m_pcMbSymbolReadIf->mbMode( rcMbDataAccess ) );
    }

    if( rcMbDataAccess.getMbData().getBLSkipFlag() )
    {
      //===== copy motion data from base layer ======
      rcMbDataAccess.getMbMvdData( LIST_0 ).clear();
      rcMbDataAccess.getMbMvdData( LIST_1 ).clear();
      rcMbDataAccess.getMbData().setBLSkipFlag( true  );
    }
    else
    {
      //===== BLOCK MODES =====
      if( rcMbDataAccess.getMbData().isInter8x8() )
      {
        DECRNOK( m_pcMbSymbolReadIf->blockModes( rcMbDataAccess ) );

        //===== set motion data for skip block mode =====
        UInt  uiFwdBwd = 0;

        for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
        {
          UInt  uiBlkFwdBwd = rcMbDataAccess.getMbData().getBlockFwdBwd( c8x8Idx.b8x8Index() );

          if( rcMbDataAccess.getMbData().getBlkMode( c8x8Idx.b8x8Index() ) == BLK_SKIP )
          {
            uiBlkFwdBwd = 3;
            rcMbDataAccess.getMbMotionData( LIST_0 ).setRefIdx( 1,            c8x8Idx.b8x8() );
            rcMbDataAccess.getMbMotionData( LIST_1 ).setRefIdx( 1,            c8x8Idx.b8x8() );
            rcMbDataAccess.getMbMvdData   ( LIST_0 ).setAllMv ( Mv::ZeroMv(), c8x8Idx.b8x8() );
            rcMbDataAccess.getMbMvdData   ( LIST_1 ).setAllMv ( Mv::ZeroMv(), c8x8Idx.b8x8() );
          }

          uiFwdBwd |= ( uiBlkFwdBwd << ( c8x8Idx.b8x8Index() * 4 ) );
        }

        rcMbDataAccess.getMbData().setFwdBwd( uiFwdBwd );
      }
      rcMbDataAccess.resetQp(); // set QP to that of the last macroblock


      //===== MOTION DATA =====
      MbMode eMbMode = rcMbDataAccess.getMbData().getMbMode();
      if( rcMbDataAccess.getMbData().isIntra() )
      {
        //===== clear mtoion data for intra blocks =====
        rcMbDataAccess.getMbMotionData( LIST_0 ).clear( BLOCK_NOT_PREDICTED );
        rcMbDataAccess.getMbMvdData   ( LIST_0 ).clear();
        if( rcMbDataAccess.getSH().isBSlice() )
        {
          rcMbDataAccess.getMbMotionData( LIST_1 ).clear( BLOCK_NOT_PREDICTED );
          rcMbDataAccess.getMbMvdData   ( LIST_1 ).clear();
        }
      }
      else if( eMbMode == MODE_SKIP )
      {
        if( rcMbDataAccess.getSH().isBSlice() )
        {
          rcMbDataAccess.getMbData().setFwdBwd( 0x3333 );
          rcMbDataAccess.getMbMotionData( LIST_0 ).clear( RefIdxValues(1) );
          rcMbDataAccess.getMbMvdData   ( LIST_0 ).clear();
          rcMbDataAccess.getMbMotionData( LIST_1 ).clear( RefIdxValues(1) );
          rcMbDataAccess.getMbMvdData   ( LIST_1 ).clear();
        }
        else
        {
          rcMbDataAccess.getMbData().setFwdBwd( 0x1111 );
          rcMbDataAccess.getMbMotionData( LIST_0 ).clear( RefIdxValues(1) );
          rcMbDataAccess.getMbMvdData   ( LIST_0 ).clear();
          rcMbDataAccess.getMbMotionData( LIST_1 ).clear( BLOCK_NOT_PREDICTED );
          rcMbDataAccess.getMbMvdData   ( LIST_1 ).clear();
        }
      }
      else
      {
        if( rcMbDataAccess.getSH().isBSlice() )
        {
          DECRNOK( xReadMotionPredFlags  ( rcMbDataAccess, eMbMode, LIST_0 ) );
          DECRNOK( xReadMotionPredFlags  ( rcMbDataAccess, eMbMode, LIST_1 ) );
          DECRNOK( xReadReferenceIndices ( rcMbDataAccess, eMbMode, LIST_0 ) );
          DECRNOK( xReadReferenceIndices ( rcMbDataAccess, eMbMode, LIST_1 ) );
          DECRNOK( xReadMotionVectors    ( rcMbDataAccess, eMbMode, LIST_0 ) );
          DECRNOK( xReadMotionVectors    ( rcMbDataAccess, eMbMode, LIST_1 ) );
        }
        else
        {
          DECRNOK( xReadMotionPredFlags  ( rcMbDataAccess, eMbMode, LIST_0 ) );
          DECRNOK( xReadReferenceIndices ( rcMbDataAccess, eMbMode, LIST_0 ) );
          DECRNOK( xReadMotionVectors    ( rcMbDataAccess, eMbMode, LIST_0 ) );
        }
      }
    }

    //===== TEXTURE INFO =====
    if( rcMbDataAccess.getMbData().isPCM() )
    {
      DECRNOK( m_pcMbSymbolReadIf->samplesPCM( rcMbDataAccess ) );
    }
    else
    {
      if( ! rcMbDataAccess.getMbData().getBLSkipFlag() )
      {
        DECRNOK( xReadIntraPredModes( rcMbDataAccess ) );
      }

      Bool bTrafo8x8Flag =  ( rcMbDataAccess.getSH().getPPS().getTransform8x8ModeFlag() &&
                            ( rcMbDataAccess.getMbData().getBLSkipFlag() ||
                              ( rcMbDataAccess.getMbData().is8x8TrafoFlagPresent( rcMbDataAccess.getSH().getSPS().getDirect8x8InferenceFlag() ) &&
                               !rcMbDataAccess.getMbData().isIntra4x4() ) ) );
      bBaseLayerAvailable = ! rcMbDataAccess.getSH().getNoInterLayerPredFlag();

      DECRNOK( xReadTextureInfo( rcMbDataAccess,
                                 bTrafo8x8Flag,
                                 bBaseLayerAvailable,
                                 rcMbDataAccess.getSH().getScanIdxStart(),
                                 rcMbDataAccess.getSH().getScanIdxStop() ) );
    }
  }
  m_bPrevIsSkipped = ! bIsCoded;

  if( rcMbDataAccess.getSH().isMbaffFrame() && ( rcMbDataAccess.isTopMb() ) )
  {
    rbEndOfSlice = false;
    return Err::m_nOK;
  }

  //===== terminating bits =====
  rbEndOfSlice = m_pcMbSymbolReadIf->isEndOfSlice();

  return Err::m_nOK;
}


ErrVal
MbParser::xReadIntraPredModes( MbDataAccess& rcMbDataAccess )
{
  ROFRS( rcMbDataAccess.getMbData().isIntra(), Err::m_nOK );

  if( rcMbDataAccess.getMbData().isIntra4x4() )
  {
    if( rcMbDataAccess.getSH().getPPS().getTransform8x8ModeFlag() )
    {
      DECRNOK( m_pcMbSymbolReadIf->transformSize8x8Flag( rcMbDataAccess ) );
    }

    if( rcMbDataAccess.getMbData().isTransformSize8x8() )
    {
      for( B8x8Idx cIdx; cIdx.isLegal(); cIdx++ )
      {
        DECRNOK( m_pcMbSymbolReadIf->intraPredModeLuma8x8( rcMbDataAccess, cIdx ) );
      }
    }
    else
    {
      for( S4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
      {
        DECRNOK( m_pcMbSymbolReadIf->intraPredModeLuma( rcMbDataAccess, cIdx ) );
      }
    }
  }

  if( rcMbDataAccess.getMbData().isIntra4x4() || rcMbDataAccess.getMbData().isIntra16x16() )
  {
    if( rcMbDataAccess.getSH().getSPS().getChromaFormatIdc() )
    {
      DECRNOK( m_pcMbSymbolReadIf->intraPredModeChroma( rcMbDataAccess ) );
    }
    else
    {
      rcMbDataAccess.getMbData().setChromaPredMode( 0 ); // DC prediction
    }
  }

  return Err::m_nOK;
}


ErrVal
MbParser::xGet8x8BlockMv( MbDataAccess& rcMbDataAccess, B8x8Idx c8x8Idx, ListIdx eLstIdx )
{
  ParIdx8x8 eParIdx = c8x8Idx.b8x8();

  switch( rcMbDataAccess.getMbData().getBlkMode( c8x8Idx.b8x8Index() ) )
  {
    case BLK_8x8:
    {
      DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, eParIdx ) );
      break;
    }
    case BLK_8x4:
    {
      DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_8x4_0 ) );
      DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_8x4_1 ) );
      break;
    }
    case BLK_4x8:
    {
      DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x8_0 ) );
      DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x8_1 ) );
      break;
    }
    case BLK_4x4:
    {
      DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x4_0 ) );
      DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x4_1 ) );
      DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x4_2 ) );
      DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, eParIdx, SPART_4x4_3 ) );
      break;
    }
    case BLK_SKIP:
    {
      break;
    }
    default:
    {
      return Err::m_nERR;
    }
  }

  return Err::m_nOK;
}


ErrVal
MbParser::xReadReferenceIndices( MbDataAccess& rcMbDataAccess, MbMode eMbMode, ListIdx eLstIdx )
{
  MbMotionData& rcMbMotionData  = rcMbDataAccess.getMbMotionData( eLstIdx );

  if( rcMbDataAccess.getMbData().isIntra() )
  {
    rcMbMotionData.setRefIdx( -1 );
    return Err::m_nOK;
  }

  switch( eMbMode )
  {
    case MODE_SKIP:
      {
        break;
      }
    case MODE_16x16:
      {
        if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx ) )
        {
          if( !rcMbMotionData.getMotPredFlag() )
          {
            if( 1 == rcMbDataAccess.getNumActiveRef( eLstIdx ) )
            {
              rcMbMotionData.setRefIdx( 1 );
            }
            else
            {
              DECRNOK( m_pcMbSymbolReadIf->refFrame( rcMbDataAccess, eLstIdx ) );
            }
          }
        }
        else
        {
          rcMbMotionData.setRefIdx( BLOCK_NOT_PREDICTED );
        }
        break;
      }
    case MODE_16x8:
      {
        if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx ) )
        {
          if( !rcMbMotionData.getMotPredFlag(PART_16x8_0) )
          {
            if( 1 == rcMbDataAccess.getNumActiveRef( eLstIdx ) )
            {
              rcMbMotionData.setRefIdx( 1, PART_16x8_0 );
            }
            else
            {
              DECRNOK( m_pcMbSymbolReadIf->refFrame( rcMbDataAccess, eLstIdx, PART_16x8_0 ) );
            }
          }
        }
        else
        {
          rcMbMotionData.setRefIdx( BLOCK_NOT_PREDICTED, PART_16x8_0 );
        }

        if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_2, eLstIdx )  )
        {
          if( !rcMbMotionData.getMotPredFlag(PART_16x8_1) )
          {
            if( 1 == rcMbDataAccess.getNumActiveRef( eLstIdx ) )
            {
              rcMbMotionData.setRefIdx( 1, PART_16x8_1 );
            }
            else
            {
              DECRNOK( m_pcMbSymbolReadIf->refFrame( rcMbDataAccess, eLstIdx, PART_16x8_1 ) );
            }
          }
        }
        else
        {
          rcMbMotionData.setRefIdx( BLOCK_NOT_PREDICTED, PART_16x8_1 );
        }
        break;
      }
    case MODE_8x16:
      {
        if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx ) )
        {
          if( !rcMbMotionData.getMotPredFlag(PART_8x16_0) )
          {
            if( 1 == rcMbDataAccess.getNumActiveRef( eLstIdx ) )
            {
              rcMbMotionData.setRefIdx( 1, PART_8x16_0 );
            }
            else
            {
              DECRNOK( m_pcMbSymbolReadIf->refFrame( rcMbDataAccess, eLstIdx, PART_8x16_0 ) );
            }
          }
        }
        else
        {
          rcMbMotionData.setRefIdx( BLOCK_NOT_PREDICTED, PART_8x16_0 );
        }

        if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_1, eLstIdx ) )
        {
          if( !rcMbMotionData.getMotPredFlag(PART_8x16_1) )
          {
            if( 1 == rcMbDataAccess.getNumActiveRef( eLstIdx ) )
            {
              rcMbMotionData.setRefIdx( 1, PART_8x16_1 );
            }
            else
            {
              DECRNOK( m_pcMbSymbolReadIf->refFrame( rcMbDataAccess, eLstIdx, PART_8x16_1 ) );
            }
          }
        }
        else
        {
          rcMbMotionData.setRefIdx( BLOCK_NOT_PREDICTED, PART_8x16_1 );
        }
        break;
      }
    case MODE_8x8:
      {
        for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
        {
          if( BLK_SKIP != rcMbDataAccess.getMbData().getBlkMode( c8x8Idx.b8x8Index() ) )
          {
            if( rcMbDataAccess.getMbData().isBlockFwdBwd( c8x8Idx.b8x8Index(), eLstIdx ) )
            {
              if( !rcMbMotionData.getMotPredFlag(c8x8Idx.b8x8()) )
              {
                if( 1 == rcMbDataAccess.getNumActiveRef( eLstIdx ) )
                {
                  rcMbMotionData.setRefIdx( 1, c8x8Idx.b8x8() );
                }
                else
                {
                  DECRNOK( m_pcMbSymbolReadIf->refFrame( rcMbDataAccess, eLstIdx, c8x8Idx.b8x8() ) );
                }
              }
            }
            else
            {
              rcMbMotionData.setRefIdx( BLOCK_NOT_PREDICTED, c8x8Idx.b8x8() );
            }
          }
        }
        break;
      }
    case MODE_8x8ref0:
      {
        rcMbMotionData.setRefIdx( 1 );
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
MbParser::xReadMotionPredFlags( MbDataAccess&  rcMbDataAccess,
                                MbMode         eMbMode,
                                ListIdx        eLstIdx )
{
  ROTRS( rcMbDataAccess.getSH     ().getNoInterLayerPredFlag(),  Err::m_nOK );
  ROFRS( rcMbDataAccess.getMbData ().getInCropWindowFlag(),      Err::m_nOK );

  MbMotionData& rcMbMotionData = rcMbDataAccess.getMbMotionData( eLstIdx );

  rcMbMotionData.setMotPredFlag( rcMbDataAccess.getSH().getDefaultMotionPredictionFlag() );
  ROFRS ( rcMbDataAccess.getSH().getAdaptiveMotionPredictionFlag(), Err::m_nOK );

  //--- clear ---
  rcMbMotionData.setMotPredFlag( false );

  if( rcMbDataAccess.getMbData().isIntra() )
  {
    return Err::m_nOK;
  }

  switch( eMbMode )
  {
  case MODE_SKIP:
    {
      break;
    }
  case MODE_16x16:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->motionPredFlag( rcMbDataAccess, eLstIdx ) );
      }
      break;
    }
  case MODE_16x8:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->motionPredFlag( rcMbDataAccess, eLstIdx, PART_16x8_0 ) );
      }

      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_2, eLstIdx ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->motionPredFlag( rcMbDataAccess, eLstIdx, PART_16x8_1 ) );
      }
      break;
    }
  case MODE_8x16:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->motionPredFlag( rcMbDataAccess, eLstIdx, PART_8x16_0 ) );
      }

      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_1, eLstIdx ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->motionPredFlag( rcMbDataAccess, eLstIdx, PART_8x16_1 ) );
      }
      break;
    }
  case MODE_8x8:
  case MODE_8x8ref0:
    {
      for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
      {
        if( BLK_SKIP != rcMbDataAccess.getMbData().getBlkMode   ( c8x8Idx.b8x8Index() ) &&
            rcMbDataAccess            .getMbData().isBlockFwdBwd( c8x8Idx.b8x8Index(), eLstIdx ) )
        {
          DECRNOK( m_pcMbSymbolReadIf->motionPredFlag( rcMbDataAccess, eLstIdx, c8x8Idx.b8x8() ) );
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
MbParser::xReadMotionVectors( MbDataAccess& rcMbDataAccess, MbMode eMbMode, ListIdx eLstIdx )
{
  ROTRS( rcMbDataAccess.getMbData().isIntra(), Err::m_nOK );

  switch( eMbMode )
  {
  case MODE_SKIP:
    {
      return Err::m_nOK;
    }
  case MODE_16x16:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx ) );
      }
      return Err::m_nOK;
    }
  case MODE_16x8:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, PART_16x8_0 ) );
      }

      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_2, eLstIdx ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, PART_16x8_1 ) );
      }
      return Err::m_nOK;
    }
  case MODE_8x16:
    {
      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_0, eLstIdx ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, PART_8x16_0 ) );
      }

      if( rcMbDataAccess.getMbData().isBlockFwdBwd( B_8x8_1, eLstIdx ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->mvd( rcMbDataAccess, eLstIdx, PART_8x16_1 ) );
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
          DECRNOK( xGet8x8BlockMv( rcMbDataAccess, c8x8Idx, eLstIdx ) );
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


ErrVal
MbParser::xReadTextureInfo( MbDataAccess&   rcMbDataAccess,
                            Bool            bTrafo8x8Flag,
                            Bool            bBaseLayerAvailable,
                            UInt            uiStart,
                            UInt            uiStop )
{
  Bool bReadDQp = ( uiStart < uiStop );

  if( !rcMbDataAccess.getSH().isIntraSlice() && 
      rcMbDataAccess.getMbData().getInCropWindowFlag() && 
      ( rcMbDataAccess.getMbData().getBLSkipFlag() || !rcMbDataAccess.getMbData().isIntra() ) )
  {
    if( rcMbDataAccess.getSH().getAdaptiveResidualPredictionFlag() )
    {
      DECRNOK( m_pcMbSymbolReadIf->resPredFlag( rcMbDataAccess ) );
    }
    else
    {
      rcMbDataAccess.getMbData().setResidualPredFlag( rcMbDataAccess.getSH().getDefaultResidualPredictionFlag() );
    }
  }
  else
  {
    rcMbDataAccess.getMbData().setResidualPredFlag( false );
  }

  if( rcMbDataAccess.getMbData().getBLSkipFlag() ||
     !rcMbDataAccess.getMbData().isIntra16x16() )
  {
    if( uiStart >= uiStop )
    {
      rcMbDataAccess.getMbData().setMbCbp( 0 );
    }
    else
    {
      DECRNOK( m_pcMbSymbolReadIf->cbp( rcMbDataAccess, uiStart, uiStop ) );
    }
    bReadDQp = rcMbDataAccess.getMbData().getMbCbp() != 0;
  }

  if( bTrafo8x8Flag && ( rcMbDataAccess.getMbData().getMbCbp() & 0x0F ) )
  {
    DECRNOK( m_pcMbSymbolReadIf->transformSize8x8Flag( rcMbDataAccess ) );
  }

  if( bReadDQp )
  {
    DECRNOK( m_pcMbSymbolReadIf->deltaQp( rcMbDataAccess ) );
  }
  else
  {
    rcMbDataAccess.resetQp();
  }

  Bool bIntra16x16 = ( !rcMbDataAccess.getMbData().getBLSkipFlag() &&
                        rcMbDataAccess.getMbData().isIntra16x16 ()   );
  if( bIntra16x16 )
  {
    UInt uiDummy = 0;
    if( uiStart == 0 && uiStop != 0 )
    {
      DECRNOK( m_pcMbSymbolReadIf->residualBlock( rcMbDataAccess, B4x4Idx(0), LUMA_I16_DC, uiDummy, uiStart, uiStop ) );
    }
    if( rcMbDataAccess.getMbData().isAcCoded() && uiStop > 1 )
    {
      for( S4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
      {
        DECRNOK( m_pcMbSymbolReadIf->residualBlock( rcMbDataAccess, cIdx, LUMA_I16_AC, uiDummy, uiStart, uiStop ) );
      }
      rcMbDataAccess.getMbData().setMbCbp( 0xf + ( rcMbDataAccess.getMbData().getCbpChroma16x16() << 4) );
    }
    else
    {
      rcMbDataAccess.getMbData().setMbCbp( 0x0 + ( rcMbDataAccess.getMbData().getCbpChroma16x16() << 4) );
    }

    DECRNOK( xScanChromaBlocks( rcMbDataAccess,  rcMbDataAccess.getMbData().getCbpChroma16x16(), uiStart, uiStop ) );
    return Err::m_nOK;
  }


  UInt uiMbExtCbp = rcMbDataAccess.getMbData().getMbExtCbp();

  if( rcMbDataAccess.getMbData().isTransformSize8x8() )
  {
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      if( uiMbExtCbp & ( 1 << c8x8Idx.b4x4() ) )
      {
        DECRNOK( m_pcMbSymbolReadIf->residualBlock8x8( rcMbDataAccess, c8x8Idx, uiStart, uiStop ) );
      }
    }
  }
  else
  {
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      if( uiMbExtCbp & ( 1 << c8x8Idx.b4x4() ) )
      {
        for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
        {
          DECRNOK( m_pcMbSymbolReadIf->residualBlock( rcMbDataAccess, cIdx , LUMA_SCAN, uiMbExtCbp, uiStart, uiStop ) );
        }
      }
    }

  }
  rcMbDataAccess.getMbData().setMbExtCbp( uiMbExtCbp );

  DECRNOK( xScanChromaBlocks  ( rcMbDataAccess,  rcMbDataAccess.getMbData().getCbpChroma4x4(), uiStart, uiStop ) );
  return Err::m_nOK;
}


ErrVal
MbParser::xScanChromaBlocks( MbDataAccess& rcMbDataAccess, UInt uiChromCbp, UInt uiStart, UInt uiStop )
{
  ROTRS( 1 > uiChromCbp, Err::m_nOK );

  if( uiStart == 0 )
  {
    DECRNOK( m_pcMbSymbolReadIf->residualBlock( rcMbDataAccess,  CIdx(0), CHROMA_DC, uiStart, uiStop ) );
    DECRNOK( m_pcMbSymbolReadIf->residualBlock( rcMbDataAccess,  CIdx(4), CHROMA_DC, uiStart, uiStop ) );
  }

  ROTRS( 2 > uiChromCbp, Err::m_nOK );

  for( CIdx cCIdx; cCIdx.isLegal(); cCIdx++ )
  {
    if( uiStop > 1 )
    {
      DECRNOK( m_pcMbSymbolReadIf->residualBlock( rcMbDataAccess,  cCIdx, CHROMA_AC, uiStart, uiStop ) );
    }
  }
  return Err::m_nOK;
}


Bool
MbParser::xCheckSkipSliceMb( MbDataAccess& rcMbDataAccess, UInt uiNumMbRead, Bool& rbEndOfSlice )
{
  ROFRS( rcMbDataAccess.getSH().getSliceSkipFlag(), false );

  MbData& rcMbData = rcMbDataAccess.getMbData();
  rcMbData.setSkipFlag          ( false );
  rcMbData.setBLSkipFlag        ( true );
  rcMbData.setResidualPredFlag  ( true );
  rcMbData.setMbExtCbp          ( 0 );
  rcMbData.setFwdBwd            ( 0 );
  rcMbDataAccess.resetQp        ();
  rcMbDataAccess.getMbMotionData( LIST_0 ).clear( BLOCK_NOT_PREDICTED );
  rcMbDataAccess.getMbMotionData( LIST_1 ).clear( BLOCK_NOT_PREDICTED );
  rcMbDataAccess.getMbMvdData   ( LIST_0 ).clear();
  rcMbDataAccess.getMbMvdData   ( LIST_1 ).clear();

  rbEndOfSlice = ( uiNumMbRead >= rcMbDataAccess.getSH().getNumMbsInSliceMinus1() );
  return true;
}


ErrVal
MbParser::xSkipMb( MbDataAccess& rcMbDataAccess )
{
  if( rcMbDataAccess.getSH().isBSlice() )
  {
    RNOK( rcMbDataAccess.setConvertMbType( 0 ) );
  }
  else
  {
    rcMbDataAccess.getMbMotionData( LIST_0 ).setRefIdx( 1 );
    RNOK( rcMbDataAccess.setConvertMbType( MSYS_UINT_MAX ) );
  }

  rcMbDataAccess.getMbData().setMbCbp( 0 );
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END

