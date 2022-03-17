
#include "H264AVCDecoderLib.h"
#include "H264AVCCommonLib/Tables.h"
#include "H264AVCCommonLib/ContextTables.h"
#include "H264AVCCommonLib/CabacContextModel2DBuffer.h"
#include "H264AVCCommonLib/TraceFile.h"
#include "H264AVCCommonLib/Transform.h"
#include "CabaDecoder.h"
#include "CabacReader.h"
#include "BitReadBuffer.h"
#include "DecError.h"

H264AVC_NAMESPACE_BEGIN

const int MAX_COEFF[9] = {8,16,16,16,15,4,4,15,15};
const int COUNT_THR[9] = { 3, 4, 4, 4, 3, 2, 2, 3, 3};

CabacReader::CabacReader():
  m_cFieldFlagCCModel   ( 1,                3),
  m_cFldMapCCModel      ( NUM_BLOCK_TYPES,  NUM_MAP_CTX),
  m_cFldLastCCModel     ( NUM_BLOCK_TYPES,  NUM_LAST_CTX),
  m_cBCbpCCModel        ( NUM_BLOCK_TYPES,  NUM_BCBP_CTX ),
  m_cMapCCModel         ( NUM_BLOCK_TYPES,  NUM_MAP_CTX ),
  m_cLastCCModel        ( NUM_BLOCK_TYPES,  NUM_LAST_CTX ),
  m_cOneCCModel         ( NUM_BLOCK_TYPES,  NUM_ABS_CTX ),
  m_cAbsCCModel         ( NUM_BLOCK_TYPES,  NUM_ABS_CTX ),
  m_cChromaPredCCModel  ( 1,                4 ),
  m_cBLSkipCCModel      ( 1,                NUM_BL_SKIP_FLAG_CTX ),
  m_cMbTypeCCModel      ( 3,                NUM_MB_TYPE_CTX ),
  m_cBlockTypeCCModel   ( 2,                NUM_B8_TYPE_CTX ),
  m_cMvdCCModel         ( 2,                NUM_MV_RES_CTX ),
  m_cRefPicCCModel      ( 2,                NUM_REF_NO_CTX ),
  m_cMotPredFlagCCModel ( 1,                NUM_MOT_PRED_FLAG_CTX ),
  m_cResPredFlagCCModel ( 1,                NUM_RES_PRED_FLAG_CTX ),
  m_cDeltaQpCCModel     ( 1,                NUM_DELTA_QP_CTX ),
  m_cIntraPredCCModel   ( 9,                NUM_IPR_CTX ),
  m_cCbpCCModel         ( 3,                NUM_CBP_CTX ),
  m_cTransSizeCCModel   ( 1,                NUM_TRANSFORM_SIZE_CTX ),
  m_uiBitCounter        ( 0 ),
  m_uiPosCounter        ( 0 ),
  m_uiLastDQpNonZero    ( 0 )
{
}


CabacReader::~CabacReader()
{
}


ErrVal
CabacReader::xInitContextModels( const SliceHeader& rcSliceHeader )
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
    RNOK( m_cFldMapCCModel.initBuffer(      (Short*)INIT_FLD_MAP_P        [iIndex], iQp ) );
    RNOK( m_cFldLastCCModel.initBuffer(     (Short*)INIT_FLD_LAST_P       [iIndex], iQp ) );
  }

  return Err::m_nOK;
}


ErrVal
CabacReader::create( CabacReader*& rpcCabacReader )
{
  rpcCabacReader = new CabacReader;
  ROF( rpcCabacReader );
  return Err::m_nOK;
}

ErrVal
CabacReader::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal
CabacReader::init( BitReadBuffer* pcBitReadBuffer )
{
  ROF ( pcBitReadBuffer );
  RNOK( CabaDecoder::init( pcBitReadBuffer ) );
  return Err::m_nOK;
}


ErrVal
CabacReader::uninit()
{
  RNOK( CabaDecoder::uninit() );
  return Err::m_nOK;
}


ErrVal
CabacReader::startSlice( const SliceHeader& rcSliceHeader )
{
  m_uiLastDQpNonZero  = 0;
  ROTRS( rcSliceHeader.getSliceSkipFlag(), Err::m_nOK );
  RNOK( xInitContextModels( rcSliceHeader ) );
  RNOK( CabaDecoder::start() );
  return Err::m_nOK;
}


