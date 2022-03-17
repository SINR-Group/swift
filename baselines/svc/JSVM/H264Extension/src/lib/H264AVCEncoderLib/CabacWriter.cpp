
#include "H264AVCEncoderLib.h"
#include "H264AVCCommonLib/Tables.h"
#include "H264AVCCommonLib/ContextTables.h"
#include "H264AVCCommonLib/TraceFile.h"

#include "CabaEncoder.h"
#include "CabacWriter.h"


H264AVC_NAMESPACE_BEGIN


const int MAX_COEFF[9] = { 8,16,16,16,15, 4, 4,15,15};
const int COUNT_THR[9] = { 3, 4, 4, 4, 3, 2, 2, 3, 3};


CabacWriter::CabacWriter():
	m_cFieldFlagCCModel   ( 1,                3),
  m_cFldMapCCModel      ( NUM_BLOCK_TYPES,  NUM_MAP_CTX),
  m_cFldLastCCModel     ( NUM_BLOCK_TYPES,  NUM_LAST_CTX),
  m_cBLSkipCCModel      ( 1,                NUM_BL_SKIP_FLAG_CTX ),
  m_cBCbpCCModel( NUM_BLOCK_TYPES, NUM_BCBP_CTX ),
  m_cMapCCModel( NUM_BLOCK_TYPES, NUM_MAP_CTX ),
  m_cLastCCModel( NUM_BLOCK_TYPES, NUM_LAST_CTX ),
  m_cOneCCModel( NUM_BLOCK_TYPES, NUM_ABS_CTX ),
  m_cAbsCCModel( NUM_BLOCK_TYPES, NUM_ABS_CTX ),
  m_cChromaPredCCModel( 1, 4 ),
  m_cMbTypeCCModel( 3, NUM_MB_TYPE_CTX ),
  m_cBlockTypeCCModel( 2, NUM_B8_TYPE_CTX ),
  m_cMvdCCModel( 2, NUM_MV_RES_CTX ),
  m_cRefPicCCModel( 2, NUM_REF_NO_CTX ),
  m_cMotPredFlagCCModel( 1, NUM_MOT_PRED_FLAG_CTX ),
  m_cResPredFlagCCModel( 1, NUM_RES_PRED_FLAG_CTX ),
  m_cDeltaQpCCModel( 1, NUM_DELTA_QP_CTX ),
  m_cIntraPredCCModel( 9, NUM_IPR_CTX ),
  m_cCbpCCModel( 3, NUM_CBP_CTX ),
  m_cTransSizeCCModel( 1, NUM_TRANSFORM_SIZE_CTX ),
  m_pcSliceHeader( NULL ),
  m_uiBitCounter( 0 ),
  m_uiPosCounter( 0 ),
  m_uiLastDQpNonZero(0),
  m_bTraceEnable(true)
, m_pcNextCabacWriter( NULL )
{
}

CabacWriter::~CabacWriter()
{
}


ErrVal CabacWriter::xInitContextModels( const SliceHeader& rcSliceHeader )
{
  Int  iQp    = rcSliceHeader.getSliceQp();
  Bool bIntra = rcSliceHeader.isIntraSlice();
  Int  iIndex = rcSliceHeader.getCabacInitIdc();

  if( bIntra )
  {
    RNOK( m_cMbTypeCCModel.initBuffer(      (Short*)INIT_MB_TYPE_I,       iQp ) );
    RNOK( m_cBlockTypeCCModel.initBuffer(   (Short*)INIT_B8_TYPE_I,       iQp ) );
    RNOK( m_cMvdCCModel.initBuffer(         (Short*)INIT_MV_RES_I,        iQp ) );
    RNOK( m_cRefPicCCModel.initBuffer(      (Short*)INIT_REF_NO_I,        iQp ) );
    RNOK( m_cMotPredFlagCCModel.initBuffer( (Short*)INIT_MOTION_PRED_FLAG,iQp ) );
    RNOK( m_cResPredFlagCCModel.initBuffer( (Short*)INIT_RES_PRED_FLAG,   iQp ) );
    RNOK( m_cDeltaQpCCModel.initBuffer(     (Short*)INIT_DELTA_QP_I,      iQp ) );
    RNOK( m_cIntraPredCCModel.initBuffer(   (Short*)INIT_IPR_I,           iQp ) );
    RNOK( m_cChromaPredCCModel.initBuffer(  (Short*)INIT_CIPR_I,          iQp ) );
    RNOK( m_cBLSkipCCModel.initBuffer(      (Short*)INIT_BL_SKIP_I,       iQp ) );

    RNOK( m_cCbpCCModel.initBuffer(         (Short*)INIT_CBP_I,           iQp ) );
    RNOK( m_cBCbpCCModel.initBuffer(        (Short*)INIT_BCBP_I,          iQp ) );
    RNOK( m_cMapCCModel.initBuffer(         (Short*)INIT_MAP_I,           iQp ) );
    RNOK( m_cLastCCModel.initBuffer(        (Short*)INIT_LAST_I,          iQp ) );
    RNOK( m_cOneCCModel.initBuffer(         (Short*)INIT_ONE_I,           iQp ) );
    RNOK( m_cAbsCCModel.initBuffer(         (Short*)INIT_ABS_I,           iQp ) );

    RNOK( m_cTransSizeCCModel.initBuffer(   (Short*)INIT_TRANSFORM_SIZE_I,iQp ) );
    RNOK( m_cFieldFlagCCModel.initBuffer(   (Short*)INIT_MB_AFF_I,        iQp ) );
    RNOK( m_cFldMapCCModel.initBuffer(      (Short*)INIT_FLD_MAP_I,       iQp ) );
    RNOK( m_cFldLastCCModel.initBuffer(     (Short*)INIT_FLD_LAST_I,      iQp ) );
  }
  else
  {
    RNOK( m_cMbTypeCCModel.initBuffer(      (Short*)INIT_MB_TYPE_P        [iIndex], iQp ) );
    RNOK( m_cBlockTypeCCModel.initBuffer(   (Short*)INIT_B8_TYPE_P        [iIndex], iQp ) );
    RNOK( m_cMvdCCModel.initBuffer(         (Short*)INIT_MV_RES_P         [iIndex], iQp ) );
    RNOK( m_cRefPicCCModel.initBuffer(      (Short*)INIT_REF_NO_P         [iIndex], iQp ) );
    RNOK( m_cMotPredFlagCCModel.initBuffer( (Short*)INIT_MOTION_PRED_FLAG,          iQp ) );
    RNOK( m_cResPredFlagCCModel.initBuffer( (Short*)INIT_RES_PRED_FLAG,             iQp ) );
    RNOK( m_cDeltaQpCCModel.initBuffer(     (Short*)INIT_DELTA_QP_P       [iIndex], iQp ) );
    RNOK( m_cIntraPredCCModel.initBuffer(   (Short*)INIT_IPR_P            [iIndex], iQp ) );
    RNOK( m_cChromaPredCCModel.initBuffer(  (Short*)INIT_CIPR_P           [iIndex], iQp ) );
    RNOK( m_cBLSkipCCModel.initBuffer(      (Short*)INIT_BL_SKIP_P,                 iQp ) );

    RNOK( m_cCbpCCModel.initBuffer(         (Short*)INIT_CBP_P            [iIndex], iQp ) );
    RNOK( m_cBCbpCCModel.initBuffer(        (Short*)INIT_BCBP_P           [iIndex], iQp ) );
    RNOK( m_cMapCCModel.initBuffer(         (Short*)INIT_MAP_P            [iIndex], iQp ) );
    RNOK( m_cLastCCModel.initBuffer(        (Short*)INIT_LAST_P           [iIndex], iQp ) );
    RNOK( m_cOneCCModel.initBuffer(         (Short*)INIT_ONE_P            [iIndex], iQp ) );
    RNOK( m_cAbsCCModel.initBuffer(         (Short*)INIT_ABS_P            [iIndex], iQp ) );

    RNOK( m_cTransSizeCCModel.initBuffer(   (Short*)INIT_TRANSFORM_SIZE_P [iIndex], iQp ) );
    RNOK( m_cFieldFlagCCModel.initBuffer(   (Short*)INIT_MB_AFF_P         [iIndex], iQp ) );
    RNOK( m_cFldMapCCModel.initBuffer(      (Short*)INIT_FLD_MAP_P[ iIndex ],       iQp ) );
    RNOK( m_cFldLastCCModel.initBuffer(     (Short*)INIT_FLD_LAST_P[ iIndex ],      iQp ) );
  }

  return Err::m_nOK;
}



