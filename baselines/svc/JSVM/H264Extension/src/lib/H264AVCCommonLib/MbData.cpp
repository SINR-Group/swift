
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/MbData.h"
#include "H264AVCCommonLib/MbDataCtrl.h"
#include "H264AVCCommonLib/MbDataAccess.h"


H264AVC_NAMESPACE_BEGIN



Void MbData::copy( const MbData& rcMbData )
{
  MbDataStruct::          copy    ( rcMbData );
  m_pcMbTCoeffs         ->copyFrom( *rcMbData.m_pcMbTCoeffs );
  m_apcMbMotionData [0] ->copyFrom( *rcMbData.m_apcMbMotionData [0] );
  m_apcMbMvdData    [0] ->copyFrom( *rcMbData.m_apcMbMvdData    [0] );
  m_apcMbMotionData [1] ->copyFrom( *rcMbData.m_apcMbMotionData [1] );
  m_apcMbMvdData    [1] ->copyFrom( *rcMbData.m_apcMbMvdData    [1] );
}


ErrVal
MbData::saveAll( FILE* pFile )
{
  RNOK( MbDataStruct::          save( pFile ) );
  RNOK( m_pcMbTCoeffs         ->save( pFile ) );
  RNOK( m_apcMbMotionData [0] ->save( pFile ) );
  RNOK( m_apcMbMvdData    [0] ->save( pFile ) );
  RNOK( m_apcMbMotionData [1] ->save( pFile ) );
  RNOK( m_apcMbMvdData    [1] ->save( pFile ) );

  return Err::m_nOK;
}


ErrVal
MbData::loadAll( FILE* pFile )
{
  RNOK( MbDataStruct::          load( pFile ) );
  RNOK( m_pcMbTCoeffs         ->load( pFile ) );
  RNOK( m_apcMbMotionData [0] ->load( pFile ) );
  RNOK( m_apcMbMvdData    [0] ->load( pFile ) );
  RNOK( m_apcMbMotionData [1] ->load( pFile ) );
  RNOK( m_apcMbMvdData    [1] ->load( pFile ) );

  return Err::m_nOK;
}

Bool MbData::calcBCBP( UInt uiStart, UInt uiStop, UInt uiPos ) const
{
  AOF( uiStart != uiStop );
  if( uiPos < 16 )
  {
    if( isTransformSize8x8() )
    {
      UInt uiTab[] = { 0, 1, 0, 1, 2, 3, 2, 3 };
      return ( ( calcMbCbp( uiStart, uiStop ) >> uiTab[uiPos>>1] ) & 1 ) != 0;
    }
    // Luma 4x4 block
    if( uiStart == 0 && isIntra16x16() )
      uiStart = 1;

    const UChar  *pucScan = getFieldFlag() ? g_aucFieldScan : g_aucFrameScan;
    const TCoeff *piCoeff = getMbTCoeffs().get( B4x4Idx(uiPos) );
    for( UInt ui = uiStart; ui < uiStop; ui++ )
    {
      if( piCoeff[pucScan[ui]] )
      {
        return true;
      }
    }
  }
  else if( uiPos < 24 )
  {
    // Chroma AC 4x4 block
    AOF( uiStop > 1 );
    uiStart = gMax( 1, uiStart );
    const UChar  *pucScan = getFieldFlag() ? g_aucFieldScan : g_aucFrameScan;
    const TCoeff *piCoeff = getMbTCoeffs().get( CIdx(uiPos - 16) );
    for( UInt ui = uiStart; ui < uiStop; ui++ )
    {
      if( piCoeff[pucScan[ui]] )
      {
        return true;
      }
    }
  }
  else if( uiPos < 26 )
  {
    // Chroma DC 4x4 block
    AOF( uiStart == 0 );
    CPlaneIdx cCPlane( uiPos - 24 );
    for( CIdx cCIdx( cCPlane ) ; cCIdx.isLegal( cCPlane ); cCIdx++ )
    {
      if( getMbTCoeffs().get( cCIdx )[0] )
      {
        return true;
      }
    }
  }
  else
  {
    AOF( uiPos == 26 );
    AOF( uiStart == 0 );
    if( !isIntra16x16() )
    {
      return false;
    }
    // Intra 16x16 DC coeff (scan is not important here)
    for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      if( getMbTCoeffs().get( cIdx )[0] )
      {
        return true;
      }
    }
  }
  return false;
}