ErrVal CabacReader::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  UInt uiRefFrame;
  DECRNOK( xRefFrame( rcMbDataAccess, uiRefFrame, eLstIdx, PART_8x8_0 ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setRefIdx( uiRefFrame );
  return Err::m_nOK;
}


ErrVal CabacReader::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  )
{
  UInt uiRefFrame;
  DECRNOK( xRefFrame( rcMbDataAccess, uiRefFrame, eLstIdx, ParIdx8x8( eParIdx ) ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setRefIdx( uiRefFrame, eParIdx );
  return Err::m_nOK;
}

ErrVal CabacReader::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  )
{
  UInt uiRefFrame;
  DECRNOK( xRefFrame( rcMbDataAccess, uiRefFrame, eLstIdx, ParIdx8x8( eParIdx ) ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setRefIdx( uiRefFrame, eParIdx );
  return Err::m_nOK;
}

ErrVal CabacReader::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  )
{
  UInt uiRefFrame;
  DECRNOK( xRefFrame( rcMbDataAccess, uiRefFrame, eLstIdx, eParIdx ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setRefIdx( uiRefFrame, eParIdx );
  return Err::m_nOK;
}


ErrVal CabacReader::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  Bool  bFlag;
  DECRNOK( xMotionPredFlag( bFlag, eLstIdx ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setMotPredFlag( bFlag );
  return Err::m_nOK;
}
ErrVal CabacReader::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  )
{
  Bool  bFlag;
  DECRNOK( xMotionPredFlag( bFlag, eLstIdx ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setMotPredFlag( bFlag, eParIdx );
  return Err::m_nOK;
}
ErrVal CabacReader::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  )
{
  Bool  bFlag;
  DECRNOK( xMotionPredFlag( bFlag, eLstIdx ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setMotPredFlag( bFlag, eParIdx );
  return Err::m_nOK;
}
ErrVal CabacReader::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8 eParIdx  )
{
  Bool  bFlag;
  DECRNOK( xMotionPredFlag( bFlag, eLstIdx ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setMotPredFlag( bFlag, eParIdx );
  return Err::m_nOK;
}



ErrVal CabacReader::xRefFrame( MbDataAccess& rcMbDataAccess, UInt& ruiRefFrame, ListIdx eLstIdx, ParIdx8x8 eParIdx )
{
  UInt uiCtx = rcMbDataAccess.getCtxRefIdx( eLstIdx, eParIdx );

  RNOK( CabaDecoder::getSymbol( ruiRefFrame, m_cRefPicCCModel.get( 0, uiCtx ) ) );

  if ( 0 != ruiRefFrame )
  {
    RNOK( CabaDecoder::getUnarySymbol( ruiRefFrame, &m_cRefPicCCModel.get( 0, 4 ), 1 ) );
    ruiRefFrame++;
  }

  ruiRefFrame++;
  DTRACE_TH( "RefFrame" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE(ruiRefFrame-1);
  DTRACE_N;

  return Err::m_nOK;
}


ErrVal CabacReader::xMotionPredFlag(  Bool& bFlag, ListIdx eLstIdx )
{
  UInt  uiCode;

  RNOK( CabaDecoder::getSymbol( uiCode, m_cMotPredFlagCCModel.get( 0, eLstIdx ) ) );

  bFlag = ( uiCode != 0 );

  DTRACE_TH( "MotionPredFlag" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( uiCode );
  DTRACE_N;

  return Err::m_nOK;
}



ErrVal CabacReader::blockModes( MbDataAccess& rcMbDataAccess )
{
  UInt uiSymbol;
  for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
  {
    UInt uiBlockMode = 0;

    if( ! rcMbDataAccess.getSH().isBSlice() )
    {
      RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 0, 1 ) ) );
      if( 0 == uiSymbol )
      {
        RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 0, 3 ) ) );
        if( 0 != uiSymbol )
        {
          RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 0, 4 ) ) );
          uiBlockMode = ( 0 != uiSymbol) ? 2 : 3;
        }
        else
        {
          uiBlockMode = 1;
        }
      }
    }
    else
    {
      RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 0 ) ) );
      if( 0 != uiSymbol )
      {
        RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 1 ) ) );
        if( 0 != uiSymbol )
        {
          RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 2 ) ) );
          if( 0 != uiSymbol )
          {
            RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
            if( 0 != uiSymbol )
            {
              uiBlockMode = 10;
              RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
              uiBlockMode += uiSymbol;
            }
            else
            {
              uiBlockMode = 6;
              RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
              uiBlockMode += uiSymbol << 1;
              RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
              uiBlockMode += uiSymbol;
            }
          }
          else
          {
            uiBlockMode = 2;
            RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
            uiBlockMode += uiSymbol << 1;
            RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
            uiBlockMode += uiSymbol;
          }
        }
        else
        {
          RNOK( CabaDecoder::getSymbol( uiSymbol, m_cBlockTypeCCModel.get( 1, 3 ) ) );
          uiBlockMode = uiSymbol;
        }
        uiBlockMode++;
      }
    }

    DTRACE_TH( "BlockMode" );
    DTRACE_TY( "ae(v)" );
    DTRACE_CODE(uiBlockMode);
    DTRACE_N;

    rcMbDataAccess.setConvertBlkMode( c8x8Idx.b8x8Index(), uiBlockMode );
  }

  return Err::m_nOK;
}



Bool CabacReader::isMbSkipped( MbDataAccess& rcMbDataAccess, UInt& uiNextSkippedVLC )
{
  uiNextSkippedVLC = 0;
  ROTRS( rcMbDataAccess.getSH().isIntraSlice(), false );
  UInt uiSymbol;

  UInt uiCtx = rcMbDataAccess.getCtxMbSkipped();
  if( rcMbDataAccess.getSH().isBSlice() )
  {
    CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 7 + uiCtx ) );
  }
  else
  {
    CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, uiCtx ) );
  }
  rcMbDataAccess.getMbData().setSkipFlag(0!=uiSymbol);


  ROTRS( 0 == uiSymbol, false );
  m_uiLastDQpNonZero = 0; // no DeltaQP for Skipped Macroblock
  DTRACE_TH( "MbMode" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( 0 );
  DTRACE_N;
  return true;
}


