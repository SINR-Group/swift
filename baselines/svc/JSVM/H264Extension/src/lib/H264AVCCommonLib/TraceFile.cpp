
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/TraceFile.h"



#ifdef MSYS_WIN32
#define snprintf _snprintf
#endif


H264AVC_NAMESPACE_BEGIN


ErrVal
TraceFile::TraceDQId::create( TraceDQId*& rpcTraceDQId,
                              const Char* pucBaseName,
                              UInt        uiDQIdplus3 )
{
  rpcTraceDQId = 0;
  Char cFileName[1024];
  if( uiDQIdplus3 == 0 )
  {
    ::snprintf( cFileName, 1023, "%s_ScalSEI.txt", pucBaseName );
  }
  else if( uiDQIdplus3 == 1 )
  {
    ::snprintf( cFileName, 1023, "%s_BPSEI.txt", pucBaseName );
  }
  else if( uiDQIdplus3 == 2 )
  {
    ::snprintf( cFileName, 1023, "%s_SEI.txt", pucBaseName );
  }
  else
  {
    ::snprintf( cFileName, 1023, "%s_DQId%03d.txt", pucBaseName, Int(uiDQIdplus3-3) );
  }
  FILE*       pFile   = ::fopen( cFileName, "wt" );
  ROF( pFile );
  TraceDQId*  pTrace  = new TraceDQId( pFile );
  ROF( pTrace );
  rpcTraceDQId        = pTrace;
  return Err::m_nOK;
}

ErrVal
TraceFile::TraceDQId::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal
TraceFile::TraceDQId::output( const Char* pucLine )
{
  Int  iRes = ::fprintf( m_pFile, "%s", pucLine );
  ROT( iRes < 0 );
  ::fflush( m_pFile );
  return Err::m_nOK;
}

ErrVal
TraceFile::TraceDQId::storePos()
{
  m_filePos = ::ftell( m_pFile );
  return Err::m_nOK;
}

ErrVal
TraceFile::TraceDQId::resetPos()
{
  if( m_filePos )
  {
    ROF( ::fseek( m_pFile, (long)m_filePos, SEEK_SET ) >= 0 );
    m_filePos     = 0;
  }
  return Err::m_nOK;
}

TraceFile::TraceDQId::TraceDQId( FILE *pFile )
: m_pFile   ( pFile )
, m_filePos ( 0 )
{
}

TraceFile::TraceDQId::~TraceDQId()
{
  ::fflush( m_pFile );
  ::fclose( m_pFile );
}





Bool                  TraceFile::sm_bInit       = false;
Bool                  TraceFile::sm_bDisable    = true;
const Char*           TraceFile::sm_pucBaseName = 0;
UInt                  TraceFile::sm_uiDQIdplus3;
TraceFile::TraceDQId* TraceFile::sm_fTrace        [MAX_TRACE_LAYERS];
UInt                  TraceFile::sm_uiFrameNum    [MAX_TRACE_LAYERS];
UInt                  TraceFile::sm_uiSliceNum    [MAX_TRACE_LAYERS];
UInt                  TraceFile::sm_uiPosCounter  [MAX_TRACE_LAYERS];
UInt                  TraceFile::sm_uiSymCounter  [MAX_TRACE_LAYERS];
UInt                  TraceFile::sm_uiStorePosCnt [MAX_TRACE_LAYERS];
UInt                  TraceFile::sm_uiStoreSymCnt [MAX_TRACE_LAYERS];
Char                  TraceFile::sm_acLine        [MAX_TRACE_LAYERS][MAX_LINE_LENGTH];
Char                  TraceFile::sm_acType        [MAX_TRACE_LAYERS][16];
Char                  TraceFile::sm_acPos         [MAX_TRACE_LAYERS][16];
Char                  TraceFile::sm_acCode        [MAX_TRACE_LAYERS][16];
Char                  TraceFile::sm_acBits        [MAX_TRACE_LAYERS][MAX_BITS_LENGTH];

TraceFile::TraceFile()
{
}

TraceFile::~TraceFile()
{
}

ErrVal
TraceFile::initTrace( const Char* pucBaseName )
{
  ROT( sm_bInit );
  ROF( pucBaseName );
  sm_bInit        = true;
  sm_bDisable     = false;
  sm_pucBaseName  = pucBaseName;
  sm_uiDQIdplus3  = MSYS_UINT_MAX;
  for( UInt ui = 0; ui < MAX_TRACE_LAYERS; ui++ )
  {
    sm_fTrace       [ui]    = 0;
    sm_uiFrameNum   [ui]    = 0;
    sm_uiSliceNum   [ui]    = 0;
    sm_uiPosCounter [ui]    = 0;
    sm_uiSymCounter [ui]    = 0;
    sm_uiStorePosCnt[ui]    = 0;
    sm_uiStoreSymCnt[ui]    = 0;
    sm_acLine       [ui][0] = '\0';
    sm_acType       [ui][0] = '\0';
    sm_acPos        [ui][0] = '\0';
    sm_acCode       [ui][0] = '\0';
    sm_acBits       [ui][0] = '\0';
  }
  RNOK( setLayer( 0 ) );
  return Err::m_nOK;
}

