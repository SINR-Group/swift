
#include "H264AVCDecoderLib.h"


#include "H264AVCDecoder.h"
#include "GOPDecoder.h"
#include "ControlMngH264AVCDecoder.h"
#include "SliceReader.h"
#include "SliceDecoder.h"
#include "UvlcReader.h"
#include "MbParser.h"
#include "MbDecoder.h"
#include "NalUnitParser.h"
#include "BitReadBuffer.h"
#include "CabacReader.h"
#include "CabaDecoder.h"

#include "H264AVCCommonLib/MbData.h"
#include "H264AVCCommonLib/LoopFilter.h"
#include "H264AVCCommonLib/Transform.h"
#include "H264AVCCommonLib/IntraPrediction.h"
#include "H264AVCCommonLib/MotionCompensation.h"
#include "H264AVCCommonLib/YuvBufferCtrl.h"
#include "H264AVCCommonLib/QuarterPelFilter.h"
#include "H264AVCCommonLib/SampleWeighting.h"
#include "H264AVCCommonLib/ParameterSetMng.h"
#include "H264AVCCommonLib/PocCalculator.h"

#include "CreaterH264AVCDecoder.h"

#include "H264AVCCommonLib/TraceFile.h"
#include "H264AVCCommonLib/ReconstructionBypass.h"



H264AVC_NAMESPACE_BEGIN


NonVCLNALUnit::NonVCLNALUnit( BinData* pcBinData, Bool bSEI, Bool bScalableSEI, Bool bBufferingPeriod )
: m_pcBinData       ( pcBinData )
, m_eNalUnitType    ( NalUnitType( pcBinData->data()[ 0 ] & 0x1F ) )
, m_bIsSEI          ( bSEI )
, m_bScalableSEI    ( bScalableSEI )
, m_bBufferingPeriod( bBufferingPeriod )
{
  setInstance( this );
}

NonVCLNALUnit::~NonVCLNALUnit()
{
  if( m_pcBinData )
  {
    m_pcBinData->deleteData();
    delete m_pcBinData;
  }
}

Void
NonVCLNALUnit::destroyNALOnly()
{
  m_pcBinData       = 0;
  delete this;
}


SliceDataNALUnit::SliceDataNALUnit( BinData* pcBinData, const SliceHeader& rcSliceHeader )
: m_pcBinDataPrefix                     ( 0 )
, m_pcBinData                           ( pcBinData )
, m_eNalRefIdc                          ( rcSliceHeader.getNalRefIdc() )
, m_eNalUnitType                        ( rcSliceHeader.getNalUnitType() )
, m_bIdrFlag                            ( rcSliceHeader.getIdrFlag() )
, m_uiPriorityId                        ( rcSliceHeader.getPriorityId() )
, m_bNoInterLayerPredFlag               ( rcSliceHeader.getNoInterLayerPredFlag() )
, m_uiDependencyId                      ( rcSliceHeader.getDependencyId() )
, m_uiQualityId                         ( rcSliceHeader.getQualityId() )
, m_uiTemporalId                        ( rcSliceHeader.getTemporalId() )
, m_bUseRefBasePicFlag                  ( rcSliceHeader.getUseRefBasePicFlag() )
, m_bDiscardableFlag                    ( rcSliceHeader.getDiscardableFlag() )
, m_bOutputFlag                         ( rcSliceHeader.getOutputFlag() )
, m_bTCoeffLevelPredictionFlag          ( rcSliceHeader.getTCoeffLevelPredictionFlag() )
, m_uiPPSId                             ( rcSliceHeader.getPicParameterSetId() )
, m_uiSPSId                             ( rcSliceHeader.getPPS().getSeqParameterSetId() )
, m_uiFrameNum                          ( rcSliceHeader.getFrameNum() )
, m_uiRedundantPicCnt                   ( rcSliceHeader.getRedundantPicCnt() )
, m_uiRefLayerDQId                      ( rcSliceHeader.getRefLayerDQId() )
, m_uiFrameWidthInMb                    ( rcSliceHeader.getSPS().getFrameWidthInMbs() )
, m_uiFrameHeightInMb                   ( rcSliceHeader.getSPS().getFrameHeightInMbs() )
, m_bIsDQIdMax                          ( false )
, m_bIsDependencyIdMax                  ( false )
, m_bLastSliceInLayerRepresentation     ( false )
, m_bLastSliceInDependencyRepresentation( false )
, m_bLastSliceInAccessUnit              ( false )
, m_bLastAccessUnitInStream             ( false )
, m_bPartOfIDRAccessUnit                ( false )
, m_bHighestRewriteLayer                ( false )
{
  setInstance( this );
  m_auiCroppingRectangle[0] = rcSliceHeader.getSPS().getFrameCropLeftOffset  () << 1;
  m_auiCroppingRectangle[1] = rcSliceHeader.getSPS().getFrameCropRightOffset () << 1;
  m_auiCroppingRectangle[2] = rcSliceHeader.getSPS().getFrameCropTopOffset   () << ( rcSliceHeader.getSPS().getFrameMbsOnlyFlag() ? 1 : 2 );
  m_auiCroppingRectangle[3] = rcSliceHeader.getSPS().getFrameCropBottomOffset() << ( rcSliceHeader.getSPS().getFrameMbsOnlyFlag() ? 1 : 2 );
}

SliceDataNALUnit::SliceDataNALUnit( BinData* pcBinDataPrefix, const PrefixHeader& rcPrefixHeader,
                                    BinData* pcBinData,       const SliceHeader&  rcSliceHeader )
