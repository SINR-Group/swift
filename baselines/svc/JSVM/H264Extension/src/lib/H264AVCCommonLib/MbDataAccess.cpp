
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/MbDataAccess.h"
#include "H264AVCCommonLib/Frame.h"

H264AVC_NAMESPACE_BEGIN

const BlkMode MbDataAccess::m_aucBMTabB0[13] =
{
  BLK_SKIP,  //0
  BLK_8x8,   //1
  BLK_8x8,   //2
  BLK_8x8,   //3
  BLK_8x4,   //4
  BLK_4x8,   //5
  BLK_8x4,   //6
  BLK_4x8,   //7
  BLK_8x4,   //8
  BLK_4x8,   //9
  BLK_4x4,   //10
  BLK_4x4,   //11
  BLK_4x4    //12
};

const UChar MbDataAccess::m_aucBMTabB1[13] =
{
  0 ,//0
  1 ,//1
  2 ,//2
  3 ,//3
  1 ,//4
  1 ,//5
  2 ,//6
  2 ,//7
  3 ,//8
  3 ,//9
  1 ,//10
  2 ,//11
  3 ,//12
};

const BlkMode MbDataAccess::m_aucBMTabP[4] =
{
  BLK_8x8,
  BLK_8x4,
  BLK_4x8,
  BLK_4x4
};


const MbMode MbDataAccess::m_aausInterBMbType0[23] =
{
  MODE_SKIP,  //0
  MODE_16x16, //1
  MODE_16x16, //2
  MODE_16x16, //3
  MODE_16x8,  //4
  MODE_8x16,  //5
  MODE_16x8,  //6
  MODE_8x16,  //7
  MODE_16x8,  //8
  MODE_8x16,  //9
  MODE_16x8,  //10
  MODE_8x16,  //11
  MODE_16x8,  //12
  MODE_8x16,  //13
  MODE_16x8,  //14
  MODE_8x16,  //15
  MODE_16x8,  //16
  MODE_8x16,  //17
  MODE_16x8,  //18
  MODE_8x16,  //19
  MODE_16x8,  //20
  MODE_8x16,  //21
  MODE_8x8    //22
};

const UShort MbDataAccess::m_aausInterBMbType1[23] =
{
  0x0000, //0
  0x1111,	//1
  0x2222,	//2
  0x3333,	//3
  0x1111, //4
  0x1111, //5
  0x2222, //6
  0x2222, //7
  0x2211, //8
  0x2121, //9
  0x1122, //10
  0x1212, //11
  0x3311, //12
  0x3131, //13
  0x3322, //14
  0x3232, //15
  0x1133, //16
  0x1313, //17
  0x2233, //18
  0x2323, //19
  0x3333, //20
  0x3333, //21
  0x0000  //22
};


const UChar MbDataAccess::m_aucMbType1x2[9] =
{
  12, //2, 0x0
   8, //2, 0x1
   4, //2, 0x2
  14, //2, 0x3
   6, //2, 0x4
  10, //2, 0x5
  20, //2, 0x6
  18, //2, 0x7
  16  //2, 0x8
};

const UChar MbDataAccess::m_aucMbType2x1[9] =
{
  13, //3, 0x0
   9, //3, 0x1
   5, //3, 0x2
  15, //3, 0x3
   7, //3, 0x4
  11, //3, 0x5
  21, //3, 0x6
  19, //3, 0x7
  17 //3, 0x8
};

const UChar MbDataAccess::m_aucChroma2LumaIdx[8 ]  = { 0, 2, 8,10,   0, 2, 8,10 };
const UChar MbDataAccess::m_auc4x4Idx28x8Idx [16 ]  = { 0, 0, 1, 1,   0, 0, 1, 1,   2, 2, 3, 3,   2, 2, 3, 3 };



Void MbDataAccess::xGetMvPredictor( Mv& rcMvPred, SChar scRef, ListIdx eListIdx, PredictionType ePredType, LumaIdx cIdx, LumaIdx cIdxEnd )
{
	const Bool bCurrentFieldFlag = m_rcMbCurr.getFieldFlag();

  //===== set motion vector predictors: A, B, C =====
  B4x4Idx cIdxA             = cIdx   .b4x4();
  B4x4Idx cIdxB             = cIdx   .b4x4();
  B4x4Idx cIdxD             = cIdx   .b4x4();
  B4x4Idx cIdxC             = cIdxEnd.b4x4();

  const MbData& rcMbDataA   = xGetBlockLeft      ( cIdxA );
  const MbData& rcMbDataB   = xGetBlockAbove     ( cIdxB );
  const MbData& rcMbDataC   = xGetBlockAboveRight( cIdxC );

  rcMbDataA.getMbMotionData( eListIdx ).getMv3DNeighbour( m_cMv3D_A, cIdxA, bCurrentFieldFlag );
  rcMbDataB.getMbMotionData( eListIdx ).getMv3DNeighbour( m_cMv3D_B, cIdxB, bCurrentFieldFlag );
  rcMbDataC.getMbMotionData( eListIdx ).getMv3DNeighbour( m_cMv3D_C, cIdxC, bCurrentFieldFlag );
  if( m_cMv3D_C == BLOCK_NOT_AVAILABLE )
  {
    const MbData& rcMbDataD = xGetBlockAboveLeft ( cIdxD );
		rcMbDataD.getMbMotionData( eListIdx ).getMv3DNeighbour( m_cMv3D_C, cIdxD, bCurrentFieldFlag );
  }

  //===== check directional prediction types =====
  if( ( ePredType == PRED_A && m_cMv3D_A == scRef ) ||
      ( m_cMv3D_A == scRef  && m_cMv3D_B != scRef && m_cMv3D_C != scRef ) ||
      ( m_cMv3D_B == BLOCK_NOT_AVAILABLE && m_cMv3D_C == BLOCK_NOT_AVAILABLE ) )
  {
    rcMvPred = m_cMv3D_A;
    return;
  }
  if( ( ePredType == PRED_B && m_cMv3D_B == scRef ) ||
      ( m_cMv3D_A != scRef  && m_cMv3D_B == scRef && m_cMv3D_C != scRef ) )
  {
    rcMvPred = m_cMv3D_B;
    return;
  }
  if( ( ePredType == PRED_C && m_cMv3D_C == scRef ) ||
      ( m_cMv3D_A != scRef  && m_cMv3D_B != scRef && m_cMv3D_C == scRef ) )
  {
    rcMvPred = m_cMv3D_C;
    return;
  }

#define MEDIAN(a,b,c)  ((a)>(b)?(a)>(c)?(b)>(c)?(b):(c):(a):(b)>(c)?(a)>(c)?(a):(c):(b))
  {
    rcMvPred.setHor( MEDIAN( m_cMv3D_A.getHor(), m_cMv3D_B.getHor(), m_cMv3D_C.getHor() ) );
    rcMvPred.setVer( MEDIAN( m_cMv3D_A.getVer(), m_cMv3D_B.getVer(), m_cMv3D_C.getVer() ) );
  }
#undef MEDIAN
}



