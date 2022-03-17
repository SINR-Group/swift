
#include "H264AVCDecoderLib.h"
#include "GOPDecoder.h"
#include "SliceReader.h"
#include "SliceDecoder.h"
#include "CreaterH264AVCDecoder.h"
#include "ControlMngH264AVCDecoder.h"
#include "H264AVCCommonLib/TraceFile.h"
#include <math.h>
#include "H264AVCCommonLib/CFMO.h"
#include "H264AVCCommonLib/ReconstructionBypass.h"



H264AVC_NAMESPACE_BEGIN



//////////////////////////////////////////////////////////////////////////
// DPB UNIT
//////////////////////////////////////////////////////////////////////////
DPBUnit::DPBUnit( YuvBufferCtrl& rcYuvBufferCtrl )
: m_rcYuvBufferCtrl       ( rcYuvBufferCtrl )
, m_ePicStatus            ( PicType( 0 ) )
, m_uiFrameNum            ( MSYS_UINT_MAX )
, m_iLongTermFrameIdx     ( -1  )
, m_bExisting             ( false )
, m_bBaseRepresentation   ( false )
, m_bWaitForOutput        ( false )
, m_bRefPic               ( false )
, m_pcFrame               ( 0 )
, m_pcMbDataCtrlBaseLayer ( 0 )
{
  m_aiPoc                 [0] = m_aiPoc                 [1] = MSYS_INT_MAX;
  m_abUseBasePred         [0] = m_abUseBasePred         [1] = false;
  m_abNeededForReference  [0] = m_abNeededForReference  [1] = false;
  m_abLongTerm            [0] = m_abLongTerm            [1] = false;
}

DPBUnit::~DPBUnit()
{
}

ErrVal
DPBUnit::create( DPBUnit*&                    rpcDPBUnit,
                 YuvBufferCtrl&               rcYuvBufferCtrl,
                 const SequenceParameterSet&  rcSPS,
                 Bool                         bBaseLayer )
{
  ROF(( rpcDPBUnit = new DPBUnit( rcYuvBufferCtrl ) ));
  RNOK( rpcDPBUnit->xCreateData ( rcSPS, bBaseLayer ) );
  return Err::m_nOK;
}

ErrVal
DPBUnit::destroy()
{
  RNOK( xDeleteData() );
  delete this;
  return Err::m_nOK;
}

ErrVal
DPBUnit::xCreateData( const SequenceParameterSet& rcSPS,
                      Bool                        bBaseLayer )
{
  ROT( m_pcFrame );
  ROT( m_pcMbDataCtrlBaseLayer );
  //===== create and init frame =====
  {
    ROF(( m_pcFrame = new Frame( m_rcYuvBufferCtrl, m_rcYuvBufferCtrl, FRAME, 0 ) ));
    RNOK( m_pcFrame->init() );
    m_pcFrame->setDPBUnit( this );
  }
  //===== create and init base layer macroblock data =====
  if( bBaseLayer )
  {
    ROF(( m_pcMbDataCtrlBaseLayer = new MbDataCtrl() ));
    RNOK( m_pcMbDataCtrlBaseLayer->init( rcSPS ) );
  }
  return Err::m_nOK;
}

ErrVal
DPBUnit::xDeleteData()
{
  if( m_pcFrame )
  {
    RNOK( m_pcFrame->uninit () );
    RNOK( m_pcFrame->destroy() );
    m_pcFrame = 0;
  }
  if( m_pcMbDataCtrlBaseLayer )
  {
    RNOK(  m_pcMbDataCtrlBaseLayer->uninit() );
    delete m_pcMbDataCtrlBaseLayer;
    m_pcMbDataCtrlBaseLayer = 0;
  }
  return Err::m_nOK;
}

ErrVal
DPBUnit::uninit()
{
  m_ePicStatus                = PicType( 0 );
  m_uiFrameNum                = MSYS_UINT_MAX;
  m_iLongTermFrameIdx         = -1;
  m_bExisting                 = false;
  m_bBaseRepresentation       = false;
  m_bWaitForOutput            = false;
  m_bRefPic                   = false;
  m_aiPoc                 [0] = m_aiPoc                 [1] = MSYS_INT_MAX;
  m_abUseBasePred         [0] = m_abUseBasePred         [1] = false;
  m_abNeededForReference  [0] = m_abNeededForReference  [1] = false;
  m_abLongTerm            [0] = m_abLongTerm            [1] = false;
  return Err::m_nOK;
}

ErrVal
DPBUnit::dump( UInt uiNumber, Bool bLineBefore, Bool bLineAfter )
{
  ROF( m_ePicStatus );
  Int iWidth  = 60;
  Int iMinFld = ( m_ePicStatus == BOT_FIELD ? 1 : 0 );
  Int iMaxFld = ( m_ePicStatus == TOP_FIELD ? 1 : 2 );
  if( bLineBefore )
  {
    for( Int i = 0; i < iWidth; i++ )         printf( "-" );
    printf  ( "\n" );
  }
  for( Int iFld = iMinFld; iFld < iMaxFld; iFld++ )
  {
    if      ( iFld == iMinFld )               printf( "#%3d:", uiNumber );
    else                                      printf( "     " );            // 5
    if      ( iFld == 0 )                     printf( " [T]" );
    else                                      printf( " [B]" );             // 9 (+4)
    if      ( !m_abNeededForReference[iFld] ) printf( " --" );
    else if ( m_abLongTerm[iFld] )            printf( " LT" );
    else                                      printf( " ST" );              // 12 (+3)
    if      ( m_bWaitForOutput )              printf( "|WO" );
    else                                      printf( "|--" );              // 15 (+3)
    if      ( !m_bExisting )                  printf( " [NE]" );
    else if ( m_bBaseRepresentation )         printf( " [BR]" );
    else if ( m_bRefPic )                     printf( " [RP]" );
    else                                      printf( " [NR]" );            // 20 (+5)
    printf  ( "   FN=%- 6d",    m_uiFrameNum );                             // 32 (+12)
    printf  ( "   LTI=%- 3d",   m_iLongTermFrameIdx );                      // 42 (+10)
    printf  ( "   POC=%- 11d",  m_aiPoc[iFld] );                            // 60 (+18)
    printf  ( "\n" );
  }
  if( bLineAfter )
  {
    for( Int i = 0; i < iWidth; i++ )         printf( "-" );
    printf  ( "\n" );
  }
  return Err::m_nOK;
}

Bool
DPBUnit::isRequired() const
{
  ROTRS( m_abNeededForReference[0], true );
  ROTRS( m_abNeededForReference[1], true );
  return m_bWaitForOutput;
}

Bool
DPBUnit::isUsedForRef() const
{
  ROTRS( m_abNeededForReference[0], true );
  ROTRS( m_abNeededForReference[1], true );
  return false;
}

Bool
DPBUnit::isShortTermUnit() const
{
  ROTRS( m_abNeededForReference[0] && !m_abLongTerm[0], true );
  ROTRS( m_abNeededForReference[1] && !m_abLongTerm[1], true );
  return false;
}

Bool
DPBUnit::isLongTermUnit() const
{
  ROTRS( m_abNeededForReference[0] && m_abLongTerm[0], true );
  ROTRS( m_abNeededForReference[1] && m_abLongTerm[1], true );
  return false;
}

Int
DPBUnit::getMaxPoc( Bool bPocMode0 ) const
{
  ROTRS( bPocMode0 && !m_bExisting, MSYS_INT_MIN );
  ROTRS( m_ePicStatus == TOP_FIELD, m_aiPoc[0] );
  ROTRS( m_ePicStatus == BOT_FIELD, m_aiPoc[1] );
  return gMax( m_aiPoc[0], m_aiPoc[1] );
}

Int
DPBUnit::getPoc() const
{
  ROTRS( m_ePicStatus == TOP_FIELD, m_aiPoc[0] );
  ROTRS( m_ePicStatus == BOT_FIELD, m_aiPoc[1] );
  return gMin( m_aiPoc[0], m_aiPoc[1] );
}

Int
DPBUnit::getFrameNumWrap( UInt uiCurrFrameNum, UInt uiMaxFrameNum ) const
{
  ROFRS( m_uiFrameNum > uiCurrFrameNum, (Int)m_uiFrameNum );
  return Int( m_uiFrameNum - uiMaxFrameNum );
}

Bool
DPBUnit::isShortTermRef( PicType ePicType ) const
{
  ROFRS( ePicType == FRAME, m_abNeededForReference[ePicType-1] && !m_abLongTerm[ePicType-1] );
  return isShortTermRef( TOP_FIELD ) && isShortTermRef( BOT_FIELD );
}

Bool
DPBUnit::isLongTermRef( PicType ePicType ) const
{
  ROFRS( ePicType == FRAME, m_abNeededForReference[ePicType-1] && m_abLongTerm[ePicType-1] );
  return isLongTermRef( TOP_FIELD ) && isLongTermRef( BOT_FIELD );
}

Bool
DPBUnit::isRefPic( PicType ePicType ) const
{
  ROFRS( ePicType == FRAME, m_abNeededForReference[ePicType-1] );
  return isRefPic( TOP_FIELD ) && isRefPic( BOT_FIELD );
}

ErrVal
DPBUnit::setNonExisting( UInt uiFrameNum, Int iTopFieldPoc, Int iBotFieldPoc )
{
  RNOK( uninit() );
  m_ePicStatus              = FRAME;
  m_uiFrameNum              = uiFrameNum;
  m_bRefPic                 = true;
  m_aiPoc               [0] = iTopFieldPoc;
  m_aiPoc               [1] = iBotFieldPoc;
  m_abNeededForReference[0] = true;
  m_abNeededForReference[1] = true;
  return Err::m_nOK;
}

ErrVal
DPBUnit::output( PicBufferList& rcPicBufferInputList, PicBufferList& rcPicBufferOutputList, PicBufferList& rcPicBufferUnusedList )
{
  ROF( m_bWaitForOutput );
  ROT( rcPicBufferInputList.empty() );
  //----- store frame in picture buffer and insert into list -----
  PicBuffer*  pcPicBuffer = rcPicBufferInputList.popFront();
  ROF( pcPicBuffer );
  ROF( pcPicBuffer->getBuffer () );
  if ( pcPicBuffer->isUsed    () )
  {
    pcPicBuffer->setUnused();
  }
  RNOK( m_pcFrame->store( pcPicBuffer, m_ePicStatus ) );
  rcPicBufferOutputList.push_back( pcPicBuffer );
  rcPicBufferUnusedList.push_back( pcPicBuffer );
  //----- update DPB unit status -----
  m_bWaitForOutput = false;
  return Err::m_nOK;
}

ErrVal
DPBUnit::markUnusedForRef( PicType ePicType, Bool bRemoveOutputFlag )
{
  if( ( ePicType & TOP_FIELD ) == TOP_FIELD )
  {
    m_abNeededForReference[0] = false;
    m_abLongTerm          [0] = false;
  }
  if( ( ePicType & BOT_FIELD ) == BOT_FIELD )
  {
    m_abNeededForReference[1] = false;
    m_abLongTerm          [1] = false;
  }
  if( !m_abLongTerm[0] && !m_abLongTerm[1] )
  {
    m_iLongTermFrameIdx = -1;
  }
  if( bRemoveOutputFlag )
  {
    ROF( ePicType == FRAME );
    m_bWaitForOutput = false;
  }
  return Err::m_nOK;
}

ErrVal
DPBUnit::markLongTerm( PicType ePicType, Int iLongTermFrameIdx )
{
  if( ( ePicType & TOP_FIELD ) == TOP_FIELD )
  {
    ROF( m_abNeededForReference[0] );
    ROT( m_abLongTerm          [0] );
    m_abLongTerm[0] = true;
  }
  if( ( ePicType & BOT_FIELD ) == BOT_FIELD )
  {
    ROF( m_abNeededForReference[1] );
    ROT( m_abLongTerm          [1] );
    m_abLongTerm[1] = true;
  }
  ROF( m_iLongTermFrameIdx == -1 || m_iLongTermFrameIdx == iLongTermFrameIdx );
  m_iLongTermFrameIdx = iLongTermFrameIdx;
  return Err::m_nOK;
}

ErrVal
DPBUnit::decreasePoc( Int iMMCO5Poc )
{
  ROF( m_ePicStatus );
  if( ( m_ePicStatus & TOP_FIELD ) == TOP_FIELD )
  {
    m_aiPoc[0] -= iMMCO5Poc;
  }
  if( ( m_ePicStatus & BOT_FIELD ) == BOT_FIELD )
  {
    m_aiPoc[1] -= iMMCO5Poc;
  }
  return Err::m_nOK;
}

ErrVal
DPBUnit::checkStatus( Int iMaxLongTermFrameIdx )
{
  ROF(  m_ePicStatus );
  ROT( !m_abLongTerm[0] && !m_abLongTerm[1] && m_iLongTermFrameIdx != -1 );
  ROT( !m_abNeededForReference[0] && m_abLongTerm[0] );
  ROT( !m_abNeededForReference[1] && m_abLongTerm[1] );
  ROT(  m_abNeededForReference[0] && ( m_ePicStatus & TOP_FIELD ) == 0 );
  ROT(  m_abNeededForReference[1] && ( m_ePicStatus & BOT_FIELD ) == 0 );
  ROT(  m_abNeededForReference[0] && m_abNeededForReference[1] && m_abLongTerm[0] != m_abLongTerm[1] );
  ROT(  m_iLongTermFrameIdx > iMaxLongTermFrameIdx );
  return Err::m_nOK;
}





//////////////////////////////////////////////////////////////////////////
// CURRENT DPB UNIT
//////////////////////////////////////////////////////////////////////////
CurrDPBUnit::CurrDPBUnit( YuvBufferCtrl& rcYuvBufferCtrl )
: DPBUnit             ( rcYuvBufferCtrl )
, m_bInUse            ( false )
, m_bRefBasePicInUse  ( false )
, m_bCompleted        ( false )
, m_uiQualityId       ( 0 )
, m_pcControlData     ( 0 )
, m_pcRefBasePicFrame ( 0 )
{
}

CurrDPBUnit::~CurrDPBUnit()
{
}

