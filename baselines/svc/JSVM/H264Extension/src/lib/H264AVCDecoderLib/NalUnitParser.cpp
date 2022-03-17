
#include "H264AVCDecoderLib.h"
#include "BitReadBuffer.h"
#include "NalUnitParser.h"

#include "H264AVCCommonLib/TraceFile.h"


H264AVC_NAMESPACE_BEGIN


NalUnitParser::NalUnitParser()
: m_bInitialized        ( false )
, m_bNalUnitInitialized ( false )
, m_pcBitReadBuffer     ( 0 )
{
}

NalUnitParser::~NalUnitParser()
{
}

ErrVal
NalUnitParser::create( NalUnitParser*& rpcNalUnitParser )
{
  rpcNalUnitParser = new NalUnitParser;
  ROT( NULL == rpcNalUnitParser );
  return Err::m_nOK;
}

ErrVal
NalUnitParser::destroy()
{
  ROT( m_bInitialized );
  ROT( m_bNalUnitInitialized );
  delete this;
  return Err::m_nOK;
}

ErrVal
NalUnitParser::init( BitReadBuffer*       pcBitReadBuffer,
                     HeaderSymbolReadIf*  pcHeaderSymbolReadIf )
{
  ROT( m_bInitialized );
  ROT( m_bNalUnitInitialized );
  ROF( pcBitReadBuffer );
  ROF( pcHeaderSymbolReadIf );
  m_bInitialized          = true;
  m_bNalUnitInitialized   = false;
  m_pcBitReadBuffer       = pcBitReadBuffer;
  m_pcHeaderSymbolReadIf  = pcHeaderSymbolReadIf;
  return Err::m_nOK;
}

ErrVal
NalUnitParser::uninit()
{
  ROF( m_bInitialized );
  ROT( m_bNalUnitInitialized );
  m_bInitialized          = false;
  m_bNalUnitInitialized   = false;
  m_pcBitReadBuffer       = 0;
  m_pcHeaderSymbolReadIf  = 0;
  return Err::m_nOK;
}

ErrVal
NalUnitParser::initNalUnit( BinDataAccessor& rcBinDataAccessor, Int iDQId )
{
  ROF( m_bInitialized );
  ROT( m_bNalUnitInitialized );
  ROF( rcBinDataAccessor.size() );
  ROF( rcBinDataAccessor.data() );

  //===== remove zeros at end =====
  while( rcBinDataAccessor.size() > 1 && rcBinDataAccessor.data()[ rcBinDataAccessor.size() - 1 ] == 0 )
  {
    rcBinDataAccessor.decreaseEndPos( 1 );
  }

  //===== determine NAL unit type =====
  DTRACE_LAYER( iDQId == MSYS_INT_MIN ? 0 : iDQId );
  UChar       ucFirstByte   = rcBinDataAccessor.data()[ 0 ];
  NalUnitType eNalUnitType  = NalUnitType ( ucFirstByte & 0x1F );
  Bool        bTrailingBits = true;
  switch( eNalUnitType )
  {
  case NAL_UNIT_SPS:
    DTRACE_HEADER( "SEQUENCE PARAMETER SET" );
    break;
  case NAL_UNIT_SPS_EXTENSION:
    DTRACE_HEADER( "SEQUENCE PARAMETER SET EXTENSION" );
    break;
  case NAL_UNIT_SUBSET_SPS:
    DTRACE_HEADER( "SUBSET SEQUENCE PARAMETER SET" );
    break;
  case NAL_UNIT_PPS:
    DTRACE_HEADER( "PICTURE PARAMETER SET" );
    break;
  case NAL_UNIT_SEI:
    DTRACE_HEADER( "SEI NAL UNIT" );
    break;
  case NAL_UNIT_FILLER_DATA:
    DTRACE_HEADER( "FILLER DATA" );
    break;
  case NAL_UNIT_ACCESS_UNIT_DELIMITER:
    DTRACE_HEADER( "ACCESS UNIT DELIMITER" );
    break;
  case NAL_UNIT_END_OF_SEQUENCE:
    DTRACE_HEADER( "END OF SEQUENCE" );
    bTrailingBits = false;
    break;
  case NAL_UNIT_END_OF_STREAM:
    DTRACE_HEADER( "END OF STREAM" );
    bTrailingBits = false;
    break;
  case NAL_UNIT_PREFIX:
    DTRACE_HEADER( "PREFIX UNIT" );
    bTrailingBits = ( rcBinDataAccessor.size() > 4 );
    break;
  case NAL_UNIT_CODED_SLICE:
  case NAL_UNIT_CODED_SLICE_DATAPART_A:
  case NAL_UNIT_CODED_SLICE_DATAPART_B:
  case NAL_UNIT_CODED_SLICE_DATAPART_C:
  case NAL_UNIT_CODED_SLICE_IDR:
  case NAL_UNIT_AUX_CODED_SLICE:
  case NAL_UNIT_CODED_SLICE_SCALABLE:
    DTRACE_NEWSLICE;
    break;
  case NAL_UNIT_RESERVED_16:
  case NAL_UNIT_RESERVED_17:
  case NAL_UNIT_RESERVED_18:
  case NAL_UNIT_RESERVED_21:
  case NAL_UNIT_RESERVED_22:
  case NAL_UNIT_RESERVED_23:
    DTRACE_HEADER( "RESERVED" );
    bTrailingBits = false;
    break;
  default:
    DTRACE_HEADER( "UNSPECIFIED" );
    bTrailingBits = false;
    break;
  }

  //===== init bit read buffer and read NAL unit header =====
  RNOK( xInitSODB( rcBinDataAccessor, bTrailingBits ) );
  RNOK( NalUnitHeader::read( *m_pcHeaderSymbolReadIf ) );
  m_bNalUnitInitialized = true;

  return Err::m_nOK;
}

