
#if !defined(AFX_INTYUVPICBUFFER_H__5AB262CF_4876_47A2_97A8_5500F7416A8C__INCLUDED_)
#define AFX_INTYUVPICBUFFER_H__5AB262CF_4876_47A2_97A8_5500F7416A8C__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#include "H264AVCCommonLib/YuvBufferCtrl.h"
#include "H264AVCCommonLib/MbTransformCoeffs.h"
#include "H264AVCCommonLib/MbDataCtrl.h"


H264AVC_NAMESPACE_BEGIN



class YuvMbBuffer;



class H264AVCCOMMONLIB_API YuvPicBuffer
{
public:
	YuvPicBuffer         ( YuvBufferCtrl& rcYuvBufferCtrl, PicType ePicType );
	virtual ~YuvPicBuffer();

	const Int     getLStride    ()                const { return m_rcBufferParam.getStride()   ; }
  const Int     getCStride    ()                const { return m_rcBufferParam.getStride()>>1; }

  XPel*         getLumBlk     ()                      { return m_pPelCurrY; }
  XPel*         getCbBlk      ()                      { return m_pPelCurrU; }
  XPel*         getCrBlk      ()                      { return m_pPelCurrV; }

  Void          set4x4Block   ( LumaIdx cIdx )
  {
    m_pPelCurrY = m_pucYuvBuffer + m_rcBufferParam.getYBlk( cIdx );
    m_pPelCurrU = m_pucYuvBuffer + m_rcBufferParam.getUBlk( cIdx );
    m_pPelCurrV = m_pucYuvBuffer + m_rcBufferParam.getVBlk( cIdx );
  }

  // Hanke@RWTH
  Bool          isCurr4x4BlkNotZero ( LumaIdx c4x4Idx );
  Bool          isCurr8x8BlkNotZero ( B8x8Idx c8x8Idx );

  XPel*         getYBlk       ( LumaIdx cIdx )        { AOF_DBG(m_pucYuvBuffer); return m_pucYuvBuffer + m_rcBufferParam.getYBlk( cIdx ); }
  XPel*         getUBlk       ( LumaIdx cIdx )        { AOF_DBG(m_pucYuvBuffer); return m_pucYuvBuffer + m_rcBufferParam.getUBlk( cIdx ); }
  XPel*         getVBlk       ( LumaIdx cIdx )        { AOF_DBG(m_pucYuvBuffer); return m_pucYuvBuffer + m_rcBufferParam.getVBlk( cIdx ); }

  XPel*         getMbLumAddr  ()                const { AOF_DBG(m_pucYuvBuffer); return m_pucYuvBuffer + m_rcBufferParam.getMbLum(); }
  XPel*         getMbCbAddr   ()                const { AOF_DBG(m_pucYuvBuffer); return m_pucYuvBuffer + m_rcBufferParam.getMbCb (); }
  XPel*         getMbCrAddr   ()                const { AOF_DBG(m_pucYuvBuffer); return m_pucYuvBuffer + m_rcBufferParam.getMbCr (); }

  XPel*         getMbLumAddr  ( UInt uiX, UInt uiY, Bool bMbAff ) const { AOF_DBG(m_pucYuvBuffer); return m_pucYuvBuffer + m_rcYuvBufferCtrl.getMbLum( m_ePicType, uiY, uiX, bMbAff ); }
  XPel*         getMbCbAddr   ( UInt uiX, UInt uiY, Bool bMbAff ) const { AOF_DBG(m_pucYuvBuffer); return m_pucYuvBuffer + m_rcYuvBufferCtrl.getMbCb ( m_ePicType, uiY, uiX, bMbAff ); }
  XPel*         getMbCrAddr   ( UInt uiX, UInt uiY, Bool bMbAff ) const { AOF_DBG(m_pucYuvBuffer); return m_pucYuvBuffer + m_rcYuvBufferCtrl.getMbCr ( m_ePicType, uiY, uiX, bMbAff ); }

  const Int     getLWidth     ()                const { return m_rcBufferParam.getWidth ();    }
  const Int     getLHeight    ()                const { return m_rcBufferParam.getHeight();    }
  const Int     getCWidth     ()                const { return m_rcBufferParam.getWidth ()>>1; }
  const Int     getCHeight    ()                const { return m_rcBufferParam.getHeight()>>1; }

