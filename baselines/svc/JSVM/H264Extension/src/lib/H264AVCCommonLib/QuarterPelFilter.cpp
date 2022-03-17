
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/QuarterPelFilter.h"

H264AVC_NAMESPACE_BEGIN


QuarterPelFilter::QuarterPelFilter()
{
  uninit();
}

QuarterPelFilter::~QuarterPelFilter()
{
}

ErrVal QuarterPelFilter::create( QuarterPelFilter*& rpcQuarterPelFilter )
{
  QuarterPelFilter* pcQuarterPelFilter;

  pcQuarterPelFilter = new QuarterPelFilter;

  rpcQuarterPelFilter = pcQuarterPelFilter;

  ROT( NULL == rpcQuarterPelFilter );

  return Err::m_nOK;
}


ErrVal QuarterPelFilter::destroy()
{
  delete this;

  return Err::m_nOK;
}


ErrVal QuarterPelFilter::init()
{
  m_afpXFilterBlockFunc[0] = QuarterPelFilter::xXFilter1;
  m_afpXFilterBlockFunc[1] = QuarterPelFilter::xXFilter2;
  m_afpXFilterBlockFunc[2] = QuarterPelFilter::xXFilter3;
  m_afpXFilterBlockFunc[3] = QuarterPelFilter::xXFilter4;
  return Err::m_nOK;
}

ErrVal QuarterPelFilter::uninit()
{
  m_afpXFilterBlockFunc[0] = NULL;
  m_afpXFilterBlockFunc[1] = NULL;
  m_afpXFilterBlockFunc[2] = NULL;
  m_afpXFilterBlockFunc[3] = NULL;
  return Err::m_nOK;
}


const Int g_aiTapCoeff[6] = { 1, -5,20,20,-5, 1};


ErrVal QuarterPelFilter::filterFrame( YuvPicBuffer *pcPelBuffer, YuvPicBuffer *pcHalfPelBuffer )
{
  ROT( NULL == pcPelBuffer );
  ROT( NULL == pcHalfPelBuffer );

  XPel*   pucSrc      = pcPelBuffer->getMbLumAddr ();
  Int     iHeight     = pcPelBuffer->getLHeight   ();
  Int     iWidth      = pcPelBuffer->getLWidth    ();
  Int     iStride     = pcPelBuffer->getLStride   ();
  Int     iMargin     = pcPelBuffer->getLXMargin  ();
  Int     iTmpXSize  = (iMargin + iWidth + iMargin) * 2;
  Int     iTmpYSize  = iMargin + iHeight + iMargin;
  Int     iMarginNew  = iMargin-4;
  Int     x, y;

  XXPel*  psTemp      = new XXPel[iTmpXSize * iTmpYSize];
  ::memset( psTemp, 0x00, (iTmpXSize * iTmpYSize)*sizeof(XXPel) );

  ROT( NULL == psTemp )

  XXPel*  ps          = &psTemp[ iMargin * iTmpXSize + 2*iMargin ]; // fix provided by Shijun Sun

  for( y = 0; y < iHeight; y++ )
  {
    for( x = -iMarginNew; x < iWidth+iMarginNew; x++ )
    {
      Int iTemp;
      iTemp  = pucSrc[x - 0];
      iTemp += pucSrc[x + 1];
      iTemp  = iTemp << 2;
      iTemp -= pucSrc[x - 1];
      iTemp -= pucSrc[x + 2];
      iTemp += iTemp << 2;
      iTemp += pucSrc[x - 2];
      iTemp += pucSrc[x + 3];
      ps[2*x]    = pucSrc[x]<<5;
      ps[2*x+1]  = iTemp;
    }

    ps     += iTmpXSize;
    pucSrc += iStride;
  }

  // bot
  ps -= iMargin*2;               // fix provided by Shijun Sun
  for( y = 0; y < iMargin; y++ ) // fix provided by Shijun Sun
  {
    ::memcpy( &ps[y*iTmpXSize], &ps[(y-1)*iTmpXSize], iTmpXSize*sizeof(XXPel) );
  }

  //top
  ps = &psTemp[ iMargin * iTmpXSize ]; // fix provided by Shijun Sun
  for( y = 0; y < iMargin; y++ )        // fix provided by Shijun Sun
  {
    ::memcpy( &ps[-(y+1)*iTmpXSize], &ps[-y*iTmpXSize], iTmpXSize*sizeof(XXPel) );
  }

  ps = &psTemp[ 4*iTmpXSize + 2*iMargin ]; // fix provided by Shijun Sun
  iStride = iTmpXSize;

  Int   iDesStrideHP  = pcHalfPelBuffer->getLStride();
  XPel* pucDesHP      = pcHalfPelBuffer->getMbLumAddr();
  pucDesHP -= (iMarginNew*iDesStrideHP)<<1;

  for( y = -iMarginNew; y < iHeight+iMarginNew; y++ )
  {
    for( x = -2*iMarginNew; x < 2*(iWidth+iMarginNew); x++ )
    {
      Int iTemp;
      iTemp  = ps[x - 0*iStride];
      iTemp += ps[x + 1*iStride];
      iTemp  = iTemp << 2;
      iTemp -= ps[x - 1*iStride];
      iTemp -= ps[x + 2*iStride];
      iTemp += iTemp << 2;
      iTemp += ps[x - 2*iStride];
      iTemp += ps[x + 3*iStride];

      pucDesHP[x]              = gClip( ( ps[x] + 16) / 32);
      pucDesHP[x+iDesStrideHP] = gClip( ( iTemp + 512) / 1024);
    }
    pucDesHP += iDesStrideHP<<1;
    ps     += iStride;
  }

  delete [] psTemp;
  psTemp = NULL;
  return Err::m_nOK;
}


