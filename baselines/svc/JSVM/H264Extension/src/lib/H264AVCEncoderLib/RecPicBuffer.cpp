
#include "H264AVCEncoderLib.h"
#include "H264AVCCommonLib.h"
#include "RecPicBuffer.h"


H264AVC_NAMESPACE_BEGIN


RecPicBufUnit::RecPicBufUnit()
: m_iPoc                  ( MSYS_INT_MIN )
, m_uiFrameNum            ( MSYS_UINT_MAX )
, m_bExisting             ( false )
, m_bNeededForReference   ( false )
, m_bOutputted            ( false )
, m_pcReconstructedFrame  ( NULL )
, m_pcMbDataCtrl          ( NULL )
, m_pcPicBuffer           ( NULL )
{
}


RecPicBufUnit::~RecPicBufUnit()
{
  if( m_pcMbDataCtrl )
  {
    m_pcMbDataCtrl->uninit();
  }
  if( m_pcReconstructedFrame )
  {
    m_pcReconstructedFrame->uninit();
  }
  delete m_pcMbDataCtrl;
  delete m_pcReconstructedFrame;
}


ErrVal
RecPicBufUnit::create( RecPicBufUnit*&              rpcRecPicBufUnit,
                       YuvBufferCtrl&               rcYuvBufferCtrlFullPel,
                       YuvBufferCtrl&               rcYuvBufferCtrlHalfPel,
                       const SequenceParameterSet&  rcSPS )
{
  rpcRecPicBufUnit = new RecPicBufUnit();
  ROF( rpcRecPicBufUnit );

  rpcRecPicBufUnit->m_pcReconstructedFrame  = new Frame  ( rcYuvBufferCtrlFullPel, rcYuvBufferCtrlHalfPel, FRAME, 0 );
  rpcRecPicBufUnit->m_pcMbDataCtrl          = new MbDataCtrl();
  ROF( rpcRecPicBufUnit->m_pcReconstructedFrame );
  ROF( rpcRecPicBufUnit->m_pcMbDataCtrl );

  RNOK( rpcRecPicBufUnit->m_pcReconstructedFrame  ->init() );
  RNOK( rpcRecPicBufUnit->m_pcMbDataCtrl          ->init( rcSPS ) );

  rpcRecPicBufUnit->m_pcReconstructedFrame->setRecPicBufUnit( rpcRecPicBufUnit );

  return Err::m_nOK;
}


ErrVal
RecPicBufUnit::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal
RecPicBufUnit::init( SliceHeader* pcSliceHeader,
                     PicBuffer*   pcPicBuffer )
{
  m_iPoc                  = pcSliceHeader->getPoc();
  m_uiFrameNum            = pcSliceHeader->getFrameNum();
  m_bExisting             = true;
  m_bNeededForReference   = pcSliceHeader->getNalRefIdc() != NAL_REF_IDC_PRIORITY_LOWEST;
  m_bOutputted            = false;
  m_pcPicBuffer           = pcPicBuffer;

  m_pcReconstructedFrame->setPoc( *pcSliceHeader );

  return Err::m_nOK;
}


ErrVal
RecPicBufUnit::initNonEx( Int   iPoc,
                          UInt  uiFrameNum )
{
  m_iPoc                  = iPoc;
  m_uiFrameNum            = uiFrameNum;
  m_bExisting             = false;
  m_bNeededForReference   = true;
  m_bOutputted            = false;
  m_pcPicBuffer           = NULL;

  m_pcReconstructedFrame->setPoc( m_iPoc );

  return Err::m_nOK;
}


ErrVal
RecPicBufUnit::uninit()
{
  m_iPoc                  = MSYS_INT_MIN;
  m_uiFrameNum            = MSYS_UINT_MAX;
  m_bExisting             = false;
  m_bNeededForReference   = false;
  m_bOutputted            = false;
  m_pcPicBuffer           = NULL;

  return Err::m_nOK;
}


ErrVal
RecPicBufUnit::markNonRef()
{
  ROF( m_bNeededForReference );
  m_bNeededForReference = false;
  return Err::m_nOK;
}


ErrVal
RecPicBufUnit::markOutputted()
{
  ROT( m_bOutputted );
  m_bOutputted  = true;
  m_pcPicBuffer = NULL;
  return Err::m_nOK;
}














