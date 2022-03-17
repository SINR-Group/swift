
#include "H264AVCCommonLib.h"

#include "H264AVCCommonLib/SampleWeighting.h"


H264AVC_NAMESPACE_BEGIN


SampleWeighting::SampleWeighting()
: m_uiLumaLogWeightDenom    ( 5 )
, m_uiChromaLogWeightDenom  ( 5 )
, m_bExplicit               ( false )
, m_bWeightedPredDisableP   ( true )
, m_bWeightedPredDisableB   ( true )
{
  m_afpMixSampleFunc[0] = NULL;
  m_afpMixSampleFunc[1] = NULL;
  m_afpMixSampleFunc[2] = NULL;
  m_afpMixSampleFunc[3] = NULL;
  m_afpMixSampleFunc[4] = NULL;

  m_afpXMixSampleFunc[0] = NULL;
  m_afpXMixSampleFunc[1] = NULL;
  m_afpXMixSampleFunc[2] = NULL;
  m_afpXMixSampleFunc[3] = NULL;
  m_afpXMixSampleFunc[4] = NULL;
}


ErrVal SampleWeighting::create( SampleWeighting*& rpcSampleWeighting )
{
  rpcSampleWeighting = new SampleWeighting;

  ROT( NULL == rpcSampleWeighting );

  return Err::m_nOK;
}


ErrVal SampleWeighting::destroy()
{
  delete this;

  return Err::m_nOK;
}

ErrVal SampleWeighting::init()
{
  m_afpMixSampleFunc[1] = SampleWeighting::xMixB4x;
  m_afpMixSampleFunc[2] = SampleWeighting::xMixB8x;
  m_afpMixSampleFunc[4] = SampleWeighting::xMixB16x;

  m_afpXMixSampleFunc[1] = SampleWeighting::xXMixB4x;
  m_afpXMixSampleFunc[2] = SampleWeighting::xXMixB8x;
  m_afpXMixSampleFunc[4] = SampleWeighting::xXMixB16x;
  return Err::m_nOK;
}


ErrVal SampleWeighting::uninit()
{
  m_afpMixSampleFunc[0] = NULL;
  m_afpMixSampleFunc[1] = NULL;
  m_afpMixSampleFunc[2] = NULL;
  m_afpMixSampleFunc[3] = NULL;
  m_afpMixSampleFunc[4] = NULL;

  m_afpXMixSampleFunc[0] = NULL;
  m_afpXMixSampleFunc[1] = NULL;
  m_afpXMixSampleFunc[2] = NULL;
  m_afpXMixSampleFunc[3] = NULL;
  m_afpXMixSampleFunc[4] = NULL;

  return Err::m_nOK;
}


ErrVal
SampleWeighting::initSlice( const SliceHeader& rcSliceHeader )
{
  if( rcSliceHeader.isIntraSlice() )
  {
    m_bWeightedPredDisableP = true;
    m_bWeightedPredDisableB = true;
    m_bExplicit             = false;
    return Err::m_nOK;
  }
  if( rcSliceHeader.isPSlice() )
  {
    m_bExplicit             = rcSliceHeader.getPPS().getWeightedPredFlag();
    m_bWeightedPredDisableP = ! m_bExplicit;
    m_bWeightedPredDisableB = true;
    if( m_bExplicit )
    {
      m_uiLumaLogWeightDenom   = rcSliceHeader.getLumaLog2WeightDenom();
      m_uiChromaLogWeightDenom = rcSliceHeader.getChromaLog2WeightDenom();
    }
    return Err::m_nOK;
  }
  if( rcSliceHeader.isBSlice() )
  {
    switch( rcSliceHeader.getPPS().getWeightedBiPredIdc() )
    {
    case 0:
      {
        m_bExplicit               = false;
        m_bWeightedPredDisableP   = true;
        m_bWeightedPredDisableB   = true;
        m_uiLumaLogWeightDenom    = 0;
        m_uiChromaLogWeightDenom  = 0;
      }
      break;
    case 1:
      {
        m_bExplicit               = true;
        m_bWeightedPredDisableP   = false;
        m_bWeightedPredDisableB   = false;
        m_uiLumaLogWeightDenom    = rcSliceHeader.getLumaLog2WeightDenom();
        m_uiChromaLogWeightDenom  = rcSliceHeader.getChromaLog2WeightDenom();
      }
      break;
    case 2:
      {
        m_bExplicit               = false;
        m_bWeightedPredDisableP   = true;
        m_bWeightedPredDisableB   = false;
        m_uiLumaLogWeightDenom    = 5;
        m_uiChromaLogWeightDenom  = 5;
      }
      break;
    default:
      {
        AF();
      }
      break;
    }
  }
  return Err::m_nOK;
}