Bool CabacReader::isBLSkipped( MbDataAccess& rcMbDataAccess )
{
  UInt uiSymbol = 0;
  UInt uiCtx    = rcMbDataAccess.getCtxBLSkipFlag();

  CabaDecoder::getSymbol( uiSymbol, m_cBLSkipCCModel.get( 0, uiCtx ) );

  DTRACE_TH( "BLSkipFlag" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( uiSymbol );
  DTRACE_N;

  rcMbDataAccess.getMbData().setBLSkipFlag( ( uiSymbol != 0 ) );
  return rcMbDataAccess.getMbData().getBLSkipFlag();
}

ErrVal CabacReader::resPredFlag( MbDataAccess& rcMbDataAccess )
{
  UInt  uiSymbol;

  UInt  uiCtx = ( rcMbDataAccess.getMbData().getBLSkipFlag() ? 0 : 1 );

  RNOK( CabaDecoder::getSymbol( uiSymbol, m_cResPredFlagCCModel.get( 0, uiCtx ) ) );
  rcMbDataAccess.getMbData().setResidualPredFlag( (uiSymbol!=0) );

  DTRACE_TH( "ResidualPredFlag" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( uiSymbol );
  DTRACE_N;

  return Err::m_nOK;
}


ErrVal CabacReader::mbMode( MbDataAccess& rcMbDataAccess )
{
  rcMbDataAccess.getMbData().setBCBPAll( 0 );

  UInt uiMbMode;
  UInt act_sym;
  UInt mod_sym;

  if( rcMbDataAccess.getSH().isIntraSlice() )
  {
    RNOK( CabaDecoder::getSymbol( act_sym, m_cMbTypeCCModel.get( 0, rcMbDataAccess.getCtxMbIntra4x4() ) ) );

    if( 0 != act_sym )
    {
      RNOK( CabaDecoder::getTerminateBufferBit( act_sym ) )
      if( 0 != act_sym )
      {
        act_sym = 25;
      }
      else
      {
        RNOK( CabaDecoder::getSymbol( act_sym, m_cMbTypeCCModel.get( 0, 4 ) ) );
        act_sym = 12* act_sym + 1;

        RNOK( CabaDecoder::getSymbol( mod_sym, m_cMbTypeCCModel.get( 0, 5 ) ) );

        if( 0 != mod_sym )
        {
          RNOK( CabaDecoder::getSymbol( mod_sym, m_cMbTypeCCModel.get( 0, 6 ) ) );
          act_sym += ++mod_sym << 2;
        }

        RNOK( CabaDecoder::getSymbol( mod_sym, m_cMbTypeCCModel.get( 0, 7 ) ) );
        act_sym += mod_sym << 1;

        RNOK( CabaDecoder::getSymbol( mod_sym, m_cMbTypeCCModel.get( 0, 8 ) ) );
        act_sym += mod_sym;
      }
    }
    uiMbMode = act_sym;
  }
  else
  {
    uiMbMode = 0;
    UInt uiSymbol;

    if( ! rcMbDataAccess.getSH().isBSlice() )
    {
      RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 4 ) ) );
      if( 0 != uiSymbol )
      {
        RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 7 ) ) );
        uiMbMode = ( 0 != uiSymbol ) ? 7 : 6;
      }
      else
      {
        RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 5 ) ) );
        if( 0 != uiSymbol )
        {
          RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 7 ) ) );
          uiMbMode = ( 0 != uiSymbol ) ? 2 : 3;
        }
        else
        {
          RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 6 ) ) );
          uiMbMode = ( 0 != uiSymbol ) ? 4 : 1;
        }
      }
    }
    else
    {
      RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, rcMbDataAccess.getCtxMbType() ) ) );
      if( 0 != uiSymbol )
      {
        RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 4 ) ) );
        if( 0 != uiSymbol )
        {
          RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 5 ) ) );
          if( 0 != uiSymbol )
          {
            uiMbMode = 12;
            RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
            uiMbMode += uiSymbol << 3;
            RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
            uiMbMode += uiSymbol << 2;
            RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
            uiMbMode += uiSymbol << 1;
            if( 24 == uiMbMode)
            {
              uiMbMode = 11;
            }
            else
            {
              if( 26 == uiMbMode)
              {
                uiMbMode = 22;
              }
              else
              {
                if( 22 == uiMbMode)
                {
                  uiMbMode = 23;
                }
                RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
                uiMbMode += uiSymbol;
              }
            }
          }
          else
          {
            uiMbMode = 3;
            RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
            uiMbMode += uiSymbol << 2;
            RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
            uiMbMode += uiSymbol << 1;
            RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
            uiMbMode += uiSymbol;
          }
        }
        else
        {
          RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 2, 6 ) ) );
          uiMbMode = ( 0 != uiSymbol ) ? 2 : 1;
        }
      }
    }

    if( ! ( uiMbMode <= 6 || (rcMbDataAccess.getSH().isBSlice() && uiMbMode <= 23) ) )
    {
      RNOK( CabaDecoder::getTerminateBufferBit( uiSymbol ) )
      if( 0 != uiSymbol )
      {
        uiMbMode += 24;
      }
      else
      {
        RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 8 ) ) );
        uiMbMode += ( 0 != uiSymbol ) ? 12 : 0;

        RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 9 ) ) );
        if( 0 != uiSymbol )
        {
          RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 9 ) ) );
          uiMbMode += ( 0 != uiSymbol ) ? 8 : 4;
        }

        RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 10 ) ) );
        uiMbMode += uiSymbol << 1;
        RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMbTypeCCModel.get( 1, 10 ) ) );
        uiMbMode += uiSymbol;
      }
    }
    if( ! rcMbDataAccess.getSH().isBSlice() )
    {
      uiMbMode--;
    }
  }

  rcMbDataAccess.setConvertMbType( uiMbMode );
  if( rcMbDataAccess.getMbData().isPCM() )
  {
    m_uiLastDQpNonZero = 0; // no DQP for IPCM
  }

  DTRACE_TH( "MbMode" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( ( ! rcMbDataAccess.getSH().isIntraSlice()) ? uiMbMode+1:uiMbMode );
  DTRACE_N;

  return Err::m_nOK;
}