RecPicBuffer::RecPicBuffer()
: m_bInitDone               ( false )
, m_pcYuvBufferCtrlFullPel  ( NULL )
, m_pcYuvBufferCtrlHalfPel  ( NULL )
, m_uiNumRefFrames          ( 0 )
, m_uiMaxFrameNum           ( 0 )
, m_uiLastRefFrameNum       ( MSYS_UINT_MAX )
, m_pcCurrRecPicBufUnit     ( NULL )
{
}

RecPicBuffer::~RecPicBuffer()
{
}

ErrVal
RecPicBuffer::create( RecPicBuffer*& rpcRecPicBuffer )
{
  rpcRecPicBuffer = new RecPicBuffer();
  ROF( rpcRecPicBuffer );
  return Err::m_nOK;
}

ErrVal
RecPicBuffer::destroy()
{
  ROT( m_bInitDone );
  delete this;
  return Err::m_nOK;
}


ErrVal
RecPicBuffer::init( YuvBufferCtrl*  pcYuvBufferCtrlFullPel,
                    YuvBufferCtrl*  pcYuvBufferCtrlHalfPel )
{
  ROT( m_bInitDone );
  ROF( pcYuvBufferCtrlFullPel );
  ROF( pcYuvBufferCtrlHalfPel );

  m_pcYuvBufferCtrlFullPel  = pcYuvBufferCtrlFullPel;
  m_pcYuvBufferCtrlHalfPel  = pcYuvBufferCtrlHalfPel;
  m_uiNumRefFrames          = 0;
  m_uiMaxFrameNum           = 0;
  m_uiLastRefFrameNum       = MSYS_UINT_MAX;
  m_pcCurrRecPicBufUnit     = NULL;
  m_bInitDone               = true;

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::initSPS( const SequenceParameterSet& rcSPS )
{
  ROF( m_bInitDone );

  UInt uiMaxFramesInDPB = rcSPS.getMaxDPBSize();
  RNOK( xCreateData( uiMaxFramesInDPB, rcSPS ) );
  m_uiNumRefFrames      = rcSPS.getNumRefFrames();
  m_uiMaxFrameNum       = ( 1 << ( rcSPS.getLog2MaxFrameNum() ) );

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::uninit()
{
  ROF( m_bInitDone );

  RNOK( xDeleteData() );

  m_pcYuvBufferCtrlFullPel  = NULL;
  m_pcYuvBufferCtrlHalfPel  = NULL;
  m_uiNumRefFrames          = 0;
  m_uiMaxFrameNum           = 0;
  m_uiLastRefFrameNum       = MSYS_UINT_MAX;
  m_bInitDone               = false;

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::clear( PicBufferList& rcOutputList,
                     PicBufferList& rcUnusedList )
{
  RNOK( xClearOutputAll( rcOutputList, rcUnusedList ) );
  return Err::m_nOK;
}


RecPicBufUnit*
RecPicBuffer::getRecPicBufUnit( Int iPoc )
{
  RecPicBufUnit*            pcRecPicBufUnit = 0;
  RecPicBufUnitList::iterator iter  = m_cUsedRecPicBufUnitList.begin();
  RecPicBufUnitList::iterator end   = m_cUsedRecPicBufUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->getPoc() == iPoc )
    {
      pcRecPicBufUnit = *iter;
      break;
    }
  }
  return pcRecPicBufUnit;
}


ErrVal
RecPicBuffer::initCurrRecPicBufUnit( RecPicBufUnit*&  rpcCurrRecPicBufUnit,
                                     PicBuffer*       pcPicBuffer,
                                     SliceHeader*     pcSliceHeader,
                                     PicBufferList&   rcOutputList,
                                     PicBufferList&   rcUnusedList )
{
  ROF( m_bInitDone );
  ROF( pcPicBuffer );
  ROF( pcSliceHeader );

  //===== check for missing pictures =====
  RNOK( xCheckMissingPics( pcSliceHeader, rcOutputList, rcUnusedList ) );

  //===== initialize current DPB unit =====
  RNOK( m_pcCurrRecPicBufUnit->init( pcSliceHeader, pcPicBuffer ) );

  //===== load picture =====
  RNOK( m_pcCurrRecPicBufUnit->getRecFrame()->load( pcPicBuffer ) );

  //===== set reference =====
  rpcCurrRecPicBufUnit = m_pcCurrRecPicBufUnit;

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::store( RecPicBufUnit*   pcRecPicBufUnit,
                     SliceHeader*     pcSliceHeader,
                     PicBufferList&   rcOutputList,
                     PicBufferList&   rcUnusedList )
{
  RNOK( xStorePicture( pcRecPicBufUnit, rcOutputList, rcUnusedList, pcSliceHeader, pcSliceHeader->getIdrFlag() ) );

  if( pcRecPicBufUnit->isNeededForRef() )
  {
    m_uiLastRefFrameNum = pcRecPicBufUnit->getFrameNum();
  }

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::getRefLists( RefFrameList&  rcList0,
                           RefFrameList&  rcList1,
                           SliceHeader&   rcSliceHeader )
{
  //===== clear lists =====
  rcList0.reset();
  rcList1.reset();
  ROTRS( rcSliceHeader.isIntraSlice(), Err::m_nOK );

  if( rcSliceHeader.isPSlice() )
  {
    RNOK( xInitRefListPSlice  ( rcList0 ) );
    RNOK( xRefListRemapping   ( rcList0, LIST_0, &rcSliceHeader ) );
    RNOK( xAdaptListSize      ( rcList0, LIST_0,  rcSliceHeader ) );
    RNOK( xDumpRefList        ( rcList0, LIST_0 ) );
  }
  else // rcSliceHeader.isBSlice()
  {
    RNOK( xInitRefListsBSlice ( rcList0, rcList1 ) );
    RNOK( xRefListRemapping   ( rcList0, LIST_0, &rcSliceHeader ) );
    RNOK( xRefListRemapping   ( rcList1, LIST_1, &rcSliceHeader ) );
    RNOK( xAdaptListSize      ( rcList0, LIST_0,  rcSliceHeader ) );
    RNOK( xAdaptListSize      ( rcList1, LIST_1,  rcSliceHeader ) );
    RNOK( xDumpRefList        ( rcList0, LIST_0 ) );
    RNOK( xDumpRefList        ( rcList1, LIST_1 ) );
  }

  return Err::m_nOK;
}



ErrVal
RecPicBuffer::xAdaptListSize( RefFrameList& rcList,
                              ListIdx       eListIdx,
                              SliceHeader&  rcSliceHeader )
{
  UInt  uiDefaultListSize = rcSliceHeader.getNumRefIdxActive( eListIdx );
  UInt  uiMaximumListSize = rcList.getActive();
  UInt  uiCurrentListSize = gMin( uiDefaultListSize, uiMaximumListSize );

  //===== update slice header =====
  rcList.       setActive         (           uiCurrentListSize );
  rcSliceHeader.setNumRefIdxActive( eListIdx, uiCurrentListSize );
  if( uiCurrentListSize != rcSliceHeader.getPPS().getNumRefIdxActive( eListIdx ) )
  {
    rcSliceHeader.setNumRefIdxActiveOverrideFlag( true );
  }

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xCreateData( UInt                         uiMaxFramesInDPB,
                           const SequenceParameterSet&  rcSPS )
{
  ROF( m_bInitDone );
  RNOK( xDeleteData() );

  while( uiMaxFramesInDPB-- )
  {
    RecPicBufUnit* pcRecPicBufUnit = 0;
    RNOK( RecPicBufUnit::create( pcRecPicBufUnit, *m_pcYuvBufferCtrlFullPel, *m_pcYuvBufferCtrlHalfPel, rcSPS ) );
    m_cFreeRecPicBufUnitList.push_back( pcRecPicBufUnit );
  }
  RNOK( RecPicBufUnit::create( m_pcCurrRecPicBufUnit, *m_pcYuvBufferCtrlFullPel, *m_pcYuvBufferCtrlHalfPel, rcSPS ) );
  RNOK( m_pcCurrRecPicBufUnit->uninit() );

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xDeleteData()
{
  ROF( m_bInitDone );

  m_cFreeRecPicBufUnitList += m_cUsedRecPicBufUnitList;
  m_cUsedRecPicBufUnitList.clear();

  while( m_cFreeRecPicBufUnitList.size() )
  {
    RecPicBufUnit* pcRecPicBufUnit = m_cFreeRecPicBufUnitList.popFront();
    pcRecPicBufUnit->destroy();
  }
  if( m_pcCurrRecPicBufUnit )
  {
    m_pcCurrRecPicBufUnit->destroy();
    m_pcCurrRecPicBufUnit = NULL;
  }
  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xCheckMissingPics( SliceHeader*   pcSliceHeader,
                                 PicBufferList& rcOutputList,
                                 PicBufferList& rcUnusedList )
{
  ROTRS( pcSliceHeader->getIdrFlag(), Err::m_nOK );
  ROTRS( ( ( m_uiLastRefFrameNum + 1 ) % m_uiMaxFrameNum ) == pcSliceHeader->getFrameNum(), Err::m_nOK );

  UInt  uiMissingFrames = pcSliceHeader->getFrameNum() - m_uiLastRefFrameNum - 1;
  if( pcSliceHeader->getFrameNum() <= m_uiLastRefFrameNum )
  {
    uiMissingFrames += m_uiMaxFrameNum;
  }
  ROF( pcSliceHeader->getSPS().getGapsInFrameNumValueAllowedFlag());

  for( UInt uiIndex = 1; uiIndex <= uiMissingFrames; uiIndex++ )
  {
    Bool  bTreatAsIdr   = ( m_cUsedRecPicBufUnitList.empty() );
    Int   iPoc          = ( bTreatAsIdr ? 0 : m_cUsedRecPicBufUnitList.back()->getPoc() );
    UInt  uiFrameNum    = ( m_uiLastRefFrameNum + uiIndex ) % m_uiMaxFrameNum;

    RNOK( m_pcCurrRecPicBufUnit->initNonEx( iPoc, uiFrameNum ) );
    RNOK( xStorePicture( m_pcCurrRecPicBufUnit, rcOutputList, rcUnusedList, pcSliceHeader, bTreatAsIdr ) );
  }

  m_uiLastRefFrameNum = ( m_uiLastRefFrameNum + uiMissingFrames ) % m_uiMaxFrameNum;
  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xStorePicture( RecPicBufUnit* pcRecPicBufUnit,
                             PicBufferList& rcOutputList,
                             PicBufferList& rcUnusedList,
                             SliceHeader*   pcSliceHeader,
                             Bool           bTreatAsIdr )
{
  ROF( pcRecPicBufUnit == m_pcCurrRecPicBufUnit );

  if( bTreatAsIdr )
  {
    RNOK( xClearOutputAll( rcOutputList, rcUnusedList ) );
    m_cUsedRecPicBufUnitList.push_back( pcRecPicBufUnit );
  }
  else
  {
    m_cUsedRecPicBufUnitList.push_back( pcRecPicBufUnit );
    RNOK( xUpdateMemory( pcSliceHeader ) );
    RNOK( xOutput( rcOutputList, rcUnusedList ) );
  }
  RNOK( xDumpRecPicBuffer() );

  m_pcCurrRecPicBufUnit = m_cFreeRecPicBufUnitList.popFront();

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xOutput( PicBufferList& rcOutputList,
                       PicBufferList& rcUnusedList )
{
  ROTRS( m_cFreeRecPicBufUnitList.size(), Err::m_nOK );

  //===== smallest non-ref/output poc value =====
  Int                         iMinOutputPoc   = MSYS_INT_MAX;
  RecPicBufUnit*              pcElemToRemove  = 0;
  RecPicBufUnitList::iterator iter            = m_cUsedRecPicBufUnitList.begin();
  RecPicBufUnitList::iterator end             = m_cUsedRecPicBufUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    Bool bOutput = ( ! (*iter)->isOutputted() && (*iter)->isExisting() && ! (*iter)->isNeededForRef() );
    if( bOutput && (*iter)->getPoc() < iMinOutputPoc )
    {
      iMinOutputPoc   = (*iter)->getPoc();
      pcElemToRemove  = (*iter);
    }
  }
  ROF( pcElemToRemove ); // error, nothing can be removed

  //===== copy all output elements to temporary list =====
  RecPicBufUnitList cOutputList;
  Int               iMaxPoc = iMinOutputPoc;
  Int               iMinPoc = MSYS_INT_MAX;
  iter                      = m_cUsedRecPicBufUnitList.begin();
  for( ; iter != end; iter++ )
  {
    Bool bOutput = ( (*iter)->getPoc() <= iMinOutputPoc && ! (*iter)->isOutputted() );
    if( bOutput )
    {
      if( (*iter)->isExisting() )
      {
        cOutputList.push_back( *iter );
        if( (*iter)->getPoc() < iMinPoc )
        {
          iMinPoc = (*iter)->getPoc();
        }
      }
      else
      {
        RNOK( (*iter)->markOutputted() );
      }
    }
  }

  //===== real output =====
  for( Int iPoc = iMinPoc; iPoc <= iMaxPoc; iPoc++ )
  {
    iter = cOutputList.begin();
    end  = cOutputList.end  ();
    for( ; iter != end; iter++ )
    {
      if( (*iter)->getPoc() == iPoc )
      {
        RecPicBufUnit* pcRecPicBufUnit = *iter;
        cOutputList.remove( pcRecPicBufUnit );

        PicBuffer* pcPicBuffer = pcRecPicBufUnit->getPicBuffer();
        ROF( pcPicBuffer );
        pcRecPicBufUnit->getRecFrame()->store( pcPicBuffer );
        rcOutputList.push_back( pcPicBuffer );
        rcUnusedList.push_back( pcPicBuffer );

        pcRecPicBufUnit->markOutputted();
        break; // only one picture per POC
      }
    }
  }
  ROT( cOutputList.size() );

  //===== clear buffer ====
  RNOK( xClearBuffer() );

  //===== check =====
  ROT( m_cFreeRecPicBufUnitList.empty() ); // this should never happen

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xClearOutputAll( PicBufferList& rcOutputList,
                               PicBufferList& rcUnusedList )
{
  //===== create output list =====
  RecPicBufUnitList           cOutputList;
  Int                         iMinPoc = MSYS_INT_MAX;
  Int                         iMaxPoc = MSYS_INT_MIN;
  RecPicBufUnitList::iterator iter    = m_cUsedRecPicBufUnitList.begin();
  RecPicBufUnitList::iterator end     = m_cUsedRecPicBufUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    Bool bOutput = ( ! (*iter)->isOutputted() && (*iter)->isExisting() );
    if( bOutput )
    {
      cOutputList.push_back( *iter );
      if( (*iter)->getPoc() < iMinPoc )   iMinPoc = (*iter)->getPoc();
      if( (*iter)->getPoc() > iMaxPoc )   iMaxPoc = (*iter)->getPoc();
    }
  }

  //===== real output =====
  for( Int iPoc = iMinPoc; iPoc <= iMaxPoc; iPoc++ )
  {
    iter = cOutputList.begin();
    end  = cOutputList.end  ();
    for( ; iter != end; iter++ )
    {
      if( (*iter)->getPoc() == iPoc )
      {
        RecPicBufUnit* pcRecPicBufUnit = *iter;
        cOutputList.remove( pcRecPicBufUnit );

        //--- output ---
        PicBuffer* pcPicBuffer = pcRecPicBufUnit->getPicBuffer();
        ROF( pcPicBuffer );
        pcRecPicBufUnit->getRecFrame()->store( pcPicBuffer );
        rcOutputList.push_back( pcPicBuffer );
        rcUnusedList.push_back( pcPicBuffer );
        break; // only one picture per poc
      }
    }
  }
  ROT( cOutputList.size() );

  //===== uninit all elements and move to free list =====
  while( m_cUsedRecPicBufUnitList.size() )
  {
    RecPicBufUnit* pcRecPicBufUnit = m_cUsedRecPicBufUnitList.popFront();
    RNOK( pcRecPicBufUnit->uninit() );
    m_cFreeRecPicBufUnitList.push_back( pcRecPicBufUnit );
  }
  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xUpdateMemory( SliceHeader* pcSliceHeader )
{
  ROTRS( pcSliceHeader && pcSliceHeader->getNalRefIdc() == NAL_REF_IDC_PRIORITY_LOWEST, Err::m_nOK );

  if( pcSliceHeader && pcSliceHeader->getDecRefPicMarking().getAdaptiveRefPicMarkingModeFlag() )
  {
    RNOK( xMMCO( pcSliceHeader ) );
  }
  else
  {
    RNOK( xSlidingWindow() );
  }

  //===== clear buffer -> remove non-ref pictures =====
  RNOK( xClearBuffer() );

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xClearBuffer()
{
  //===== remove non-output / non-ref pictures =====
  //--- store in temporary list ---
  RecPicBufUnitList           cTempList;
  RecPicBufUnitList::iterator iter  = m_cUsedRecPicBufUnitList.begin();
  RecPicBufUnitList::iterator end   = m_cUsedRecPicBufUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    Bool  bNoOutput   = ( ! (*iter)->isExisting() || (*iter)->isOutputted() );
    Bool  bNonRef     = ( ! (*iter)->isNeededForRef() );

    if( bNonRef && bNoOutput )
    {
      cTempList.push_back( *iter );
    }
  }
  //--- uninit and move to free list ---
  while( cTempList.size() )
  {
    RecPicBufUnit*  pcRecPicBufUnit = cTempList.popFront();
    RNOK( pcRecPicBufUnit->uninit() );
    m_cUsedRecPicBufUnitList.remove   ( pcRecPicBufUnit );
    m_cFreeRecPicBufUnitList.push_back( pcRecPicBufUnit );
  }
  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xMMCO( SliceHeader* pcSliceHeader )
{
  ROF( pcSliceHeader );

  Mmco            eMmcoOp;
  const DecRefPicMarking& rcMmcoBuffer  = pcSliceHeader->getDecRefPicMarking();
  Int               iIndex        = 0;
  UInt              uiVal1, uiVal2;

  while( MMCO_END != ( eMmcoOp = rcMmcoBuffer.get( iIndex++ ).getCommand( uiVal1, uiVal2 ) ) )
  {
    switch( eMmcoOp )
    {
    case MMCO_SHORT_TERM_UNUSED:
      RNOK( xMarkShortTermUnused( m_pcCurrRecPicBufUnit, uiVal1 ) );
      break;
    case MMCO_RESET:
    case MMCO_MAX_LONG_TERM_IDX:
    case MMCO_ASSIGN_LONG_TERM:
    case MMCO_LONG_TERM_UNUSED:
    case MMCO_SET_LONG_TERM:
    default:
      RERR();
    }
  }
  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xMarkShortTermUnused( RecPicBufUnit*  pcCurrentRecPicBufUnit,
                                    UInt            uiDiffOfPicNums )
{
  ROF( pcCurrentRecPicBufUnit );

  UInt  uiCurrPicNum  = pcCurrentRecPicBufUnit->getFrameNum();
  Int   iPicNumN      = (Int)uiCurrPicNum - (Int)uiDiffOfPicNums - 1;

  RecPicBufUnitList::iterator iter            = m_cUsedRecPicBufUnitList.begin();
  RecPicBufUnitList::iterator end             = m_cUsedRecPicBufUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isNeededForRef() && (*iter)->getPicNum( uiCurrPicNum, m_uiMaxFrameNum ) == iPicNumN )
    {
      (*iter)->markNonRef();
      return Err::m_nOK;
    }
  }
  RERR();
}


ErrVal
RecPicBuffer::xSlidingWindow()
{
  //===== get number of reference frames =====
  UInt                        uiCurrNumRefFrames  = 0;
  RecPicBufUnitList::iterator iter                = m_cUsedRecPicBufUnitList.begin();
  RecPicBufUnitList::iterator end                 = m_cUsedRecPicBufUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isNeededForRef() )
    {
      uiCurrNumRefFrames++;
    }
  }
  ROTRS( uiCurrNumRefFrames <= m_uiNumRefFrames, Err::m_nOK );

  //===== sliding window reference picture update =====
  //--- look for last ref frame that shall be removed ---
  UInt uiRefFramesToRemove = uiCurrNumRefFrames - m_uiNumRefFrames;
  iter                     = m_cUsedRecPicBufUnitList.begin();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isNeededForRef() )
    {
      uiRefFramesToRemove--;
      if( uiRefFramesToRemove == 0 )
      {
        break;
      }
    }
  }
  ROT( uiRefFramesToRemove );
  //--- delete reference label ---
  end  = ++iter;
  iter = m_cUsedRecPicBufUnitList.begin();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isNeededForRef() )
    {
      RNOK( (*iter)->markNonRef() );
    }
  }

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xDumpRecPicBuffer()
{
#if 1 // NO_DEBUG
  return Err::m_nOK;
#endif

  printf( "\nRECONSTRUCTED PICTURE BUFFER:\n" );
  RecPicBufUnitList::iterator iter  = m_cUsedRecPicBufUnitList.begin();
  RecPicBufUnitList::iterator end   = m_cUsedRecPicBufUnitList.end  ();
  for( Int iIndex = 0; iter != end; iter++ )
  {
    RecPicBufUnit* p = (*iter);
    printf( "\tPOS=%d:\tFN=%d\tPoc=%d\t%s\t", iIndex, p->getFrameNum(), p->getPoc(), (p->isNeededForRef()?"REF":"   ") );
    if(  p->isOutputted() )   printf("Outputted  ");
    if( !p->isExisting () )   printf("NotExisting  ");
    printf("\n");
  }
  printf("\n");
  return Err::m_nOK;
}



ErrVal
RecPicBuffer::xInitRefListPSlice( RefFrameList& rcList )
{
  //----- get current frame num -----
  UInt uiCurrFrameNum = m_pcCurrRecPicBufUnit->getFrameNum();

  //----- generate decreasing POC list -----
  for( Int iMaxPicNum = (Int)uiCurrFrameNum; true; )
  {
    RecPicBufUnit*              pNext = 0;
    RecPicBufUnitList::iterator iter  = m_cUsedRecPicBufUnitList.begin();
    RecPicBufUnitList::iterator end   = m_cUsedRecPicBufUnitList.end  ();
    for( ; iter != end; iter++ )
    {
      if( (*iter)->isNeededForRef() &&
          (*iter)->getPicNum( uiCurrFrameNum, m_uiMaxFrameNum ) < iMaxPicNum &&
         ( ! pNext ||
          (*iter)->getPicNum( uiCurrFrameNum, m_uiMaxFrameNum ) > pNext->getPicNum( uiCurrFrameNum, m_uiMaxFrameNum ) ) )
      {
        pNext = (*iter);
      }
    }
    if( ! pNext )
    {
      break;
    }
    iMaxPicNum = pNext->getPicNum( uiCurrFrameNum, m_uiMaxFrameNum );
    rcList.add( pNext->getRecFrame() );
  }

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xInitRefListsBSlice( RefFrameList&  rcList0,
                                   RefFrameList&  rcList1 )
{
  RefFrameList  cDecreasingPocList;
  RefFrameList  cIncreasingPocList;
  Int           iCurrPoc = m_pcCurrRecPicBufUnit->getPoc();

  //----- generate decreasing Poc list -----
  for( Int iMaxPoc = iCurrPoc; true; )
  {
    RecPicBufUnit*              pNext = 0;
    RecPicBufUnitList::iterator iter  = m_cUsedRecPicBufUnitList.begin();
    RecPicBufUnitList::iterator end   = m_cUsedRecPicBufUnitList.end  ();
    for( ; iter != end; iter++ )
    {
      if( (*iter)->isNeededForRef() &&
        (*iter)->getPoc() < iMaxPoc &&
        ( ! pNext ||
        (*iter)->getPoc() > pNext->getPoc() ) )
      {
        pNext = (*iter);
      }
    }
    if( ! pNext )
    {
      break;
    }
    iMaxPoc = pNext->getPoc();
    cDecreasingPocList.add( pNext->getRecFrame() );
  }

  //----- generate increasing Poc list -----
  for( Int iMinPoc = iCurrPoc; true; )
  {
    RecPicBufUnit*              pNext = 0;
    RecPicBufUnitList::iterator iter  = m_cUsedRecPicBufUnitList.begin();
    RecPicBufUnitList::iterator end   = m_cUsedRecPicBufUnitList.end  ();
    for( ; iter != end; iter++ )
    {
      if( (*iter)->isNeededForRef() &&
        (*iter)->getPoc() > iMinPoc &&
        ( ! pNext ||
        (*iter)->getPoc() < pNext->getPoc() ) )
      {
        pNext = (*iter);
      }
    }
    if( ! pNext )
    {
      break;
    }
    iMinPoc = pNext->getPoc();
    cIncreasingPocList.add( pNext->getRecFrame() );
  }

  //----- list 0 and list 1 -----
  UInt uiPos;
  for( uiPos = 0; uiPos < cDecreasingPocList.getSize(); uiPos++ )
  {
    RNOK( rcList0.add( cDecreasingPocList.getEntry( uiPos ) ) );
  }
  for( uiPos = 0; uiPos < cIncreasingPocList.getSize(); uiPos++ )
  {
    RNOK( rcList0.add( cIncreasingPocList.getEntry( uiPos ) ) );
    RNOK( rcList1.add( cIncreasingPocList.getEntry( uiPos ) ) );
  }
  for( uiPos = 0; uiPos < cDecreasingPocList.getSize(); uiPos++ )
  {
    RNOK( rcList1.add( cDecreasingPocList.getEntry( uiPos ) ) );
  }

  //----- check for element switching -----
  if( rcList1.getActive() >= 2 && rcList0.getActive() == rcList1.getActive() )
  {
    Bool bSwitch = true;
    for( uiPos = 0; uiPos < rcList1.getActive(); uiPos++ )
    {
      if( rcList0.getEntry( uiPos ) != rcList1.getEntry( uiPos ) )
      {
        bSwitch = false;
        break;
      }
    }
    if( bSwitch )
    {
      rcList1.switchFirst();
    }
  }

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xRefListRemapping( RefFrameList&  rcList,
                                 ListIdx        eListIdx,
                                 SliceHeader*   pcSliceHeader )
{
  ROF( pcSliceHeader );
  const RefPicListReOrdering& rcRplrBuffer = pcSliceHeader->getRefPicListReordering( eListIdx );

  //===== re-ordering ======
  if( rcRplrBuffer.getRefPicListReorderingFlag() )
  {
    UInt  uiPicNumPred    = pcSliceHeader->getFrameNum();
    UInt  uiIndex         = 0;
    UInt  uiCommand       = 0;
    UInt  uiIdentifier    = 0;

    while( RPLR_END != ( uiCommand = rcRplrBuffer.get( uiIndex ).getCommand( uiIdentifier ) ) )
    {
      Frame* pcFrame = 0;

      if( uiCommand == RPLR_LONG )
      {
        //===== long-term index =====
        RERR(); // long-term not supported
      }
      else
      {
        //===== short-term index =====
        UInt uiAbsDiff = uiIdentifier + 1;

        //----- set short-term index (pic num) -----
        if( uiCommand == RPLR_NEG )
        {
          if( uiPicNumPred < uiAbsDiff )
          {
            uiPicNumPred -= ( uiAbsDiff - m_uiMaxFrameNum );
          }
          else
          {
            uiPicNumPred -=   uiAbsDiff;
          }
        }
        else // uiCommand == RPLR_POS
        {
          if( uiPicNumPred + uiAbsDiff > m_uiMaxFrameNum - 1 )
          {
            uiPicNumPred += ( uiAbsDiff - m_uiMaxFrameNum );
          }
          else
          {
            uiPicNumPred +=   uiAbsDiff;
          }
        }
        uiIdentifier = uiPicNumPred;

        //----- get frame -----
        RecPicBufUnitList::iterator iter = m_cUsedRecPicBufUnitList.begin();
        RecPicBufUnitList::iterator end  = m_cUsedRecPicBufUnitList.end  ();
        for( ; iter != end; iter++ )
        {
          if( (*iter)->isNeededForRef() &&
              (*iter)->getFrameNum() == uiIdentifier )
          {
            pcFrame = (*iter)->getRecFrame();
            break;
          }
        }
        if( ! pcFrame )
        {
          fprintf( stderr, "\nERROR: MISSING PICTURE for RPLR\n\n" );
          RERR();
        }
        //----- find picture in reference list -----
        UInt uiRemoveIndex = MSYS_UINT_MAX;
        for( UInt uiPos = uiIndex; uiPos < rcList.getActive(); uiPos++ ) // active is equal to size
        {
          if( rcList.getEntry( uiPos ) == pcFrame )
          {
            uiRemoveIndex = uiPos;
            break;
          }
        }

        //----- reference list re-ordering -----
        RNOK( rcList.setElementAndRemove( uiIndex, uiRemoveIndex, pcFrame ) );
        uiIndex++;
      } // short-term
    } // while
  }

  return Err::m_nOK;
}


ErrVal
RecPicBuffer::xDumpRefList( RefFrameList& rcList,
                            ListIdx       eListIdx  )
{
#if 1 // NO_DEBUG
  return Err::m_nOK;
#endif

  printf( "List %d =", eListIdx );
  for( UInt uiIndex = 1; uiIndex <= rcList.getActive(); uiIndex++ )
  {
    printf( " %d", rcList[uiIndex]->getPoc() );
  }
  printf( "\n" );
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
