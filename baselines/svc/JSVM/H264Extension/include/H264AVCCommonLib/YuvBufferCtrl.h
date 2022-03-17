
#if !defined(AFX_YUVBUFFERCTRL_H__CF27CE5A_C6CC_4DA0_9AE0_698B326B8C03__INCLUDED_)
#define AFX_YUVBUFFERCTRL_H__CF27CE5A_C6CC_4DA0_9AE0_698B326B8C03__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN

class H264AVCCOMMONLIB_API YuvBufferCtrl
{
public:
  class H264AVCCOMMONLIB_API YuvBufferParameter
  {
  public:
    YuvBufferParameter() {}
    const UInt getMbLum()   const { return m_uiLumOffset; }
    const UInt getMbCb()    const { return m_uiCbOffset; }
    const UInt getMbCr()    const { return m_uiCrOffset; }

    const Int getStride()   const { return m_iStride; }
    const Int getHeight()   const { return m_iHeight; }
    const Int getWidth()    const { return m_iWidth; }

    const UInt getYBlk( LumaIdx cIdx ) const    { return m_uiLumOffset +  ((cIdx.y() * m_iStride + cIdx.x())<<(2 + m_iResolution)); }
    const UInt getUBlk( LumaIdx cIdx ) const    { return m_uiCbOffset  +  ((cIdx.y() * m_iStride + (cIdx.x()<<1)) << m_iResolution); }
    const UInt getVBlk( LumaIdx cIdx ) const    { return m_uiCrOffset  +  ((cIdx.y() * m_iStride + (cIdx.x()<<1)) << m_iResolution); }

    UInt m_uiLumOffset;
    UInt m_uiCbOffset;
    UInt m_uiCrOffset;
    Int m_iStride;
    Int m_iResolution;
    Int m_iHeight;
    Int m_iWidth;
  };

protected:
	YuvBufferCtrl();
	virtual ~YuvBufferCtrl();

public:
  static ErrVal create( YuvBufferCtrl*& rpcYuvBufferCtrl );
  ErrVal destroy();
	const YuvBufferParameter& getBufferParameter( PicType ePicType=FRAME ) const { return m_acBufferParam[ePicType]; }
	ErrVal initMb( UInt uiMbY, UInt uiMbX, Bool bMbAff );
	ErrVal initMb() { return initMb( 0, 0, false ); }

  UInt getMbLum( PicType ePicType, UInt uiMbY, UInt uiMbX, Bool bMbAff ) const
  {
    UInt uiXPos = (uiMbX<<4) << m_iResolution;
    UInt uiYPos = (uiMbY<<4) << m_iResolution;

    if( ePicType == FRAME )
    {
      return m_uiLumBaseOffset + uiXPos + uiYPos * m_acBufferParam[FRAME].m_iStride;
    }

    if( bMbAff )
    {
      uiMbY >>= 1;
    }
    UInt uiYPosFld  = (uiMbY<<4) << m_iResolution;
    UInt uiTopOff = m_uiLumBaseOffset + uiXPos + uiYPosFld * m_acBufferParam[FRAME].m_iStride * 2;
    if( ePicType == TOP_FIELD )
    {
      return uiTopOff;
    }

    AOF( ePicType == BOT_FIELD );
    return uiTopOff + m_acBufferParam[FRAME].m_iStride;
  }

  UInt getMbCb( PicType ePicType, UInt uiMbY, UInt uiMbX, Bool bMbAff ) const
  {
    UInt uiXPos = (uiMbX<<3) << m_iResolution;
    UInt uiYPos = (uiMbY<<3) << m_iResolution;

    if( ePicType == FRAME )
    {
      return m_uiCbBaseOffset + uiXPos + uiYPos    * m_acBufferParam[FRAME].m_iStride / 2;
    }

    if( bMbAff )
    {
      uiMbY >>= 1;
    }
    UInt uiYPosFld  = (uiMbY<<3) << m_iResolution;
    UInt uiTopOff = m_uiCbBaseOffset + uiXPos + uiYPosFld * m_acBufferParam[FRAME].m_iStride;
    if( ePicType == TOP_FIELD )
    {
      return uiTopOff;
    }
    AOF( ePicType == BOT_FIELD );

    return uiTopOff + m_acBufferParam[FRAME].m_iStride / 2;
  }

  UInt getMbCr( PicType ePicType, UInt uiMbY, UInt uiMbX, Bool bMbAff ) const
  {
    UInt uiXPos = (uiMbX<<3) << m_iResolution;
    UInt uiYPos = (uiMbY<<3) << m_iResolution;

    if( ePicType == FRAME )
    {
      return m_uiCbBaseOffset + uiXPos + uiYPos * m_acBufferParam[FRAME].m_iStride / 2 + m_uiChromaSize;
    }

    if( bMbAff )
    {
      uiMbY >>= 1;
    }
    UInt uiYPosFld  = (uiMbY<<3) << m_iResolution;
    UInt uiTopOff = m_uiCbBaseOffset + uiXPos + uiYPosFld * m_acBufferParam[FRAME].m_iStride + m_uiChromaSize;
    if( ePicType == TOP_FIELD )
    {
      return uiTopOff;
    }
    AOF( ePicType == BOT_FIELD );
    return uiTopOff + m_acBufferParam[FRAME].m_iStride / 2;
  }

  ErrVal initSlice( UInt uiYFrameSize, UInt uiXFrameSize, UInt uiYMarginSize = 0, UInt uiXMarginSize = 0, UInt uiResolution = 0 );
  ErrVal initSPS( UInt uiYFrameSize, UInt uiXFrameSize, UInt uiYMarginSize = 0, UInt uiXMarginSize = 0, UInt uiResolution = 0 );
  ErrVal uninit();

  Bool isInitDone() { return m_bInitDone; }
  ErrVal getChromaSize( UInt& ruiChromaSize );
  const Int getXMargin()  const { return m_uiXMargin; }
  const Int getYMargin()  const { return m_uiYMargin; }

  UInt getLumOrigin ( PicType ePicType ) const { return m_uiLumBaseOffset + (( ePicType == BOT_FIELD) ? m_acBufferParam[FRAME].m_iStride   : 0); }
  UInt getCbOrigin  ( PicType ePicType ) const { return m_uiCbBaseOffset  + (( ePicType == BOT_FIELD) ? m_acBufferParam[FRAME].m_iStride/2 : 0); }
  UInt getCrOrigin  ( PicType ePicType ) const { return m_uiCbBaseOffset
		                                                     + m_uiChromaSize + (( ePicType == BOT_FIELD) ? m_acBufferParam[FRAME].m_iStride/2 : 0); }

protected:
	YuvBufferParameter m_acBufferParam[ MAX_FRAME_TYPE ];
  UInt  m_uiLumBaseOffset;
  UInt  m_uiCbBaseOffset;
  UInt  m_uiChromaSize;
  Int   m_iResolution;
  UInt m_uiXMargin;
  UInt m_uiYMargin;
  Bool m_bInitDone;
};



H264AVC_NAMESPACE_END


#endif // !defined(AFX_YUVBUFFERCTRL_H__CF27CE5A_C6CC_4DA0_9AE0_698B326B8C03__INCLUDED_)