UInt MbData::calcMbCbp( UInt uiStart, UInt uiStop ) const
{
  UInt uiCbp  = 0;
  for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
  {
    if( isTransformSize8x8() )
    {
      const UChar *pucScan = getFieldFlag() ? g_aucFieldScan64 : g_aucFrameScan64;
      const TCoeff *piCoeff = getMbTCoeffs().get8x8( c8x8Idx );
      for( UInt ui = uiStart*4; ui < uiStop*4; ui++ )
      {
        if( piCoeff[pucScan[ui]] )
        {
          uiCbp |= 0x33 << c8x8Idx.b8x8();
          break;
        }
      }
    }
    else
    {
      const UChar *pucScan = getFieldFlag() ? g_aucFieldScan : g_aucFrameScan;
      for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
      {
        const TCoeff *piCoeff = getMbTCoeffs().get( cIdx );
        for( UInt ui = uiStart; ui < uiStop; ui++ )
        {
          if( piCoeff[pucScan[ui]] )
          {
            uiCbp |= 1 << cIdx;
            break;
          }
        }
      }
    }
  }
  Bool bChroma   = false;
  Bool bChromaAC = false;
  for( CIdx cCIdx; cCIdx.isLegal(); cCIdx++ )
  {
    const UChar *pucScan = getFieldFlag() ? g_aucFieldScan : g_aucFrameScan;
    const TCoeff *piCoeff = getMbTCoeffs().get( cCIdx );
    for( UInt ui = uiStart; ui < uiStop; ui++ )
    {
      if( piCoeff[pucScan[ui]] )
      {
        bChroma = true;
        if( ui != 0 )
        {
          bChromaAC = true;
          break;
        }
      }
    }
  }

  if( isIntra16x16() )
  {
    uiCbp = ( ( uiCbp & 0xFFFF ) ? 15 : 0 ) |
            ( bChromaAC          ? 32 : ( bChroma ? 16 : 0 ) );
  }
  else
  {
    uiCbp = ( ( uiCbp & 0x0033 ) ?  1 : 0 ) |
            ( ( uiCbp & 0x00cc ) ?  2 : 0 ) |
            ( ( uiCbp & 0x3300 ) ?  4 : 0 ) |
            ( ( uiCbp & 0xcc00 ) ?  8 : 0 ) |
            ( bChromaAC          ? 32 : ( bChroma ? 16 : 0 ) );
  }
  return uiCbp;
}

ErrVal
MbData::copyMotion( const MbData& rcMbData,
                    UInt    uiSliceId )
{
  m_uiSliceId   = ( uiSliceId != MSYS_UINT_MAX ? uiSliceId : m_uiSliceId );
  m_bBLSkipFlag = rcMbData.m_bBLSkipFlag;
  m_eMbMode     = rcMbData.m_eMbMode;
  m_aBlkMode[0] = rcMbData.m_aBlkMode[0];
  m_aBlkMode[1] = rcMbData.m_aBlkMode[1];
  m_aBlkMode[2] = rcMbData.m_aBlkMode[2];
  m_aBlkMode[3] = rcMbData.m_aBlkMode[3];
  m_usFwdBwd    = rcMbData.m_usFwdBwd;
  m_bFieldFlag  = rcMbData.m_bFieldFlag;

  m_apcMbMotionData[0]->copyFrom( *rcMbData.m_apcMbMotionData[0] );
  m_apcMbMotionData[1]->copyFrom( *rcMbData.m_apcMbMotionData[1] );

  m_apcMbMvdData[0]->copyFrom( *rcMbData.m_apcMbMvdData[0] );
  m_apcMbMvdData[1]->copyFrom( *rcMbData.m_apcMbMvdData[1] );

  return Err::m_nOK;
}



// for SVC to AVC rewrite
ErrVal
MbData::copyIntraPred( MbData &rcMbData )
{
	// COPY INTRA PREDICTION MODES
	memcpy(m_ascIPredMode, rcMbData.m_ascIPredMode, sizeof(m_ascIPredMode) );
	memcpy( &m_ucChromaPredMode, &rcMbData.m_ucChromaPredMode, sizeof(m_ucChromaPredMode) );

	// COPY TRANSFORM SIZE
	m_bTransformSize8x8 = rcMbData.m_bTransformSize8x8;

    // COPY CBP
   setMbExtCbp( rcMbData.getMbExtCbp() );

	// COPY QUANTIZER DATA
	m_ucQp    = rcMbData.m_ucQp;
  m_ucQp4LF = rcMbData.m_ucQp4LF;

	return Err::m_nOK;
}

