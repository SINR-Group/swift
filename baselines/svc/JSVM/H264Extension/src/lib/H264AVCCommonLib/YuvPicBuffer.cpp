
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/YuvBufferCtrl.h"
#include "H264AVCCommonLib/YuvPicBuffer.h"
#include "H264AVCCommonLib/YuvMbBuffer.h"
#include "H264AVCCommonLib/MbDataCtrl.h"

#include <math.h>

H264AVC_NAMESPACE_BEGIN

YuvPicBuffer::YuvPicBuffer( YuvBufferCtrl& rcYuvBufferCtrl, PicType ePicType )
: m_rcBufferParam   ( rcYuvBufferCtrl.getBufferParameter( ePicType ) ),
  m_ePicType        ( ePicType ),
  m_rcYuvBufferCtrl ( rcYuvBufferCtrl ),
  m_pPelCurrY       ( NULL ),
  m_pPelCurrU       ( NULL ),
  m_pPelCurrV       ( NULL ),
  m_pucYuvBuffer    ( NULL ),
  m_pucOwnYuvBuffer ( NULL )
{
}



YuvPicBuffer::~YuvPicBuffer()
{
}



ErrVal
YuvPicBuffer::init( XPel*& rpucYuvBuffer )
{
  ROT( NULL != m_pucYuvBuffer );
  ROF( m_rcYuvBufferCtrl.isInitDone() )

  UInt uiSize;
  RNOK( m_rcYuvBufferCtrl.getChromaSize( uiSize ) );

  if( NULL == rpucYuvBuffer )
  {
    m_pucOwnYuvBuffer = new XPel[ 6 * uiSize ];
	  ::memset( m_pucOwnYuvBuffer, 0x00, (6 * uiSize)*sizeof(XPel) );
    ROT( NULL == m_pucOwnYuvBuffer );
    rpucYuvBuffer = m_pucYuvBuffer = m_pucOwnYuvBuffer;
  }
  else
  {
   //TMM_INTERLACE
    if( m_pucOwnYuvBuffer) delete [] m_pucOwnYuvBuffer;

    m_pucOwnYuvBuffer = NULL;
    m_pucYuvBuffer = rpucYuvBuffer;
  }

  m_pucYuvBuffer[(6 * uiSize)-1] = 0xde;

  m_rcYuvBufferCtrl.initMb();

  return Err::m_nOK;
}



ErrVal
YuvPicBuffer::loadFromPicBuffer( PicBuffer* pcPicBuffer )
{
  ROF( pcPicBuffer );
  Pel* pSrc = pcPicBuffer->getBuffer();
  ROF( pSrc );

  UInt uiSize;
  RNOK( m_rcYuvBufferCtrl.getChromaSize( uiSize ) );
  uiSize *= 6;

  for( UInt ui = 0; ui < uiSize; ui++ )
  {
		m_pucYuvBuffer[ui]  = (XPel)pSrc[ui];
  }

  return Err::m_nOK;
}



ErrVal
YuvPicBuffer::clear()
{
  UInt uiSize;
  RNOK( m_rcYuvBufferCtrl.getChromaSize( uiSize ) );
  uiSize *= 6;

  for( UInt ui = 0; ui < uiSize; ui++ )
  {
    m_pucYuvBuffer[ui] = 0;
  }

  return Err::m_nOK;
}


ErrVal
YuvPicBuffer::clearCurrMb()
{
  XPel *pData = getMbLumAddr();

  Int iStride = getLStride();
  UInt y=0;

  for( y=0; y<16; y++ )
  {
    ::memset( pData, 0, sizeof(XPel)*16 );
    pData += iStride;
  }

  pData = getMbCbAddr();
  iStride = getCStride();
  for( y=0; y<8; y++ )
  {
    ::memset( pData, 0, sizeof(XPel)*8 );
    pData += iStride;
  }

  pData = getMbCrAddr();
  for( y=0; y<8; y++ )
  {
    ::memset( pData, 0, sizeof(XPel)*8 );
    pData += iStride;
  }
  return Err::m_nOK;
}



ErrVal YuvPicBuffer::getSSD( Double& dSSDY, Double& dSSDU, Double& dSSDV, PicBuffer* pcOrgPicBuffer )
{
  ROF( pcOrgPicBuffer );
  Pel*  pOrgBase  = pcOrgPicBuffer->getBuffer();
  ROF( pOrgBase );
  XPel* pSrcBase  = m_pucYuvBuffer;
  Int   iStride   = getLStride();
  UInt  uiHeight  = getLHeight();
  UInt  uiWidth   = getLWidth ();
  UInt  y, x;

  m_rcYuvBufferCtrl.initMb();

  XPel*   pSrc    = getMbLumAddr();
  Pel*    pOrg    = pOrgBase + ( pSrc - pSrcBase );
  Double  dDiff;

  dSSDY = 0;
  dSSDU = 0;
  dSSDV = 0;

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      dDiff  = (Double)( pOrg[x] - gMin( 255, gMax( 0, pSrc[x] ) ) );
      dSSDY += dDiff * dDiff;
    }
    pSrc += iStride;
    pOrg += iStride;
  }

  iStride   >>= 1;
  uiHeight  >>= 1;
  uiWidth   >>= 1;
  pSrc        = getMbCbAddr();
  pOrg        = pOrgBase + ( pSrc - pSrcBase );

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      dDiff  = (Double)( pOrg[x] - gMin( 255, gMax( 0, pSrc[x] ) ) );
      dSSDU += dDiff * dDiff;
    }
    pSrc += iStride;
    pOrg += iStride;
  }

  pSrc        = getMbCrAddr();
  pOrg        = pOrgBase + ( pSrc - pSrcBase );

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      dDiff  = (Double)( pOrg[x] - gMin( 255, gMax( 0, pSrc[x] ) ) );
      dSSDV += dDiff * dDiff;
    }
    pSrc += iStride;
    pOrg += iStride;
  }

  return Err::m_nOK;
}


ErrVal YuvPicBuffer::storeToPicBuffer( PicBuffer* pcPicBuffer )
{
  ROF( pcPicBuffer );
  Pel*  pDesBase  = pcPicBuffer->getBuffer();
  ROF( pDesBase );
  XPel* pSrcBase  = m_pucYuvBuffer;
  Int   iStride   = getLStride();
  UInt  uiHeight  = getLHeight();
  UInt  uiWidth   = getLWidth ();
  UInt  y, x;

  m_rcYuvBufferCtrl.initMb();

  XPel* pSrc      = getMbLumAddr();
  Pel*  pDes      = pDesBase + ( pSrc - pSrcBase );

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = (UChar)( gMin( 255, gMax( 0, pSrc[x] ) ) );
    }
    pSrc += iStride;
    pDes += iStride;
  }

  iStride   >>= 1;
  uiHeight  >>= 1;
  uiWidth   >>= 1;
  pSrc        = getMbCbAddr();
  pDes        = pDesBase + ( pSrc - pSrcBase );

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = (UChar)( gMin( 255, gMax( 0, pSrc[x] ) ) );
    }
    pSrc += iStride;
    pDes += iStride;
  }

  pSrc        = getMbCrAddr();
  pDes        = pDesBase + ( pSrc - pSrcBase );

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = (UChar)( gMin( 255, gMax( 0, pSrc[x] ) ) );
    }
    pSrc += iStride;
    pDes += iStride;
  }

  return Err::m_nOK;
}

