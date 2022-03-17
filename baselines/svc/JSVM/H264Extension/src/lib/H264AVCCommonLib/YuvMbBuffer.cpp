
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/YuvMbBuffer.h"
#include "H264AVCCommonLib/YuvPicBuffer.h"


H264AVC_NAMESPACE_BEGIN


YuvMbBuffer::YuvMbBuffer()
: m_pPelCurrY( NULL )
, m_pPelCurrU( NULL )
, m_pPelCurrV( NULL )
{
  DO_DBG( ::memset( m_aucYuvBuffer, 0 , sizeof(m_aucYuvBuffer) ) );// TMM_INTERLACE
}


YuvMbBuffer::~YuvMbBuffer()
{
}

Void
YuvMbBuffer::setZero()
{
  ::memset( m_aucYuvBuffer, 0 , sizeof(m_aucYuvBuffer) );
}

Void YuvMbBuffer::loadIntraPredictors( const YuvPicBuffer* pcSrcBuffer )
{
  Int y;

  XPel* pSrc = pcSrcBuffer->getMbLumAddr();
  XPel* pDes = getMbLumAddr();

  Int iSrcStride = pcSrcBuffer->getLStride();
  Int iDesStride = getLStride();

  pSrc -= iSrcStride+1;
  pDes -= iDesStride+1;

  memcpy( pDes, pSrc, sizeof(XPel)*21 );
  memcpy( pDes+iDesStride+17, pSrc+21, sizeof(XPel)*4 );

  for( y = 0; y < 16; y++)
  {
    pSrc += iSrcStride;
    pDes += iDesStride;
    *pDes = *pSrc;
  }

  pSrc = pcSrcBuffer->getMbCbAddr();
  pDes = getMbCbAddr();

  iSrcStride = pcSrcBuffer->getCStride();
  iDesStride = getCStride();

  pSrc -= iSrcStride+1;
  pDes -= iDesStride+1;

  memcpy( pDes, pSrc, sizeof(XPel)*9 );

  for( y = 0; y < 8; y++)
  {
    pSrc += iSrcStride;
    pDes += iDesStride;
    *pDes = *pSrc;
  }

  pSrc = pcSrcBuffer->getMbCrAddr();
  pDes = getMbCrAddr();

  pSrc -= iSrcStride+1;
  pDes -= iDesStride+1;

  memcpy( pDes, pSrc, sizeof(XPel)*9 );

  for( y = 0; y < 8; y++)
  {
    pSrc += iSrcStride;
    pDes += iDesStride;
    *pDes = *pSrc;
  }
}


Void YuvMbBuffer::loadBuffer( const YuvPicBuffer* pcSrcBuffer )
{
  Int   y;
  XPel* pSrc;
  XPel* pDes;
  Int   iSrcStride;
  Int   iDesStride;

  pSrc = pcSrcBuffer->getMbLumAddr();
  pDes = getMbLumAddr();
  iDesStride = getLStride();
  iSrcStride = pcSrcBuffer->getLStride();

  for( y = 0; y < 16; y++ )
  {
    memcpy( pDes, pSrc, 16 * sizeof(XPel) );
    pDes += iDesStride;
    pSrc += iSrcStride;
  }

  pSrc = pcSrcBuffer->getMbCbAddr();
  pDes = getMbCbAddr();
  iDesStride = getCStride();
  iSrcStride = pcSrcBuffer->getCStride();

  for( y = 0; y < 8; y++ )
  {
    memcpy( pDes, pSrc, 8 * sizeof(XPel) );
    pDes += iDesStride;
    pSrc += iSrcStride;
  }

  pSrc = pcSrcBuffer->getMbCrAddr();
  pDes = getMbCrAddr();

  for( y = 0; y < 8; y++ )
  {
    memcpy( pDes, pSrc, 8 * sizeof(XPel) );
    pDes += iDesStride;
    pSrc += iSrcStride;
  }
}