UInt MbDataAccess::getCtxCoeffCount( LumaIdx cIdx, UInt uiStart, UInt uiStop )  const
{
  Bool bLeftAvailable = false;
  UInt uiCoeffCount   = 0;

  {
    B4x4Idx cIdxL( cIdx.b4x4() );
    const MbData& rcMbDataLeft = xGetBlockLeft( cIdxL );

    if( xIsAvailable( rcMbDataLeft ) )
    {
      bLeftAvailable = true;
      if( ( uiStart == 0 && uiStop == 16 ) || rcMbDataLeft.isPCM() )
      {
        uiCoeffCount = rcMbDataLeft.getMbTCoeffs().getCoeffCount( cIdxL );
      }
      else
      {
        uiCoeffCount = rcMbDataLeft.getMbTCoeffs().calcCoeffCount( cIdxL, rcMbDataLeft.isTransformSize8x8(),
                                                                  rcMbDataLeft.getFieldFlag(),
                                                                  rcMbDataLeft.isIntra16x16() ? gMax( 1, uiStart) : uiStart,
                                                                  uiStop );
      }
    }
  }

  const MbData& rcMbDataAbove = xGetBlockAbove( cIdx );
  if( xIsAvailable( rcMbDataAbove ) )
  {
    if( ( uiStart == 0 && uiStop == 16 ) || rcMbDataAbove.isPCM() )
    {
      uiCoeffCount += rcMbDataAbove.getMbTCoeffs().getCoeffCount( cIdx );
    }
    else
    {
      uiCoeffCount += rcMbDataAbove.getMbTCoeffs().calcCoeffCount( cIdx, rcMbDataAbove.isTransformSize8x8(),
                                                                   rcMbDataAbove.getFieldFlag(),
                                                                   rcMbDataAbove.isIntra16x16() ? gMax( 1, uiStart) : uiStart,
                                                                   uiStop );
    }
    if( bLeftAvailable )
    {
      uiCoeffCount  += 1;
      uiCoeffCount >>= 1;
    }
  }

  if( 4 > uiCoeffCount )
  {
    uiCoeffCount >>= 1;
  }
  else
  {
    uiCoeffCount = ( 8 > uiCoeffCount) ? 2 : 3;
  }

  return uiCoeffCount;
}


UInt MbDataAccess::getCtxCoeffCount( ChromaIdx cIdx, UInt uiStart, UInt uiStop )  const
{
  Bool bLeftAvailable = false;
  UInt uiCoeffCount     = 0;

  B4x4Idx       cLumIdx( m_aucChroma2LumaIdx[ cIdx ] );
  Int           iComp = ( cIdx >> 2) << 2;

  {
    B4x4Idx cLumIdxL = cLumIdx;
    const MbData& rcMbDataLeft = xGetBlockLeft( cLumIdxL );

    if( xIsAvailable( rcMbDataLeft ) )
    {
      bLeftAvailable = true;
      if( ( uiStart == 0 && uiStop == 16 ) || rcMbDataLeft.isPCM() )
      {
        uiCoeffCount += rcMbDataLeft.getMbTCoeffs().getCoeffCount( CIdx( iComp + m_auc4x4Idx28x8Idx[ cLumIdxL.b4x4() ] ) );
      }
      else
      {
        uiCoeffCount = rcMbDataLeft.getMbTCoeffs().calcCoeffCount( CIdx( iComp + m_auc4x4Idx28x8Idx[ cLumIdxL.b4x4() ] ),
                                                                  rcMbDataLeft.getFieldFlag() ? g_aucFieldScan : g_aucFrameScan,
                                                                  uiStart,
                                                                  uiStop );
      }
    }
  }

  const MbData& rcMbDataAbove = xGetBlockAbove( cLumIdx );
  if( xIsAvailable( rcMbDataAbove ) )
  {
    if( ( uiStart == 0 && uiStop == 16 ) || rcMbDataAbove.isPCM() )
    {
      uiCoeffCount += rcMbDataAbove.getMbTCoeffs().getCoeffCount( CIdx( iComp + m_auc4x4Idx28x8Idx[ cLumIdx.b4x4() ] ) );
    }
    else
    {
      uiCoeffCount += rcMbDataAbove.getMbTCoeffs().calcCoeffCount( CIdx( iComp + m_auc4x4Idx28x8Idx[ cLumIdx.b4x4() ] ),
                                                                  rcMbDataAbove.getFieldFlag() ? g_aucFieldScan : g_aucFrameScan,
                                                                  uiStart,
                                                                  uiStop );
    }
    if( bLeftAvailable )
    {
      uiCoeffCount  += 1;
      uiCoeffCount >>= 1;
    }
  }

  if( 4 > uiCoeffCount )
  {
    uiCoeffCount >>= 1;
  }
  else
  {
    uiCoeffCount = ( 8 > uiCoeffCount) ? 2 : 3;
  }

  return uiCoeffCount;
}


