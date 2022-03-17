
#if !defined(AFX_INTYUVMBBUFFER_H__C9AFC3F1_5EEF_43A6_8105_C3BCD7B098FA__INCLUDED_)
#define AFX_INTYUVMBBUFFER_H__C9AFC3F1_5EEF_43A6_8105_C3BCD7B098FA__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN


class YuvPicBuffer;


#define OFFSET 19
class H264AVCCOMMONLIB_API YuvMbBuffer
{
public:
	YuvMbBuffer();
	virtual ~YuvMbBuffer();

  XPel*     getLumBlk     ()                      { return m_pPelCurrY; }
  XPel*     getCbBlk      ()                      { return m_pPelCurrU; }
  XPel*     getCrBlk      ()                      { return m_pPelCurrV; }

  Void      set4x4Block   ( LumaIdx cIdx )
  {
    m_pPelCurrY = &m_aucYuvBuffer[   MB_BUFFER_WIDTH +  4 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<2)];
    m_pPelCurrU = &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH +  4 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<1)];
    m_pPelCurrV = &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH + 16 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<1)];
  }

  const Int getLStride    ()                const { return MB_BUFFER_WIDTH;}
  const Int getCStride    ()                const { return MB_BUFFER_WIDTH;}

  const XPel* getYBlk     ( LumaIdx   cIdx )  const { return &m_aucYuvBuffer[   MB_BUFFER_WIDTH +  4 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<2)]; }
  const XPel* getUBlk     ( LumaIdx   cIdx )  const { return &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH +  4 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<1)]; }
  const XPel* getVBlk     ( LumaIdx   cIdx )  const { return &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH + 16 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<1)]; }
  const XPel* getCBlk     ( ChromaIdx cIdx )  const { return &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH +  4 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<2) + 12*cIdx.plane()]; }
  XPel*       getYBlk     ( LumaIdx   cIdx )        { return &m_aucYuvBuffer[   MB_BUFFER_WIDTH +  4 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<2)]; }
  XPel*       getUBlk     ( LumaIdx   cIdx )        { return &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH +  4 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<1)]; }
  XPel*       getVBlk     ( LumaIdx   cIdx )        { return &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH + 16 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<1)]; }
  XPel*       getCBlk     ( ChromaIdx cIdx )        { return &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH +  4 + ((cIdx.x() + cIdx.y()* MB_BUFFER_WIDTH)<<2) + 12*cIdx.plane()]; }
  const XPel* getMbLumAddr()                  const { return &m_aucYuvBuffer[   MB_BUFFER_WIDTH +  4]; }
  const XPel* getMbCbAddr ()                  const { return &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH +  4]; }
  const XPel* getMbCrAddr ()                  const { return &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH + 16];  }
  XPel*       getMbLumAddr()                        { return &m_aucYuvBuffer[   MB_BUFFER_WIDTH +  4]; }
  XPel*       getMbCbAddr ()                        { return &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH +  4]; }
  XPel*       getMbCrAddr ()                        { return &m_aucYuvBuffer[OFFSET*MB_BUFFER_WIDTH + 16];  }

  const Int getLWidth     ()                const { return 16; }
  const Int getLHeight    ()                const { return 16; }
  const Int getCWidth     ()                const { return 8; }
  const Int getCHeight    ()                const { return 8; }

  Void  loadBuffer         ( const YuvPicBuffer*  pcSrcBuffer );
  Void  loadLuma           ( const YuvMbBuffer&   rcSrcBuffer, LumaIdx c4x4Idx);
  Void  loadLuma           ( const YuvMbBuffer&   rcSrcBuffer, B8x8Idx c8x8Idx);
  Void  loadLuma           ( const YuvMbBuffer&   rcSrcBuffer );
  Void  loadChroma         ( const YuvMbBuffer&   rcSrcBuffer );
  Void  loadIntraPredictors( const YuvPicBuffer*  pcSrcBuffer );
  Void  setAllSamplesToZero();

  Void  add     ( const YuvMbBuffer& rcIntYuvMbBuffer );
  Void  addRes  ( const YuvMbBuffer& rcIntYuvMbBuffer );
  Void  addClip ( const YuvMbBuffer& rcIntYuvMbBuffer );
  Void  subtract( const YuvMbBuffer& rcIntYuvMbBuffer );
  Void  clip    ();
  Bool  isZero  ();
	Void  setZero ();

protected:
  XPel* m_pPelCurrY;
  XPel* m_pPelCurrU;
  XPel* m_pPelCurrV;
  XPel  m_aucYuvBuffer[MB_BUFFER_WIDTH*(29+1)];
};

class H264AVCCOMMONLIB_API YuvMbBufferExtension : public YuvMbBuffer
{
public:
  Void loadSurrounding      ( const YuvPicBuffer* pcSrcBuffer,               Int iMbXOffset = 0, Int iMbYOffset = 0 );
  Void loadSurrounding_MbAff( const YuvPicBuffer* pcSrcBuffer, UInt uiMask,  Int iMbXOffset = 0, Int iMbYOffset = 0 ); //TMM_INTERLACE

  Void mergeFromLeftAbove ( LumaIdx cIdx, Bool bCornerMbPresent, Bool bHalfYSize = false );
  Void mergeFromRightBelow( LumaIdx cIdx, Bool bCornerMbPresent, Bool bHalfYSize = false );
  Void mergeFromRightAbove( LumaIdx cIdx, Bool bCornerMbPresent, Bool bHalfYSize = false );
  Void mergeFromLeftBelow ( LumaIdx cIdx, Bool bCornerMbPresent, Bool bHalfYSize = false );

  Void copyFromBelow      ( LumaIdx cIdx, Bool bHalfYSize = false );
  Void copyFromLeft       ( LumaIdx cIdx );
  Void copyFromAbove      ( LumaIdx cIdx, Bool bHalfYSize = false );
  Void copyFromRight      ( LumaIdx cIdx );

  Void copyFromLeftAbove  ( LumaIdx cIdx, Bool bHalfYSize = false );
  Void copyFromRightAbove ( LumaIdx cIdx, Bool bHalfYSize = false );
  Void copyFromLeftBelow  ( LumaIdx cIdx, Bool bHalfYSize = false );
  Void copyFromRightBelow ( LumaIdx cIdx, Bool bHalfYSize = false );

  Void xFill  ( LumaIdx cIdx, XPel cY, XPel cU, XPel cV, Bool bHalfYSize, Bool bLowerHalf );
  Void xMerge ( Int xDir, Int yDir, Int iSize, XPel* puc, Int iStride, Bool bCornerMbPresent, Bool bHalfYSize );
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_INTYUVMBBUFFER_H__C9AFC3F1_5EEF_43A6_8105_C3BCD7B098FA__INCLUDED_)