// for SVC to AVC rewrite
ErrVal
MbData::copyTCoeffs( MbData& rcMbData )
{
  // DECLARATIONS
  TCoeff *piCoeff;

  // COPY COEFFICIENTS
  // Luma
  for( B4x4Idx b4x4Idx; b4x4Idx.isLegal(); b4x4Idx++ )
  {
	  piCoeff = getMbTCoeffs().get(b4x4Idx);
    memcpy( piCoeff, rcMbData.getMbTCoeffs().get(b4x4Idx), 16*sizeof(TCoeff) );
  }

  // Chroma
  for( CIdx cIdx; cIdx.isLegal(); cIdx++ )
  {
	  piCoeff = getMbTCoeffs().get(cIdx);
	  memcpy( piCoeff, rcMbData.getMbTCoeffs().get(cIdx), 16*sizeof(TCoeff) );
  }

  // COPY TRANSFORM SIZE
  m_bTransformSize8x8 = rcMbData.m_bTransformSize8x8;

  // COPY CBP
  setMbExtCbp( rcMbData.getMbExtCbp() );

  // COPY QP
  m_ucQp    = rcMbData.m_ucQp;
  m_ucQp4LF = rcMbData.m_ucQp4LF;

  return Err::m_nOK;
}



MbStatus::MbStatus()
: m_uiSliceIdc            ( MSYS_UINT_MAX )
, m_uiQ0SliceIdc          ( MSYS_UINT_MAX )
, m_uiLastCodedSliceIdc   ( MSYS_UINT_MAX )
, m_uiLastCodedDQId       ( MSYS_UINT_MAX )
, m_uiMbCbpDQId0          ( 0 )
, m_bIsCoded              ( false )
, m_pcSliceHeader         ( 0 )
, m_pcLastCodedSliceHeader( 0 )
{
}

MbStatus::~MbStatus()
{
}

Void
MbStatus::reset()
{
  m_uiSliceIdc              = MSYS_UINT_MAX;
  m_uiQ0SliceIdc            = MSYS_UINT_MAX;
  m_uiLastCodedSliceIdc     = MSYS_UINT_MAX;
  m_uiLastCodedDQId         = MSYS_UINT_MAX;
  m_uiMbCbpDQId0            = 0;
  m_bIsCoded                = false;
  m_pcSliceHeader           = 0;
  m_pcLastCodedSliceHeader  = 0;
}

Bool
MbStatus::canBeUpdated( const SliceHeader& rcSliceHeader )
{
  ROTRS( rcSliceHeader.getQualityId() == 0 && m_uiSliceIdc == MSYS_UINT_MAX,                   true );
  ROTRS( rcSliceHeader.getQualityId() != 0 && rcSliceHeader.getRefLayerDQId() == getDQId() &&
        ( m_bIsCoded || !rcSliceHeader.isTrueSlice() ),                                       true );
  return false;
}

ErrVal
MbStatus::update( MbDataAccess& rcMbDataAccess )
{
  SliceHeader& rcSliceHeader = rcMbDataAccess.getSH();
  ROF( canBeUpdated( rcSliceHeader ) );
  m_uiSliceIdc        =  rcSliceHeader.getFirstMbInSlice() << 7;
  m_uiSliceIdc       +=  rcSliceHeader.getDependencyId  () << 4;
  m_uiSliceIdc       +=  rcSliceHeader.getQualityId     ();
  m_bIsCoded          =  rcSliceHeader.isTrueSlice      ();
  m_pcSliceHeader     = &rcSliceHeader;
  if( rcSliceHeader.getQualityId() == 0 )
  {
    m_uiQ0SliceIdc    =  m_uiSliceIdc;
  }
  if( m_bIsCoded )
  {
    m_uiLastCodedSliceIdc     =  m_uiSliceIdc;
    m_uiLastCodedDQId         =  getDQId();
    if( m_uiLastCodedDQId == 0 )
    {
      m_uiMbCbpDQId0          =  rcMbDataAccess.getMbData().getMbExtCbp();
    }
    m_pcLastCodedSliceHeader  = m_pcSliceHeader;
  }
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