Void QuarterPelFilter::xXFilter1( XPel* pDes, XPel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize )
{
  UInt y, x;
  for( y = 0; y < uiYSize; y++ )
  {
    for( x = 0; x < uiXSize; x++ )
    {
      Int a = pSrc[-1];
      Int b = pSrc[-iSrcStride];
      Int c = pSrc[+1];
      Int d = pSrc[iSrcStride];
      Int o = pSrc[0];

      pDes[0x000] = (a + b + 1) >> 1;
      pDes[0x100] = (b + o + 1) >> 1;
      pDes[0x200] = (b + c + 1) >> 1;
      pDes[0x300] = (a + o + 1) >> 1;
      pDes[0x500] = (c + o + 1) >> 1;
      pDes[0x600] = (a + d + 1) >> 1;
      pDes[0x700] = (d + o + 1) >> 1;
      pDes[0x800] = (d + c + 1) >> 1;
      pSrc += 2;
      pDes++;
    }
    pSrc += 2*(iSrcStride - uiXSize);
    pDes += 16 - uiXSize;
  }
}


Void QuarterPelFilter::xXFilter2( XPel* pDes, XPel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize )
{
  UInt y, x;

  for( y = 0; y < uiYSize; y++ )
  {
    for( x = 0; x < uiXSize; x++ )
    {
      Int o = pSrc[0];

      pDes[0x000] = (o + pSrc[-iSrcStride-1] + 1) >> 1;
      pDes[0x100] = (o + pSrc[-iSrcStride  ] + 1) >> 1;
      pDes[0x200] = (o + pSrc[-iSrcStride+1] + 1) >> 1;
      pDes[0x300] = (o + pSrc[-1] + 1) >> 1;
      pDes[0x500] = (o + pSrc[+1] + 1) >> 1;
      pDes[0x600] = (o + pSrc[iSrcStride-1] + 1) >> 1;
      pDes[0x700] = (o + pSrc[iSrcStride  ] + 1) >> 1;
      pDes[0x800] = (o + pSrc[iSrcStride+1] + 1) >> 1;
      pSrc += 2;
      pDes++;
    }
    pSrc += 2*(iSrcStride - uiXSize);
    pDes += 16 - uiXSize;
  }
}

