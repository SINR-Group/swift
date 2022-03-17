
#if !defined(AFX_INTERPOLATIONFILTER_H__79B49DF9_A41E_4043_AD0E_CAAC3A0A9DA1__INCLUDED_)
#define AFX_INTERPOLATIONFILTER_H__79B49DF9_A41E_4043_AD0E_CAAC3A0A9DA1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/YuvMbBuffer.h"
#include "H264AVCCommonLib/YuvPicBuffer.h"

H264AVC_NAMESPACE_BEGIN

typedef Void (*FilterBlockFunc)( Pel* pDes, Pel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize );
typedef Void (*XFilterBlockFunc)( XPel* pDes, XPel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize );

class H264AVCCOMMONLIB_API QuarterPelFilter
{
protected:
	QuarterPelFilter();
	virtual ~QuarterPelFilter();

public:
  static ErrVal create( QuarterPelFilter*& rpcQuarterPelFilter );
  ErrVal destroy();
  virtual ErrVal init();
  ErrVal uninit();

  virtual ErrVal filterFrame( YuvPicBuffer* pcPelBuffer, YuvPicBuffer* pcHalfPelBuffer );
  Void filterBlock( XPel* pDes, XPel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize, UInt uiFilter )
  {
    m_afpXFilterBlockFunc[uiFilter]( pDes, pSrc, iSrcStride, uiXSize, uiYSize );
  }

  Void predBlk( YuvMbBuffer*     pcDesBuffer, YuvPicBuffer*     pcSrcBuffer, LumaIdx cIdx, Mv cMv, Int iSizeY, Int iSizeX);

  Void weightOnEnergy(UShort *usWeight, XPel* pucSrc, Int iSrcStride, Int iSizeY, Int iSizeX );
  Void xUpdInterpBlnr(Int* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy,
                                      Int uiSizeY, Int uiSizeX );
  Void xUpdInterp4Tap(Int* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy,
                                      Int iSizeY, Int iSizeX );
  Void xUpdInterpChroma( Int* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Mv cMv, Int iSizeY, Int iSizeX );

protected:
  Void xPredElse    ( XPel*  pucDest, XPel*  pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy, UInt uiSizeY, UInt uiSizeX );
  Void xPredDy2Dx13 ( XPel*  pucDest, XPel*  pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy, UInt uiSizeY, UInt uiSizeX );
  Void xPredDx2Dy13 ( XPel*  pucDest, XPel*  pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy, UInt uiSizeY, UInt uiSizeX );
  Void xPredDx2Dy2  ( XPel*  pucDest, XPel*  pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy, UInt uiSizeY, UInt uiSizeX );
  Void xPredDx0Dy13 ( XPel*  pucDest, XPel*  pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy, UInt uiSizeY, UInt uiSizeX );
  Void xPredDx0Dy2  ( XPel*  pucDest, XPel*  pucSrc, Int iDestStride, Int iSrcStride, UInt uiSizeY, UInt uiSizeX );
  Void xPredDy0Dx13 ( XPel*  pucDest, XPel*  pucSrc, Int iDestStride, Int iSrcStride, Int iDx, UInt uiSizeY, UInt uiSizeX );
  Void xPredDy0Dx2  ( XPel*  pucDest, XPel*  pucSrc, Int iDestStride, Int iSrcStride, UInt uiSizeY, UInt uiSizeX );
  Void xPredDx2     ( XXPel* psDest,  XPel*  pucSrc, Int iSrcStride, UInt uiSizeY,   UInt uiSizeX );

  static Void xXFilter1( XPel* pDes, XPel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize );
  static Void xXFilter2( XPel* pDes, XPel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize );
  static Void xXFilter3( XPel* pDes, XPel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize );
  static Void xXFilter4( XPel* pDes, XPel* pSrc, Int iSrcStride, UInt uiXSize, UInt uiYSize );

protected:
  XFilterBlockFunc m_afpXFilterBlockFunc[4];
};

H264AVC_NAMESPACE_END

#endif // !defined(AFX_INTERPOLATIONFILTER_H__79B49DF9_A41E_4043_AD0E_CAAC3A0A9DA1__INCLUDED_)
