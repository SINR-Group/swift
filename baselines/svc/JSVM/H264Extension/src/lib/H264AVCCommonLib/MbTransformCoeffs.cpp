
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/MbTransformCoeffs.h"
#include "H264AVCCommonLib/YuvMbBuffer.h"

#include <stdio.h>


H264AVC_NAMESPACE_BEGIN

ErrVal
MbTransformCoeffs::save( FILE* pFile )
{
  ROF( pFile );

  UInt uiSave  = (UInt) ::fwrite( this, sizeof(MbTransformCoeffs), 1, pFile );

  ROF( uiSave == 1 );

  return Err::m_nOK;
}


ErrVal
MbTransformCoeffs::load( FILE* pFile )
{
  ROF( pFile );

  UInt uiRead  = (UInt) ::fread( this, sizeof(MbTransformCoeffs), 1, pFile );

  ROF( uiRead == 1 );

  return Err::m_nOK;
}



Void MbTransformCoeffs::clear()
{
  ::memset( m_aaiLevel, 0, sizeof( m_aaiLevel ) );
  ::memset( m_aaucCoeffCount, 0, sizeof(m_aaucCoeffCount));
}

Void MbTransformCoeffs::clearAcBlk( ChromaIdx cChromaIdx )
{
  ::memset( &m_aaiLevel[16+cChromaIdx][1], 0, sizeof( TCoeff) * 15 );
}

ErrVal MbTransformCoeffs::clearPrediction()
{
  TCoeff *pcDst     = get( B4x4Idx(0) );
  for( UInt ui = 0; ui < 384; ui++ )
  {
    (pcDst++)->setSPred( 0 );
  }
  return Err::m_nOK;
}

Bool
MbTransformCoeffs::allCoeffsZero() const
{
  const TCoeff* pcDst = get( B4x4Idx(0) );
  for( UInt ui = 0; ui < 384; ui++ )
  {
    ROTRS( pcDst[ui].getCoeff(), false );
  }
  return true;  
}

Bool  
MbTransformCoeffs::allLevelsZero() const
{
  const TCoeff* pcDst = get( B4x4Idx(0) );
  for( UInt ui = 0; ui < 384; ui++ )
  {
    ROTRS( pcDst[ui].getLevel() , false );
  }
  return true;  
}

Bool  
MbTransformCoeffs::allLevelsAndPredictionsZero() const
{
  const TCoeff* pcDst = get( B4x4Idx(0) );
  for( UInt ui = 0; ui < 384; ui++ )
  {
    ROTRS( pcDst[ui].getLevel() || pcDst[ui].getSPred(), false );
  }
  return true;  
}

ErrVal MbTransformCoeffs::copyPredictionFrom( YuvMbBuffer &rcPred )
{
  TCoeff *pcDst     = get( B4x4Idx(0) );
  XPel   *pSrc      = rcPred.getMbLumAddr();
  Int   iSrcStride  = rcPred.getLStride();
  UInt uiY, uiX;
  for( uiY = 0; uiY < 16; uiY++ )
  {
    for( uiX = 0; uiX < 16; uiX++ )
    {
      (pcDst++)->setSPred( pSrc[uiX] );
    }
    pSrc += iSrcStride;
  }
  iSrcStride = rcPred.getCStride();
  pSrc       = rcPred.getMbCbAddr();

  for( uiY = 0; uiY < 8; uiY++ )
  {
    for( uiX = 0; uiX < 8; uiX++ )
    {
      (pcDst++)->setSPred( pSrc[uiX] );
    }
    pSrc += iSrcStride;
  }

  pSrc       = rcPred.getMbCrAddr();

  for( uiY = 0; uiY < 8; uiY++ )
  {
    for( uiX = 0; uiX < 8; uiX++ )
    {
      (pcDst++)->setSPred( pSrc[uiX] );
    }
    pSrc += iSrcStride;
  }
  return Err::m_nOK;
}

ErrVal MbTransformCoeffs::copyPredictionTo( YuvMbBuffer &rcPred )
{
  TCoeff *pcSrc     = get( B4x4Idx(0) );
  XPel   *pcDst      = rcPred.getMbLumAddr();
  Int   iSrcStride  = rcPred.getLStride();
  UInt uiY, uiX;
  for( uiY = 0; uiY < 16; uiY++ )
  {
    for( uiX = 0; uiX < 16; uiX++ )
    {
      pcDst[uiX] = (pcSrc++)->getSPred();
    }
    pcDst += iSrcStride;
  }
  iSrcStride = rcPred.getCStride();
  pcDst       = rcPred.getMbCbAddr();

  for( uiY = 0; uiY < 8; uiY++ )
  {
    for( uiX = 0; uiX < 8; uiX++ )
    {
      pcDst[uiX] = (pcSrc++)->getSPred();
    }
    pcDst += iSrcStride;
  }

  pcDst       = rcPred.getMbCrAddr();

  for( uiY = 0; uiY < 8; uiY++ )
  {
    for( uiX = 0; uiX < 8; uiX++ )
    {
      pcDst[uiX] = (pcSrc++)->getSPred();
    }
    pcDst += iSrcStride;
  }
  return Err::m_nOK;
}



