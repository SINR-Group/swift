
#if !defined(AFX_WRITEBITSTREAMTOFILE_H__B008532B_0DD3_488A_85D9_F68395BF2E26__INCLUDED_)
#define AFX_WRITEBITSTREAMTOFILE_H__B008532B_0DD3_488A_85D9_F68395BF2E26__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#if defined( MSYS_WIN32 )
#pragma warning( disable: 4275 )
#endif

#if defined( MSYS_WIN32 )
#pragma warning( disable: 4250 )
#endif



#include "LargeFile.h"
#include "WriteBitstreamIf.h"

class H264AVCVIDEOIOLIB_API WriteBitstreamToFile :
public WriteBitstreamIf
{
protected:
  WriteBitstreamToFile() : m_uiNumber(0), m_bNewFileOnNewAu(false) {}
  virtual ~WriteBitstreamToFile() {}

public:
  static ErrVal create( WriteBitstreamToFile*& rpcWriteBitstreamToFile );
  virtual ErrVal destroy();
  ErrVal init( const std::string& rcFileName, Bool bNewFileOnNewAu = false );
  virtual ErrVal uninit();

  virtual ErrVal writePacket( BinDataAccessor* pcBinDataAccessor, Bool bNewAU = false );
  virtual ErrVal writePacket( BinData* pcBinData, Bool bNewAU = false );

  virtual ErrVal writePacket( Void* pBuffer, UInt uiLength );

private:
  UInt m_uiNumber;
  LargeFile m_cFile;
  std::string m_cFileName;
  Bool m_bNewFileOnNewAu;
};

#if defined( MSYS_WIN32 )
#pragma warning( default: 4275 )
#endif

#if defined( MSYS_WIN32 )
#pragma warning( default: 4250 )
#endif


#endif // !defined(AFX_WRITEBITSTREAMTOFILE_H__B008532B_0DD3_488A_85D9_F68395BF2E26__INCLUDED_)
