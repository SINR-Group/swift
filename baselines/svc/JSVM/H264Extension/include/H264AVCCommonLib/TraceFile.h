
#if !defined(AFX_TRACEFILE_H__B87E26CF_023E_4DC7_8F94_D3E38F59ABA1__INCLUDED_)
#define AFX_TRACEFILE_H__B87E26CF_023E_4DC7_8F94_D3E38F59ABA1__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#define ENCODER_TRACE     0
#define DECODER_TRACE     0

#ifdef  SHARP_AVC_REWRITE_OUTPUT
#if     DECODER_TRACE
#undef  DECODER_TRACE
#define DECODER_TRACE 0
#endif
#endif

#define MAX_TRACE_LAYERS  (16*8+3)
#define MAX_LINE_LENGTH   1024
#define MAX_BITS_LENGTH   128


H264AVC_NAMESPACE_BEGIN


class H264AVCCOMMONLIB_API TraceFile
{
protected:
  class TraceDQId
  {
  public:
    static ErrVal create  ( TraceDQId*& rpcTraceDQId,
                            const Char* pucBaseName,
                            UInt        uiDQIdplus1 );
    ErrVal        destroy ();
    ErrVal        output  ( const Char* pucLine );
    ErrVal        storePos();
    ErrVal        resetPos();
  protected:
    TraceDQId ( FILE* pFile );
    ~TraceDQId();
  private:
    FILE*   m_pFile;
    size_t  m_filePos;
  };

public:
	TraceFile         ();
	virtual ~TraceFile();

  static ErrVal initTrace   ( const Char* pucBaseName );
  static ErrVal uninitTrace ();

  static ErrVal disable     ();
  static ErrVal enable      ();
  static ErrVal storeFilePos();
  static ErrVal resetFilePos();

  static ErrVal setLayer    ( Int         iDQId );
  static ErrVal startPicture();
  static ErrVal startSlice  ();
  static ErrVal startMb     ( UInt        uiMbAddress  );
  static ErrVal printHeading( const Char* pcString,
                              Bool        bReset );

  static ErrVal printPos    ();
  static ErrVal countBits   ( UInt        uiBitCount );
  static ErrVal printSCnt   ();

  static ErrVal addBits     ( UInt        uiVal,
                              UInt        uiLength );
  static ErrVal printCode   ( UInt        uiVal );
  static ErrVal printCode   ( Int         iVal );

  static ErrVal printString ( const Char* pcString );
  static ErrVal printType   ( const Char* pcString);
  static ErrVal printVal    ( UInt        uiVal );
  static ErrVal printVal    ( Int         iVal );
  static ErrVal printXVal   ( UInt        uiVal );

  static ErrVal newLine     ();

protected:
  static Bool         sm_bInit;
  static Bool         sm_bDisable;
  static const Char*  sm_pucBaseName;
  static UInt         sm_uiDQIdplus3;
  static TraceDQId*   sm_fTrace       [MAX_TRACE_LAYERS];
  static UInt         sm_uiFrameNum   [MAX_TRACE_LAYERS];
  static UInt         sm_uiSliceNum   [MAX_TRACE_LAYERS];
  static UInt         sm_uiPosCounter [MAX_TRACE_LAYERS];
  static UInt         sm_uiSymCounter [MAX_TRACE_LAYERS];
  static UInt         sm_uiStorePosCnt[MAX_TRACE_LAYERS];
  static UInt         sm_uiStoreSymCnt[MAX_TRACE_LAYERS];
  static Char         sm_acLine       [MAX_TRACE_LAYERS][MAX_LINE_LENGTH];
  static Char         sm_acType       [MAX_TRACE_LAYERS][16];
  static Char         sm_acPos        [MAX_TRACE_LAYERS][16];
  static Char         sm_acCode       [MAX_TRACE_LAYERS][16];
  static Char         sm_acBits       [MAX_TRACE_LAYERS][MAX_BITS_LENGTH];
};



H264AVC_NAMESPACE_END