ErrVal
CurrDPBUnit::create( CurrDPBUnit*&                rpcCurrDPBUnit,
                     YuvBufferCtrl&               rcYuvBufferCtrl,
                     const SequenceParameterSet&  rcSPS,
                     Bool                         bCreateRefBasePicBuffer,
                     Bool                         bBaseLayer )
{
  ROF(( rpcCurrDPBUnit = new CurrDPBUnit( rcYuvBufferCtrl ) ));
  RNOK( rpcCurrDPBUnit->xCreateData     ( rcSPS, bCreateRefBasePicBuffer, bBaseLayer ) );
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::destroy()
{
  RNOK( xDeleteData() );
  delete this;
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::xCreateData( const SequenceParameterSet& rcSPS,
                          Bool                        bCreateRefBasePicBuffer,
                          Bool                        bBaseLayer )
{
  ROT ( m_pcControlData );
  ROT ( m_pcRefBasePicFrame );
  //===== create data of base class =====
  RNOK( DPBUnit::xCreateData( rcSPS, bBaseLayer ) );
  m_pcFrame->setDPBUnit( 0 );
  //===== create and init control data =====
  {
    MbDataCtrl* pcMbDataCtrl = 0;
    ROF(( m_pcControlData = new ControlData () ));
    ROF(( pcMbDataCtrl    = new MbDataCtrl  () ));
    RNOK( pcMbDataCtrl    ->init            ( rcSPS ) );
    RNOK( m_pcControlData ->setMbDataCtrl   ( pcMbDataCtrl ) );
  }
  //===== create and init reference base picture frame =====
  if( bCreateRefBasePicBuffer )
  {
    ROF(( m_pcRefBasePicFrame = new Frame( m_rcYuvBufferCtrl, m_rcYuvBufferCtrl, FRAME, 0 ) ));
    RNOK( m_pcRefBasePicFrame->init() );
  }
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::xDeleteData()
{
  RNOK( DPBUnit::xDeleteData() );
  if( m_pcControlData )
  {
    if( m_pcControlData->getMbDataCtrl() )
    {
      RNOK(  m_pcControlData->getMbDataCtrl()->uninit() );
      delete m_pcControlData->getMbDataCtrl();
      m_pcControlData->setMbDataCtrl( 0 );
    }
    delete m_pcControlData;
    m_pcControlData = 0;
  }
  if( m_pcRefBasePicFrame )
  {
    RNOK( m_pcRefBasePicFrame->uninit () );
    RNOK( m_pcRefBasePicFrame->destroy() );
    m_pcRefBasePicFrame = 0;
  }
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::init( SliceHeader& rcSliceHeader )
{
  ROF( !m_bInUse && !m_bRefBasePicInUse && !m_bCompleted );
  ROT( rcSliceHeader.getStoreRefBasePicFlag() && ! rcSliceHeader.getNalRefIdc() );
  ROT( rcSliceHeader.getStoreRefBasePicFlag() && ! m_pcRefBasePicFrame );
  ROT( rcSliceHeader.getDependencyId() == 0   && ! m_pcMbDataCtrlBaseLayer );
  //===== init parameters =====
  m_bInUse              = true;
  m_bRefBasePicInUse    = rcSliceHeader.getStoreRefBasePicFlag();
  m_bCompleted          = false;
  m_uiQualityId         = rcSliceHeader.getQualityId          ();
  m_ePicStatus          = rcSliceHeader.getPicType            ();
  m_uiFrameNum          = rcSliceHeader.getFrameNum           ();
  m_iLongTermFrameIdx   = -1;
  m_bExisting           = true;
  m_bBaseRepresentation = false;
  m_bWaitForOutput      = rcSliceHeader.getOutputFlag         ();
  m_bRefPic             = rcSliceHeader.getNalRefIdc          () > 0;
  Int iFldMin           = ( m_ePicStatus == BOT_FIELD ? 1 : 0 );
  Int iFldMax           = ( m_ePicStatus == TOP_FIELD ? 1 : 2 );
  for( Int iF = iFldMin; iF < iFldMax; iF++ )
  {
    m_aiPoc               [iF]  = rcSliceHeader.getPoc              ( m_ePicStatus );
    m_abUseBasePred       [iF]  = rcSliceHeader.getUseRefBasePicFlag();
    m_abNeededForReference[iF]  = m_bRefPic;
    m_abLongTerm          [iF]  = false;
  }
  //===== init frame =====
  RNOK( m_pcFrame->addFrameFieldBuffer() );
  m_pcFrame->setPoc     ( rcSliceHeader );
  //===== init reference base picture frame =====
  if( m_pcRefBasePicFrame )
  {
    RNOK( m_pcRefBasePicFrame->addFrameFieldBuffer() );
    m_pcRefBasePicFrame->setPoc     ( rcSliceHeader );
  }
  //===== init control data =====
  m_pcControlData->clear();
  RNOK( m_pcControlData->setSliceHeader ( &rcSliceHeader )  );
  RNOK( m_pcControlData->getMbDataCtrl  ()->reset ()        );
  RNOK( m_pcControlData->getMbDataCtrl  ()->clear ()        );
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::reinit( SliceHeader& rcSliceHeader, Bool bNewLayerRepresentation )
{
  ROF( m_bInUse );
  ROF( m_bCompleted       ==  bNewLayerRepresentation );
  ROF( m_bRefBasePicInUse ==  rcSliceHeader.getStoreRefBasePicFlag() );
  ROF( m_uiQualityId      ==  rcSliceHeader.getQualityId          () - ( bNewLayerRepresentation ? 1 : 0 ) );
  ROF( m_ePicStatus       ==  rcSliceHeader.getPicType            () );
  ROF( m_uiFrameNum       ==  rcSliceHeader.getFrameNum           () );
  ROF( m_bWaitForOutput   ==  rcSliceHeader.getOutputFlag         () );
  ROF( m_bRefPic          == (rcSliceHeader.getNalRefIdc          () > 0) );
  Int iFldMin = ( m_ePicStatus == BOT_FIELD ? 1 : 0 );
  Int iFldMax = ( m_ePicStatus == TOP_FIELD ? 1 : 2 );
  for( Int iF = iFldMin; iF < iFldMax; iF++ )
  {
    ROF( m_aiPoc                [iF] == rcSliceHeader.getPoc              ( m_ePicStatus ) );
    ROF( m_abUseBasePred        [iF] == rcSliceHeader.getUseRefBasePicFlag() );
    ROF( m_abNeededForReference [iF] == m_bRefPic );
    ROF( m_abLongTerm           [iF] == false );
  }
  if( bNewLayerRepresentation )
  {
    RNOK( uninit() );
    RNOK( init  ( rcSliceHeader ) );
  }
  else
  {
    RNOK( m_pcControlData->setSliceHeader( &rcSliceHeader ) );
  }
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::resetMMCO5( SliceHeader& rcSliceHeader )
{
  ROT( m_bBaseRepresentation );
  ROF( m_bExisting );
  ROF( m_bRefPic );
  ROF( m_ePicStatus == rcSliceHeader.getPicType () );
  ROF( m_uiFrameNum == rcSliceHeader.getFrameNum() );
  m_uiFrameNum  = 0;
  m_aiPoc[0]    = ( m_ePicStatus == BOT_FIELD ? m_aiPoc[0] : rcSliceHeader.getTopFieldPoc() );
  m_aiPoc[1]    = ( m_ePicStatus == TOP_FIELD ? m_aiPoc[1] : rcSliceHeader.getBotFieldPoc() );
  DPBUnit::getFrame()->setPoc( rcSliceHeader );
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::setComplete( CurrDPBUnit& rcILPredUnit, Bool bDependencyRepresentationFinished )
{
  ROF( m_bInUse && !m_bCompleted );
  //===== copy base layer macroblock data =====
  if( m_pcMbDataCtrlBaseLayer && !m_uiQualityId )
  {
    RNOK( m_pcMbDataCtrlBaseLayer->copyMotion( *m_pcControlData->getMbDataCtrl(), m_ePicStatus ) );
  }
  //===== set complete =====
  m_bCompleted = true;
  //===== set inter-layer prediction DPB unit =====
  if( bDependencyRepresentationFinished )
  {
    if( rcILPredUnit.isCompleted() )
    {
      RNOK( rcILPredUnit.uninit() );
    }
  }
  else
  {
    Frame*        pcFrame         = m_pcFrame;
    MbDataCtrl*   pcMbDataCtrl    = m_pcControlData->getMbDataCtrl  ();
    SliceHeader*  pcSliceHeader   = m_pcControlData->getSliceHeader ();
    m_pcFrame                     = rcILPredUnit.m_pcFrame;
    rcILPredUnit.m_pcFrame        = pcFrame;
    rcILPredUnit.m_bInUse         = true;
    rcILPredUnit.m_bCompleted     = true;
    rcILPredUnit.m_uiQualityId    = m_uiQualityId;
    RNOK( m_pcControlData->setMbDataCtrl( rcILPredUnit.m_pcControlData->getMbDataCtrl() ) );
    RNOK( rcILPredUnit.m_pcControlData->setMbDataCtrl ( pcMbDataCtrl  ) );
    RNOK( rcILPredUnit.m_pcControlData->setSliceHeader( pcSliceHeader ) );
  }
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::store( DPBUnit& rcDPBUnit, Bool bRefBasePic )
{
  //===== store picture =====
  if( rcDPBUnit.m_ePicStatus )
  {
    RNOK( xStore2ndField( rcDPBUnit, bRefBasePic ) );
  }
  else
  {
    RNOK( xStoreFrame   ( rcDPBUnit, bRefBasePic ) );
  }
  //===== fill image border =====
  {
    SliceHeader* pcSliceHeader = m_pcControlData->getSliceHeader();
    ROF ( pcSliceHeader );
    RNOK( rcDPBUnit.m_rcYuvBufferCtrl.initMb() );
    RNOK( rcDPBUnit.m_pcFrame->extendFrame( 0, rcDPBUnit.m_ePicStatus, pcSliceHeader->getSPS().getFrameMbsOnlyFlag() ) );
  }
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::xStoreFrame( DPBUnit& rcDPBUnit, Bool bRefBasePic )
{
  ROF( m_bInUse && m_bCompleted );
  //===== safety checks =====
  ROF( m_ePicStatus > 0 && m_ePicStatus <= FRAME );
  ROF( m_iLongTermFrameIdx >= 0 || ( m_ePicStatus == FRAME ? ( !m_abLongTerm[0] && !m_abLongTerm[1] ) : !m_abLongTerm[m_ePicStatus-1] ) );
  //===== set parameters =====
  rcDPBUnit.m_ePicStatus          = m_ePicStatus;
  rcDPBUnit.m_uiFrameNum          = m_uiFrameNum;
  rcDPBUnit.m_iLongTermFrameIdx   = m_iLongTermFrameIdx;
  rcDPBUnit.m_bExisting           = true;
  rcDPBUnit.m_bBaseRepresentation = bRefBasePic;
  rcDPBUnit.m_bWaitForOutput      = ( bRefBasePic ? false : m_bWaitForOutput );
  rcDPBUnit.m_bRefPic             = m_bRefPic;
  Int iMinFld                     = ( m_ePicStatus == BOT_FIELD ? 1 : 0 );
  Int iMaxFld                     = ( m_ePicStatus == TOP_FIELD ? 1 : 2 );
  for( Int iFld = iMinFld; iFld < iMaxFld; iFld++ )
  {
    rcDPBUnit.m_aiPoc               [iFld]  = m_aiPoc               [iFld];
    rcDPBUnit.m_abUseBasePred       [iFld]  = m_abUseBasePred       [iFld];
    rcDPBUnit.m_abNeededForReference[iFld]  = m_abNeededForReference[iFld];
    rcDPBUnit.m_abLongTerm          [iFld]  = m_abLongTerm          [iFld];
  }
  //===== switch frame =====
  {
    Frame*  pcTmpFrame  = rcDPBUnit.m_pcFrame;
    Frame*& rpcFrame    = ( bRefBasePic ? m_pcRefBasePicFrame : m_pcFrame );
    ROF( rpcFrame && pcTmpFrame );
    rcDPBUnit.m_pcFrame = rpcFrame;
    rpcFrame            = pcTmpFrame;
    rcDPBUnit.m_pcFrame->setDPBUnit( &rcDPBUnit );
    rpcFrame           ->setDPBUnit( 0 );
  }
  //===== switch base layer macroblock data control =====
  if( rcDPBUnit.m_pcMbDataCtrlBaseLayer )
  {
    ROF( m_pcMbDataCtrlBaseLayer );
    MbDataCtrl* pcTmpMbDataCtrl       = rcDPBUnit.m_pcMbDataCtrlBaseLayer;
    rcDPBUnit.m_pcMbDataCtrlBaseLayer = m_pcMbDataCtrlBaseLayer;
    m_pcMbDataCtrlBaseLayer           = pcTmpMbDataCtrl;
  }
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::xStore2ndField( DPBUnit& rcDPBUnit, Bool bRefBasePic )
{
  ROF( m_bInUse && m_bCompleted );
  //===== safety checks =====
  ROF( rcDPBUnit.m_bExisting );
  ROF( rcDPBUnit.m_ePicStatus == TOP_FIELD || rcDPBUnit.m_ePicStatus == BOT_FIELD );
  ROF( rcDPBUnit.m_ePicStatus + m_ePicStatus  == FRAME );
  ROF( rcDPBUnit.m_uiFrameNum                 == m_uiFrameNum );
  ROF( rcDPBUnit.m_iLongTermFrameIdx          == m_iLongTermFrameIdx || rcDPBUnit.m_abLongTerm[rcDPBUnit.m_ePicStatus-1] != m_abLongTerm[m_ePicStatus-1] );
  ROF( rcDPBUnit.m_bExisting                  == m_bExisting );
  ROF( rcDPBUnit.m_bBaseRepresentation        == bRefBasePic );
  ROF( rcDPBUnit.m_bWaitForOutput             == m_bWaitForOutput );
  ROF( rcDPBUnit.m_bRefPic                    == m_bRefPic );
  //===== update parameters =====
  Int i2ndF                               = m_ePicStatus - 1;
  rcDPBUnit.m_ePicStatus                  = FRAME;
  rcDPBUnit.m_iLongTermFrameIdx           = ( m_abLongTerm[i2ndF] ? m_iLongTermFrameIdx : rcDPBUnit.m_iLongTermFrameIdx );
  rcDPBUnit.m_bBaseRepresentation         = bRefBasePic;
  rcDPBUnit.m_aiPoc               [i2ndF] = m_aiPoc               [i2ndF];
  rcDPBUnit.m_abUseBasePred       [i2ndF] = m_abUseBasePred       [i2ndF];
  rcDPBUnit.m_abNeededForReference[i2ndF] = m_abNeededForReference[i2ndF];
  rcDPBUnit.m_abLongTerm          [i2ndF] = m_abLongTerm          [i2ndF];
  //===== update frame =====
  {
    SliceHeader*  pcSliceHeader = m_pcControlData->getSliceHeader();
    Frame*        pcFrame       = ( bRefBasePic ? m_pcRefBasePicFrame : m_pcFrame );
    ROF ( pcSliceHeader );
    ROF ( pcFrame );
    RNOK( rcDPBUnit.m_pcFrame->copy( pcFrame, m_ePicStatus ) );
    rcDPBUnit.m_pcFrame->setPoc( *pcSliceHeader );
  }
  //===== update base layer macroblock data =====
  if( rcDPBUnit.m_pcMbDataCtrlBaseLayer )
  {
    ROF ( m_pcMbDataCtrlBaseLayer );
    RNOK( rcDPBUnit.m_pcMbDataCtrlBaseLayer->copyMotion( *m_pcMbDataCtrlBaseLayer, m_ePicStatus ) );
  }
  return Err::m_nOK;
}

ErrVal
CurrDPBUnit::uninit()
{
  ROF ( m_bInUse && m_bCompleted );
  RNOK( DPBUnit::uninit() );
  m_bInUse            = false;
  m_bRefBasePicInUse  = false;
  m_bCompleted        = false;
  m_uiQualityId       = 0;
  return Err::m_nOK;
}





//////////////////////////////////////////////////////////////////////////
// REFERENCE PICTURE ENTRY
//////////////////////////////////////////////////////////////////////////
RefPicEntry::RefPicEntry( PicType ePicType, DPBUnit* pcDPBUnit )
: m_ePicType  ( ePicType )
, m_pcDPBUnit ( pcDPBUnit )
{
}

RefPicEntry::RefPicEntry( const RefPicEntry& rcRefPicEntry )
: m_ePicType  ( rcRefPicEntry.m_ePicType )
, m_pcDPBUnit ( rcRefPicEntry.m_pcDPBUnit )
{
}

RefPicEntry::~RefPicEntry()
{
}



//////////////////////////////////////////////////////////////////////////
// DECODED PICTURE BUFFER
//////////////////////////////////////////////////////////////////////////
DecodedPicBuffer::DecodedPicBuffer()
: m_bInitDone               ( false )
, m_bInitBufferDone         ( false )
, m_bDebugOutput            ( false )
, m_pcYuvBufferCtrl         ( 0 )
, m_uiDependencyId          ( 0 )
, m_uiFrameWidthInMbs       ( 0 )
, m_uiFrameHeightInMbs      ( 0 )
, m_uiBufferSizeInFrames    ( 0 )
, m_uiDPBSizeInFrames       ( 0 )
, m_uiMaxNumRefFrames       ( 0 )
, m_uiMaxFrameNum           ( 0 )
, m_uiLastRefFrameNum       ( MSYS_UINT_MAX )
, m_iMaxLongTermFrameIdx    ( -1 )
, m_pcLastDPBUnit           ( 0 )
, m_pcLastDPBUnitRefBasePic ( 0 )
, m_pcCurrDPBUnit           ( 0 )
, m_pcCurrDPBUnitILPred     ( 0 )
{
}

DecodedPicBuffer::~DecodedPicBuffer()
{
}

ErrVal
DecodedPicBuffer::create( DecodedPicBuffer*& rpcDecodedPicBuffer )
{
  ROF(( rpcDecodedPicBuffer = new DecodedPicBuffer() ));
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::destroy()
{
  RNOK( uninit() );
  delete this;
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::init( YuvBufferCtrl* pcYuvBufferCtrl, UInt uiDependencyId )
{
  ROT( m_bInitDone );
  ROT( m_bInitBufferDone );
  ROF( pcYuvBufferCtrl );
  m_bInitDone       = true;
  m_bInitBufferDone = false;
  m_pcYuvBufferCtrl = pcYuvBufferCtrl;
  m_uiDependencyId  = uiDependencyId;
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::uninit()
{
  RNOK( xDeleteData() );
  m_bInitDone               = false;
  m_bInitBufferDone         = false;
  m_pcYuvBufferCtrl         = 0;
  m_uiDependencyId          = 0;
  m_uiFrameWidthInMbs       = 0;
  m_uiFrameHeightInMbs      = 0;
  m_uiBufferSizeInFrames    = 0;
  m_uiDPBSizeInFrames       = 0;
  m_uiMaxNumRefFrames       = 0;
  m_uiMaxFrameNum           = 0;
  m_uiLastRefFrameNum       = MSYS_UINT_MAX;
  m_iMaxLongTermFrameIdx    = -1;
  m_pcLastDPBUnit           = 0;
  m_pcLastDPBUnitRefBasePic = 0;
  m_pcCurrDPBUnit           = 0;
  m_pcCurrDPBUnitILPred     = 0;
  return Err::m_nOK;
}

Bool
DecodedPicBuffer::xNewBufferDimension( const SequenceParameterSet& rcSPS )
{
  ROFRS ( m_bInitBufferDone,                                      true );
  ROFRS ( m_uiFrameWidthInMbs     == rcSPS.getFrameWidthInMbs (), true );
  ROFRS ( m_uiFrameHeightInMbs    == rcSPS.getFrameHeightInMbs(), true );
  ROFRS ( m_uiBufferSizeInFrames  >= rcSPS.getMaxDPBSize      (), true );
  return  false;
}

ErrVal
DecodedPicBuffer::xInitBuffer( const SliceHeader& rcSliceHeader )
{
  ROF( m_bInitDone );
  ROF( rcSliceHeader.getIdrFlag() );
  ROF( rcSliceHeader.getSPS().getMaxDPBSize   () );
  ROF( rcSliceHeader.getSPS().getNumRefFrames () <= rcSliceHeader.getSPS().getMaxDPBSize() );
  m_uiDPBSizeInFrames       = rcSliceHeader.getSPS().getMaxDPBSize();
  m_uiMaxNumRefFrames       = gMax( 1, rcSliceHeader.getSPS().getNumRefFrames() );
  m_uiMaxFrameNum           = 1 << rcSliceHeader.getSPS().getLog2MaxFrameNum();
  m_uiLastRefFrameNum       = MSYS_UINT_MAX;
  m_iMaxLongTermFrameIdx    = -1;
  m_pcLastDPBUnit           = 0;
  m_pcLastDPBUnitRefBasePic = 0;
  ROFRS( xNewBufferDimension( rcSliceHeader.getSPS() ), Err::m_nOK );
  RNOK ( xCreateData        ( rcSliceHeader.getSPS(), m_uiDPBSizeInFrames, m_uiDependencyId == 0 ) );
  m_uiFrameWidthInMbs       = rcSliceHeader.getSPS().getFrameWidthInMbs ();
  m_uiFrameHeightInMbs      = rcSliceHeader.getSPS().getFrameHeightInMbs();
  m_uiBufferSizeInFrames    = m_uiDPBSizeInFrames;
  m_bInitBufferDone         = true;
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xCreateData( const SequenceParameterSet&  rcSPS,
                               UInt                         uiDPBSizeInFrames,
                               Bool                         bBaseLayer )
{
  ROF ( m_bInitDone );
  RNOK( xDeleteData() );
  //===== create DPB buffer list =====
  uiDPBSizeInFrames += 2; // up to two frame buffers for temporary usage
  while( uiDPBSizeInFrames-- )
  {
    DPBUnit* pcDPBUnit = NULL;
    RNOK( DPBUnit::create( pcDPBUnit, *m_pcYuvBufferCtrl, rcSPS, bBaseLayer ) );
    m_cFreeDPBUnitList.pushBack( pcDPBUnit );
  }
  //===== create current DPB units =====
  RNOK( CurrDPBUnit::create( m_pcCurrDPBUnit,       *m_pcYuvBufferCtrl, rcSPS, true,  bBaseLayer ) );
  RNOK( CurrDPBUnit::create( m_pcCurrDPBUnitILPred, *m_pcYuvBufferCtrl, rcSPS, false, false      ) );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xDeleteData()
{
  //===== check picture buffer list =====
  ROF( m_cPicBufferList.empty() );
  //===== delete DPB units =====
  m_cFreeDPBUnitList += m_cUsedDPBUnitList;
  m_cUsedDPBUnitList.clear();
  while( m_cFreeDPBUnitList.size() )
  {
    DPBUnit* pcDPBUnit = m_cFreeDPBUnitList.popFront();
    ROF ( pcDPBUnit );
    RNOK( pcDPBUnit->destroy() );
  }
  //===== delete current DPB units =====
  if( m_pcCurrDPBUnit )
  {
    RNOK( m_pcCurrDPBUnit->destroy() );
    m_pcCurrDPBUnit = 0;
  }
  if( m_pcCurrDPBUnitILPred )
  {
    RNOK( m_pcCurrDPBUnitILPred->destroy() );
    m_pcCurrDPBUnitILPred = 0;
  }
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::initCurrDPBUnit( CurrDPBUnit*&  rpcCurrDPBUnit,
                                   PicBuffer*&    rpcPicBuffer,
                                   SliceHeader&   rcSliceHeader,
                                   PocCalculator& rcPocCalculator,
                                   PicBufferList& rcOutputList,
                                   PicBufferList& rcUnusedList,
                                   Bool           bFirstSliceInDependencyRepresentation,
                                   Bool           bFirstSliceInLayerRepresentation )
{
  ROF( m_bInitDone );
  ROF( m_bInitBufferDone || rcSliceHeader.getIdrFlag() );
  ROF( rcSliceHeader.getDependencyId() == m_uiDependencyId );
  if( bFirstSliceInDependencyRepresentation )
  {
    ROF ( rpcPicBuffer );
    ROF ( bFirstSliceInLayerRepresentation );
    ROT ( rcSliceHeader.getQualityId() );
    //--- initialize buffer when required ---
    if( rcSliceHeader.getIdrFlag() )
    {
      if( m_bInitBufferDone )
      {
        RNOK( xMarkAllUnusedForRef( rcSliceHeader.getNoOutputOfPriorPicsFlag(), &rcUnusedList ) );
        RNOK( xBumpingOutput      ( rcOutputList, rcUnusedList, true ) );
        RNOK( xCheckBufferStatus  () );
      }
      RNOK  ( xInitBuffer         ( rcSliceHeader ) );
    }
    //--- check status ---
    ROF ( m_pcCurrDPBUnit       ->isUninitialized() );
    ROF ( m_pcCurrDPBUnitILPred ->isUninitialized() );
    //--- store picture buffer ---
    if( rcSliceHeader.getOutputFlag() && !xIs2ndFieldOfCompFieldPair( rcSliceHeader ) )
    {
      m_cPicBufferList.push_back( rpcPicBuffer );
      rpcPicBuffer = 0;
    }
    //--- check for gaps in frame_num ---
    RNOK( xCheckGapsInFrameNum          ( rcSliceHeader, rcPocCalculator, rcOutputList, rcUnusedList ) );
    //--- initialize POC and DPB unit ---
    RNOK( rcPocCalculator .calculatePoc ( rcSliceHeader ) );
    RNOK( m_pcCurrDPBUnit->init         ( rcSliceHeader ) );
  }
  else
  {
    //--- check status ---
    ROF ( m_pcCurrDPBUnit       ->isCompleted () == bFirstSliceInLayerRepresentation     );
    ROF ( m_pcCurrDPBUnitILPred ->isCompleted () == ( rcSliceHeader.getQualityId() > 0 ) );
    //--- initialize POC and DPB unit ---
    RNOK( rcPocCalculator        .calculatePoc( rcSliceHeader ) );
    RNOK( m_pcCurrDPBUnit       ->reinit      ( rcSliceHeader, bFirstSliceInLayerRepresentation ) );
  }
  rpcCurrDPBUnit = m_pcCurrDPBUnit;
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::storeCurrDPBUnit( Bool bDependencyRepresentationFinished )
{
  ROF ( m_bInitDone && m_bInitBufferDone );
  ROF ( m_pcCurrDPBUnit->inCurrentUse () );
  RNOK( m_pcCurrDPBUnit->setComplete  ( *m_pcCurrDPBUnitILPred, bDependencyRepresentationFinished ) );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::updateBuffer( PocCalculator& rcPocCalculator, PicBufferList& rcOutputList, PicBufferList& rcUnusedList )
{
  ROF   ( m_bInitDone );
  ROTRS ( !m_bInitBufferDone || m_pcCurrDPBUnit->isUninitialized(),  Err::m_nOK );
  ROF   ( m_pcCurrDPBUnit->isCompleted()   );
  //===== ununit inter-layer prediction DPB unit =====
  if( m_pcCurrDPBUnitILPred->isCompleted() )
  {
    RNOK( m_pcCurrDPBUnitILPred->uninit() );
  }
  //===== update DPB and check whether current DPB unit was correctly stored =====
  RNOK  ( xUpdateAndStoreCurrentPic( rcPocCalculator, rcOutputList, rcUnusedList ) );
  ROF   ( m_pcCurrDPBUnit->isUninitialized() );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::finish( PicBufferList& rcOutputList, PicBufferList& rcUnusedList )
{
  ROF   ( m_bInitDone );
  ROFRS ( m_bInitBufferDone, Err::m_nOK );
  ROF   ( m_pcCurrDPBUnit      ->isUninitialized() );
  ROF   ( m_pcCurrDPBUnitILPred->isUninitialized() );
  RNOK  ( xMarkAllUnusedForRef    () );
  RNOK  ( xBumpingOutput          ( rcOutputList, rcUnusedList, true ) );
  RNOK  ( xCheckBufferStatus      () );
  ROF   ( m_cUsedDPBUnitList.empty() );
  ROF   ( m_cPicBufferList  .empty() );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::setPrdRefLists( CurrDPBUnit* pcCurrDPBUnit )
{
  ROF( m_pcCurrDPBUnit == pcCurrDPBUnit  );
  ROF( m_pcCurrDPBUnit->inCurrentUse  () );
  ROF( m_pcCurrDPBUnit->getSliceHeader() );
  SliceHeader&  rcSliceHeader = *m_pcCurrDPBUnit->getSliceHeader();
  RefFrameList& rcRefPicList0 =  m_pcCurrDPBUnit->getCtrlData   ().getPrdFrameList( LIST_0 );
  RefFrameList& rcRefPicList1 =  m_pcCurrDPBUnit->getCtrlData   ().getPrdFrameList( LIST_1 );
  MbDataCtrl*   pcMbDataCtrl0 =  0;
  rcRefPicList0.reset();
  rcRefPicList1.reset();
  m_pcCurrDPBUnit->getCtrlData().setMbDataCtrl0L1( pcMbDataCtrl0 );
  ROTRS ( rcSliceHeader.isIntraSlice(), Err::m_nOK );
  //===== set list 0 =====
  RefPicEntryList cRefPicEntryList0;
  RefPicEntryList cRefPicEntryList1;
  RNOK  ( xCreateInitialRefPicLists ( cRefPicEntryList0,  cRefPicEntryList1,  rcSliceHeader ) );
  RNOK  ( xRefPicListModification   ( cRefPicEntryList0,  rcSliceHeader,      LIST_0 ) );
  RNOK  ( xSetReferencePictureList  ( rcRefPicList0,      cRefPicEntryList0,  rcSliceHeader.getNumRefIdxL0Active() ) );
  RNOK  ( xDumpRefPicList           ( rcRefPicList0,      LIST_0,             "final" ) );
  ROFRS ( rcSliceHeader.isBSlice(), Err::m_nOK );
  //===== set list 1 =====
  RNOK  ( xRefPicListModification   ( cRefPicEntryList1,  rcSliceHeader,     LIST_1 ) );
  RNOK  ( xSetReferencePictureList  ( rcRefPicList1,      cRefPicEntryList1, rcSliceHeader.getNumRefIdxL1Active() ) );
  RNOK  ( xDumpRefPicList           ( rcRefPicList1,      LIST_1,             "final" ) );
  if( rcSliceHeader.isH264AVCCompatible() )
  {
    RNOK( xSetMbDataCtrlEntry0      ( pcMbDataCtrl0,      cRefPicEntryList1 ) );
    m_pcCurrDPBUnit->getCtrlData().setMbDataCtrl0L1( pcMbDataCtrl0 );
  }
  return Err::m_nOK;
}

CurrDPBUnit*
DecodedPicBuffer::getILPredDPBUnit()
{
  ROTRS( m_pcCurrDPBUnitILPred->isCompleted(),  m_pcCurrDPBUnitILPred );
  ROTRS( m_pcCurrDPBUnit      ->isCompleted(),  m_pcCurrDPBUnit );
  return 0;
}

Bool
DecodedPicBuffer::xIs2ndFieldOfCompFieldPair( const SliceHeader& rcSliceHeader )
{
  ROFRS( m_pcLastDPBUnit,                                                                   false );
  ROFRS( m_pcLastDPBUnit->isExisting      (),                                               false );
  ROFRS( rcSliceHeader.getFieldPicFlag    (),                                               false );
  ROTRS( rcSliceHeader.getIdrFlag         (),                                               false );
  ROTRS( rcSliceHeader.getDecRefPicMarking().hasMMCO5(),                                    false );
  ROFRS( rcSliceHeader.getPicType         ()  + m_pcLastDPBUnit->getPicStatus () == FRAME,  false );
  ROFRS( rcSliceHeader.getFrameNum        () == m_pcLastDPBUnit->getFrameNum  (),           false );
  ROFRS( rcSliceHeader.isRefPic           () == m_pcLastDPBUnit->isRefPicUnit (),           false );
  return true;
}

Bool
DecodedPicBuffer::xIs2ndFieldOfCompFieldPair( Bool bRefBasePic )
{
  DPBUnit*  pcLastDPBUnit = ( bRefBasePic ? m_pcLastDPBUnitRefBasePic : m_pcLastDPBUnit );
  ROFRS( pcLastDPBUnit,                                                               false );
  ROFRS( pcLastDPBUnit->isExisting  (),                                               false );
  ROTRS( pcLastDPBUnit->getPicStatus() == FRAME,                                      false );
  ROFRS( pcLastDPBUnit->getPicStatus() +  m_pcCurrDPBUnit->getPicStatus () == FRAME,  false );
  ROFRS( pcLastDPBUnit->getFrameNum () == m_pcCurrDPBUnit->getFrameNum  (),           false );
  ROFRS( pcLastDPBUnit->isRefPicUnit() == m_pcCurrDPBUnit->isRefPicUnit (),           false );
  return true;
}

ErrVal
DecodedPicBuffer::xInsertNonExistingFrame( const SliceHeader* pcSliceHeader, UInt uiFrameNum )
{
  //===== determine POC =====
  Int iTopFieldPoc = ( pcSliceHeader ? pcSliceHeader->getPoc( TOP_FIELD ) : MSYS_INT_MIN );
  Int iBotFieldPoc = ( pcSliceHeader ? pcSliceHeader->getPoc( BOT_FIELD ) : MSYS_INT_MIN );
  //===== insert new non-existing frame unit =====
  ROT( m_cFreeDPBUnitList.empty() );
  DPBUnit* pcDPBUnit = m_cFreeDPBUnitList.popFront();
  RNOK(    pcDPBUnit->setNonExisting( uiFrameNum, iTopFieldPoc, iBotFieldPoc ) );
  m_cUsedDPBUnitList.push_back( pcDPBUnit );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xInsertCurrentInNewBuffer( DPBUnit*& rpcStoredDPBUnit, Bool bRefBasePic )
{
  ROF ( m_uiMaxNumRefFrames > ( bRefBasePic ? 1U : 0U ) );
  ROF ( m_pcCurrDPBUnit->isCompleted() );
  ROT ( m_cFreeDPBUnitList.empty() );
  ROF(( rpcStoredDPBUnit = m_cFreeDPBUnitList.popFront() ));
  RNOK( m_pcCurrDPBUnit->store( *rpcStoredDPBUnit, bRefBasePic ) );
  m_cUsedDPBUnitList.push_back(  rpcStoredDPBUnit );
  if( ! bRefBasePic && m_pcCurrDPBUnit->isRefPicUnit() )
  {
    m_uiLastRefFrameNum = m_pcCurrDPBUnit->getFrameNum();
  }
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xCheckGapsInFrameNum( const SliceHeader& rcSliceHeader, PocCalculator& rcPocCalculator, PicBufferList& rcOutputList, PicBufferList& rcUnusedList )
{
  ROTRS( xIs2ndFieldOfCompFieldPair ( rcSliceHeader ),                                        Err::m_nOK );
  ROTRS( rcSliceHeader.getIdrFlag   (),                                                       Err::m_nOK );
  ROTRS( rcSliceHeader.getFrameNum  () == ( ( m_uiLastRefFrameNum + 1 ) % m_uiMaxFrameNum ),  Err::m_nOK );

  SliceHeader*  pcNonExSliceHeader  = 0;
  UInt          uiCurrFrameNum      = rcSliceHeader.getFrameNum();
  UInt          uiMissingFrames     = uiCurrFrameNum - m_uiLastRefFrameNum - 1 + ( uiCurrFrameNum <= m_uiLastRefFrameNum ? m_uiMaxFrameNum : 0 );
  if( !rcSliceHeader.getSPS().getGapsInFrameNumValueAllowedFlag() )
  {
    fprintf( stderr, "\nLOST FRAMES = %d\n", uiMissingFrames );
    RERR();
  }
  if( rcSliceHeader.getSPS().getPicOrderCntType() )
  {
    ROF(( pcNonExSliceHeader = new SliceHeader( rcSliceHeader ) ));
    pcNonExSliceHeader->setNalRefIdc          ( NAL_REF_IDC_PRIORITY_HIGH );
    pcNonExSliceHeader->setNalUnitType        ( NAL_UNIT_CODED_SLICE );
    pcNonExSliceHeader->setIdrFlag            ( false );
    pcNonExSliceHeader->setFieldPicFlag       ( false );
    pcNonExSliceHeader->setBottomFieldFlag    ( false );
    pcNonExSliceHeader->setDeltaPicOrderCnt0  ( 0 );
    pcNonExSliceHeader->setDeltaPicOrderCnt1  ( 0 );
  }
  while( uiMissingFrames-- )
  {
    UInt  uiFrameNum    = ( m_uiLastRefFrameNum + 1 ) % m_uiMaxFrameNum;
    m_uiLastRefFrameNum = uiFrameNum;
    if( pcNonExSliceHeader )
    {
      pcNonExSliceHeader  ->setFrameNum ( uiFrameNum );
      RNOK( rcPocCalculator.calculatePoc( *pcNonExSliceHeader ) );
    }
    RNOK( xSlidingWindow          ( uiFrameNum ) );
    RNOK( xInsertNonExistingFrame ( pcNonExSliceHeader, uiFrameNum   ) );
    RNOK( xBumpingOutput          ( rcOutputList,       rcUnusedList ) );
    RNOK( xCheckBufferStatus      () );
  }
  m_pcLastDPBUnit           = 0;
  m_pcLastDPBUnitRefBasePic = 0;
  ROF( ( ( m_uiLastRefFrameNum + 1 ) % m_uiMaxFrameNum ) == uiCurrFrameNum );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xUpdateAndStoreCurrentPic( PocCalculator& rcPocCalculator, PicBufferList& rcOutputList, PicBufferList& rcUnusedList )
{
  ROF( m_bInitDone && m_bInitBufferDone );
  ROF( m_pcCurrDPBUnit->isCompleted() );

  SliceHeader* pcSliceHeader = m_pcCurrDPBUnit->getSliceHeader();
  ROF( pcSliceHeader );
  ROF( pcSliceHeader->isRefPic      () == m_pcCurrDPBUnit->isRefPicUnit       () );
  ROF( pcSliceHeader->getOutputFlag () == m_pcCurrDPBUnit->isWaitingForOutput () );
  ROF( pcSliceHeader->getFrameNum   () == m_pcCurrDPBUnit->getFrameNum        () );

  UInt  uiCurrFrameNum    = pcSliceHeader->getFrameNum  ();
  Bool  bIsIDR            = pcSliceHeader->getIdrFlag   ();
  Bool  bIsRefPic         = pcSliceHeader->isRefPic     ();
  Bool  bIsOutput         = pcSliceHeader->getOutputFlag();
  Bool  bIs2ndFld         = xIs2ndFieldOfCompFieldPair  ();
  Bool  bIs2ndFldBase     = xIs2ndFieldOfCompFieldPair  ( true );
  Bool  bStoredAsLongTerm = false;

  //===== possible output of single non-reference field that might occupy an extra DPB unit (for output of complementary field pairs) =====
  if( ! bIs2ndFld )
  {
    RNOK  ( xBumpingOutput    ( rcOutputList, rcUnusedList ) );
    RNOK  ( xCheckBufferStatus() );
  }

  //===== non-reference/non-output picture =====
  if( ! bIsRefPic && ! bIsOutput )
  {
    RNOK  ( m_pcCurrDPBUnit->uninit () );
    RNOK  ( xCheckBufferStatus      () );
    RNOK  ( xDumpDPB                ( "after update (non-output and non-ref pic)" ) );
    m_pcLastDPBUnit           = 0;
    m_pcLastDPBUnitRefBasePic = 0;
    return Err::m_nOK;
  }

  //===== 2nd field of non-reference/output complementary field pair =====
  if( ! bIsRefPic && bIs2ndFld )
  {
    ROT   ( m_pcLastDPBUnitRefBasePic );
    RNOK  ( m_pcCurrDPBUnit->store  ( *m_pcLastDPBUnit ) );
    RNOK  ( m_pcCurrDPBUnit->uninit () );
    RNOK  ( xBumpingOutput          ( rcOutputList, rcUnusedList ) );
    RNOK  ( xCheckBufferStatus      () );
    RNOK  ( xDumpDPB                ( "after update (non-ref 2nd field)" ) );
    return Err::m_nOK;
  }

  //===== non-reference/output frame/field (not 2nd field of a complementary field pair) =====
  if( ! bIsRefPic )
  {
    RNOK  ( xInsertCurrentInNewBuffer     ( m_pcLastDPBUnit ) );
    RNOK  ( m_pcCurrDPBUnit->uninit       () );
    if( ! pcSliceHeader->getFieldPicFlag  () )
    {
      RNOK( xBumpingOutput                ( rcOutputList, rcUnusedList ) );
      RNOK( xCheckBufferStatus            () );
      RNOK( xDumpDPB                      ( "after update (non-ref frame)" ) );
    }
    else
    {
      RNOK( xDumpDPB                      ( "after update (non-ref 1st field)" ) );
    }
    m_pcLastDPBUnitRefBasePic = 0;
    return  Err::m_nOK;
  }

  //===== IDR picture =====
  if( bIsIDR )
  {
    ROF   ( m_cUsedDPBUnitList.empty() ); // has been cleaned in initCurrDPBUnit()
    ROT   ( m_pcLastDPBUnit );
    ROT   ( m_pcLastDPBUnitRefBasePic );
    if( pcSliceHeader->getLongTermReferenceFlag() )
    {
      m_iMaxLongTermFrameIdx = 0;
      RNOK( m_pcCurrDPBUnit->markLongTerm ( pcSliceHeader->getPicType(), m_iMaxLongTermFrameIdx ) );
    }
    RNOK  ( xInsertCurrentInNewBuffer     ( m_pcLastDPBUnit ) );
    if( pcSliceHeader->getStoreRefBasePicFlag() )
    {
      RNOK( xInsertCurrentInNewBuffer     ( m_pcLastDPBUnitRefBasePic, true ) );
    }
    RNOK  ( m_pcCurrDPBUnit->uninit       () );
    RNOK  ( xCheckBufferStatus            () );
    RNOK  ( xDumpDPB                      ( "after update (IDR pic)" ) );
    return  Err::m_nOK;
  }

  //===== non-IDR reference pictures =====
  //----- MMCO for reference base pictures -----
  if( pcSliceHeader->getDecRefBasePicMarking().getAdaptiveRefPicMarkingModeFlag() )
  {
    RNOK( xMMCO( rcPocCalculator, *pcSliceHeader, bStoredAsLongTerm, true ) );
    ROT ( bStoredAsLongTerm );
  }
  //----- memory management for decoded picture -----
  if( pcSliceHeader->getDecRefPicMarking().getAdaptiveRefPicMarkingModeFlag() )
  {
    RNOK( xMMCO( rcPocCalculator, *pcSliceHeader, bStoredAsLongTerm ) );
  }
  else if( ! bIs2ndFld )
  {
    RNOK( xSlidingWindow( uiCurrFrameNum ) );
  }
  //----- store pictures -----
  if( ! bStoredAsLongTerm )
  {
    if( bIs2ndFld )
    {
      RNOK( m_pcCurrDPBUnit->store    ( *m_pcLastDPBUnit ) );
    }
    else
    {
      RNOK( xInsertCurrentInNewBuffer (  m_pcLastDPBUnit ) );
    }
    if( pcSliceHeader->getStoreRefBasePicFlag() )
    {
      if( bIs2ndFldBase )
      {
        RNOK( m_pcCurrDPBUnit->store    ( *m_pcLastDPBUnitRefBasePic, true ) );
      }
      else
      {
        RNOK( xSlidingWindow            (  uiCurrFrameNum ) );
        RNOK( xInsertCurrentInNewBuffer (  m_pcLastDPBUnitRefBasePic, true ) );
      }
    }
    else
    {
      m_pcLastDPBUnitRefBasePic = 0;
    }
  }
  //----- uninit, output, check ----
  RNOK( m_pcCurrDPBUnit->uninit () );
  RNOK( xBumpingOutput          ( rcOutputList, rcUnusedList ) );
  RNOK( xCheckBufferStatus      () );
  RNOK( xDumpDPB                ( "after update (non-IDR ref pic)" ) );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xCheckBufferStatus()
{
  ROF( m_bInitDone && m_bInitBufferDone );
  UInt                  uiNumNonRequired    = 0;
  UInt                  uiNumRequiredRef    = 0;
  UInt                  uiNumRequiredNonRef = 0;
  DPBUnitList::iterator iter                = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end                 = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    RNOK( (*iter)->checkStatus( m_iMaxLongTermFrameIdx ) );
    if( ! (*iter)->isRequired () )
    {
      uiNumNonRequired++;
    }
    else if( (*iter)->isUsedForRef() )
    {
      uiNumRequiredRef++;
    }
    else
    {
      uiNumRequiredNonRef++;
    }
  }
  ROT( uiNumNonRequired );
  ROF( uiNumRequiredRef                       <= m_uiMaxNumRefFrames );
  ROF( uiNumRequiredRef + uiNumRequiredNonRef <= m_uiDPBSizeInFrames );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xBumpingOutput( PicBufferList& rcOutputList, PicBufferList& rcUnusedList, Bool bOutputAll )
{
  ROF( m_bInitDone && m_bInitBufferDone );
  //===== determine number of required and non-required picture and determine DPB unit with minimum POC =====
  DPBUnitList           cSortedByPocList;
  DPBUnitList           cNonRequiredList;
  UInt                  uiNumRequiredRef    = 0;
  UInt                  uiNumRequiredNonRef = 0;
  DPBUnitList::iterator iter                = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end                 = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( ! (*iter)->isRequired() )
    {
      cNonRequiredList.push_back( *iter );
    }
    else
    {
      if( (*iter)->isUsedForRef() )
      {
        uiNumRequiredRef++;
      }
      else
      {
        uiNumRequiredNonRef++;
      }
      if( (*iter)->isWaitingForOutput() )
      {
        //----- insert in POC sorted list -----
        DPBUnitList::iterator iterPocList = cSortedByPocList.begin();
        DPBUnitList::iterator endPocList  = cSortedByPocList.end  ();
        for( ; iterPocList != endPocList && (*iterPocList)->getPoc() < (*iter)->getPoc(); iterPocList++ );
        cSortedByPocList.insert( iterPocList, *iter );
      }
    }
  }
  ROT( uiNumRequiredRef  > m_uiDPBSizeInFrames );
  ROT( uiNumRequiredRef && bOutputAll );
  //===== output non-reference unit with minimum POC when required =====
  while( uiNumRequiredRef + uiNumRequiredNonRef > m_uiDPBSizeInFrames || ( bOutputAll && uiNumRequiredNonRef ) )
  {
    DPBUnit* pcDPBUnitToOutput = cSortedByPocList.popFront();
    ROF ( pcDPBUnitToOutput );
    RNOK( pcDPBUnitToOutput->output( m_cPicBufferList, rcOutputList, rcUnusedList ) );
    if( ! pcDPBUnitToOutput->isUsedForRef() )
    {
      cNonRequiredList.push_back( pcDPBUnitToOutput );
      uiNumRequiredNonRef--;
    }
  }
  //===== clean-up used DPB unit list =====
  while( cNonRequiredList.size() )
  {
    DPBUnit* pcDPBUnit = cNonRequiredList.popFront();
    ROF ( pcDPBUnit );
    RNOK( pcDPBUnit->uninit() );
    m_cUsedDPBUnitList.remove   ( pcDPBUnit );
    m_cFreeDPBUnitList.push_back( pcDPBUnit );
  }
  ROF( ! bOutputAll || m_cUsedDPBUnitList.empty() );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xMarkAllUnusedForRef( Bool bRemoveOutputFlag, PicBufferList* pcUnusedList )
{
  ROT( bRemoveOutputFlag && !pcUnusedList );
  m_iMaxLongTermFrameIdx      = -1;
  DPBUnitList::iterator iter  = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end   = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    RNOK( (*iter)->markUnusedForRef( FRAME, bRemoveOutputFlag ) );
    if( bRemoveOutputFlag )
    {
      PicBuffer* pcPicBuffer = m_cPicBufferList.popFront();
      ROF( pcPicBuffer );
      ROF( pcPicBuffer->getBuffer() );
      if( pcPicBuffer->isUsed() )
      {
        pcPicBuffer->setUnused();
      }
      pcUnusedList->push_back( pcPicBuffer );
    }
  }
  m_pcLastDPBUnit           = 0;
  m_pcLastDPBUnitRefBasePic = 0;
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xSlidingWindow( UInt uiCurrFrameNum )
{
  ROF( m_bInitDone && m_bInitBufferDone );
  //===== determine number of reference frames and DPB units with smallest FrameNumWrap value =====
  UInt                  uiNumShortTerm    = 0;
  UInt                  uiNumLongTerm     = 0;
  Int                   iMinFrameNumWrap  = MSYS_INT_MAX;
  DPBUnit*              pcMinFrameNumUnit = 0;
  DPBUnitList::iterator iter              = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end               = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isLongTermUnit() )
    {
      uiNumLongTerm++;
    }
    else if( (*iter)->isShortTermUnit() )
    {
      uiNumShortTerm++;
      if( ( (*iter)->getFrameNumWrap( uiCurrFrameNum, m_uiMaxFrameNum )  < iMinFrameNumWrap ) ||
          ( (*iter)->getFrameNumWrap( uiCurrFrameNum, m_uiMaxFrameNum ) == iMinFrameNumWrap && (*iter)->isRefBasePicUnit() ) )
      {
        pcMinFrameNumUnit = *iter;
        iMinFrameNumWrap  = pcMinFrameNumUnit->getFrameNumWrap( uiCurrFrameNum, m_uiMaxFrameNum );
      }
    }
  }
  //===== check number of reference frames =====
  ROTRS ( uiNumShortTerm + uiNumLongTerm < m_uiMaxNumRefFrames,  Err::m_nOK );
  ROT   ( uiNumShortTerm + uiNumLongTerm > m_uiMaxNumRefFrames );
  ROF   ( pcMinFrameNumUnit ); // no short-term reference picture available
  //===== mark DPB unit with smallest FrameNumWrap value as "unused for reference" =====
  RNOK  ( pcMinFrameNumUnit->markUnusedForRef( FRAME ) );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xMMCO( PocCalculator& rcPocCalculator, SliceHeader& rcSliceHeader, Bool& rbMMCO6, Bool bRefBasePic )
{
  Mmco                    eMmcoOp;
  UInt                    uiVal1, uiVal2;
  UInt                    uiCurrFrameNum  = rcSliceHeader.getFrameNum ();
  PicType                 eCurrPicType    = rcSliceHeader.getPicType  ();
  const DecRefPicMarking& rcMmcoBuffer    = ( bRefBasePic ? rcSliceHeader.getDecRefBasePicMarking() : rcSliceHeader.getDecRefPicMarking() );
  Int                     iIndex          = 0;
  Int                     iMMCO6LTFrmIdx  = -1;
  Bool                    bMMCO123        = false;
  Bool                    bMMCO4          = false;
  Bool                    bMMCO5          = false;
  rbMMCO6                                 = false;

  while( MMCO_END != ( eMmcoOp = rcMmcoBuffer.get( iIndex++ ).getCommand( uiVal1, uiVal2 ) ) )
  {
    switch( eMmcoOp )
    {
    case MMCO_SHORT_TERM_UNUSED:
      ROT ( bMMCO5 );
      RNOK( xMarkShortTermUnused( uiCurrFrameNum, eCurrPicType, uiVal1, bRefBasePic ) );
      bMMCO123  = true;
      break;
    case MMCO_LONG_TERM_UNUSED:
      ROT ( bMMCO5 );
      ROT ( rbMMCO6 && iMMCO6LTFrmIdx == (Int)( eCurrPicType == FRAME ? uiVal1 : ( uiVal1 >> 1 ) ) );
      RNOK( xMarkLongTermUnused( eCurrPicType, uiVal1, bRefBasePic ) );
      bMMCO123  = true;
      break;
    case MMCO_ASSIGN_LONG_TERM:
      ROT ( bRefBasePic );
      ROT ( bMMCO5 );
      ROT ( rbMMCO6 && iMMCO6LTFrmIdx == (Int)uiVal2 );
      ROF ( (Int)uiVal2 <= m_iMaxLongTermFrameIdx );
      RNOK( xAssignLongTermIndex( uiCurrFrameNum, eCurrPicType, uiVal1, uiVal2 ) );
      bMMCO123  = true;
      break;
    case MMCO_MAX_LONG_TERM_IDX:
      ROT ( bRefBasePic );
      ROT ( bMMCO4 );
      ROT ( rbMMCO6 && iMMCO6LTFrmIdx >= (Int)uiVal1 )
      RNOK( xSetMaxLongTermIndex( uiVal1 ) );
      bMMCO4  = true;
      break;
    case MMCO_RESET:
      ROT ( bRefBasePic );
      ROT ( bMMCO123 );
      ROT ( bMMCO5 );
      ROT ( rbMMCO6 );
      RNOK( xReset( rcPocCalculator, rcSliceHeader ) );
      bMMCO5  = true;
      break;
    case MMCO_SET_LONG_TERM:
      ROT ( bRefBasePic );
      ROT ( rbMMCO6 );
      ROF ( (Int)uiVal1 <= m_iMaxLongTermFrameIdx );
      RNOK( xStoreCurrentLongTerm( uiVal1, rcSliceHeader.getStoreRefBasePicFlag() ) );
      rbMMCO6         = true;
      iMMCO6LTFrmIdx  = uiVal1;
      break;
    default:
      ROT ( true );
    }
  }
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xMarkShortTermUnused( UInt uiCurrFrameNum, PicType eCurrPicType, UInt uiPicNumDiff, Bool bRefBasePic )
{
  Int     iCurrPicNum     = ( eCurrPicType == FRAME ? (Int)uiCurrFrameNum : 2 * (Int)uiCurrFrameNum + 1 );
  Int     iPicNumX        = ( iCurrPicNum - (Int)uiPicNumDiff - 1 );
  PicType ePicTypeX       = ( eCurrPicType == FRAME ? FRAME    : ( iPicNumX  % 2 ? eCurrPicType : PicType( FRAME - eCurrPicType ) ) );
  UInt    iFrameNumWrapX  = ( eCurrPicType == FRAME ? iPicNumX : ( iPicNumX >> 1 ) );
  //===== find short-term picture and mark "unused for reference" =====
  DPBUnit*              pcDPBUnit = 0;
  DPBUnitList::iterator iter      = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end       = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isShortTermRef   ( ePicTypeX )                                         &&
        (*iter)->isRefBasePicUnit ()                                  == bRefBasePic    &&
        (*iter)->getFrameNumWrap  ( uiCurrFrameNum, m_uiMaxFrameNum ) == iFrameNumWrapX   )
    {
      ROT( pcDPBUnit );
      pcDPBUnit = *iter;
    }
  }
  ROF ( pcDPBUnit ); // not found
  RNOK( pcDPBUnit->markUnusedForRef( ePicTypeX ) );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xMarkLongTermUnused( PicType eCurrPicType, UInt uiLongTermPicNum, Bool bRefBasePic )
{
  PicType ePicTypeX         = ( eCurrPicType == FRAME ? FRAME                 : ( (Int)uiLongTermPicNum % 2 ? eCurrPicType : PicType( FRAME - eCurrPicType ) ) );
  Int     iLongTermFrameIdx = ( eCurrPicType == FRAME ? (Int)uiLongTermPicNum : ( (Int)uiLongTermPicNum >> 1 ) );
  //===== find long-term picture and mark "unused for reference" =====
  DPBUnit*              pcDPBUnit = 0;
  DPBUnitList::iterator iter      = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end       = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isLongTermRef    ( ePicTypeX )                       &&
        (*iter)->isRefBasePicUnit ()            == bRefBasePic        &&
        (*iter)->getLongTermIndex ()            == iLongTermFrameIdx    )
    {
      ROT( pcDPBUnit );
      pcDPBUnit = *iter;
    }
  }
  ROF ( pcDPBUnit ); // not found
  RNOK( pcDPBUnit->markUnusedForRef( ePicTypeX ) );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xAssignLongTermIndex( UInt uiCurrFrameNum, PicType eCurrPicType, UInt uiPicNumDiff, UInt uiLongTermFrameIndex )
{
  ROF( (Int)uiLongTermFrameIndex <= m_iMaxLongTermFrameIdx );
  Int     iCurrPicNum       = ( eCurrPicType == FRAME ? (Int)uiCurrFrameNum : 2 * (Int)uiCurrFrameNum + 1 );
  Int     iPicNumX          = ( iCurrPicNum - (Int)uiPicNumDiff - 1 );
  PicType ePicTypeX         = ( eCurrPicType == FRAME ? FRAME    : ( iPicNumX  % 2 ? eCurrPicType : PicType( FRAME - eCurrPicType ) ) );
  UInt    iFrameNumWrapX    = ( eCurrPicType == FRAME ? iPicNumX : ( iPicNumX >> 1 ) );
  Int     iLongTermFrameIdx = (Int)uiLongTermFrameIndex;
  //===== find short-term pictures =====
  DPBUnit*              pcDPBUnit     = 0;
  DPBUnit*              pcDPBUnitBase = 0;
  DPBUnitList::iterator iter          = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end           = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isShortTermRef   ( ePicTypeX )                                         &&
        (*iter)->getFrameNumWrap  ( uiCurrFrameNum, m_uiMaxFrameNum ) == iFrameNumWrapX   )
    {
      if( (*iter)->isRefBasePicUnit() )
      {
        ROT( pcDPBUnitBase );
        pcDPBUnitBase = *iter;
      }
      else
      {
        ROT( pcDPBUnit );
        pcDPBUnit = *iter;
      }
    }
  }
  //===== mark long-term pictures with specified index as "unused for reference" =====
  iter  = m_cUsedDPBUnitList.begin();
  end   = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isLongTermUnit   ()                      &&
        (*iter)->getLongTermIndex () == iLongTermFrameIdx &&
        (*iter)                      != pcDPBUnit           )
    {
      RNOK( (*iter)->markUnusedForRef( FRAME ) );
    }
  }
  //===== mark pictures =====
  ROF   ( pcDPBUnit );
  RNOK  ( pcDPBUnit     ->markLongTerm( ePicTypeX, iLongTermFrameIdx ) );
  ROFRS ( pcDPBUnitBase,  Err::m_nOK );
  RNOK  ( pcDPBUnitBase ->markLongTerm( ePicTypeX, iLongTermFrameIdx ) );
  return  Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xSetMaxLongTermIndex( UInt uiMaxLongTermFrameIdxPlus1 )
{
  m_iMaxLongTermFrameIdx = (Int)uiMaxLongTermFrameIdxPlus1 - 1;
  //===== mark long-term pictures with higher index as "unused for reference" =====
  DPBUnitList::iterator iter  = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end   = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isLongTermUnit   ()                          &&
        (*iter)->getLongTermIndex () > m_iMaxLongTermFrameIdx   )
    {
      RNOK( (*iter)->markUnusedForRef( FRAME ) );
    }
  }
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xReset( PocCalculator& rcPocCalculator, SliceHeader& rcSliceHeader )
{
  //===== check maximum POC of all required pictures =====
  Int                   iMaxPoc = MSYS_INT_MIN;
  DPBUnitList::iterator iter    = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end     = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isRequired() && (*iter)->isExisting() )
    {
      Int iPoc  = (*iter)->getMaxPoc( rcSliceHeader.getSPS().getPicOrderCntType() == 0 );
      iMaxPoc   = gMax( iMaxPoc, iPoc );
    }
  }
  ROF( m_pcCurrDPBUnit->getPoc() > iMaxPoc );
  //===== modify POC values of pictures in DPB =====
  iter  = m_cUsedDPBUnitList.begin();
  end   = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    RNOK( (*iter)->decreasePoc( m_pcCurrDPBUnit->getPoc() ) );
  }
  //===== mark all pictures as "unused for reference" =====
  RNOK( xMarkAllUnusedForRef() );
  //===== reset PocCalculator and POC values in slice Header =====
  RNOK( rcPocCalculator .resetMMCO5 ( rcSliceHeader ) );
  //===== update parameters in current DPB unit =====
  ROF ( m_pcCurrDPBUnit->isCompleted() );
  RNOK( m_pcCurrDPBUnit->resetMMCO5 ( rcSliceHeader ) );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xStoreCurrentLongTerm( UInt uiLongTermFrameIndex, Bool bStoreRefBasePic )
{
  Int  iLongTermFrameIdx  = (Int)uiLongTermFrameIndex;
  ROF( iLongTermFrameIdx <= m_iMaxLongTermFrameIdx );
  //===== mark long-term pictures with specified index as "unused for reference" =====
  DPBUnitList::iterator iter  = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end   = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( (*iter)->isLongTermUnit   ()                      &&
        (*iter)->getLongTermIndex () == iLongTermFrameIdx   )
    {
      if( xIs2ndFieldOfCompFieldPair( bStoreRefBasePic ) )
      {
        ROF( *iter == ( bStoreRefBasePic ? m_pcLastDPBUnitRefBasePic : m_pcLastDPBUnit ) );
      }
      else
      {
        RNOK( (*iter)->markUnusedForRef( FRAME ) );
      }
    }
  }
  //===== mark current picture as long-term =====
  RNOK( m_pcCurrDPBUnit->markLongTerm( m_pcCurrDPBUnit->getPicStatus(), uiLongTermFrameIndex ) );
  //===== store current picture =====
  if( xIs2ndFieldOfCompFieldPair() )
  {
    RNOK( m_pcCurrDPBUnit->store    ( *m_pcLastDPBUnit ) );
  }
  else
  {
    RNOK( xInsertCurrentInNewBuffer (  m_pcLastDPBUnit ) );
  }
  //===== store reference base picture =====
  if( bStoreRefBasePic )
  {
    if( xIs2ndFieldOfCompFieldPair( true ) )
    {
      RNOK( m_pcCurrDPBUnit->store    ( *m_pcLastDPBUnitRefBasePic, true ) );
    }
    else
    {
      RNOK( xInsertCurrentInNewBuffer (  m_pcLastDPBUnitRefBasePic, true ) );
    }
  }
  else
  {
    m_pcLastDPBUnitRefBasePic = 0;
  }
  return Err::m_nOK;
}

Bool
DecodedPicBuffer::xExistsRefBasePicShortTerm( UInt uiFrameNum )
{
  DPBUnitList::iterator iter = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end  = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    ROTRS( (*iter)->isShortTermUnit() && (*iter)->isRefBasePicUnit() && (*iter)->getFrameNum() == uiFrameNum, true );
  }
  return false;
}

Bool
DecodedPicBuffer::xExistsRefBasePicLongTerm ( Int iLongTermFrameIdx )
{
  DPBUnitList::iterator iter = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end  = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    ROTRS( (*iter)->isLongTermUnit() && (*iter)->isRefBasePicUnit() && (*iter)->getLongTermIndex() == iLongTermFrameIdx, true );
  }
  return false;
}

Bool
DecodedPicBuffer::xIsAvailableForRefLists( const DPBUnit* pcDPBUnit, Bool bFieldPicture, Bool bExcludeNonExisting, Bool bUseRefBasePic )
{
  AOF  ( pcDPBUnit );
  ROFRS( bFieldPicture       ||  pcDPBUnit->isRefFrame(), false );
  ROTRS( bExcludeNonExisting && !pcDPBUnit->isExisting(), false );
  if( !bUseRefBasePic )
  {
    ROTRS( pcDPBUnit->isUsedForRef () && ! pcDPBUnit->isRefBasePicUnit(), true );
    return false;
  }
  ROTRS( pcDPBUnit->isShortTermUnit() && ( pcDPBUnit->isRefBasePicUnit() || !xExistsRefBasePicShortTerm( pcDPBUnit->getFrameNum     () ) ), true );
  ROTRS( pcDPBUnit->isLongTermUnit () && ( pcDPBUnit->isRefBasePicUnit() || !xExistsRefBasePicLongTerm ( pcDPBUnit->getLongTermIndex() ) ), true );
  return false;
}

ErrVal
DecodedPicBuffer::xCreateOrderedDPBUnitLists( DPBUnitList& rcOrderedShortTermList0,
                                              DPBUnitList& rcOrderedShortTermList1,
                                              DPBUnitList& rcOrderedLongTermList,
                                              Bool&        rbShortTermListIdentical,
                                              SliceHeader& rcSliceHeader )
{
  rcOrderedShortTermList0 .clear();
  rcOrderedShortTermList1 .clear();
  rcOrderedLongTermList   .clear();
  rbShortTermListIdentical                  = false;
  Int                   iCurrPoc            = rcSliceHeader.getPoc              ();
  UInt                  uiCurrFrameNum      = rcSliceHeader.getFrameNum         ();
  Bool                  bBSlice             = rcSliceHeader.isBSlice            ();
  Bool                  bFieldPicture       = rcSliceHeader.getFieldPicFlag     ();
  Bool                  bUseRefBasePicFlag  = rcSliceHeader.getUseRefBasePicFlag();
  Bool                  bExcludeNonExisting = ( bBSlice && rcSliceHeader.getSPS ().getPicOrderCntType() == 0 );
  DPBUnitList::iterator iter                = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end                 = m_cUsedDPBUnitList.end  ();
  for( ; iter != end; iter++ )
  {
    if( xIsAvailableForRefLists( *iter, bFieldPicture, bExcludeNonExisting, bUseRefBasePicFlag ) )
    {
      if( (*iter)->isLongTermUnit() )
      {
        DPBUnitList::iterator iterLTI = rcOrderedLongTermList.begin();
        DPBUnitList::iterator endLTI  = rcOrderedLongTermList.end  ();
        for( ; iterLTI != endLTI && (*iter)->getLongTermIndex() > (*iterLTI)->getLongTermIndex(); iterLTI++ );
        rcOrderedLongTermList.insert( iterLTI, *iter );
      }
      else if( !bBSlice )
      {
        DPBUnitList::iterator iterFNW = rcOrderedShortTermList0.begin();
        DPBUnitList::iterator endFNW  = rcOrderedShortTermList0.end  ();
        for( ; iterFNW != endFNW && (*iter)->getFrameNumWrap( uiCurrFrameNum, m_uiMaxFrameNum ) < (*iterFNW)->getFrameNumWrap( uiCurrFrameNum, m_uiMaxFrameNum ); iterFNW++ );
        rcOrderedShortTermList0.insert( iterFNW, *iter );
      }
      else
      {
        DPBUnitList::iterator iterPOC = rcOrderedShortTermList0.begin();
        DPBUnitList::iterator endPOC  = rcOrderedShortTermList0.end  ();
        for( ; iterPOC != endPOC && (*iter)->getPoc() < (*iterPOC)->getPoc(); iterPOC++ );
        rcOrderedShortTermList0.insert( iterPOC, *iter );
      }
    }
  }
  ROT( rcOrderedShortTermList0.empty() && rcOrderedLongTermList.empty() );
  if( bBSlice )
  {
    DPBUnitList cDPBUnitsWithGreaterPocList;
    while( !rcOrderedShortTermList0.empty() && rcOrderedShortTermList0.front()->getPoc() >= iCurrPoc )
    {
      cDPBUnitsWithGreaterPocList.push_front( rcOrderedShortTermList0.popFront() );
    }
    rbShortTermListIdentical = ( rcOrderedShortTermList0.empty() || cDPBUnitsWithGreaterPocList.empty() );
    rcOrderedShortTermList1  = cDPBUnitsWithGreaterPocList;
    rcOrderedShortTermList1 += rcOrderedShortTermList0;
    rcOrderedShortTermList0 += cDPBUnitsWithGreaterPocList;
  }
  return Err::m_nOK;
}

DPBUnit*
DecodedPicBuffer::xGetDPBEntry( DPBUnitList& rcDPBUnitList, UInt uiIndex )
{
  ROFRS( uiIndex < rcDPBUnitList.size(), 0 );
  DPBUnitList::iterator iter = rcDPBUnitList.begin();
  for( ; uiIndex; uiIndex--, iter++ );
  return *iter;
}

ErrVal
DecodedPicBuffer::xGetRefPicEntryList( RefPicEntryList& rcRefPicEntryList, DPBUnitList& rcDPBUnitList, PicType eCurrPicType )
{
  ROF( eCurrPicType );
  rcRefPicEntryList.clear();
  //===== get frame list =====
  if( eCurrPicType == FRAME )
  {
    while( rcDPBUnitList.size() )
    {
      rcRefPicEntryList.push_back( RefPicEntry( FRAME, rcDPBUnitList.popFront() ) );
    }
    return Err::m_nOK;
  }
  //===== get field list =====
  PicType eCurrentParity      = eCurrPicType;
  PicType eOppositeParity     = PicType( FRAME - eCurrentParity );
  UInt    uiCurrentParityIdx  = 0;
  UInt    uiOppositeParityIdx = 0;
  UInt    uiFrameListSize     = (UInt)rcDPBUnitList.size();
  while( uiCurrentParityIdx < uiFrameListSize || uiOppositeParityIdx < uiFrameListSize )
  {
    // current parity
    while( uiCurrentParityIdx < uiFrameListSize )
    {
      DPBUnit* pcDPBUnit = xGetDPBEntry( rcDPBUnitList, uiCurrentParityIdx++ );
      if( pcDPBUnit->isRefPic( eCurrentParity ) )
      {
        rcRefPicEntryList.push_back( RefPicEntry( eCurrentParity, pcDPBUnit ) );
        break;
      }
    }
    // opposite parity
    while( uiOppositeParityIdx < uiFrameListSize )
    {
      DPBUnit* pcDPBUnit = xGetDPBEntry( rcDPBUnitList, uiOppositeParityIdx++ );
      if( pcDPBUnit->isRefPic( eOppositeParity ) )
      {
        rcRefPicEntryList.push_back( RefPicEntry( eOppositeParity, pcDPBUnit ) );
        break;
      }
    }
  }
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xCreateInitialRefPicLists( RefPicEntryList& rcRefPicEntryList0,
                                             RefPicEntryList& rcRefPicEntryList1,
                                             SliceHeader&     rcSliceHeader )
{
  ROT( rcSliceHeader.isIntraSlice() );
  //===== get DPBUnit lists =====
  DPBUnitList     cShortTermDPBUnitList0;
  DPBUnitList     cShortTermDPBUnitList1;
  DPBUnitList     cLongTermDPBUnitList;
  RefPicEntryList cLongTermList;
  PicType         ePicType                  = rcSliceHeader.getPicType();
  Bool            bBSlice                   = rcSliceHeader.isBSlice  ();
  Bool            bShortTermListsIdentical  = false;
  RNOK ( xCreateOrderedDPBUnitLists( cShortTermDPBUnitList0, cShortTermDPBUnitList1, cLongTermDPBUnitList, bShortTermListsIdentical, rcSliceHeader ) );
  //===== create reference picture entry list 0 =====
  RNOK ( xGetRefPicEntryList( rcRefPicEntryList0, cShortTermDPBUnitList0, ePicType ) );
  RNOK ( xGetRefPicEntryList( cLongTermList,      cLongTermDPBUnitList,   ePicType ) );
  rcRefPicEntryList0 += cLongTermList;
  ROT  ( rcRefPicEntryList0.empty() );
  ROFRS( bBSlice, Err::m_nOK );
  //===== create reference picture entry list 1 =====
  RNOK ( xGetRefPicEntryList( rcRefPicEntryList1, cShortTermDPBUnitList1, ePicType ) );
  rcRefPicEntryList1 += cLongTermList;
  ROT  ( rcRefPicEntryList1.empty() );
  ROFRS( rcRefPicEntryList1.size () > 1 && bShortTermListsIdentical, Err::m_nOK );
  //===== switch first entries of reference picture entry list 1
  RefPicEntry c2ndEntry = rcRefPicEntryList1.popFront();
  RefPicEntry c1stEntry = rcRefPicEntryList1.popFront();
  rcRefPicEntryList1.push_front( c2ndEntry );
  rcRefPicEntryList1.push_front( c1stEntry );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xModifyRefPicList( RefPicEntryList& rcRefPicEntryList, UInt& ruiRefIdx, Bool bLongTerm, PicType ePicType, UInt uiFrameNumOrLTIdx )
{
  ROF( ruiRefIdx <= rcRefPicEntryList.size() );
  //===== find specified picture in list =====
  RefPicEntryList::iterator iter      = rcRefPicEntryList.begin();
  RefPicEntryList::iterator end       = rcRefPicEntryList.end  ();
  UInt                      uiLastIdx = 0;
  for( ; iter != end; iter++, uiLastIdx++ )
  {
    if( (*iter).getPicType() == ePicType )
    {
      UInt uiIdentifier  = ( bLongTerm ? (UInt)(*iter).getDPBUnit()->getLongTermIndex() : (*iter).getDPBUnit()->getFrameNum() );
      if(  uiIdentifier == uiFrameNumOrLTIdx )
      {
        break;
      }
    }
  }
  ROT(   iter == end );                       // specified picture was not found
  ROF( (*iter).getDPBUnit()->isExisting() );  // specified picture is a non-existing picture
  //===== modify list =====
  if( uiLastIdx != ruiRefIdx )
  {
    RefPicEntry cRefPicEntry = *iter;
    //----- remove entry from old position -----
    if( uiLastIdx > ruiRefIdx )
    {
      rcRefPicEntryList.erase( iter );
    }
    //----- insert at new position -----
    iter = rcRefPicEntryList.begin();
    for( UInt uiIdx = 0; uiIdx < ruiRefIdx; uiIdx++, iter++ );
    rcRefPicEntryList.insert( iter, cRefPicEntry );
  }
  //===== update reference index position =====
  ruiRefIdx++;
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xRefPicListModification( RefPicEntryList& rcRefPicEntryList, SliceHeader& rcSliceHeader, ListIdx eListIdx )
{
  ROFRS( rcSliceHeader.getRefPicListReordering( eListIdx ).getRefPicListReorderingFlag(), Err::m_nOK );
  RefPicListReOrdering&   rcRefPicListReordering  = rcSliceHeader.getRefPicListReordering( eListIdx );
  PicType                 eCurrPicType            = rcSliceHeader.getPicType  ();
  UInt                    uiFrameNum              = rcSliceHeader.getFrameNum ();
  Int                     iMaxPicNum              = ( eCurrPicType == FRAME ? (Int)m_uiMaxFrameNum : 2*(Int)m_uiMaxFrameNum );
  Int                     iPicNumPred             = ( eCurrPicType == FRAME ? (Int)uiFrameNum      : 2*(Int)uiFrameNum + 1  );
  UInt                    uiRefIdx                = 0;
  ReOrderingOfPicNumsIdc  eCommand                = RPLR_END;
  UInt                    uiIdentifier            = 0;
  UInt                    uiCommandIndex          = 0;
  while( RPLR_END != ( eCommand = rcRefPicListReordering.get( uiCommandIndex++ ).getCommand( uiIdentifier ) ) )
  {
    if( eCommand == RPLR_NEG || eCommand == RPLR_POS )
    {
      Int     iPicNumDiff = (Int)uiIdentifier + 1;
      if( eCommand == RPLR_NEG )
      {
        iPicNumPred      -= iPicNumDiff - ( iPicNumPred <               iPicNumDiff ? iMaxPicNum : 0 );
      }
      else
      {
        iPicNumPred      += iPicNumDiff - ( iPicNumPred >= iMaxPicNum - iPicNumDiff ? iMaxPicNum : 0 );
      }
      PicType ePicTypeX   = ( eCurrPicType == FRAME ? FRAME             : ( iPicNumPred % 2 ? eCurrPicType : PicType( FRAME - eCurrPicType ) ) );
      UInt    uiFrameNumX = ( eCurrPicType == FRAME ? (UInt)iPicNumPred : (UInt)iPicNumPred >> 1 );
      RNOK( xModifyRefPicList( rcRefPicEntryList, uiRefIdx, false, ePicTypeX, uiFrameNumX ) );
    }
    else
    {
      ROF( eCommand == RPLR_LONG );
      PicType ePicTypeX   = ( eCurrPicType == FRAME ? FRAME             : ( uiIdentifier % 2 ? eCurrPicType : PicType( FRAME - eCurrPicType ) ) );
      UInt    uiLTIndexX  = ( eCurrPicType == FRAME ? uiIdentifier      : uiIdentifier >> 1 );
      RNOK( xModifyRefPicList( rcRefPicEntryList, uiRefIdx, true,  ePicTypeX, uiLTIndexX ) );
    }
  }
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xSetReferencePictureList( RefFrameList& rcRefFrameList, RefPicEntryList& rcRefPicEntryList, UInt uiNumActiveEntries )
{
  ROF( uiNumActiveEntries );
  ROF( uiNumActiveEntries <= rcRefPicEntryList.size() );
  rcRefFrameList.reset();
  RefPicEntryList::iterator iter = rcRefPicEntryList.begin();
  for( UInt uiRefIdx = 0; uiRefIdx < uiNumActiveEntries; uiRefIdx++, iter++ )
  {
    ROF( (*iter).getDPBUnit() );
    ROF( (*iter).getDPBUnit()->getFrame() );
    PicType   ePicType  = (*iter)   .getPicType     ();
    DPBUnit*  pcDPBUnit = (*iter)   .getDPBUnit     ();
    Frame*    pcFrame   = pcDPBUnit->getFrame       ();
    Frame*    pcPic     = pcFrame  ->getPic         ( ePicType );
    pcFrame->setLongTerm( pcDPBUnit->isLongTermUnit () );
    RNOK( rcRefFrameList.add( pcPic ) );
  }
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xSetMbDataCtrlEntry0( MbDataCtrl*& rpcMbDataCtrl, RefPicEntryList& rcRefPicEntryList )
{
  ROT( rcRefPicEntryList.empty() );
  DPBUnit*  pcDPBUnit = (*rcRefPicEntryList.begin()).getDPBUnit();
  ROF( pcDPBUnit );
  rpcMbDataCtrl       = pcDPBUnit->getMbDataCtrlBase();
  ROF( rpcMbDataCtrl );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xDumpDPB( const Char* pcString )
{
  ROFRS( m_bDebugOutput, Err::m_nOK );
  printf( "\nDPB [D=%d, S=%d, MR=%d, MLT=%d]", m_uiDependencyId, m_uiDPBSizeInFrames, m_uiMaxNumRefFrames, m_iMaxLongTermFrameIdx );
  if( pcString )  printf( " : %s\n", pcString );
  else            printf( "\n" );
  DPBUnitList::iterator iter  = m_cUsedDPBUnitList.begin();
  DPBUnitList::iterator end   = m_cUsedDPBUnitList.end  ();
  for( UInt uiIdx = 0; iter != end; iter++, uiIdx++ )
  {
    RNOK( (*iter)->dump( uiIdx, uiIdx == 0, true ) );
  }
  printf( "\n" );
  return Err::m_nOK;
}

ErrVal
DecodedPicBuffer::xDumpRefPicList( RefFrameList& rcRefFrameList, ListIdx eListIdx, const Char* pcString )
{
  ROFRS( m_bDebugOutput, Err::m_nOK );
  printf( "\nList %d [active = %d]", (Int)eListIdx, rcRefFrameList.getActive() );
  if( pcString )  printf( " : %s\n", pcString );
  else            printf( "\n" );
  const Char*     apcTypeString[3] = { "TOP-FLD", "BOT-FLD", "FRAME  " };
  printf( "------------------------------\n" );
  for( UInt uiRefIdx = 0; uiRefIdx < rcRefFrameList.getActive(); uiRefIdx++ )
  {
    Frame*      pcFrame   = rcRefFrameList[uiRefIdx+1];
    PicType     ePicType  = pcFrame->getPicType();
    Int         iPoc      = pcFrame->getPoc    ();
    printf("#%3u: %s  POC=%- 11d\n", uiRefIdx, apcTypeString[ePicType-1], iPoc );
  }
  printf( "------------------------------\n\n" );
  return Err::m_nOK;
}






LayerDecoder::LayerDecoder()
: m_pcH264AVCDecoder                    ( 0 )
, m_pcNalUnitParser                     ( 0 )
, m_pcSliceReader                       ( 0 )
, m_pcSliceDecoder                      ( 0 )
, m_pcControlMng                        ( 0 )
, m_pcLoopFilter                        ( 0 )
, m_pcHeaderSymbolReadIf                ( 0 )
, m_pcParameterSetMng                   ( 0 )
, m_pcPocCalculator                     ( 0 )
, m_pcYuvFullPelBufferCtrl              ( 0 )
, m_pcDecodedPictureBuffer              ( 0 )
, m_pcMotionCompensation                ( 0 )
, m_pcReconstructionBypass              ( 0 )
#ifdef SHARP_AVC_REWRITE_OUTPUT
, m_pcRewriteEncoder                    ( 0 )
#endif
, m_bInitialized                        ( false )
, m_bSPSInitialized                     ( false )
, m_bDependencyRepresentationInitialized( false )
, m_bLayerRepresentationInitialized     ( false )
, m_uiFrameWidthInMb                    ( 0 )
, m_uiFrameHeightInMb                   ( 0 )
, m_uiMbNumber                          ( 0 )
, m_uiDependencyId                      ( 0 )
, m_uiQualityId                         ( 0 )
, m_pacMbStatus                         ( 0 )
, m_pcCurrDPBUnit                       ( 0 )
, m_pcBaseLayerCtrl                     ( 0 )
, m_pcBaseLayerCtrlField                ( 0 )
, m_pcResidualLF                        ( 0 )
, m_pcResidualILPred                    ( 0 )
, m_pcILPrediction                      ( 0 )
, m_pcBaseLayerFrame                    ( 0 )
, m_pcBaseLayerResidual                 ( 0 )
{
  ::memset( m_apcFrameTemp, 0x00, sizeof( m_apcFrameTemp ) );
  m_apabBaseModeFlagAllowedArrays[0] = 0;
  m_apabBaseModeFlagAllowedArrays[1] = 0;
}


LayerDecoder::~LayerDecoder()
{
  while( m_cSliceHeaderList.size() )
  {
    SliceHeader*  pcSliceHeader = m_cSliceHeaderList.popFront();
    delete        pcSliceHeader;
  }
}


ErrVal
LayerDecoder::create( LayerDecoder*& rpcLayerDecoder )
{
  rpcLayerDecoder = new LayerDecoder;
  ROT( NULL == rpcLayerDecoder );
  return Err::m_nOK;
}


ErrVal
LayerDecoder::destroy()
{
  ROT( m_bInitialized );
  delete this;
  return Err::m_nOK;
}


ErrVal
LayerDecoder::init( UInt                   uiDependencyId,
                   H264AVCDecoder*        pcH264AVCDecoder,
                   NalUnitParser*         pcNalUnitParser,
                   SliceReader*           pcSliceReader,
                   SliceDecoder*          pcSliceDecoder,
                   ControlMngIf*          pcControlMng,
                   LoopFilter*            pcLoopFilter,
                   HeaderSymbolReadIf*    pcHeaderSymbolReadIf,
                   ParameterSetMng*       pcParameterSetMng,
                   PocCalculator*         pcPocCalculator,
                   YuvBufferCtrl*         pcYuvFullPelBufferCtrl,
                   DecodedPicBuffer*      pcDecodedPictureBuffer,
                   MotionCompensation*    pcMotionCompensation,
				           ReconstructionBypass*  pcReconstructionBypass
#ifdef SHARP_AVC_REWRITE_OUTPUT
                   ,RewriteEncoder*       pcRewriteEncoder
#endif
                   )
{
  ROT( m_bInitialized );
  ROF( pcH264AVCDecoder );
  ROF( pcNalUnitParser );
  ROF( pcSliceReader );
  ROF( pcSliceDecoder );
  ROF( pcControlMng );
  ROF( pcLoopFilter );
  ROF( pcHeaderSymbolReadIf );
  ROF( pcParameterSetMng );
  ROF( pcPocCalculator );
  ROF( pcYuvFullPelBufferCtrl );
  ROF( pcDecodedPictureBuffer );
  ROF( pcMotionCompensation );
  ROF( pcReconstructionBypass );
#ifdef SHARP_AVC_REWRITE_OUTPUT
  ROF( pcRewriteEncoder );
#endif

  m_pcH264AVCDecoder                      = pcH264AVCDecoder;
  m_pcNalUnitParser                       = pcNalUnitParser;
  m_pcSliceReader                         = pcSliceReader;
  m_pcSliceDecoder                        = pcSliceDecoder ;
  m_pcControlMng                          = pcControlMng;
  m_pcLoopFilter                          = pcLoopFilter;
  m_pcHeaderSymbolReadIf                  = pcHeaderSymbolReadIf;
  m_pcParameterSetMng                     = pcParameterSetMng;
  m_pcPocCalculator                       = pcPocCalculator;
  m_pcYuvFullPelBufferCtrl                = pcYuvFullPelBufferCtrl;
  m_pcDecodedPictureBuffer                = pcDecodedPictureBuffer;
  m_pcMotionCompensation                  = pcMotionCompensation;
  m_pcReconstructionBypass                = pcReconstructionBypass;
#ifdef SHARP_AVC_REWRITE_OUTPUT
  m_pcRewriteEncoder                      = pcRewriteEncoder;
#endif

  m_bInitialized                          = true;
  m_bSPSInitialized                       = false;
  m_bDependencyRepresentationInitialized  = false;
  m_bLayerRepresentationInitialized       = false;
  m_bInterLayerPredLayerRep               = false;
  m_bFirstILPredSliceInLayerRep           = false;
  m_bRewritingLayerRep                    = false;
  m_uiFrameWidthInMb                      = 0;
  m_uiFrameHeightInMb                     = 0;
  m_uiMbNumber                            = 0;
  m_uiDependencyId                        = uiDependencyId;
  m_uiQualityId                           = 0;

  m_pacMbStatus                           = 0;

  m_pcCurrDPBUnit                         = 0;
  m_pcBaseLayerCtrl                       = 0;
  m_pcBaseLayerCtrlField                  = 0;
  m_pcResidualLF                          = 0;
  m_pcResidualILPred                      = 0;
  m_pcILPrediction                        = 0;
  m_pcBaseLayerFrame                      = 0;
  m_pcBaseLayerResidual                   = 0;

  return Err::m_nOK;
}


ErrVal
LayerDecoder::uninit()
{
  ROF ( m_bInitialized );
  RNOK( xDeleteData() );
  m_bInitialized  = false;
  return Err::m_nOK;
}


ErrVal
LayerDecoder::processSliceData( PicBuffer*         pcPicBuffer,
                                PicBufferList&     rcPicBufferOutputList,
                                PicBufferList&     rcPicBufferUnusedList,
                                BinDataList&       rcBinDataList,
                                SliceDataNALUnit&  rcSliceDataNALUnit )
{
  ROF( m_bInitialized );
  ROF( pcPicBuffer );

  //===== process slice =====
  SliceHeader*  pcSliceHeader                     = 0;
  Bool          bFirstSliceInLayerRepresentation  = ! m_bLayerRepresentationInitialized;
  RNOK( xInitSlice  ( pcSliceHeader, pcPicBuffer, rcPicBufferOutputList, rcPicBufferUnusedList, rcSliceDataNALUnit ) );

  //===== parse, decode, and finish slice =====
  RNOK( xParseSlice ( *pcSliceHeader ) );
  RNOK( xDecodeSlice( *pcSliceHeader, rcSliceDataNALUnit, bFirstSliceInLayerRepresentation ) );
  RNOK( xFinishSlice( *pcSliceHeader, rcPicBufferOutputList, rcPicBufferUnusedList, rcSliceDataNALUnit, rcBinDataList ) );
  return Err::m_nOK;
}


ErrVal
LayerDecoder::finishProcess( PicBufferList&  rcPicBufferOutputList,
                             PicBufferList&  rcPicBufferUnusedList )
{
  ROF  ( m_bInitialized );
  RNOK ( m_pcDecodedPictureBuffer->finish( rcPicBufferOutputList, rcPicBufferUnusedList ) );
  while( m_cSliceHeaderList.size() )
  {
    SliceHeader*  pcSliceHeader = m_cSliceHeaderList.popFront();
    delete        pcSliceHeader;
  }
  return Err::m_nOK;
}

ErrVal
LayerDecoder::updateDPB( PicBufferList& rcPicBufferOutputList,
                         PicBufferList& rcPicBufferUnusedList )
{
  ROF ( m_bInitialized );
  RNOK( m_pcDecodedPictureBuffer->updateBuffer( *m_pcPocCalculator, rcPicBufferOutputList, rcPicBufferUnusedList ) );
  return Err::m_nOK;
}

ErrVal
LayerDecoder::xInitSlice( SliceHeader*&     rpcSliceHeader,
                          PicBuffer*        pcPicBuffer,
                          PicBufferList&    rcPicBufferOutputList,
                          PicBufferList&    rcPicBufferUnusedList,
                          SliceDataNALUnit& rcSliceDataNalUnit )
{
  //===== delete non-required slice headers =====
  if( ! m_bDependencyRepresentationInitialized )
  {
    while( m_cSliceHeaderList.size() )
    {
      SliceHeader*  pcSliceHeader = m_cSliceHeaderList.popFront();
      delete        pcSliceHeader;
    }
  }

  //===== create, read, init, and store slice header (and init SPS when required) =====
  RNOK( xReadSliceHeader          (  rpcSliceHeader, rcSliceDataNalUnit ) );
  RNOK( xInitSliceHeader          ( *rpcSliceHeader ) );
  RNOK( xInitSPS                  ( *rpcSliceHeader ) );
  m_cSliceHeaderList.push_back    (  rpcSliceHeader );

  //===== init DPB unit =====
  RNOK( xInitDPBUnit              ( *rpcSliceHeader, pcPicBuffer, rcPicBufferOutputList, rcPicBufferUnusedList ) );

  //===== init resize parameters =====  ---> write new function xInitResizeParameters, which is somewhat nicer
  RNOK( xInitESSandCroppingWindow ( *rpcSliceHeader, *m_pcCurrDPBUnit->getCtrlData().getMbDataCtrl(), m_pcCurrDPBUnit->getCtrlData() ) );

  //===== update parameters =====
  m_bLayerRepresentationInitialized       = true;
  m_bDependencyRepresentationInitialized  = true;
  Bool  bInterLayerPredSlice              = !rpcSliceHeader->getNoInterLayerPredFlag      ();
  Bool  bRewritingSlice                   =  rpcSliceHeader->getTCoeffLevelPredictionFlag ();
  if( m_bInterLayerPredLayerRep && bInterLayerPredSlice && ( m_bRewritingLayerRep != bRewritingSlice ) )
  {
    fprintf( stderr, "Layer representation has different values of tcoeff_level_prediction_flag -> not supported\n" );
    ROT( 1 );
  }
  m_bFirstILPredSliceInLayerRep           = (!m_bInterLayerPredLayerRep && bInterLayerPredSlice );
  m_bInterLayerPredLayerRep               = ( m_bInterLayerPredLayerRep || bInterLayerPredSlice );
  m_bRewritingLayerRep                    = ( m_bRewritingLayerRep      || bRewritingSlice      );

  return Err::m_nOK;
}

ErrVal
LayerDecoder::xParseSlice( SliceHeader& rcSliceHeader )
{
  ROF( m_pcCurrDPBUnit );
  ControlData&  rcControlData = m_pcCurrDPBUnit->getCtrlData();
  MbDataCtrl*   pcMbDataCtrl  = rcControlData.getMbDataCtrl();
  UInt          uiMbRead      = 0;

  RNOK( m_pcControlMng  ->initSliceForReading ( rcSliceHeader ) );
  RNOK( m_pcSliceReader ->read                ( rcSliceHeader,
                                                pcMbDataCtrl,
                                                m_pacMbStatus,
                                                m_uiFrameWidthInMb,
                                                uiMbRead ) );

  return Err::m_nOK;
}

ErrVal
LayerDecoder::xDecodeSlice( SliceHeader&            rcSliceHeader,
                            const SliceDataNALUnit& rcSliceDataNalUnit,
                            Bool                    bFirstSliceInLayerRepresentation )
{
  ROF( m_pcCurrDPBUnit );
  Bool          bReconstructBaseRep = rcSliceHeader.getStoreRefBasePicFlag() && ! rcSliceHeader.getQualityId();
  Bool          bReconstructAll     = rcSliceDataNalUnit.isDQIdMax();
  Bool          bReconstructMCMbs   = bReconstructAll || ( rcSliceDataNalUnit.isDependencyIdMax() && bReconstructBaseRep );
  PicType       ePicType            = rcSliceHeader.getPicType();
  ControlData&  rcControlData       = m_pcCurrDPBUnit->getCtrlData  ();
  Frame*        pcFrame             = m_pcCurrDPBUnit->getFrame     ();
  Frame*        pcBaseRepFrame      = m_pcCurrDPBUnit->getRefBasePic();
  Frame*        pcRecFrame          = ( bReconstructBaseRep ? pcBaseRepFrame : pcFrame );
  MbDataCtrl*   pcMbDataCtrl        = rcControlData.getMbDataCtrl ();

  //===== get reference frame lists =====
  RNOK( m_pcDecodedPictureBuffer->setPrdRefLists( m_pcCurrDPBUnit ) );
  RefFrameList& rcRefFrameList0 = rcControlData.getPrdFrameList( LIST_0 );
  RefFrameList& rcRefFrameList1 = rcControlData.getPrdFrameList( LIST_1 );
  MbDataCtrl*   pcMbDataCtrl0L1 = rcControlData.getMbDataCtrl0L1();
  rcSliceHeader.setRefFrameList( &rcRefFrameList0, ePicType, LIST_0 );
  rcSliceHeader.setRefFrameList( &rcRefFrameList1, ePicType, LIST_1 );

  //===== init base layer =====
  RNOK( xInitBaseLayer( rcControlData, bFirstSliceInLayerRepresentation ) );

  //----- decoding -----
  RNOK( m_pcControlMng->initSliceForDecoding( rcSliceHeader ) );

  if( rcSliceHeader.isMbaffFrame() )
  {
    RNOK( m_pcSliceDecoder->decodeMbAff( rcSliceHeader,
                                         pcMbDataCtrl,
                                         rcControlData.getBaseLayerCtrl(),
                                         rcControlData.getBaseLayerCtrlField(),
                                         pcRecFrame,
                                         m_pcResidualLF,
                                         m_pcResidualILPred,
                                         rcControlData.getBaseLayerRec(),
                                         rcControlData.getBaseLayerSbb(),
                                         &rcRefFrameList0,
                                         &rcRefFrameList1,
                                         pcMbDataCtrl0L1,
                                         bReconstructMCMbs ) );
  }
  else
  {
    RNOK( m_pcSliceDecoder->decode     ( rcSliceHeader,
                                         pcMbDataCtrl,
                                         rcControlData.getBaseLayerCtrl(),
                                         pcRecFrame,
                                         m_pcResidualLF,
                                         m_pcResidualILPred,
                                         rcControlData.getBaseLayerRec(),
                                         rcControlData.getBaseLayerSbb(),
                                         &rcRefFrameList0,
                                         &rcRefFrameList1,
                                         pcMbDataCtrl0L1,
                                         bReconstructMCMbs ) );
  }

  printf("  %s %4d ( LId%2d, TL%2d, QL%2d, %s-%c, BId%2d, AP%2d, QP%3d )\n",
    ePicType == FRAME ?  "Frame" : ePicType == TOP_FIELD ? "TopFd" : "BotFd",
    rcSliceHeader.getPoc                  (),
    rcSliceHeader.getDependencyId         (),
    rcSliceHeader.getTemporalId           (),
    rcSliceHeader.getQualityId            (),
    rcSliceHeader.isH264AVCCompatible     () ? "AVC" : "SVC",
    rcSliceHeader.getSliceType            () == I_SLICE ? 'I' :
    rcSliceHeader.getSliceType            () == P_SLICE ? 'P' : 'B',
    rcSliceHeader.getRefLayerDQId         (),
    rcSliceHeader.getAdaptiveBaseModeFlag () ? 1 : 0,
    rcSliceHeader.getSliceQp              () );
  return Err::m_nOK;
}

ErrVal
LayerDecoder::xFinishLayerRepresentation( SliceHeader&            rcSliceHeader,
                                          PicBufferList&          rcPicBufferOutputList,
                                          PicBufferList&          rcPicBufferUnusedList,
                                          const SliceDataNALUnit& rcSliceDataNalUnit,
                                          BinDataList&            rcBinDataList )
{
  ROF( m_pcCurrDPBUnit );
  Bool          bDepRepFinished     = rcSliceDataNalUnit.isLastSliceInDependencyRepresentation();
  Bool          bDPBUpdate          = rcSliceDataNalUnit.isDQIdMax();
  ControlData&  rcControlData       = m_pcCurrDPBUnit->getCtrlData();
  MbDataCtrl*   pcMbDataCtrl        = rcControlData.getMbDataCtrl ();
#ifdef SHARP_AVC_REWRITE_OUTPUT
#else
  Frame*        pcFrame             = m_pcCurrDPBUnit->getFrame   ();
  Frame*        pcBaseRepFrame      = m_pcCurrDPBUnit->getRefBasePic();
  PicType       ePicType            = rcSliceHeader.getPicType();
  Bool          bReconstructBaseRep = rcSliceHeader.getStoreRefBasePicFlag() && ! rcSliceHeader.getQualityId();
  Bool          bReconstructAll     = rcSliceDataNalUnit.isDQIdMax();
  Bool          bDeblockBaseRep     = bReconstructBaseRep && rcSliceDataNalUnit.isDependencyIdMax();
#endif

  //===== check for missing slices =====
  RNOK( xCheckForMissingSlices( rcSliceDataNalUnit, rcPicBufferOutputList, rcPicBufferUnusedList ) );

  //===== determine loop filter QPs =====
  RNOK( xSetLoopFilterQPs( rcSliceHeader, *pcMbDataCtrl ) );

#ifdef SHARP_AVC_REWRITE_OUTPUT
  //===== rewrite picture =====
  if( rcSliceDataNalUnit.isHighestRewriteLayer() )
  {
    RNOK( xRewritePicture( rcBinDataList, *pcMbDataCtrl ) );
  }
#else
  //===== loop filter =====
  if( bReconstructBaseRep )
  {
    //----- copy non-filtered frame -----
    RNOK( pcFrame->copy( pcBaseRepFrame, ePicType ) );

    //----- loop-filtering and store in DPB as base representation -----
    if( bDeblockBaseRep )
    {
      RNOK( m_pcLoopFilter->process( rcSliceHeader,
                                     pcBaseRepFrame,
                                     m_pcResidualLF,
                                     pcMbDataCtrl,
                                     0, rcControlData.getSpatialScalability(), m_pacMbStatus ) );
    }
  }
  RNOK( m_pcILPrediction->copy( pcFrame, ePicType ) );
  if( bReconstructAll )
  {
    RNOK( m_pcLoopFilter->process( rcSliceHeader,
                                   pcFrame,
                                   m_pcResidualLF,
                                   pcMbDataCtrl,
                                   0, rcControlData.getSpatialScalability(), m_pacMbStatus ) );
  }
#endif

  //===== store picture =====
  RNOK( m_pcDecodedPictureBuffer->storeCurrDPBUnit( bDepRepFinished ) );
  if( bDPBUpdate )
  {
    RNOK( m_pcH264AVCDecoder->updateDPB( m_uiDependencyId, rcPicBufferOutputList, rcPicBufferUnusedList ) );
  }

  DTRACE_NEWPIC;
  return Err::m_nOK;
}

ErrVal
LayerDecoder::xFinishSlice( SliceHeader&            rcSliceHeader,
                            PicBufferList&          rcPicBufferOutputList,
                            PicBufferList&          rcPicBufferUnusedList,
                            const SliceDataNALUnit& rcSliceDataNalUnit,
                            BinDataList&            rcBinDataList )
{
  //===== close Nal unit =====
  RNOK  ( m_pcNalUnitParser->closeNalUnit() );
  ROFRS ( rcSliceDataNalUnit.isLastSliceInLayerRepresentation(), Err::m_nOK );

  //===== finish layer representation and update parameters =====
  RNOK  ( xFinishLayerRepresentation( rcSliceHeader, rcPicBufferOutputList, rcPicBufferUnusedList, rcSliceDataNalUnit, rcBinDataList ) );
  m_bLayerRepresentationInitialized = false;
  m_bFirstILPredSliceInLayerRep     = false;
  m_bInterLayerPredLayerRep         = false;
  m_bRewritingLayerRep              = false;
  m_uiQualityId++;
  ROFRS ( rcSliceDataNalUnit.isLastSliceInDependencyRepresentation(), Err::m_nOK );

  //===== reset macroblock status map and update parameters =====
  for( UInt uiMb = 0; uiMb < m_uiMbNumber; uiMb++ )
  {
    m_pacMbStatus[ uiMb ].reset();
  }
  m_uiQualityId                           = 0;
  m_bDependencyRepresentationInitialized  = false;

  return Err::m_nOK;
}

ErrVal
LayerDecoder::xCheckForMissingSlices( const SliceDataNALUnit& rcSliceDataNalUnit,
                                      PicBufferList&          rcPicBufferOutputList,
                                      PicBufferList&          rcPicBufferUnusedList )
{
  UInt                  uiDQId            = ( m_uiDependencyId << 4 ) + m_uiQualityId;
  SliceHeader*          pcLastSliceHeader = m_cSliceHeaderList.back();
  FMO&                  rcFMO             = *pcLastSliceHeader->getFMO();
  MyList<SliceHeader*>  cVirtualSkipSliceList;

  //===== check whether slices are missing =====
  {
    for( Int iSliceGroupId = rcFMO.getFirstSliceGroupId(); ! rcFMO.SliceGroupCompletelyCoded( iSliceGroupId ); iSliceGroupId = rcFMO.getNextSliceGroupId( iSliceGroupId ) )
    {
      Int   iFirstMbInSliceGroup = rcFMO.getFirstMBOfSliceGroup( iSliceGroupId );
      Int   iLastMbInSliceGroup  = rcFMO.getLastMBInSliceGroup ( iSliceGroupId );
      Int   iNextMbAddress       = -1;
      UInt  uiFirstMbAddress     = MSYS_UINT_MAX;
      UInt  uiNumMbsInSlice      = 0;

      for( Int iMbAddress = iFirstMbInSliceGroup; iMbAddress != -1; iMbAddress = iNextMbAddress )
      {
        iNextMbAddress = rcFMO.getNextMBNr( iMbAddress );
        if( m_pacMbStatus[ iMbAddress ].getDQId() != uiDQId )
        {
          if( ! uiNumMbsInSlice )
          {
            uiFirstMbAddress  = (UInt)iMbAddress;
          }
          uiNumMbsInSlice++;

          Bool bLastMissingSliceMb  = ( iMbAddress == iLastMbInSliceGroup );
          if( !bLastMissingSliceMb )
          {
            assert( iNextMbAddress != -1 );
            bLastMissingSliceMb     = ( m_pacMbStatus[ iNextMbAddress ].getDQId() == uiDQId ||
                                        m_pacMbStatus[ iNextMbAddress ].getLastCodedSliceIdc() != m_pacMbStatus[ iMbAddress ].getLastCodedSliceIdc() );
          }

          if( bLastMissingSliceMb )
          {
            if( m_bRewritingLayerRep )
            {
#ifdef SHARP_AVC_REWRITE_OUTPUT
              fprintf( stderr, "WARNING: Rewriting of incomplete layer representations may result in non-conforming bitstreams\n" );
#else
              fprintf( stderr, "INFORMATION: Decoding of incomplete layer representations with MaxTCoeffLevelPredFlag may result in unsuitable visual quality\n" );
#endif
            }
            //===== create copy virtual skip slice and add to list =====
            SliceHeader* pcSliceHeader = new SliceHeader( *pcLastSliceHeader );
            ROF( pcSliceHeader );
            pcSliceHeader->setTrueSlice                 ( false );
            pcSliceHeader->setSliceSkipFlag             ( true );
            pcSliceHeader->setFirstMbInSlice            ( uiFirstMbAddress );
            pcSliceHeader->setNumMbsInSliceMinus1       ( uiNumMbsInSlice - 1 );
            pcSliceHeader->setTCoeffLevelPredictionFlag ( m_bRewritingLayerRep );
            pcSliceHeader->setLastCodedSliceHeader      ( m_pacMbStatus[ uiFirstMbAddress ].getLastCodedSliceHeader() );
            cVirtualSkipSliceList.push_back( pcSliceHeader );
            uiNumMbsInSlice = 0;
          }
        }
      }
    }
  }
  ROTRS( cVirtualSkipSliceList.empty(), Err::m_nOK );

  //===== check for transmission errors -> handle in error concealment in later versions
  ROF( rcSliceDataNalUnit.isDependencyIdMax() );
  ROF( rcSliceDataNalUnit.getQualityId() );

  //==== concealment ====
  while( !cVirtualSkipSliceList.empty() )
  {
    PicBuffer*   pcPicBuffer   = 0;
    SliceHeader* pcSliceHeader = cVirtualSkipSliceList.popFront();
    m_cSliceHeaderList.push_back( pcSliceHeader );

    //===== init virtual slice =====
    RNOK( xInitSliceHeader          ( *pcSliceHeader ) );
    RNOK( xInitDPBUnit              ( *pcSliceHeader, pcPicBuffer, rcPicBufferOutputList, rcPicBufferUnusedList ) );
    RNOK( xInitESSandCroppingWindow ( *pcSliceHeader, *m_pcCurrDPBUnit->getCtrlData().getMbDataCtrl(), m_pcCurrDPBUnit->getCtrlData() ) );

    //===== parse and decode virtual slice =====
    RNOK( xParseSlice ( *pcSliceHeader ) );
    RNOK( xDecodeSlice( *pcSliceHeader, rcSliceDataNalUnit, false ) );
  }

  return Err::m_nOK;
}

ErrVal
LayerDecoder::xSetLoopFilterQPs( SliceHeader& rcSliceHeader, MbDataCtrl& rcMbDataCtrl )
{
  RNOK( rcMbDataCtrl.initSlice( rcSliceHeader, DECODE_PROCESS, false, 0 ) );

  FMO& rcFMO = *rcSliceHeader.getFMO();
  for( Int iSliceGroupId = rcFMO.getFirstSliceGroupId(); ! rcFMO.SliceGroupCompletelyCoded( iSliceGroupId ); iSliceGroupId = rcFMO.getNextSliceGroupId( iSliceGroupId ) )
  {
    UInt  uiFirstMbInSliceGroup = rcFMO.getFirstMBOfSliceGroup( iSliceGroupId );
    UInt  uiLastMbInSliceGroup  = rcFMO.getLastMBInSliceGroup ( iSliceGroupId );

    for( UInt uiMbAddress = uiFirstMbInSliceGroup; uiMbAddress <= uiLastMbInSliceGroup; uiMbAddress = rcFMO.getNextMBNr( uiMbAddress ) )
    {
      MbDataAccess* pcMbDataAccess  = 0;
      UInt          uiMbX           = 0;
      UInt          uiMbY           = 0;
      rcSliceHeader.getMbPositionFromAddress( uiMbY, uiMbX, uiMbAddress );
      RNOK( rcMbDataCtrl.initMb( pcMbDataAccess, uiMbY, uiMbX ) );

      if( uiMbAddress != uiFirstMbInSliceGroup &&
          pcMbDataAccess->getSH().getTCoeffLevelPredictionFlag() &&
         !pcMbDataAccess->getSH().getNoInterLayerPredFlag() &&
         !pcMbDataAccess->getMbData().isIntra16x16() &&
          pcMbDataAccess->getMbData().getMbExtCbp() == 0 )
      {
        pcMbDataAccess->getMbData().setQp4LF( pcMbDataAccess->getLastQp4LF() );
      }
      else
      {
        pcMbDataAccess->getMbData().setQp4LF( pcMbDataAccess->getMbData().getQp() );
      }
    }
  }
  return Err::m_nOK;
}


#ifdef SHARP_AVC_REWRITE_OUTPUT
ErrVal
LayerDecoder::xRewritePicture( BinDataList& rcBinDataList, MbDataCtrl& rcMbDataCtrl )
{
  const SequenceParameterSet& rcSPS   =  m_cSliceHeaderList.back()->getSPS();
  FMO&                        rcFMO   = *m_cSliceHeaderList.back()->getFMO();
  Bool                        bMbAff  =  m_cSliceHeaderList.back()->isMbaffFrame();
  RNOK( m_pcRewriteEncoder->startPicture( rcSPS ) );

  for( Int iSliceGroupId = rcFMO.getFirstSliceGroupId(); ! rcFMO.SliceGroupCompletelyCoded( iSliceGroupId ); iSliceGroupId = rcFMO.getNextSliceGroupId( iSliceGroupId ) )
  {
    UInt          uiFirstMbInSliceGroup = rcFMO.getFirstMBOfSliceGroup( iSliceGroupId );
    UInt          uiLastMbInSliceGroup  = rcFMO.getLastMBInSliceGroup ( iSliceGroupId );
    SliceHeader*  pcLastSliceHeader     = 0;

    for( UInt uiMbAddress = uiFirstMbInSliceGroup; uiMbAddress <= uiLastMbInSliceGroup; uiMbAddress = rcFMO.getNextMBNr( uiMbAddress ) )
    {
      //===== start new slice =====
      if( pcLastSliceHeader != m_pacMbStatus[ uiMbAddress ].getSliceHeader() )
      {
        pcLastSliceHeader = m_pacMbStatus[ uiMbAddress ].getSliceHeader();
        rcMbDataCtrl.initSlice( *pcLastSliceHeader, DECODE_PROCESS, false, 0 );
      }

      //===== process macroblock =====
      MbDataAccess* pcMbDataAccess  = 0;
      UInt          uiMbX           = 0;
      UInt          uiMbY           = 0;
      pcLastSliceHeader       ->getMbPositionFromAddress  ( uiMbY, uiMbX, uiMbAddress );
      Bool          bSendEOS        = ( !bMbAff || ( uiMbAddress & 1 ) == 1 );
      RNOK( rcMbDataCtrl       .initMb   (  pcMbDataAccess, uiMbY, uiMbX ) );
      RNOK( m_pcRewriteEncoder->rewriteMb( *pcMbDataAccess, bSendEOS ) );
    }
  }

  RNOK( m_pcRewriteEncoder->finishPicture( rcBinDataList ) );
  return Err::m_nOK;
}
#endif

ErrVal
LayerDecoder::xReadSliceHeader( SliceHeader*&      rpcSliceHeader,
                                SliceDataNALUnit&  rcSliceDataNalUnit )
{
  //===== parse prefix header when available =====
  PrefixHeader* pcPrefixHeader = 0;
  if( rcSliceDataNalUnit.getBinDataPrefix() )
  {
    BinDataAccessor cBinDataAccessorPrefix;
    rcSliceDataNalUnit.getBinDataPrefix()->setMemAccessor( cBinDataAccessorPrefix );
    RNOK( m_pcNalUnitParser ->initNalUnit   ( cBinDataAccessorPrefix, 0 )  );
    ROF ( m_pcNalUnitParser ->getNalUnitType() == NAL_UNIT_PREFIX );
    pcPrefixHeader = new PrefixHeader( *m_pcNalUnitParser );
    ROF ( pcPrefixHeader );
    RNOK( pcPrefixHeader    ->read          ( *m_pcHeaderSymbolReadIf ) );
    RNOK( m_pcNalUnitParser ->closeNalUnit  () );
  }

  //===== parse slice header =====
  BinDataAccessor cBinDataAccessor;
  rcSliceDataNalUnit.getBinData()->setMemAccessor( cBinDataAccessor );
  RNOK( m_pcNalUnitParser ->initNalUnit   ( cBinDataAccessor, (Int)rcSliceDataNalUnit.getDQId() )  );
  if( pcPrefixHeader )
  {
    ROT( m_uiDependencyId );
    ROF( m_pcNalUnitParser->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR || m_pcNalUnitParser->getNalUnitType() == NAL_UNIT_CODED_SLICE );
    rpcSliceHeader  = new SliceHeader( *pcPrefixHeader );
    ROF( rpcSliceHeader );
    delete pcPrefixHeader; pcPrefixHeader = 0;
    rpcSliceHeader->NalUnitHeader::copy( *m_pcNalUnitParser, false );
  }
  else
  {
    rpcSliceHeader  = new SliceHeader( *m_pcNalUnitParser );
    ROF( rpcSliceHeader );
  }
  RNOK( rpcSliceHeader->read( *m_pcParameterSetMng, *m_pcHeaderSymbolReadIf ) );
  {
    //---- check for unsupported SI/SP slices ----
    SliceType eSliceType = rpcSliceHeader->getSliceType();
    if( eSliceType == SP_SLICE || eSliceType == SI_SLICE || Int( eSliceType ) > 7 )
    {
      fprintf( stderr, "\n" );
      fprintf( stderr, "Unsupported conformance point:\n" );
      fprintf( stderr, "   slice_type = %d\n", (Int)eSliceType );
      fprintf( stderr, "\n" );
      RERR();
    }
  }

  //==== set required buffer size =====
  rpcSliceHeader->getSPS().setAllocFrameMbsX( rcSliceDataNalUnit.getAllocFrameWidthInMbs () );
  rpcSliceHeader->getSPS().setAllocFrameMbsY( rcSliceDataNalUnit.getAllocFrameHeightInMbs() );

  return Err::m_nOK;
}

ErrVal
LayerDecoder::xInitSliceHeader( SliceHeader& rcSliceHeader )
{
  //===== check =====
  ROF( rcSliceHeader.getDependencyId() == m_uiDependencyId );
  ROF( rcSliceHeader.getQualityId   () == m_uiQualityId    );

  //===== infer slice header parameters =====
  if( rcSliceHeader.getQualityId() )
  {
    const SliceHeader* pcSliceHeader = m_pacMbStatus[ rcSliceHeader.getFirstMbInSlice() ].getSliceHeader();
    ROF( pcSliceHeader );
    rcSliceHeader.setDirectSpatialMvPredFlag        ( pcSliceHeader->getDirectSpatialMvPredFlag     () );
    rcSliceHeader.setNumRefIdxActiveOverrideFlag    ( pcSliceHeader->getNumRefIdxActiveOverrideFlag () );
    rcSliceHeader.setNumRefIdxL0ActiveMinus1        ( pcSliceHeader->getNumRefIdxL0ActiveMinus1     () );
    rcSliceHeader.setNumRefIdxL1ActiveMinus1        ( pcSliceHeader->getNumRefIdxL1ActiveMinus1     () );
    rcSliceHeader.getRefPicListReorderingL0 ().copy ( pcSliceHeader->getRefPicListReorderingL0      () );
    rcSliceHeader.getRefPicListReorderingL1 ().copy ( pcSliceHeader->getRefPicListReorderingL1      () );
    rcSliceHeader.setBasePredWeightTableFlag        ( pcSliceHeader->getBasePredWeightTableFlag     () );
    rcSliceHeader.setLumaLog2WeightDenom            ( pcSliceHeader->getLumaLog2WeightDenom         () );
    rcSliceHeader.setChromaLog2WeightDenom          ( pcSliceHeader->getChromaLog2WeightDenom       () );
    rcSliceHeader.getPredWeightTableL0      ().copy ( pcSliceHeader->getPredWeightTableL0           () );
    rcSliceHeader.getPredWeightTableL1      ().copy ( pcSliceHeader->getPredWeightTableL1           () );
    rcSliceHeader.setNoOutputOfPriorPicsFlag        ( pcSliceHeader->getNoOutputOfPriorPicsFlag     () );
    rcSliceHeader.setLongTermReferenceFlag          ( pcSliceHeader->getLongTermReferenceFlag       () );
    rcSliceHeader.getDecRefPicMarking       ().copy ( pcSliceHeader->getDecRefPicMarking            () );
    rcSliceHeader.setStoreRefBasePicFlag            ( pcSliceHeader->getStoreRefBasePicFlag         () );
    rcSliceHeader.getDecRefBasePicMarking   ().copy ( pcSliceHeader->getDecRefBasePicMarking        () );
    rcSliceHeader.setSliceGroupChangeCycle          ( pcSliceHeader->getSliceGroupChangeCycle       () );
  }

  //===== infer prediction weights when required =====
  if( rcSliceHeader.getPPS().getWeightedBiPredIdc () == 1       &&
      rcSliceHeader.getSliceType                  () == B_SLICE &&
     !rcSliceHeader.getNoInterLayerPredFlag       ()            &&
      rcSliceHeader.getBasePredWeightTableFlag    ()              )
  {
    SliceHeader*  pcBaseSliceHeader = 0;
    RNOK( m_pcH264AVCDecoder->getBaseSliceHeader( pcBaseSliceHeader, rcSliceHeader.getRefLayerDependencyId() ) );
    rcSliceHeader.setLumaLog2WeightDenom  ( pcBaseSliceHeader->getLumaLog2WeightDenom  () );
    rcSliceHeader.setChromaLog2WeightDenom( pcBaseSliceHeader->getChromaLog2WeightDenom() );
    rcSliceHeader.getPredWeightTableL0().copy( pcBaseSliceHeader->getPredWeightTableL0 () );
    rcSliceHeader.getPredWeightTableL1().copy( pcBaseSliceHeader->getPredWeightTableL1 () );
  }
  else if( rcSliceHeader.getPPS().getWeightedPredFlag ()            &&
           rcSliceHeader.getSliceType                 () == P_SLICE &&
          !rcSliceHeader.getNoInterLayerPredFlag      ()            &&
           rcSliceHeader.getBasePredWeightTableFlag   ()              )
  {
    SliceHeader*  pcBaseSliceHeader = 0;
    RNOK( m_pcH264AVCDecoder->getBaseSliceHeader( pcBaseSliceHeader, rcSliceHeader.getRefLayerDependencyId() ) );
    rcSliceHeader.setLumaLog2WeightDenom  ( pcBaseSliceHeader->getLumaLog2WeightDenom  () );
    rcSliceHeader.setChromaLog2WeightDenom( pcBaseSliceHeader->getChromaLog2WeightDenom() );
    rcSliceHeader.getPredWeightTableL0().copy( pcBaseSliceHeader->getPredWeightTableL0 () );
  }

  //===== init FMO ==== !!!! check & improve that part !!!!!
  rcSliceHeader.FMOInit();
#if 1 // what is that -> is this really required ?????
  if( rcSliceHeader.getNumMbsInSlice() )
  {
    rcSliceHeader.setLastMbInSlice( rcSliceHeader.getFMO()->getLastMbInSlice( rcSliceHeader.getFirstMbInSlice(), rcSliceHeader.getNumMbsInSlice() ) );
  }
  else
  {
    rcSliceHeader.setLastMbInSlice( rcSliceHeader.getFMO()->getLastMBInSliceGroup( rcSliceHeader.getFMO()->getSliceGroupId( rcSliceHeader.getFirstMbInSlice() ) ) );
  }
#endif

  return Err::m_nOK;
}

ErrVal
LayerDecoder::xInitSPS( const SliceHeader& rcSliceHeader )
{
  ROTRS( m_bSPSInitialized, Err::m_nOK );

  //===== init control manager =====
  RNOK( m_pcControlMng->initSlice0( const_cast<SliceHeader*>(&rcSliceHeader) ) );

  //===== set frame size parameters =====
  m_uiFrameWidthInMb  = rcSliceHeader.getSPS().getFrameWidthInMbs();
  m_uiFrameHeightInMb = rcSliceHeader.getSPS().getFrameHeightInMbs();
  m_uiMbNumber        = m_uiFrameHeightInMb * m_uiFrameWidthInMb;

  //===== re-allocate dynamic memory =====
  RNOK( xDeleteData() );
  RNOK( xCreateData( rcSliceHeader.getSPS() ) );

  //===== set initialization status =====
  m_bSPSInitialized = true;
  return Err::m_nOK;
}

ErrVal
LayerDecoder::xInitDPBUnit( SliceHeader&   rcSliceHeader,
                            PicBuffer*     pcPicBuffer,
                            PicBufferList& rcPicBufferOutputList,
                            PicBufferList& rcPicBufferUnusedList )
{
  RNOK( m_pcDecodedPictureBuffer->initCurrDPBUnit( m_pcCurrDPBUnit, pcPicBuffer, rcSliceHeader,
                                                   *m_pcPocCalculator, rcPicBufferOutputList, rcPicBufferUnusedList,
                                                   !m_bDependencyRepresentationInitialized,
                                                   !m_bLayerRepresentationInitialized ) );

  m_pcCurrDPBUnit->getCtrlData().setBaseLayerRec      ( 0 );
  m_pcCurrDPBUnit->getCtrlData().setBaseLayerSbb      ( 0 );
  m_pcCurrDPBUnit->getCtrlData().setBaseLayerCtrl     ( 0 );
  m_pcCurrDPBUnit->getCtrlData().setBaseLayerCtrlField( 0 );

  if( pcPicBuffer )
  {
    rcPicBufferUnusedList.push_back( pcPicBuffer );
  }

  return Err::m_nOK;
}


ErrVal
LayerDecoder::getBaseSliceHeader( SliceHeader*& rpcSliceHeader )
{
  ROF( m_bInitialized );
  CurrDPBUnit* pcBaseDPBUnit  = m_pcDecodedPictureBuffer->getILPredDPBUnit();
  ROF( pcBaseDPBUnit );
  rpcSliceHeader              = pcBaseDPBUnit->getSliceHeader();
  ROF( rpcSliceHeader );
  return Err::m_nOK;
}


ErrVal
LayerDecoder::getBaseLayerData( SliceHeader&      rcELSH,
                                Frame*&           pcFrame,
                                Frame*&           pcResidual,
                                MbDataCtrl*&      pcMbDataCtrl,
                                ResizeParameters& rcResizeParameters )
{
  ROF( m_bInitialized );

  CurrDPBUnit*  pcBaseDPBUnit   = m_pcDecodedPictureBuffer->getILPredDPBUnit();
  ROF( pcBaseDPBUnit );
  pcMbDataCtrl                  = pcBaseDPBUnit->getMbDataCtrl ();
  SliceHeader*  pcSliceHeader   = pcBaseDPBUnit->getSliceHeader();
  pcFrame                       = m_pcILPrediction;
  pcResidual                    = m_pcResidualILPred;
  ROF( pcMbDataCtrl );
  ROF( pcSliceHeader );
  ROF( pcFrame );
  ROF( pcResidual );

  //===== set correct reference layer slice header =====
  pcMbDataCtrl->setSliceHeader( pcSliceHeader );

  //===== check chroma format idc =====
  ROF( pcSliceHeader->getSPS().getChromaFormatIdc() == rcELSH.getSPS().getChromaFormatIdc() );

  if( rcELSH.getQualityId() )
  {
    Bool  bSPSOk  = rcELSH.getSPS().doesFulfillMGSConstraint( pcSliceHeader->getSPS() );
    if(  !bSPSOk )
    {
      fprintf( stdout, "WARNING: SPS of (D=%d,Q=%d) does not fulfill all MGS requirement -> ignore issue\n", rcELSH.getDependencyId(), rcELSH.getQualityId() );
    }
  }

  //===== update resize parameters =====
  rcResizeParameters.updateRefLayerParameters( *pcSliceHeader );

  if( rcResizeParameters.getSpatialResolutionChangeFlag() )
  {
    RNOK( m_apcFrameTemp[0]->copy( pcFrame, pcSliceHeader->getPicType() ) );
    pcFrame = m_apcFrameTemp[0];
    RNOK( m_pcLoopFilter->process( *pcSliceHeader, pcFrame, NULL, pcMbDataCtrl,
                                   &rcELSH.getInterLayerDeblockingFilterParameter(),
                                   pcBaseDPBUnit->getCtrlData().getSpatialScalability() ) );
  }

  return Err::m_nOK;
}

ErrVal
LayerDecoder::xCreateData( const SequenceParameterSet& rcSPS )
{
  UInt uiIndex;

  //========== CREATE FRAME MEMORIES ==========
  for( uiIndex = 0; uiIndex < NUM_TMP_FRAMES;  uiIndex++ )
  {
  	RNOK( Frame::create( m_apcFrameTemp[ uiIndex ], *m_pcYuvFullPelBufferCtrl, *m_pcYuvFullPelBufferCtrl, FRAME, 0 ) );
    RNOK( m_apcFrameTemp[ uiIndex ]->init() );
  }

  RNOK( Frame::create( m_pcResidualLF,        *m_pcYuvFullPelBufferCtrl, *m_pcYuvFullPelBufferCtrl, FRAME, 0 ) );
  RNOK( Frame::create( m_pcResidualILPred,    *m_pcYuvFullPelBufferCtrl, *m_pcYuvFullPelBufferCtrl, FRAME, 0 ) );
  RNOK( Frame::create( m_pcILPrediction,      *m_pcYuvFullPelBufferCtrl, *m_pcYuvFullPelBufferCtrl, FRAME, 0 ) );
  RNOK( Frame::create( m_pcBaseLayerFrame,    *m_pcYuvFullPelBufferCtrl, *m_pcYuvFullPelBufferCtrl, FRAME, 0 ) );
  RNOK( Frame::create( m_pcBaseLayerResidual, *m_pcYuvFullPelBufferCtrl, *m_pcYuvFullPelBufferCtrl, FRAME, 0 ) );
  RNOK( m_pcResidualLF        ->init() );
  RNOK( m_pcResidualILPred    ->init() );
  RNOK( m_pcILPrediction      ->init() );
  RNOK( m_pcBaseLayerFrame    ->init() );
  RNOK( m_pcBaseLayerResidual ->init() );

  //========== CREATE MACROBLOCK DATA MEMORIES ==========
  ROFS( ( m_pcBaseLayerCtrl = new MbDataCtrl() ) );
  RNOK(   m_pcBaseLayerCtrl ->init( rcSPS ) );
  ROFS( ( m_pcBaseLayerCtrlField = new MbDataCtrl() ) );
  RNOK(   m_pcBaseLayerCtrlField ->init( rcSPS ) );

  //========== CREATE UPDATE WEIGHTS ARRAY and WRITE BUFFER ==========
  UInt  uiAllocSizeX = rcSPS.getAllocFrameMbsX() << 4;
  UInt  uiAllocSizeY = rcSPS.getAllocFrameMbsY() << 4;
  ROT ( m_cDownConvert.init( uiAllocSizeX, uiAllocSizeY, 16 ) );

  //========= CREATE STATUS MAP ======
  ROFS( ( m_pacMbStatus = new MbStatus[ m_uiMbNumber ] ) );

  //========= CREATE ARRAYS for BaseModeFlag checking =====
  ROFS( ( m_apabBaseModeFlagAllowedArrays[0] = new Bool [ m_uiMbNumber ] ) );
  ROFS( ( m_apabBaseModeFlagAllowedArrays[1] = new Bool [ m_uiMbNumber ] ) );

  return Err::m_nOK;
}


ErrVal
LayerDecoder::xDeleteData()
{
  UInt uiIndex;

  //========== DELETE FRAME MEMORIES ==========
  for( uiIndex = 0; uiIndex < NUM_TMP_FRAMES; uiIndex++ )
  {
    if( m_apcFrameTemp[ uiIndex ] )
    {
      RNOK(   m_apcFrameTemp[ uiIndex ]->uninit() );
      RNOK( m_apcFrameTemp[ uiIndex ]->destroy() );
      m_apcFrameTemp[ uiIndex ] = NULL;

    }
  }

  if( m_pcResidualLF )
  {
    RNOK( m_pcResidualLF->uninit() );
    RNOK( m_pcResidualLF->destroy() );
    m_pcResidualLF = NULL;
  }
  if( m_pcResidualILPred )
  {
    RNOK( m_pcResidualILPred->uninit() );
    RNOK( m_pcResidualILPred->destroy() );
    m_pcResidualILPred = NULL;
  }

  if( m_pcILPrediction )
  {
    RNOK(   m_pcILPrediction->uninit() );
    RNOK( m_pcILPrediction->destroy() );
    m_pcILPrediction = 0;
  }

  if( m_pcBaseLayerFrame )
  {
    RNOK(   m_pcBaseLayerFrame->uninit() );
    RNOK( m_pcBaseLayerFrame->destroy() );
    m_pcBaseLayerFrame = NULL;
  }

  if( m_pcBaseLayerResidual )
  {
    RNOK(   m_pcBaseLayerResidual->uninit() );
    RNOK( m_pcBaseLayerResidual->destroy() );
    m_pcBaseLayerResidual = NULL;
  }

  if( m_pcBaseLayerCtrl )
  {
    RNOK( m_pcBaseLayerCtrl->uninit() );
    delete m_pcBaseLayerCtrl;
    m_pcBaseLayerCtrl = 0;
  }

  if( m_pcBaseLayerCtrlField )
  {
    RNOK( m_pcBaseLayerCtrlField->uninit() );
    delete m_pcBaseLayerCtrlField;
    m_pcBaseLayerCtrlField = NULL;
  }

  if( m_pacMbStatus )
  {
    delete [] m_pacMbStatus;
    m_pacMbStatus = 0;
  }

  if( m_apabBaseModeFlagAllowedArrays[0] )
  {
    delete [] m_apabBaseModeFlagAllowedArrays [0];
    m_apabBaseModeFlagAllowedArrays[0] = 0;
  }
  if( m_apabBaseModeFlagAllowedArrays[1] )
  {
    delete [] m_apabBaseModeFlagAllowedArrays [1];
    m_apabBaseModeFlagAllowedArrays[1] = 0;
  }

  ROF( m_cSliceHeaderList.empty() );

  return Err::m_nOK;
}


ErrVal
LayerDecoder::xInitESSandCroppingWindow( SliceHeader&  rcSliceHeader,
                                         MbDataCtrl&   rcMbDataCtrl,
                                         ControlData&  rcControlData)
{
  m_cResizeParameters.updateCurrLayerParameters( rcSliceHeader );
  if( rcSliceHeader.getQualityId() == 0 )
  {
    m_cResizeParametersAtQ0 = m_cResizeParameters;
  }
  m_pcCurrDPBUnit->getFrame()->setPicParameters( m_cResizeParametersAtQ0, &rcSliceHeader );
  if( m_pcCurrDPBUnit->getRefBasePic() )
  {
    m_pcCurrDPBUnit->getRefBasePic()->setPicParameters( m_cResizeParametersAtQ0, &rcSliceHeader );
  }

  if( rcSliceHeader.getNoInterLayerPredFlag() || rcSliceHeader.getQualityId() )
  {
    for( Int iMbY = 0; iMbY < (Int)m_uiFrameHeightInMb; iMbY++ )
    for( Int iMbX = 0; iMbX < (Int)m_uiFrameWidthInMb;  iMbX++ )
    {
      rcMbDataCtrl.getMbDataByIndex( (UInt)iMbY*m_uiFrameWidthInMb+(UInt)iMbX ).setInCropWindowFlag( true );
    }
    return Err::m_nOK;
  }

  //===== clear cropping window flags =====
  for( UInt uiMbY = 0; uiMbY < m_uiFrameHeightInMb; uiMbY++ )
  for( UInt uiMbX = 0; uiMbX < m_uiFrameWidthInMb;  uiMbX++ )
  {
    rcMbDataCtrl.getMbDataByIndex( uiMbY * m_uiFrameWidthInMb + uiMbX ).setInCropWindowFlag( false );
  }


  //===== set crop window flag: in current macroblock data (we don't need the base layer here) =====
  Int iScaledBaseOrigX  = m_cResizeParameters.m_iLeftFrmOffset;
  Int iScaledBaseOrigY  = m_cResizeParameters.m_iTopFrmOffset;
  Int iScaledBaseWidth  = m_cResizeParameters.m_iScaledRefFrmWidth;
  Int iScaledBaseHeight = m_cResizeParameters.m_iScaledRefFrmHeight;

	if( ! m_cResizeParameters.m_bIsMbAffFrame && ! m_cResizeParameters.m_bFieldPicFlag )
	{
    for( Int iMbY = 0; iMbY < (Int)m_uiFrameHeightInMb; iMbY++ )
    for( Int iMbX = 0; iMbX < (Int)m_uiFrameWidthInMb;  iMbX++ )
    {
      if( ( iMbX >= ( iScaledBaseOrigX + 15 ) / 16 ) && ( iMbX < ( iScaledBaseOrigX + iScaledBaseWidth  ) / 16 ) &&
          ( iMbY >= ( iScaledBaseOrigY + 15 ) / 16 ) && ( iMbY < ( iScaledBaseOrigY + iScaledBaseHeight ) / 16 )   )
      {
        rcMbDataCtrl.getMbDataByIndex( (UInt)iMbY*m_uiFrameWidthInMb+(UInt)iMbX ).setInCropWindowFlag( true );
      }
    }
	}
	else
	{
		AOT( m_uiFrameHeightInMb % 2 );
		for( Int iMbY0 = 0; iMbY0 < (Int)m_uiFrameHeightInMb; iMbY0+=2 )
		for( Int iMbX  = 0; iMbX  < (Int)m_uiFrameWidthInMb;  iMbX ++  )
		{
      Int   iMbY1  = iMbY0 + 1;
			if( ( iMbX  >= ( iScaledBaseOrigX + 15 ) / 16 ) && ( iMbX  < ( iScaledBaseOrigX + iScaledBaseWidth  ) / 16 ) &&
					( iMbY0 >= ( iScaledBaseOrigY + 15 ) / 16 ) && ( iMbY1 < ( iScaledBaseOrigY + iScaledBaseHeight ) / 16 )   )
			{
				rcMbDataCtrl.getMbDataByIndex( (UInt)iMbY0*m_uiFrameWidthInMb+(UInt)iMbX ).setInCropWindowFlag( true );
				rcMbDataCtrl.getMbDataByIndex( (UInt)iMbY1*m_uiFrameWidthInMb+(UInt)iMbX ).setInCropWindowFlag( true );
			}
		}
	}

  return Err::m_nOK;
}


ErrVal
LayerDecoder::xInitBaseLayer( ControlData&   rcControlData,
                              Bool           bFirstSliceInLayerRepresentation )
{
  //===== init =====
  rcControlData.setBaseLayerRec       ( 0 );
  rcControlData.setBaseLayerSbb       ( 0 );
  rcControlData.setBaseLayerCtrl      ( 0 );
  rcControlData.setBaseLayerCtrlField ( 0 );

  Frame*        pcBaseFrame         = 0;
  Frame*        pcBaseResidual      = 0;
  MbDataCtrl*   pcBaseDataCtrl      = 0;
  SliceHeader*  pcSliceHeader       = rcControlData.getSliceHeader();

  if( bFirstSliceInLayerRepresentation )
  {
    rcControlData.setSpatialScalability( false );
  }
  if( ! pcSliceHeader->getNoInterLayerPredFlag() )
  {
    RNOK( xGetBaseLayerData( rcControlData,
                             pcBaseFrame,
                             pcBaseResidual,
                             pcBaseDataCtrl,
                             m_cResizeParameters ) );
    rcControlData.setSpatialScalability( m_cResizeParameters.getSpatialResolutionChangeFlag() );
  }

  //===== motion data =====
  if( pcBaseDataCtrl )
  {
    RefFrameList* pcList0 = &rcControlData.getPrdFrameList( LIST_0 );
    RefFrameList* pcList1 = &rcControlData.getPrdFrameList( LIST_1 );

    pcSliceHeader->setSCoeffResidualPredFlag( &m_cResizeParameters );
    RNOK( m_pcBaseLayerCtrl->initSlice( *pcSliceHeader, PRE_PROCESS, false, NULL ) );
    RNOK( m_pcBaseLayerCtrl->upsampleMotion( pcSliceHeader, &m_cResizeParameters, pcBaseDataCtrl, pcList0, pcList1, m_cResizeParameters.m_bFieldPicFlag ) );
    rcControlData.setBaseLayerCtrl( m_pcBaseLayerCtrl );

    Bool bSNR = ( pcSliceHeader->getSCoeffResidualPredFlag() || pcSliceHeader->getTCoeffLevelPredictionFlag() );
    if( m_cResizeParameters.m_bIsMbAffFrame && !bSNR )
    {
      RNOK( m_pcBaseLayerCtrlField->initSlice( *pcSliceHeader, PRE_PROCESS, false, NULL ) );
      RNOK( m_pcBaseLayerCtrlField->upsampleMotion( pcSliceHeader, &m_cResizeParameters, pcBaseDataCtrl, pcList0, pcList1, true ) );
      rcControlData.setBaseLayerCtrlField( m_pcBaseLayerCtrlField );
    }

    pcSliceHeader->setBaseSliceHeader( pcBaseDataCtrl->getSliceHeader() );
  }

  //===== residual data =====
  if( pcBaseResidual )
  {
    if( m_bFirstILPredSliceInLayerRep )
    {
      RNOK( m_pcBaseLayerResidual->residualUpsampling( pcBaseResidual, m_cDownConvert, &m_cResizeParameters, pcBaseDataCtrl ) );
    }
    rcControlData.setBaseLayerSbb( m_pcBaseLayerResidual );
  }

  //===== reconstruction (intra) =====
  if( pcBaseFrame )
  {
    if( m_bFirstILPredSliceInLayerRep )
    {
      Frame*  pcTempBaseFrame = m_apcFrameTemp[0];
      Frame*  pcTempFrame     = m_apcFrameTemp[1];
      RNOK( m_pcBaseLayerFrame->intraUpsampling( pcBaseFrame, pcTempBaseFrame, pcTempFrame, m_cDownConvert, &m_cResizeParameters,
                                                 pcBaseDataCtrl, m_pcBaseLayerCtrl, m_pcBaseLayerCtrlField,
                                                 m_pcReconstructionBypass, rcControlData.getSliceHeader()->getConstrainedIntraResamplingFlag(),
                                                 m_apabBaseModeFlagAllowedArrays[0], m_apabBaseModeFlagAllowedArrays[1] ) );
      m_pcSliceDecoder->setIntraBLFlagArrays   ( m_apabBaseModeFlagAllowedArrays );
    }
    rcControlData.setBaseLayerRec( m_pcBaseLayerFrame );
  }

  xSetMCResizeParameters( m_cResizeParameters );

  return Err::m_nOK;
}


Void
LayerDecoder::xSetMCResizeParameters( ResizeParameters& rcResizeParameters )
{
  m_pcMotionCompensation->setResizeParameters( &rcResizeParameters );
}


ErrVal
LayerDecoder::xGetBaseLayerData( ControlData&      rcControlData,
                                 Frame*&           rpcBaseFrame,
                                 Frame*&           rpcBaseResidual,
                                 MbDataCtrl*&      rpcBaseDataCtrl,
                                 ResizeParameters& rcResizeParameters )

{
  SliceHeader* pcSliceHeader = rcControlData.getSliceHeader();

	RNOK( m_pcH264AVCDecoder->getBaseLayerData( *pcSliceHeader,
                                              rpcBaseFrame,
   	                                          rpcBaseResidual,
   	                                          rpcBaseDataCtrl,
   	                                          rcResizeParameters,
    																					pcSliceHeader->getRefLayerDependencyId() ) );
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END