Void SampleWeighting::getTargetBuffers( YuvMbBuffer* apcTarBuffer[2], YuvMbBuffer* pcRecBuffer, const PredWeight* pcPW0, const PredWeight* pcPW1 )
{
  if( pcPW0 != 0 && pcPW1 != 0 )
  {
    apcTarBuffer[0] = pcRecBuffer;
    apcTarBuffer[1] = &m_cIntYuvBiBuffer;
  }
  else
  {
    apcTarBuffer[0] = pcRecBuffer;
    apcTarBuffer[1] = pcRecBuffer;
  }
}



Void SampleWeighting::weightLumaSamples( YuvMbBuffer* pcRecBuffer, Int iSizeX, Int iSizeY, LumaIdx cIdx, const PredWeight* pcPW0, const PredWeight* pcPW1 )
{
  AOT_DBG( iSizeY < 8 );
  AOT_DBG( iSizeX < 8 );

  if( pcPW0 != NULL && pcPW1 != NULL )
  {
    // bidirectional prediction
    if( m_bWeightedPredDisableB )
    {
      xMixB( pcRecBuffer     ->getYBlk( cIdx ),  pcRecBuffer     ->getLStride(),
             m_cIntYuvBiBuffer.getYBlk( cIdx ),  m_cIntYuvBiBuffer.getLStride(),
             iSizeY, iSizeX );
    }
    else
  {
      xMixBWeight( pcRecBuffer     ->getYBlk( cIdx ), pcRecBuffer     ->getLStride(),
                   m_cIntYuvBiBuffer.getYBlk( cIdx ), m_cIntYuvBiBuffer.getLStride(),
                   iSizeY, iSizeX,
                   pcPW0->getLumaWeight(),
                   pcPW1->getLumaWeight(),
                   pcPW0->getLumaOffset() + pcPW1->getLumaOffset(),
                   m_uiLumaLogWeightDenom );
  }
}
  else
  {
    ROTVS( m_bWeightedPredDisableP );

    // unidirectionl prediction
    const PredWeight* pcPredWeight = (pcPW0 != NULL) ? pcPW0 : pcPW1;
    AOT_DBG( NULL == pcPredWeight );

    if( pcPredWeight->getLumaWeightFlag() )
    {
      xWeight( pcRecBuffer->getYBlk( cIdx ), pcRecBuffer->getLStride(),
               iSizeY, iSizeX,
               pcPredWeight->getLumaWeight(),
               pcPredWeight->getLumaOffset(),
               m_uiLumaLogWeightDenom );
    }
  }
}