Void
YuvMbBuffer::add( const YuvMbBuffer& rcIntYuvMbBuffer )
{
  Int         y, x;
  Int         iStride = getLStride  ();
  const XPel* pSrc    = rcIntYuvMbBuffer.getMbLumAddr();
  XPel*       pDes    = getMbLumAddr();

  for( y = 0; y < 16; y++ )
  {
    for( x = 0; x < 16; x++ )   pDes[x] += pSrc[x];
    pDes += iStride;
    pSrc += iStride;
  }

  iStride = getCStride  ();
  pSrc    = rcIntYuvMbBuffer.getMbCbAddr ();
  pDes    = getMbCbAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8;  x++ )   pDes[x] += pSrc[x];
    pDes += iStride;
    pSrc += iStride;
  }

  pSrc    = rcIntYuvMbBuffer.getMbCrAddr ();
  pDes    = getMbCrAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8;  x++ )   pDes[x] += pSrc[x];
    pDes += iStride;
    pSrc += iStride;
  }
}

Void
YuvMbBuffer::addRes( const YuvMbBuffer& rcIntYuvMbBuffer )
{
  Int         y, x;
  Int         iStride = getLStride  ();
  const XPel* pSrc    = rcIntYuvMbBuffer.getMbLumAddr();
  XPel*       pDes    = getMbLumAddr();

  for( y = 0; y < 16; y++ )
  {
    for( x = 0; x < 16; x++ )
    {
      pDes[x] += pSrc[x];
      pDes[x]  = gClipMinMax( pDes[x], -255, 255 );
    }
    pDes += iStride;
    pSrc += iStride;
  }

  iStride = getCStride  ();
  pSrc    = rcIntYuvMbBuffer.getMbCbAddr ();
  pDes    = getMbCbAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8;  x++ )
    {
      pDes[x] += pSrc[x];
      pDes[x]  = gClipMinMax( pDes[x], -255, 255 );
    }
    pDes += iStride;
    pSrc += iStride;
  }

  pSrc    = rcIntYuvMbBuffer.getMbCrAddr ();
  pDes    = getMbCrAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8;  x++ )
    {
      pDes[x] += pSrc[x];
      pDes[x]  = gClipMinMax( pDes[x], -255, 255 );
    }
    pDes += iStride;
    pSrc += iStride;
  }
}


Void
YuvMbBuffer::addClip( const YuvMbBuffer& rcIntYuvMbBuffer )
{
  Int         y, x;
  Int         iStride = getLStride  ();
  const XPel* pSrc    = rcIntYuvMbBuffer.getMbLumAddr();
  XPel*       pDes    = getMbLumAddr();

  for( y = 0; y < 16; y++ )
  {
    for( x = 0; x < 16; x++ )
    {
      pDes[x] += pSrc[x];
      pDes[x]  = gClip( pDes[x] );
    }
    pDes += iStride;
    pSrc += iStride;
  }

  iStride = getCStride  ();
  pSrc    = rcIntYuvMbBuffer.getMbCbAddr ();
  pDes    = getMbCbAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8;  x++ )
    {
      pDes[x] += pSrc[x];
      pDes[x]  = gClip( pDes[x] );
    }
    pDes += iStride;
    pSrc += iStride;
  }

  pSrc    = rcIntYuvMbBuffer.getMbCrAddr ();
  pDes    = getMbCrAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8;  x++ ) 
    {
      pDes[x] += pSrc[x];
      pDes[x]  = gClip( pDes[x] );
    }
    pDes += iStride;
    pSrc += iStride;
  }
}


Void
YuvMbBuffer::clip()
{
  Int   y, x;
  Int   iStride = getLStride  ();
  XPel* pDes    = getMbLumAddr();

  for( y = 0; y < 16; y++ )
  {
    for( x = 0; x < 16; x++ )   pDes[x] = gClip( pDes[x] );
    pDes += iStride;
  }

  iStride = getCStride  ();
  pDes    = getMbCbAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8;  x++ )   pDes[x] = gClip( pDes[x] );
    pDes += iStride;
  }

  pDes    = getMbCrAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8;  x++ )   pDes[x] = gClip( pDes[x] );
    pDes += iStride;
  }
}


Bool
YuvMbBuffer::isZero()
{
  Int   x, y;
  XPel* pPel    = getMbLumAddr();
  Int   iStride = getLStride  ();

  for( y = 0; y < 16; y++ )
  {
    for( x = 0; x < 16; x++ )
    {
      if( pPel[x] )
      {
        return false;
      }
    }
    pPel += iStride;
  }

  pPel    = getMbCbAddr ();
  iStride = getCStride  ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8; x++ )
    {
      if( pPel[x] )
      {
        return false;
      }
    }
    pPel += iStride;
  }

  pPel    = getMbCrAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8; x++ )
    {
      if( pPel[x] )
      {
        return false;
      }
    }
    pPel += iStride;
  }

  return true;
}