ErrVal CabacWriter::create( CabacWriter*& rpcCabacWriter )
{
  rpcCabacWriter = new CabacWriter;

  ROT( NULL == rpcCabacWriter );

  return Err::m_nOK;
}

ErrVal CabacWriter::destroy()
{
  if( m_pcNextCabacWriter )
  {
    m_pcNextCabacWriter->destroy();
  }
  delete this;
  return Err::m_nOK;
}

ErrVal CabacWriter::init(  BitWriteBufferIf *pcBitWriteBufferIf )
{
  ROT( NULL == pcBitWriteBufferIf );

  RNOK( CabaEncoder::init( pcBitWriteBufferIf ) );

  return Err::m_nOK;
}

ErrVal CabacWriter::uninit()
{
  if( m_pcNextCabacWriter )
  {
    m_pcNextCabacWriter->uninit();
  }
  RNOK( CabaEncoder::uninit() );
  return Err::m_nOK;
}


MbSymbolWriteIf* CabacWriter::getSymbolWriteIfNextSlice()
{
  if( !m_pcNextCabacWriter )
  {
    CabacWriter::create( m_pcNextCabacWriter );
    m_pcNextCabacWriter->init( m_pcBitWriteBufferIf->getNextBitWriteBuffer( false ) );
  }
  return m_pcNextCabacWriter;
}


ErrVal CabacWriter::startSlice( const SliceHeader& rcSliceHeader )
{
  m_pcSliceHeader     = &rcSliceHeader;
  m_uiLastDQpNonZero  = 0;
  ROTRS( m_pcSliceHeader->getSliceSkipFlag(), Err::m_nOK );
  RNOK( xInitContextModels( rcSliceHeader ) );
  RNOK( CabaEncoder::start() );

  return Err::m_nOK;
}

//FIX_FRAG_CAVLC
ErrVal CabacWriter::getLastByte(UChar &uiLastByte, UInt &uiLastBitPos)
{
  RNOK(CabaEncoder::getLastByte(uiLastByte, uiLastBitPos ));
  return Err::m_nOK;
}
ErrVal CabacWriter::setFirstBits(UChar ucByte, UInt uiLastBitPos)
{
  RNOK( CabaEncoder::setFirstBits(ucByte, uiLastBitPos));
  return Err::m_nOK;
}
//~FIX_FRAG_CAVLC
ErrVal CabacWriter::finishSlice()
{
  RNOK( CabaEncoder::finish() );

  return Err::m_nOK;
}


ErrVal CabacWriter::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  UInt uiRefFrame = rcMbDataAccess.getMbMotionData( eLstIdx ).getRefIdx();
  RNOK( xRefFrame( rcMbDataAccess, uiRefFrame, eLstIdx, PART_8x8_0 ) );
  return Err::m_nOK;
}

ErrVal CabacWriter::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  )
{
  UInt uiRefFrame = rcMbDataAccess.getMbMotionData( eLstIdx ).getRefIdx( eParIdx );
  RNOK( xRefFrame( rcMbDataAccess, uiRefFrame, eLstIdx, ParIdx8x8( eParIdx ) ) );
  return Err::m_nOK;
}

ErrVal CabacWriter::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  )
{
  UInt uiRefFrame = rcMbDataAccess.getMbMotionData( eLstIdx ).getRefIdx( eParIdx );
  RNOK( xRefFrame( rcMbDataAccess, uiRefFrame, eLstIdx, ParIdx8x8( eParIdx ) ) );
  return Err::m_nOK;
}

ErrVal CabacWriter::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  )
{
  UInt uiRefFrame = rcMbDataAccess.getMbMotionData( eLstIdx ).getRefIdx( eParIdx );
  RNOK( xRefFrame( rcMbDataAccess, uiRefFrame, eLstIdx, ParIdx8x8( eParIdx ) ) );
  return Err::m_nOK;
}


ErrVal CabacWriter::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  return xMotionPredFlag( rcMbDataAccess.getMbMotionData( eLstIdx ).getMotPredFlag(), eLstIdx );
}
ErrVal CabacWriter::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx )
{
  return xMotionPredFlag( rcMbDataAccess.getMbMotionData( eLstIdx ).getMotPredFlag(eParIdx), eLstIdx );
}
ErrVal CabacWriter::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx )
{
  return xMotionPredFlag( rcMbDataAccess.getMbMotionData( eLstIdx ).getMotPredFlag(eParIdx), eLstIdx );
}
ErrVal CabacWriter::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8 eParIdx )
{
  return xMotionPredFlag( rcMbDataAccess.getMbMotionData( eLstIdx ).getMotPredFlag(eParIdx), eLstIdx );
}


