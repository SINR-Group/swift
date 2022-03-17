
#include "H264AVCVideoIoLib.h"
#include "ReadYuvFile.h"


ReadYuvFile::ReadYuvFile()
: m_uiLumPicHeight( 0 )
, m_uiLumPicWidth ( 0 )
, m_uiStartLine   ( 0 )
, m_uiEndLine     ( MSYS_UINT_MAX )
, m_eFillMode     ( FILL_CLEAR )
{
}

ReadYuvFile::~ReadYuvFile()
{
}


ErrVal ReadYuvFile::create( ReadYuvFile*& rpcReadYuvFile )
{
  rpcReadYuvFile = new ReadYuvFile;
  ROT( NULL == rpcReadYuvFile );
  return Err::m_nOK;
}


ErrVal ReadYuvFile::destroy()
{
  AOT_DBG( m_cFile.is_open() );
  RNOK( uninit() )
  delete this;
  return Err::m_nOK;
}

ErrVal ReadYuvFile::uninit()
{
  if( m_cFile.is_open() )
  {
    RNOK( m_cFile.close() );
  }
  m_uiLumPicHeight = 0;
  m_uiLumPicWidth  = 0;
  return Err::m_nOK;
}


ErrVal ReadYuvFile::init( const std::string& rcFileName, UInt uiLumPicHeight, UInt uiLumPicWidth, UInt uiStartLine, UInt uiEndLine, FillMode eFillMode )
{
  ROT( 0 == uiLumPicHeight );
  ROT( 0 == uiLumPicWidth );

  m_uiLumPicWidth  = uiLumPicWidth;
  m_uiLumPicHeight = uiLumPicHeight;

  m_uiStartLine = uiStartLine;
  m_uiEndLine   = uiEndLine;
  m_eFillMode   = eFillMode;

  if( Err::m_nOK != m_cFile.open( rcFileName, LargeFile::OM_READONLY ) )
  {
    std::cerr << "failed to open YUV input file " << rcFileName.data() << std::endl;
    return Err::m_nERR;
  }

  return Err::m_nOK;
}




ErrVal ReadYuvFile::xReadPlane( UChar *pucDest, UInt uiBufHeight, UInt uiBufWidth, UInt uiBufStride, UInt uiPicHeight, UInt uiPicWidth, UInt uiStartLine, UInt uiEndLine )
{
  UInt uiClearSize = uiBufWidth - uiPicWidth;

  ROT( 0 > (Int)uiClearSize );
  ROT( uiBufHeight < uiPicHeight );

  // clear skiped buffer above reading section and skip in file
  if( 0 != uiStartLine )
  {
    UInt uiLines = uiStartLine;
    ::memset( pucDest, 0, uiBufWidth * uiLines );
    pucDest += uiBufStride * uiLines;
    RNOKRS(m_cFile.seek( uiPicWidth * uiLines, SEEK_CUR), Err::m_nEndOfFile);
  }


  UInt uiEnd = gMin (uiPicHeight, uiEndLine);

  for( UInt yR = uiStartLine; yR < uiEnd; yR++ )
  {
    UInt uiBytesRead;
    RNOKS( m_cFile.read( pucDest, uiPicWidth, uiBytesRead ) );
    ::memset( &pucDest[uiPicWidth], 0, uiClearSize );
    pucDest += uiBufStride;
  }

  // clear skiped buffer below reading section and skip in file
  if( uiEnd != uiPicHeight )
  {
    UInt uiLines = uiPicHeight - uiEnd;
    ::memset( pucDest, 0, uiBufWidth * uiLines );
    pucDest += uiBufStride * uiLines;
    RNOKRS(m_cFile.seek( uiPicWidth * uiLines, SEEK_CUR), Err::m_nEndOfFile);
  }

  // clear remaining buffer
  if( uiPicHeight != uiBufHeight )
  {
    if( uiEnd != uiPicHeight )
    {
      UInt uiLines = uiBufHeight - uiPicHeight;
      ::memset( pucDest, 0, uiBufWidth * uiLines);
    }
    else
    {
      switch( m_eFillMode )
      {
        case FILL_CLEAR:
        {
          UInt uiLines = uiBufHeight - uiPicHeight;
          ::memset( pucDest, 0, uiBufWidth * uiLines);
        }
        break;
        case FILL_FRAME:
        {
          for( UInt y = uiPicHeight; y < uiBufHeight; y++ )
          {
            memcpy( pucDest, pucDest - uiBufStride, uiBufStride );
            pucDest += uiBufStride;
          }
        }
        break;
        case FILL_FIELD:
        {
          ROT( (uiBufHeight - uiPicHeight) & 1 );
          for( UInt y = uiPicHeight; y < uiBufHeight; y+=2 )
          {
            memcpy( pucDest, pucDest - 2*uiBufStride, 2*uiBufStride );
            pucDest += 2*uiBufStride;
          }
        }
        break;
        default:
          AF()
        break;
      }
    }
  }

  return Err::m_nOK;
}


ErrVal ReadYuvFile::readFrame( UChar *pLum,
                               UChar *pCb,
                               UChar *pCr,
                               UInt uiBufHeight,
                               UInt uiBufWidth,
                               UInt uiBufStride)
{
  ROT( uiBufHeight < m_uiLumPicHeight || uiBufWidth < m_uiLumPicWidth );

  UInt uiPicHeight = m_uiLumPicHeight;
  UInt uiPicWidth  = m_uiLumPicWidth;
  UInt uiClearSize = uiBufWidth - uiPicWidth;
  UInt uiStartLine = m_uiStartLine;
  UInt uiEndLine   = m_uiEndLine;

  RNOKS( xReadPlane( pLum, uiBufHeight, uiBufWidth, uiBufStride, uiPicHeight, uiPicWidth, uiStartLine, uiEndLine ) );

  uiPicHeight  >>= 1;
  uiPicWidth   >>= 1;
  uiClearSize  >>= 1;
  uiBufHeight  >>= 1;
  uiBufWidth   >>= 1;
  uiBufStride  >>= 1;
  uiStartLine  >>= 1;
  uiEndLine    >>= 1;

  RNOKS( xReadPlane( pCb, uiBufHeight, uiBufWidth, uiBufStride, uiPicHeight, uiPicWidth, uiStartLine, uiEndLine ) );
  RNOKS( xReadPlane( pCr, uiBufHeight, uiBufWidth, uiBufStride, uiPicHeight, uiPicWidth, uiStartLine, uiEndLine ) );

  return Err::m_nOK;

}