Void
YuvMbBuffer::subtract( const YuvMbBuffer& rcIntYuvMbBuffer )
{
  Int         y, x;
  Int         iStride = getLStride  ();
  const XPel* pSrc    = rcIntYuvMbBuffer.getMbLumAddr();
  XPel*       pDes    = getMbLumAddr();

  for( y = 0; y < 16; y++ )
  {
    for( x = 0; x < 16; x++ )
      pDes[x] -= pSrc[x];
    pDes += iStride;
    pSrc += iStride;
  }

  iStride = getCStride  ();
  pSrc    = rcIntYuvMbBuffer.getMbCbAddr ();
  pDes    = getMbCbAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8;  x++ )
      pDes[x] -= pSrc[x];
    pDes += iStride;
    pSrc += iStride;
  }

  pSrc    = rcIntYuvMbBuffer.getMbCrAddr ();
  pDes    = getMbCrAddr ();

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8;  x++ )
      pDes[x] -= pSrc[x];
    pDes += iStride;
    pSrc += iStride;
  }
}




Void YuvMbBuffer::loadChroma( const YuvMbBuffer& rcSrcBuffer )
{
  const Int   iStride = getCStride();
  XPel*       pDes    = getMbCbAddr();
  const XPel* pSrc    = rcSrcBuffer.getMbCbAddr();
  Int         y;

  for( y = 0; y < 8; y++ )
  {
    memcpy( pDes, pSrc, 8 * sizeof(XPel) );
    pDes += iStride;
    pSrc += iStride;
  }

  pDes = getMbCrAddr();
  pSrc = rcSrcBuffer.getMbCrAddr();

  for( y = 0; y < 8; y++ )
  {
    memcpy( pDes, pSrc, 8 * sizeof(XPel) );
    pDes += iStride;
    pSrc += iStride;
  }
}


Void YuvMbBuffer::loadLuma( const YuvMbBuffer& rcSrcBuffer, LumaIdx c4x4Idx )
{
  const Int   iStride = getLStride();
  XPel*       pDes    = getYBlk( c4x4Idx );
  const XPel* pSrc    = rcSrcBuffer.getYBlk( c4x4Idx );

  for( Int y = 0; y < 4; y++ )
  {
    memcpy( pDes, pSrc, 4 * sizeof(XPel) );
    pDes += iStride;
    pSrc += iStride;
  }
}


Void YuvMbBuffer::loadLuma( const YuvMbBuffer& rcSrcBuffer, B8x8Idx c8x8Idx )
{
  const Int   iStride = getLStride();
  XPel*       pDes    = getYBlk( c8x8Idx );
  const XPel* pSrc    = rcSrcBuffer.getYBlk( c8x8Idx );

  for( Int y = 0; y < 8; y++ )
  {
    memcpy( pDes, pSrc, 8 * sizeof(XPel) );
    pDes += iStride;
    pSrc += iStride;
  }
}


Void YuvMbBuffer::loadLuma( const YuvMbBuffer& rcSrcBuffer )
{
  const Int   iStride = getLStride();
  XPel*       pDes    = getMbLumAddr();
  const XPel* pSrc    = rcSrcBuffer.getMbLumAddr();

  for( Int y = 0; y < 16; y++ )
  {
    memcpy( pDes, pSrc, 16 * sizeof(XPel) );
    pDes += iStride;
    pSrc += iStride;
  }
}


Void YuvMbBuffer::setAllSamplesToZero()
{
  Int   y;
  XPel* pPel    = getMbLumAddr();
  Int   iStride = getLStride();

  for( y = 0; y < 16; y++ )
  {
    ::memset( pPel, 0x00, 16 * sizeof(XPel) );
    pPel += iStride;
  }

  pPel    = getMbCbAddr();
  iStride = getCStride();

  for( y = 0; y < 8; y++ )
  {
    ::memset( pPel, 0x00, 8 * sizeof(XPel) );
    pPel += iStride;
  }

  pPel    = getMbCrAddr();

  for( y = 0; y < 8; y++ )
  {
    ::memset( pPel, 0x00, 8 * sizeof(XPel) );
    pPel += iStride;
  }
}