ErrVal CabacWriter::xRefFrame( MbDataAccess& rcMbDataAccess, UInt uiRefFrame, ListIdx eLstIdx, ParIdx8x8 eParIdx )
{
  AOT_DBG( (Int)uiRefFrame == -1 );
  AOT_DBG( (Int)uiRefFrame == -2 );
  AOT_DBG( (Int)uiRefFrame == 0 );

  UInt uiCtx = rcMbDataAccess.getCtxRefIdx( eLstIdx, eParIdx );

  RNOK( CabaEncoder::writeSymbol( ( uiRefFrame==1 ? 0 : 1 ), m_cRefPicCCModel.get( 0, uiCtx ) ) );

  if ( uiRefFrame > 1 )
  {
    RNOK( CabaEncoder::writeUnarySymbol( uiRefFrame-2, &m_cRefPicCCModel.get( 0, 4 ), 1 ) );
  }

  ETRACE_TH( "RefFrame" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( uiRefFrame -1 );
  ETRACE_N;

  return Err::m_nOK;
}


ErrVal CabacWriter::xMotionPredFlag( Bool bFlag, ListIdx eLstIdx )
{
  UInt  uiCode  = ( bFlag ? 1: 0 );

  RNOK( CabaEncoder::writeSymbol( uiCode, m_cMotPredFlagCCModel.get( 0, eLstIdx ) ) );

  ETRACE_TH( "MotionPredFlag" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( uiCode );
  ETRACE_N;

  return Err::m_nOK;
}


ErrVal CabacWriter::blockModes( MbDataAccess& rcMbDataAccess )
{
  for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
  {
    RNOK( xWriteBlockMode( rcMbDataAccess.getConvertBlkMode( c8x8Idx.b8x8Index() ) ) )
  }
  return Err::m_nOK;
}

ErrVal CabacWriter::xWriteBlockMode( UInt uiBlockMode )
{
  UInt uiSymbol;

  if( ! m_pcSliceHeader->isBSlice() )
  {
    uiSymbol = (0 == uiBlockMode) ? 1 : 0;
    RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 0, 1 ) ) );
    if( !uiSymbol )
    {
      uiSymbol = (1 == uiBlockMode) ? 0 : 1;
      RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 0, 3 ) ) );
      if( uiSymbol )
      {
        uiSymbol = (2 == uiBlockMode) ? 1:0;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 0, 4 ) ) );
      }
    }
  }
  else
  {
    uiSymbol = ( uiBlockMode ? 1 : 0 );
    RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 0 ) ) );
    if( uiSymbol )
    {
      uiSymbol = (3 > uiBlockMode) ? 0 : 1;
      RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 1 ) ) );
      if( uiSymbol )
      {
        uiSymbol = (7 > uiBlockMode) ? 0 : 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 2 ) ) );
        if( uiSymbol )
        {
          uiBlockMode -= 7;
          uiSymbol = ( uiBlockMode >> 2 ) & 1;
          RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
          if( ! uiSymbol )
          {
            uiSymbol = ( uiBlockMode >> 1 ) & 1;
            RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
          }
          uiSymbol = uiBlockMode & 1;
          RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
          uiBlockMode += 7; // just for correct trace file
        }
        else
        {
          uiSymbol = (5 > uiBlockMode) ? 0 : 1;
          RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
          uiSymbol = ( 1 == (1 & uiBlockMode) ) ? 0 : 1;
          RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
        }
      }
      else
      {
        uiSymbol = (1 == uiBlockMode) ? 0 : 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
      }
    }
  }

  ETRACE_TH( "BlockMode" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE(uiBlockMode);
  ETRACE_N;

  return Err::m_nOK;
}



ErrVal CabacWriter::fieldFlag( MbDataAccess& rcMbDataAccess )
{
  UInt uiSymbol = rcMbDataAccess.getMbData().getFieldFlag() ? 1 : 0;

  RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cFieldFlagCCModel.get( 0, rcMbDataAccess.getCtxFieldFlag() ) ) );

  ETRACE_TH( "FieldFlag:" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( rcMbDataAccess.getMbData().getFieldFlag() );
  ETRACE_N;

  return Err::m_nOK;
}
ErrVal CabacWriter::skipFlag( MbDataAccess& rcMbDataAccess )
{
  ROTRS( m_pcSliceHeader->isIntraSlice(), Err::m_nOK );

  UInt uiSymbol = rcMbDataAccess.isSkippedMb() ? 1 : 0;

  if( m_pcSliceHeader->isBSlice() )
  {
    RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 7 + rcMbDataAccess.getCtxMbSkipped() ) ) );
  }
  else
  {
    RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, rcMbDataAccess.getCtxMbSkipped() ) ) );
  }
  rcMbDataAccess.getMbData().setSkipFlag(uiSymbol!=0);
  ROTRS( 0 == uiSymbol, Err::m_nOK );

  m_uiLastDQpNonZero = 0; // no DeltaQP for Skipped Macroblock
  ETRACE_TH( "MbMode" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( 0 );
  ETRACE_N;

  return Err::m_nOK;
}


ErrVal CabacWriter::BLSkipFlag( MbDataAccess& rcMbDataAccess )
{
  UInt uiSymbol = rcMbDataAccess.getMbData().getBLSkipFlag() ? 1 : 0;
  UInt uiCtx    = rcMbDataAccess.getCtxBLSkipFlag();

  RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cBLSkipCCModel.get( 0, uiCtx ) ) );

  ETRACE_TH( "BLSkipFlag" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( uiSymbol );
  ETRACE_N;

  return Err::m_nOK;
}


ErrVal CabacWriter::resPredFlag( MbDataAccess& rcMbDataAccess )
{
  UInt  uiSymbol = ( rcMbDataAccess.getMbData().getResidualPredFlag() ? 1 : 0 );
  UInt  uiCtx    = ( rcMbDataAccess.getMbData().getBLSkipFlag() ? 0 : 1 );

  RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cResPredFlagCCModel.get( 0, uiCtx ) ) );

  ETRACE_TH( "ResidualPredFlag " );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( uiSymbol );
  ETRACE_N;

  return Err::m_nOK;
}