Void MbDataAccess::xSetMvPredictorsBL( const Mv& rcMvPredBL, ListIdx eListIdx, LumaIdx cIdx, LumaIdx cIdxEnd )
{
  const Bool bCurrentFieldFlag = m_rcMbCurr.getFieldFlag();

  //===== set motion vector predictors: A, B, C =====
  B4x4Idx cIdxA             = cIdx   .b4x4();
  B4x4Idx cIdxB             = cIdx   .b4x4();

  const MbData& rcMbDataA   = xGetBlockLeft      ( cIdxA );
  const MbData& rcMbDataB   = xGetBlockAbove     ( cIdxB );

	rcMbDataA.getMbMotionData( eListIdx ).getMv3DNeighbour( m_cMv3D_A, cIdxA, bCurrentFieldFlag );
  rcMbDataB.getMbMotionData( eListIdx ).getMv3DNeighbour( m_cMv3D_B, cIdxB, bCurrentFieldFlag );
  m_cMv3D_C.set( rcMvPredBL, 1 );
}




Void MbDataAccess::xSetNeighboursMvPredictor( ListIdx eListIdx, LumaIdx cIdx, LumaIdx cIdxEnd )
{
  const Bool bCurrentFieldFlag = m_rcMbCurr.getFieldFlag();
  //===== set motion vector predictors: A, B, C =====
  B4x4Idx cIdxA             = cIdx   .b4x4();
  B4x4Idx cIdxB             = cIdx   .b4x4();
  B4x4Idx cIdxD             = cIdx   .b4x4();
  B4x4Idx cIdxC             = cIdxEnd.b4x4();

  const MbData& rcMbDataA   = xGetBlockLeft      ( cIdxA );
  const MbData& rcMbDataB   = xGetBlockAbove     ( cIdxB );
  const MbData& rcMbDataC   = xGetBlockAboveRight( cIdxC );

  rcMbDataA.getMbMotionData( eListIdx ).getMv3DNeighbour( m_cMv3D_A, cIdxA, bCurrentFieldFlag );
  rcMbDataB.getMbMotionData( eListIdx ).getMv3DNeighbour( m_cMv3D_B, cIdxB, bCurrentFieldFlag );
  rcMbDataC.getMbMotionData( eListIdx ).getMv3DNeighbour( m_cMv3D_C, cIdxC, bCurrentFieldFlag );
  if( m_cMv3D_C == BLOCK_NOT_AVAILABLE )
  {
    const MbData& rcMbDataD = xGetBlockAboveLeft ( cIdxD );
		rcMbDataD.getMbMotionData( eListIdx ).getMv3DNeighbour( m_cMv3D_C, cIdxD, bCurrentFieldFlag );
  }
}


Void MbDataAccess::xGetMvPredictorUseNeighbours( Mv& rcMvPred, SChar scRef, PredictionType ePredType )
{
  //===== check directional prediction types =====
  if( ( ePredType == PRED_A && m_cMv3D_A == scRef ) ||
      ( m_cMv3D_A == scRef  && m_cMv3D_B != scRef && m_cMv3D_C != scRef ) ||
      ( m_cMv3D_B == BLOCK_NOT_AVAILABLE && m_cMv3D_C == BLOCK_NOT_AVAILABLE ) )
  {
    rcMvPred = m_cMv3D_A;
    return;
  }
  if( ( ePredType == PRED_B && m_cMv3D_B == scRef ) ||
      ( m_cMv3D_A != scRef  && m_cMv3D_B == scRef && m_cMv3D_C != scRef ) )
  {
    rcMvPred = m_cMv3D_B;
    return;
  }
  if( ( ePredType == PRED_C && m_cMv3D_C == scRef ) ||
      ( m_cMv3D_A != scRef  && m_cMv3D_B != scRef && m_cMv3D_C == scRef ) )
  {
    rcMvPred = m_cMv3D_C;
    return;
  }

#define MEDIAN(a,b,c)  ((a)>(b)?(a)>(c)?(b)>(c)?(b):(c):(a):(b)>(c)?(a)>(c)?(a):(c):(b))
  {
    rcMvPred.setHor( MEDIAN( m_cMv3D_A.getHor(), m_cMv3D_B.getHor(), m_cMv3D_C.getHor() ) );
    rcMvPred.setVer( MEDIAN( m_cMv3D_A.getVer(), m_cMv3D_B.getVer(), m_cMv3D_C.getVer() ) );
  }
#undef MEDIAN
}


