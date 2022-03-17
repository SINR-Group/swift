
#include "H264AVCEncoderLib.h"
#include "BitWriteBuffer.h"

H264AVC_NAMESPACE_BEGIN


BitWriteBuffer::BitWriteBuffer():
  m_uiDWordsLeft    ( 0 ),
  m_uiBitsWritten   ( 0 ),
  m_iValidBits      ( 0 ),
  m_ulCurrentBits   ( 0 ),
  m_pulStreamPacket ( NULL )
, m_uiInitPacketLength( 0 )
, m_pcNextBitWriteBuffer( NULL )
, m_pucNextStreamPacket( NULL )
{
}

BitWriteBuffer::~BitWriteBuffer()
{
}


ErrVal BitWriteBuffer::init()
{
  m_uiDWordsLeft    = 0;
  m_uiBitsWritten   = 0;
  m_iValidBits      = 0;
  m_ulCurrentBits   = 0;
  m_pulStreamPacket = NULL;
  if( m_pucNextStreamPacket )
  {
    delete[] m_pucNextStreamPacket;
    m_pucNextStreamPacket = NULL;
  }
  if( m_pcNextBitWriteBuffer )
  {
    m_pcNextBitWriteBuffer->uninit();
  }
  m_uiInitPacketLength = 0;
  return Err::m_nOK;
}


BitWriteBufferIf* BitWriteBuffer::getNextBitWriteBuffer( Bool bStartNewBitstream  )
{
  if( bStartNewBitstream )
  {
    if( !m_pcNextBitWriteBuffer )
    {
      BitWriteBuffer::create( m_pcNextBitWriteBuffer );
      m_pcNextBitWriteBuffer->init();
    }

    AOT( m_pucNextStreamPacket );
    m_pucNextStreamPacket = new UChar [m_uiInitPacketLength + 1];
    m_pcNextBitWriteBuffer->initPacket( (UInt*)m_pucNextStreamPacket, m_uiInitPacketLength );
  }
  else
  {
    AOF( m_pcNextBitWriteBuffer );
    AOF( m_pucNextStreamPacket );
  }
  return m_pcNextBitWriteBuffer;
}


ErrVal BitWriteBuffer::create( BitWriteBuffer*& rpcBitWriteBuffer )
{
  rpcBitWriteBuffer = new BitWriteBuffer;

  ROT( NULL == rpcBitWriteBuffer );

  return Err::m_nOK;
}

ErrVal BitWriteBuffer::destroy()
{
  if( m_pcNextBitWriteBuffer )
  {
    m_pcNextBitWriteBuffer->destroy();
    m_pcNextBitWriteBuffer = NULL;
  }
  delete this;

  return Err::m_nOK;
}


ErrVal BitWriteBuffer::initPacket( UInt* pulBits, UInt uiPacketLength )
{
  // invalidate all members if something is wrong
  uninit();

  m_uiInitPacketLength = uiPacketLength;

  // check the parameter
  ROT( uiPacketLength < 4);
  ROT( NULL == pulBits );

  // now init the Bitstream object
  m_pulStreamPacket = pulBits;
  m_uiDWordsLeft = (uiPacketLength + 3) / 4;
  m_iValidBits    = 32;
  return Err::m_nOK;

}
//FIX_FRAG_CAVLC
ErrVal BitWriteBuffer::getLastByte(UChar &uiLastByte, UInt &uiLastBitPos)
{
  UInt uiBytePos = m_iValidBits/8;
  if(m_iValidBits%8 != 0)
  {
    uiBytePos = uiBytePos*8;
    uiLastByte = (UChar)(m_ulCurrentBits >> uiBytePos);
    uiLastBitPos = (m_iValidBits/8+1)*8-m_iValidBits;
    uiLastByte = (uiLastByte >> (8-uiLastBitPos));
  }
  else
  {
    uiLastByte = 0;
    uiLastBitPos = 0;
  }
  return Err::m_nOK;
}
//~FIX_FRAG_CAVLC
ErrVal BitWriteBuffer::write( UInt uiBits, UInt uiNumberOfBits )
{
  if( uiNumberOfBits > 32 )
  {
    RNOK( write( 0x00, uiNumberOfBits - 32 ) );
    uiNumberOfBits = 32;
  }

  AOF_DBG( ! ( (uiBits >> 1) >> (uiNumberOfBits - 1)) ); // because shift with 32 has no effect

  m_uiBitsWritten += uiNumberOfBits;

  if( (Int)uiNumberOfBits < m_iValidBits)  // one word
  {
    m_iValidBits -= uiNumberOfBits;

    m_ulCurrentBits |= uiBits << m_iValidBits;

    return Err::m_nOK;
  }


  ROT( 0 == m_uiDWordsLeft );
  m_uiDWordsLeft--;

  UInt uiShift = uiNumberOfBits - m_iValidBits;

  // add the last bits
  m_ulCurrentBits |= uiBits >> uiShift;

  *m_pulStreamPacket++ = xSwap( m_ulCurrentBits );


  // note: there is a problem with left shift with 32
  m_iValidBits = 32 - uiShift;

  m_ulCurrentBits = uiBits << m_iValidBits;

  if( 0 == uiShift )
  {
    m_ulCurrentBits = 0;
  }


  return Err::m_nOK;
}


ErrVal BitWriteBuffer::writeAlignZero()
{
  return write( 0, m_iValidBits & 0x7 );
}

ErrVal BitWriteBuffer::writeAlignOne()
{
  return write( ( 1 << (m_iValidBits & 0x7) ) - 1, m_iValidBits & 0x7 );
}


ErrVal BitWriteBuffer::flushBuffer()
{
  ROT( 0 == m_uiDWordsLeft );

  *m_pulStreamPacket = xSwap( m_ulCurrentBits );

  m_uiBitsWritten = (m_uiBitsWritten+7)/8;

  m_uiBitsWritten *= 8;
  if( nextBitWriteBufferActive() )
  {
    getNextBitWriteBuffer( false )->flushBuffer();
  }
  return Err::m_nOK;
}



ErrVal
BitWriteBuffer::pcmSamples( const TCoeff* pCoeff, UInt uiNumberOfSamples )
{
  for( UInt n = 0; n < uiNumberOfSamples; n++)
  {
    RNOK( write( pCoeff[n].getLevel(), 8) );
  }
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END