ErrVal CabacReader::xGetMvdComponent( Short& rsMvdComp, UInt uiAbsSum, UInt uiCtx )
{

  UInt uiLocalCtx = uiCtx;

  if( uiAbsSum >= 3)
  {
    uiLocalCtx += ( uiAbsSum > 32) ? 3 : 2;
  }

  rsMvdComp = 0;

  UInt uiSymbol;
  RNOK( CabaDecoder::getSymbol( uiSymbol, m_cMvdCCModel.get( 0, uiLocalCtx ) ) );

  ROTRS( 0 == uiSymbol, Err::m_nOK )

  RNOK( CabaDecoder::getExGolombMvd( uiSymbol, &m_cMvdCCModel.get( 1, uiCtx ), 3 ) );
  uiSymbol++;

  UInt uiSign;
  RNOK( CabaDecoder::getEpSymbol( uiSign ) );

  rsMvdComp = ( 0 != uiSign ) ? -(Int)uiSymbol : (Int)uiSymbol;

  return Err::m_nOK;
}


ErrVal CabacReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  Mv cMv;
  DECRNOK( xGetMvd( rcMbDataAccess, cMv, B4x4Idx(0), eLstIdx ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv );
  return Err::m_nOK;
}

ErrVal CabacReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  )
{
  Mv cMv;
  DECRNOK( xGetMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx), eLstIdx ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx );
  return Err::m_nOK;
}