: m_pcBinDataPrefix                     ( pcBinDataPrefix )
, m_pcBinData                           ( pcBinData )
, m_eNalRefIdc                          ( rcSliceHeader.getNalRefIdc() )
, m_eNalUnitType                        ( rcSliceHeader.getNalUnitType() )
, m_bIdrFlag                            ( rcSliceHeader.getIdrFlag() )
, m_uiPriorityId                        ( rcPrefixHeader.getPriorityId() )
, m_bNoInterLayerPredFlag               ( rcPrefixHeader.getNoInterLayerPredFlag() )
, m_uiDependencyId                      ( rcPrefixHeader.getDependencyId() )
, m_uiQualityId                         ( rcPrefixHeader.getQualityId() )
, m_uiTemporalId                        ( rcPrefixHeader.getTemporalId() )
, m_bUseRefBasePicFlag                  ( rcPrefixHeader.getUseRefBasePicFlag() )
, m_bDiscardableFlag                    ( rcPrefixHeader.getDiscardableFlag() )
, m_bOutputFlag                         ( rcPrefixHeader.getOutputFlag() )
, m_bTCoeffLevelPredictionFlag          ( false )
, m_uiPPSId                             ( rcSliceHeader.getPicParameterSetId() )
, m_uiSPSId                             ( rcSliceHeader.getPPS().getSeqParameterSetId() )
, m_uiFrameNum                          ( rcSliceHeader.getFrameNum() )
, m_uiRedundantPicCnt                   ( rcSliceHeader.getRedundantPicCnt() )
, m_uiRefLayerDQId                      ( rcSliceHeader.getRefLayerDQId() )
, m_uiFrameWidthInMb                    ( rcSliceHeader.getSPS().getFrameWidthInMbs() )
, m_uiFrameHeightInMb                   ( rcSliceHeader.getSPS().getFrameHeightInMbs() )
, m_bIsDQIdMax                          ( false )
, m_bIsDependencyIdMax                  ( false )
, m_bLastSliceInLayerRepresentation     ( false )
, m_bLastSliceInDependencyRepresentation( false )
, m_bLastSliceInAccessUnit              ( false )
, m_bLastAccessUnitInStream             ( false )
, m_bPartOfIDRAccessUnit                ( false )
, m_bHighestRewriteLayer                ( false )
{
  setInstance( this );
  m_auiCroppingRectangle[0] = rcSliceHeader.getSPS().getFrameCropLeftOffset  () << 1;
  m_auiCroppingRectangle[1] = rcSliceHeader.getSPS().getFrameCropRightOffset () << 1;
  m_auiCroppingRectangle[2] = rcSliceHeader.getSPS().getFrameCropTopOffset   () << ( rcSliceHeader.getSPS().getFrameMbsOnlyFlag() ? 1 : 2 );
  m_auiCroppingRectangle[3] = rcSliceHeader.getSPS().getFrameCropBottomOffset() << ( rcSliceHeader.getSPS().getFrameMbsOnlyFlag() ? 1 : 2 );
}


SliceDataNALUnit::~SliceDataNALUnit()
{
  if( m_pcBinDataPrefix )
  {
    m_pcBinDataPrefix->deleteData();
    delete m_pcBinDataPrefix;
  }
  if( m_pcBinData )
  {
    m_pcBinData->deleteData();
    delete m_pcBinData;
  }
}




AccessUnit::AccessUnit()
: m_bEndOfStream      ( false )
, m_bComplete         ( false )
, m_pcLastVCLNALUnit  ( 0 )
, m_pcLastPrefixHeader( 0 )
, m_pcLastSliceHeader ( 0 )
{
}

AccessUnit::~AccessUnit()
{
  while( ! m_cNalUnitList.empty() )
  {
    NALUnit*  pcNalUnit = m_cNalUnitList.popFront();
    delete    pcNalUnit;
  }
  while( ! m_cStartOfNewAccessUnit.empty() )
  {
    NALUnit*  pcNalUnit = m_cStartOfNewAccessUnit.popFront();
    delete    pcNalUnit;
  }
  delete m_pcLastPrefixHeader;
  delete m_pcLastSliceHeader;
}


ErrVal
AccessUnit::update( BinData* pcBinData, PrefixHeader& rcPrefixHeader )
{
  ROT( m_bComplete );
  ROT( m_bEndOfStream );
  ROF( pcBinData );

  //===== create or extract and update slice data NAL unit =====
  NonVCLNALUnit* pcNonVCLNalUnit = new NonVCLNALUnit( pcBinData, false, false, false );
  ROF( pcNonVCLNalUnit );

  //===== update slice data NAL unit list and header references =====
  m_cNalUnitList.push_back( pcNonVCLNalUnit );
  delete m_pcLastPrefixHeader;
  m_pcLastPrefixHeader = &rcPrefixHeader;
  return Err::m_nOK;
}


ErrVal
AccessUnit::update( BinData* pcBinData, SliceHeader& rcSliceHeader )
{
  ROT( m_bComplete );
  ROT( m_bEndOfStream );
  ROF( pcBinData );

  //===== create or extract and update slice data NAL unit =====
  SliceDataNALUnit* pcSliceDataNalUnit = 0;
  if( m_pcLastPrefixHeader && rcSliceHeader.isH264AVCCompatible() )
  {
    NALUnit*        pcLastNALUnit   = m_cNalUnitList.popBack();
    NonVCLNALUnit*  pcPrefixNALUnit = (NonVCLNALUnit*)pcLastNALUnit->getInstance();
    BinData*        pcBinDataPrefix = pcPrefixNALUnit->getBinData();
    pcPrefixNALUnit->destroyNALOnly();
    pcSliceDataNalUnit = new SliceDataNALUnit( pcBinDataPrefix, *m_pcLastPrefixHeader, pcBinData, rcSliceHeader );
  }
  else
  {
    pcSliceDataNalUnit = new SliceDataNALUnit( pcBinData, rcSliceHeader );
  }
  ROF( pcSliceDataNalUnit );

  //===== check for new access unit =====
  if( rcSliceHeader.isFirstSliceOfNextAccessUnit( m_pcLastSliceHeader ) )
  {
    xSetComplete( pcSliceDataNalUnit, &rcSliceHeader );
    return Err::m_nOK;
  }

  //===== update slice data NAL unit list and header references =====
  m_cNalUnitList.push_back( pcSliceDataNalUnit );
  delete m_pcLastPrefixHeader;
  delete m_pcLastSliceHeader;
  m_pcLastPrefixHeader  = 0;
  m_pcLastSliceHeader   = &rcSliceHeader;
  return Err::m_nOK;
}


ErrVal
AccessUnit::update( BinData* pcBinData, Bool bSEI, Bool bScalableSEI, Bool bBufferingPeriod )
{
  ROT( m_bComplete );
  ROT( m_bEndOfStream );

  Bool bEOS = ( pcBinData == 0 );

  if( pcBinData )
  {
    NonVCLNALUnit* pcNonVCLNALUnit = new NonVCLNALUnit( pcBinData, bSEI, bScalableSEI, bBufferingPeriod );
    ROF( pcNonVCLNALUnit );
    m_cNalUnitList.push_back( pcNonVCLNALUnit );
    bEOS = ( pcNonVCLNALUnit->getNalUnitType() == NAL_UNIT_END_OF_STREAM );
  }
  if( bEOS )
  {
    xSetComplete();
  }
  return Err::m_nOK;
}


ErrVal
AccessUnit::getAndRemoveNextNalUnit( NALUnit*& rpcNalUnit )
{
  ROF( m_bComplete );
  rpcNalUnit = m_cNalUnitList.popFront();
  if( rpcNalUnit->isVCLNALUnit() && rpcNalUnit->getInstance() == (Void*)m_pcLastVCLNALUnit )
  {
    m_pcLastVCLNALUnit = 0;
  }
  if( m_cNalUnitList.empty() )
  {
    xReInit();
  }
  return Err::m_nOK;
}


#define ISVCL(i)  ((*i)->isVCLNALUnit())
#define SLICE(i)  ((SliceDataNALUnit*)((*i)->getInstance()))


