
#if !defined(AFX_SCHEDULER_H__4242CFD4_A40A_4FCE_B740_60624D030E86__INCLUDED_)
#define AFX_SCHEDULER_H__4242CFD4_A40A_4FCE_B740_60624D030E86__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/Sei.h"
#include "CodingParameter.h"

// h264 namespace begin
H264AVC_NAMESPACE_BEGIN

// #define DEBUG_SCHEDULER
#define DEBUG_SCHEDULER_FILE "scheduler.txt"

class Scheduler
{
  class TimingUnit
  {
  public:
    Int64   m_iDataLength; // in Bytes
    Int64   m_iBitRate;    // in Bits
    Double  m_dInitialArrivalEarliest;
    Double  m_dInitialArrival;
    Double  m_dFinalArrival;
    Double  m_dRemoval;
    UInt    m_uiFirstIrd;
    UInt    m_uiIrd;
    UInt    m_uiIrdOffset;
    Bool    m_bCbr;

    TimingUnit();
    ErrVal  calcTiming( UInt uiSize, Double dTime, Bool bIsIdr);
    ErrVal  calcIrd(Double dTime);
  };


protected:
        Scheduler();
  virtual ~Scheduler() {};

public:
  static ErrVal create( Scheduler*& rpcScheduler );
  ErrVal destroy();

  ErrVal createBufferingSei( SEI::BufferingPeriod*& rpcBufferingPeriod, ParameterSetMng* pcParameterSetMng, UInt uiDQId );
  ErrVal createTimingSei( SEI::PicTiming*& rpcPicTiming, const VUI* pcVui, UInt uiPicNumOffset, SliceHeader &rcSH, UInt uiInputFormat, UInt uiLayerIndex);

  ErrVal calculateTiming( UInt uiVclSize, UInt uiAUSize, Bool bIsIdr, Bool bFieldPicFlag);
//  ErrVal calculateIrd();
  ErrVal init( CodingParameter *pcCodingParameter, UInt uiLayer );
  ErrVal initBuffer( const VUI* pcVui, UInt uiLayerIndex);
  ErrVal uninit();

  Void setLayerBits( UInt uiBits ) { m_uiLayerBits = uiBits; }
  UInt getLayerBits()              { return m_uiLayerBits; }
protected:
  ErrVal xInitHrd (const HRD& rcHrd, const HRD::HrdParamType eHrdParamType);
  ErrVal xCreateBufferingSeiHrd( HRD::HrdParamType eHrdParamType, const HRD &rcHrd, SEI::BufferingPeriod* pcBPSei);
  ErrVal xCalculateTiming( HRD::HrdParamType eHrdParamType, UInt uiSize, Bool bIsIdr );

  Double  m_dFieldTimeDelta;
  Double  m_dOutputFrequency;
  UInt    m_uiOutputTicks;
  Double  m_dClockFrequency;
  Double  m_dActualOutTime;
  Double  m_dActualInTime;
  Double  m_dLastBPTime;

  StatBuf< DynBuf< TimingUnit >,2 >  m_aacTiming;
  const HRD* m_apcHrd[2];

  CodingParameter *m_pcCodingParameter;

  FILE *m_pfFileDebug;
  Bool m_bInitDone;
  UInt m_uiLayerBits;
};

// h264 namespace end
H264AVC_NAMESPACE_END

#endif // !defined(AFX_Scheduler_H__4242CFD4_A40A_4FCE_B740_60624D030E86__INCLUDED_)