Void MbDataAccess::getMvPredictorSkipMode()
{
  Mv cMvPred;
  xGetMvPredictor( cMvPred, 1, LIST_0, MEDIAN, B4x4Idx(0), B4x4Idx(3) );

  if( ( m_cMv3D_A.getRef()==BLOCK_NOT_AVAILABLE ||
        m_cMv3D_B.getRef()==BLOCK_NOT_AVAILABLE      ) ||
      ( m_cMv3D_A.getHor()==0 && m_cMv3D_A.getVer()==0 && m_cMv3D_A.getRef()==1 ) ||
      ( m_cMv3D_B.getHor()==0 && m_cMv3D_B.getVer()==0 && m_cMv3D_B.getRef()==1 )   )
  {
    cMvPred.setZero();
  }

  getMbMotionData( LIST_0 ).setRefIdx ( 1 );
  getMbMotionData( LIST_0 ).setAllMv( cMvPred );
}



Void MbDataAccess::getMvPredictorSkipMode( Mv& rcMvPred )
{
  xGetMvPredictor( rcMvPred, 1, LIST_0, MEDIAN, B4x4Idx(0), B4x4Idx(3) );

  if( ( m_cMv3D_A.getRef()==BLOCK_NOT_AVAILABLE ||
        m_cMv3D_B.getRef()==BLOCK_NOT_AVAILABLE      ) ||
      ( m_cMv3D_A.getHor()==0 && m_cMv3D_A.getVer()==0 && m_cMv3D_A.getRef()==1 ) ||
      ( m_cMv3D_B.getHor()==0 && m_cMv3D_B.getVer()==0 && m_cMv3D_B.getRef()==1 )   )
  {
    rcMvPred.setZero();
  }
}



Void MbDataAccess::addMvPredictors( std::vector<Mv>& rcMvPredList ) const
{
  rcMvPredList.push_back( m_cMv3D_A );
  rcMvPredList.push_back( m_cMv3D_B );
  rcMvPredList.push_back( m_cMv3D_C );
}



Void MbDataAccess::setAvailableMask()
{
  B4x4Idx   cIdx;
  B4x4Idx   cIdx0(0);
  B4x4Idx   cIdx3(3);
  B4x4Idx   cIdx8(8);

  //===== get availability for upper half of the macroblock =====
  UInt  bAvailable0 = 0;
  if( ! xIsAvailableIntra( xGetBlockLeft( ( cIdx = cIdx0 ) ) ) )
  {
    bAvailable0 |= NO_LEFT_REF;
  }
  if( ! xIsAvailableIntra( xGetBlockAbove( ( cIdx = cIdx0 ) ) ) )
  {
    bAvailable0 |= NO_ABOVE_REF;
  }
  if( ! xIsAvailableIntra( xGetBlockAboveLeft( ( cIdx = cIdx0 ) ) ) )
  {
    bAvailable0 |= NO_ABOVELEFT_REF;
  }
  if( ! xIsAvailableIntra( xGetBlockAboveRight( ( cIdx = cIdx3 ) ) ) )
  {
    bAvailable0 |= NO_ABOVERIGHT_REF;
  }

  //===== get availability for lower half of the macroblock =====
  UInt  bAvailable8 = NO_ABOVERIGHT_REF;
  if( ! xIsAvailableIntra( xGetBlockLeft( ( cIdx = cIdx8 ) ) ) )
  {
    bAvailable8 |= NO_LEFT_REF;
  }
  if( ! xIsAvailableIntra( xGetBlockAboveLeft( ( cIdx = cIdx8 ) ) ) )
  {
    bAvailable8 |= NO_ABOVELEFT_REF;
  }

  if( ! getMbData().getFieldFlag() && m_rcMbLeft.getFieldFlag() )
  {
    if( ! ( bAvailable0 & NO_LEFT_REF ) )
    {
      if( ! xIsAvailableIntra( xGetBlockLeftBottom( cIdx = cIdx0 ) ) )
      {
        bAvailable0 |= NO_LEFT_REF;
      }
    }
    if( ! ( bAvailable8 & NO_LEFT_REF ) )
    {
      if( ! xIsAvailableIntra( xGetBlockLeftBottom( cIdx = cIdx0 ) ) )
      {
        bAvailable8 |= NO_LEFT_REF;
      }
    }
  }

  m_uiAvailable = bAvailable0 + ( bAvailable8 << 4 );
}



UInt MbDataAccess::getConvertBlkMode( Par8x8 ePar8x8 )
{
  const BlkMode eBlkMode = m_rcMbCurr.getBlkMode( ePar8x8 );
  AOT_DBG( m_rcSliceHeader.isIntraSlice() );
  ROTRS( ! m_rcSliceHeader.isBSlice(), eBlkMode - BLK_8x8 );

  UInt uiCode;

  UInt uiFwdBwd = m_rcMbCurr.getBlockFwdBwd( ePar8x8 );
  switch( eBlkMode )
  {
  case BLK_SKIP:
    {
      uiCode = 0;
      break;
    }
  case BLK_8x8:
    {
      uiCode = uiFwdBwd;
      break;
    }
  case BLK_8x4:
    {
      uiCode = 2+2*uiFwdBwd;
      break;
    }
  case BLK_4x8:
    {
      uiCode = 3+2*uiFwdBwd;
      break;
    }
  case BLK_4x4:
    {
      uiCode = 9 + uiFwdBwd;
      break;
    }
  default:
    {
      AF();
      return MSYS_UINT_MAX;
    }
  }
  return uiCode;
}



