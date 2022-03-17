
#if !defined(AFX_READYUVFILE_H__F07173C5_E390_46AD_AACE_7A529A0DCE33__INCLUDED_)
#define AFX_READYUVFILE_H__F07173C5_E390_46AD_AACE_7A529A0DCE33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "LargeFile.h"

class H264AVCVIDEOIOLIB_API ReadYuvFile
{

public:
enum FillMode
{
  FILL_CLEAR = 0,
  FILL_FRAME,
  FILL_FIELD
};

protected:
  ReadYuvFile();
  virtual ~ReadYuvFile();

public:
  static ErrVal create( ReadYuvFile*& rpcReadYuvFile );
  ErrVal init( const std::string& rcFileName, UInt uiLumPicHeight, UInt uiLumPicWidth, UInt uiStartLine = 0, UInt uiEndLine = MSYS_UINT_MAX, FillMode eFillMode = FILL_CLEAR );
  ErrVal uninit();
  ErrVal destroy();

  ErrVal readFrame(  UChar *pLum,
                     UChar *pCb,
                     UChar *pCr,
                     UInt uiBufHeight,
                     UInt uiBufWidth,
                     UInt uiBufStride );

  Int  getFillMode() {return m_eFillMode;};

protected:
  ErrVal xReadPlane( UChar *pucDest, UInt uiBufHeight, UInt uiBufWidth, UInt uiBufStride, UInt uiPicHeight, UInt uiPicWidth, UInt uiStartLine, UInt uiEndLine );

protected:
  LargeFile m_cFile;
  UInt m_uiLumPicHeight;
  UInt m_uiLumPicWidth;
  UInt m_uiStartLine;
  UInt m_uiEndLine;
  FillMode m_eFillMode;
};

#endif // !defined(AFX_READYUVFILE_H__F07173C5_E390_46AD_AACE_7A529A0DCE33__INCLUDED_)