ErrVal
YuvPicBuffer::interpolatedPicBuffer( PicBuffer* pcPicBuffer, Bool bBotField )
{
  ROF( pcPicBuffer );
  ROF( pcPicBuffer->getBuffer() );
  UInt y, x;

  m_rcYuvBufferCtrl.initMb();

  for( Int iComp = 0; iComp < 3; iComp++ )
  {
    Int   iStride   = ( iComp ? getCStride() : getLStride() );
    UInt  uiHeight  = ( iComp ? getCHeight() : getLHeight() ) / 2 - 1;
    UInt  uiWidth   = ( iComp ? getCWidth () : getLWidth () );
    UInt  uiOffset  = UInt( ( iComp == 0 ? getMbLumAddr() : iComp == 1 ? getMbCbAddr() : getMbCrAddr() ) - m_pucYuvBuffer );
    Pel*  pDes      = pcPicBuffer->getBuffer() + uiOffset + ( bBotField ? iStride : 0 );
    if( ! bBotField )
    {
      memcpy( pDes, pDes + iStride, uiWidth*sizeof(Pel) );
      pDes += 2*iStride;
    }
    for( y = 0; y < uiHeight; y++, pDes += 2*iStride )
    {
      Pel*  pSrc0 = pDes - iStride;
      Pel*  pSrc1 = pDes + iStride;
      for( x = 0; x < uiWidth; x++ )
      {
        pDes[x] = ( pSrc0[x] + pSrc1[x] ) >> 1;
      }
    }
    if( bBotField )
    {
      memcpy( pDes, pDes - iStride, uiWidth*sizeof(Pel) );
    }
  }

  return Err::m_nOK;
}

ErrVal YuvPicBuffer::loadFromFile8Bit( FILE* pFILE )
{
  m_rcYuvBufferCtrl.initMb();

  XPel*   pPel      = getMbLumAddr();
  Int     iStride   = getLStride();
  UInt    uiHeight  = getLHeight();
  UInt    uiWidth   = getLWidth ();
  UInt    y, x;

  UChar*  pTBuffer  = new UChar[ uiWidth ];
  ROF( pTBuffer );

  for( y = 0; y < uiHeight; y++ )
  {
    ROT( uiWidth != ::fread( pTBuffer, sizeof(UChar), uiWidth, pFILE ) );
    for( x = 0; x < uiWidth; x++ )
    {
      pPel[x] = (XPel)pTBuffer[x];
    }
    pPel += iStride;
  }

  iStride   >>= 1;
  uiHeight  >>= 1;
  uiWidth   >>= 1;
  pPel        = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    ROT( uiWidth != ::fread( pTBuffer, sizeof(UChar), uiWidth, pFILE ) );
    for( x = 0; x < uiWidth; x++ )
    {
      pPel[x] = (XPel)pTBuffer[x];
    }
    pPel += iStride;
  }

  pPel        = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    ROT( uiWidth != ::fread( pTBuffer, sizeof(UChar), uiWidth, pFILE ) );
    for( x = 0; x < uiWidth; x++ )
    {
      pPel[x] = (XPel)pTBuffer[x];
    }
    pPel += iStride;
  }

  delete [] pTBuffer;

  return Err::m_nOK;
}


ErrVal YuvPicBuffer::uninit()
{
  if( m_pucOwnYuvBuffer )
  {
    delete [] m_pucOwnYuvBuffer;
  }
  m_pucYuvBuffer    = NULL;
  m_pucOwnYuvBuffer = NULL;
  m_pPelCurrY       = NULL;
  m_pPelCurrU       = NULL;
  m_pPelCurrV       = NULL;
  return Err::m_nOK;
}


ErrVal YuvPicBuffer::loadBuffer( YuvMbBuffer *pcYuvMbBuffer, Int iMbXOffset, Int iMbYOffset )
{
  Int   iSrcStride  = pcYuvMbBuffer->getLStride();
  Int   iDesStride  = getLStride();
  XPel* pDes        = getMbLumAddr() + iMbXOffset * 16 + iMbYOffset * 16 * iDesStride;
  XPel* pScr        = pcYuvMbBuffer->getMbLumAddr();
  UInt  y;

  for( y = 0; y < 16; y++ )
  {
    memcpy( pDes, pScr, 16* sizeof(XPel) );
    pDes += iDesStride,
    pScr += iSrcStride;
  }

  iDesStride  >>= 1;
  iSrcStride  = pcYuvMbBuffer->getCStride();
  pScr = pcYuvMbBuffer->getMbCbAddr();
  pDes = getMbCbAddr() + iMbXOffset * 8 + iMbYOffset * 8 * iDesStride;

  for( y = 0; y < 8; y++ )
  {
    memcpy( pDes, pScr, 8* sizeof(XPel) );
    pDes += iDesStride,
    pScr += iSrcStride;
  }

  pScr = pcYuvMbBuffer->getMbCrAddr();
  pDes = getMbCrAddr() + iMbXOffset * 8 + iMbYOffset * 8 * iDesStride;

  for( y = 0; y < 8; y++ )
  {
    memcpy( pDes, pScr, 8* sizeof(XPel) );
    pDes += iDesStride,
    pScr += iSrcStride;
  }

  return Err::m_nOK;
}




//TMM_INTERLACE {
ErrVal YuvPicBuffer::loadBuffer_MbAff( YuvMbBuffer *pcYuvMbBuffer, UInt uiMask )
{
  Int   y;
  Bool  bTopIntra     = ( ( uiMask & 0x020 ) != 0 );
  Bool  bBotIntra     = ( ( uiMask & 0x040 ) != 0 );
  Int   iYSizeLuma    = ( bTopIntra || bBotIntra ? 8 : 16 );
  Int   iYSizeChroma  = iYSizeLuma >> 1;

  Int   iSrcStride    = pcYuvMbBuffer->getLStride();
  Int   iDesStride    = getLStride();
  XPel* pSrc          = pcYuvMbBuffer->getMbLumAddr() + ( bTopIntra ? iYSizeLuma * iSrcStride : 0 );
  XPel* pDes          = getMbLumAddr()                + ( bTopIntra ? iYSizeLuma * iDesStride : 0 );

  for( y = 0; y < iYSizeLuma; y++ )
  {
    memcpy( pDes, pSrc, 16* sizeof(XPel) );
    pDes += iDesStride;
    pSrc += iSrcStride;
  }

  iSrcStride  = pcYuvMbBuffer->getCStride();
  iDesStride  = getCStride();
  pSrc        = pcYuvMbBuffer->getMbCbAddr() + ( bTopIntra ? iYSizeChroma * iSrcStride : 0 );
  pDes        = getMbCbAddr()                + ( bTopIntra ? iYSizeChroma * iDesStride : 0 );

  for( y = 0; y < iYSizeChroma; y++ )
  {
    memcpy( pDes, pSrc, 8* sizeof(XPel) );
    pDes += iDesStride;
    pSrc += iSrcStride;
  }

  pSrc  = pcYuvMbBuffer->getMbCrAddr() + ( bTopIntra ? iYSizeChroma * iSrcStride : 0 );
  pDes  = getMbCrAddr()                + ( bTopIntra ? iYSizeChroma * iDesStride : 0 );

  for( y = 0; y < iYSizeChroma; y++ )
  {
    memcpy( pDes, pSrc, 8* sizeof(XPel) );
    pDes += iDesStride;
    pSrc += iSrcStride;
  }

  return Err::m_nOK;
}