ErrVal
TraceFile::uninitTrace()
{
  ROF( sm_bInit );
  for( UInt ui = 0; ui < MAX_TRACE_LAYERS; ui++ )
  {
    if( sm_fTrace[ui] )
    {
      sm_fTrace[ui]->destroy();
    }
  }
  sm_bInit = false;
  return Err::m_nOK;
}

ErrVal
TraceFile::disable()
{
  ROF( sm_bInit );
  sm_bDisable = true;
  return Err::m_nOK;
}

ErrVal
TraceFile::enable()
{
  ROF( sm_bInit );
  sm_bDisable = false;
  return Err::m_nOK;
}

ErrVal
TraceFile::storeFilePos()
{
  ROF( sm_bInit );
  for( UInt ui = 0; ui < MAX_TRACE_LAYERS; ui++ )
  {
    if( sm_fTrace[ui] )
    {
      RNOK( sm_fTrace[ui]->storePos() );
      sm_uiStorePosCnt[ui]  = sm_uiPosCounter[ui];
      sm_uiStoreSymCnt[ui]  = sm_uiSymCounter[ui];
    }
  }
  return Err::m_nOK;
}

ErrVal
TraceFile::resetFilePos()
{
  ROF( sm_bInit );
  for( UInt ui = 0; ui < MAX_TRACE_LAYERS; ui++ )
  {
    if( sm_fTrace[ui] )
    {
      RNOK( sm_fTrace[ui]->resetPos() );
      if( sm_uiStorePosCnt[ui] )
      {
        sm_uiPosCounter [ui] = sm_uiStorePosCnt[ui];
        sm_uiStorePosCnt[ui] = 0;
      }
      if( sm_uiStoreSymCnt[ui] )
      {
        sm_uiSymCounter [ui] = sm_uiStoreSymCnt[ui];
        sm_uiStoreSymCnt[ui] = 0;
      }
    }
  }
  return Err::m_nOK;
}

ErrVal
TraceFile::setLayer( Int iDQId )
{
  ROF( sm_bInit );
  ROF( ( iDQId >= -3 ) && ( iDQId+3 < MAX_TRACE_LAYERS ) );
  sm_uiDQIdplus3 = UInt( iDQId + 3 );
  if( sm_fTrace[ sm_uiDQIdplus3 ] == 0 )
  {
    RNOK( TraceFile::TraceDQId::create( sm_fTrace[ sm_uiDQIdplus3 ], sm_pucBaseName, sm_uiDQIdplus3 ) );
    ROF ( sm_fTrace[ sm_uiDQIdplus3 ] );
  }
  return Err::m_nOK;
}

ErrVal
TraceFile::startPicture()
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  sm_uiFrameNum[ sm_uiDQIdplus3 ]++;
  sm_uiSliceNum[ sm_uiDQIdplus3 ]=0;
  return Err::m_nOK;
}

ErrVal
TraceFile::startSlice()
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  Char acSliceHeader[256];
  ::snprintf( acSliceHeader, 255, "Slice # %u in Picture # %u", sm_uiSliceNum[ sm_uiDQIdplus3 ], sm_uiFrameNum[ sm_uiDQIdplus3 ] );
  RNOK ( printHeading( acSliceHeader, true ) );
  sm_uiSliceNum[ sm_uiDQIdplus3 ]++;
  return Err::m_nOK;
}

ErrVal
TraceFile::startMb( UInt uiMbAddress )
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  Char acMbHeader[128];
  ::snprintf( acMbHeader, 127, "MB # %u", uiMbAddress );
  RNOK ( printHeading( acMbHeader, false ) );
  return Err::m_nOK;
}

ErrVal
TraceFile::printHeading( const Char* pcString, Bool bReset )
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  Char acMbHeader[MAX_LINE_LENGTH];
  ::snprintf( acMbHeader, MAX_LINE_LENGTH-1, "-------------------- %s --------------------\n", pcString );
  RNOK( sm_fTrace[ sm_uiDQIdplus3 ]->output( acMbHeader ) );
  if( bReset )
  {
    sm_uiPosCounter[ sm_uiDQIdplus3 ] = 0;
    sm_uiSymCounter[ sm_uiDQIdplus3 ] = 0;
  }
  return Err::m_nOK;
}

ErrVal
TraceFile::printPos()
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  ::snprintf( sm_acPos[sm_uiDQIdplus3], 15, "@%d", sm_uiPosCounter[ sm_uiDQIdplus3 ] );
  return Err::m_nOK;
}