ErrVal CabacWriter::mbMode( MbDataAccess& rcMbDataAccess )
{
  UInt uiMbMode     = rcMbDataAccess.getConvertMbType();
  ETRACE_DECLARE( UInt uiOrigMbMode = uiMbMode );

  if( m_pcSliceHeader->isIntraSlice() )
  {
    UInt uiSymbol;
    uiSymbol = ( 0 == uiMbMode) ? 0 : 1;
    RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 0, rcMbDataAccess.getCtxMbIntra4x4() ) ) );

    if( uiSymbol )
    {
      uiSymbol = ( 25 == uiMbMode) ? 1 : 0;
      RNOK( CabaEncoder::writeTerminatingBit( uiSymbol ) );

      if( ! uiSymbol )
      {
        uiSymbol = ( 13 > uiMbMode) ? 0 : 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 0, 4 ) ) );
        uiMbMode -= 12* uiSymbol + 1;

        uiSymbol = ( 4 > uiMbMode) ? 0 : 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 0, 5 ) ) );

        if( uiSymbol )
        {
          uiSymbol = ( 8 > uiMbMode) ? 0 : 1;
          RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 0, 6 ) ) );
        }

        uiSymbol = (uiMbMode>>1) & 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 0, 7 ) ) );

        uiSymbol = uiMbMode & 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 0, 8 ) ) );
      }
    }
  }
  else
  {
    UInt uiSymbol, uiIntra16x16Symbol = 0;

    if( ! m_pcSliceHeader->isBSlice() )
    {
      uiSymbol = ( 6 > uiMbMode) ? 0 : 1;
      RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 4 ) ) );
      if( uiSymbol )
      {
        uiSymbol = ( 6 == uiMbMode) ? 0 : 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 7 ) ) );

        if( uiSymbol )
        {
          uiIntra16x16Symbol = uiMbMode - 6;
        }
      }
      else
      {
        uiSymbol = (uiMbMode>>1) & 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 5 ) ) );
        if( uiSymbol )
        {
          uiSymbol = 1-(uiMbMode&1);
          RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 7 ) ) );
        }
        else
        {
          uiSymbol = 1-(uiMbMode&1);
          RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 6 ) ) );
        }
      }
    }
    else
    {
      ROFRS( uiMbMode, Err::m_nERR );

      uiMbMode--;
      uiSymbol = ( uiMbMode ? 1 : 0 );
      RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, rcMbDataAccess.getCtxMbType() ) ) );
      if( uiSymbol )
      {
        uiSymbol = ( 3 > uiMbMode) ? 0 : 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 4 ) ) );
        if( uiSymbol )
        {
          uiSymbol = ( 11 > uiMbMode) ? 0 : 1;
          RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 5 ) ) );
          if( uiSymbol )
          {
            if( ( uiSymbol = ( 22 == uiMbMode ) ) || 11 == uiMbMode )
            {
              RNOK( CabaEncoder::writeSymbol(        1, m_cMbTypeCCModel.get( 2, 6 ) ) );
              RNOK( CabaEncoder::writeSymbol(        1, m_cMbTypeCCModel.get( 2, 6 ) ) );
              RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
            }
            else
            {
              if( uiMbMode < 23 )
              {
                uiMbMode -= 12;
              }
              else if( uiMbMode < 24 )
              {
                uiMbMode -= 13;
              }
              else
              {
                uiIntra16x16Symbol = uiMbMode - 23;
                uiMbMode = 11;
              }

              uiSymbol   = (uiMbMode>>3) & 1;
              RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
              uiSymbol   = (uiMbMode>>2) & 1;
              RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
              uiSymbol   = (uiMbMode>>1) & 1;
              RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
              uiSymbol   = uiMbMode & 1;
              RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
            }
          }
          else
          {
            uiMbMode -= 3;
            uiSymbol = (uiMbMode>>2) & 1;
            RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
            uiSymbol = (uiMbMode>>1) & 1;
            RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
            uiSymbol = uiMbMode & 1;
            RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
          }
        }
        else
        {
          uiSymbol = uiMbMode >> 1;
          RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
        }
      }
    }

    if( uiIntra16x16Symbol )
    {
      uiSymbol = ( 25 == uiIntra16x16Symbol) ? 1 : 0;
      RNOK( CabaEncoder::writeTerminatingBit( uiSymbol ) );

      if( ! uiSymbol )
      {
        uiSymbol = ( uiIntra16x16Symbol < 13 ? 0 : 1 );
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 8 ) ) );
        uiIntra16x16Symbol -= ( 12 * uiSymbol + 1 );

        uiSymbol = ( 4 > uiIntra16x16Symbol) ? 0 : 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 9 ) ) );

        if( uiSymbol )
        {
          uiSymbol = ( 8 > uiIntra16x16Symbol) ? 0 : 1;
          RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 9 ) ) );
        }

        uiSymbol = (uiIntra16x16Symbol>>1) & 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 10 ) ) );

        uiSymbol = uiIntra16x16Symbol & 1;
        RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 10 ) ) );
      }
    }
  }

  if( rcMbDataAccess.getMbData().isPCM() )
  {
    m_uiLastDQpNonZero = 0; // no DQP for IPCM
  }

  ETRACE_TH( "MbMode" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( uiOrigMbMode );
  ETRACE_N;

  return Err::m_nOK;
}





ErrVal CabacWriter::xWriteMvdComponent( Short sMvdComp, UInt uiAbsSum, UInt uiCtx )
{
  //--- set context ---
  UInt uiLocalCtx = uiCtx;
  if( uiAbsSum >= 3)
  {
    uiLocalCtx += ( uiAbsSum > 32) ? 3 : 2;
  }

  //--- first symbol: if non-zero ---
  UInt uiSymbol = ( 0 == sMvdComp) ? 0 : 1;
  RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cMvdCCModel.get( 0, uiLocalCtx ) ) );
  ROTRS( 0 == uiSymbol, Err::m_nOK );

  //--- absolute value and sign
  UInt uiSign = 0;
  if( 0 > sMvdComp )
  {
    uiSign   = 1;
    sMvdComp = -sMvdComp;
  }
  RNOK( CabaEncoder::writeExGolombMvd( sMvdComp-1, &m_cMvdCCModel.get( 1, uiCtx ), 3 ) );
  RNOK( CabaEncoder::writeEPSymbol( uiSign ) );

  return Err::m_nOK;
}


ErrVal CabacWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv();
  RNOK( xWriteMvd( rcMbDataAccess, cMv, B4x4Idx(0), eLstIdx ) );
  return Err::m_nOK;
}

ErrVal CabacWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx );
  RNOK( xWriteMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx), eLstIdx ) );
  return Err::m_nOK;
}

ErrVal CabacWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx );
  RNOK( xWriteMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx), eLstIdx ) );
  return Err::m_nOK;
}

ErrVal CabacWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx );
  RNOK( xWriteMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx), eLstIdx ) );
  return Err::m_nOK;
}

ErrVal CabacWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx, eSParIdx );
  RNOK( xWriteMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx+eSParIdx), eLstIdx ) );
  return Err::m_nOK;
}

ErrVal CabacWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx, eSParIdx );
  RNOK( xWriteMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx+eSParIdx), eLstIdx ) );
  return Err::m_nOK;
}

