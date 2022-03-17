
#include "H264AVCEncoderLib.h"

#include "H264AVCCommonLib/ParameterSetMng.h"
#include "Scheduler.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// h264 namespace begin
//##ModelId=404734B00263
H264AVC_NAMESPACE_BEGIN

Scheduler::Scheduler():
  m_dFieldTimeDelta     ( 0.0208 ),
  m_dOutputFrequency    ( 30 ),
  m_uiOutputTicks       ( 1000 ),
  m_dClockFrequency     ( 30000),
  m_dActualOutTime      ( 0 ),
  m_dActualInTime       ( 0 ),
  m_dLastBPTime         ( 0),
  m_pcCodingParameter   ( NULL ),
  m_pfFileDebug         ( NULL ),
  m_bInitDone           ( false )
{
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=404734B00266
ErrVal Scheduler::create( Scheduler*& rpcScheduler )
{
  rpcScheduler = new Scheduler;

  ROT( NULL == rpcScheduler );

  return Err::m_nOK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=404734B00284
ErrVal Scheduler::init(  CodingParameter* pcCodingParameter, UInt uiLayer )
{
  ROT( NULL == pcCodingParameter );
  ROT ( m_bInitDone );

  m_pcCodingParameter = pcCodingParameter;

#ifdef DEBUG_SCHEDULER
  m_pfFileDebug = fopen(DEBUG_SCHEDULER_FILE, "w");
  ROT (NULL==m_pfFileDebug);
#endif

  m_dOutputFrequency = pcCodingParameter->getLayerParameters(uiLayer).getOutputFrameRate();

  m_bInitDone = true;
  m_uiLayerBits = 0;

  return Err::m_nOK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=404734B00288
ErrVal Scheduler::uninit()
{
  ROF ( m_bInitDone );

  if( NULL != m_pfFileDebug )
  {
    fclose (  m_pfFileDebug );
  }

  m_aacTiming[ HRD::VCL_HRD ].uninit();
  m_aacTiming[ HRD::NAL_HRD ].uninit();

  m_bInitDone = false;

  return Err::m_nOK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=404734B00286
ErrVal Scheduler::initBuffer( const VUI* pcVui, UInt uiLayerIndex)
{
  ROF ( m_bInitDone );

  RNOK ( xInitHrd (pcVui->getNalHrd(uiLayerIndex), HRD::NAL_HRD) );
  RNOK ( xInitHrd (pcVui->getVclHrd(uiLayerIndex), HRD::VCL_HRD) );

  if(pcVui->getTimingInfo(uiLayerIndex).getTimingInfoPresentFlag() )
  {
    m_dClockFrequency = (Double)pcVui->getTimingInfo(uiLayerIndex).getTimeScale() / (Double)pcVui->getTimingInfo(uiLayerIndex).getNumUnitsInTick();
    m_uiOutputTicks = 1;
    m_dFieldTimeDelta = 1 / m_dClockFrequency;
  }
  return Err::m_nOK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=40DFBB09007D
ErrVal Scheduler::xInitHrd ( const HRD &rcHrd, const HRD::HrdParamType eHrdParamType)
{
  m_aacTiming[ eHrdParamType ].uninit();
  m_apcHrd[eHrdParamType] = &rcHrd;
  // initial filling level for CPB
  //Double dInitialCpbLevel = m_pcCodingParameter->getVUIParams().getInitialCpbLevel() / 100.0;
  Double dInitialCpbLevel = 0.7; // fixed to 90 percent

  if( rcHrd.getHrdParametersPresentFlag() )
  {
    UInt uiCpbCnt = rcHrd.getCpbCnt();
    UInt uiBrScale = 6 + rcHrd.getBitRateScale();
    UInt uiCSScale = 4 + rcHrd.getCpbSizeScale();
    m_aacTiming[eHrdParamType].init( uiCpbCnt );
    for( UInt i=0; i<uiCpbCnt; i++)
    {
      UInt uiBitRate = ( rcHrd.getCntBuf(i).getBitRateValue() << uiBrScale);
      m_aacTiming[eHrdParamType][i].m_iBitRate = uiBitRate;
      UInt uiCPBSize = ( rcHrd.getCntBuf(i).getCpbSizeValue() << uiCSScale);
      m_aacTiming[eHrdParamType][i].m_uiFirstIrd = UInt((dInitialCpbLevel*uiCPBSize/(Double)uiBitRate)*90000);
      m_aacTiming[eHrdParamType][i].m_dRemoval   = m_aacTiming[eHrdParamType][i].m_uiFirstIrd/Double(90000);
      m_aacTiming[eHrdParamType][i].m_uiIrd      = m_aacTiming[eHrdParamType][i].m_uiFirstIrd;
      m_aacTiming[eHrdParamType][i].m_bCbr       = rcHrd.getCntBuf(i).getVbrCbrFlag();
    }
  }
  return Err::m_nOK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=404734B00272
ErrVal Scheduler::destroy()
{
  if (m_bInitDone)
  {
    uninit();
  }
  delete this;
  return Err::m_nOK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=407BE99E002E
ErrVal Scheduler::createBufferingSei( SEI::BufferingPeriod*& rpcBufferingPeriod, ParameterSetMng* pcParameterSetMng, UInt uiDQId )
{
  ROF ( m_bInitDone );

  RNOK( SEI::BufferingPeriod::create( rpcBufferingPeriod, pcParameterSetMng, uiDQId > 0 ) );

  SequenceParameterSet *pcSPS;
  RNOK( pcParameterSetMng->getActiveSPS( pcSPS, uiDQId ) );

  rpcBufferingPeriod->setHRD(pcSPS->getSeqParameterSetId(), pcSPS->isSubSetSPS(), m_apcHrd );

  RNOK (xCreateBufferingSeiHrd(HRD::NAL_HRD, *m_apcHrd[HRD::NAL_HRD], rpcBufferingPeriod));
  RNOK (xCreateBufferingSeiHrd(HRD::VCL_HRD, *m_apcHrd[HRD::VCL_HRD], rpcBufferingPeriod));

  return Err::m_nOK;
}

//##ModelId=40DFBB090080
ErrVal Scheduler::xCreateBufferingSeiHrd( HRD::HrdParamType eHrdParamType, const HRD &rcHrd, SEI::BufferingPeriod* pcBPSei)
{
  if( rcHrd.getHrdParametersPresentFlag())
  {
    for( UInt i=0; i<rcHrd.getCpbCnt(); i++)
    {
      TimingUnit* myTU = &m_aacTiming[eHrdParamType][i];
      myTU->calcIrd( m_dActualInTime);
      pcBPSei->getSchedSel(eHrdParamType, i).setDelay(myTU->m_uiIrd);
      pcBPSei->getSchedSel(eHrdParamType, i).setDelayOffset(myTU->m_uiIrdOffset);
      if( NULL != m_pfFileDebug )
      {
        if (HRD::VCL_HRD == eHrdParamType)
        {
          fprintf( m_pfFileDebug, "VCL Delay= %d    Offset = %d\n", myTU->m_uiIrd, myTU->m_uiIrdOffset );
        }
        else
        {
          fprintf( m_pfFileDebug, "NAL Delay= %d    Offset = %d\n", myTU->m_uiIrd, myTU->m_uiIrdOffset );
        }
      }
    }
  }
  return Err::m_nOK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=407BE99E003F
ErrVal Scheduler::createTimingSei( SEI::PicTiming*& rpcPicTiming, const VUI* pcVui, UInt uiPicNumOffset,  SliceHeader &rcSH, UInt uiInputFormat, UInt uiLayerIndex)
{
  ROF ( m_bInitDone );

  RNOK( SEI::PicTiming::create( rpcPicTiming, pcVui, uiLayerIndex ) );

  rpcPicTiming->setCpbRemovalDelay(UInt((m_dActualInTime - m_dLastBPTime) * m_dClockFrequency + 0.5) );

  if( rcSH.getIdrFlag() )
  {
    m_dLastBPTime = m_dActualInTime;
  }

  UInt uiTopAdd = (uiInputFormat==2)?1:0;
  UInt uiBotAdd = (uiInputFormat==1)?1:0;

  UInt uiDpbOutputDelay = 2*(uiPicNumOffset) + (rcSH.getFieldPicFlag() ? (rcSH.getBottomFieldFlag()?uiBotAdd:uiTopAdd):0);
  rpcPicTiming->setDpbOutputDelay(m_uiOutputTicks*uiDpbOutputDelay);

  PicStruct ePictstruct;

  if (rcSH.getFieldPicFlag())
  {
    ePictstruct=rcSH.getBottomFieldFlag()?PS_BOT:PS_TOP;
  }
  else
  {
    switch (uiInputFormat)
    {
    case 0:
      ePictstruct = PS_FRAME;
      break;
    case 1:
      ePictstruct = PS_TOP_BOT;
      break;
    case 2:
      ePictstruct = PS_BOT_TOP;
      break;
    default:
      ePictstruct = PS_NOT_SPECIFIED;
      break;
    }
  }
  rpcPicTiming->setPicStruct(ePictstruct);

//  printf ("\nOutput delay: %d", uiDpbOutputDelay);

  return Err::m_nOK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=404734B00280
ErrVal Scheduler::calculateTiming( UInt uiVclSize, UInt uiAUSize, Bool bIsIdr, Bool bFieldPicFlag)
{
  ROF ( m_bInitDone );

  RNOK (xCalculateTiming(HRD::NAL_HRD,  uiAUSize, bIsIdr));
  RNOK (xCalculateTiming(HRD::VCL_HRD, uiVclSize, bIsIdr));

  if (bFieldPicFlag)
  {
    m_dActualInTime += m_dFieldTimeDelta;
  }
  else
  {
    m_dActualInTime += 2 * m_dFieldTimeDelta;
  }

  return Err::m_nOK;
}

//##ModelId=40DFBB09008D
ErrVal Scheduler::xCalculateTiming( HRD::HrdParamType eHrdParamType, UInt uiSize, Bool bIsIdr )
{
  UInt i;
  for( i=0; i<m_aacTiming[eHrdParamType].size(); i++)
  {
    m_aacTiming[eHrdParamType][i].calcTiming( uiSize, m_dActualInTime, bIsIdr);
    if ( HRD::VCL_HRD == eHrdParamType )
    {
      if( NULL != m_pfFileDebug )
      {
        fprintf( m_pfFileDebug, "Ird = %d   Irdo = %d   Taie = %f   Tai = %f   Taf = %f   Trn = %f   Size = %d\n",
          m_aacTiming[eHrdParamType][i].m_uiIrd, m_aacTiming[eHrdParamType][i].m_uiIrdOffset, m_aacTiming[eHrdParamType][i].m_dInitialArrivalEarliest,
          m_aacTiming[eHrdParamType][i].m_dInitialArrival, m_aacTiming[eHrdParamType][i].m_dFinalArrival, m_aacTiming[eHrdParamType][i].m_dRemoval,
          uiSize);
        fflush (m_pfFileDebug);
      }
    }
  }
  return Err::m_nOK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=404734B0030E
Scheduler::TimingUnit::TimingUnit( ) //KR
: m_iDataLength(0),
  m_iBitRate(0),
  m_dInitialArrivalEarliest(0),
  m_dInitialArrival(0),
  m_dFinalArrival(0),
  m_dRemoval(0),
  m_uiFirstIrd(0),
  m_uiIrd(0),
  m_uiIrdOffset(0),
  m_bCbr(false)
{
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=404734B00313
ErrVal Scheduler::TimingUnit::calcIrd( Double dTime) //KR
{
  m_dRemoval = dTime + (Double)m_uiFirstIrd/(Double)90000;
  UInt uiTmp = UInt((m_dRemoval - m_dFinalArrival) * 90000 + 0.5); // C-16
  if(m_bCbr)
  {
    m_uiIrd = uiTmp;
    m_dInitialArrivalEarliest = 0;  //not used in this case
    m_uiIrdOffset = 0;
  }
  else
  {
    m_uiIrd = (uiTmp > m_uiFirstIrd) ? m_uiFirstIrd : uiTmp; // final arrival last AU
    m_dInitialArrivalEarliest = m_dRemoval - Double(m_uiIrd)/(Double)90000;
    m_uiIrdOffset = m_uiFirstIrd - m_uiIrd;
  }

  return Err::m_nOK;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//##ModelId=404734B0030F
ErrVal Scheduler::TimingUnit::calcTiming( UInt uiSize, Double dTime, Bool bIsIdr) //KR
{
  m_iDataLength = uiSize * 8; // in Bits
  if( !bIsIdr)
  {
    m_dRemoval = dTime + (Double)m_uiFirstIrd/(Double)90000;
    m_dInitialArrivalEarliest = (m_bCbr) ? 0 : m_dRemoval - Double(m_uiIrd + m_uiIrdOffset)/(Double)90000;
  }

  m_dInitialArrival = gMax(m_dFinalArrival, m_dInitialArrivalEarliest);
  m_dFinalArrival = m_dInitialArrival + (Double)m_iDataLength / (Double)(m_iBitRate);

  return Err::m_nOK;
}

// h264 namespace end
H264AVC_NAMESPACE_END
