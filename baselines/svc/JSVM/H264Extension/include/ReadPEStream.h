
#if !defined(AFX_READPESTREAM_H__DE3116D9_2399_41C8_9C01_E3118DDDE939__INCLUDED_)
#define AFX_READPESTREAM_H__DE3116D9_2399_41C8_9C01_E3118DDDE939__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "LargeFile.h"

class H264AVCVIDEOIOLIB_API ReadPEStream
{
protected:
  ReadPEStream();
  virtual ~ReadPEStream();
public:
  ErrVal extractPacket( BinData*& rpcBinData, UInt& uiCts, Bool& rbEOS );
  ErrVal releasePacket( BinData* pcBinData );

  static ErrVal create( ReadPEStream*& rpcReadPEStream );
  ErrVal destroy();

  ErrVal reset();

  ErrVal init( const std::string& rcFileName );
  ErrVal uninit();
protected:
  LargeFile m_cFile;
  UInt   m_uiAULength;
  UChar* m_pucReadPtr;
  UChar* m_pucPayload;
  UInt64 m_uiCts90;
  UInt64 m_uiDts90;
  ErrVal xGetPtsDts( UChar* pucHeader, UInt64& uiTS90);
};

#endif // !defined(AFX_READPESTREAM_H__DE3116D9_2399_41C8_9C01_E3118DDDE939__INCLUDED_)