ErrVal CabacWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx, eSParIdx );
  RNOK( xWriteMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx+eSParIdx), eLstIdx ) );
  return Err::m_nOK;
}


ErrVal CabacWriter::xWriteMvd( MbDataAccess& rcMbDataAccess, Mv cMv, LumaIdx cIdx, ListIdx eLstIdx )
{
  Mv    cMvA;
  Mv    cMvB;

  rcMbDataAccess.getMvdAbove( cMvA, eLstIdx, cIdx );
  rcMbDataAccess.getMvdLeft ( cMvB, eLstIdx, cIdx );

  Short sHor = cMv.getHor();
  Short sVer = cMv.getVer();

  RNOK( xWriteMvdComponent( sHor, cMvA.getAbsHor() + cMvB.getAbsHor(), 0 ) );

  ETRACE_TH( "Mvd: x" );
  ETRACE_TY( "ae(v)" );
  ETRACE_V( sHor );
  ETRACE_TH( " above " );
  ETRACE_V( cMvA.getHor() );
  ETRACE_TH( " left " );
  ETRACE_V( cMvB.getHor() );
  ETRACE_N;

  RNOK( xWriteMvdComponent( sVer, cMvA.getAbsVer() + cMvB.getAbsVer(), 5 ) );

  ETRACE_TH( "Mvd: y" );
  ETRACE_TY( "ae(v)" );
  ETRACE_V( sVer );
  ETRACE_TH( " above " );
  ETRACE_V( cMvA.getVer() );
  ETRACE_TH( " left " );
  ETRACE_V( cMvB.getVer() );
  ETRACE_N;

  return Err::m_nOK;
}