Void QuarterPelFilter::xXFilter3( XPel* pDes, XPel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize )
{
  UInt y, x;

  for( y = 0; y < uiYSize; y++ )
  {
    for( x = 0; x < uiXSize; x++ )
    {
      Int o = pSrc[0];

      pDes[0x000] = (o + pSrc[-iSrcStride-1] + 1) >> 1;
      pDes[0x100] = (o + pSrc[-iSrcStride  ] + 1) >> 1;
      pDes[0x200] = (o + pSrc[-iSrcStride+1] + 1) >> 1;
      pDes[0x300] = (o + pSrc[-1] + 1) >> 1;
      pDes[0x500] = (o + pSrc[+1] + 1) >> 1;
      pDes[0x600] = (o + pSrc[iSrcStride-1] + 1) >> 1;
      pDes[0x700] = (o + pSrc[iSrcStride  ] + 1) >> 1;
      pDes[0x800] = (o + pSrc[iSrcStride+1] + 1) >> 1;
      pSrc += 2;
      pDes++;
    }
    pSrc += 2*(iSrcStride - uiXSize);
    pDes += 16 - uiXSize;
  }
}

Void QuarterPelFilter::xXFilter4( XPel* pDes, XPel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize )
{
  UInt y, x;

  for( y = 0; y < uiYSize; y++ )
  {
    for( x = 0; x < uiXSize; x++ )
    {
      Int a = pSrc[-1];
      Int b = pSrc[-iSrcStride];
      Int c = pSrc[+1];
      Int d = pSrc[iSrcStride];
      Int o = pSrc[0];

      pDes[0x000] = (a + b + 1) >> 1;
      pDes[0x100] = (b + o + 1) >> 1;
      pDes[0x200] = (b + c + 1) >> 1;
      pDes[0x300] = (a + o + 1) >> 1;
      pDes[0x500] = (c + o + 1) >> 1;
      pDes[0x600] = (a + d + 1) >> 1;
      pDes[0x700] = (d + o + 1) >> 1;
      pDes[0x800] = (c + d + 1) >> 1;
      pSrc += 2;
      pDes++;
    }
    pSrc += 2*(iSrcStride - uiXSize);
    pDes += 16 - uiXSize;
  }
}



Void QuarterPelFilter::predBlk( YuvMbBuffer* pcDesBuffer, YuvPicBuffer* pcSrcBuffer, LumaIdx cIdx, Mv cMv, Int iSizeY, Int iSizeX)
{
  XPel* pucDes    = pcDesBuffer->getYBlk( cIdx );
  XPel* pucSrc    = pcSrcBuffer->getYBlk( cIdx );
  Int iDesStride  = pcDesBuffer->getLStride();
  Int iSrcStride  = pcSrcBuffer->getLStride();
  Int iOffset     = (cMv.getHor() >> 2) + (cMv.getVer() >> 2) * iSrcStride;

  pucSrc += iOffset;

  Int iDx = cMv.getHor() & 3;
  Int iDy = cMv.getVer() & 3;
  if( iDy == 0)
  {
    if( iDx == 0 )
    {
      for( Int y = 0; y < iSizeY; y++)
      {
        for( Int x = 0; x < iSizeX; x++ )
        {
          pucDes[x] = pucSrc[x];
        }
        pucDes += iDesStride;
        pucSrc += iSrcStride;
      }
      return;
    }


    if( iDx == 2 )
    {
      xPredDy0Dx2( pucDes, pucSrc, iDesStride, iSrcStride, iSizeY, iSizeX );
      return;
    }

    // if( iDx == 1 || iDx == 3)
    xPredDy0Dx13( pucDes, pucSrc, iDesStride, iSrcStride, iDx, iSizeY, iSizeX );
    return;
  }


  if( iDx == 0)
  {
    if( iDy == 2 )
    {
      xPredDx0Dy2( pucDes, pucSrc, iDesStride, iSrcStride, iSizeY, iSizeX );
      return;
    }

    // if( iDx == 1 || iDx == 3)
    xPredDx0Dy13( pucDes, pucSrc, iDesStride, iSrcStride, iDx, iDy, iSizeY, iSizeX );
    return;
  }


  if( iDx == 2)
  {
    if( iDy == 2 )
    {
      xPredDx2Dy2( pucDes, pucSrc, iDesStride, iSrcStride, iDx, iDy, iSizeY, iSizeX );
      return;
    }

    // if( iDy == 1 || iDy == 3 )
    xPredDx2Dy13( pucDes, pucSrc, iDesStride, iSrcStride, iDx, iDy, iSizeY, iSizeX );
    return;
  }



  if( iDy == 2)
  {
    // if( iDx == 2 ) is already done
    // if( iDx == 1 || iDx == 3 )
    xPredDy2Dx13( pucDes, pucSrc, iDesStride, iSrcStride, iDx, iDy, iSizeY, iSizeX );
    return;
  }


  xPredElse( pucDes, pucSrc, iDesStride, iSrcStride, iDx, iDy, iSizeY, iSizeX );
  return;
}




