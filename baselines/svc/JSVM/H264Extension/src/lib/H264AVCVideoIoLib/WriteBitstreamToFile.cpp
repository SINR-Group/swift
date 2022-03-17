
#include "H264AVCVideoIoLib.h"
#include "WriteBitstreamToFile.h"


ErrVal WriteBitstreamToFile::create( WriteBitstreamToFile*& rpcWriteBitstreamToFile )
{
  rpcWriteBitstreamToFile = new WriteBitstreamToFile;
  ROT( NULL == rpcWriteBitstreamToFile );
  return Err::m_nOK;
}



ErrVal WriteBitstreamToFile::init( const std::string& rcFileName, Bool bNewFileOnNewAu )
{
  m_bNewFileOnNewAu = bNewFileOnNewAu;
  m_cFileName = rcFileName;
  if( Err::m_nOK != m_cFile.open( rcFileName, LargeFile::OM_WRITEONLY ) )
  {
    std::cerr << "Failed to create output bitstream " << rcFileName.data() << std::endl;
    return Err::m_nERR;
  }

  m_uiNumber = 0;
  return Err::m_nOK;
}

ErrVal WriteBitstreamToFile::uninit()
{
  if( m_cFile.is_open() )
  {
    RNOK( m_cFile.close() );
  }
  return Err::m_nOK;
}

ErrVal WriteBitstreamToFile::destroy()
{
  ROT( m_cFile.is_open() );
  RNOK( uninit() );
  delete this;
  return Err::m_nOK;
}

ErrVal WriteBitstreamToFile::writePacket( BinData* pcBinData, Bool bNewAU )
{
  BinDataAccessor cBinDataAccessor;
  pcBinData->setMemAccessor( cBinDataAccessor );
  RNOK( writePacket( &cBinDataAccessor, bNewAU ) );
  return Err::m_nOK;
}

ErrVal WriteBitstreamToFile::writePacket( BinDataAccessor* pcBinDataAccessor, Bool bNewAU )
{
  ROTRS( NULL == pcBinDataAccessor, Err::m_nOK );

  if( bNewAU && m_bNewFileOnNewAu )
  {
#if defined MSYS_WIN32
    if( m_cFile.is_open() )
    {
      RNOK( m_cFile.close() );
    }

    std::string cFileName = m_cFileName;
    Int iPos = (Int)cFileName.find_last_of(".");

    Char acBuffer[20];
    itoa( ++m_uiNumber, acBuffer, 10 );
    cFileName.insert( iPos, acBuffer );
    if( Err::m_nOK != m_cFile.open( cFileName, LargeFile::OM_WRITEONLY ) )
    {
      std::cerr << "Failed to create output bitstream " << cFileName.data() << std::endl;
      return Err::m_nERR;
    }
#else
   std::cerr << "multiple output bitstreams only supported in Win32";
   AF();
#endif
  }

  if( 0 != pcBinDataAccessor->size())
  {
    RNOK( m_cFile.write( pcBinDataAccessor->data(), pcBinDataAccessor->size() ) );
  }

  return Err::m_nOK;
}

ErrVal
WriteBitstreamToFile::writePacket( Void* pBuffer, UInt uiLength )
{
  if( uiLength )
  {
    RNOK( m_cFile.write( pBuffer, uiLength ) );
  }
  return Err::m_nOK;
}