ErrVal CabacWriter::intraPredModeChroma( MbDataAccess& rcMbDataAccess )
{
  UInt uiCtx = rcMbDataAccess.getCtxChromaPredMode();
  UInt uiIntraPredModeChroma = rcMbDataAccess.getMbData().getChromaPredMode();

  if( 0 == uiIntraPredModeChroma )
  {
    CabaEncoder::writeSymbol( 0, m_cChromaPredCCModel.get( 0, uiCtx ) );
  }
  else
  {
    CabaEncoder::writeSymbol( 1, m_cChromaPredCCModel.get( 0, uiCtx ) );

    CabaEncoder::writeUnaryMaxSymbol( uiIntraPredModeChroma - 1,
                                          m_cChromaPredCCModel.get( 0 ) + 3,
                                          0, 2 );

  }

  ETRACE_TH( "IntraPredModeChroma" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( uiIntraPredModeChroma );
  ETRACE_N;

  return Err::m_nOK;
}

ErrVal CabacWriter::intraPredModeLuma( MbDataAccess& rcMbDataAccess, LumaIdx cIdx )
{
  Int iIntraPredModeLuma = rcMbDataAccess.encodeIntraPredMode(cIdx);

  RNOK( CabaEncoder::writeSymbol( iIntraPredModeLuma >= 0 ? 0 : 1, m_cIntraPredCCModel.get( 0, 0 ) ) );
  if( iIntraPredModeLuma >= 0 )
  {
    RNOK( CabaEncoder::writeSymbol( (iIntraPredModeLuma & 0x01)     , m_cIntraPredCCModel.get( 0, 1 ) ) );
    RNOK( CabaEncoder::writeSymbol( (iIntraPredModeLuma & 0x02) >> 1, m_cIntraPredCCModel.get( 0, 1 ) ) );
    RNOK( CabaEncoder::writeSymbol( (iIntraPredModeLuma & 0x04) >> 2, m_cIntraPredCCModel.get( 0, 1 ) ) );
  }

  ETRACE_TH( "IntraPredModeLuma" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( iIntraPredModeLuma );
  ETRACE_N;

  return Err::m_nOK;
}


ErrVal CabacWriter::cbp( MbDataAccess& rcMbDataAccess, UInt uiStart, UInt uiStop )
{
  UInt uiCbp = rcMbDataAccess.getMbData().calcMbCbp( uiStart, uiStop );
  UInt uiCtx = 0, a, b;

  a = rcMbDataAccess.getLeftLumaCbp ( B4x4Idx( 0 ), uiStart, uiStop, true );
  b = rcMbDataAccess.getAboveLumaCbp( B4x4Idx( 0 ), uiStart, uiStop, true ) << 1;

  RNOK( CabaEncoder::writeSymbol( uiCbp & 1, m_cCbpCCModel.get( uiCtx, 3 - (a + b) ) ) );

  a = uiCbp & 1;
  b = rcMbDataAccess.getAboveLumaCbp( B4x4Idx( 2 ), uiStart, uiStop, true ) << 1;

  RNOK( CabaEncoder::writeSymbol( (uiCbp>>1) & 1, m_cCbpCCModel.get( uiCtx, 3 - (a + b) ) ) );

  a = rcMbDataAccess.getLeftLumaCbp ( B4x4Idx( 8 ), uiStart, uiStop, true );
  b = (uiCbp  << 1) & 2;

  RNOK( CabaEncoder::writeSymbol( (uiCbp>>2) & 1, m_cCbpCCModel.get( uiCtx, 3 - (a + b) ) ) );

  a = ( uiCbp >> 2 ) & 1;
  b = uiCbp & 2;

  RNOK( CabaEncoder::writeSymbol( (uiCbp>>3) & 1, m_cCbpCCModel.get( uiCtx, 3 - (a + b) ) ) );


  uiCtx = 1;

  UInt  uiLeftChromaCbp   = rcMbDataAccess.getLeftChromaCbp ( uiStart, uiStop, true );
  UInt  uiAboveChromaCbp  = rcMbDataAccess.getAboveChromaCbp( uiStart, uiStop, true );

  a = uiLeftChromaCbp  > 0 ? 1 : 0;
  b = uiAboveChromaCbp > 0 ? 2 : 0;

  UInt uiBit = ( 0 == (uiCbp>>4)) ? 0 : 1;

  RNOK( CabaEncoder::writeSymbol( uiBit, m_cCbpCCModel.get( uiCtx, a + b ) ) );

  if( uiBit )
  {
    a = uiLeftChromaCbp  > 1 ? 1 : 0;
    b = uiAboveChromaCbp > 1 ? 2 : 0;

    uiBit = ( 0 == (uiCbp>>5)) ? 0 : 1;

    RNOK( CabaEncoder::writeSymbol( uiBit, m_cCbpCCModel.get( ++uiCtx, a + b ) ) );
  }

  if( !uiCbp )
  {
    m_uiLastDQpNonZero = 0;
  }

  AOF_DBG( 48 >= uiCbp );

  ETRACE_TH( "Cbp" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( uiCbp );
  ETRACE_N;

  return Err::m_nOK;
}


ErrVal CabacWriter::residualBlock( MbDataAccess&  rcMbDataAccess,
                                   LumaIdx        cIdx,
                                   ResidualMode   eResidualMode,
                                   UInt           uiStart,
                                   UInt           uiStop )
{
  const Bool bFrame    = ( FRAME == rcMbDataAccess.getMbPicType());
  const UChar* pucScan = ( eResidualMode==LUMA_I16_DC ? ((bFrame) ? g_aucLumaFrameDCScan : g_aucLumaFieldDCScan) : ((bFrame) ? g_aucFrameScan : g_aucFieldScan) );

  ETRACE_TH( "LUMA:" );
  ETRACE_V( cIdx );
  ETRACE_N;

  TCoeff* piCoeff = rcMbDataAccess.getMbTCoeffs().get( cIdx );

  ROT( uiStart == uiStop );
  UInt uiNumSig = xGetNumberOfSigCoeff( piCoeff, eResidualMode, pucScan, uiStart, uiStop );
  RNOK( xWriteBCbp( rcMbDataAccess, uiNumSig, eResidualMode, cIdx, uiStart, uiStop ) );

  if( uiNumSig )
  {
    RNOK( xWriteCoeff( uiNumSig, piCoeff, eResidualMode, pucScan, bFrame, uiStart, uiStop ) );
  }
  return Err::m_nOK;
}



ErrVal CabacWriter::residualBlock( MbDataAccess&  rcMbDataAccess,
                                   ChromaIdx      cIdx,
                                   ResidualMode   eResidualMode,
                                   UInt           uiStart,
                                   UInt           uiStop )
{
  const Bool bFrame    = ( FRAME == rcMbDataAccess.getMbPicType());
  const UChar* pucScan;

  switch( eResidualMode )
  {
  case CHROMA_DC:
    {
      ETRACE_TH( "CHROMA_DC:" );
      pucScan = g_aucIndexChromaDCScan;
      break;
    }
  case CHROMA_AC:
    {
      ETRACE_TH( "CHROMA_AC:" );
      pucScan = (bFrame ? g_aucFrameScan : g_aucFieldScan);
      break;
    }
  default:
    {
      AF();
      return Err::m_nERR;
    }
  }
  ETRACE_V( cIdx );
  ETRACE_N;

  TCoeff* piCoeff = rcMbDataAccess.getMbTCoeffs().get( cIdx );

  ROT( uiStart == uiStop );
  UInt uiNumSig = xGetNumberOfSigCoeff( piCoeff, eResidualMode, pucScan, uiStart, uiStop );
  RNOK( xWriteBCbp( rcMbDataAccess, uiNumSig, eResidualMode, cIdx, uiStart, uiStop ) );

  if( uiNumSig )
  {
    RNOK( xWriteCoeff( uiNumSig, piCoeff, eResidualMode, pucScan, bFrame, uiStart, uiStop ) );
  }

  return Err::m_nOK;
}



ErrVal CabacWriter::xWriteBCbp( MbDataAccess& rcMbDataAccess, UInt uiNumSig, ResidualMode eResidualMode, LumaIdx cIdx, UInt uiStart, UInt uiStop )
{
  UInt uiBitPos = 0;

  if( LUMA_SCAN == eResidualMode || LUMA_I16_AC == eResidualMode )
  {
    uiBitPos = cIdx;
  }
  else if( LUMA_I16_DC == eResidualMode )
  {
    uiBitPos = 26;
  }
  else
  {
    // full stop
    AF();
  }
  UInt uiCtx  = rcMbDataAccess.getCtxCodedBlockBit( uiBitPos, uiStart, uiStop, true );
  UInt uiBit  = uiNumSig ? 1 : 0;

  RNOK( CabaEncoder::writeSymbol( uiBit, m_cBCbpCCModel.get( type2ctx1[ eResidualMode ], uiCtx) ) );

  rcMbDataAccess.getMbData().setBCBP( uiBitPos, uiBit);

  return Err::m_nOK;
}

ErrVal CabacWriter::xWriteBCbp( MbDataAccess& rcMbDataAccess, UInt uiNumSig, ResidualMode eResidualMode, ChromaIdx cIdx, UInt uiStart, UInt uiStop )
{
  UInt uiBitPos;

  if( CHROMA_AC == eResidualMode )
  {
    uiBitPos = 16 + cIdx;
  }
  else if( CHROMA_DC == eResidualMode )
  {
    uiBitPos = 24 + cIdx.plane();
  }
  else
  {
    AF();
    return Err::m_nERR;
  }

  UInt uiCtx      = rcMbDataAccess.getCtxCodedBlockBit( uiBitPos, uiStart, uiStop, true );
  UInt uiBit      = uiNumSig ? 1 : 0;

  RNOK( CabaEncoder::writeSymbol( uiBit, m_cBCbpCCModel.get( type2ctx1[ eResidualMode ], uiCtx) ) );

  rcMbDataAccess.getMbData().setBCBP( uiBitPos, uiBit);

  return Err::m_nOK;
}

UInt CabacWriter::xGetNumberOfSigCoeff( TCoeff* piCoeff, ResidualMode eResidualMode, const UChar* pucScan, UInt uiStart, UInt uiStop )
{
  UInt uiNumSig = 0;
  if( CHROMA_DC == eResidualMode || LUMA_I16_DC == eResidualMode )
  {
    if( uiStart == 0 && uiStop > 0 )
    {
      // process all DC coefficients if the range uiStart to uiStop demands the processing of scanpos 0
      uiStop  = ( CHROMA_DC == eResidualMode ? 4 : 16 );
    }
    else
    {
      return 0;
    }
  }
  else if( CHROMA_AC == eResidualMode || LUMA_I16_AC == eResidualMode )
  {
    if( uiStop > 1 )
    {
      uiStart = gMax( 1, uiStart );
    }
    else
    {
      return 0;
    }
  }

  for( UInt ui = uiStart; ui < uiStop; ui++ )
  {
    if( piCoeff[ pucScan[ ui ] ] )
    {
      uiNumSig++;
    }
  }

  return uiNumSig;
}


ErrVal CabacWriter::xWriteCoeff( UInt         uiNumSig,
                                 TCoeff*      piCoeff,
                                 ResidualMode eResidualMode,
                                 const UChar* pucScan,
                                 Bool         bFrame,
                                 UInt         uiStart,
                                 UInt         uiStop )
{
  CabacContextModel2DBuffer&  rcMapCCModel  = (bFrame ? m_cMapCCModel : m_cFldMapCCModel );
  CabacContextModel2DBuffer&  rcLastCCModel = (bFrame ? m_cLastCCModel: m_cFldLastCCModel);
  UInt uiCodedSig = 0;

  if( CHROMA_DC == eResidualMode || LUMA_I16_DC == eResidualMode )
  {
    if( uiStart == 0 && uiStop > 0 )
    {
      uiStop = ( CHROMA_DC == eResidualMode ? 3 : 15 );
    }
    else
    {
      ROT( 1 );
    }
  }
  else
  {
    if( CHROMA_AC == eResidualMode || LUMA_I16_AC == eResidualMode )
    {
      if( uiStop > 1 )
      {
        uiStart = gMax( 1, uiStart );
      }
      else
      {
        ROT( 1 );
      }
    }
    uiStop -= 1;
  }

  UInt ui;
  //----- encode significance map -----
  for( ui = uiStart; ui < uiStop; ui++ ) // if last coeff is reached, it has to be significant
  {
    UInt uiSig = piCoeff[ pucScan[ ui ] ] ? 1 : 0;
    RNOK( CabaEncoder::writeSymbol( uiSig, rcMapCCModel.get( type2ctx2 [eResidualMode], ui ) ) );

    if( uiSig )
    {
      UInt uiLast = (++uiCodedSig == uiNumSig ? 1 : 0);

      RNOK( CabaEncoder::writeSymbol( uiLast, rcLastCCModel.get( type2ctx2 [eResidualMode], ui ) ) );

      if( uiLast)
      {
        break;
      }
    }
  }

  int   c1 = 1;
  int   c2 = 0;
  //----- encode significant coefficients -----
  ui++;
  while( (ui--) != uiStart )
  {
    UInt  uiAbs, uiSign;
    Int   iCoeff = piCoeff[ pucScan[ ui ] ];

    if( iCoeff )
    {
      if( iCoeff > 0) { uiAbs = static_cast<UInt>( iCoeff);  uiSign = 0; }
      else            { uiAbs = static_cast<UInt>(-iCoeff);  uiSign = 1; }

      UInt uiCtx    = gMin (c1, 4);
      UInt uiSymbol = uiAbs > 1 ? 1 : 0;
      RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cOneCCModel.get( type2ctx1 [eResidualMode], uiCtx ) ) );

      if( uiSymbol )
      {
        uiCtx  = gMin (c2,4);
        uiAbs -= 2;
        c1     = 0;
        c2++;
        RNOK( CabaEncoder::writeExGolombLevel( uiAbs, m_cAbsCCModel.get( type2ctx1 [eResidualMode], uiCtx ) ) );
      }
      else if( c1 )
      {
        c1++;
      }
      RNOK( CabaEncoder::writeEPSymbol( uiSign ) );
    }
  }

  return Err::m_nOK;
}


ErrVal CabacWriter::terminatingBit ( UInt uiIsLast )
{
  RNOK( CabaEncoder::writeTerminatingBit( uiIsLast ) );

  ETRACE_TH( "EOS" );
  ETRACE_CODE( uiIsLast );
  ETRACE_N;

  return Err::m_nOK;
}


ErrVal CabacWriter::deltaQp( MbDataAccess& rcMbDataAccess )
{
  Int   iDQp  = rcMbDataAccess.getDeltaQp();
  UInt  uiCtx = m_uiLastDQpNonZero;
  UInt  uiDQp = ( iDQp ? 1 : 0 );

  RNOK( CabaEncoder::writeSymbol( uiDQp, m_cDeltaQpCCModel.get( 0, uiCtx ) ) );

  m_uiLastDQpNonZero = ( 0 != uiDQp ? 1 : 0 );

  if( uiDQp )
  {
    uiDQp = (UInt)( iDQp > 0 ? ( 2 * iDQp - 2 ) : ( -2 * iDQp - 1 ) );
    RNOK( CabaEncoder::writeUnarySymbol( uiDQp, &m_cDeltaQpCCModel.get( 0, 2 ), 1 ) );
  }

  ETRACE_TH( "DQp" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE ( iDQp );
  ETRACE_N;
  return Err::m_nOK;
}


ErrVal CabacWriter::samplesPCM( MbDataAccess& rcMbDataAccess )
{
  ETRACE_POS;
  ETRACE_TH( "  PCM SAMPLES: " );

  RNOK( CabaEncoder::finish() );
  RNOK( m_pcBitWriteBufferIf->write(1) );
  RNOK( m_pcBitWriteBufferIf->writeAlignZero() );

  rcMbDataAccess.getMbData().setBCBPAll( 1 );

  AOF_DBG( rcMbDataAccess.getMbData().isPCM() );

  TCoeff* pCoeff = rcMbDataAccess.getMbTCoeffs().getTCoeffBuffer();

  const UInt uiFactor = 8*8;
  const UInt uiSize   = uiFactor*2*3;
  RNOK( m_pcBitWriteBufferIf->pcmSamples( pCoeff, uiSize ) );

  ETRACE_N;
  RNOK( CabaEncoder::start() );

  return Err::m_nOK;
}

UInt CabacWriter::getNumberOfWrittenBits()
{
  return CabaEncoder::getWrittenBits();
}


ErrVal CabacWriter::transformSize8x8Flag( MbDataAccess& rcMbDataAccess, UInt uiStart, UInt uiStop )
{
  UInt uiSymbol = rcMbDataAccess.getMbData().isTransformSize8x8() ? 1 : 0;
  UInt uiCtx = rcMbDataAccess.getCtx8x8Flag( uiStart, uiStop );

  RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cTransSizeCCModel.get( 0, uiCtx ) ) );

  ETRACE_TH( "transformSize8x8Flag:" );
  ETRACE_TY( "ae(v)" );
  ETRACE_CODE( rcMbDataAccess.getMbData().isTransformSize8x8() );
  ETRACE_N;

  return Err::m_nOK;
}


