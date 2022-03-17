
#if !defined(AFX_READBITSTREAMFILE_H__7390ED11_2FA8_11D4_991C_0080C6053E4F__INCLUDED_)
#define AFX_READBITSTREAMFILE_H__7390ED11_2FA8_11D4_991C_0080C6053E4F__INCLUDED_



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "H264AVCVideoIoLib.h"
#include "LargeFile.h"
#include "ReadBitstreamIf.h"

#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif

class H264AVCVIDEOIOLIB_API ReadBitstreamFile :
  public ReadBitstreamIf
{
protected:
  ReadBitstreamFile();
  virtual ~ReadBitstreamFile();

public:
  virtual ErrVal extractPacket( BinData*& rpcBinData, Bool& rbEOS );
  virtual ErrVal releasePacket( BinData* pcBinData );

  static ErrVal create( ReadBitstreamFile*& rpcReadBitstreamFile );
  virtual ErrVal destroy();

  ErrVal init( const std::string& rcFileName );
  virtual ErrVal uninit();

  virtual ErrVal getPosition( Int& iPos );
  virtual ErrVal setPosition( Int  iPos );

  virtual Int64  getFilePos() { return m_cFile.tell(); }

protected:
  LargeFile m_cFile;
};

#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif


#endif // !defined(AFX_READBITSTREAMFILE_H__7390ED11_2FA8_11D4_991C_0080C6053E4F__INCLUDED_)
