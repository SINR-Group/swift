
#if !defined(AFX_WRITEYUVIF_H__2111DC3E_D59F_45FC_9161_C3004A48842D__INCLUDED_)
#define AFX_WRITEYUVIF_H__2111DC3E_D59F_45FC_9161_C3004A48842D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class H264AVCVIDEOIOLIB_API WriteYuvIf
{
protected:
  WriteYuvIf() {}
  virtual ~WriteYuvIf() {}

public:
  virtual ErrVal writeFrame( const UChar *pLum,
                             const UChar *pCb,
                             const UChar *pCr,
                             UInt uiLumHeight,
                             UInt uiLumWidth,
                             UInt uiLumStride,
                             const UInt rauiCropping[] )  = 0;

  virtual ErrVal destroy() = 0;
};

#endif // !defined(AFX_WRITEYUVIF_H__2111DC3E_D59F_45FC_9161_C3004A48842D__INCLUDED_)