Void
AccessUnit::xSetComplete( SliceDataNALUnit* pcFirstSliceDataNALUnitOfNextAccessUnit,
                          SliceHeader*      pcFirstSliceHeaderOfNextAccessUnit )
{
  delete m_pcLastPrefixHeader;
  delete m_pcLastSliceHeader;
  m_bEndOfStream        = ( pcFirstSliceDataNALUnitOfNextAccessUnit == 0 );
  m_bComplete           = true;
  m_pcLastVCLNALUnit    = 0;
  m_pcLastPrefixHeader  = 0;
  m_pcLastSliceHeader   = pcFirstSliceHeaderOfNextAccessUnit;

  //===== remove redundant NAL units =====
  xRemoveRedundant();

  //===== find last slice data NAL unit =====
  {
    MyList<NALUnit*>::reverse_iterator iIter = m_cNalUnitList.rbegin();
    MyList<NALUnit*>::reverse_iterator iEnd  = m_cNalUnitList.rend  ();
    while( iIter != iEnd )
    {
      if( ISVCL(iIter) )
      {
        m_pcLastVCLNALUnit = SLICE(iIter);
        break;
      }
      iIter++;
    }
    AOF( m_pcLastVCLNALUnit );
  }

  //===== set NAL units of next access unit =====
  if( ! m_bEndOfStream )
  {
    MyList<NALUnit*>::iterator iIter = m_cNalUnitList.begin();
    MyList<NALUnit*>::iterator iEnd  = m_cNalUnitList.end  ();
    //--- goto the NAL unit after the last VCL NAL unit ---
    while( iIter != iEnd )
    {
      if( ISVCL(iIter) && SLICE(iIter) == m_pcLastVCLNALUnit )
      {
        iIter++;
        break;
      }
      iIter++;
    }
    //--- find first NAL of next access unit ---
    while( iIter != iEnd )
    {
      NalUnitType eNalUnitType = (*iIter)->getNalUnitType();
      if( ( eNalUnitType >= 6 && eNalUnitType <= 9 ) || ( eNalUnitType >= 14 && eNalUnitType <= 18 ) )
      {
        break;
      }
      iIter++;
    }
    //--- set NAL units of next access unit ---
    UInt  uiNumNALUnitsToRemove = 0;
    while( iIter != iEnd )
    {
      m_cStartOfNewAccessUnit.push_back( *iIter );
      uiNumNALUnitsToRemove++;
      iIter++;
    }
    m_cStartOfNewAccessUnit.push_back( pcFirstSliceDataNALUnitOfNextAccessUnit );
    //--- remove the NAL units from the current access unit ---
    while( uiNumNALUnitsToRemove )
    {
      m_cNalUnitList.pop_back();
      uiNumNALUnitsToRemove--;
    }
  }

  //===== analyse NAL units and set parameters of slice data NAL units =====
  xSetParameters();
}


MyList<NALUnit*>::iterator
getNextVCL( MyList<NALUnit*>::iterator iIter, MyList<NALUnit*>::iterator iEnd, Bool bFirst = false )
{
  if( iIter != iEnd )
  {
    if( ! bFirst )
    {
      iIter++;
    }
    while( iIter != iEnd )
    {
      if( (*iIter)->isVCLNALUnit() )
      {
        break;
      }
      iIter++;
    }
  }
  return iIter;
}


Void
AccessUnit::xRemoveRedundant()
{
  //===== remove redundant slices (could be used for error concealment in later version) =====
  {
    MyList<NALUnit*>::iterator iIter = m_cNalUnitList.begin();
    MyList<NALUnit*>::iterator iEnd  = m_cNalUnitList.end  ();
    while( iIter != iEnd )
    {
      if( ISVCL(iIter) && SLICE(iIter)->getRedundantPicCnt() )
      {
        delete *iIter;
        iIter = m_cNalUnitList.erase( iIter );
      }
      else
      {
        iIter++;
      }
    }
  }
}

