
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/YuvBufferCtrl.h"

H264AVC_NAMESPACE_BEGIN

YuvBufferCtrl::YuvBufferCtrl() :
  m_uiChromaSize  ( 0 ),
  m_iResolution   ( 0 ),
  m_bInitDone     ( false )
{
}

YuvBufferCtrl::~YuvBufferCtrl()
{
}

ErrVal YuvBufferCtrl::getChromaSize( UInt& ruiChromaSize )
{
  ROF( m_bInitDone );
  ruiChromaSize = m_uiChromaSize;
  return Err::m_nOK;
}

ErrVal YuvBufferCtrl::create( YuvBufferCtrl*& rpcYuvBufferCtrl )
{
  rpcYuvBufferCtrl = new YuvBufferCtrl;

  ROT( NULL == rpcYuvBufferCtrl );

  return Err::m_nOK;
}

ErrVal YuvBufferCtrl::destroy()
{
  delete this;

  return Err::m_nOK;
}

ErrVal YuvBufferCtrl::initMb( UInt uiMbY, UInt uiMbX, Bool bMbAff )
{
  ROF( m_bInitDone );

  UInt uiXPos     = (uiMbX<<3) << m_iResolution;
  UInt uiYPos     = (uiMbY<<3) << m_iResolution;

  if( bMbAff )
  {
    uiMbY >>= 1;
  }

  UInt uiYPosFld  = (uiMbY<<3) << m_iResolution;
  Int  iStride    = m_acBufferParam[FRAME].m_iStride;

  m_acBufferParam[FRAME]    .m_uiCbOffset = m_uiCbBaseOffset + uiXPos + uiYPos    * iStride / 2;
  m_acBufferParam[TOP_FIELD].m_uiCbOffset = m_uiCbBaseOffset + uiXPos + uiYPosFld * iStride;

  m_acBufferParam[FRAME]    .m_uiCrOffset = m_acBufferParam[FRAME]    .m_uiCbOffset + m_uiChromaSize;
  m_acBufferParam[TOP_FIELD].m_uiCrOffset = m_acBufferParam[TOP_FIELD].m_uiCbOffset + m_uiChromaSize;
  uiXPos    <<= 1;
  uiYPos    <<= 1;
  uiYPosFld <<= 1;

  m_acBufferParam[FRAME]    .m_uiLumOffset  = m_uiLumBaseOffset + uiXPos + uiYPos    * iStride;
  m_acBufferParam[TOP_FIELD].m_uiLumOffset  = m_uiLumBaseOffset + uiXPos + uiYPosFld * iStride * 2;

  m_acBufferParam[BOT_FIELD].m_uiLumOffset  = m_acBufferParam[TOP_FIELD].m_uiLumOffset + iStride;
  m_acBufferParam[BOT_FIELD].m_uiCbOffset   = m_acBufferParam[TOP_FIELD].m_uiCbOffset  + iStride / 2;
  m_acBufferParam[BOT_FIELD].m_uiCrOffset   = m_acBufferParam[TOP_FIELD].m_uiCrOffset  + iStride / 2;

  return Err::m_nOK;
}