ErrVal
TraceFile::countBits( UInt uiBitCount )
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  sm_uiPosCounter[ sm_uiDQIdplus3 ] += uiBitCount;
  return Err::m_nOK;
}

ErrVal
TraceFile::printSCnt()
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  RNOK ( printVal( sm_uiSymCounter[ sm_uiDQIdplus3 ]++ ) );
  return Err::m_nOK;
}

ErrVal
TraceFile::addBits( UInt uiVal, UInt uiLength )
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  ROT  ( uiLength > 100 );

  Char acBuffer[101];
  UInt uiOverLength = (UInt)gMax( 0, (Int)uiLength - 32 );
  UInt i;
  for ( i = 0; i < uiOverLength; i++ )
  {
    acBuffer[i] = '0';
  }
  for ( i = uiOverLength; i < uiLength; i++ )
  {
    acBuffer[i] = '0' + ( ( uiVal & ( 1 << ( uiLength - i - 1 ) ) ) >> ( uiLength - i - 1 ) );
  }
  acBuffer[uiLength] = '\0';

  i = (UInt)strlen( sm_acBits[sm_uiDQIdplus3] );
  if( i < 2 )
  {
    sm_acBits[sm_uiDQIdplus3][0] = '[';
    sm_acBits[sm_uiDQIdplus3][1] = '\0';
  }
  else
  {
    sm_acBits[sm_uiDQIdplus3][i-1] = '\0';
  }
  sm_acBits[sm_uiDQIdplus3][ sizeof( sm_acBits[sm_uiDQIdplus3] ) - 1 ] = '\0';
  strncat( sm_acBits[sm_uiDQIdplus3], acBuffer, sizeof( sm_acBits[sm_uiDQIdplus3] ) - 1 - strlen( sm_acBits[sm_uiDQIdplus3] ) );
  strncat( sm_acBits[sm_uiDQIdplus3], "]",      sizeof( sm_acBits[sm_uiDQIdplus3] ) - 1 - strlen( sm_acBits[sm_uiDQIdplus3] ) );

  return Err::m_nOK;
}

ErrVal
TraceFile::printCode( UInt uiVal )
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  ::snprintf( sm_acCode[sm_uiDQIdplus3], 15, "%u", uiVal );
  return Err::m_nOK;
}


ErrVal
TraceFile::printCode(Int iVal)
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  ::snprintf( sm_acCode[sm_uiDQIdplus3], 15, "%i", iVal );
  return Err::m_nOK;
}

ErrVal
TraceFile::printString( const Char* pcString )
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  ::strncat( sm_acLine[sm_uiDQIdplus3], pcString, MAX_LINE_LENGTH-1 );
  return Err::m_nOK;
}

ErrVal
TraceFile::printType( const Char* pcString )
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  ::snprintf( sm_acType[sm_uiDQIdplus3], 15, "%s", pcString );
  return Err::m_nOK;
}

ErrVal
TraceFile::printVal( UInt uiVal )
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  Char tmp[16];
  ::snprintf( tmp, 15, "%3u", uiVal);
  ::strncat ( sm_acLine[sm_uiDQIdplus3], tmp, MAX_LINE_LENGTH-1 );
  return Err::m_nOK;
}

ErrVal
TraceFile::printVal( Int iVal )
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  Char tmp[16];
  ::snprintf( tmp, 15, "%3i", iVal );
  ::strncat ( sm_acLine[sm_uiDQIdplus3], tmp, MAX_LINE_LENGTH-1 );
  return Err::m_nOK;
}

ErrVal
TraceFile::printXVal( UInt uiVal )
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  Char tmp[16];
  ::snprintf( tmp, 15, "0x%04x", uiVal );
  ::strncat ( sm_acLine[sm_uiDQIdplus3], tmp, MAX_LINE_LENGTH-1 );
  return Err::m_nOK;
}

ErrVal
TraceFile::newLine()
{
  ROF  ( sm_bInit );
  ROTRS( sm_bDisable, Err::m_nOK );
  Char acTmp[1024];
  ::snprintf( acTmp, 1023, "%-6s %-50s %-8s %5s %s\n",
              sm_acPos[sm_uiDQIdplus3], sm_acLine[sm_uiDQIdplus3], sm_acType[sm_uiDQIdplus3], sm_acCode[sm_uiDQIdplus3], sm_acBits[sm_uiDQIdplus3] );
  RNOK( sm_fTrace[ sm_uiDQIdplus3 ]->output( acTmp ) );
  sm_acLine[sm_uiDQIdplus3][0] ='\0';
  sm_acType[sm_uiDQIdplus3][0] ='\0';
  sm_acCode[sm_uiDQIdplus3][0] ='\0';
  sm_acBits[sm_uiDQIdplus3][0] ='\0';
  sm_acPos [sm_uiDQIdplus3][0] ='\0';
  return Err::m_nOK;
}

H264AVC_NAMESPACE_END
