
#if !defined(AFX_WRITEYUVATORGB_H__4C65F4FD_2AC8_4464_A27B_F01632777FFC__INCLUDED_)
#define AFX_WRITEYUVATORGB_H__4C65F4FD_2AC8_4464_A27B_F01632777FFC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class WriteYuvaToRgb
{

protected:
  WriteYuvaToRgb();
  virtual ~WriteYuvaToRgb();

public:
  static ErrVal create( WriteYuvaToRgb*& rpcWriteYuvaToRgb );
  ErrVal destroy();

  ErrVal setFrameDimension( UInt uiLumHeight, UInt uiLumWidth );

  virtual ErrVal writeFrameRGB(  UChar* pucRGB,
                                 UInt uiDestStride,
                                 const UChar *pLum,
                                 const UChar *pCb,
                                 const UChar *pCr,
                                 UInt uiLumHeight,
                                 UInt uiLumWidth,
                                 UInt uiLumStride );

  virtual ErrVal writeFrameYUYV( UChar* pucYUYV,
                                 UInt uiDestStride,
                                 const UChar *pLum,
                                 const UChar *pCb,
                                 const UChar *pCr,
                                 UInt uiLumHeight,
                                 UInt uiLumWidth,
                                 UInt uiLumStride );

  virtual ErrVal writeFrameYV12( UChar* pucYUYV,
                                 UInt uiDestStride,
                                 const UChar *pLum,
                                 const UChar *pCb,
                                 const UChar *pCr,
                                 UInt uiLumHeight,
                                 UInt uiLumWidth,
                                 UInt uiLumStride );

protected:
  UInt m_uiHeight;
  UInt m_uiWidth;
};

#endif // !defined(AFX_WRITEYUVATORGB_H__4C65F4FD_2AC8_4464_A27B_F01632777FFC__INCLUDED_)