Void YuvMbBufferExtension::loadSurrounding( const YuvPicBuffer* pcSrcBuffer, Int iMbXOffset, Int iMbYOffset )
{
  Int x, y;
  Int iDesStride = getLStride();
  Int iSrcStride = pcSrcBuffer->getLStride();
  XPel*     pSrc = pcSrcBuffer->getMbLumAddr() + iMbXOffset * 16 + iMbYOffset * 16 * iSrcStride;
  XPel*     pDes = getMbLumAddr();

  for( x = 0; x < 18; x++ )
  {
    pDes[x-iDesStride-1] = pSrc[x-iSrcStride-1];
  }
  for( y = 0; y < 16; y++ )
  {
    pDes[-1] = pSrc[-1];
    pDes[16] = pSrc[16];
    pDes += iDesStride;
    pSrc += iSrcStride;
  }
  for( x = 0; x < 18; x++ )
  {
    pDes[x-1] = pSrc[x-1];
  }

  iDesStride  = getCStride();
  iSrcStride  = pcSrcBuffer->getCStride();
  pSrc        = pcSrcBuffer->getMbCbAddr() + iMbXOffset * 8 + iMbYOffset * 8 * iSrcStride;
  pDes        = getMbCbAddr();

  for( x = 0; x < 10; x++ )
  {
    pDes[x-iDesStride-1] = pSrc[x-iSrcStride-1];
  }
  for( y = 0; y < 8; y++ )
  {
    pDes[-1] = pSrc[-1];
    pDes[8]  = pSrc[8];
    pDes += iDesStride;
    pSrc += iSrcStride;
  }
  for( x = 0; x < 10; x++ )
  {
    pDes[x-1] = pSrc[x-1];
  }

  pSrc        = pcSrcBuffer->getMbCrAddr() + iMbXOffset * 8 + iMbYOffset * 8 * iSrcStride;
  pDes        = getMbCrAddr();

  for( x = 0; x < 10; x++ )
  {
    pDes[x-iDesStride-1] = pSrc[x-iSrcStride-1];
  }
  for( y = 0; y < 8; y++ )
  {
    pDes[-1] = pSrc[-1];
    pDes[8]  = pSrc[8];
    pDes += iDesStride;
    pSrc += iSrcStride;
  }
  for( x = 0; x < 10; x++ )
  {
    pDes[x-1] = pSrc[x-1];
  }
}


//TMM_INTERLACE {
Void YuvMbBufferExtension::loadSurrounding_MbAff( const YuvPicBuffer* pcSrcBuffer, UInt uiMask, Int iMbXOffset, Int iMbYOffset )
{
  Int   x, y;
  Bool  bTopIntra     = ( ( uiMask & 0x020 ) != 0 );
  Bool  bBotIntra     = ( ( uiMask & 0x040 ) != 0 );
  Int   iYSizeLuma    = ( bTopIntra || bBotIntra ? 8 : 16 );
  Int   iYSizeChroma  = iYSizeLuma >> 1;

  Int   iSrcStride    = pcSrcBuffer->getLStride();
  Int   iDesStride    = getLStride();
  XPel* pSrc          = pcSrcBuffer->getMbLumAddr() + ( bTopIntra ? iYSizeLuma * iSrcStride : 0 ) + iMbXOffset * 16 + iMbYOffset * 16 * iSrcStride;
  XPel* pDes          = getMbLumAddr()              + ( bTopIntra ? iYSizeLuma * iDesStride : 0 );

  for( x = 0; x < 18; x++ )
  {
    pDes[x-iDesStride-1] = pSrc[x-iSrcStride-1];
  }
  for( y = 0; y < iYSizeLuma; y++ )
  {
    pDes[-1] = pSrc[-1];
    pDes[16] = pSrc[16];
    pDes += iDesStride;
    pSrc += iSrcStride;
  }
  for( x = 0; x < 18; x++ )
  {
    pDes[x-1] = pSrc[x-1];
  }

  iSrcStride  = pcSrcBuffer->getCStride();
  iDesStride  = getCStride();
  pSrc        = pcSrcBuffer->getMbCbAddr() + ( bTopIntra ? iYSizeChroma * iSrcStride : 0 ) + iMbXOffset * 8 + iMbYOffset * 8 * iSrcStride;
  pDes        = getMbCbAddr()              + ( bTopIntra ? iYSizeChroma * iDesStride : 0 );

  for( x = 0; x < 10; x++ )
  {
    pDes[x-iDesStride-1] = pSrc[x-iSrcStride-1];
  }
  for( y = 0; y < iYSizeChroma; y++ )
  {
    pDes[-1] = pSrc[-1];
    pDes[ 8] = pSrc[ 8];
    pDes += iDesStride;
    pSrc += iSrcStride;
  }
  for( x = 0; x < 10; x++ )
  {
    pDes[x-1] = pSrc[x-1];
  }

  pSrc  = pcSrcBuffer->getMbCrAddr() + ( bTopIntra ? iYSizeChroma * iSrcStride : 0 ) + iMbXOffset * 8 + iMbYOffset * 8 * iSrcStride;
  pDes  = getMbCrAddr()              + ( bTopIntra ? iYSizeChroma * iDesStride : 0 );

  for( x = 0; x < 10; x++ )
  {
    pDes[x-iDesStride-1] = pSrc[x-iSrcStride-1];
  }
  for( y = 0; y < iYSizeChroma; y++ )
  {
    pDes[-1] = pSrc[-1];
    pDes[ 8] = pSrc[ 8];
    pDes += iDesStride;
    pSrc += iSrcStride;
  }
 for( x = 0; x < 10; x++ )
  {
    pDes[x-1] = pSrc[x-1];
  }
}