Void SampleWeighting::weightChromaSamples( YuvMbBuffer* pcRecBuffer, Int iSizeX, Int iSizeY, LumaIdx cIdx, const PredWeight* pcPW0, const PredWeight* pcPW1 )
{
  AOT_DBG( iSizeY < 4 );
  AOT_DBG( iSizeX < 4 );

  if( pcPW0 != NULL && pcPW1 != NULL )
  {
    // bidirectional prediction
    if( m_bWeightedPredDisableB )
    {
      xMixB( pcRecBuffer     ->getUBlk( cIdx ), pcRecBuffer     ->getCStride(),
             m_cIntYuvBiBuffer.getUBlk( cIdx ), m_cIntYuvBiBuffer.getCStride(),
             iSizeY, iSizeX );
      xMixB( pcRecBuffer     ->getVBlk( cIdx ), pcRecBuffer     ->getCStride(),
             m_cIntYuvBiBuffer.getVBlk( cIdx ), m_cIntYuvBiBuffer.getCStride(),
             iSizeY, iSizeX );
  }
    else
    {
      xMixBWeight( pcRecBuffer     ->getUBlk( cIdx ), pcRecBuffer     ->getCStride(),
                   m_cIntYuvBiBuffer.getUBlk( cIdx ), m_cIntYuvBiBuffer.getCStride(),
                   iSizeY, iSizeX,
                   pcPW0->getChromaCbWeight(),
                   pcPW1->getChromaCbWeight(),
                   pcPW0->getChromaCbOffset() + pcPW1->getChromaCbOffset(),
                   m_uiChromaLogWeightDenom );
      xMixBWeight( pcRecBuffer     ->getVBlk( cIdx ), pcRecBuffer     ->getCStride(),
                   m_cIntYuvBiBuffer.getVBlk( cIdx ), m_cIntYuvBiBuffer.getCStride(),
                   iSizeY, iSizeX,
                   pcPW0->getChromaCrWeight(),
                   pcPW1->getChromaCrWeight(),
                   pcPW0->getChromaCrOffset() + pcPW1->getChromaCrOffset(),
                   m_uiChromaLogWeightDenom );
}
  }
  else
  {
    ROTVS( m_bWeightedPredDisableP );

    // unidirectionl prediction
    const PredWeight* pcPredWeight = (pcPW0 != NULL) ? pcPW0 : pcPW1;
    AOT_DBG( NULL == pcPredWeight );

    if( pcPredWeight->getChromaWeightFlag() )
    {
      xWeight( pcRecBuffer->getUBlk( cIdx ), pcRecBuffer->getCStride(),
               iSizeY, iSizeX,
               pcPredWeight->getChromaCbWeight(),
               pcPredWeight->getChromaCbOffset(),
               m_uiChromaLogWeightDenom );
      xWeight( pcRecBuffer->getVBlk( cIdx ), pcRecBuffer->getCStride(),
               iSizeY, iSizeX,
               pcPredWeight->getChromaCrWeight(),
               pcPredWeight->getChromaCrOffset(),
               m_uiChromaLogWeightDenom );
    }
  }
}




Void SampleWeighting::inverseLumaSamples( YuvMbBuffer* pcDesBuffer,
                                          YuvMbBuffer* pcOrgBuffer,
                                          YuvMbBuffer* pcFixBuffer,
                                          Int             iYSize,
                                          Int             iXSize )
{
  XPel* pFix         = pcFixBuffer->getLumBlk();
  XPel* pOrg         = pcOrgBuffer->getLumBlk();
  XPel* pDes         = pcDesBuffer->getLumBlk();
  const Int iStride  = pcDesBuffer->getLStride();

  Int iLine = 0;
  for( Int y = 0; y < iYSize; y++)
  {
    for( Int x = 0; x < iXSize; x++)
    {
      pDes[x+iLine] = gClip((Int)(2*pOrg[x+iLine] - pFix[x+iLine])) ;
    }
    iLine += iStride;
  }
}



Void SampleWeighting::xMixB16x( Pel* pucDest, Int iDestStride, Pel* pucSrc, Int iSrcStride, Int iSizeY )
  {
  for( Int y = 0; y < iSizeY; y++)
    {
    for( Int x = 0; x < 16; x++)
      {
      pucDest[x] = (pucDest[x] + pucSrc[x] + 1) >> 1;
      }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
    }
  }