ErrVal MbDataAccess::setConvertBlkMode( Par8x8 ePar8x8, UInt uiBlockMode )
{
  if( m_rcSliceHeader.isBSlice() )
  {
    ROT( uiBlockMode > 13 );
    m_rcMbCurr.setBlkMode( ePar8x8, m_aucBMTabB0[uiBlockMode] );
    m_rcMbCurr.addFwdBwd(  ePar8x8, m_aucBMTabB1[uiBlockMode] );
    return Err::m_nOK;
  }
  else
  {
    ROT( uiBlockMode >= 4 );
    m_rcMbCurr.setBlkMode( ePar8x8, m_aucBMTabP[uiBlockMode] );
    m_rcMbCurr.addFwdBwd(  ePar8x8, 1 );
    return Err::m_nOK;
  }
  return Err::m_nERR;
}



UInt MbDataAccess::getConvertMbType()
{
  MbMode eMbMode = m_rcMbCurr.getMbMode();

  ROTRS( m_rcSliceHeader.isPSlice(), eMbMode );

  ROTRS( m_rcSliceHeader.isIntraSlice(), eMbMode - INTRA_4X4 );

  if( m_rcSliceHeader.isBSlice() )
  {
    UInt uiMbType = 0;
    switch( eMbMode )
    {
    case MODE_SKIP:
      {
        uiMbType = 1;
        break;
      }
    case MODE_16x16:
      {
         uiMbType = 1 + m_rcMbCurr.getBlockFwdBwd( B_8x8_0 );
         break;
      }
    case MODE_16x8:
      {
        UInt uiIndex = 3*m_rcMbCurr.getBlockFwdBwd( B_8x8_0 );
        uiIndex     -=   m_rcMbCurr.getBlockFwdBwd( B_8x8_2 );
        uiMbType = 1 + m_aucMbType1x2[uiIndex];
        break;
      }
    case MODE_8x16:
      {
        UInt uiIndex = 3*m_rcMbCurr.getBlockFwdBwd( B_8x8_0 );
        uiIndex     -=   m_rcMbCurr.getBlockFwdBwd( B_8x8_1 );
        uiMbType = 1 + m_aucMbType2x1[uiIndex];
        break;
      }
    case MODE_8x8:
      {
        uiMbType = 1 + 22;
        break;
      }
    case MODE_PCM:
      {
        uiMbType = 1 + 48;
        break;
      }
    default:
      {
        ROT( eMbMode < INTRA_4X4 );
        uiMbType = 1 + (eMbMode - INTRA_4X4 + 23);
        break;
      }
    }
    return uiMbType;
  }

  AF();
  return MSYS_UINT_MAX;
}




ErrVal  MbDataAccess::setConvertMbType( UInt uiMbType )
{
  if( m_rcSliceHeader.isBSlice() )
  {
    if( uiMbType < 23 )
    {
      m_rcMbCurr.setMbMode( m_aausInterBMbType0[ uiMbType ] );
      m_rcMbCurr.setFwdBwd( m_aausInterBMbType1[ uiMbType ] );
      return Err::m_nOK;
    }

    ROT( uiMbType > 25 + 23 );
    m_rcMbCurr.setMbMode( MbMode(uiMbType-23+INTRA_4X4) );
    m_rcMbCurr.setFwdBwd( 0 );
    return Err::m_nOK;
  }

  if( m_rcSliceHeader.isIntraSlice() )
  {
    ROT( uiMbType > 25 );
    m_rcMbCurr.setMbMode( MbMode(uiMbType + INTRA_4X4) );
    m_rcMbCurr.setFwdBwd( 0 );
    return Err::m_nOK;
  }

  // inter P
  m_rcMbCurr.setMbMode( MbMode(++uiMbType) );
  m_rcMbCurr.setFwdBwd( (uiMbType < INTRA_4X4) ? 0x1111 : 0 );
  ROT( uiMbType > 25 + 6 );
  return Err::m_nOK;
}

ErrVal
MbDataAccess::setSVCDirectModeMvAndRef( RefFrameList& rcRefList0,
                                        RefFrameList& rcRefList1,
                                        Int           i8x8Blk     /* = -1 for entire macroblock */ )
{
  UInt  uiFirst4x4Index = ( i8x8Blk == 3 ? 10 : i8x8Blk == 2 ? 8 : i8x8Blk == 1 ? 2 : i8x8Blk == 0 ? 0 : 0 );
  UInt  uiLast4x4Index  = ( i8x8Blk == 3 ? 11 : i8x8Blk == 2 ? 9 : i8x8Blk == 1 ? 3 : i8x8Blk == 0 ? 1 : 3 );
  SChar ascRefIdx [2];
  Mv    acMv      [2];
  for(  UInt uiListIdx = 0; uiListIdx < 2; uiListIdx++ )
  {
    ListIdx eListIdx = ( uiListIdx ? LIST_1 : LIST_0 );
    xSetNeighboursMvPredictor( eListIdx, B4x4Idx( uiFirst4x4Index ), B4x4Idx( uiLast4x4Index ) );
    if( ( ascRefIdx[uiListIdx] = m_cMv3D_A.minRefIdx( m_cMv3D_B ).minRefIdx( m_cMv3D_C ).getRef() ) > 0 )
    {
      xGetMvPredictorUseNeighbours( acMv[uiListIdx], ascRefIdx[uiListIdx], MEDIAN );
    }
  }
  if( ascRefIdx[0] < 1 && ascRefIdx[1] < 1 )
  {
    ascRefIdx[0] = 1;
    ascRefIdx[1] = 1;
  }
  ROF( ascRefIdx[0] <= (SChar)rcRefList0.getActive() && ascRefIdx[1] <= (SChar)rcRefList1.getActive() );
  if ( i8x8Blk < 0 || i8x8Blk > 3 ) // 16x16
  {
    getMbMotionData( LIST_0 ).setRefIdx( ascRefIdx[0] );
    getMbMotionData( LIST_1 ).setRefIdx( ascRefIdx[1] );
    getMbMotionData( LIST_0 ).setAllMv ( acMv     [0] );
    getMbMotionData( LIST_1 ).setAllMv ( acMv     [1] );
  }
  else
  {
    ParIdx8x8 eParIdx8x8 = ( i8x8Blk == 0 ? PART_8x8_0 : i8x8Blk == 1 ? PART_8x8_1 : i8x8Blk == 2 ? PART_8x8_2 : PART_8x8_3 );
    getMbMotionData( LIST_0 ).setRefIdx( ascRefIdx[0], eParIdx8x8 );
    getMbMotionData( LIST_1 ).setRefIdx( ascRefIdx[1], eParIdx8x8 );
    getMbMotionData( LIST_0 ).setAllMv ( acMv     [0], eParIdx8x8 );
    getMbMotionData( LIST_1 ).setAllMv ( acMv     [1], eParIdx8x8 );
  }
  return Err::m_nOK;
}