//TMM_INTERLACE }




Void YuvMbBufferExtension::xMerge( Int xDir, Int yDir, Int iSize, XPel* puc, Int iStride, Bool bCornerMbPresent, Bool bHalfYSize )
{
  XPel  pPelH[9];
  XPel  pPelV[9];
  Int   iXSize  = iSize;
  Int   iYSize  = ( bHalfYSize ? iSize >> 1 : iSize );
  Int   iAdd    = 1;
  Int   x, y, xo;

  if( yDir < 0 )
  {
    puc    +=  iStride * ( iSize - 1 );
    iStride = -iStride;
  }
  if( xDir < 0 )
  {
    puc    += ( iSize - 1);
    iAdd    = -1;
  }

  for( x = 0; x <= iXSize; x++ )
  {
    pPelH[x] = puc[(x-1)*iAdd - iStride];
  }
  for( y = 0; y <= iYSize; y++ )
  {
    pPelV[y] = puc[(y-1)*iStride - iAdd];
  }

  if( ! bCornerMbPresent )
  {
    pPelV[0] = pPelH[0] = ( pPelH[1] + pPelV[1] + 1 ) >> 1;
  }

  for( y = 0; y < iYSize; y++, puc += iStride )
  {
    for( xo = 0, x = 0; x < iXSize; x++, xo += iAdd )
    {
      const Int iOffset = x-y;

      if( iOffset > 0 )
      {
        puc[xo] = ( pPelH[ iOffset-1] + 2*pPelH[ iOffset] + pPelH[ iOffset+1] + 2 ) >> 2;
      }
      else if( iOffset < 0 )
      {
        puc[xo] = ( pPelV[-iOffset-1] + 2*pPelV[-iOffset] + pPelV[-iOffset+1] + 2 ) >> 2;
      }
      else
      {
        puc[xo] = ( pPelH[1] + 2*pPelV[0] + pPelV[1] + 2 ) >> 2;
      }
    }
  }
}

Void YuvMbBufferExtension::mergeFromLeftAbove ( LumaIdx cIdx, Bool bCornerMbPresent, Bool bHalfYSize )
{
  xMerge(  1,  1, 8, getYBlk( cIdx ), getLStride(), bCornerMbPresent, bHalfYSize );
  xMerge(  1,  1, 4, getUBlk( cIdx ), getCStride(), bCornerMbPresent, bHalfYSize );
  xMerge(  1,  1, 4, getVBlk( cIdx ), getCStride(), bCornerMbPresent, bHalfYSize );
}

