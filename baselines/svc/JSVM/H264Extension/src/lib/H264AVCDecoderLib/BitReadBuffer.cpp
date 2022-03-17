
#include "H264AVCDecoderLib.h"
#include "BitReadBuffer.h"
#include "DecError.h"



H264AVC_NAMESPACE_BEGIN

BitReadBuffer::BitReadBuffer()
: m_uiDWordsLeft    ( 0 )
, m_uiBitsLeft      ( 0 )
, m_iValidBits      ( 0 )
, m_ulCurrentBits   ( 0xdeaddead )
, m_uiNextBits      ( 0xdeaddead )
, m_pulStreamPacket ( 0 )
{
}

BitReadBuffer::~BitReadBuffer()
{
}

ErrVal
BitReadBuffer::create( BitReadBuffer*& rpcBitReadBuffer )
{
  rpcBitReadBuffer = new BitReadBuffer;
  ROF( rpcBitReadBuffer );
  return Err::m_nOK;
}

ErrVal
BitReadBuffer::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal
BitReadBuffer::initPacket( UInt*   puiBits,
                           UInt    uiBitsInPacket )
{
  // invalidate all members if something is wrong
  m_pulStreamPacket    = NULL;
  m_ulCurrentBits      = 0xdeaddead;
  m_uiNextBits         = 0xdeaddead;
  m_uiBitsLeft         = 0;
  m_iValidBits         = 0;
  m_uiDWordsLeft     = 0;

  ROT( NULL == puiBits );

  // now init the Bitstream object
  m_pulStreamPacket  = puiBits;

  m_uiBitsLeft = uiBitsInPacket;

  m_uiDWordsLeft = m_uiBitsLeft >> 5;
  m_iValidBits     = -32;

  // preload first two dwords if valid
  xReadNextWord();
  xReadNextWord();

  return Err::m_nOK;
}

ErrVal
BitReadBuffer::flush( UInt uiNumberOfBits  )
{
  // check the number_of_bits parameter matches the range
  AOF_DBG( uiNumberOfBits <= 32 );

  DECROTR( uiNumberOfBits > m_uiBitsLeft, Err::m_nEndOfBuffer );

  // sub the desired number of bits
  m_uiBitsLeft -= uiNumberOfBits;
  m_iValidBits -= uiNumberOfBits;

  // check the current word for beeing still valid
  if( 0 < m_iValidBits )
  {
    m_ulCurrentBits <<= uiNumberOfBits;
    return Err::m_nOK;
  }

  xReadNextWord();

  // shift to the right position
  m_ulCurrentBits <<= 32 - m_iValidBits;

  return Err::m_nOK;
}


ErrVal
BitReadBuffer::get( UInt& ruiBits  )
{
  if( 0 == m_uiBitsLeft )
  {
    return Err::m_nEndOfStream;
  }

  m_uiBitsLeft --;
  m_iValidBits --;

  // mask out the value
  ruiBits  = m_ulCurrentBits >> 31;

  //prepare for next access
  m_ulCurrentBits <<= 1;

  // check the current word for beeing empty
  ROTRS( 0 < m_iValidBits, Err::m_nOK );

  xReadNextWord();

  return Err::m_nOK;
}


ErrVal
BitReadBuffer::get( UInt& ruiBits, UInt uiNumberOfBits  )
{
  UInt ui_right_shift;

  // check the number_of_bits parameter matches the range
  AOT_DBG( uiNumberOfBits > 32 );

  ROT( uiNumberOfBits > m_uiBitsLeft );

  m_uiBitsLeft  -= uiNumberOfBits;
  m_iValidBits -= uiNumberOfBits;

  if( 0 <= m_iValidBits )
  {
    // calculate the number of bits to extract the desired number of bits
    ui_right_shift = 32 - uiNumberOfBits ;

    // mask out the value
    ruiBits  = m_ulCurrentBits >> ui_right_shift;

    //prepare for next access
    m_ulCurrentBits = m_ulCurrentBits << uiNumberOfBits;
  }
  else
  {
    // mask out the value in the current word
    ruiBits = m_ulCurrentBits;

    // calculate the number of bits to extract the desired number of bits
    ui_right_shift = m_iValidBits + uiNumberOfBits ;

    // mask out the value in the next word
    ruiBits |= m_uiNextBits >> ui_right_shift;

    ruiBits >>= 32 - uiNumberOfBits;

    m_uiNextBits <<=  -m_iValidBits;
  }

  // check the current word for beeing empty
  ROTRS( 0 < m_iValidBits, Err::m_nOK );

  xReadNextWord();

  return Err::m_nOK;
}


ErrVal
BitReadBuffer::pcmSamples ( TCoeff* pCoeff, UInt uiNumberOfSamples )
{
  AOF_DBG( isByteAligned() );
  // can be done in a faster way
  for( UInt n = 0; n < uiNumberOfSamples; n++)
  {
    UInt uiTemp;
    DECRNOK( get( uiTemp, 8) );
    pCoeff[n].setLevel( uiTemp );
  }
  return Err::m_nOK;
}


__inline Void
BitReadBuffer::xReadNextWord()
{
  m_ulCurrentBits = m_uiNextBits;
  m_iValidBits += 32;

  // chech if there are bytes left in the packet
  if( m_uiDWordsLeft )
  {
    // read 32 bit from the packet
    m_uiNextBits = xSwap( *m_pulStreamPacket++ );
    m_uiDWordsLeft--;
  }
  else
  {
    Int iBytesLeft  = ((Int)m_uiBitsLeft - m_iValidBits+7) >> 3;
    UChar* puc      = (UChar*) m_pulStreamPacket;
    m_uiNextBits  = 0;

    if( iBytesLeft > 0)
    {
      for( Int iByte = 0; iByte < iBytesLeft; iByte++ )
      {
        m_uiNextBits <<= 8;
        m_uiNextBits += puc[iByte];
      }
      m_uiNextBits <<= (4-iBytesLeft)<<3;
    }
  }
}


H264AVC_NAMESPACE_END