Void SampleWeighting::xMixB8x( Pel* pucDest, Int iDestStride, Pel* pucSrc, Int iSrcStride, Int iSizeY)
    {
  for( Int y = 0; y < iSizeY; y++)
      {
    for( Int x = 0; x < 8; x++)
    {
      pucDest[x] = (pucDest[x] + pucSrc[x] + 1) >> 1;
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}


Void SampleWeighting::xMixB4x( Pel* pucDest, Int iDestStride, Pel* pucSrc, Int iSrcStride, Int iSizeY )
{
  for( Int y = 0; y < iSizeY; y++)
  {
    for( Int x = 0; x < 4; x++)
    {
      pucDest[x] = (pucDest[x] + pucSrc[x] + 1) >> 1;
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}


Void SampleWeighting::xWeight( Pel* pucDest, Int iDestStride, Int iSizeY, Int iSizeX, Int iWeight, Int iOffset, UInt uiDenom )
{
  Int iAdd = ((1+iOffset*2)<<uiDenom)>>1;

  AOT_DBG( iWeight >  128 );
  AOT_DBG( iWeight < -128 );

  for( Int y = 0; y < iSizeY; y++)
  {
    for( Int x = 0; x < iSizeX; x++)
    {
      Int iTemp  = (( iWeight * pucDest[x] + iAdd) >> uiDenom);
      pucDest[x] = gClip( iTemp );
    }
    pucDest += iDestStride;
  }
}


Void SampleWeighting::xMixBWeight( Pel* pucDest, Int iDestStride, Pel* pucSrc, Int iSrcStride, Int iSizeY, Int iSizeX, Int iWD, Int iWS, Int iOffset, UInt uiDenom )
{
  Int iAdd = (1<<uiDenom);

  AOT_DBG( (iWD + iWS) > ((uiDenom == 7) ? 127 : 128));
  AOT_DBG( iWD + iWS < -128 );

  uiDenom++;
  iOffset = (iOffset+1) >> 1;
  for( Int y = 0; y < iSizeY; y++)
  {
    for( Int x = 0; x < iSizeX; x++)
    {
      Int iTemp = (( iWD * pucDest[x] + iWS * pucSrc[x] + iAdd) >> uiDenom) + iOffset;
      pucDest[x] = gClip( iTemp );
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}



__inline Void SampleWeighting::xMixB(Pel* pucDest, Int iDestStride, Pel* pucSrc, Int iSrcStride, Int iSizeY, Int iSizeX)
{
  m_afpMixSampleFunc[iSizeX>>2]( pucDest, iDestStride, pucSrc, iSrcStride, iSizeY );
}




Void SampleWeighting::xXMixB16x( XPel* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Int iSizeY )
{
  for( Int y = 0; y < iSizeY; y++)
  {
    for( Int x = 0; x < 16; x++)
    {
      pucDest[x] = (pucDest[x] + pucSrc[x] + 1) >> 1;
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}

Void SampleWeighting::xXMixB8x( XPel* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Int iSizeY)
{
  for( Int y = 0; y < iSizeY; y++)
  {
    for( Int x = 0; x < 8; x++)
    {
      pucDest[x] = (pucDest[x] + pucSrc[x] + 1) >> 1;
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}


Void SampleWeighting::xXMixB4x( XPel* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Int iSizeY )
{
  for( Int y = 0; y < iSizeY; y++)
  {
    for( Int x = 0; x < 4; x++)
    {
      pucDest[x] = (pucDest[x] + pucSrc[x] + 1) >> 1;
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}

Void SampleWeighting::xWeight( XPel* pucDest, Int iDestStride, Int iSizeY, Int iSizeX, Int iWeight, Int iOffset, UInt uiDenom )
{
  Int iAdd = ((1+iOffset*2)<<uiDenom)>>1;

  AOT_DBG( iWeight >  128 );
  AOT_DBG( iWeight < -128 );

  for( Int y = 0; y < iSizeY; y++)
  {
    for( Int x = 0; x < iSizeX; x++)
    {
      Int iTemp  = (( iWeight * pucDest[x] + iAdd) >> uiDenom);
      pucDest[x] = gClip( iTemp );
    }
    pucDest += iDestStride;
  }
}


Void SampleWeighting::xMixBWeight( XPel* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Int iSizeY, Int iSizeX, Int iWD, Int iWS, Int iOffset, UInt uiDenom )
{
  Int iAdd = (1<<uiDenom);

  AOT_DBG( (iWD + iWS) > ((uiDenom == 7) ? 127 : 128));
  AOT_DBG( iWD + iWS < -128 );

  uiDenom++;
  iOffset = (iOffset+1) >> 1;
  for( Int y = 0; y < iSizeY; y++)
  {
    for( Int x = 0; x < iSizeX; x++)
    {
      Int iTemp = (( iWD * pucDest[x] + iWS * pucSrc[x] + iAdd) >> uiDenom) + iOffset;
      pucDest[x] = gClip( iTemp );
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}


__inline Void SampleWeighting::xMixB(XPel* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Int iSizeY, Int iSizeX)
{
  m_afpXMixSampleFunc[iSizeX>>2]( pucDest, iDestStride, pucSrc, iSrcStride, iSizeY );
}


Void
SampleWeighting::weightInverseLumaSamples( YuvMbBuffer*  pcDesBuffer,
                                           YuvMbBuffer*  pcOrgBuffer,
                                           YuvMbBuffer*  pcFixBuffer,
                                           const PredWeight*        pcSearchPW,
                                           const PredWeight*        pcFixPW,
                                           Double&          rdWeight,
                                           Int              iYSize,
                                           Int              iXSize )
{
  XPel* pFix        = pcFixBuffer->getLumBlk();
  XPel* pOrg        = pcOrgBuffer->getLumBlk();
  XPel* pDes        = pcDesBuffer->getLumBlk();
  Int   iStride     = pcDesBuffer->getLStride();

  Int   iFixWeight  = pcFixPW   ->getLumaWeight();
  Int   iWeight     = pcSearchPW->getLumaWeight();
  Int   iOffset     = ( pcFixPW ->getLumaOffset() + pcSearchPW->getLumaOffset() + 1 ) >> 1;
  Int   iLWD        = m_uiLumaLogWeightDenom;
  Int   iAdd        = ( 1 << iLWD );

  AOT_DBG( m_bExplicit && iWeight >  128 );
  AOT_DBG( m_bExplicit && iWeight < -128 );

  if( iWeight == 0 ) // doesn't make sense to transmit a motion vector for that case
  {
    for( Int iLine = 0, y = 0; y < iYSize; y++, iLine += iStride )
    for( Int            x = 0; x < iXSize; x++ )
    {
      pDes[x+iLine] = 128;
    }
    rdWeight        = 1.0;
  }
  else
  {
    Int iShift      = ( iLWD + 8 );
    Int iInvWeight  = ( 1    << iShift ) / iWeight;
    Int iInvAdd     = ( iAdd << iShift ) / iWeight;

    for( Int iLine = 0, y = 0; y < iYSize; y++, iLine += iStride )
    for( Int            x = 0; x < iXSize; x++ )
    {
      Int iTemp     = ( iInvWeight * ( ( ( pOrg[x+iLine] - iOffset ) << (iLWD+1) ) - iFixWeight * pFix[x+iLine] ) - iInvAdd ) >> iShift;
      pDes[x+iLine] = gClip( iTemp );
    }
    rdWeight = 128.0 / abs(iInvWeight);
  }
}




Void
SampleWeighting::weightInverseLumaSamples( YuvMbBuffer* pcDesBuffer,
                                           YuvMbBuffer* pcOrgBuffer,
                                           const PredWeight*       pcPW,
                                           Double&         rdWeight,
                                           Int             iYSize,
                                           Int             iXSize )
{
  XPel* pOrg      = pcOrgBuffer->getLumBlk();
  XPel* pDes      = pcDesBuffer->getLumBlk();
  Int   iStride   = pcDesBuffer->getLStride();

  Int   iWeight   = pcPW->getLumaWeight();
  Int   iOffset   = pcPW->getLumaOffset();
  Int   iAdd      = ( 1 << m_uiLumaLogWeightDenom ) >> 1;

  AOT_DBG( iWeight >  127 );
  AOT_DBG( iWeight < -128 );

  if( iWeight == 0 ) // motion vector doesn't make sense
  {
    for( Int y = 0; y < iYSize; y++, pDes+=iStride )
    for( Int x = 0; x < iXSize; x++)
    {
      pDes[x]       = 128;
    }
    rdWeight        = 1.0;
  }
  else if( ! pcPW->getLumaWeightFlag() )
  {
    //===== unweighted copy =====
    for( Int y = 0; y < iYSize; y++, pDes+=iStride )
    for( Int x = 0; x < iXSize; x++)
    {
      pDes[x]       = pOrg[x];
    }
    rdWeight        = 1.0;
  }
  else
  {
    Int iInvWeight  = ( 1    << ( m_uiLumaLogWeightDenom + 8 ) ) / iWeight;
    Int iInvAdd     = ( iAdd <<                            8   ) / iWeight;

    for( Int y = 0; y < iYSize; y++, pDes+=iStride, pOrg+=iStride )
    for( Int x = 0; x < iXSize; x++)
    {
      Int iTemp     = ( iInvWeight * ( pOrg[x] - iOffset ) - iInvAdd ) >> 8;
      pDes[x]       = gClip( iTemp );
    }
    rdWeight        = 256.0 / abs(iInvWeight);
  }
}



Void
SampleWeighting::weightInverseChromaSamples( YuvMbBuffer* pcDesBuffer,
                                             YuvMbBuffer* pcOrgBuffer,
                                             const PredWeight*       pcPW,
                                             Double*         padWeight,
                                             Int             iYSize,
                                             Int             iXSize )
{
  iYSize >>= 1;
  iXSize >>= 1;

  for( Int C = 0; C < 2; C++ )
  {
    XPel* pOrg      = ( C ? pcOrgBuffer->getCrBlk() : pcOrgBuffer->getCbBlk() );
    XPel* pDes      = ( C ? pcDesBuffer->getCrBlk() : pcDesBuffer->getCbBlk() );
    Int   iStride   = pcDesBuffer->getCStride();

    Int   iWeight   = ( C ? pcPW->getChromaCrWeight() : pcPW->getChromaCbWeight() );
    Int   iOffset   = ( C ? pcPW->getChromaCrOffset() : pcPW->getChromaCbOffset() );
    Int   iAdd      = ( 1 << m_uiChromaLogWeightDenom ) >> 1;

    AOT_DBG( iWeight >  127 );
    AOT_DBG( iWeight < -128 );

    if( iWeight == 0 ) // motion vector doesn't make sense
    {
      for( Int y = 0; y < iYSize; y++, pDes+=iStride )
      for( Int x = 0; x < iXSize; x++)
      {
        pDes[x]       = 128;
      }
      padWeight[C]    = 1.0;
    }
    else if( ! pcPW->getChromaWeightFlag() )
    {
      //===== unweighted copy =====
      for( Int y = 0; y < iYSize; y++, pDes+=iStride )
      for( Int x = 0; x < iXSize; x++)
      {
        pDes[x]       = pOrg[x];
      }
      padWeight[C]    = 1.0;
    }
    else
    {
      Int iInvWeight  = ( 1    << ( m_uiChromaLogWeightDenom + 8 ) ) / iWeight;
      Int iInvAdd     = ( iAdd <<                              8   ) / iWeight;

      for( Int y = 0; y < iYSize; y++, pDes+=iStride, pOrg+=iStride )
      for( Int x = 0; x < iXSize; x++)
      {
        Int iTemp     = ( iInvWeight * ( pOrg[x] - iOffset ) - iInvAdd ) >> 8;
        pDes[x]       = gClip( iTemp );
      }
      padWeight[C]    = 256.0 / abs(iInvWeight);
    }
  }
}


//TMM_WP
ErrVal SampleWeighting::initSliceForWeighting( const SliceHeader& rcSliceHeader)
{
  if( rcSliceHeader.isIntraSlice() )
  {
    m_bWeightedPredDisableP = true;
    m_bWeightedPredDisableB = true;
    m_bExplicit             = false;
    return Err::m_nOK;
  }

  if( rcSliceHeader.isPSlice() )
  {
    m_bExplicit             = rcSliceHeader.getPPS().getWeightedPredFlag();
    m_bWeightedPredDisableP = ! m_bExplicit;
    m_bWeightedPredDisableB = true;

    if( m_bExplicit )
    {
      m_uiLumaLogWeightDenom   = rcSliceHeader.getLumaLog2WeightDenom();
      m_uiChromaLogWeightDenom = rcSliceHeader.getChromaLog2WeightDenom();
    }

    return Err::m_nOK;
  }


  else if( rcSliceHeader.isBSlice() )
  {
    switch( rcSliceHeader.getPPS().getWeightedBiPredIdc() )
    {
    case 0:
      {
        m_bExplicit               = false;
        m_bWeightedPredDisableP   = true;
        m_bWeightedPredDisableB   = true;
        m_uiLumaLogWeightDenom    = 0;
        m_uiChromaLogWeightDenom  = 0;
      }
      break;
    case 1:
      {
        m_bExplicit               = true;
        m_bWeightedPredDisableP   = false;
        m_bWeightedPredDisableB   = false;
        m_uiLumaLogWeightDenom    = rcSliceHeader.getLumaLog2WeightDenom();
        m_uiChromaLogWeightDenom  = rcSliceHeader.getChromaLog2WeightDenom();
      }
      break;
    case 2:
      {
        m_bExplicit               = false;
        m_bWeightedPredDisableP   = true;
        m_bWeightedPredDisableB   = false;
        m_uiLumaLogWeightDenom    = 5;
        m_uiChromaLogWeightDenom  = 5;
      }
      break;
    default:
      {
        AF();
      }
      break;
    }
  }

  return Err::m_nOK;
}

//TMM_WP

H264AVC_NAMESPACE_END

