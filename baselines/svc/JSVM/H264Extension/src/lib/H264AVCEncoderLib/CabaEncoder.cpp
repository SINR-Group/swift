
#include "H264AVCEncoderLib.h"
#include "H264AVCCommonLib/CabacTables.h"
#include "H264AVCCommonLib/CabacContextModel.h"
#include "H264AVCCommonLib/TraceFile.h"
#include "CabaEncoder.h"



H264AVC_NAMESPACE_BEGIN


CabaEncoder::CabaEncoder() :
  m_pcBitWriteBufferIf( NULL ),
  m_uiRange( 0 ),
  m_uiLow( 0 ),
  m_uiByte( 0 ),
  m_uiBitsLeft( 0 ),
  m_uiBitsToFollow( 0 ),
  m_bTraceEnable(true)
{
}

CabaEncoder::~CabaEncoder()
{
}


__inline ErrVal CabaEncoder::xWriteBit( UInt uiBit)
{
  m_uiByte += m_uiByte + uiBit;
  if( ! --m_uiBitsLeft )
  {
    const UInt uiByte = m_uiByte;
    m_uiBitsLeft = 8;
    m_uiByte     = 0;
    return m_pcBitWriteBufferIf->write( uiByte, 8);
  }
  return Err::m_nOK;
}


__inline ErrVal CabaEncoder::xWriteBitAndBitsToFollow( UInt uiBit)
{
  RNOK( xWriteBit( uiBit ) );
  // invert bit
  uiBit = 1-uiBit;

  while( m_uiBitsToFollow > 0)
  {
    m_uiBitsToFollow--;
    RNOK( xWriteBit( uiBit ) );
  }
  return Err::m_nOK;
}


ErrVal CabaEncoder::init( BitWriteBufferIf* pcBitWriteBufferIf )
{
  ROT( NULL == pcBitWriteBufferIf )

  m_pcBitWriteBufferIf = pcBitWriteBufferIf;

  return Err::m_nOK;
}


ErrVal CabaEncoder::start()
{
  m_uiRange = HALF-2;
  m_uiLow = 0;
  m_uiBitsToFollow = 0;
  m_uiByte = 0;
  m_uiBitsLeft = 9;

  RNOK( m_pcBitWriteBufferIf->writeAlignOne() );
  return Err::m_nOK;
}

//FIX_FRAG_CAVLC
ErrVal CabaEncoder::getLastByte(UChar &uiLastByte, UInt &uiLastBitPos)
{
  RNOK(m_pcBitWriteBufferIf->getLastByte(uiLastByte, uiLastBitPos));
  return Err::m_nOK;
}
ErrVal CabaEncoder::setFirstBits(UChar ucByte,UInt uiLastBitPos)
{
  RNOK( m_pcBitWriteBufferIf->write(ucByte,uiLastBitPos));
  return Err::m_nOK;
}
//~FIX_FRAG_CAVLC
ErrVal CabaEncoder::uninit()
{
  m_pcBitWriteBufferIf = NULL;
  m_uiRange = 0;
  return Err::m_nOK;
}


ErrVal CabaEncoder::writeUnaryMaxSymbol( UInt uiSymbol, CabacContextModel* pcCCModel, Int iOffset, UInt uiMaxSymbol )
{
  RNOK( writeSymbol( uiSymbol ? 1 : 0, pcCCModel[ 0 ] ) );

  ROTRS( 0 == uiSymbol, Err::m_nOK );

  Bool bCodeLast = ( uiMaxSymbol > uiSymbol );

  while( --uiSymbol )
  {
    RNOK( writeSymbol( 1, pcCCModel[ iOffset ] ) );
  }
  if( bCodeLast )
  {
    RNOK( writeSymbol( 0, pcCCModel[ iOffset ] ) );
  }

  return Err::m_nOK;
}


ErrVal CabaEncoder::writeExGolombLevel( UInt uiSymbol, CabacContextModel& rcCCModel  )
{
  if( uiSymbol )
  {
    RNOK( writeSymbol( 1, rcCCModel ) );
    UInt uiCount = 0;
    Bool bNoExGo = (uiSymbol < 13);

    while( --uiSymbol && ++uiCount < 13 )
    {
      RNOK( writeSymbol( 1, rcCCModel ) );
    }
    if( bNoExGo )
    {
      RNOK( writeSymbol( 0, rcCCModel ) );
    }
    else
    {
      RNOK( writeEpExGolomb( uiSymbol, 0 ) );
    }
  }
  else
  {
    RNOK( writeSymbol( 0, rcCCModel ) );
  }

  return Err::m_nOK;
}


ErrVal CabaEncoder::writeEpExGolomb( UInt uiSymbol, UInt uiCount )
{
  while( uiSymbol >= (UInt)(1<<uiCount) )
  {
    writeEPSymbol( 1 );
    uiSymbol -= 1<<uiCount;
    uiCount  ++;
  }
  writeEPSymbol( 0 );
  while( uiCount-- )
  {
    writeEPSymbol( (uiSymbol>>uiCount) & 1 );
  }

  return Err::m_nOK;
}