Bool MbDataAccess::getMvPredictorDirect( ParIdx8x8 eParIdx,
                                         Bool& rbOneMv,
                                         Bool bFaultTolerant,
                                         RefFrameList* pcL0RefFrameList,
                                         RefFrameList* pcL1RefFrameList )
{
  rbOneMv = getSH().getSPS().getDirect8x8InferenceFlag();

  if( getSH().getDirectSpatialMvPredFlag() )
  {
    return xSpatialDirectMode ( eParIdx, rbOneMv, pcL0RefFrameList, pcL1RefFrameList );
  }
  return xTemporalDirectMode( eParIdx, rbOneMv, bFaultTolerant );
}

Bool MbDataAccess::xSpatialDirectMode( ParIdx8x8 eParIdx, Bool b8x8, RefFrameList* pcL0RefFrameList, RefFrameList* pcL1RefFrameList )

{
  UInt          uiLstIdx;
  Bool          bDirectZeroPred   = false;
  Bool          bAllColNonZero    = false;
  Bool          bColZeroFlagBlk0  = false;
  Bool          bColZeroFlagBlk1  = false;
  Bool          bColZeroFlagBlk2  = false;
  Bool          bColZeroFlagBlk3  = false;

  //===== get reference indices and spatially predicted motion vectors =====
  Mv    acMvPred[2];
  SChar ascRefIdx[2];
  xSetNeighboursMvPredictor(LIST_0, B4x4Idx(0), B4x4Idx(3) );
  if( ( ascRefIdx[LIST_0] = m_cMv3D_A.minRefIdx( m_cMv3D_B ).minRefIdx( m_cMv3D_C ).getRef() ) > 0 )
  {
    xGetMvPredictorUseNeighbours( acMvPred[LIST_0], ascRefIdx[LIST_0], MEDIAN );
  }
  xSetNeighboursMvPredictor(LIST_1, B4x4Idx(0), B4x4Idx(3) );
  if( ( ascRefIdx[LIST_1] = m_cMv3D_A.minRefIdx( m_cMv3D_B ).minRefIdx( m_cMv3D_C ).getRef() ) > 0 )
  {
    xGetMvPredictorUseNeighbours( acMvPred[LIST_1], ascRefIdx[LIST_1], MEDIAN );
  }

  //===== check reference indices =====
  if( ascRefIdx[LIST_0] < 1 && ascRefIdx[LIST_1] < 1 )
  {
    ascRefIdx[LIST_0] = 1;
    ascRefIdx[LIST_1] = 1;
    bDirectZeroPred   = true;
    bAllColNonZero    = true;
  }

  //===== check co-located =====
  if( ! bAllColNonZero )
  {
    SChar   scRefIdxCol;
    Mv      acMvCol[4];

    if( ! bAllColNonZero )
    {
      if( NULL != pcL0RefFrameList && NULL != pcL1RefFrameList )
      {
        bAllColNonZero  = (*pcL1RefFrameList)[1]->isLongTerm();
      }
      else
      {
        AOT(1);
      }
    }

    if( ! bAllColNonZero )
    {
      if( b8x8 )
      {
        SParIdx4x4 eSubMbPartIdx = ( eParIdx <= PART_8x8_1 ? ( eParIdx == PART_8x8_0 ? SPART_4x4_0 : SPART_4x4_1 )
                                                           : ( eParIdx == PART_8x8_2 ? SPART_4x4_2 : SPART_4x4_3 ) );

        xGetColocatedMvRefIdx( acMvCol[0], scRefIdxCol, B4x4Idx( eParIdx + eSubMbPartIdx ) );
      }
      else
      {
        //===== THIS SHALL NEVER BE CALLED FOR INTERLACED SEQUENCES =====
        xGetColocatedMvsRefIdxNonInterlaced( acMvCol, scRefIdxCol, eParIdx );
      }

      bAllColNonZero = ( scRefIdxCol != 1 );
    }

    if( ! bAllColNonZero )
    {
      bColZeroFlagBlk0   = ( acMvCol[0].getAbsHor() <= 1 && acMvCol[0].getAbsVer() <= 1 );

      if( ! b8x8 )
      {
        bColZeroFlagBlk1 = ( acMvCol[1].getAbsHor() <= 1 && acMvCol[1].getAbsVer() <= 1 );
        bColZeroFlagBlk2 = ( acMvCol[2].getAbsHor() <= 1 && acMvCol[2].getAbsVer() <= 1 );
        bColZeroFlagBlk3 = ( acMvCol[3].getAbsHor() <= 1 && acMvCol[3].getAbsVer() <= 1 );
      }
    }
  }

  //===== set motion vectors and reference frames =====
  for( uiLstIdx = 0; uiLstIdx < 2; uiLstIdx++ )
  {
    ListIdx       eListIdx          = ListIdx( uiLstIdx );
    MbMotionData& rcMbMotionDataLX  = getMbMotionData( eListIdx );
    SChar         scRefIdx          = ascRefIdx[ eListIdx ];
    Bool          bZeroMv;
		Bool          bMvValid = true;

    //----- set motion vectors -----
    if( b8x8 || bAllColNonZero || scRefIdx < 1 )
    {
      bZeroMv         = ( bDirectZeroPred || scRefIdx < 1 || ( scRefIdx == 1 && bColZeroFlagBlk0 ) );
      const Mv& rcMv  = ( bZeroMv ? Mv::ZeroMv() : acMvPred [ eListIdx ] );
      rcMbMotionDataLX.setAllMv( rcMv, eParIdx );
			bMvValid = xCheckMv( rcMv );
    }
    else
    {
      bZeroMv         = ( scRefIdx == 1 && bColZeroFlagBlk0 );
      const Mv& rcMv0 = ( bZeroMv ? Mv::ZeroMv() : acMvPred [ eListIdx ] );
      rcMbMotionDataLX.setAllMv( rcMv0, eParIdx, SPART_4x4_0 );
			bMvValid &= xCheckMv( rcMv0 );

      bZeroMv         = ( scRefIdx == 1 && bColZeroFlagBlk1 );
      const Mv& rcMv1 = ( bZeroMv ? Mv::ZeroMv() : acMvPred [ eListIdx ] );
      rcMbMotionDataLX.setAllMv( rcMv1, eParIdx, SPART_4x4_1 );
			bMvValid &= xCheckMv( rcMv1 );

      bZeroMv         = ( scRefIdx == 1 && bColZeroFlagBlk2 );
      const Mv& rcMv2 = ( bZeroMv ? Mv::ZeroMv() : acMvPred [ eListIdx ] );
      rcMbMotionDataLX.setAllMv( rcMv2, eParIdx, SPART_4x4_2 );
			bMvValid &= xCheckMv( rcMv2 );

      bZeroMv         = ( scRefIdx == 1 && bColZeroFlagBlk3 );
      const Mv& rcMv3 = ( bZeroMv ? Mv::ZeroMv() : acMvPred [ eListIdx ] );
      rcMbMotionDataLX.setAllMv( rcMv3, eParIdx, SPART_4x4_3 );
			bMvValid &= xCheckMv( rcMv3 );
    }

    //----- set reference indices and reference pictures -----
    rcMbMotionDataLX.setRefIdx ( scRefIdx,  eParIdx );

    if ( !bMvValid )
    {
      return false;
  }
}
	return true;
}