Void
MbTransformCoeffs::clearLumaLevels()
{
  ::memset( &m_aaiLevel[0][0], 0, sizeof(TCoeff)*16*16 );
}

Void
MbTransformCoeffs::clearChromaLevels()
{
  ::memset( get( CIdx(0) ), 0, sizeof(TCoeff)*128 );
}

Void
MbTransformCoeffs::clearLumaLevels4x4( LumaIdx c4x4Idx )
{
  ::memset( get( c4x4Idx ), 0, sizeof(TCoeff)*16 );
}

Void
MbTransformCoeffs::clearLumaLevels8x8( B8x8Idx c8x8Idx )
{
  UInt uiIndex = c8x8Idx.b8x8();
  ::memset( &m_aaiLevel[uiIndex  ][0], 0, sizeof(TCoeff)*16 );
  ::memset( &m_aaiLevel[uiIndex+1][0], 0, sizeof(TCoeff)*16 );
  ::memset( &m_aaiLevel[uiIndex+4][0], 0, sizeof(TCoeff)*16 );
  ::memset( &m_aaiLevel[uiIndex+5][0], 0, sizeof(TCoeff)*16 );
}

Void
MbTransformCoeffs::clearLumaLevels8x8Block( B8x8Idx c8x8Idx )
{
  ::memset( get8x8( c8x8Idx ), 0, sizeof(TCoeff)*64 );
}


UInt MbTransformCoeffs::calcCoeffCount( LumaIdx cLumaIdx, Bool bIs8x8, Bool bInterlaced, UInt uiStart, UInt uiStop ) const
{
  UInt uiCount = 0;
  if( bIs8x8 )
  {
    const UChar *pucScan = bInterlaced ? g_aucFieldScan64 : g_aucFrameScan64;
    B8x8Idx c8x8Idx( cLumaIdx.getPar8x8() );
    UInt uiBlock = cLumaIdx.getS4x4();
    const TCoeff *piCoeff = get8x8( c8x8Idx );
    for( UInt ui = uiStart; ui < uiStop; ui++ )
    {
      if( piCoeff[pucScan[ui*4+uiBlock]] )
        uiCount++;
    }
  }
  else
  {
    const UChar *pucScan = bInterlaced ? g_aucFieldScan : g_aucFrameScan;
    const TCoeff *piCoeff = get( cLumaIdx );
    for( UInt ui = uiStart; ui < uiStop; ui++ )
    {
      if( piCoeff[pucScan[ui]] )
        uiCount++;
    }
  }
  return uiCount;
}

UInt MbTransformCoeffs::calcCoeffCount( ChromaIdx cChromaIdx, const UChar *pucScan, UInt uiStart, UInt uiStop ) const
{
  const TCoeff *piCoeff = get( cChromaIdx );
  UInt uiCount = 0;
  for( UInt ui = gMax( 1, uiStart ); ui < uiStop; ui++ )
  {
    if( piCoeff[pucScan[ui]] )
      uiCount++;
  }
  return uiCount;
}


Void MbTransformCoeffs::setAllCoeffCount( UChar ucCoeffCountValue )
{
  ::memset( m_aaucCoeffCount, ucCoeffCountValue, sizeof(m_aaucCoeffCount));
}

Void MbTransformCoeffs::copyFrom( const MbTransformCoeffs& rcMbTransformCoeffs )
{
  memcpy( m_aaiLevel, rcMbTransformCoeffs.m_aaiLevel, sizeof( m_aaiLevel ) );
  memcpy( m_aaucCoeffCount, rcMbTransformCoeffs.m_aaucCoeffCount, sizeof( m_aaucCoeffCount ) );
}

Void MbTransformCoeffs::copyCoeffCounts( const MbTransformCoeffs& rcMbTransformCoeffs )
{
  memcpy( m_aaucCoeffCount, rcMbTransformCoeffs.m_aaucCoeffCount, sizeof( m_aaucCoeffCount ) );
}

MbTransformCoeffs::MbTransformCoeffs()
{
  clear();
}