Void QuarterPelFilter::xPredDy0Dx2( XPel* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride,  UInt uiSizeY, UInt uiSizeX )
{
  for( Int y = 0; y < (Int)uiSizeY; y++)
  {
    for( Int x = 0; x < (Int)uiSizeX; x++)
    {
      Int iTemp;
      iTemp  = pucSrc[x - 0];
      iTemp += pucSrc[x + 1];
      iTemp  = iTemp << 2;
      iTemp -= pucSrc[x - 1];
      iTemp -= pucSrc[x + 2];
      iTemp += iTemp << 2;
      iTemp += pucSrc[x - 2];
      iTemp += pucSrc[x + 3];
      pucDest[x] = gClip( (iTemp + 16) / 32 );
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}


Void QuarterPelFilter::xPredDy0Dx13( XPel* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, UInt uiSizeY, UInt uiSizeX )
{

  iDx >>= 1;
  for( Int y = 0; y < (Int)uiSizeY; y++)
  {
    for( Int x = 0; x < (Int)uiSizeX; x++)
    {
      Int iTemp;
      iTemp  = pucSrc[x - 0];
      iTemp += pucSrc[x + 1];
      iTemp  = iTemp << 2;
      iTemp -= pucSrc[x - 1];
      iTemp -= pucSrc[x + 2];
      iTemp += iTemp << 2;
      iTemp += pucSrc[x - 2];
      iTemp += pucSrc[x + 3];
      iTemp = gClip( (iTemp + 16) / 32 );
      pucDest[x] = (iTemp + pucSrc[ x + iDx] + 1) / 2;
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}


Void QuarterPelFilter::xPredDx0Dy2( XPel* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, UInt uiSizeY, UInt uiSizeX )
{
  for( Int y = 0; y < (Int)uiSizeY; y++)
  {
    for( Int x = 0; x < (Int)uiSizeX; x++)
    {
      Int iTemp;
      iTemp  = pucSrc[x - 0*iSrcStride];
      iTemp += pucSrc[x + 1*iSrcStride];
      iTemp  = iTemp << 2;
      iTemp -= pucSrc[x - 1*iSrcStride];
      iTemp -= pucSrc[x + 2*iSrcStride];
      iTemp += iTemp << 2;
      iTemp += pucSrc[x - 2*iSrcStride];
      iTemp += pucSrc[x + 3*iSrcStride];
      pucDest[x] = gClip( (iTemp + 16) / 32 );
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}


Void QuarterPelFilter::xPredDx0Dy13( XPel* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy, UInt uiSizeY, UInt uiSizeX )
{
  iDy = (iDy>>1) * iSrcStride;
#if 0
  for( UInt y = 0; y < uiSizeY; y++)
  {
    for( UInt x = 0; x < uiSizeX; x++)
#else
  for( Int y = 0; y < (Int)uiSizeY; y++)
  {
    for( Int x = 0; x < (Int)uiSizeX; x++)
#endif
    {
      Int iTemp;
      iTemp  = pucSrc[x - 0*iSrcStride];
      iTemp += pucSrc[x + 1*iSrcStride];
      iTemp  = iTemp << 2;
      iTemp -= pucSrc[x - 1*iSrcStride];
      iTemp -= pucSrc[x + 2*iSrcStride];
      iTemp += iTemp << 2;
      iTemp += pucSrc[x - 2*iSrcStride];
      iTemp += pucSrc[x + 3*iSrcStride];
      iTemp = gClip( (iTemp + 16) / 32 );
      pucDest[x] = (iTemp + pucSrc[ x + iDy] + 1)/2;
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}



Void QuarterPelFilter::xPredDx2( XXPel*  psDest, XPel*  pucSrc, Int iSrcStride, UInt uiSizeY, UInt uiSizeX )
{
  pucSrc -= 2*iSrcStride;

  for( UInt y = 0; y < uiSizeY + 5; y++)
  {
    for( UInt x = 0; x < uiSizeX; x++)
    {
      XPel* puc = pucSrc + x;
      Int iTemp;
      iTemp  = puc[0];
      iTemp += puc[1];
      iTemp  = iTemp << 2;
      iTemp -= puc[-1];
      iTemp -= puc[2];
      iTemp += iTemp << 2;
      iTemp += puc[-2];
      iTemp += puc[3];
      psDest[x] = iTemp;

    }
    psDest += 16;
    pucSrc += iSrcStride;
  }
}



Void QuarterPelFilter::xPredDx2Dy2( XPel* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy, UInt uiSizeY, UInt uiSizeX )
{
  XXPel   asTemp[16*(16+6)];
  XXPel*  psTemp = asTemp;
  xPredDx2( asTemp, pucSrc, iSrcStride, uiSizeY, uiSizeX );

  for( UInt y = 0; y < uiSizeY; y++)
  {
    for( UInt x = 0; x < uiSizeX; x++)
    {
      Int iIndex = x + 0x20;
      Int iTemp;
      iTemp  = psTemp[ 0x00 + iIndex];
      iTemp += psTemp[ 0x10 + iIndex];
      iTemp  = iTemp << 2;
      iTemp -= psTemp[-0x10 + iIndex];
      iTemp -= psTemp[ 0x20 + iIndex];
      iTemp += iTemp << 2;
      iTemp += psTemp[-0x20 + iIndex];
      iTemp += psTemp[ 0x30 + iIndex];
      pucDest[x] = gClip( (iTemp + 512) / 1024 );
    }
    psTemp  += 0x10;
    pucDest += iDestStride;
  }

}


Void QuarterPelFilter::xPredDx2Dy13( XPel* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy, UInt uiSizeY, UInt uiSizeX )
{
  XXPel   asTemp[16*(16+6)];
  XXPel*  psTemp = asTemp;
  xPredDx2( asTemp, pucSrc, iSrcStride, uiSizeY, uiSizeX );

  iDy = (iDy == 1) ? 0 : 0x10;

  for( UInt y = 0; y < uiSizeY; y++)
  {
    for( UInt x = 0; x < uiSizeX; x++)
    {
      Int iIndex = x + 0x20;
      Int iTemp;
      iTemp  = psTemp[ 0x00 + iIndex];
      iTemp += psTemp[ 0x10 + iIndex];
      iTemp  = iTemp << 2;
      iTemp -= psTemp[-0x10 + iIndex];
      iTemp -= psTemp[ 0x20 + iIndex];
      iTemp += iTemp << 2;
      iTemp += psTemp[-0x20 + iIndex];
      iTemp += psTemp[ 0x30 + iIndex];
      iTemp = gClip( (iTemp + 512) / 1024 );
      pucDest[x] = (iTemp + gClip( (psTemp[iDy + iIndex] + 16) / 32 ) + 1) / 2;
    }
    psTemp  += 0x10;
    pucDest += iDestStride;
  }
}


Void QuarterPelFilter::xPredDy2Dx13( XPel* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy, UInt uiSizeY, UInt uiSizeX )
{
  iDx = (iDx == 1) ? 2 : 3;
  for( UInt y = 0; y < uiSizeY; y++)
  {
    Int aiTemp[6+16];
    for( UInt n = 0; n < 6+uiSizeX; n++)
    {
      XPel* puc = pucSrc + n - 2;
      aiTemp[n]  = puc[ 0*iSrcStride];
      aiTemp[n] += puc[ 1*iSrcStride];
      aiTemp[n]  = aiTemp[n] << 2;
      aiTemp[n] -= puc[-1*iSrcStride];
      aiTemp[n] -= puc[ 2*iSrcStride];
      aiTemp[n] += aiTemp[n] << 2;
      aiTemp[n] += puc[-2*iSrcStride];
      aiTemp[n] += puc[ 3*iSrcStride];
    }

    for( UInt x = 0; x < uiSizeX; x++)
    {
      Int iTemp = 0;
      for( Int n = 0; n < 6; n++ )
      {
        iTemp += aiTemp[n+x]*g_aiTapCoeff[n];
      }
      iTemp = gClip( (iTemp + 512) / 1024 );
      pucDest[x] = (iTemp + gClip( (aiTemp[x+iDx] + 16) / 32 ) + 1) / 2;
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}



Void QuarterPelFilter::xPredElse( XPel* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy, UInt uiSizeY, UInt uiSizeX )
{
  XPel* pucSrcX = pucSrc;
  XPel* pucSrcY = pucSrc;

  pucSrcY += (iDx == 1) ? 0 : 1;
  pucSrcX += (iDy == 1) ? 0 : iSrcStride;

  for( Int y = 0; y < (Int)uiSizeY; y++)
  {
    for( Int x = 0; x < (Int)uiSizeX; x++)
    {
      Int iTempX;
      iTempX  = pucSrcX[x - 0];
      iTempX += pucSrcX[x + 1];
      iTempX  = iTempX << 2;
      iTempX -= pucSrcX[x - 1];
      iTempX -= pucSrcX[x + 2];
      iTempX += iTempX << 2;
      iTempX += pucSrcX[x - 2];
      iTempX += pucSrcX[x + 3];
      iTempX = gClip( (iTempX + 16) / 32 );

      Int iTempY;
      iTempY  = pucSrcY[x - 0*iSrcStride];
      iTempY += pucSrcY[x + 1*iSrcStride];
      iTempY  = iTempY << 2;
      iTempY -= pucSrcY[x - 1*iSrcStride];
      iTempY -= pucSrcY[x + 2*iSrcStride];
      iTempY += iTempY << 2;
      iTempY += pucSrcY[x - 2*iSrcStride];
      iTempY += pucSrcY[x + 3*iSrcStride];
      iTempY = gClip( (iTempY + 16) / 32 );
      pucDest[x] = (iTempX + iTempY + 1) >> 1;
    }
    pucDest += iDestStride;
    pucSrcX += iSrcStride;
    pucSrcY += iSrcStride;
  }
}


Void QuarterPelFilter::xUpdInterpBlnr(Int* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy,
                                    Int iSizeY, Int iSizeX )
{
  static int f2tap[4][2] = {
    { 4,  0},
    { 3,  1},
    { 2,  2},
    { 1,  3}
  };
  int sx1, sx2, sy1, sy2;
  for( Int y = 0; y < iSizeY + 1; y++)
  {
    for( Int x = 0; x < iSizeX + 1; x++)
    {
      Int iTemp1[2], iTemp2;
      int i, j;
      sx1 = gMax(0, x + 1 - iSizeX);
      sx2 = gMin(2, x + 1);
      sy1 = gMax(0, y + 1 - iSizeY);
      sy2 = gMin(2, y + 1);

      for(i = sy1; i < sy2; i++)
      {
        iTemp1[i] = 0;

        for(j = sx1; j < sx2; j++ )
          iTemp1[i] += pucSrc[x - i*iSrcStride - j] * f2tap[iDx][j];
      }

      iTemp2 = 0;
      for(i = sy1; i < sy2; i++)
        iTemp2 += iTemp1[i] * f2tap[iDy][i];

      pucDest[x] = iTemp2;
    }
    pucDest += iDestStride;
    pucSrc += iSrcStride;
  }
}


Void QuarterPelFilter::xUpdInterp4Tap(Int* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy,
                                    Int iSizeY, Int iSizeX )
{
  static int f4tap[4][4] = {
    { 0, 16,  0,  0},
    {-2, 14,  5, -1},
    {-2, 10, 10, -2},
    {-1,  5, 14, -2}
  };

  int sx1, sx2, sy1, sy2;
  for( Int y = 0; y < iSizeY + 3; y++)
  {
    for( Int x = 0; x < iSizeX + 3; x++)
    {
      Int iTemp1[4], iTemp2;
      int i, j;
      sx1 = gMax(0, x + 1 - iSizeX);
      sx2 = gMin(4, x + 1);
      sy1 = gMax(0, y + 1 - iSizeY);
      sy2 = gMin(4, y + 1);

      for(i = sy1; i < sy2; i++)
      {
        iTemp1[i] = 0;

        for(j = sx1; j < sx2; j++ )
          iTemp1[i] += pucSrc[x - i*iSrcStride - j] * f4tap[iDx][j];
      }

      iTemp2 = 0;
      for(i = sy1; i < sy2; i++)
        iTemp2 += iTemp1[i] * f4tap[iDy][i];

      pucDest[x] = iTemp2;
    }
    pucDest += iDestStride;
    pucSrc += iSrcStride;
  }
}

Void QuarterPelFilter::xUpdInterpChroma( Int* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Mv cMv, Int iSizeY, Int iSizeX )
{
  Int iDx = (cMv.getHor() & 0x7);
  Int iDy = (cMv.getVer() & 0x7);

  static int f2tapC[8][2] = {
    { 8,  0},
    { 7,  1},
    { 6,  2},
    { 5,  3},
    { 4,  4},
    { 3,  5},
    { 2,  6},
    { 1,  7}
  };
  int sx1, sx2, sy1, sy2;
  for( Int y = 0; y < iSizeY + 1; y++)
  {
    for( Int x = 0; x < iSizeX + 1; x++)
    {
      Int iTemp1[2], iTemp2;
      int i, j;
      sx1 = gMax(0, x + 1 - (int)iSizeX);
      sx2 = gMin(2, x + 1);
      sy1 = gMax(0, y + 1 - (int)iSizeY);
      sy2 = gMin(2, y + 1);

      for(i = sy1; i < sy2; i++)
      {
        iTemp1[i] = 0;

        for(j = sx1; j < sx2; j++ )
          iTemp1[i] += pucSrc[x - i*iSrcStride - j] * f2tapC[iDx][j];
      }

      iTemp2 = 0;
      for(i = sy1; i < sy2; i++)
        iTemp2 += iTemp1[i] * f2tapC[iDy][i];

      pucDest[x] = iTemp2;
    }
    pucDest += iDestStride;
    pucSrc += iSrcStride;
  }
}

Void QuarterPelFilter::weightOnEnergy(UShort *usWeight, XPel* pucSrc, Int iSrcStride, Int iSizeY, Int iSizeX)
{
  Int iSSD = 0;
  int bitsShift = 8, i;

  for( Int y = 0; y < iSizeY; y++)
  {
    for( Int x = 0; x < iSizeX; x++)
    {
      Int iTemp;
      iTemp  = gClip( pucSrc[iSrcStride*y + x]);
      iSSD += iTemp*iTemp;
    }
  }


  for( i = (iSizeY/4)*(iSizeX/4); i > 1; i >>= 1 )
    bitsShift ++;

  iSSD                                = ( iSSD + (1 << (bitsShift-1)) ) >> bitsShift;
  *usWeight                           = (UShort) gMax( 0, gMin( 16, 20 - iSSD ) );
}

H264AVC_NAMESPACE_END