Bool
MbDataAccess::xTemporalDirectModeMvRef( Mv acMv[], SChar ascRefIdx[], LumaIdx cIdx, Bool bFaultTolerant )
{
  SChar             scRefIdxCol;
  Mv                cMvCol;
  const RefPicIdc&  rcRefPicCol = xGetColocatedMvRefPic( cMvCol, scRefIdxCol, cIdx );

  //----- get reference index for list 0 -----
  if( scRefIdxCol > 0 )
  {
    if( rcRefPicCol.isValid() )
    {
      const Frame* pcFrame = rcRefPicCol.getPic();
      AOF( pcFrame ); // something wrong
      if( ( getMbPicType() == FRAME || pcFrame->getPicType() == FRAME ) && ( getMbPicType() != pcFrame->getPicType() ) )
      {
        pcFrame = pcFrame->getFrame()->getPic( getMbPicType() );
        AOF( pcFrame );
      }
      RefFrameList& rcRefList = *m_rcSliceHeader.getRefFrameList( getMbPicType(), LIST_0 );
      SChar         scRefIdx  = BLOCK_NOT_AVAILABLE;
      for( UInt uiIndex = 1; uiIndex <= rcRefList.getSize(); uiIndex++ )
      {
        if( rcRefList[uiIndex] == pcFrame )
        {
          scRefIdx = uiIndex;
          break;
        }
      }
      ascRefIdx[LIST_0] = scRefIdx;
      if( scRefIdx < 1 )
      {
        AOT(1);
        ROFRS( bFaultTolerant, false ); // not allowed
        ascRefIdx[LIST_0] = 1;
      }
    }
    else
    {
      AOT(1);
      ROFRS( bFaultTolerant, false ); // not allowed
    }
  }

  Int iScale = m_rcSliceHeader.getDistScaleFactor( getMbPicType(), ascRefIdx[LIST_0], ascRefIdx[LIST_1] );
  if( iScale == 1024 )
  {
    acMv[LIST_0]  = cMvCol;
    acMv[LIST_1]  = Mv::ZeroMv();
  }
  else
  {
    acMv[LIST_0]  = cMvCol.scaleMv( iScale );
    acMv[LIST_1]  = acMv[LIST_0] - cMvCol;
  }
  Bool   bMvValid = xCheckMv( acMv[LIST_0]);
  return bMvValid;
}