Void
AccessUnit::xSetParameters()
{
  //===== remove non-required NAL units =====
  {
    MyList<NALUnit*>::reverse_iterator iIter = m_cNalUnitList.rbegin();
    MyList<NALUnit*>::reverse_iterator iEnd  = m_cNalUnitList.rend  ();
    while( iIter != iEnd )
    {
      if( ISVCL(iIter) )
      {
        break;
      }
      iIter++;
    }
    AOT( iIter == iEnd );
    UInt  uiCurrDQId = MSYS_UINT_MAX;
    UInt  uiNextDQId = SLICE(iIter)->getDQId();
    while( iIter != iEnd )
    {
      if( ! ISVCL(iIter) )
      {
        iIter++;
      }
      else if( SLICE(iIter)->getDQId() == uiCurrDQId )
      {
        if( uiNextDQId == MSYS_UINT_MAX )
        {
          uiNextDQId = SLICE(iIter)->getRefLayerDQId();
        }
        iIter++;
      }
      else if( uiNextDQId == MSYS_UINT_MAX || SLICE(iIter)->getDQId() > uiNextDQId )
      {
        // remove non-required layer representation
        MyList<NALUnit*>::reverse_iterator iNext     = iIter; iNext++;
        MyList<NALUnit*>::iterator         iToDelete = iNext.base();
        delete *iToDelete;
        iToDelete = m_cNalUnitList.erase( iToDelete );
        iIter     = static_cast<MyList<NALUnit*>::reverse_iterator>( iToDelete );
        iEnd      = m_cNalUnitList.rend();
      }
      else if( ( SLICE(iIter)->getDQId() >> 4 ) == ( uiNextDQId >> 4 ) )
      {
        if( SLICE(iIter)->getDQId() != uiNextDQId )
        {
          printf( "WARNING: missing layer representation (Q=%d), chosing Q=%d\n", uiNextDQId & 15, SLICE(iIter)->getDQId() & 15 );
        }
        uiCurrDQId  = SLICE(iIter)->getDQId();
        uiNextDQId  = SLICE(iIter)->getRefLayerDQId();
        iIter++;
      }
      else
      {
        printf( "ERROR: referenced dependency representation missing (D=%d)\n", uiNextDQId >> 4 );
        AOT( 1 ); // replace with error concealment code in later versions
      }
    }
    AOF( uiNextDQId == MSYS_UINT_MAX ); // we are missing required packets (replace with error concealment in later version)
  }

  //===== set highest DQId for which AVC rewriting is possible =====
  UInt  uiHighestRewriteDQId = MSYS_UINT_MAX;
  {
    MyList<NALUnit*>::iterator iIter = m_cNalUnitList.begin();
    MyList<NALUnit*>::iterator iEnd  = m_cNalUnitList.end  ();
    while( iIter != iEnd )
    {
      if( ISVCL(iIter) )
      {
        if( SLICE(iIter)->getNoInterLayerPredFlag() || SLICE(iIter)->getTCoeffLevelPredictionFlag() )
        {
          uiHighestRewriteDQId = SLICE(iIter)->getDQId();
        }
      }
      iIter++;
    }
  }

  //===== set required buffer sizes =====
  {
    UInt                       uiAllocMbX = 0;
    UInt                       uiAllocMbY = 0;
    MyList<NALUnit*>::iterator iIter      = m_cNalUnitList.begin();
    MyList<NALUnit*>::iterator iEnd       = m_cNalUnitList.end  ();
    while( iIter != iEnd )
    {
      if( ISVCL(iIter) )
      {
        uiAllocMbX  = gMax( uiAllocMbX, SLICE(iIter)->getFrameWidthInMb () );
        uiAllocMbY  = gMax( uiAllocMbY, SLICE(iIter)->getFrameHeightInMb() );
        SLICE(iIter)->setAllocFrameWidthInMbs ( uiAllocMbX );
        SLICE(iIter)->setAllocFrameHeightInMbs( uiAllocMbY );
      }
      iIter++;
    }
  }

  //===== set slice data properties =====
  MyList<NALUnit*>::iterator iEnd  = m_cNalUnitList.end();
  MyList<NALUnit*>::iterator iIter = getNextVCL( m_cNalUnitList.begin(), iEnd, true );
  MyList<NALUnit*>::iterator iNext = getNextVCL( iIter, iEnd );
  while( iIter != iEnd )
  {
    //----- check whether slice is the last slice in a layer or dependency representation ----
    Bool  bLastSliceInLayerRepresentation       = false;
    Bool  bLastSliceInDependencyRepresentation  = false;
    if( iNext == iEnd )
    {
      bLastSliceInLayerRepresentation       = true;
      bLastSliceInDependencyRepresentation  = true;
    }
    else
    {
      bLastSliceInLayerRepresentation       = ( SLICE(iIter)->getDQId         () != SLICE(iNext)->getDQId         () );
      bLastSliceInDependencyRepresentation  = ( SLICE(iIter)->getDependencyId () != SLICE(iNext)->getDependencyId () );
    }
    //----- check whether slices is part of an IDR access unit -----
    Bool  bIsPartOfIDRAccessUnit  = false;
    for( MyList<NALUnit*>::iterator iNIter = iIter; iNIter != iEnd && ! bIsPartOfIDRAccessUnit; iNIter = getNextVCL( iNIter, iEnd ) )
    {
      bIsPartOfIDRAccessUnit = SLICE(iNIter)->getIdrFlag();
    }
    //----- set parameters -----
    SLICE(iIter)->setDQIdMax                            ( SLICE(iIter)->getDQId         () == m_pcLastVCLNALUnit->getDQId        () );
    SLICE(iIter)->setDependencyIdMax                    ( SLICE(iIter)->getDependencyId () == m_pcLastVCLNALUnit->getDependencyId() );
    SLICE(iIter)->setLastSliceInLayerRepresentation     ( bLastSliceInLayerRepresentation );
    SLICE(iIter)->setLastSliceInDependencyRepresentation( bLastSliceInDependencyRepresentation );
    SLICE(iIter)->setLastSliceInAccessUnit              ( iNext == iEnd );
    SLICE(iIter)->setLastAccessUnitInStream             ( m_bEndOfStream );
    SLICE(iIter)->setPartOfIDRAccessUnit                ( bIsPartOfIDRAccessUnit );
    SLICE(iIter)->setHighestRewriteLayer                ( SLICE(iIter)->getDQId         () == uiHighestRewriteDQId );
    //----- update iterators ----
    iIter = iNext;
    iNext = getNextVCL( iIter, iEnd );
  }
}


Void
AccessUnit::xReInit()
{
  m_cNalUnitList = m_cStartOfNewAccessUnit;
  m_bComplete    = false;
  m_cStartOfNewAccessUnit.clear();
}

#undef ISVCL
#undef SLICE





CreaterH264AVCDecoder::CreaterH264AVCDecoder()
: m_pcH264AVCDecoder        ( 0 )
, m_pcParameterSetMngAUInit ( 0 )
, m_pcParameterSetMngDecode ( 0 )
, m_pcSliceReader           ( 0 )
, m_pcNalUnitParser         ( 0 )
, m_pcSliceDecoder          ( 0 )
, m_pcControlMng            ( 0 )
, m_pcBitReadBuffer         ( 0 )
, m_pcUvlcReader            ( 0 )
, m_pcMbParser              ( 0 )
, m_pcLoopFilter            ( 0 )
, m_pcMbDecoder             ( 0 )
, m_pcTransform             ( 0 )
, m_pcIntraPrediction       ( 0 )
, m_pcMotionCompensation    ( 0 )
, m_pcQuarterPelFilter      ( 0 )
, m_pcCabacReader           ( 0 )
, m_pcSampleWeighting       ( 0 )
#ifdef SHARP_AVC_REWRITE_OUTPUT
, m_pcRewriteEncoder        ( 0 )
#endif
{
  ::memset( m_apcDecodedPicBuffer,     0x00, MAX_LAYERS * sizeof( Void* ) );
  ::memset( m_apcLayerDecoder,         0x00, MAX_LAYERS * sizeof( Void* ) );
  ::memset( m_apcPocCalculator,        0x00, MAX_LAYERS * sizeof( Void* ) );
  ::memset( m_apcYuvFullPelBufferCtrl, 0x00, MAX_LAYERS * sizeof( Void* ) );
}

CreaterH264AVCDecoder::~CreaterH264AVCDecoder()
{
}

ErrVal
CreaterH264AVCDecoder::create( CreaterH264AVCDecoder*& rpcCreaterH264AVCDecoder )
{
  rpcCreaterH264AVCDecoder = new CreaterH264AVCDecoder;
  ROT( NULL == rpcCreaterH264AVCDecoder );
  RNOK( rpcCreaterH264AVCDecoder->xCreateDecoder() )
  return Err::m_nOK;
}

ErrVal
CreaterH264AVCDecoder::xCreateDecoder()
{
  RNOK( ParameterSetMng         ::create( m_pcParameterSetMngAUInit ) );
  RNOK( ParameterSetMng         ::create( m_pcParameterSetMngDecode ) );
  RNOK( BitReadBuffer           ::create( m_pcBitReadBuffer ) );
  RNOK( NalUnitParser           ::create( m_pcNalUnitParser) );
  RNOK( SliceReader             ::create( m_pcSliceReader ) );
  RNOK( SliceDecoder            ::create( m_pcSliceDecoder ) );
  RNOK( UvlcReader              ::create( m_pcUvlcReader ) );
  RNOK( CabacReader             ::create( m_pcCabacReader ) );
  RNOK( MbParser                ::create( m_pcMbParser ) );
  RNOK( MbDecoder               ::create( m_pcMbDecoder ) );
  RNOK( LoopFilter              ::create( m_pcLoopFilter ) );
  RNOK( IntraPrediction         ::create( m_pcIntraPrediction ) );
  RNOK( MotionCompensation      ::create( m_pcMotionCompensation ) );
  RNOK( H264AVCDecoder          ::create( m_pcH264AVCDecoder ) );
  RNOK( ControlMngH264AVCDecoder::create( m_pcControlMng ) );
  RNOK( ReconstructionBypass    ::create( m_pcReconstructionBypass ) );
  RNOK( SampleWeighting         ::create( m_pcSampleWeighting ) );
  RNOK( QuarterPelFilter        ::create( m_pcQuarterPelFilter ) );
  RNOK( Transform               ::create( m_pcTransform ) );
#ifdef SHARP_AVC_REWRITE_OUTPUT
  RNOK( RewriteEncoder          ::create( m_pcRewriteEncoder ) );
#endif

  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    RNOK( DecodedPicBuffer      ::create( m_apcDecodedPicBuffer     [uiLayer] ) );
    RNOK( LayerDecoder          ::create( m_apcLayerDecoder         [uiLayer] ) );
    RNOK( PocCalculator         ::create( m_apcPocCalculator        [uiLayer] ) );
    RNOK( YuvBufferCtrl         ::create( m_apcYuvFullPelBufferCtrl [uiLayer] ) );
  }

  return Err::m_nOK;
}


