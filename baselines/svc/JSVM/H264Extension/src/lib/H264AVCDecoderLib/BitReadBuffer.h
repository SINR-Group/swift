
#if !defined(AFX_BITREADBUFFER_H__F1308E37_7998_4953_9F78_FF6A3DBC22B5__INCLUDED_)
#define AFX_BITREADBUFFER_H__F1308E37_7998_4953_9F78_FF6A3DBC22B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

H264AVC_NAMESPACE_BEGIN

class BitReadBuffer
{
protected:
  BitReadBuffer();
  virtual ~BitReadBuffer();

public:
  static ErrVal create( BitReadBuffer*& rpcBitReadBuffer );
  ErrVal destroy();
  ErrVal init   () { return Err::m_nOK; }
  ErrVal uninit () { return Err::m_nOK; }

  ErrVal  initPacket  ( UInt*  puiBits,
                        UInt    uiBitsInPacket );
  Void    setModeCabac()
  { // problem with cabac, cause cabac decoder uses stop bit + trailing stuffing bits
    m_uiBitsLeft = 8 * ( ( m_uiBitsLeft + 7 ) / 8 );
  }

  ErrVal  get     ( UInt& ruiBits, UInt uiNumberOfBits );
  ErrVal  get     ( UInt& ruiBits );
  ErrVal  flush   ( UInt uiNumberOfBits );
  ErrVal  pcmSamples ( TCoeff* pCoeff, UInt uiNumberOfSamples );

  Int     getBitsUntilByteAligned ()  { return m_iValidBits & 0x7;  }
  Bool    isWordAligned           ()  { return ( 0 == ( m_iValidBits & 0x1f ) ); }
  Bool    isByteAligned           ()  { return ( 0 == ( m_iValidBits & 0x7  ) ); }
  Bool    isValid                 ()  { return ( m_uiBitsLeft > 1 ); }

private:
  __inline Void xReadNextWord();

  UInt  xSwap( UInt ul )
  {
#ifdef MSYS_BIG_ENDIAN
    return ul;
#else
    UInt ul2;
    ul2  = ul>>24;
    ul2 |= (ul>>8) & 0x0000ff00;
    ul2 |= (ul<<8) & 0x00ff0000;
    ul2 |= ul<<24;
    return ul2;
#endif
  }

protected:
  UInt   m_uiDWordsLeft;
  UInt   m_uiBitsLeft;
  Int    m_iValidBits;
  UInt   m_ulCurrentBits;
  UInt   m_uiNextBits;
  UInt*  m_pulStreamPacket;
};


H264AVC_NAMESPACE_END

#endif // !defined(AFX_BITREADBUFFER_H__F1308E37_7998_4953_9F78_FF6A3DBC22B5__INCLUDED_)
