
#include "H264AVCVideoIoLib.h"

#include "LargeFile.h"

#include <errno.h>

#if defined( MSYS_WIN32 )
# include <io.h>
# include <sys/stat.h>
# include <fcntl.h>
#else
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
// heiko.schwarz@hhi.fhg.de: support for BSD systems as proposed by Steffen Kamp [kamp@ient.rwth-aachen.de]
#define _FILE_OFFSET_BITS 64
#endif
# include <sys/ioctl.h>
# include <sys/types.h>
# include <fcntl.h>
# include <dlfcn.h>
# include <cerrno>
# include <unistd.h>
#endif


LargeFile::LargeFile() :
  m_iFileHandle( -1 )
{
}


LargeFile::~LargeFile()
{
  if( -1 != m_iFileHandle )
  {
    ::close( m_iFileHandle );
  }
}


ErrVal LargeFile::open( const std::string& rcFilename, enum OpenMode eOpenMode, int iPermMode )
{
  ROT( rcFilename.empty() );
  ROF( -1 == m_iFileHandle );

  int iOpenMode;

#if defined( MSYS_WIN32 )

  if( eOpenMode == OM_READONLY )
  {
    iOpenMode = _O_RDONLY;
  }
  else if( eOpenMode == OM_WRITEONLY )
  {
    iOpenMode = _O_CREAT | _O_TRUNC | _O_WRONLY;
  }
  else if( eOpenMode == OM_APPEND )
  {
    //append mode does not imply write access and the create flag
    //simplifies the program's logic (in most cases).
    iOpenMode = _O_APPEND | _O_CREAT | _O_WRONLY;
  }
  else if( eOpenMode == OM_READWRITE )
  {
    iOpenMode = _O_CREAT | _O_RDWR;
  }
  else
  {
    AF();
    return Err::m_nERR;
  }

  iOpenMode |= _O_SEQUENTIAL | _O_BINARY;

  m_iFileHandle = ::open( rcFilename.c_str(), iOpenMode, iPermMode );

#elif defined( MSYS_UNIX_LARGEFILE )

  if( eOpenMode == OM_READONLY )
  {
    iOpenMode = O_RDONLY;
  }
  else if( eOpenMode == OM_WRITEONLY )
  {
    iOpenMode = O_CREAT | O_TRUNC | O_WRONLY;
  }
  else if( eOpenMode == OM_APPEND )
  {
    iOpenMode = O_APPEND | O_CREAT | O_WRONLY;
  }
  else if( eOpenMode == OM_READWRITE )
  {
    iOpenMode = O_CREAT | O_RDWR;
  }
  else
  {
    AOT( 1 );
    return Err::m_nERR;
  }

  // heiko.schwarz@hhi.fhg.de: support for BSD systems as proposed by Steffen Kamp [kamp@ient.rwth-aachen.de]
  //m_iFileHandle = open64( rcFilename.c_str(), iOpenMode, iPermMode );
  m_iFileHandle = ::open( rcFilename.c_str(), iOpenMode, iPermMode );

#endif

  // check if file is really open
  ROTS( -1 == m_iFileHandle );

  // and return
  return Err::m_nOK;
}

ErrVal LargeFile::close()
{
  int iRetv;

  ROTS( -1 == m_iFileHandle );

  iRetv = ::close( m_iFileHandle );

  m_iFileHandle = -1;

  return ( iRetv == 0 ) ? Err::m_nOK : Err::m_nERR;
}


ErrVal LargeFile::seek( Int64 iOffset, int iOrigin )
{
  Int64 iNewOffset;
  ROT( -1 == m_iFileHandle );

#if defined( MSYS_WIN32 )
  iNewOffset = _lseeki64( m_iFileHandle, iOffset, iOrigin );
#elif defined( MSYS_UNIX_LARGEFILE )
  // heiko.schwarz@hhi.fhg.de: support for BSD systems as proposed by Steffen Kamp [kamp@ient.rwth-aachen.de]
  //iNewOffset = lseek64( m_iFileHandle, iOffset, iOrigin );
  iNewOffset = ::lseek( m_iFileHandle, iOffset, iOrigin );
#endif

  return ( iNewOffset == -1 ) ? Err::m_nERR : Err::m_nOK;
}


Int64 LargeFile::tell()
{
  ROTR( -1 == m_iFileHandle, -1 );
  Int64 iOffset;
#if defined( MSYS_WIN32 )
  iOffset = _telli64( m_iFileHandle );
#elif defined( MSYS_UNIX_LARGEFILE )
  // heiko.schwarz@hhi.fhg.de: support for BSD systems as proposed by Steffen Kamp [kamp@ient.rwth-aachen.de]
  //iOffset = lseek64( m_iFileHandle, 0, SEEK_CUR );
  iOffset = ::lseek( m_iFileHandle, 0, SEEK_CUR );
#endif
  ROT( iOffset == -1 )

  // and return
  return iOffset;
}


ErrVal LargeFile::read( Void *pvBuffer, UInt32 uiCount, UInt32& ruiBytesRead )
{
  int iRetv;

  ROT( -1 == m_iFileHandle );
  ROT( 0 == uiCount );

  ruiBytesRead = 0;

  iRetv = ::read( m_iFileHandle, pvBuffer, uiCount );
  if( iRetv != (Int)uiCount )
  {
    //need to handle partial reads before hitting EOF

    //If the function tries to read at end of file, it returns 0.
    //If the handle is invalid, or the file is not open for reading,
    //or the file is locked, the function returns -1 and sets errno to EBADF.
    if( iRetv > 0 )
    {
      //partial reads are acceptable and return the standard success code. Anything
      //else must be implemented by the caller.
      ruiBytesRead = iRetv;
      return Err::m_nOK;
    }
    else if( iRetv == -1 )
    {
      return errno;
    }
    else if( iRetv == 0)
    {
      return Err::m_nEndOfFile;
    }
    else
    {
      AOF( ! "fix me, unexpected return code" );
      return Err::m_nERR;
    }
  }
  else
  {
    ruiBytesRead = uiCount;
  }

  ROF( iRetv == (Int)uiCount );

  return Err::m_nOK;
}


ErrVal LargeFile::write( const Void *pvBuffer, UInt32 uiCount )
{
  int iRetv;

  ROT( -1 == m_iFileHandle );
  ROT( 0 == uiCount );

  iRetv = ::write( m_iFileHandle, pvBuffer, uiCount );
  ROF( iRetv == (Int)uiCount );

  return Err::m_nOK;
}