Void
MbTransformCoeffs::clearNewLumaLevels( MbTransformCoeffs& rcBaseMbTCoeffs )
{
  for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
  {
    clearNewLumaLevels8x8Block( c8x8Idx, rcBaseMbTCoeffs );
  }
}

Void
MbTransformCoeffs::clearNewLumaLevels8x8( B8x8Idx             c8x8Idx,
                                          MbTransformCoeffs&  rcBaseMbTCoeffs )
{
  for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
  {
    TCoeff* piCoeff     = get( cIdx );
    TCoeff* piCoeffBase = rcBaseMbTCoeffs.get( cIdx );
    for( UInt ui = 0; ui < 16; ui++ )
    {
      if( ! piCoeffBase[ui] )
      {
        piCoeff[ui] = 0;
      }
    }
  }
}

Void
MbTransformCoeffs::clearNewLumaLevels8x8Block( B8x8Idx            c8x8Idx,
                                               MbTransformCoeffs& rcBaseMbTCoeffs )
{
  TCoeff* piCoeff     = get8x8( c8x8Idx );
  TCoeff* piCoeffBase = rcBaseMbTCoeffs.get8x8( c8x8Idx );
  for( UInt ui = 0; ui < 64; ui++ )
  {
    if( !piCoeffBase[ui] )
    {
      piCoeff[ui] = 0;
    }
  }
}


Void MbTransformCoeffs::dump ( FILE* hFile ) const
{
#if 0
return;
#endif

	for( unsigned char i=0; i<24; i++ )
	{
		for( unsigned char j=0; j<16; j++ )
		{
			::fprintf( hFile, "[%2u][%2u]=%5d\t", i, j, (Int)(Short)m_aaiLevel[i][j] );
		}
		::fprintf( hFile, "\n" );
	}
}

// functions for SVC to AVC rewrite JVT-V035
Void
MbTransformCoeffs::storeLevelData()
{
	for( B4x4Idx bIdx; bIdx.isLegal(); bIdx++ )
	{
		//TCoeff* piCoeff = m_aaiLevel[bIdx];
		TCoeff* piCoeff = get( bIdx );
		for( UInt ui=0; ui<16; ui++ )
			piCoeff[ui].setLevel( piCoeff[ui].getCoeff() );
	}

	for( CIdx cIdx; cIdx.isLegal(); cIdx++ )
	{
		//TCoeff* piCoeff = m_aaiLevel[cIdx];
		TCoeff* piCoeff = get( cIdx );
		for( UInt ui=0; ui<16; ui++ )
			piCoeff[ui].setLevel( piCoeff[ui].getCoeff() );
	}

}

Void
MbTransformCoeffs::switchLevelCoeffData()
{
	TCoeff sTemp;

	for( B4x4Idx bIdx; bIdx.isLegal(); bIdx++ )
	{
		//TCoeff* piCoeff = m_aaiLevel[bIdx];
		TCoeff* piCoeff = get( bIdx );
		for( UInt ui=0; ui<16; ui++ )
		{
			sTemp = piCoeff[ui].getLevel();
			piCoeff[ui].setLevel( piCoeff[ui].getCoeff() );
			piCoeff[ui].setCoeff( sTemp );
		}
	}

	for( CIdx cIdx; cIdx.isLegal(); cIdx++ )
	{
		//TCoeff* piCoeff = m_aaiLevel[cIdx];
		TCoeff* piCoeff = get( cIdx );
		for( UInt ui=0; ui<16; ui++ )
		{
			sTemp = piCoeff[ui].getLevel();
			piCoeff[ui].setLevel( piCoeff[ui].getCoeff() );
			piCoeff[ui].setCoeff( sTemp );
		}
	}

}



Void MbTransformCoeffs::add( MbTransformCoeffs* pcCoeffs, Bool bLuma, Bool bChroma )
{
  if( bLuma )
  {
	  for( B4x4Idx bIdx; bIdx.isLegal(); bIdx++ )
	  {
		  TCoeff* piCoeff = get( bIdx );
		  TCoeff* piSrcCoeff = pcCoeffs->get( bIdx );

		  for( UInt ui=0; ui<16; ui++ )
      {
        piCoeff[ui]+= piSrcCoeff[ui];
      }
	  }
  }

  if( bChroma )
  {
    for( CIdx cIdx; cIdx.isLegal(); cIdx++ )
    {
      TCoeff* piCoeff = get( cIdx );
      TCoeff* piSrcCoeff = pcCoeffs->get( cIdx );

      for( UInt ui=0; ui<16; ui++ )
      {
        piCoeff[ui] += piSrcCoeff[ui];
      }
    }
  }
}


H264AVC_NAMESPACE_END