ErrVal
CreaterH264AVCDecoder::destroy()
{
  RNOK( m_pcSliceDecoder          ->destroy() );
  RNOK( m_pcSliceReader           ->destroy() );
  RNOK( m_pcBitReadBuffer         ->destroy() );
  RNOK( m_pcUvlcReader            ->destroy() );
  RNOK( m_pcMbParser              ->destroy() );
  RNOK( m_pcLoopFilter            ->destroy() );
  RNOK( m_pcMbDecoder             ->destroy() );
  RNOK( m_pcTransform             ->destroy() );
  RNOK( m_pcIntraPrediction       ->destroy() );
  RNOK( m_pcMotionCompensation    ->destroy() );
  RNOK( m_pcQuarterPelFilter      ->destroy() );
  RNOK( m_pcCabacReader           ->destroy() );
  RNOK( m_pcNalUnitParser         ->destroy() );
  RNOK( m_pcParameterSetMngAUInit ->destroy() );
  RNOK( m_pcParameterSetMngDecode ->destroy() );
  RNOK( m_pcSampleWeighting       ->destroy() );
  RNOK( m_pcH264AVCDecoder        ->destroy() );
  RNOK( m_pcControlMng            ->destroy() );
  RNOK( m_pcReconstructionBypass  ->destroy() );
#ifdef SHARP_AVC_REWRITE_OUTPUT
  RNOK( m_pcRewriteEncoder        ->destroy() );
#endif

  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    RNOK( m_apcDecodedPicBuffer    [uiLayer]->destroy() );
    RNOK( m_apcLayerDecoder        [uiLayer]->destroy() );
    RNOK( m_apcPocCalculator       [uiLayer]->destroy() );
    RNOK( m_apcYuvFullPelBufferCtrl[uiLayer]->destroy() );
  }
  delete this;
  return Err::m_nOK;
}

ErrVal
CreaterH264AVCDecoder::init( Bool bOpenTrace )
{
  if( bOpenTrace )
  {
    INIT_DTRACE;
  }

  RNOK( m_pcBitReadBuffer         ->init() );
  RNOK( m_pcNalUnitParser         ->init( m_pcBitReadBuffer, m_pcUvlcReader ) );
  RNOK( m_pcUvlcReader            ->init( m_pcBitReadBuffer ) );
  RNOK( m_pcCabacReader           ->init( m_pcBitReadBuffer ) );
  RNOK( m_pcQuarterPelFilter      ->init() );
  RNOK( m_pcParameterSetMngAUInit ->init() );
  RNOK( m_pcParameterSetMngDecode ->init() );
  RNOK( m_pcSampleWeighting       ->init() );
  RNOK( m_pcSliceDecoder          ->init( m_pcMbDecoder, m_pcControlMng ) );
  RNOK( m_pcSliceReader           ->init( m_pcMbParser ) );
  RNOK( m_pcMbParser              ->init() );
  RNOK( m_pcLoopFilter            ->init( m_pcControlMng, m_pcReconstructionBypass, false ) );
  RNOK( m_pcIntraPrediction       ->init() );
  RNOK( m_pcMotionCompensation    ->init( m_pcQuarterPelFilter, m_pcTransform, m_pcSampleWeighting ) );
  RNOK( m_pcMbDecoder             ->init( m_pcTransform, m_pcIntraPrediction, m_pcMotionCompensation ) );
  RNOK( m_pcH264AVCDecoder        ->init( m_pcNalUnitParser,
                                          m_pcUvlcReader,
                                          m_pcParameterSetMngAUInit,
                                          m_pcParameterSetMngDecode,
                                          m_apcLayerDecoder ) );
  RNOK( m_pcReconstructionBypass  ->init() );
#ifdef SHARP_AVC_REWRITE_OUTPUT
  RNOK( m_pcRewriteEncoder        ->init() );
#endif

  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    RNOK( m_apcDecodedPicBuffer[uiLayer]->init ( m_apcYuvFullPelBufferCtrl [uiLayer], uiLayer ) );
#ifdef SHARP_AVC_REWRITE_OUTPUT
    RNOK( m_apcLayerDecoder    [uiLayer]->init ( uiLayer,
                                                 m_pcH264AVCDecoder,
                                                 m_pcNalUnitParser,
                                                 m_pcSliceReader,
                                                 m_pcSliceDecoder,
                                                 m_pcControlMng,
                                                 m_pcLoopFilter,
                                                 m_pcUvlcReader,
                                                 m_pcParameterSetMngDecode,
                                                 m_apcPocCalculator        [uiLayer],
                                                 m_apcYuvFullPelBufferCtrl [uiLayer],
                                                 m_apcDecodedPicBuffer     [uiLayer],
                                                 m_pcMotionCompensation,
												                         m_pcReconstructionBypass,
                                                 m_pcRewriteEncoder ) );
#else
    RNOK( m_apcLayerDecoder    [uiLayer]->init ( uiLayer,
                                                 m_pcH264AVCDecoder,
                                                 m_pcNalUnitParser,
                                                 m_pcSliceReader,
                                                 m_pcSliceDecoder,
                                                 m_pcControlMng,
                                                 m_pcLoopFilter,
                                                 m_pcUvlcReader,
                                                 m_pcParameterSetMngDecode,
                                                 m_apcPocCalculator        [uiLayer],
                                                 m_apcYuvFullPelBufferCtrl [uiLayer],
                                                 m_apcDecodedPicBuffer     [uiLayer],
                                                 m_pcMotionCompensation,
												                         m_pcReconstructionBypass ) );
#endif
  }

  RNOK( m_pcControlMng            ->init( m_pcUvlcReader,
                                          m_pcMbParser,
                                          m_pcMotionCompensation,
                                          m_apcYuvFullPelBufferCtrl,
                                          m_pcCabacReader,
                                          m_pcSampleWeighting,
                                          m_apcLayerDecoder ) );

  return Err::m_nOK;
}