//TMM_INTERLACE }
//JVT-X046 {
  void	YuvPicBuffer::setMBZero( UInt uiMBY, UInt uiMBX )
	{
		m_rcYuvBufferCtrl.initMb(uiMBY,uiMBX,false);
		Int iDesStride = getLStride();
		XPel* pDes = getMbLumAddr();
		UInt y,x;

		for ( y = 0; y < 16; y++ )
		{
			for ( x = 0; x < 16; x++ )
				pDes[x] = 0;
			pDes += iDesStride;
		}
		iDesStride >>= 1;

		pDes = getMbCbAddr();
		for ( y = 0; y < 8; y++ )
		{
			for ( x = 0; x < 8; x++ )
				pDes[x] = 0;
			pDes += iDesStride;
		}

		pDes = getMbCrAddr();
		for ( y = 0; y < 8; y++ )
		{
			for ( x = 0; x < 8; x++)
				pDes[x] = 0;
			pDes += iDesStride;
		}
		m_rcYuvBufferCtrl.initMb();
	}
  ErrVal YuvPicBuffer::predictionSlices(YuvPicBuffer*  pcSrcYuvPicBuffer, YuvPicBuffer*  pcMCPYuvPicBuffer, UInt uiMbY, UInt uiMbX )
  {
	  pcSrcYuvPicBuffer->m_rcYuvBufferCtrl.initMb(uiMbY,uiMbX,false);
	  pcMCPYuvPicBuffer->m_rcYuvBufferCtrl.initMb(uiMbY,uiMbX,false);
	  m_rcYuvBufferCtrl.initMb(uiMbY,uiMbX,false);
	  XPel* pSrc = pcSrcYuvPicBuffer->getMbLumAddr();
	  XPel* pMCP = pcMCPYuvPicBuffer->getMbLumAddr();
	  XPel* pDes        = getMbLumAddr();
	  Int   iSrcStride  = pcSrcYuvPicBuffer->getLStride();
	  Int   iMCPStride  = pcMCPYuvPicBuffer->getLStride();
	  Int   iDesStride  = getLStride();
	  UInt x,y;
	  //===== luminance =====
	  for( y = 0; y < 16; y++ )
	  {
		  for (x = 0; x < 16; x++)
		  {
			  pDes[x] = pSrc[x] - pMCP[x];
		  }
		  pSrc  += iSrcStride;
		  pMCP  += iMCPStride;
		  pDes  += iDesStride;
	  }

	  //===== chrominance U =====
	  iSrcStride  >>= 1;
	  iMCPStride  >>= 1;
	  iDesStride  >>= 1;
	  pSrc          = pcSrcYuvPicBuffer->getMbCbAddr();
	  pMCP          = pcMCPYuvPicBuffer->getMbCbAddr();
	  pDes          = getMbCbAddr();

	  for( y = 0; y < 8; y++ )
	  {
		  for( x = 0; x < 8; x++ )
		  {
			  pDes[x] = pSrc[x] - pMCP[x];
		  }
		  pSrc  += iSrcStride;
		  pMCP  += iMCPStride;
		  pDes  += iDesStride;
	  }
	  //===== chrominance V =====
	  pSrc          = pcSrcYuvPicBuffer->getMbCrAddr();
	  pMCP          = pcMCPYuvPicBuffer->getMbCrAddr();
	  pDes          = getMbCrAddr();

	  for( y = 0; y < 8; y++ )
	  {
		  for( x = 0; x < 8; x++ )
		  {
			  pDes[x] = pSrc[x] - pMCP[x];
		  }
		  pSrc  += iSrcStride;
		  pMCP  += iMCPStride;
		  pDes  += iDesStride;
	  }
	  return Err::m_nOK;
  }
  ErrVal YuvPicBuffer::inversepredictionSlices(YuvPicBuffer*  pcSrcYuvPicBuffer, YuvPicBuffer*  pcMCPYuvPicBuffer, UInt uiMbY, UInt uiMbX )
  {
	  pcSrcYuvPicBuffer->m_rcYuvBufferCtrl.initMb(uiMbY,uiMbX,false);
	  pcMCPYuvPicBuffer->m_rcYuvBufferCtrl.initMb(uiMbY,uiMbX,false);
	  m_rcYuvBufferCtrl.initMb(uiMbY,uiMbX,false);
	  XPel* pSrc = pcSrcYuvPicBuffer->getMbLumAddr();
	  XPel* pMCP = pcMCPYuvPicBuffer->getMbLumAddr();
	  XPel* pDes        = getMbLumAddr();
	  Int   iSrcStride  = pcSrcYuvPicBuffer->getLStride();
	  Int   iMCPStride  = pcMCPYuvPicBuffer->getLStride();
	  Int   iDesStride  = getLStride();
	  UInt x,y;
	  //===== luminance =====
	  for( y = 0; y < 16; y++ )
	  {
		  for (x = 0; x < 16; x++)
		  {
			  pDes[x] = pSrc[x] + pMCP[x];
		  }
		  pSrc  += iSrcStride;
		  pMCP  += iMCPStride;
		  pDes  += iDesStride;
	  }

	  //===== chrominance U =====
	  iSrcStride  >>= 1;
	  iMCPStride  >>= 1;
	  iDesStride  >>= 1;
	  pSrc          = pcSrcYuvPicBuffer->getMbCbAddr();
	  pMCP          = pcMCPYuvPicBuffer->getMbCbAddr();
	  pDes          = getMbCbAddr();

	  for( y = 0; y < 8; y++ )
	  {
		  for( x = 0; x < 8; x++ )
		  {
			  pDes[x] = pSrc[x] + pMCP[x];
		  }
		  pSrc  += iSrcStride;
		  pMCP  += iMCPStride;
		  pDes  += iDesStride;
	  }
	  //===== chrominance V =====
	  pSrc          = pcSrcYuvPicBuffer->getMbCrAddr();
	  pMCP          = pcMCPYuvPicBuffer->getMbCrAddr();
	  pDes          = getMbCrAddr();

	  for( y = 0; y < 8; y++ )
	  {
		  for( x = 0; x < 8; x++ )
		  {
			  pDes[x] = pSrc[x] + pMCP[x];
		  }
		  pSrc  += iSrcStride;
		  pMCP  += iMCPStride;
		  pDes  += iDesStride;
	  }
	  return Err::m_nOK;
  }
  ErrVal YuvPicBuffer::copyMb(YuvPicBuffer* pcSrcYuvPicBuffer,UInt uiMbY, UInt uiMbX)
  {
	  pcSrcYuvPicBuffer->m_rcYuvBufferCtrl.initMb(uiMbY,uiMbX,false);
	  m_rcYuvBufferCtrl.initMb(uiMbY,uiMbX,false);
	  XPel* pSrc = pcSrcYuvPicBuffer->getMbLumAddr();
	  XPel* pDes        = getMbLumAddr();
	  Int   iSrcStride  = pcSrcYuvPicBuffer->getLStride();
	  Int   iDesStride  = getLStride();
	  UInt x,y;
	  //===== luminance =====
	  for( y = 0; y < 16; y++ )
	  {
		  for (x = 0; x < 16; x++)
		  {
			  pDes[x] = pSrc[x];
		  }
		  pSrc  += iSrcStride;
		  pDes  += iDesStride;
	  }

	  //===== chrominance U =====
	  iSrcStride  >>= 1;
	  iDesStride  >>= 1;
	  pSrc          = pcSrcYuvPicBuffer->getMbCbAddr();
	  pDes          = getMbCbAddr();

	  for( y = 0; y < 8; y++ )
	  {
		  for( x = 0; x < 8; x++ )
		  {
			  pDes[x] = pSrc[x];
		  }
		  pSrc  += iSrcStride;
		  pDes  += iDesStride;
	  }
	  //===== chrominance V =====
	  pSrc          = pcSrcYuvPicBuffer->getMbCrAddr();
	  pDes          = getMbCrAddr();

	  for( y = 0; y < 8; y++ )
	  {
		  for( x = 0; x < 8; x++ )
		  {
			  pDes[x] = pSrc[x];
		  }
		  pSrc  += iSrcStride;
		  pDes  += iDesStride;
	  }
	  return Err::m_nOK;
  }