Bool
MbDataAccess::xTemporalDirectModeMvsRefNonInterlaced( Mv aacMv[][4], SChar ascRefIdx[], ParIdx8x8 eParIdx, Bool bFaultTolerant )
{
  SChar             scRefIdxCol;
  Mv                acMvCol[4];
  const RefPicIdc&  rcRefPicCol = xGetColocatedMvsRefPicNonInterlaced( acMvCol, scRefIdxCol, eParIdx );

  //----- get reference index for list 0 -----
  if( scRefIdxCol > 0 )
  {
    if( rcRefPicCol.isValid() )
    {
      const Frame* pcFrame = rcRefPicCol.getPic();
      AOF( pcFrame );                         // something wrong
      AOF( pcFrame->getPicType() == FRAME );  // something wrong
      RefFrameList& rcRefList = *m_rcSliceHeader.getRefFrameList( getMbPicType(), LIST_0 );
      SChar         scRefIdx  = BLOCK_NOT_AVAILABLE;
      for( UInt uiIndex = 1; uiIndex <= rcRefList.getSize(); uiIndex++ )
      {
        if( rcRefList[uiIndex] == pcFrame )
        {
          scRefIdx = uiIndex;
          break;
        }
      }
      ascRefIdx[LIST_0] = scRefIdx;
      if( scRefIdx < 1 )
      {
        AOT(1);
        ROFRS( bFaultTolerant, false ); // not allowed
        ascRefIdx[LIST_0] = 1;
      }
    }
    else
    {
      AOT(1);
      ROFRS( bFaultTolerant, false ); // not allowed
    }
  }
  Bool bMvValid = true;
  Int iScale = m_rcSliceHeader.getDistScaleFactor     ( getMbPicType(), ascRefIdx[LIST_0], ascRefIdx[LIST_1] );
  if( iScale == 1024 )
  {
    for( UInt uiIndex = 0; uiIndex < 4; uiIndex++ )
    {
      aacMv[LIST_0][uiIndex] = acMvCol[uiIndex];
      aacMv[LIST_1][uiIndex] = Mv::ZeroMv();
    }
  }
  else
  {
    for( UInt uiIndex = 0; uiIndex < 4; uiIndex++ )
    {
      aacMv[LIST_0][uiIndex] = acMvCol[uiIndex].scaleMv( iScale );
      aacMv[LIST_1][uiIndex] = aacMv[LIST_0][uiIndex] - acMvCol[uiIndex];
      bMvValid &= xCheckMv( aacMv[LIST_0][uiIndex] );
    }
  }
  return bMvValid;
}


Bool MbDataAccess::xTemporalDirectMode( ParIdx8x8 eParIdx, Bool b8x8, Bool bFaultTolerant )
{
  Bool  bModeAllowed;
  SChar ascRefIdx[2]  = { 1, 1 };

  if( b8x8 )
  {
    Mv          acMv[2];
    SParIdx4x4  eSubMbPartIdx = ( eParIdx <= PART_8x8_1 ? ( eParIdx == PART_8x8_0 ? SPART_4x4_0 : SPART_4x4_1 )
                                                        : ( eParIdx == PART_8x8_2 ? SPART_4x4_2 : SPART_4x4_3 ) );
    B4x4Idx     cIdx          = B4x4Idx( eParIdx + eSubMbPartIdx );

    bModeAllowed = xTemporalDirectModeMvRef( acMv, ascRefIdx, cIdx, bFaultTolerant );
    ROFRS( bModeAllowed, bModeAllowed );
    for( UInt uiLstIdx = 0; uiLstIdx < 2; uiLstIdx++ )
    {
      ListIdx       eListIdx          = ListIdx( uiLstIdx );
      MbMotionData& rcMbMotionDataLX  = getMbMotionData( eListIdx );
      rcMbMotionDataLX.setAllMv    ( acMv     [eListIdx], eParIdx );
      rcMbMotionDataLX.setRefIdx   ( ascRefIdx[eListIdx], eParIdx );
    }
  }
  else // do not do this for interlaced stuff
  {
    Mv      aacMv[2][4];
    bModeAllowed = xTemporalDirectModeMvsRefNonInterlaced( aacMv, ascRefIdx, eParIdx, bFaultTolerant );
    ROFRS( bModeAllowed, bModeAllowed );
    for( UInt uiLstIdx = 0; uiLstIdx < 2; uiLstIdx++ )
    {
      ListIdx       eListIdx          = ListIdx( uiLstIdx );
      MbMotionData& rcMbMotionDataLX  = getMbMotionData( eListIdx );
      rcMbMotionDataLX.setAllMv     ( aacMv[eListIdx][0],  eParIdx, SPART_4x4_0 );
      rcMbMotionDataLX.setAllMv     ( aacMv[eListIdx][1],  eParIdx, SPART_4x4_1 );
      rcMbMotionDataLX.setAllMv     ( aacMv[eListIdx][2],  eParIdx, SPART_4x4_2 );
      rcMbMotionDataLX.setAllMv     ( aacMv[eListIdx][3],  eParIdx, SPART_4x4_3 );
      rcMbMotionDataLX.setRefIdx    ( ascRefIdx[eListIdx], eParIdx );
    }
  }

  return bModeAllowed;
}

H264AVC_NAMESPACE_END