ErrVal CabacReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  )
{
  Mv cMv;
  DECRNOK( xGetMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx), eLstIdx ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx );
  return Err::m_nOK;
}
ErrVal CabacReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  )
{
  Mv cMv;
  DECRNOK( xGetMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx), eLstIdx ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx );
  return Err::m_nOK;
}
ErrVal CabacReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx )
{
  Mv cMv;
  DECRNOK( xGetMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx+eSParIdx), eLstIdx ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx, eSParIdx );
  return Err::m_nOK;
}
ErrVal CabacReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx )
{
  Mv cMv;
  DECRNOK( xGetMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx+eSParIdx), eLstIdx ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx, eSParIdx );
  return Err::m_nOK;
}
ErrVal CabacReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx )
{
  Mv cMv;
  DECRNOK( xGetMvd( rcMbDataAccess, cMv, B4x4Idx(eParIdx+eSParIdx), eLstIdx ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx, eSParIdx );
  return Err::m_nOK;
}




ErrVal CabacReader::xGetMvd( MbDataAccess& rcMbDataAccess, Mv& rcMv, LumaIdx cIdx, ListIdx eLstIdx )
{
  Mv    cMvA;
  Mv    cMvB;
  Short sMvdComponent;

  rcMbDataAccess.getMvdAbove( cMvA, eLstIdx, cIdx );
  rcMbDataAccess.getMvdLeft ( cMvB, eLstIdx, cIdx );

  DECRNOK( xGetMvdComponent( sMvdComponent, cMvA.getAbsHor() + cMvB.getAbsHor(), 0 ) );
  rcMv.setHor( sMvdComponent );

  DTRACE_TH( "Mvd: x" );
  DTRACE_TY( "ae(v)" );
  DTRACE_V( sMvdComponent );
  DTRACE_TH( " above " );
  DTRACE_V( cMvA.getHor() );
  DTRACE_TH( " left " );
  DTRACE_V( cMvB.getHor() );
  DTRACE_N;

  DECRNOK( xGetMvdComponent( sMvdComponent, cMvA.getAbsVer() + cMvB.getAbsVer(), 5 ) );
  rcMv.setVer( sMvdComponent );

  DTRACE_TH( "Mvd: y" );
  DTRACE_TY( "ae(v)" );
  DTRACE_V( sMvdComponent );
  DTRACE_TH( " above " );
  DTRACE_V( cMvA.getVer() );
  DTRACE_TH( " left " );
  DTRACE_V( cMvB.getVer() );
  DTRACE_N;

  return Err::m_nOK;
}

ErrVal CabacReader::fieldFlag( MbDataAccess& rcMbDataAccess )
{
  UInt uiSymbol;
  RNOK( CabaDecoder::getSymbol( uiSymbol, m_cFieldFlagCCModel.get( 0, rcMbDataAccess.getCtxFieldFlag() ) ) );

  rcMbDataAccess.setFieldMode( uiSymbol != 0 );

  DTRACE_TH( "FieldFlag:" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( uiSymbol );
  DTRACE_N;

  return Err::m_nOK;
}

ErrVal CabacReader::intraPredModeChroma( MbDataAccess& rcMbDataAccess )
{

  UInt uiSymbol;
  RNOK( CabaDecoder::getSymbol( uiSymbol, m_cChromaPredCCModel.get( 0, rcMbDataAccess.getCtxChromaPredMode() ) ) );

  if( uiSymbol )
  {
    RNOK( CabaDecoder::getUnaryMaxSymbol( uiSymbol, m_cChromaPredCCModel.get( 0 ) + 3, 0, 2 ) );
    uiSymbol++;
  }

  rcMbDataAccess.getMbData().setChromaPredMode( uiSymbol );
  DTRACE_TH( "IntraPredModeChroma" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( uiSymbol );
  DTRACE_N;

  return Err::m_nOK;
}



ErrVal CabacReader::intraPredModeLuma( MbDataAccess& rcMbDataAccess, LumaIdx cIdx )
{
  UInt uiSymbol;
  UInt uiIPredMode;

  RNOK( CabaDecoder::getSymbol( uiSymbol, m_cIntraPredCCModel.get( 0, 0 ) ) );

  if( ! uiSymbol )
  {
    RNOK( CabaDecoder::getSymbol( uiSymbol, m_cIntraPredCCModel.get( 0, 1 ) ) );
    uiIPredMode  = uiSymbol;
    RNOK( CabaDecoder::getSymbol( uiSymbol, m_cIntraPredCCModel.get( 0, 1 ) ) );
    uiIPredMode |= (uiSymbol << 1);
    RNOK( CabaDecoder::getSymbol( uiSymbol, m_cIntraPredCCModel.get( 0, 1 ) ) );
    rcMbDataAccess.getMbData().intraPredMode( cIdx ) = ( uiIPredMode | (uiSymbol << 2) );
  }
  else
  {
    rcMbDataAccess.getMbData().intraPredMode( cIdx ) = -1;
  }

  DTRACE_TH( "IntraPredModeLuma" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( rcMbDataAccess.getMbData().intraPredMode( cIdx ) );
  DTRACE_N;

  return Err::m_nOK;
}


ErrVal CabacReader::cbp( MbDataAccess& rcMbDataAccess, UInt uiStart, UInt uiStop )
{
  UInt uiCbp;
  UInt uiBit;
  UInt uiCtx = 0, a, b;

  a = rcMbDataAccess.getLeftLumaCbp ( B4x4Idx( 0 ), 0, 16, false );
  b = rcMbDataAccess.getAboveLumaCbp( B4x4Idx( 0 ), 0, 16, false ) << 1;

  RNOK( CabaDecoder::getSymbol( uiBit, m_cCbpCCModel.get( uiCtx, 3 - (a + b) ) ) );
  uiCbp = uiBit;

  a = uiCbp & 1;
  b = rcMbDataAccess.getAboveLumaCbp( B4x4Idx( 2 ), 0, 16, false ) << 1;

  RNOK( CabaDecoder::getSymbol( uiBit, m_cCbpCCModel.get( uiCtx, 3 - (a + b) ) ) );
  uiCbp += uiBit << 1;

  a = rcMbDataAccess.getLeftLumaCbp ( B4x4Idx( 8 ), 0, 16, false );
  b = (uiCbp  << 1) & 2;

  RNOK( CabaDecoder::getSymbol( uiBit, m_cCbpCCModel.get( uiCtx, 3 - (a + b) ) ) );
  uiCbp += uiBit << 2;

  a = ( uiCbp >> 2 ) & 1;
  b = uiCbp & 2;

  RNOK( CabaDecoder::getSymbol( uiBit, m_cCbpCCModel.get( uiCtx, 3 - (a + b) ) ) );
  uiCbp += uiBit << 3;

  if( rcMbDataAccess.getSH().getSPS().getChromaFormatIdc() )
  {
    uiCtx = 1;

    UInt  uiLeftChromaCbp   = rcMbDataAccess.getLeftChromaCbp ( 0, 16, false );
    UInt  uiAboveChromaCbp  = rcMbDataAccess.getAboveChromaCbp( 0, 16, false );

    a = uiLeftChromaCbp  > 0 ? 1 : 0;
    b = uiAboveChromaCbp > 0 ? 2 : 0;

    RNOK( CabaDecoder::getSymbol( uiBit, m_cCbpCCModel.get( uiCtx, a + b ) ) );

    if( uiBit )
    {
      a = uiLeftChromaCbp  > 1 ? 1 : 0;
      b = uiAboveChromaCbp > 1 ? 2 : 0;

      RNOK( CabaDecoder::getSymbol( uiBit, m_cCbpCCModel.get( ++uiCtx, a + b ) ) );
      uiCbp += (1 == uiBit) ? 32 : 16;
    }
  }

  if( !uiCbp )
  {
    m_uiLastDQpNonZero = 0; // no DeltaQP for Macroblocks with zero Cbp
  }

  AOF_DBG( 48 >= uiCbp );

  DTRACE_TH( "Cbp" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( uiCbp );
  DTRACE_N;

  rcMbDataAccess.getMbData().setMbCbp( uiCbp );
  return Err::m_nOK;
}


ErrVal CabacReader::residualBlock( MbDataAccess&  rcMbDataAccess,
                                   LumaIdx        cIdx,
                                   ResidualMode   eResidualMode,
                                   UInt&          ruiMbExtCbp,
                                   UInt           uiStart,
                                   UInt           uiStop )
{
  const Bool bFrame    = ( FRAME == rcMbDataAccess.getMbPicType() );
  const UChar* pucScan = ( eResidualMode == LUMA_I16_DC ? ((bFrame) ? g_aucLumaFrameDCScan : g_aucLumaFieldDCScan) : ((bFrame) ? g_aucFrameScan : g_aucFieldScan) );

  Bool bCoded;

  DTRACE_TH( "LUMA:" );
  DTRACE_V( cIdx );
  DTRACE_N;

  DECRNOK( xReadBCbp( rcMbDataAccess, bCoded, eResidualMode, cIdx, uiStart, uiStop ) );

  if( ! bCoded )
  {
    ruiMbExtCbp &= ~(1 << cIdx.b4x4() );
    return Err::m_nOK;
  }

  TCoeff*       piCoeff   = rcMbDataAccess.getMbTCoeffs().get( cIdx );

  DECRNOK( xReadCoeff( piCoeff, eResidualMode, pucScan, bFrame, uiStart, uiStop ) );

  return Err::m_nOK;
}


ErrVal CabacReader::residualBlock( MbDataAccess&  rcMbDataAccess,
                                   ChromaIdx      cIdx,
                                   ResidualMode   eResidualMode,
                                   UInt           uiStart,
                                   UInt           uiStop )
{
  const Bool bFrame    = ( FRAME == rcMbDataAccess.getMbPicType() );
  const UChar* pucScan = ( eResidualMode == CHROMA_DC ? g_aucIndexChromaDCScan : ((bFrame) ? g_aucFrameScan : g_aucFieldScan) );

  Bool bCoded;

  DTRACE_TH( eResidualMode == CHROMA_DC ? "CHROMA_DC:" : "CHROMA_AC:" );

  DTRACE_V( cIdx );
  DTRACE_N;

  TCoeff*       piCoeff   = rcMbDataAccess.getMbTCoeffs().get( cIdx );

  DECRNOK( xReadBCbp( rcMbDataAccess, bCoded, eResidualMode, cIdx, uiStart, uiStop ) );

  ROTRS( ! bCoded, Err::m_nOK );

  DECRNOK( xReadCoeff( piCoeff, eResidualMode, pucScan, bFrame, uiStart, uiStop ) );

  return Err::m_nOK;
}



ErrVal CabacReader::deltaQp( MbDataAccess& rcMbDataAccess )
{
  UInt uiDQp;
  Int iDQp = 0;
  UInt uiCtx = m_uiLastDQpNonZero;

  RNOK( CabaDecoder::getSymbol( uiDQp, m_cDeltaQpCCModel.get( 0, uiCtx ) ) );

  m_uiLastDQpNonZero = uiDQp;

  if( uiDQp )
  {
    RNOK( CabaDecoder::getUnarySymbol( uiDQp, &m_cDeltaQpCCModel.get( 0, 2 ), 1 ) );

    iDQp = (uiDQp + 2) / 2;

    if( uiDQp & 1 )
    {
      iDQp = -iDQp;
    }
  }

  DTRACE_TH( "DQp" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE ( iDQp );
  DTRACE_N;

  rcMbDataAccess.addDeltaQp( iDQp );

  Quantizer::setQp( rcMbDataAccess, false );

  return Err::m_nOK;
}


Bool CabacReader::isEndOfSlice()
{
  UInt uiEOS;

  CabaDecoder::getTerminateBufferBit( uiEOS );
  DTRACE_TH( "EOS" );
  DTRACE_CODE( uiEOS );
  DTRACE_N;

  return (uiEOS == 1);
}

ErrVal CabacReader::xReadBCbp( MbDataAccess& rcMbDataAccess, Bool& rbCoded, ResidualMode eResidualMode, ChromaIdx cIdx, UInt uiStart, UInt uiStop )
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
    // full stop
    AF();
    return Err::m_nERR;
  }

  UInt uiCtx  = rcMbDataAccess.getCtxCodedBlockBit( uiBitPos, uiStart, uiStop, false );
  UInt uiBit;

  RNOK( CabaDecoder::getSymbol( uiBit, m_cBCbpCCModel.get( type2ctx1[ eResidualMode ], uiCtx) ) );

  rbCoded = ( uiBit == 1 );
  rcMbDataAccess.getMbData().setBCBP( uiBitPos, uiBit);

  return Err::m_nOK;
}

ErrVal CabacReader::xReadBCbp( MbDataAccess& rcMbDataAccess, Bool& rbCoded, ResidualMode eResidualMode, LumaIdx cIdx, UInt uiStart, UInt uiStop )
{
  UInt uiBitPos;

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
    return Err::m_nERR;
  }

  UInt uiCtx = rcMbDataAccess.getCtxCodedBlockBit( uiBitPos, uiStart, uiStop, false );
  UInt uiBit;

  RNOK( CabaDecoder::getSymbol( uiBit, m_cBCbpCCModel.get( type2ctx1[ eResidualMode ], uiCtx) ) );

  rbCoded = ( uiBit == 1 );
  rcMbDataAccess.getMbData().setBCBP( uiBitPos, uiBit);


  return Err::m_nOK;
}

ErrVal CabacReader::xReadCoeff( TCoeff*         piCoeff,
                                ResidualMode    eResidualMode,
                                const UChar*    pucScan,
                                Bool            bFrame,
                                UInt            uiStart,
                                UInt            uiStop )
{
	CabacContextModel2DBuffer&  rcMapCCModel  = ( (bFrame) ? m_cMapCCModel  : m_cFldMapCCModel  );
	CabacContextModel2DBuffer&  rcLastCCModel = ( (bFrame) ? m_cLastCCModel : m_cFldLastCCModel );

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
  for( ui = uiStart; ui < uiStop; ui++ ) // if last coeff is reached, it has to be significant
  {
    UInt uiSig;
    //--- read significance symbol ---
    RNOK( CabaDecoder::getSymbol( uiSig, rcMapCCModel.get( type2ctx2[eResidualMode], ui ) ) );

    if( uiSig )
    {
      piCoeff[pucScan[ui]] = uiSig;

      UInt uiLast;
      RNOK( CabaDecoder::getSymbol( uiLast, rcLastCCModel.get( type2ctx2[eResidualMode], ui ) ) );
      if( uiLast )
      {
        break;
      }

    }
  }
  //--- last coefficient must be significant if no last symbol was received ---
  if( ui == uiStop )
  {
    piCoeff[pucScan[ui]] = 1;
  }

  int   c1 = 1;
  int   c2 = 0;

  ui++;

  while( (ui--) != uiStart )
  {
    UInt uiCoeff = piCoeff[pucScan[ui]];
    if( uiCoeff )
    {
      UInt uiCtx = gMin (c1,4);

      RNOK( CabaDecoder::getSymbol( uiCoeff, m_cOneCCModel.get( type2ctx1[eResidualMode], uiCtx ) ) );

      if( 1 == uiCoeff )
      {
        uiCtx = gMin (c2,4);
        RNOK( CabaDecoder::getExGolombLevel( uiCoeff, m_cAbsCCModel.get( type2ctx1[eResidualMode], uiCtx ) ) );
        uiCoeff += 2;

        c1=0;
        c2++;
      }
      else if (c1)
      {
        uiCoeff++;
        c1++;
      }
      else
      {
        uiCoeff++;
      }

      UInt uiSign;
      RNOK( CabaDecoder::getEpSymbol( uiSign ) );

      piCoeff[pucScan[ui]] = ( uiSign ? -(Int)uiCoeff : (Int)uiCoeff );
    }
  }

  return Err::m_nOK;
}



ErrVal CabacReader::samplesPCM( MbDataAccess& rcMbDataAccess )
{
  RNOK( CabaDecoder::finish() );

  DTRACE_POS;
  DTRACE_TH( "  PCM SAMPLES: " );

  AOF_DBG( rcMbDataAccess.getMbData().isPCM() );

  TCoeff* pCoeff = rcMbDataAccess.getMbTCoeffs().getTCoeffBuffer();

  // get chroma mode
  const UInt uiFactor = 8*8;
  const UInt uiSize   = uiFactor*2*3;
  DECRNOK( m_pcBitReadBuffer->pcmSamples( pCoeff, uiSize ) );

  DTRACE_N;

  rcMbDataAccess.getMbData().setBCBPAll( 1 );

  RNOK( CabaDecoder::start() );
  return Err::m_nOK;
}







ErrVal CabacReader::transformSize8x8Flag( MbDataAccess& rcMbDataAccess)
{
  UInt uiSymbol;
  UInt uiCtx = rcMbDataAccess.getCtx8x8Flag();

  RNOK( CabaDecoder::getSymbol( uiSymbol, m_cTransSizeCCModel.get( 0, uiCtx ) ) );

  rcMbDataAccess.getMbData().setTransformSize8x8( uiSymbol != 0 );

  DTRACE_TH( "transformSize8x8Flag:" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( rcMbDataAccess.getMbData().isTransformSize8x8() );
  DTRACE_N;

  return Err::m_nOK;
}


ErrVal CabacReader::intraPredModeLuma8x8( MbDataAccess& rcMbDataAccess, B8x8Idx cIdx )
{
  UInt uiSymbol;
  UInt uiIPredMode;

  RNOK( CabaDecoder::getSymbol( uiSymbol, m_cIntraPredCCModel.get( 0, 0 ) ) );

  if( ! uiSymbol )
  {
    RNOK( CabaDecoder::getSymbol( uiSymbol, m_cIntraPredCCModel.get( 0, 1 ) ) );
    uiIPredMode  = uiSymbol;
    RNOK( CabaDecoder::getSymbol( uiSymbol, m_cIntraPredCCModel.get( 0, 1 ) ) );
    uiIPredMode |= (uiSymbol << 1);
    RNOK( CabaDecoder::getSymbol( uiSymbol, m_cIntraPredCCModel.get( 0, 1 ) ) );
    rcMbDataAccess.getMbData().intraPredMode( cIdx ) = ( uiIPredMode | (uiSymbol << 2) );
  }
  else
  {
    rcMbDataAccess.getMbData().intraPredMode( cIdx ) = -1;
  }

  DTRACE_TH( "IntraPredModeLuma" );
  DTRACE_TY( "ae(v)" );
  DTRACE_CODE( rcMbDataAccess.getMbData().intraPredMode( cIdx ) );
  DTRACE_N;

  return Err::m_nOK;
}


ErrVal CabacReader::residualBlock8x8( MbDataAccess& rcMbDataAccess,
                                      B8x8Idx       cIdx,
                                      UInt          uiStart,
                                      UInt          uiStop )
{
  const Bool bFrame    = ( FRAME == rcMbDataAccess.getMbPicType() );
  const UChar* pucScan = (bFrame) ? g_aucFrameScan64 : g_aucFieldScan64;

  DTRACE_TH( "LUMA_8x8:" );
  DTRACE_V( cIdx.b8x8Index() );
  DTRACE_N;

  {
    UInt uiBitPos = cIdx;
    rcMbDataAccess.getMbData().setBCBP( uiBitPos,   1);
    rcMbDataAccess.getMbData().setBCBP( uiBitPos+1, 1);
    rcMbDataAccess.getMbData().setBCBP( uiBitPos+4, 1);
    rcMbDataAccess.getMbData().setBCBP( uiBitPos+5, 1);
  }

  TCoeff*     piCoeff     = rcMbDataAccess.getMbTCoeffs().get8x8( cIdx );
  const Int*  pos2ctx_map = (bFrame) ? pos2ctx_map8x8 : pos2ctx_map8x8i;

  ROT( uiStart == uiStop );
  uiStart <<= 2;
  uiStop  <<= 2;
  uiStop -= 1;
  UInt        ui;


  {
    const UInt          uiCtxOffset       = 2;
    CabacContextModel*  pcMapCCModel  = ( (bFrame) ? m_cMapCCModel  : m_cFldMapCCModel). get( uiCtxOffset );
    CabacContextModel*  pcLastCCModel = ( (bFrame) ? m_cLastCCModel : m_cFldLastCCModel).get( uiCtxOffset );

    for( ui = uiStart; ui < uiStop; ui++ ) // if last coeff is reached, it has to be significant
    {
      UInt uiSig;
      //--- read significance symbol ---
      RNOK( CabaDecoder::getSymbol( uiSig, pcMapCCModel[pos2ctx_map[ui]] ) );

      if( uiSig )
      {
        piCoeff[pucScan[ui]] = uiSig;
        UInt uiLast;

          RNOK( CabaDecoder::getSymbol( uiLast, pcLastCCModel[pos2ctx_last8x8[ui]] ) );

        if( uiLast )
        {
          break;
        }
      }
    }
    //--- last coefficient must be significant if no last symbol was received ---
    if( ui == uiStop )
    {
      piCoeff[pucScan[ui]] = 1;
    }
  }


  int   c1 = 1;
  int   c2 = 0;

  ui++;
  const UInt uiCtxOffset = 2;


  while( (ui--) != uiStart )
  {
    Int   iIndex  = pucScan[ui];
    UInt  uiCoeff = piCoeff[iIndex];

    if( uiCoeff )
    {
      UInt uiCtx = gMin (c1,4);

      RNOK( CabaDecoder::getSymbol( uiCoeff, m_cOneCCModel.get( uiCtxOffset, uiCtx ) ) );

      if( 1 == uiCoeff )
      {
        uiCtx = gMin (c2,4);
        RNOK( CabaDecoder::getExGolombLevel( uiCoeff, m_cAbsCCModel.get( uiCtxOffset, uiCtx ) ) );
        uiCoeff += 2;

        c1=0;
        c2++;
      }
      else if (c1)
      {
        uiCoeff++;
        c1++;
      }
      else
      {
        uiCoeff++;
      }

      UInt uiSign;
      RNOK( CabaDecoder::getEpSymbol( uiSign ) );

      piCoeff[iIndex] = ( uiSign ? -(Int)uiCoeff : (Int)uiCoeff );
    }
  }

  return Err::m_nOK;
}

H264AVC_NAMESPACE_END