//JVT-X046 }
ErrVal YuvPicBuffer::prediction( YuvPicBuffer*  pcSrcYuvPicBuffer,
                                    YuvPicBuffer*  pcMCPYuvPicBuffer )
{
  pcSrcYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  pcMCPYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  m_rcYuvBufferCtrl.initMb();

  XPel* pSrc        = pcSrcYuvPicBuffer->getMbLumAddr();
  XPel* pMCP        = pcMCPYuvPicBuffer->getMbLumAddr();
  XPel* pDes        = getMbLumAddr();
  Int   iSrcStride  = pcSrcYuvPicBuffer->getLStride();
  Int   iMCPStride  = pcMCPYuvPicBuffer->getLStride();
  Int   iDesStride  = getLStride();
  UInt  uiHeight    = getLHeight();
  UInt  uiWidth     = getLWidth ();
  UInt  y, x;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = pSrc[x] - pMCP[x];
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  //===== chrominance U =====
  iSrcStride  >>= 1;
  iMCPStride  >>= 1;
  iDesStride  >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pSrc          = pcSrcYuvPicBuffer->getMbCbAddr();
  pMCP          = pcMCPYuvPicBuffer->getMbCbAddr();
  pDes          = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = pSrc[x] - pMCP[x];
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  //===== chrominance V =====
  pSrc          = pcSrcYuvPicBuffer->getMbCrAddr();
  pMCP          = pcMCPYuvPicBuffer->getMbCrAddr();
  pDes          = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = pSrc[x] - pMCP[x];
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  return Err::m_nOK;
}



ErrVal YuvPicBuffer::update( YuvPicBuffer*  pcSrcYuvPicBuffer,
                                YuvPicBuffer*  pcMCPYuvPicBuffer,
                                UInt              uiShift )
{
  pcSrcYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  pcMCPYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  m_rcYuvBufferCtrl.initMb();

  XPel* pSrc        = pcSrcYuvPicBuffer->getMbLumAddr();
  XPel* pMCP        = pcMCPYuvPicBuffer->getMbLumAddr();
  XPel* pDes        = getMbLumAddr();
  Int   iSrcStride  = pcSrcYuvPicBuffer->getLStride();
  Int   iMCPStride  = pcMCPYuvPicBuffer->getLStride();
  Int   iDesStride  = getLStride();
  UInt  uiHeight    = getLHeight();
  UInt  uiWidth     = getLWidth ();
  UInt  y, x;

  //UInt  uiShift     = 1;
  XPel  pAdd = 0;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] + ( ( pMCP[x] + pAdd ) >> uiShift ) );
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  //===== chrominance U =====
  iSrcStride  >>= 1;
  iMCPStride  >>= 1;
  iDesStride  >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pSrc          = pcSrcYuvPicBuffer->getMbCbAddr();
  pMCP          = pcMCPYuvPicBuffer->getMbCbAddr();
  pDes          = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] + ( ( pMCP[x] + pAdd ) >> uiShift ) );
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  //===== chrominance V =====
  pSrc          = pcSrcYuvPicBuffer->getMbCrAddr();
  pMCP          = pcMCPYuvPicBuffer->getMbCrAddr();
  pDes          = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] + ( ( pMCP[x] + pAdd ) >> uiShift ) );
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  return Err::m_nOK;
}


ErrVal YuvPicBuffer::clip()
{
  m_rcYuvBufferCtrl.initMb();

  XPel* pDes        = getMbLumAddr();
  Int   iDesStride  = getLStride();
  UInt  uiHeight    = getLHeight();
  UInt  uiWidth     = getLWidth ();
  UInt  y, x;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pDes[x] );
    }
    pDes  += iDesStride;
  }

  //===== chrominance U =====
  iDesStride  >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pDes          = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pDes[x] );
    }
    pDes  += iDesStride;
  }

  //===== chrominance V =====
  pDes          = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pDes[x] );
    }
    pDes  += iDesStride;
  }

  return Err::m_nOK;
}




ErrVal YuvPicBuffer::subtract( YuvPicBuffer*  pcSrcYuvPicBuffer0,
                                  YuvPicBuffer*  pcSrcYuvPicBuffer1 )
{
  pcSrcYuvPicBuffer0->m_rcYuvBufferCtrl.initMb();
  pcSrcYuvPicBuffer1->m_rcYuvBufferCtrl.initMb();
  m_rcYuvBufferCtrl.initMb();

  XPel* pSrc0       = pcSrcYuvPicBuffer0->getMbLumAddr();
  XPel* pSrc1       = pcSrcYuvPicBuffer1->getMbLumAddr();
  XPel* pDes        = getMbLumAddr();
  Int   iSrc0Stride = pcSrcYuvPicBuffer0->getLStride();
  Int   iSrc1Stride = pcSrcYuvPicBuffer1->getLStride();
  Int   iDesStride  = getLStride();
  UInt  uiHeight    = getLHeight();
  UInt  uiWidth     = getLWidth ();
  UInt  y, x;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = pSrc0[x] - pSrc1[x];
    }
    pSrc0 += iSrc0Stride;
    pSrc1 += iSrc1Stride;
    pDes  += iDesStride;
  }

  //===== chrominance U =====
  iSrc0Stride >>= 1;
  iSrc1Stride >>= 1;
  iDesStride  >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pSrc0         = pcSrcYuvPicBuffer0->getMbCbAddr();
  pSrc1         = pcSrcYuvPicBuffer1->getMbCbAddr();
  pDes          = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = pSrc0[x] - pSrc1[x];
    }
    pSrc0 += iSrc0Stride;
    pSrc1 += iSrc1Stride;
    pDes  += iDesStride;
  }

  //===== chrominance V =====
  pSrc0         = pcSrcYuvPicBuffer0->getMbCrAddr();
  pSrc1         = pcSrcYuvPicBuffer1->getMbCrAddr();
  pDes          = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = pSrc0[x] - pSrc1[x];
    }
    pSrc0 += iSrc0Stride;
    pSrc1 += iSrc1Stride;
    pDes  += iDesStride;
  }

  return Err::m_nOK;
}


ErrVal YuvPicBuffer::add( YuvPicBuffer*  pcSrcYuvPicBuffer )
{
  pcSrcYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  m_rcYuvBufferCtrl.initMb();

  XPel* pSrc        = pcSrcYuvPicBuffer->getMbLumAddr();
  XPel* pDes        = getMbLumAddr();
  Int   iSrcStride  = pcSrcYuvPicBuffer->getLStride();
  Int   iDesStride  = getLStride();
  UInt  uiHeight    = pcSrcYuvPicBuffer->getLHeight();
  UInt  uiWidth     = pcSrcYuvPicBuffer->getLWidth ();
  UInt  y, x;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] += pSrc[x];
    }
    pSrc  += iSrcStride;
    pDes  += iDesStride;
  }

  //===== chrominance U =====
  iSrcStride  >>= 1;
  iDesStride  >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pSrc          = pcSrcYuvPicBuffer->getMbCbAddr();
  pDes          = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] += pSrc[x];
    }
    pSrc  += iSrcStride;
    pDes  += iDesStride;
  }

  //===== chrominance V =====
  pSrc          = pcSrcYuvPicBuffer->getMbCrAddr();
  pDes          = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] += pSrc[x];
    }
    pSrc  += iSrcStride;
    pDes  += iDesStride;
  }

  return Err::m_nOK;
}



ErrVal YuvPicBuffer::addWeighted( YuvPicBuffer* pcSrcYuvPicBuffer,
                                     Double           dWeight )
{
  pcSrcYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  m_rcYuvBufferCtrl.initMb();

  XPel* pSrc        = pcSrcYuvPicBuffer->getMbLumAddr();
  XPel* pDes        = getMbLumAddr();
  Int   iSrcStride  = pcSrcYuvPicBuffer->getLStride();
  Int   iDesStride  = getLStride();
  UInt  uiHeight    = pcSrcYuvPicBuffer->getLHeight();
  UInt  uiWidth     = pcSrcYuvPicBuffer->getLWidth ();
  UInt  y, x;
  Int   iWeightT, iWeightS;

  iWeightS = (Int) (dWeight * 256 + 0.5);
  iWeightT = 256 - iWeightS;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = ( pDes[x] * iWeightT + pSrc[x] * iWeightS + 128) >> 8;
//      pDes[x] += pSrc[x];
    }
    pSrc  += iSrcStride;
    pDes  += iDesStride;
  }

  //===== chrominance U =====
  iSrcStride  >>= 1;
  iDesStride  >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pSrc          = pcSrcYuvPicBuffer->getMbCbAddr();
  pDes          = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = ( pDes[x] * iWeightT + pSrc[x] * iWeightS + 128) >> 8;