ErrVal YuvBufferCtrl::initSlice( UInt uiYFrameSize, UInt uiXFrameSize, UInt uiYMarginSize, UInt uiXMarginSize, UInt uiResolution )
{
  ROT( 0 ==  uiYFrameSize );
  ROT( 0 != (uiYFrameSize&0xf) );
  ROT( 0 ==  uiXFrameSize );
  ROT( 0 != (uiXFrameSize&0xf) );
  ROT( 2 < uiResolution );
  ROT( 1 & uiXMarginSize );

  uiYFrameSize  <<= uiResolution;
  uiXFrameSize  <<= uiResolution;
  uiYMarginSize <<= uiResolution;
  uiXMarginSize <<= uiResolution;

  m_uiXMargin = uiXMarginSize;
  m_uiYMargin = uiYMarginSize/2;

  m_acBufferParam[FRAME].    m_iHeight = uiYFrameSize;
  m_acBufferParam[TOP_FIELD].m_iHeight =
  m_acBufferParam[BOT_FIELD].m_iHeight = uiYFrameSize >> 1;

  m_acBufferParam[FRAME].    m_iStride = uiXFrameSize   + 2*uiXMarginSize;
  m_acBufferParam[TOP_FIELD].m_iStride =
  m_acBufferParam[BOT_FIELD].m_iStride = 2*uiXFrameSize + 4*uiXMarginSize;

  m_acBufferParam[FRAME].    m_iWidth = uiXFrameSize;
  m_acBufferParam[TOP_FIELD].m_iWidth = uiXFrameSize;
  m_acBufferParam[BOT_FIELD].m_iWidth = uiXFrameSize;

  m_acBufferParam[FRAME].    m_iResolution = uiResolution;
  m_acBufferParam[TOP_FIELD].m_iResolution = uiResolution;
  m_acBufferParam[BOT_FIELD].m_iResolution = uiResolution;

  m_iResolution   = uiResolution;
  m_uiChromaSize  = ((uiYFrameSize >> 1) + uiYMarginSize)
                  * ((uiXFrameSize >> 1) + uiXMarginSize);

  m_uiLumBaseOffset = (m_acBufferParam[FRAME].m_iStride) * uiYMarginSize + uiXMarginSize;
  m_uiCbBaseOffset  = (m_acBufferParam[FRAME].m_iStride/2) * uiYMarginSize/2 + uiXMarginSize/2;
  m_uiCbBaseOffset += 4*m_uiChromaSize;
  m_bInitDone = true;

  return Err::m_nOK;
}

ErrVal YuvBufferCtrl::initSPS( UInt uiYFrameSize, UInt uiXFrameSize, UInt uiYMarginSize, UInt uiXMarginSize, UInt uiResolution )
{
  ROT( 0 ==  uiYFrameSize );
  ROT( 0 != (uiYFrameSize&0xf) );
  ROT( 0 ==  uiXFrameSize );
  ROT( 0 != (uiXFrameSize&0xf) );
  ROT( 2 < uiResolution );
  ROT( 1 & uiXMarginSize );

  uiYFrameSize  <<= uiResolution;
  uiXFrameSize  <<= uiResolution;
  uiYMarginSize <<= uiResolution;
  uiXMarginSize <<= uiResolution;

  m_uiXMargin = uiXMarginSize;
  m_uiYMargin = uiYMarginSize/2;

  m_acBufferParam[FRAME].    m_iHeight = uiYFrameSize;
  m_acBufferParam[TOP_FIELD].m_iHeight =
  m_acBufferParam[BOT_FIELD].m_iHeight = uiYFrameSize >> 1;

  m_acBufferParam[FRAME].    m_iStride = uiXFrameSize   + 2*uiXMarginSize;
  m_acBufferParam[TOP_FIELD].m_iStride =
  m_acBufferParam[BOT_FIELD].m_iStride = 2*uiXFrameSize + 4*uiXMarginSize;

  m_acBufferParam[FRAME].    m_iWidth = uiXFrameSize;
  m_acBufferParam[TOP_FIELD].m_iWidth = uiXFrameSize;
  m_acBufferParam[BOT_FIELD].m_iWidth = uiXFrameSize;

  m_acBufferParam[FRAME].    m_iResolution = uiResolution;
  m_acBufferParam[TOP_FIELD].m_iResolution = uiResolution;
  m_acBufferParam[BOT_FIELD].m_iResolution = uiResolution;

  m_iResolution   = uiResolution;
  m_uiChromaSize  = ((uiYFrameSize >> 1) + uiYMarginSize)
                  * ((uiXFrameSize >> 1) + uiXMarginSize);

  m_uiLumBaseOffset = (m_acBufferParam[FRAME].m_iStride) * uiYMarginSize + uiXMarginSize;
  m_uiCbBaseOffset  = (m_acBufferParam[FRAME].m_iStride/2) * uiYMarginSize/2 + uiXMarginSize/2;
  m_uiCbBaseOffset += 4*m_uiChromaSize;
  m_bInitDone = true;

  return Err::m_nOK;
}


ErrVal YuvBufferCtrl::uninit()
{
  m_bInitDone = false;
  return Err::m_nOK;
}



H264AVC_NAMESPACE_END