ErrVal
CreaterH264AVCDecoder::uninit( Bool bCloseTrace )
{
  RNOK( m_pcSampleWeighting       ->uninit() );
  RNOK( m_pcQuarterPelFilter      ->uninit() );
  RNOK( m_pcParameterSetMngAUInit ->uninit() );
  RNOK( m_pcParameterSetMngDecode ->uninit() );
  RNOK( m_pcSliceDecoder          ->uninit() );
  RNOK( m_pcSliceReader           ->uninit() );
  RNOK( m_pcNalUnitParser         ->uninit() );
  RNOK( m_pcBitReadBuffer         ->uninit() );
  RNOK( m_pcUvlcReader            ->uninit() );
  RNOK( m_pcMbParser              ->uninit() );
  RNOK( m_pcLoopFilter            ->uninit() );
  RNOK( m_pcMbDecoder             ->uninit() );
  RNOK( m_pcIntraPrediction       ->uninit() );
  RNOK( m_pcMotionCompensation    ->uninit() );
  RNOK( m_pcCabacReader           ->uninit() );
  RNOK( m_pcH264AVCDecoder        ->uninit() );
  RNOK( m_pcControlMng            ->uninit() );
  RNOK( m_pcReconstructionBypass  ->uninit() );
#ifdef SHARP_AVC_REWRITE_OUTPUT
  RNOK( m_pcRewriteEncoder        ->uninit() );
#endif

  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    RNOK( m_apcDecodedPicBuffer    [uiLayer] ->uninit() );
    RNOK( m_apcLayerDecoder        [uiLayer] ->uninit() );
    RNOK( m_apcYuvFullPelBufferCtrl[uiLayer] ->uninit() );
  }

  if( bCloseTrace )
  {
    UNINIT_DTRACE;
  }

  return Err::m_nOK;
}

ErrVal
CreaterH264AVCDecoder::initNALUnit( BinData*& rpcBinData, AccessUnit& rcAccessUnit )
{
  RNOK( m_pcH264AVCDecoder->initNALUnit( rpcBinData, rcAccessUnit ) );
  return Err::m_nOK;
}

ErrVal
CreaterH264AVCDecoder::processNALUnit( PicBuffer*         pcPicBuffer,
                                       PicBufferList&     rcPicBufferOutputList,
                                       PicBufferList&     rcPicBufferUnusedList,
                                       BinDataList&       rcBinDataList,
                                       NALUnit&           rcNALUnit )
{
  RNOK( m_pcH264AVCDecoder->processNALUnit( pcPicBuffer, rcPicBufferOutputList, rcPicBufferUnusedList, rcBinDataList, rcNALUnit ) );
  return Err::m_nOK;
}








H264AVCPacketAnalyzer::H264AVCPacketAnalyzer()
: m_pcBitReadBuffer       ( 0 )
, m_pcUvlcReader          ( 0 )
, m_pcNalUnitParser       ( 0 )
, m_uiStdAVCOffset        ( 0 )
, m_pcNonRequiredSEI      ( 0 )
, m_uiNonRequiredSeiFlag  ( 0 )
, m_uiPrevPicLayer        ( 0 )
, m_uiCurrPicLayer        ( 0 )
{
  for( Int iLayer = 0; iLayer < MAX_SCALABLE_LAYERS; iLayer++ )
  {
    m_silceIDOfSubPicLayer[iLayer] = -1;
  }
}


H264AVCPacketAnalyzer::~H264AVCPacketAnalyzer()
{
  if( m_pcNonRequiredSEI )
  {
    m_pcNonRequiredSEI->destroy();
  }
}


ErrVal
H264AVCPacketAnalyzer::create( H264AVCPacketAnalyzer*& rpcH264AVCPacketAnalyzer )
{
  rpcH264AVCPacketAnalyzer = new H264AVCPacketAnalyzer;
  ROT ( NULL == rpcH264AVCPacketAnalyzer );
  RNOK( rpcH264AVCPacketAnalyzer->xCreate() );
  return Err::m_nOK;
}



ErrVal
H264AVCPacketAnalyzer::xCreate()
{
  RNOK( BitReadBuffer::create( m_pcBitReadBuffer ) );
  RNOK( UvlcReader   ::create( m_pcUvlcReader    ) );
  RNOK( NalUnitParser::create( m_pcNalUnitParser  ) );

  return Err::m_nOK;
}



ErrVal
H264AVCPacketAnalyzer::destroy()
{
  RNOK( m_pcBitReadBuffer ->destroy() );
  RNOK( m_pcUvlcReader    ->destroy() );
  RNOK( m_pcNalUnitParser ->destroy() );
  delete this;
  return Err::m_nOK;
}



ErrVal
H264AVCPacketAnalyzer::init()
{
  RNOK( m_pcBitReadBuffer ->init() );
  RNOK( m_pcUvlcReader    ->init( m_pcBitReadBuffer ) );
  RNOK( m_pcNalUnitParser ->init( m_pcBitReadBuffer, m_pcUvlcReader ) );
  m_uiPrevPicLayer = 0;
  return Err::m_nOK;
}


ErrVal
H264AVCPacketAnalyzer::uninit()
{
  RNOK( m_pcBitReadBuffer ->uninit() );
  RNOK( m_pcUvlcReader    ->uninit() );
  RNOK( m_pcNalUnitParser ->uninit() );
  return Err::m_nOK;
}