//      pDes[x] += pSrc[x];
    }
    pSrc  += iSrcStride;
    pDes  += iDesStride;
  }

  //===== chrominance V =====
  pSrc          = pcSrcYuvPicBuffer->getMbCrAddr();
  pDes          = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = ( pDes[x] * iWeightT + pSrc[x] * iWeightS + 128) >> 8;
//      pDes[x] += pSrc[x];
    }
    pSrc  += iSrcStride;
    pDes  += iDesStride;
  }

  return Err::m_nOK;
}



ErrVal YuvPicBuffer::inverseUpdate( YuvPicBuffer*  pcSrcYuvPicBuffer,
                                       YuvPicBuffer*  pcMCPYuvPicBuffer,
                                       UInt              uiShift )
{
  pcSrcYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  pcMCPYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  m_rcYuvBufferCtrl.initMb();

  XPel* pSrc        = pcSrcYuvPicBuffer->getMbLumAddr();
  XPel* pMCP        = pcMCPYuvPicBuffer->getMbLumAddr();
  XPel* pDes        = getMbLumAddr();
  Int   iSrcStride  = pcSrcYuvPicBuffer->getLStride();
  Int   iMCPStride  = pcMCPYuvPicBuffer->getLStride();
  Int   iDesStride  = getLStride();
  UInt  uiHeight    = getLHeight();
  UInt  uiWidth     = getLWidth ();
  UInt  y, x;

  //UInt  uiShift     = 1;
  XPel  pAdd = 0;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] - ( ( pMCP[x] + pAdd ) >> uiShift ) );
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  //===== chrominance U =====
  iSrcStride  >>= 1;
  iMCPStride  >>= 1;
  iDesStride  >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pSrc          = pcSrcYuvPicBuffer->getMbCbAddr();
  pMCP          = pcMCPYuvPicBuffer->getMbCbAddr();
  pDes          = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] - ( ( pMCP[x] + pAdd ) >> uiShift ) );
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  //===== chrominance V =====
  pSrc          = pcSrcYuvPicBuffer->getMbCrAddr();
  pMCP          = pcMCPYuvPicBuffer->getMbCrAddr();
  pDes          = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] - ( ( pMCP[x] + pAdd ) >> uiShift ) );
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  return Err::m_nOK;
}



ErrVal YuvPicBuffer::inversePrediction( YuvPicBuffer*  pcSrcYuvPicBuffer,
                                           YuvPicBuffer*  pcMCPYuvPicBuffer )
{
  pcSrcYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  pcMCPYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  m_rcYuvBufferCtrl.initMb();

  XPel* pSrc        = pcSrcYuvPicBuffer->getMbLumAddr();
  XPel* pMCP        = pcMCPYuvPicBuffer->getMbLumAddr();
  XPel* pDes        = getMbLumAddr();
  Int   iSrcStride  = pcSrcYuvPicBuffer->getLStride();
  Int   iMCPStride  = pcMCPYuvPicBuffer->getLStride();
  Int   iDesStride  = getLStride();
  UInt  uiHeight    = getLHeight();
  UInt  uiWidth     = getLWidth ();
  UInt  y, x;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] + pMCP[x] );
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  //===== chrominance U =====
  iSrcStride  >>= 1;
  iMCPStride  >>= 1;
  iDesStride  >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pSrc          = pcSrcYuvPicBuffer->getMbCbAddr();
  pMCP          = pcMCPYuvPicBuffer->getMbCbAddr();
  pDes          = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] + pMCP[x] );
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  //===== chrominance V =====
  pSrc          = pcSrcYuvPicBuffer->getMbCrAddr();
  pMCP          = pcMCPYuvPicBuffer->getMbCrAddr();
  pDes          = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] + pMCP[x] );
    }
    pSrc  += iSrcStride;
    pMCP  += iMCPStride;
    pDes  += iDesStride;
  }

  return Err::m_nOK;
}




ErrVal YuvPicBuffer::copy( YuvPicBuffer*  pcSrcYuvPicBuffer )
{
  pcSrcYuvPicBuffer->m_rcYuvBufferCtrl.initMb();
  m_rcYuvBufferCtrl.initMb();

  XPel* pSrc        = pcSrcYuvPicBuffer->getMbLumAddr();
  XPel* pDes        = getMbLumAddr();
  Int   iSrcStride  = pcSrcYuvPicBuffer->getLStride();
  Int   iDesStride  = getLStride();
  UInt  uiHeight    = gMin( pcSrcYuvPicBuffer->getLHeight(), getLHeight() );
  UInt  uiWidth     = gMin( pcSrcYuvPicBuffer->getLWidth (), getLWidth () );
  UInt  y, x;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = pSrc[x];
    }
    pSrc  += iSrcStride;
    pDes  += iDesStride;
  }

  //===== chrominance U =====
  iSrcStride  >>= 1;
  iDesStride  >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pSrc          = pcSrcYuvPicBuffer->getMbCbAddr();
  pDes          = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = pSrc[x];
    }
    pSrc  += iSrcStride;
    pDes  += iDesStride;
  }

  //===== chrominance V =====
  pSrc          = pcSrcYuvPicBuffer->getMbCrAddr();
  pDes          = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = pSrc[x];
    }
    pSrc  += iSrcStride;
    pDes  += iDesStride;
  }

  return Err::m_nOK;
}

ErrVal YuvPicBuffer::dumpLPS( FILE* pFile )
{
  UChar*  pChar     = new UChar [ getLWidth() ];
  ROF( pChar );

  m_rcYuvBufferCtrl.initMb();

  XPel*   pPel      = getMbLumAddr();
  Int     iStride   = getLStride();
  UInt    uiHeight  = getLHeight();
  UInt    uiWidth   = getLWidth ();
  UInt    y, x;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pChar[x] = (UChar)( gMax( (Int)0, gMin( (Int)255, pPel[x] ) ) );
    }
    pPel += iStride;
    ROF( uiWidth == ::fwrite( pChar, sizeof(UChar), uiWidth, pFile ) );
  }

  //===== chrominance U =====
  iStride   >>= 1;
  uiHeight  >>= 1;
  uiWidth   >>= 1;
  pPel        = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pChar[x] = (UChar)( gMax( (Int)0, gMin( (Int)255, pPel[x] ) ) );
    }
    pPel += iStride;
    ROF( uiWidth == ::fwrite( pChar, sizeof(UChar), uiWidth, pFile ) );
  }

  //===== chrominance V =====
  pPel        = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      pChar[x] = (UChar)( gMax( (Int)0, gMin( (Int)255, pPel[x] ) ) );
    }
    pPel += iStride;
    ROF( uiWidth == ::fwrite( pChar, sizeof(UChar), uiWidth, pFile ) );
  }

  delete [] pChar; // bug-fix by H. Schwarz / J. Reichel

  return Err::m_nOK;
}