  const Int     getLXMargin   ()                const { return m_rcYuvBufferCtrl.getXMargin(); }
  const Int     getLYMargin   ()                const { return m_rcYuvBufferCtrl.getYMargin(); }
  const Int     getCXMargin   ()                const { return m_rcYuvBufferCtrl.getXMargin()>>1; }
  const Int     getCYMargin   ()                const { return m_rcYuvBufferCtrl.getYMargin()>>1; }

  Bool          isValid       ()                      { return NULL != m_pucYuvBuffer; }
  XPel*         getLumOrigin  ()                const { return m_pucYuvBuffer + m_rcYuvBufferCtrl.getLumOrigin( m_ePicType ); }
  XPel*         getCbOrigin   ()                const { return m_pucYuvBuffer + m_rcYuvBufferCtrl.getCbOrigin ( m_ePicType ); }
  XPel*         getCrOrigin   ()                const { return m_pucYuvBuffer + m_rcYuvBufferCtrl.getCrOrigin ( m_ePicType ); }

  ErrVal        loadFromPicBuffer       ( PicBuffer*        pcPicBuffer );
  ErrVal        storeToPicBuffer        ( PicBuffer*        pcPicBuffer );
  ErrVal        interpolatedPicBuffer   ( PicBuffer*        pcPicBuffer, Bool bBotField );

  ErrVal        loadBuffer              ( YuvMbBuffer*   pcYuvMbBuffer, Int iMbXOffset = 0, Int iMbYOffset = 0 );
  ErrVal        loadBuffer_MbAff        ( YuvMbBuffer*   pcYuvMbBuffer, UInt uiMask ); //TMM_INTERLACE

  ErrVal        fillMargin              ();

  ErrVal        prediction              ( YuvPicBuffer*  pcSrcYuvPicBuffer, YuvPicBuffer*  pcMCPYuvPicBuffer );
  ErrVal        update                  ( YuvPicBuffer*  pcSrcYuvPicBuffer, YuvPicBuffer*  pcMCPYuvPicBuffer, UInt uiShift );
  ErrVal        inversePrediction       ( YuvPicBuffer*  pcSrcYuvPicBuffer, YuvPicBuffer*  pcMCPYuvPicBuffer );
  ErrVal        inverseUpdate           ( YuvPicBuffer*  pcSrcYuvPicBuffer, YuvPicBuffer*  pcMCPYuvPicBuffer, UInt uiShift );

  ErrVal        update                  ( YuvPicBuffer*  pcSrcYuvPicBuffer, YuvPicBuffer*  pcMCPYuvPicBuffer0, YuvPicBuffer*  pcMCPYuvPicBuffer1 );
  ErrVal        inverseUpdate           ( YuvPicBuffer*  pcSrcYuvPicBuffer, YuvPicBuffer*  pcMCPYuvPicBuffer0, YuvPicBuffer*  pcMCPYuvPicBuffer1 );

  ErrVal        copy                    ( YuvPicBuffer*  pcSrcYuvPicBuffer );
  ErrVal        setZeroMB               ();

  ErrVal        subtract                ( YuvPicBuffer*  pcSrcYuvPicBuffer0, YuvPicBuffer* pcSrcYuvPicBuffer1 );
  ErrVal        add                     ( YuvPicBuffer*  pcSrcYuvPicBuffer );

  ErrVal        addWeighted             ( YuvPicBuffer*  pcSrcYuvPicBuffer, Double dWeight );

  ErrVal        dumpLPS                 ( FILE* pFile );
  ErrVal        dumpHPS                 ( FILE* pFile, MbDataCtrl* pcMbDataCtrl );

  ErrVal        init                    ( XPel*&            rpucYuvBuffer );
  ErrVal        uninit                  ();
  Void          setZero                 ();
  ErrVal        clip                    ();

  ErrVal        loadFromFile8Bit        ( FILE* pFile );

  ErrVal        getSSD                  ( Double& dSSDY, Double& dSSDU, Double& dSSDV, PicBuffer* pcOrgPicBuffer );

  ErrVal        setNonZeroFlags         ( UShort* pusNonZeroFlags, UInt uiStride );

  ErrVal        clear();
  ErrVal        clearCurrMb();


  YuvBufferCtrl& getBufferCtrl()  { return m_rcYuvBufferCtrl; }
  XPel*          getBuffer    ()  { return m_pucYuvBuffer; }

  ErrVal loadBufferAndFillMargin( YuvPicBuffer *pcSrcYuvPicBuffer );
  ErrVal loadBuffer             ( YuvPicBuffer *pcSrcYuvPicBuffer );