Void YuvMbBufferExtension::mergeFromRightBelow( LumaIdx cIdx, Bool bCornerMbPresent, Bool bHalfYSize )
{
  xMerge( -1, -1, 8, getYBlk( cIdx ), getLStride(), bCornerMbPresent, bHalfYSize );
  xMerge( -1, -1, 4, getUBlk( cIdx ), getCStride(), bCornerMbPresent, bHalfYSize );
  xMerge( -1, -1, 4, getVBlk( cIdx ), getCStride(), bCornerMbPresent, bHalfYSize );
}

Void YuvMbBufferExtension::mergeFromRightAbove( LumaIdx cIdx, Bool bCornerMbPresent, Bool bHalfYSize )
{
  xMerge( -1,  1, 8, getYBlk( cIdx ), getLStride(), bCornerMbPresent, bHalfYSize );
  xMerge( -1,  1, 4, getUBlk( cIdx ), getCStride(), bCornerMbPresent, bHalfYSize );
  xMerge( -1,  1, 4, getVBlk( cIdx ), getCStride(), bCornerMbPresent, bHalfYSize );
}

Void YuvMbBufferExtension::mergeFromLeftBelow ( LumaIdx cIdx, Bool bCornerMbPresent, Bool bHalfYSize )
{
  xMerge(  1, -1, 8, getYBlk( cIdx ), getLStride(), bCornerMbPresent, bHalfYSize );
  xMerge(  1, -1, 4, getUBlk( cIdx ), getCStride(), bCornerMbPresent, bHalfYSize );
  xMerge(  1, -1, 4, getVBlk( cIdx ), getCStride(), bCornerMbPresent, bHalfYSize );
}

Void YuvMbBufferExtension::copyFromBelow      ( LumaIdx cIdx, Bool bHalfYSize )
{
  Int   iYSize  = ( bHalfYSize ? 4 : 8 );
  XPel* pPel    = getYBlk( cIdx );
  Int   iStride = getLStride();
  Int   y;

  pPel += 8*iStride;
  for( y = 0; y < iYSize; y++ )
  {
    memcpy( pPel-iStride, pPel, 8 * sizeof(XPel) );
    pPel -= iStride;
  }

  pPel    = getUBlk( cIdx );
  iStride = getCStride();
  iYSize  = iYSize >> 1;

  pPel += 4*iStride;
  for( y = 0; y < iYSize; y++ )
  {
    memcpy( pPel-iStride, pPel, 4 * sizeof(XPel) );
    pPel -= iStride;
  }

  pPel    = getVBlk( cIdx );

  pPel += 4*iStride;
  for( y = 0; y < iYSize; y++ )
  {
    memcpy( pPel-iStride, pPel, 4 * sizeof(XPel) );
    pPel -= iStride;
  }
}

Void YuvMbBufferExtension::copyFromLeft       ( LumaIdx cIdx )
{
  XPel* pPel    = getYBlk( cIdx );
  Int   iStride = getLStride();
  Int   x, y;

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8; x++ )
    {
      pPel[x] = pPel[-1];
    }
    pPel += iStride;
  }

  pPel    = getUBlk( cIdx );
  iStride = getCStride();

  for( y = 0; y < 4; y++ )
  {
    for( x = 0; x < 4; x++ )
    {
      pPel[x] = pPel[-1];
    }
    pPel += iStride;
  }

  pPel    = getVBlk( cIdx );

  for( y = 0; y < 4; y++ )
  {
    for( x = 0; x < 4; x++ )
    {
      pPel[x] = pPel[-1];
    }
    pPel += iStride;
  }
}

Void YuvMbBufferExtension::copyFromAbove      ( LumaIdx cIdx, Bool bHalfYSize )
{
  Int   iYSize  = ( bHalfYSize ? 4 : 8 );
  XPel* pPel    = getYBlk( cIdx );
  Int   iStride = getLStride();
  Int   y;

  for( y = 0; y < iYSize; y++ )
  {
    memcpy( pPel, pPel-iStride, 8 * sizeof(XPel) );
    pPel += iStride;
  }

  pPel    = getUBlk( cIdx );
  iStride = getCStride();
  iYSize  = iYSize >> 1;

  for( y = 0; y < iYSize; y++ )
  {
    memcpy( pPel, pPel-iStride, 4 * sizeof(XPel) );
    pPel += iStride;
  }

  pPel    = getVBlk( cIdx );

  for( y = 0; y < iYSize; y++ )
  {
    memcpy( pPel, pPel-iStride, 4 * sizeof(XPel) );
    pPel += iStride;
  }
}