ErrVal YuvPicBuffer::dumpHPS( FILE* pFile, MbDataCtrl* pcMbDataCtrl )
{
  Int     iNumMbY   = getLHeight() >> 4;
  Int     iNumMbX   = getLWidth () >> 4;
  UChar*  pucIntra  = new UChar[iNumMbX*iNumMbY];
  ROF( pucIntra );
  ::memset( pucIntra, 0x00, iNumMbX*iNumMbY*sizeof(UChar) );

  if( pcMbDataCtrl )
  {
    for( Int iMbY = 0; iMbY < iNumMbY; iMbY++ )
    for( Int iMbX = 0; iMbX < iNumMbX; iMbX++ )
    {
      if( pcMbDataCtrl->getMbData( iMbX, iMbY ).isIntra() )
      {
        pucIntra[iMbY*iNumMbX+iMbX] = 1;
      }
    }
  }


  UChar*  pChar     = new UChar [ getLWidth() ];
  ROF( pChar );

  m_rcYuvBufferCtrl.initMb();

  XPel*   pPel      = getMbLumAddr();
  Int     iStride   = getLStride();
  UInt    uiHeight  = getLHeight();
  UInt    uiWidth   = getLWidth ();
  UInt    y, x;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      if( pucIntra[(y>>4)*iNumMbX+(x>>4)] )
        pChar[x] = (UChar)( gMax( (Int)0, gMin( (Int)255,       pPel[x] ) ) );
      else
        pChar[x] = (UChar)( gMax( (Int)0, gMin( (Int)255, 127 + pPel[x] ) ) );
    }
    pPel += iStride;
    ROF( uiWidth == ::fwrite( pChar, sizeof(UChar), uiWidth, pFile ) );
  }

  //===== chrominance U =====
  iStride   >>= 1;
  uiHeight  >>= 1;
  uiWidth   >>= 1;
  pPel        = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      if( pucIntra[(y>>3)*iNumMbX+(x>>3)] )
        pChar[x] = (UChar)( gMax( (Int)0, gMin( (Int)255,       pPel[x] ) ) );
      else
        pChar[x] = (UChar)( gMax( (Int)0, gMin( (Int)255, 127 + pPel[x] ) ) );
    }
    pPel += iStride;
    ROF( uiWidth == ::fwrite( pChar, sizeof(UChar), uiWidth, pFile ) );
  }

  //===== chrominance V =====
  pPel        = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      if( pucIntra[(y>>3)*iNumMbX+(x>>3)] )
        pChar[x] = (UChar)( gMax( (Int)0, gMin( (Int)255,       pPel[x] ) ) );
      else
        pChar[x] = (UChar)( gMax( (Int)0, gMin( (Int)255, 127 + pPel[x] ) ) );
    }
    pPel += iStride;
    ROF( uiWidth == ::fwrite( pChar, sizeof(UChar), uiWidth, pFile ) );
  }


  delete [] pChar; // bug-fix by H. Schwarz / J. Reichel
  delete [] pucIntra; // bug-fix by H. Schwarz / J. Reichel

  return Err::m_nOK;
}

Bool
YuvPicBuffer::isCurr4x4BlkNotZero( LumaIdx c4x4Idx )
{
  Int   iStride = getLStride  ();
  XPel* pPel    = getMbLumAddr() + ( c4x4Idx.y() << 2 ) * iStride + ( c4x4Idx.x() << 2 );
  for( Int iY = 0; iY < 4; iY++, pPel += iStride )
  {
    for( Int iX = 0; iX < 4; iX++ )
    {
      ROTRS( pPel[ iX ], true );
    }
  }
  return false;
}

Bool
YuvPicBuffer::isCurr8x8BlkNotZero( B8x8Idx c8x8Idx )
{
  Int   iStride = getLStride  ();
  XPel* pPel    = getMbLumAddr() + ( c8x8Idx.y() << 2 ) * iStride + ( c8x8Idx.x() << 2 );
  for( Int iY = 0; iY < 8; iY++, pPel += iStride )
  {
    for( Int iX = 0; iX < 8; iX++ )
    {
      ROTRS( pPel[ iX ], true );
    }
  }
  return false;
}

ErrVal YuvPicBuffer::setNonZeroFlags( UShort* pusNonZeroFlags, UInt uiStride )
{
  m_rcYuvBufferCtrl.initMb();

  XPel* pData       = getMbLumAddr();
  Int   iDataStride = getLStride  ();
  UInt  uiHeight    = getLHeight  ();
  UInt  uiWidth     = getLWidth   ();
  UInt  y, x;

  //===== clear all flags =====
  for( y = 0; y < (uiHeight>>4); y++ )
  for( x = 0; x < (uiWidth >>4); x++ )
  {
    pusNonZeroFlags[y*uiStride+x] = 0;
  }

  //===== luma =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      if( pData[x] )
      {
        UShort& usMbFlags   = pusNonZeroFlags[(y>>4)*uiStride+(x>>4)];
        UInt    uiFlagsPos  = ((y%16)>>2)*4+((x%16)>>2);
        usMbFlags |= ( 1 << uiFlagsPos );
      }
    }
    pData += iDataStride;
  }

  iDataStride >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pData = getMbCbAddr();

  //===== cb =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      if( pData[x] )
      {
        UShort& usMbFlags   = pusNonZeroFlags[(y>>3)*uiStride+(x>>3)];
        UInt    uiFlagsPos  = ((y%8)>>1)*4+((x%8)>>1);
        usMbFlags |= ( 1 << uiFlagsPos );
      }
    }
    pData += iDataStride;
  }

  pData = getMbCrAddr();

  //===== cr =====
  for( y = 0; y < uiHeight; y++ )
  {
    for( x = 0; x < uiWidth; x++ )
    {
      if( pData[x] )
      {
        UShort& usMbFlags   = pusNonZeroFlags[(y>>3)*uiStride+(x>>3)];
        UInt    uiFlagsPos  = ((y%8)>>1)*4+((x%8)>>1);
        usMbFlags |= ( 1 << uiFlagsPos );
      }
    }
    pData += iDataStride;
  }

  return Err::m_nOK;
}

Void YuvPicBuffer::xCopyFillPlaneMargin( XPel *pucSrc, XPel *pucDest, Int iHeight, Int iWidth, Int iStride, Int iXMargin, Int iYMargin )
{
  XPel* puc;
  Int n;

  Int iOffset = -iXMargin;
  // rec area + left and right borders at once
  UInt uiSize = sizeof(XPel)*(iWidth + 2*iXMargin);
  for( n = 0; n < iHeight; n++ )
  {
    memcpy( pucDest + iOffset, pucSrc + iOffset, uiSize );
    iOffset += iStride;
  }

  // bot border lum
  puc = pucDest - iXMargin + iStride * iHeight;
  for( n = 0; n < iYMargin; n++ )
  {
    memcpy( puc, puc - iStride, uiSize );
    puc += iStride;
  }

  // top border lum
  puc = pucDest - iXMargin;
  for( n = 0; n < iYMargin; n++ )
  {
    memcpy( puc - iStride, puc, uiSize );
    puc -= iStride;
  }
}

Void YuvPicBuffer::xCopyPlane( XPel *pucSrc, XPel *pucDest, Int iHeight, Int iWidth, Int iStride )
{
  // don't copy if src and dest have the same address
  ROTVS( pucSrc == pucDest);

  const UInt uiSize = sizeof(XPel)*(iWidth );
  Int iOffset = 0;
  for( Int n = 0; n < iHeight; n++)
  {
    memcpy( pucDest + iOffset, pucSrc + iOffset, uiSize );
    iOffset += iStride;
  }
}

ErrVal YuvPicBuffer::loadBuffer( YuvPicBuffer *pcSrcYuvPicBuffer )
{
  m_rcYuvBufferCtrl.initMb();

  ROT( pcSrcYuvPicBuffer->getLHeight() * pcSrcYuvPicBuffer->getLStride() != getLHeight() * getLStride() );

 //TMM_INTERLACE
  Int iSrcOffset  = pcSrcYuvPicBuffer->m_ePicType == BOT_FIELD ? - pcSrcYuvPicBuffer->getCStride() : 0;
  iSrcOffset += m_ePicType == BOT_FIELD ? getCStride() : 0;

  xCopyPlane( iSrcOffset + pcSrcYuvPicBuffer->getMbLumAddr(), getMbLumAddr(), getLHeight(), getLWidth(), getLStride() );
  iSrcOffset >>= 1;
  xCopyPlane( iSrcOffset + pcSrcYuvPicBuffer->getMbCbAddr(),  getMbCbAddr(),  getCHeight(), getCWidth(), getCStride() );
  xCopyPlane( iSrcOffset + pcSrcYuvPicBuffer->getMbCrAddr(),  getMbCrAddr(),  getCHeight(), getCWidth(), getCStride() );

  return Err::m_nOK;
}