ErrVal CabacWriter::residualBlock8x8( MbDataAccess& rcMbDataAccess,
                                      B8x8Idx       c8x8Idx,
                                      ResidualMode  eResidualMode,
                                      UInt          uiStart,
                                      UInt          uiStop )
{
  const Bool bFrame    = ( FRAME == rcMbDataAccess.getMbPicType());
  const UChar* pucScan = (bFrame) ? g_aucFrameScan64 : g_aucFieldScan64;

  switch( eResidualMode )
  {
  case LUMA_SCAN:
    {
      break;
    }
  default:
    {
      AF();
      return Err::m_nERR;
    }
  }
  ETRACE_TH( "LUMA_8x8:" );
  ETRACE_V( c8x8Idx.b8x8Index() );
  ETRACE_N;

  TCoeff* piCoeff = rcMbDataAccess.getMbTCoeffs().get8x8( c8x8Idx );

  UInt ui;
  UInt uiNumSig = 0;

  ROT( uiStart == uiStop );

  uiStart <<= 2;
  uiStop  <<= 2;
  for( UInt uiC = uiStart; uiC < uiStop; uiC++ )
  {
    if( piCoeff[pucScan[uiC]] )
    {
      uiNumSig++;
    }
  }


  UInt uiBitPos = c8x8Idx.b4x4();
  rcMbDataAccess.getMbData().setBCBP( uiBitPos, 1);
  rcMbDataAccess.getMbData().setBCBP( uiBitPos+1, 1);
  rcMbDataAccess.getMbData().setBCBP( uiBitPos+4, 1);
  rcMbDataAccess.getMbData().setBCBP( uiBitPos+5, 1);

  const UInt uiCtxOffset = 2;
  CabacContextModel2DBuffer&  rcMapCCModel  = ( bFrame ? m_cMapCCModel  : m_cFldMapCCModel);
  CabacContextModel2DBuffer&  rcLastCCModel = ( bFrame ? m_cLastCCModel : m_cFldLastCCModel);

  UInt uiCodedSig = 0;
  const Int* pos2ctx_map = (bFrame) ? pos2ctx_map8x8 : pos2ctx_map8x8i;

  //----- encode significance map -----
  for( ui = uiStart; ui < uiStop - 1; ui++ ) // if last coeff is reached, it has to be significant
  {
    UInt uiSig = piCoeff[ pucScan[ ui ] ] ? 1 : 0;
      RNOK( CabaEncoder::writeSymbol( uiSig, rcMapCCModel.get( uiCtxOffset, pos2ctx_map[ui] ) ) );

    if( uiSig )
    {
      UInt uiLast = (++uiCodedSig == uiNumSig ? 1 : 0);
        RNOK( CabaEncoder::writeSymbol( uiLast, rcLastCCModel.get( uiCtxOffset, pos2ctx_last8x8[ui] ) ) );

      if( uiLast)
      {
        break;
      }
    }
  }

  int   c1 = 1;
  int   c2 = 0;
  //----- encode significant coefficients -----
  ui++;
  while( (ui--) != uiStart )
  {
    UInt  uiAbs, uiSign;
    Int   iCoeff = piCoeff[ pucScan[ ui ] ];

    if( iCoeff )
    {
      if( iCoeff > 0) { uiAbs = static_cast<UInt>( iCoeff);  uiSign = 0; }
      else            { uiAbs = static_cast<UInt>(-iCoeff);  uiSign = 1; }

      UInt uiCtx    = gMin (c1, 4);
      UInt uiSymbol = uiAbs > 1 ? 1 : 0;
      RNOK( CabaEncoder::writeSymbol( uiSymbol, m_cOneCCModel.get( uiCtxOffset, uiCtx ) ) );

      if( uiSymbol )
      {
        uiCtx  = gMin (c2,4);
        uiAbs -= 2;
        c1     = 0;
        c2++;
        RNOK( CabaEncoder::writeExGolombLevel( uiAbs, m_cAbsCCModel.get( uiCtxOffset, uiCtx ) ) );
      }
      else if( c1 )
      {
        c1++;
      }
      RNOK( CabaEncoder::writeEPSymbol( uiSign ) );
    }
  }
  return Err::m_nOK;
}

