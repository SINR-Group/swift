
#include "H264AVCVideoIoLib.h"
#include "WriteYuvToFile.h"


WriteYuvToFile::WriteYuvToFile()
{
}

WriteYuvToFile::~WriteYuvToFile()
{
}

ErrVal
WriteYuvToFile::create( WriteYuvToFile*& rpcWriteYuv )
{
  rpcWriteYuv = new WriteYuvToFile;
  ROF( rpcWriteYuv )
  return Err::m_nOK;
}

ErrVal
WriteYuvToFile::destroy()
{
  ROT( m_cFile.is_open() );
  delete this;
  return Err::m_nOK;
}

ErrVal
WriteYuvToFile::init( const std::string& rcFileName )
{
  ROF( rcFileName.size() );
#if 1  // DOLBY_ENCMUX_ENABLE
  if( rcFileName.compare("none") && rcFileName.compare("\"\"") )
#endif // DOLBY_ENCMUX_ENABLE
  {
    if( Err::m_nOK != m_cFile.open( rcFileName, LargeFile::OM_WRITEONLY ) )
    {
      std::cerr << "failed to open YUV output file " << rcFileName.data() << std::endl;
      return Err::m_nERR;
    }
  }
  return Err::m_nOK;
}

ErrVal
WriteYuvToFile::uninit()
{
  if( m_cFile.is_open() )
  {
    RNOK( m_cFile.close() );
  }
  return Err::m_nOK;
}

ErrVal
WriteYuvToFile::writeFrame( const UChar* pLum,
                            const UChar* pCb,
                            const UChar* pCr,
                            UInt         uiHeight,
                            UInt         uiWidth,
                            UInt         uiStride,
                            const UInt   rauiCropping[] )
{
  ROFRS( m_cFile.is_open(), Err::m_nOK );

  UInt          y;
  const UChar*  pucSrc;

  uiWidth   -= rauiCropping[0] + rauiCropping[1];
  uiHeight  -= rauiCropping[2] + rauiCropping[3];

  pucSrc = pLum + ( rauiCropping[0] + rauiCropping[2] * uiStride );
  for( y = 0; y < uiHeight; y++ )
  {
    RNOK( m_cFile.write( pucSrc, uiWidth ) );
    pucSrc += uiStride;
  }

  uiStride >>= 1;
  uiHeight >>= 1;
  uiWidth  >>= 1;

  pucSrc = pCb + ( ( rauiCropping[0] + rauiCropping[2] * uiStride ) >> 1 );
  for( y = 0; y < uiHeight; y++ )
  {
    RNOK( m_cFile.write( pucSrc, uiWidth ) );
    pucSrc += uiStride;
  }

  pucSrc = pCr + ( ( rauiCropping[0] + rauiCropping[2] * uiStride ) >> 1 );
  for( y = 0; y < uiHeight; y++ )
  {
    RNOK( m_cFile.write( pucSrc, uiWidth ) );
    pucSrc += uiStride;
  }

  return Err::m_nOK;
}