ErrVal
NalUnitParser::closeNalUnit( Bool bCheckEndOfNalUnit )
{
  ROF( m_bInitialized );
  ROF( m_bNalUnitInitialized );
  ROT( bCheckEndOfNalUnit && m_pcBitReadBuffer->isValid() );
  m_bNalUnitInitialized = false;
  return Err::m_nOK;
}

ErrVal
NalUnitParser::xInitSODB( BinDataAccessor&  rcBinDataAccessor,
                          Bool              bTrailingBits )
{
  UChar*  pucBuffer       = rcBinDataAccessor.data();
  UInt    uiRBSPSize      = 0;
  UInt    uiBitsInPacket  = 0;

  //===== convert payload to RBSP =====
  for( UInt uiNumZeros = 0, uiReadOffset = 0; uiReadOffset < rcBinDataAccessor.size(); uiReadOffset++ )
  {
    if( uiNumZeros == 2 && pucBuffer[ uiReadOffset ] == 0x03 )
    {
      uiReadOffset++;
      uiNumZeros = 0;
    }
    if( uiReadOffset < rcBinDataAccessor.size() )
    {
      pucBuffer[ uiRBSPSize++ ] = pucBuffer[ uiReadOffset ];

      if( pucBuffer[ uiReadOffset] == 0x00 )
      {
        uiNumZeros++;
      }
      else
      {
        uiNumZeros = 0;
      }
    }
  }

  //===== determine bits in NAL unit except trailing bits =====
  uiBitsInPacket      = ( uiRBSPSize << 3 );
  UInt  uiLastBytePos = uiRBSPSize - 1;

  //----- remove zero bytes at the end -----
  {
    while( pucBuffer[ uiLastBytePos ] == 0x00 )
    {
      uiLastBytePos --;
    }
    uiBitsInPacket  -= ( ( uiRBSPSize - uiLastBytePos - 1 ) << 3 );
  }

  //----- remove trailing bits -----
  if( bTrailingBits )
  {
    UChar ucLastByte    = pucBuffer[ uiLastBytePos ];
    UInt  uiNumZeroBits = 0;
    while( ( ucLastByte & 0x01 ) == 0x00 )
    {
      ucLastByte    >>= 1;
      uiNumZeroBits ++;
    }
    uiBitsInPacket  -= uiNumZeroBits;
  }

  //===== init bit read buffer =====
  UInt*  pulBuffer = (UInt*)pucBuffer;
  RNOK( m_pcBitReadBuffer->initPacket( pulBuffer, uiBitsInPacket ) );
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