//JVT-X046 {
void
CabacWriter::loadCabacWrite(MbSymbolWriteIf *pcMbSymbolWriteIf)
{
	CabacWriter * pcCabacWriter = (CabacWriter*) (pcMbSymbolWriteIf);
	m_cFieldFlagCCModel.setCabacContextModel(pcCabacWriter->getFieldFlagCCModel().getCabacContextModel());
	m_cFldMapCCModel.setCabacContextModel(pcCabacWriter->getFldLastCCModel().getCabacContextModel());
	m_cFldLastCCModel.setCabacContextModel(pcCabacWriter->getFldLastCCModel().getCabacContextModel());
	m_cBLSkipCCModel.setCabacContextModel(pcCabacWriter->getBLSkipCCModel().getCabacContextModel());

	m_cBCbpCCModel.setCabacContextModel(pcCabacWriter->getBCbpCCModel().getCabacContextModel());
	m_cMapCCModel.setCabacContextModel(pcCabacWriter->getMapCCModel().getCabacContextModel());
	m_cLastCCModel.setCabacContextModel(pcCabacWriter->getLastCCModel().getCabacContextModel());

	m_cOneCCModel.setCabacContextModel(pcCabacWriter->getOneCCModel().getCabacContextModel());
	m_cAbsCCModel.setCabacContextModel(pcCabacWriter->getAbsCCModel().getCabacContextModel());
	m_cChromaPredCCModel.setCabacContextModel(pcCabacWriter->getChromaPredCCModel().getCabacContextModel());

	m_cMbTypeCCModel.setCabacContextModel(pcCabacWriter->getMbTypeCCModel().getCabacContextModel());
	m_cBlockTypeCCModel.setCabacContextModel(pcCabacWriter->getBlockTypeCCModel().getCabacContextModel());
	m_cMvdCCModel.setCabacContextModel(pcCabacWriter->getMvdCCModel().getCabacContextModel());
	m_cRefPicCCModel.setCabacContextModel(pcCabacWriter->getRefPicCCModel().getCabacContextModel());
  m_cMotPredFlagCCModel.setCabacContextModel(pcCabacWriter->getMotPredFlagCCModel().getCabacContextModel());
	m_cResPredFlagCCModel.setCabacContextModel(pcCabacWriter->getResPredFlagCCModel().getCabacContextModel());
	m_cDeltaQpCCModel.setCabacContextModel(pcCabacWriter->getDeltaQpCCModel().getCabacContextModel());
	m_cIntraPredCCModel.setCabacContextModel(pcCabacWriter->getIntraPredCCModel().getCabacContextModel());
	m_cCbpCCModel.setCabacContextModel(pcCabacWriter->getCbpCCModel().getCabacContextModel());
	m_cTransSizeCCModel.setCabacContextModel(pcCabacWriter->getTransSizeCCModel().getCabacContextModel());
	m_uiRange        = pcCabacWriter->m_uiRange;
	m_uiLow          = pcCabacWriter->m_uiLow;
	m_uiByte         = pcCabacWriter->m_uiByte;
	m_uiBitsLeft     = pcCabacWriter->m_uiBitsLeft;
	m_uiBitsToFollow = pcCabacWriter->m_uiBitsToFollow;

  AOF( m_pcBitWriteBufferIf );
	m_pcBitWriteBufferIf->loadBitWriteBuffer(pcCabacWriter->getBitWriteBufferIf());
}
//JVT-X046 }


H264AVC_NAMESPACE_END