Void YuvMbBufferExtension::copyFromRight      ( LumaIdx cIdx )
{
  XPel* pPel    = getYBlk( cIdx );
  Int   iStride = getLStride();
  Int   y,x;

  for( y = 0; y < 8; y++ )
  {
    for( x = 0; x < 8; x++ )
    {
      pPel[x] = pPel[8];
    }
    pPel += iStride;
  }

  pPel    = getUBlk( cIdx );
  iStride = getCStride();

  for( y = 0; y < 4; y++ )
  {
    for( x = 0; x < 4; x++ )
    {
      pPel[x] = pPel[4];
    }
    pPel += iStride;
  }

  pPel    = getVBlk( cIdx );

  for( y = 0; y < 4; y++ )
  {
    for( x = 0; x < 4; x++ )
    {
      pPel[x] = pPel[4];
    }
    pPel += iStride;
  }
}

Void YuvMbBufferExtension::xFill( LumaIdx cIdx, XPel cY, XPel cU, XPel cV, Bool bHalfYSize, Bool bLowerHalf )
{
  AOT( !bHalfYSize && bLowerHalf );
  Int   iYSize  = ( bHalfYSize ? 4 : 8 );
  Int   iStride = getLStride();
  XPel* pPel    = getYBlk( cIdx ) + ( bLowerHalf ? 4*iStride : 0 );
  Int   x, y;

  for( y = 0; y < iYSize; y++ )
  {
    for( x = 0; x < 8; x++ )
    {
      pPel[x] = cY;
    }
    pPel += iStride;
  }

  iYSize  = ( bHalfYSize ? 2 : 4 );
  iStride = getCStride();
  pPel    = getUBlk( cIdx ) + ( bLowerHalf ? 2*iStride : 0 );

  for( y = 0; y < iYSize; y++ )
  {
    for( x = 0; x < 4; x++ )
    {
      pPel[x] = cU;
    }
    pPel += iStride;
  }

  pPel    = getVBlk( cIdx ) + ( bLowerHalf ? 2*iStride : 0 );

  for( y = 0; y < iYSize; y++ )
  {
    for( x = 0; x < 4; x++ )
    {
      pPel[x] = cV;
    }
    pPel += iStride;
  }
}

Void YuvMbBufferExtension::copyFromLeftAbove  ( LumaIdx cIdx, Bool bHalfYSize )
{
  XPel cY = *(getYBlk( cIdx ) - 1 - getLStride());
  XPel cU = *(getUBlk( cIdx ) - 1 - getCStride());
  XPel cV = *(getVBlk( cIdx ) - 1 - getCStride());

  xFill( cIdx, cY, cU, cV, bHalfYSize, false );
}

Void YuvMbBufferExtension::copyFromRightAbove ( LumaIdx cIdx, Bool bHalfYSize )
{
  XPel cY = *(getYBlk( cIdx ) + 8 - getLStride());
  XPel cU = *(getUBlk( cIdx ) + 4 - getCStride());
  XPel cV = *(getVBlk( cIdx ) + 4 - getCStride());

  xFill( cIdx, cY, cU, cV, bHalfYSize, false );
}

Void YuvMbBufferExtension::copyFromLeftBelow  ( LumaIdx cIdx, Bool bHalfYSize )
{
  XPel cY = *(getYBlk( cIdx ) - 1 + 8 * getLStride());
  XPel cU = *(getUBlk( cIdx ) - 1 + 4 * getCStride());
  XPel cV = *(getVBlk( cIdx ) - 1 + 4 * getCStride());

  xFill( cIdx, cY, cU, cV, bHalfYSize, bHalfYSize );
}

Void YuvMbBufferExtension::copyFromRightBelow ( LumaIdx cIdx, Bool bHalfYSize )
{
  XPel cY = *(getYBlk( cIdx ) + 8 + 8 * getLStride());
  XPel cU = *(getUBlk( cIdx ) + 4 + 4 * getCStride());
  XPel cV = *(getVBlk( cIdx ) + 4 + 4 * getCStride());

  xFill( cIdx, cY, cU, cV, bHalfYSize, bHalfYSize );
}



H264AVC_NAMESPACE_END

