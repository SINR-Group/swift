
#if !defined(AFX_WRITEBITSTREAMIF_H__F3A95178_0964_42A7_9F6E_5B3DCA116048__INCLUDED_)
#define AFX_WRITEBITSTREAMIF_H__F3A95178_0964_42A7_9F6E_5B3DCA116048__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class H264AVCVIDEOIOLIB_API WriteBitstreamIf
{
public:
  WriteBitstreamIf() {};
  virtual ~WriteBitstreamIf() {};

  virtual ErrVal destroy() = 0;
  virtual ErrVal uninit() = 0;

  virtual ErrVal writePacket( BinDataAccessor* pcBinDataAccessor, Bool bNewAU = false ) = 0;
  virtual ErrVal writePacket( BinData* pcBinData, Bool bNewAU = false ) = 0;
  virtual ErrVal writePacket( Void* pBuffer, UInt uiLength ) = 0;

};

#endif // !defined(AFX_WRITEBITSTREAMIF_H__F3A95178_0964_42A7_9F6E_5B3DCA116048__INCLUDED_)