ErrVal YuvPicBuffer::loadBufferAndFillMargin( YuvPicBuffer *pcSrcYuvPicBuffer )
{
  m_rcYuvBufferCtrl.initMb();
  ROT( pcSrcYuvPicBuffer->getLHeight() * pcSrcYuvPicBuffer->getLStride() != getLHeight() * getLStride() );

  //TMM_INTERLACE
  Int iSrcOffset  = pcSrcYuvPicBuffer->m_ePicType == BOT_FIELD ? - pcSrcYuvPicBuffer->getCStride() : 0;
  iSrcOffset += m_ePicType == BOT_FIELD ? getCStride() : 0;

  xCopyFillPlaneMargin( iSrcOffset + pcSrcYuvPicBuffer->getMbLumAddr(), getMbLumAddr(), getLHeight(), getLWidth(), getLStride(), getLXMargin(), getLYMargin() );
  iSrcOffset >>= 1;
  xCopyFillPlaneMargin( iSrcOffset + pcSrcYuvPicBuffer->getMbCbAddr(),  getMbCbAddr(),  getCHeight(), getCWidth(), getCStride(), getCXMargin(), getCYMargin() );
  xCopyFillPlaneMargin( iSrcOffset + pcSrcYuvPicBuffer->getMbCrAddr(),  getMbCrAddr(),  getCHeight(), getCWidth(), getCStride(), getCXMargin(), getCYMargin() );

  return Err::m_nOK;
}

ErrVal YuvPicBuffer::fillMargin()
{
  m_rcYuvBufferCtrl.initMb();

  xFillPlaneMargin( getMbLumAddr(), getLHeight(), getLWidth(), getLStride(), getLXMargin(), getLYMargin() );
  xFillPlaneMargin( getMbCbAddr(),  getCHeight(), getCWidth(), getCStride(), getCXMargin(), getCYMargin() );
  xFillPlaneMargin( getMbCrAddr(),  getCHeight(), getCWidth(), getCStride(), getCXMargin(), getCYMargin() );

  return Err::m_nOK;
}


Void YuvPicBuffer::xFillPlaneMargin( XPel *pucDest, Int iHeight, Int iWidth, Int iStride, Int iXMargin, Int iYMargin )
{
  XPel* puc;
  Int   n, m;

  // left and right borders at once
  puc = pucDest;
  for( n = 0; n < iHeight; n++)
  {
    // left border lum
    //::memset( puc - iXMargin, puc[0],         iXMargin*sizeof(XPel) );
    for( m = -iXMargin; m < 0; m++ )    puc[m] = puc[0];
    // right border lum
    //::memset( puc + iWidth,  puc[iWidth - 1], iXMargin*sizeof(XPel) );
    for( m = iWidth; m<iWidth+iXMargin; m++ ) puc[m] = puc[iWidth-1];
    puc += iStride;
  }

  // bot border lum
  puc = pucDest - iXMargin + iStride * iHeight;
  UInt uiSize = iWidth + 2*iXMargin;
  for( n = 0; n < iYMargin; n++)
  {
    memcpy( puc, puc - iStride, uiSize*sizeof(XPel) );
    puc += iStride;
  }

  // top border lum
  puc = pucDest - iXMargin;
  for( n = 0; n < iYMargin; n++)
  {
    memcpy( puc - iStride, puc, uiSize*sizeof(XPel) );
    puc -= iStride;
  }
}

Void YuvPicBuffer::setZero()
{
  Int     n;
  XPel*   p;
  m_rcYuvBufferCtrl.initMb();

  for(n=0,p=getMbLumAddr();n<getLHeight();n++){::memset(p,0x00,getLWidth()*sizeof(XPel) );p+=getLStride();}
  for(n=0,p=getMbCbAddr ();n<getCHeight();n++){::memset(p,0x00,getCWidth()*sizeof(XPel) );p+=getCStride();}
  for(n=0,p=getMbCrAddr ();n<getCHeight();n++){::memset(p,0x00,getCWidth()*sizeof(XPel) );p+=getCStride();}
}








ErrVal YuvPicBuffer::update( YuvPicBuffer*  pcSrcYuvPicBuffer,
                                YuvPicBuffer*  pcMCPYuvPicBuffer0,
                                YuvPicBuffer*  pcMCPYuvPicBuffer1 )
{
  pcSrcYuvPicBuffer ->m_rcYuvBufferCtrl.initMb();
  pcMCPYuvPicBuffer0->m_rcYuvBufferCtrl.initMb();
  pcMCPYuvPicBuffer1->m_rcYuvBufferCtrl.initMb();
  m_rcYuvBufferCtrl.initMb();

  XPel* pSrcAnchor  = pcSrcYuvPicBuffer ->getMbLumAddr();
  XPel* pMCP0Anchor = pcMCPYuvPicBuffer0->getMbLumAddr();
  XPel* pMCP1Anchor = pcMCPYuvPicBuffer1->getMbLumAddr();
  XPel* pDesAnchor  = getMbLumAddr();
  Int   iSrcStride  = pcSrcYuvPicBuffer ->getLStride();
  Int   iMCP0Stride = pcMCPYuvPicBuffer0->getLStride();
  Int   iMCP1Stride = pcMCPYuvPicBuffer1->getLStride();
  Int   iDesStride  = getLStride();
  UInt  uiHeight    = getLHeight();
  UInt  uiWidth     = getLWidth ();
  UInt  y, x;

  //===== luminance =====
  for( y = 0; y < uiHeight; y++ )
  {
    XPel* pSrc  = pSrcAnchor  + y * iSrcStride;
    XPel* pMCP0 = pMCP0Anchor + y * iMCP0Stride;
    XPel* pMCP1 = pMCP1Anchor + y * iMCP1Stride;
    XPel* pDes  = pDesAnchor  + y * iDesStride;

    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] + ( ( pMCP0[x] + pMCP1[x] + 1 ) >> 2 ) );
    }
  }


  //===== chrominance U =====
  iSrcStride  >>= 1;
  iMCP0Stride >>= 1;
  iMCP1Stride >>= 1;
  iDesStride  >>= 1;
  uiHeight    >>= 1;
  uiWidth     >>= 1;
  pSrcAnchor    = pcSrcYuvPicBuffer ->getMbCbAddr();
  pMCP0Anchor   = pcMCPYuvPicBuffer0->getMbCbAddr();
  pMCP1Anchor   = pcMCPYuvPicBuffer1->getMbCbAddr();
  pDesAnchor    = getMbCbAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    XPel* pSrc  = pSrcAnchor  + y * iSrcStride;
    XPel* pMCP0 = pMCP0Anchor + y * iMCP0Stride;
    XPel* pMCP1 = pMCP1Anchor + y * iMCP1Stride;
    XPel* pDes  = pDesAnchor  + y * iDesStride;

    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] + ( ( pMCP0[x] + pMCP1[x] + 1 ) >> 2 ) );
    }
  }

  //===== chrominance V =====
  pSrcAnchor    = pcSrcYuvPicBuffer ->getMbCrAddr();
  pMCP0Anchor   = pcMCPYuvPicBuffer0->getMbCrAddr();
  pMCP1Anchor   = pcMCPYuvPicBuffer1->getMbCrAddr();
  pDesAnchor    = getMbCrAddr();

  for( y = 0; y < uiHeight; y++ )
  {
    XPel* pSrc  = pSrcAnchor  + y * iSrcStride;
    XPel* pMCP0 = pMCP0Anchor + y * iMCP0Stride;
    XPel* pMCP1 = pMCP1Anchor + y * iMCP1Stride;
    XPel* pDes  = pDesAnchor  + y * iDesStride;

    for( x = 0; x < uiWidth; x++ )
    {
      pDes[x] = gClip( pSrc[x] + ( ( pMCP0[x] + pMCP1[x] + 1 ) >> 2 ) );
    }
  }

  return Err::m_nOK;
}