ErrVal CabaEncoder::writeExGolombMvd( UInt uiSymbol, CabacContextModel* pcCCModel, UInt uiMaxBin )
{
  if( ! uiSymbol )
  {
    RNOK( writeSymbol( 0, *pcCCModel ) );
    return Err::m_nOK;
  }

  RNOK( writeSymbol( 1, *pcCCModel ) );

  Bool  bNoExGo = ( uiSymbol < 8 );
  UInt  uiCount = 1;
  pcCCModel++;

  while( --uiSymbol && ++uiCount <= 8 )
  {
    RNOK( writeSymbol( 1, *pcCCModel ) );
    if( uiCount == 2 )
    {
      pcCCModel++;
    }
    if( uiCount == uiMaxBin )
    {
      pcCCModel++;
    }
  }

  if( bNoExGo )
  {
    RNOK( writeSymbol( 0, *pcCCModel ) );
  }
  else
  {
    RNOK( writeEpExGolomb( uiSymbol, 3 ) );
  }

  return Err::m_nOK;
}



ErrVal CabaEncoder::writeUnarySymbol( UInt uiSymbol, CabacContextModel* pcCCModel, Int iOffset )
{
  RNOK( writeSymbol( uiSymbol ? 1 : 0, pcCCModel[0] ) );

  ROTRS( 0 == uiSymbol, Err::m_nOK );

  while( uiSymbol-- )
  {
    RNOK( writeSymbol( uiSymbol ? 1 : 0, pcCCModel[ iOffset ] ) );
  }

  return Err::m_nOK;
}


ErrVal CabaEncoder::finish()
{
  RNOK( xWriteBitAndBitsToFollow( (m_uiLow >> (B_BITS-1)) & 1 ) );
  RNOK( xWriteBit(                (m_uiLow >> (B_BITS-2)) & 1 ) );

  RNOK( m_pcBitWriteBufferIf->write( m_uiByte, 8 - m_uiBitsLeft ) );

  return Err::m_nOK;
}


ErrVal CabaEncoder::writeSymbol( UInt uiSymbol, CabacContextModel& rcCCModel )
{
  ETRACE_SC;
  ETRACE_TH ("  ");
  ETRACE_X (m_uiRange);
  ETRACE_TH ("  ");
  ETRACE_V (rcCCModel.getState());
  ETRACE_TH ("  ");
  ETRACE_V (rcCCModel.getMps());
  ETRACE_TH ("  -  ");
  ETRACE_V (uiSymbol);
  ETRACE_N;

  UInt uiLow    = m_uiLow;
  UInt uiRange  = m_uiRange;
  UInt uiLPS = g_aucLPSTable64x4[rcCCModel.getState()][(uiRange>>6) & 3];

  AOT_DBG( 1 < uiSymbol );

  rcCCModel.incrementCount();

  uiRange -= uiLPS;
  if( uiSymbol != rcCCModel.getMps() )
  {
    uiLow += uiRange;
    uiRange = uiLPS;

    if( ! rcCCModel.getState() )
    {
      rcCCModel.toggleMps();
    }
    rcCCModel.setState( g_aucACNextStateLPS64[rcCCModel.getState()] );
  }
  else
  {
    rcCCModel.setState( g_aucACNextStateMPS64[rcCCModel.getState()] );
  }

  while( uiRange < QUARTER )
  {
    if( uiLow >= HALF )
    {
      RNOK( xWriteBitAndBitsToFollow( 1 ) );
      uiLow -= HALF;
    }
    else if( uiLow < QUARTER )
    {
      RNOK( xWriteBitAndBitsToFollow( 0 ) );
    }
    else
    {
      m_uiBitsToFollow++;
      uiLow -= QUARTER;
    }
    uiLow   <<= 1;
    uiRange <<= 1;
  }

  m_uiLow   = uiLow;
  m_uiRange = uiRange;

  return Err::m_nOK;
}


ErrVal CabaEncoder::writeEPSymbol( UInt uiSymbol )
{
  ETRACE_SC;
  ETRACE_TH ("  ");
  ETRACE_X(m_uiRange);
  ETRACE_TH ("  -  ");
  ETRACE_V (uiSymbol);
  ETRACE_N;

  UInt uiLow = m_uiLow<<1;

  if( uiSymbol != 0 )
  {
    uiLow += m_uiRange;
  }

  if (uiLow >= ONE)
  {
    RNOK( xWriteBitAndBitsToFollow( 1 ) );
    uiLow -= ONE;
  }
  else if (uiLow < HALF)
  {
    RNOK( xWriteBitAndBitsToFollow( 0 ) );
  }
  else
  {
    m_uiBitsToFollow++;
    uiLow -= HALF;
  }

  m_uiLow = uiLow;

  return Err::m_nOK;
}


ErrVal CabaEncoder::writeTerminatingBit( UInt uiBit )
{
  ETRACE_SC;
  ETRACE_TH ("  ");
  ETRACE_X(m_uiRange);
  ETRACE_TH ("  -  ");
  ETRACE_V (uiBit);
  ETRACE_N;

  UInt uiRange = m_uiRange - 2;
  UInt uiLow   = m_uiLow;

  if( uiBit )
  {
     uiLow += uiRange;
    uiRange = 2;
  }

  while( uiRange < QUARTER )
  {
    if( uiLow >= HALF )
    {
      RNOK( xWriteBitAndBitsToFollow( 1 ) );
      uiLow -= HALF;
    }
    else if( uiLow < QUARTER )
    {
      RNOK( xWriteBitAndBitsToFollow( 0 ) );
    }
    else
    {
      m_uiBitsToFollow++;
      uiLow -= QUARTER;
    }
    uiLow   <<= 1;
    uiRange <<= 1;
  }

  m_uiRange = uiRange;
  m_uiLow   = uiLow;

  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
