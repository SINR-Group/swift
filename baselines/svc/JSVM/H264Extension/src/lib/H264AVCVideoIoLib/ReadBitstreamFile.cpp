
#include "ReadBitstreamFile.h"


ReadBitstreamFile::ReadBitstreamFile()
{
}


ReadBitstreamFile::~ReadBitstreamFile()
{
}

ErrVal ReadBitstreamFile::releasePacket( BinData* pcBinData )
{
  ROFRS( pcBinData, Err::m_nOK );
  pcBinData->deleteData();
  delete pcBinData;
  return Err::m_nOK;
}

ErrVal ReadBitstreamFile::extractPacket( BinData*& rpcBinData, Bool& rbEOS )
{
  UInt   dwBytesRead;
  UInt   dwLength = 0;
  Int64  iPos;
  UInt   n;
  UChar  Buffer[0x400];
  UChar  *puc = NULL;
  UInt   uiCond;
  UInt  uiZeros;

  ROT( NULL == ( rpcBinData = new BinData ) );

  rbEOS     = false;
  // exit if there's no bitstream
  ROFS( m_cFile.is_open());

  // we max read any number of zeros
  uiZeros = 0;
  do
  {
    m_cFile.read( Buffer, 1, dwBytesRead );
    uiZeros++;
  } while ((dwBytesRead==1)&&(Buffer[0]==0));

  if( 0 == dwBytesRead )
  {
    rbEOS = true;
    return Err::m_nOK;
  }

  // next we expect "0x01"
  ROTS(Buffer[0]!=0x01);

  // there is a min of two zeros in a startcode
  ROTS(uiZeros<2);

  // get the current position
  iPos = m_cFile.tell();

   // read at first 0x400 bytes
  m_cFile.read( Buffer, 0x400, dwBytesRead );

  ROFS( dwBytesRead );

  do
  {
    if( dwBytesRead == 1 )
    {
      n = 1;
      break;
    }

    puc = Buffer;
    uiCond = 0;

    for( n = 0; n < dwBytesRead-2; n++, puc++)
    {
      uiCond = (puc[0] == 0 && puc[1] == 0 && puc[2] == 1 );
      if( uiCond )
      {
         break;
      }
    }

    if( uiCond )
    {
      break;
    }

    // found no synch so go on
    dwLength += dwBytesRead-2 ;
    // step 3 bytes back in the stream
    RNOK( m_cFile.seek( -2, SEEK_CUR ) );

    // read the next chunk or return out of bytes
    m_cFile.read( Buffer, 0x400, dwBytesRead );

    if( 2 == dwBytesRead )
    {
      n = 2;
      // end of stream cause we former stepped 4 bytes back
      break;      // this is the last pack
    }
  }
  while( true );

  // calc the complete length
  dwLength += n;

  if( 0 == dwLength )
  {
    rbEOS = true;
    return Err::m_nOK;
  }

  rpcBinData->set( new UChar[dwLength], dwLength );
  ROT( NULL == rpcBinData->data() );

  // seek the bitstream to the prev start position
  RNOK( m_cFile.seek( iPos, SEEK_SET ) );

  // read the access unit into the transport buffer
  RNOK( m_cFile.read( rpcBinData->data(), dwLength, dwBytesRead ) );

  return Err::m_nOK;
}


ErrVal ReadBitstreamFile::init( const std::string& rcFileName )
{
  // try to open the bitstream binary
  if ( Err::m_nOK != m_cFile.open( rcFileName, LargeFile::OM_READONLY ) )
  {
    std::cerr << "failed to open input bitstream file " << rcFileName.data() << std::endl;
    return Err::m_nERR;
  }

  UChar  aucBuffer[0x4];
  UInt uiBytesRead;
  RNOK( m_cFile.read( aucBuffer, 4, uiBytesRead ) );

  UInt uiStartPos = (aucBuffer[0] == 0 && aucBuffer[1] == 0 && aucBuffer[2] == 1) ? 1 : 0;

  RNOK( m_cFile.seek( uiStartPos, SEEK_SET ) );

  return Err::m_nOK;
}

ErrVal ReadBitstreamFile::create( ReadBitstreamFile*& rpcReadBitstreamFile )
{
  rpcReadBitstreamFile = new ReadBitstreamFile;
  ROT( NULL == rpcReadBitstreamFile );
  return Err::m_nOK;
}


ErrVal ReadBitstreamFile::getPosition( Int& iPos )
{
  ROFS( m_cFile.is_open());

  iPos = (Int)m_cFile.tell();

  return Err::m_nOK;
}


ErrVal ReadBitstreamFile::setPosition( Int  iPos )
{
  ROFS( m_cFile.is_open());

  // seek the bitstream to the prev start position
  RNOK( m_cFile.seek( iPos, SEEK_SET ) );

  return Err::m_nOK;
}


ErrVal ReadBitstreamFile::uninit()
{
  if( m_cFile.is_open() )
  {
    RNOK( m_cFile.close() );
  }
  return Err::m_nOK;
}

ErrVal ReadBitstreamFile::destroy()
{
  AOT_DBG( m_cFile.is_open() );

  RNOK( uninit() );

  delete this;
  return Err::m_nOK;
}