ErrVal YuvPicBuffer::inverseUpdate( YuvPicBuffer*  pcSrcYuvPicBuffer,
                                       YuvPicBuffer*  pcMCPYuvPicBuffer0,
                                       YuvPicBuffer*  pcMCPYuvPicBuffer1 )
{
  pcSrcYuvPicBuffer ->m_rcYuvBufferCtrl.initMb();
	if (pcMCPYuvPicBuffer0)
		pcMCPYuvPicBuffer0->m_rcYuvBufferCtrl.initMb();
	if (pcMCPYuvPicBuffer1)
		pcMCPYuvPicBuffer1->m_rcYuvBufferCtrl.initMb();
  m_rcYuvBufferCtrl.initMb();

	if (pcMCPYuvPicBuffer0 && pcMCPYuvPicBuffer1)
	{
		XPel* pSrcAnchor  = pcSrcYuvPicBuffer ->getMbLumAddr();
		XPel* pMCP0Anchor = pcMCPYuvPicBuffer0->getMbLumAddr();
		XPel* pMCP1Anchor = pcMCPYuvPicBuffer1->getMbLumAddr();
		XPel* pDesAnchor  = getMbLumAddr();
		Int   iSrcStride  = pcSrcYuvPicBuffer ->getLStride();
		Int   iMCP0Stride = pcMCPYuvPicBuffer0->getLStride();
		Int   iMCP1Stride = pcMCPYuvPicBuffer1->getLStride();
		Int   iDesStride  = getLStride();
		UInt  uiHeight    = getLHeight();
		UInt  uiWidth     = getLWidth ();
		UInt  y, x;

		//===== luminance =====
		for( y = 0; y < uiHeight; y++ )
		{
			XPel* pSrc  = pSrcAnchor  + y * iSrcStride;
			XPel* pMCP0 = pMCP0Anchor + y * iMCP0Stride;
			XPel* pMCP1 = pMCP1Anchor + y * iMCP1Stride;
			XPel* pDes  = pDesAnchor  + y * iDesStride;

			for( x = 0; x < uiWidth; x++ )
			{
				pDes[x] = gClip( pSrc[x] - ( ( pMCP0[x] + pMCP1[x] + 1 ) >> 2 ) );
			}
		}


		//===== chrominance U =====
		iSrcStride  >>= 1;
		iMCP0Stride >>= 1;
		iMCP1Stride >>= 1;
		iDesStride  >>= 1;
		uiHeight    >>= 1;
		uiWidth     >>= 1;
		pSrcAnchor    = pcSrcYuvPicBuffer ->getMbCbAddr();
		pMCP0Anchor   = pcMCPYuvPicBuffer0->getMbCbAddr();
		pMCP1Anchor   = pcMCPYuvPicBuffer1->getMbCbAddr();
		pDesAnchor    = getMbCbAddr();

		for( y = 0; y < uiHeight; y++ )
		{
			XPel* pSrc  = pSrcAnchor  + y * iSrcStride;
			XPel* pMCP0 = pMCP0Anchor + y * iMCP0Stride;
			XPel* pMCP1 = pMCP1Anchor + y * iMCP1Stride;
			XPel* pDes  = pDesAnchor  + y * iDesStride;

			for( x = 0; x < uiWidth; x++ )
			{
				pDes[x] = gClip( pSrc[x] - ( ( pMCP0[x] + pMCP1[x] + 1 ) >> 2 ) );
			}
		}

		//===== chrominance V =====
		pSrcAnchor    = pcSrcYuvPicBuffer ->getMbCrAddr();
		pMCP0Anchor   = pcMCPYuvPicBuffer0->getMbCrAddr();
		pMCP1Anchor   = pcMCPYuvPicBuffer1->getMbCrAddr();
		pDesAnchor    = getMbCrAddr();

		for( y = 0; y < uiHeight; y++ )
		{
			XPel* pSrc  = pSrcAnchor  + y * iSrcStride;
			XPel* pMCP0 = pMCP0Anchor + y * iMCP0Stride;
			XPel* pMCP1 = pMCP1Anchor + y * iMCP1Stride;
			XPel* pDes  = pDesAnchor  + y * iDesStride;

			for( x = 0; x < uiWidth; x++ )
			{
				pDes[x] = gClip( pSrc[x] - ( ( pMCP0[x] + pMCP1[x] + 1 ) >> 2 ) );
			}
		}
	}
	else
	{
		XPel* pSrcAnchor  = pcSrcYuvPicBuffer ->getMbLumAddr();
		XPel* pMCAnchor ;
		XPel* pDesAnchor  = getMbLumAddr();

		Int   iSrcStride  = pcSrcYuvPicBuffer ->getLStride();
		Int   iMCStride;

		Int   iDesStride  = getLStride();
		UInt  uiHeight    = getLHeight();
		UInt  uiWidth     = getLWidth ();
		UInt  y, x;

		if (pcMCPYuvPicBuffer0)
		{
			pMCAnchor = pcMCPYuvPicBuffer0->getMbLumAddr();
			iMCStride = pcMCPYuvPicBuffer0->getLStride();
		}
		else
		{
			pMCAnchor = pcMCPYuvPicBuffer1->getMbLumAddr();
			iMCStride = pcMCPYuvPicBuffer1->getLStride();
		}

		//===== luminance =====
		for( y = 0; y < uiHeight; y++ )
		{
			XPel* pSrc  = pSrcAnchor  + y * iSrcStride;
			XPel* pMC		= pMCAnchor		+ y * iMCStride;
			XPel* pDes  = pDesAnchor  + y * iDesStride;

			for( x = 0; x < uiWidth; x++ )
			{
				pDes[x] = gClip( pSrc[x] - ( ( pMC[x] + 1 ) >> 2 ) );
			}
		}


		//===== chrominance U =====
		iSrcStride  >>= 1;
		iMCStride >>= 1;
		iDesStride  >>= 1;
		uiHeight    >>= 1;
		uiWidth     >>= 1;
		pSrcAnchor    = pcSrcYuvPicBuffer ->getMbCbAddr();
		if (pcMCPYuvPicBuffer0)
		{
			pMCAnchor = pcMCPYuvPicBuffer0->getMbCbAddr();
		}
		else
		{
			pMCAnchor = pcMCPYuvPicBuffer1->getMbCbAddr();
		}
		pDesAnchor    = getMbCbAddr();

		for( y = 0; y < uiHeight; y++ )
		{
			XPel* pSrc  = pSrcAnchor  + y * iSrcStride;
			XPel* pMC		= pMCAnchor		+ y * iMCStride;
			XPel* pDes  = pDesAnchor  + y * iDesStride;

			for( x = 0; x < uiWidth; x++ )
			{
				pDes[x] = gClip( pSrc[x] - ( ( pMC[x] + 1 ) >> 2 ) );
			}
		}

		//===== chrominance V =====
		pSrcAnchor    = pcSrcYuvPicBuffer ->getMbCrAddr();
		if (pcMCPYuvPicBuffer0)
		{
			pMCAnchor = pcMCPYuvPicBuffer0->getMbCbAddr();
		}
		else
		{
			pMCAnchor = pcMCPYuvPicBuffer1->getMbCbAddr();
		}
		pDesAnchor    = getMbCrAddr();

		for( y = 0; y < uiHeight; y++ )
		{
			XPel* pSrc  = pSrcAnchor  + y * iSrcStride;
			XPel* pMC		= pMCAnchor		+ y * iMCStride;
			XPel* pDes  = pDesAnchor  + y * iDesStride;

			for( x = 0; x < uiWidth; x++ )
			{
				pDes[x] = gClip( pSrc[x] - ( ( pMC[x] + 1 ) >> 2 ) );
			}
		}
	}

  return Err::m_nOK;
}

H264AVC_NAMESPACE_END