	//--
	// JVT-R057 LA-RDO{
	YuvBufferCtrl& getYuvBufferCtrl(){ return m_rcYuvBufferCtrl;}
	// JVT-R057 LA-RDO}
	//JVT-X046 {
	void   setMBZero( UInt uiMBY, UInt uiMBX );
  ErrVal predictionSlices(YuvPicBuffer*  pcSrcYuvPicBuffer, YuvPicBuffer*  pcMCPYuvPicBuffer, UInt uiMbY, UInt uiMbX );
  ErrVal inversepredictionSlices(YuvPicBuffer*  pcSrcYuvPicBuffer, YuvPicBuffer*  pcMCPYuvPicBuffer, UInt uiMbY, UInt uiMbX );
  ErrVal copyMb(YuvPicBuffer* pcSrcYuvPicBuffer,UInt uiMbY, UInt uiMbX);
	ErrVal copyMB( YuvPicBuffer* pcSrcYuvPicBuffer, UInt uiMbAddress)
	{
		Int iSrcStride = pcSrcYuvPicBuffer->getLStride();
		Int iDesStride = getLStride();
		UInt uiWidth     = pcSrcYuvPicBuffer->getLWidth ()/16;
		UInt uiXPos,uiYPos;
		uiXPos = uiMbAddress % uiWidth;
		uiYPos = (uiMbAddress)/uiWidth;
		pcSrcYuvPicBuffer->getYuvBufferCtrl().initMb(uiYPos,uiXPos,false);
		getYuvBufferCtrl().initMb(uiYPos,uiXPos,false);
		XPel* pSrc = pcSrcYuvPicBuffer->getMbLumAddr();
		XPel* pDes = getMbLumAddr();

		UInt y,x;
		for ( y = 0; y < 16; y++ )
		{
			for ( x = 0; x < 16; x++ )
				pDes[x]=pSrc[x];
			pSrc += iSrcStride;
			pDes += iDesStride;
		}
		iSrcStride >>= 1;
		iDesStride >>= 1;

		pSrc = pcSrcYuvPicBuffer->getMbCbAddr();
		pDes = getMbCbAddr();

		for ( y = 0; y < 8; y++ )
		{
			for ( x = 0; x < 8; x++ )
				pDes[x] = pSrc[x];
			pSrc += iSrcStride;
			pDes += iDesStride;
		}

		pSrc = pcSrcYuvPicBuffer->getMbCrAddr();
		pDes = getMbCrAddr();
		for ( y = 0; y < 8; y++ )
		{

			for ( x = 0; x < 8; x++)
				pDes[x] = pSrc[x];
			pSrc += iSrcStride;
			pDes += iDesStride;
		}
		return Err::m_nOK;
	}
	ErrVal copySlice( YuvPicBuffer* pcSrcYuvPicBuffer,UInt uiFirstMB,UInt uiLastMB)
	{
		for (UInt uiMbAddress=uiFirstMB;uiMbAddress<uiLastMB;uiMbAddress++)
		{
			RNOK(copyMB(pcSrcYuvPicBuffer,uiMbAddress));
		}
		return Err::m_nOK;
	}
  //JVT-X046 }
protected:
  Void xCopyFillPlaneMargin( XPel *pucSrc, XPel *pucDest, Int iHeight, Int iWidth, Int iStride, Int iXMargin, Int iYMargin );
  Void xCopyPlane          ( XPel *pucSrc, XPel *pucDest, Int iHeight, Int iWidth, Int iStride );
  Void xFillPlaneMargin     ( XPel *pucDest, Int iHeight, Int iWidth, Int iStride, Int iXMargin, Int iYMargin );

protected:
  const YuvBufferCtrl::YuvBufferParameter&  m_rcBufferParam;
  YuvBufferCtrl&                            m_rcYuvBufferCtrl;

  XPel*           m_pPelCurrY;
  XPel*           m_pPelCurrU;
  XPel*           m_pPelCurrV;

  XPel*           m_pucYuvBuffer;
  XPel*           m_pucOwnYuvBuffer;

  const PicType   m_ePicType;
private:
  YuvPicBuffer();
};



H264AVC_NAMESPACE_END



#endif // !defined(AFX_INTYUVPICBUFFER_H__5AB262CF_4876_47A2_97A8_5500F7416A8C__INCLUDED_)