#if ENCODER_TRACE
  #define ETRACE_OFF       if( m_bTraceEnable ) TraceFile::disable     ()
  #define ETRACE_ON        if( m_bTraceEnable ) TraceFile::enable      ()

  #define INIT_ETRACE      if( m_bTraceEnable ) TraceFile::initTrace   ("TraceEncoder")
  #define UNINIT_ETRACE    if( m_bTraceEnable ) TraceFile::uninitTrace ()
  #define ETRACE_STORE     if( m_bTraceEnable ) TraceFile::storeFilePos()
  #define ETRACE_RESET     if( m_bTraceEnable ) TraceFile::resetFilePos()

  #define ETRACE_LAYER(x)  if( m_bTraceEnable ) TraceFile::setLayer    (x)
  #define ETRACE_NEWPIC    if( m_bTraceEnable ) TraceFile::startPicture()
  #define ETRACE_NEWSLICE  if( m_bTraceEnable ) TraceFile::startSlice  ()
  #define ETRACE_NEWMB(x)  if( m_bTraceEnable ) TraceFile::startMb     (x)
  #define ETRACE_HEADER(x)                      TraceFile::printHeading(x,true)

  #define ETRACE_POS       if( m_bTraceEnable ) TraceFile::printPos    ()
  #define ETRACE_COUNT(i)  if( m_bTraceEnable ) TraceFile::countBits   (i)
  #define ETRACE_SC        if( m_bTraceEnable ) TraceFile::printSCnt   ()

  #define ETRACE_BITS(v,l) if( m_bTraceEnable ) TraceFile::addBits     (v,l)
  #define ETRACE_CODE(v)   if( m_bTraceEnable ) TraceFile::printCode   (v)

  #define ETRACE_TH(t)     if( m_bTraceEnable ) TraceFile::printString (t)
  #define ETRACE_TY(t)     if( m_bTraceEnable ) TraceFile::printType   (t)
  #define ETRACE_V(t)      if( m_bTraceEnable ) TraceFile::printVal    (t)
  #define ETRACE_X(t)      if( m_bTraceEnable ) TraceFile::printXVal   (t)

  #define ETRACE_N         if( m_bTraceEnable ) TraceFile::newLine     ()
  #define ETRACE_DO(x)     if( m_bTraceEnable ) x
  #define ETRACE_DECLARE(x) x
 #else

  #define ETRACE_OFF
  #define ETRACE_ON

  #define INIT_ETRACE
  #define UNINIT_ETRACE
  #define ETRACE_STORE
  #define ETRACE_RESET

  #define ETRACE_LAYER(x)
  #define ETRACE_NEWPIC
  #define ETRACE_NEWSLICE
  #define ETRACE_NEWMB(x)
  #define ETRACE_HEADER(x)

  #define ETRACE_POS
  #define ETRACE_COUNT(i)
  #define ETRACE_SC

  #define ETRACE_BITS(v,l)
  #define ETRACE_CODE(v)

  #define ETRACE_TH(t)
  #define ETRACE_TY(t)
  #define ETRACE_V(t)
  #define ETRACE_X(t)

  #define ETRACE_N
  #define ETRACE_DO(x)
  #define ETRACE_DECLARE(x)
#endif

#if DECODER_TRACE
  #define DTRACE_OFF       TraceFile::disable     ()
  #define DTRACE_ON        TraceFile::enable      ()

  #define INIT_DTRACE      TraceFile::initTrace   ("TraceDecoder")
  #define UNINIT_DTRACE    TraceFile::uninitTrace ()

  #define DTRACE_LAYER(x)  TraceFile::setLayer    (x)
  #define DTRACE_NEWPIC    TraceFile::startPicture()
  #define DTRACE_NEWSLICE  TraceFile::startSlice  ()
  #define DTRACE_NEWMB(x)  TraceFile::startMb     (x)
  #define DTRACE_HEADER(x) TraceFile::printHeading(x,true)

  #define DTRACE_POS       TraceFile::printPos    ()
  #define DTRACE_COUNT(i)  TraceFile::countBits   (i)
  #define DTRACE_SC        TraceFile::printSCnt   ()

  #define DTRACE_BITS(v,l) TraceFile::addBits     (v,l)
  #define DTRACE_CODE(v)   TraceFile::printCode   (v)

  #define DTRACE_TH(t)     TraceFile::printString (t)
  #define DTRACE_TY(t)     TraceFile::printType   (t)
  #define DTRACE_V(t)      TraceFile::printVal    (t)
  #define DTRACE_X(t)      TraceFile::printXVal   (t)

  #define DTRACE_N         TraceFile::newLine     ()
  #define DTRACE_DO(x)     x
#else
  #define DTRACE_OFF
  #define DTRACE_ON

  #define INIT_DTRACE
  #define UNINIT_DTRACE

  #define DTRACE_LAYER(x)
  #define DTRACE_NEWPIC
  #define DTRACE_NEWSLICE
  #define DTRACE_NEWMB(x)
  #define DTRACE_HEADER(x)

  #define DTRACE_POS
  #define DTRACE_COUNT(i)
  #define DTRACE_SC

  #define DTRACE_BITS(v,l)
  #define DTRACE_CODE(v)

  #define DTRACE_TH(t)
  #define DTRACE_TY(t)
  #define DTRACE_V(t)
  #define DTRACE_X(t)

  #define DTRACE_N
  #define DTRACE_DO(x)
#endif

#endif // !defined(AFX_TRACEFILE_H__B87E26CF_023E_4DC7_8F94_D3E38F59ABA1__INCLUDED_)