ErrVal
H264AVCPacketAnalyzer::process( BinData*            pcBinData,
                                PacketDescription&  rcPacketDescription,
                                SEI::SEIMessage*&   pcScalableSEIMessage )
{
  ROF( pcBinData );

  //===== copy bin data and init NAL unit =====
  UChar*  pucBuffer = new UChar [ pcBinData->size() ];
  ROF( pucBuffer );
  memcpy( pucBuffer, pcBinData->data(), pcBinData->size() );
  BinData         cBinData( pucBuffer, pcBinData->size() );
  BinDataAccessor cBinDataAccessor;
  cBinData.setMemAccessor( cBinDataAccessor );
  RNOK( m_pcNalUnitParser->initNalUnit( cBinDataAccessor ) );


  pcScalableSEIMessage            = 0;
  NalRefIdc   eNalRefIdc          = m_pcNalUnitParser->getNalRefIdc           ();
  NalUnitType eNalUnitType        = m_pcNalUnitParser->getNalUnitType         ();
  UInt        uiLayer             = m_pcNalUnitParser->getDependencyId        ();
  UInt        uiLevel             = m_pcNalUnitParser->getTemporalId          ();
  UInt        uiFGSLayer          = m_pcNalUnitParser->getQualityId           ();
  UInt        uiSimplePriorityId  = m_pcNalUnitParser->getPriorityId          ();
  Bool        bDiscardableFlag    = m_pcNalUnitParser->getDiscardableFlag     ();
  Bool        bApplyToNext        = false;
  Bool        bParameterSet       = ( eNalUnitType == NAL_UNIT_SPS || eNalUnitType == NAL_UNIT_SUBSET_SPS || eNalUnitType == NAL_UNIT_PPS );
  Bool        bScalable           = ( eNalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE );
  UInt        uiSPSid             = 0;
  UInt        uiPPSid             = 0;

  if( eNalUnitType == NAL_UNIT_CODED_SLICE || eNalUnitType == NAL_UNIT_CODED_SLICE_IDR )
  {
    uiLevel = ( eNalRefIdc > 0 ? 0 : 1 + m_uiStdAVCOffset );
  }

  rcPacketDescription.uiNumLevelsQL       = 0;
  rcPacketDescription.bDiscardableHRDSEI  = false;
  for( UInt ui = 0; ui < MAX_NUM_RD_LEVELS; ui++ )
  {
    rcPacketDescription.auiPriorityLevelPR[ui] = 0;
  }

  if( eNalUnitType == NAL_UNIT_SEI )
  {
    SEI::MessageList cMessageList;
    ParameterSetMng* pcParameterSetMng = NULL;
    RNOK( SEI::read( m_pcUvlcReader, cMessageList, pcParameterSetMng ) );

    SEI::MessageList::iterator iter = cMessageList.begin();
    while( ! cMessageList.empty() )
    {
      SEI::SEIMessage* pcSEIMessage = cMessageList.popBack();

      switch( pcSEIMessage->getMessageType() )
      {
      case SEI::SUB_SEQ_INFO:
        {
          SEI::SubSeqInfo* pcSubSeqInfo = (SEI::SubSeqInfo*) pcSEIMessage;
          uiLevel       = pcSubSeqInfo->getSubSeqLayerNum();
          uiLayer       = 0;
          bApplyToNext  = true;
          delete pcSEIMessage;
          break;
        }
      case SEI::SCALABLE_SEI:
        {
          uiLevel = 0;
          uiLayer = 0;
          pcScalableSEIMessage = pcSEIMessage;
          {
            //====set parameters used for further parsing =====
            SEI::ScalableSei* pcSEI    = (SEI::ScalableSei*)pcSEIMessage;
            UInt uiNumScalableLayers  = pcSEI->getNumLayersMinus1() + 1;
            for(UInt uiIndex = 0; uiIndex < uiNumScalableLayers; uiIndex++ )
            {
              if( pcSEI->getDependencyId( uiIndex ) == 0 )
              {
                m_uiStdAVCOffset = pcSEI->getTemporalId( uiIndex );
                pcSEI->setStdAVCOffset( m_uiStdAVCOffset-1 );
              }
              else
              {
                break;
              }
            }
          }

          SEI::ScalableSei* pcSEI    = (SEI::ScalableSei*)pcSEIMessage;
          m_uiNum_layers = pcSEI->getNumLayersMinus1() + 1;
          for(UInt i=0; i < m_uiNum_layers; i++)
          {
            m_ID_ROI[i] = pcSEI->getRoiId(i);
            m_ID_Dependency[i] = pcSEI->getDependencyId(i);
          }
          break;
        }

      case SEI::MOTION_SEI:
        {
          SEI::MotionSEI* pcSEI               = (SEI::MotionSEI*)pcSEIMessage;
          m_silceIDOfSubPicLayer[m_layer_id]  = pcSEI->m_slice_group_id[0];
          delete pcSEIMessage;
          break;
        }

      case SEI::SCALABLE_SEI_LAYERS_NOT_PRESENT:
      case SEI::SCALABLE_SEI_DEPENDENCY_CHANGE:
        {
          pcScalableSEIMessage = pcSEIMessage;
          break;
        }

      case SEI::SUB_PIC_SEI:
        {
          SEI::SubPicSei* pcSEI    = (SEI::SubPicSei*)pcSEIMessage;
          m_layer_id          = pcSEI->getDependencyId();
          bApplyToNext  = true;
          delete pcSEIMessage;
          break;
        }

      case SEI::PRIORITYLEVEL_SEI:
        {
          UInt uiNum = 0;
          UInt uiPriorityLevel = 0;
          SEI::PriorityLevelSEI* pcSEI           = (SEI::PriorityLevelSEI*)pcSEIMessage;
          uiNum = pcSEI->getNumPriorityIds();
          rcPacketDescription.uiNumLevelsQL = uiNum;
          for(UInt ui = 0; ui < uiNum; ui++)
          {
            uiPriorityLevel = pcSEI->getAltPriorityId(ui);
            rcPacketDescription.auiPriorityLevelPR[ui] = uiPriorityLevel;
          }
          uiLayer = pcSEI->getPrDependencyId();
          bApplyToNext = true;
          delete pcSEIMessage;
          break;
        }

      case SEI::NON_REQUIRED_SEI:
        {
          m_pcNonRequiredSEI = (SEI::NonRequiredSei*) pcSEIMessage;
          m_uiNonRequiredSeiFlag = 1;
          break;
        }

      case SEI::SCALABLE_NESTING_SEI:
        {
          Bool bAllPicturesInAuFlag;
          UInt uiNumPictures;
          UInt *puiDependencyId, *puiQualityLevel;
          UInt uiTemporalId;
          SEI::ScalableNestingSei* pcSEI = (SEI::ScalableNestingSei*)pcSEIMessage;
          bAllPicturesInAuFlag = pcSEI->getAllPicturesInAuFlag();
          if( bAllPicturesInAuFlag == 0 )
          {
            uiNumPictures = pcSEI->getNumPictures();
            ROT( uiNumPictures == 0 );
            puiDependencyId = new UInt[uiNumPictures];
            puiQualityLevel = new UInt[uiNumPictures];
            for( UInt uiIndex = 0; uiIndex < uiNumPictures; uiIndex++ )
            {
              puiDependencyId[uiIndex] = pcSEI->getDependencyId(uiIndex);
              puiQualityLevel[uiIndex] = pcSEI->getQualityId(uiIndex);
            }
            uiTemporalId = pcSEI->getTemporalId();
            delete [] puiDependencyId;
            delete [] puiQualityLevel;
          }
          bApplyToNext = true;
          rcPacketDescription.bDiscardableHRDSEI = true;
          delete pcSEIMessage;
          break;
        }

      case SEI::BUFFERING_PERIOD:
        {
          rcPacketDescription.bDiscardableHRDSEI = true;
          rcPacketDescription.bDiscardable = true;
          delete pcSEIMessage;
          break;
        }

      case SEI::PIC_TIMING:
        {
          rcPacketDescription.bDiscardableHRDSEI = true;
          rcPacketDescription.bDiscardable = true;
          delete pcSEIMessage;
          break;
        }

      case SEI::INTEGRITY_CHECK_SEI:
        {
          rcPacketDescription.bDiscardableHRDSEI = true;
          rcPacketDescription.bDiscardable = true;
          delete pcSEIMessage;
          break;
        }

      case SEI::REDUNDANT_PIC_SEI:
        {
          SEI::RedundantPicSei* pcSEI = (SEI::RedundantPicSei*) pcSEIMessage;
          UInt uiNumDIdMinus1;
          UInt uiNumDId;
          UInt *puiNumQIdMinus1, *puiDependencyId;
          UInt **ppuiQualityId, **ppuiNumRedundantPicsMinus1;
          UInt ***pppuiRedundantPicCntMinus1;

          uiNumDIdMinus1 = pcSEI->getNumDIdMinus1( );
          uiNumDId = uiNumDIdMinus1 + 1;
          puiNumQIdMinus1                = new UInt[uiNumDId];
          puiDependencyId                = new UInt[uiNumDId];
          ppuiQualityId                  = new UInt*[uiNumDId];
          ppuiNumRedundantPicsMinus1     = new UInt*[uiNumDId];
          pppuiRedundantPicCntMinus1     = new UInt**[uiNumDId];
          for(UInt ui = 0; ui <= uiNumDIdMinus1; ui++)
          {
            puiDependencyId[ui] = pcSEI->getDependencyId ( ui );
            puiNumQIdMinus1[ui] = pcSEI->getNumQIdMinus1 ( ui );
            ppuiQualityId[ui]                = new UInt[puiNumQIdMinus1[ui]+1];
            ppuiNumRedundantPicsMinus1[ui]   = new UInt[puiNumQIdMinus1[ui]+1];
            pppuiRedundantPicCntMinus1[ui]    = new UInt*[puiNumQIdMinus1[ui]+1];
            for(UInt uj = 0; uj <= puiNumQIdMinus1[ui]; uj++)
            {
              ppuiQualityId[ui][uj]  = pcSEI->getQualityId ( ui, uj );
              ppuiNumRedundantPicsMinus1[ui][uj]  = pcSEI->getNumRedundantPicsMinus1 ( ui, uj );
              pppuiRedundantPicCntMinus1[ui][uj] = new UInt[ppuiNumRedundantPicsMinus1[ui][uj] +1];
              for(UInt uk = 0; uk <= ppuiNumRedundantPicsMinus1[ui][uj]; uk++)
              {
                pppuiRedundantPicCntMinus1[ui][uj][uk] = pcSEI->getRedundantPicCntMinus1 ( ui, uj, uk );
              }
            }
          }

          delete puiNumQIdMinus1;
          delete puiDependencyId;
          delete ppuiQualityId;
          delete ppuiNumRedundantPicsMinus1;
          delete pppuiRedundantPicCntMinus1;
          delete pcSEIMessage;
          break;
        }

      case SEI::TL_SWITCHING_POINT_SEI:
        {
          rcPacketDescription.bDiscardableHRDSEI = true;
          rcPacketDescription.bDiscardable = true;
          delete pcSEIMessage;
          break;
        }

      case SEI::TL0_DEP_REP_IDX_SEI:
        {
          rcPacketDescription.bDiscardableHRDSEI = true;
          rcPacketDescription.bDiscardable = true;
          delete pcSEIMessage;
          break;
        }

      default:
        {
          rcPacketDescription.bDiscardableHRDSEI = true;
          rcPacketDescription.bDiscardable = true;
          delete pcSEIMessage;
        }
      }
    }
  }
  else if( eNalUnitType == NAL_UNIT_SPS || eNalUnitType == NAL_UNIT_SUBSET_SPS )
  {
    SequenceParameterSet* pcSPS = NULL;
    RNOK( SequenceParameterSet::create  ( pcSPS   ) );
    Bool bCompletelyParsed = false;
    RNOK( pcSPS->read( m_pcUvlcReader, eNalUnitType, bCompletelyParsed ) );
    uiSPSid = pcSPS->getSeqParameterSetId();
    pcSPS->destroy();
  }
  else if( eNalUnitType == NAL_UNIT_PPS )
  {
    PictureParameterSet* pcPPS = NULL;
    RNOK( PictureParameterSet::create  ( pcPPS   ) );
    RNOK( pcPPS->read( m_pcUvlcReader, eNalUnitType ) );
    uiPPSid = pcPPS->getPicParameterSetId();
    uiSPSid = pcPPS->getSeqParameterSetId();

    m_uiNumSliceGroupsMinus1 = pcPPS->getNumSliceGroupsMinus1();
    for(UInt i=0; i<= m_uiNumSliceGroupsMinus1; i++)
    {
      uiaAddrFirstMBofROIs[uiPPSid ][i] = pcPPS->getTopLeft (i);
    }

    pcPPS->destroy();
    rcPacketDescription.SPSidRefByPPS[uiPPSid] = uiSPSid;
  }
  else if( eNalUnitType == NAL_UNIT_CODED_SLICE           ||
           eNalUnitType == NAL_UNIT_CODED_SLICE_IDR       ||
           eNalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE    )
  {
    if( ! ( uiLayer == 0 && uiFGSLayer == 0 && eNalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE ) )
    {
      UInt uiTemp;
      RNOK( m_pcUvlcReader->getUvlc( uiTemp,  "SH: first_mb_in_slice" ) );
      rcPacketDescription.uiFirstMb = uiTemp;

      RNOK( m_pcUvlcReader->getUvlc( uiTemp,  "SH: slice_type" ) );
      RNOK( m_pcUvlcReader->getUvlc( uiPPSid, "SH: pic_parameter_set_id" ) );
      uiSPSid = rcPacketDescription.SPSidRefByPPS[uiPPSid];

      m_uiCurrPicLayer = (uiLayer << 4) + uiFGSLayer;
      if((m_uiCurrPicLayer < m_uiPrevPicLayer || (m_uiCurrPicLayer == m_uiPrevPicLayer && m_uiCurrPicLayer == 0))&& m_uiNonRequiredSeiFlag != 1) //prefix unit
      {
        m_pcNonRequiredSEI->destroy();
        m_pcNonRequiredSEI = NULL;
      }
      m_uiNonRequiredSeiFlag = 0;
      m_uiPrevPicLayer = m_uiCurrPicLayer;
    }
  }

  m_pcNalUnitParser->closeNalUnit( false );

  rcPacketDescription.NalUnitType   = eNalUnitType;
  rcPacketDescription.SPSid         = uiSPSid;
  rcPacketDescription.PPSid         = uiPPSid;
  rcPacketDescription.Scalable      = bScalable;
  rcPacketDescription.ParameterSet  = bParameterSet;
  rcPacketDescription.Layer         = uiLayer;
  rcPacketDescription.FGSLayer      = uiFGSLayer;
  rcPacketDescription.Level         = uiLevel;
  rcPacketDescription.ApplyToNext   = bApplyToNext;
  rcPacketDescription.uiPId         = uiSimplePriorityId;
  rcPacketDescription.bDiscardable  = bDiscardableFlag;
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
