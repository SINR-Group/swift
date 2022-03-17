
#include "H264AVCDecoderLib.h"
#include "H264AVCDecoder.h"

#include "H264AVCCommonLib/LoopFilter.h"
#include "H264AVCCommonLib/TraceFile.h"

#include "SliceReader.h"
#include "SliceDecoder.h"

#include "CreaterH264AVCDecoder.h"
#include "ControlMngH264AVCDecoder.h"

#include "GOPDecoder.h"

#include "H264AVCCommonLib/CFMO.h"


H264AVC_NAMESPACE_BEGIN





H264AVCDecoder::H264AVCDecoder()
: m_bInitDone               ( false )
, m_pcNalUnitParser         ( 0 )
, m_pcHeaderSymbolReadIf    ( 0 )
, m_pcParameterSetMngAUInit ( 0 )
, m_pcParameterSetMngDecode ( 0 )
{
  ::memset( m_apcLayerDecoder, 0x00, MAX_LAYERS * sizeof( Void* ) );
}

H264AVCDecoder::~H264AVCDecoder()
{
}

ErrVal
H264AVCDecoder::create( H264AVCDecoder*& rpcH264AVCDecoder )
{
  rpcH264AVCDecoder = new H264AVCDecoder;
  ROT( NULL == rpcH264AVCDecoder );
  return Err::m_nOK;
}


ErrVal
H264AVCDecoder::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal
H264AVCDecoder::init( NalUnitParser*      pcNalUnitParser,
                      HeaderSymbolReadIf* pcHeaderSymbolReadIf,
                      ParameterSetMng*    pcParameterSetMngAUInit,
                      ParameterSetMng*    pcParameterSetMngDecode,
                      LayerDecoder*       apcLayerDecoder[MAX_LAYERS] )
{

  ROF( pcNalUnitParser );
  ROF( pcHeaderSymbolReadIf );
  ROF( pcParameterSetMngAUInit );
  ROF( pcParameterSetMngDecode );
  ROF( apcLayerDecoder );

  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    ROF( apcLayerDecoder[uiLayer] );
    m_apcLayerDecoder[uiLayer] = apcLayerDecoder[uiLayer];
  }

  m_bInitDone               = true;
  m_pcNalUnitParser         = pcNalUnitParser;
  m_pcHeaderSymbolReadIf    = pcHeaderSymbolReadIf;
  m_pcParameterSetMngAUInit = pcParameterSetMngAUInit;
  m_pcParameterSetMngDecode = pcParameterSetMngDecode;

  m_auiLastDQTPId[0]        = 0;
  m_auiLastDQTPId[1]        = 0;
  m_auiLastDQTPId[2]        = 0;
  m_auiLastDQTPId[3]        = 0;

  return Err::m_nOK;
}

ErrVal
H264AVCDecoder::uninit()
{
  m_bInitDone               = false;
  m_pcNalUnitParser         = 0;
  m_pcHeaderSymbolReadIf    = 0;
  m_pcParameterSetMngAUInit = 0;
  m_pcParameterSetMngDecode = 0;

  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    m_apcLayerDecoder[uiLayer] = 0;
  }

  return Err::m_nOK;
}

ErrVal
H264AVCDecoder::initNALUnit( BinData*&    rpcBinData,
                             AccessUnit&  rcAccessUnit )
{
  ROT( rcAccessUnit.isComplete    () );
  ROT( rcAccessUnit.isEndOfStream () );

  //===== check for empty NAL unit packet =====
  if( ! rpcBinData || ! rpcBinData->data() || ! rpcBinData->size() )
  {
    RNOK( rcAccessUnit.update( 0, false, false, false ) );
    return Err::m_nOK;
  }

  //===== create copy of bin data =====
  UChar*    pucBuffer     = new UChar [ rpcBinData->size() ];
  BinData*  pcBinDataCopy = new BinData;
  ROF( pucBuffer );
  ROF( pcBinDataCopy );
  memcpy( pucBuffer, rpcBinData->data(), rpcBinData->size() );
  pcBinDataCopy->set( pucBuffer, rpcBinData->size() );

  //===== parse required part of the NAL unit =====
  PrefixHeader* pcPrefixHeader    = 0;
  SliceHeader*  pcSliceHeader     = 0;
  Bool          bSEI              = false;
  Bool          bScalableSEI      = false;
  Bool          bBufferingPeriod  = false;
  {
    DTRACE_OFF;
    Bool            bCompletelyParsed = false;
    BinDataAccessor cBinDataAccessor;
    rpcBinData                ->setMemAccessor( cBinDataAccessor );
    RNOK  ( m_pcNalUnitParser ->initNalUnit   ( cBinDataAccessor ) );
    switch( m_pcNalUnitParser ->getNalUnitType() )
    {
    case NAL_UNIT_SPS:
    case NAL_UNIT_SUBSET_SPS:
      {
        //===== required for parsing of slice headers =====
        SequenceParameterSet* pcSPS = NULL;
        RNOK( SequenceParameterSet::create( pcSPS ) );
        RNOK( pcSPS->read( m_pcHeaderSymbolReadIf, m_pcNalUnitParser->getNalUnitType(), bCompletelyParsed ) );
        RNOK( m_pcParameterSetMngAUInit->store( pcSPS ) );
        break;
      }
    case NAL_UNIT_PPS:
      {
        //===== required for parsing of slice headers =====
        PictureParameterSet* pcPPS = NULL;
        RNOK( PictureParameterSet::create( pcPPS ) );
        RNOK( pcPPS->read( m_pcHeaderSymbolReadIf, m_pcNalUnitParser->getNalUnitType() ) );
        RNOK( m_pcParameterSetMngAUInit->store ( pcPPS ) );
        bCompletelyParsed = true;
        break;
      }
    case NAL_UNIT_PREFIX:
      {
        pcPrefixHeader = new PrefixHeader( *m_pcNalUnitParser );
        ROF ( pcPrefixHeader );
        RNOK( pcPrefixHeader->read( *m_pcHeaderSymbolReadIf ) );
        bCompletelyParsed = true;
        break;
      }
    case NAL_UNIT_CODED_SLICE:
    case NAL_UNIT_CODED_SLICE_IDR:
      {
        if( rcAccessUnit.getLastPrefixHeader() )
        {
          pcSliceHeader = new SliceHeader( *rcAccessUnit.getLastPrefixHeader() );
          ROF( pcSliceHeader );
          pcSliceHeader->NalUnitHeader::copy( *m_pcNalUnitParser, false );
        }
        else
        {
          pcSliceHeader = new SliceHeader( *m_pcNalUnitParser );
          ROF( pcSliceHeader );
        }
        RNOK ( pcSliceHeader->read( *m_pcParameterSetMngAUInit, *m_pcHeaderSymbolReadIf ) );
        break;
      }
    case NAL_UNIT_CODED_SLICE_SCALABLE:
      {
        pcSliceHeader = new SliceHeader( *m_pcNalUnitParser );
        ROF ( pcSliceHeader );
        RNOK( pcSliceHeader->read( *m_pcParameterSetMngAUInit, *m_pcHeaderSymbolReadIf ) );
        break;
      }
    case NAL_UNIT_SEI:
      {
        bSEI = true;
        SEI::MessageList cSEIMessageList;
        RNOK( SEI::read( m_pcHeaderSymbolReadIf, cSEIMessageList, m_pcParameterSetMngAUInit ) );
        bScalableSEI      = ( (*cSEIMessageList.begin())->getMessageType() == h264::SEI::SCALABLE_SEI );
        bBufferingPeriod  = ( (*cSEIMessageList.begin())->getMessageType() == h264::SEI::BUFFERING_PERIOD );
        if( (*cSEIMessageList.begin())->getMessageType() == h264::SEI::SCALABLE_NESTING_SEI )
        {
          SEI::SEIMessage*          pcSEI   = *cSEIMessageList.begin();
          SEI::ScalableNestingSei*  pcSNSEI = static_cast<SEI::ScalableNestingSei*>( pcSEI );
          bBufferingPeriod                  = pcSNSEI->bHasBufferingPeriod();
        }
        for( SEI::MessageList::iterator iter = cSEIMessageList.begin(); iter != cSEIMessageList.end(); iter++ )
        {
          delete (*iter);
        }
        break;
      }
    default:
      {
        // no parsing required
        bCompletelyParsed  = false;
        break;
      }
    }
    RNOK( m_pcNalUnitParser->closeNalUnit( bCompletelyParsed ) );
    DTRACE_ON;
  }

  //===== update access unit list =====
  switch( m_pcNalUnitParser->getNalUnitType() )
  {
  case NAL_UNIT_PREFIX:
    {
      RNOK( rcAccessUnit.update( pcBinDataCopy, *pcPrefixHeader ) );
      break;
    }
  case NAL_UNIT_CODED_SLICE:
  case NAL_UNIT_CODED_SLICE_IDR:
  case NAL_UNIT_CODED_SLICE_SCALABLE:
    {
      RNOK( rcAccessUnit.update( pcBinDataCopy, *pcSliceHeader ) );
      break;
    }
  case NAL_UNIT_CODED_SLICE_DATAPART_A:
  case NAL_UNIT_CODED_SLICE_DATAPART_B:
  case NAL_UNIT_CODED_SLICE_DATAPART_C:
    {
      RERR(); // not supported
      break;
    }
  default:
    {
      RNOK( rcAccessUnit.update( pcBinDataCopy, bSEI, bScalableSEI, bBufferingPeriod ) );
      break;
    }
  }
  return Err::m_nOK;
}


ErrVal
H264AVCDecoder::processNALUnit( PicBuffer*        pcPicBuffer,
                                PicBufferList&    rcPicBufferOutputList,
                                PicBufferList&    rcPicBufferUnusedList,
                                BinDataList&      rcBinDataList,
                                NALUnit&          rcNALUnit )
{
  if( ! rcNALUnit.isVCLNALUnit() )
  {
    NonVCLNALUnit& rcNonVCLNALUnit = *(NonVCLNALUnit*)rcNALUnit.getInstance();
    RNOK( xProcessNonVCLNALUnit( rcNonVCLNALUnit ) );
    rcPicBufferUnusedList.push_back( pcPicBuffer );
    return Err::m_nOK;
  }

  PicBufferList     cDummyList;
  SliceDataNALUnit& rcSliceDataNALUnit  = *(SliceDataNALUnit*)rcNALUnit.getInstance();
  PicBufferList&    rcOutputList        = ( rcSliceDataNALUnit.isDependencyIdMax() ? rcPicBufferOutputList : cDummyList );
  RNOK  ( m_apcLayerDecoder[ rcSliceDataNALUnit.getDependencyId() ]->processSliceData( pcPicBuffer, rcOutputList, rcPicBufferUnusedList, rcBinDataList, rcSliceDataNALUnit ) );
  ROFRS ( rcSliceDataNALUnit.isLastAccessUnitInStream() && rcSliceDataNALUnit.isLastSliceInAccessUnit(), Err::m_nOK );

  for( UInt uiDependencyId = 0; uiDependencyId < rcSliceDataNALUnit.getDependencyId(); uiDependencyId++ )
  {
    RNOK( m_apcLayerDecoder[ uiDependencyId                       ]->finishProcess( cDummyList,            rcPicBufferUnusedList ) );
  }
  RNOK(   m_apcLayerDecoder[ rcSliceDataNALUnit.getDependencyId() ]->finishProcess( rcPicBufferOutputList, rcPicBufferUnusedList ) );

  m_auiLastDQTPId[0] = rcSliceDataNALUnit.getDependencyId ();
  m_auiLastDQTPId[1] = rcSliceDataNALUnit.getQualityId    ();
  m_auiLastDQTPId[2] = rcSliceDataNALUnit.getTemporalId   ();
  m_auiLastDQTPId[3] = rcSliceDataNALUnit.getPriorityId   ();

  return Err::m_nOK;
}

ErrVal
H264AVCDecoder::updateDPB( UInt           uiTargetDependencyId,
                           PicBufferList& rcPicBufferOutputList,
                           PicBufferList& rcPicBufferUnusedList )
{
  PicBufferList cDummyList;
  for( UInt uiDependencyId = 0; uiDependencyId < uiTargetDependencyId; uiDependencyId++ )
  {
    RNOK( m_apcLayerDecoder[ uiDependencyId       ]->updateDPB( cDummyList,             rcPicBufferUnusedList ) );
  }
  RNOK(   m_apcLayerDecoder[ uiTargetDependencyId ]->updateDPB( rcPicBufferOutputList,  rcPicBufferUnusedList ) );
  return  Err::m_nOK;
}

ErrVal
H264AVCDecoder::xProcessNonVCLNALUnit( NonVCLNALUnit& rcNonVCLNALUnit )
{
  //===== parse NAL unit =====
  Bool            bCompletelyParsed  = true;
  BinDataAccessor cBinDataAccessor;
  rcNonVCLNALUnit.getBinData()->setMemAccessor( cBinDataAccessor );
  RNOK  ( m_pcNalUnitParser   ->initNalUnit   ( cBinDataAccessor, ( rcNonVCLNALUnit.isScalableSEI     () ? -3 : 
                                                                    rcNonVCLNALUnit.isBufferingPeriod () ? -2 :
                                                                    rcNonVCLNALUnit.isSEI             () ? -1 : 0 ) ) );
  switch( m_pcNalUnitParser   ->getNalUnitType() )
  {
  case NAL_UNIT_SPS:
    {
      SequenceParameterSet* pcSPS = NULL;
      RNOK( SequenceParameterSet::create( pcSPS ) );
      RNOK( pcSPS->read( m_pcHeaderSymbolReadIf, m_pcNalUnitParser->getNalUnitType(), bCompletelyParsed ) );
      RNOK( m_pcParameterSetMngDecode->store( pcSPS ) );
      printf("  NON-VCL: SEQUENCE PARAMETER SET (ID=%d)\n", pcSPS->getSeqParameterSetId() );
      break;
    }
  case NAL_UNIT_SUBSET_SPS:
    {
      SequenceParameterSet* pcSPS = NULL;
      RNOK( SequenceParameterSet::create( pcSPS ) );
      RNOK( pcSPS->read( m_pcHeaderSymbolReadIf, m_pcNalUnitParser->getNalUnitType(), bCompletelyParsed ) );
      RNOK( m_pcParameterSetMngDecode->store( pcSPS ) );
      printf("  NON-VCL: SUBSET SEQUENCE PARAMETER SET (ID=%d)\n", pcSPS->getSeqParameterSetId() );
      break;
    }
  case NAL_UNIT_PPS:
    {
      PictureParameterSet* pcPPS = NULL;
      RNOK( PictureParameterSet::create( pcPPS ) );
      RNOK( pcPPS->read( m_pcHeaderSymbolReadIf, m_pcNalUnitParser->getNalUnitType() ) );
      RNOK( m_pcParameterSetMngDecode->store ( pcPPS ) );
      printf("  NON-VCL: PICTURE PARAMETER SET (ID=%d)\n", pcPPS->getPicParameterSetId() );
      break;
    }
  case NAL_UNIT_SEI: // just read, but ignore
    {
      SEI::MessageList cSEIMessageList;
      RNOK( SEI::read( m_pcHeaderSymbolReadIf, cSEIMessageList, m_pcParameterSetMngDecode ) );
      printf("  NON-VCL: SEI NAL UNIT [message(s):" );
      for( SEI::MessageList::iterator iter = cSEIMessageList.begin(); iter != cSEIMessageList.end(); iter++ )
      {
        printf(" %d", (*iter)->getMessageType() );
        delete (*iter);
      }
      printf("]\n" );
      break;
    }
  case NAL_UNIT_ACCESS_UNIT_DELIMITER: // just read, but ignore
    {
      AUDelimiter cAUDelimiter( *m_pcNalUnitParser );
      RNOK( cAUDelimiter.read ( *m_pcHeaderSymbolReadIf ) );
      printf("  NON-VCL: ACCESS UNIT DELIMITER\n" );
      break;
    }
  case NAL_UNIT_END_OF_SEQUENCE: // just read, but ignore
    {
      EndOfSequence cEndOfSequence( *m_pcNalUnitParser );
      RNOK( cEndOfSequence.read   ( *m_pcHeaderSymbolReadIf ) );
      printf("  NON-VCL: END OF SEQUENCE\n" );
      bCompletelyParsed  = false;
      break;
    }
  case NAL_UNIT_END_OF_STREAM: // just read, but ignore
    {
      EndOfStream cEndOfStream( *m_pcNalUnitParser );
      RNOK( cEndOfStream.read ( *m_pcHeaderSymbolReadIf ) );
      printf("  NON-VCL: END OF STREAM\n" );
      bCompletelyParsed  = false;
      break;
    }
  case NAL_UNIT_FILLER_DATA: // just read, but ignore
    {
      FillerData cFillerData( *m_pcNalUnitParser );
      RNOK( cFillerData.read( *m_pcHeaderSymbolReadIf ) );
      cFillerData.setDependencyId ( m_auiLastDQTPId[0] );
      cFillerData.setQualityId    ( m_auiLastDQTPId[1] );
      cFillerData.setTemporalId   ( m_auiLastDQTPId[2] );
      cFillerData.setPriorityId   ( m_auiLastDQTPId[3] );
      printf("  NON-VCL: FILLER DATA (D=%d,Q=%d)\n", cFillerData.getDependencyId(), cFillerData.getQualityId() );
      break;
    }
  default:
    {
      // ignore
      bCompletelyParsed  = false;
      break;
    }
  }
  RNOK( m_pcNalUnitParser->closeNalUnit( bCompletelyParsed ) );
  return Err::m_nOK;
}

ErrVal
H264AVCDecoder::getBaseLayerData( SliceHeader&      rcELSH,
                                  Frame*&           pcFrame,
                                  Frame*&           pcResidual,
                                  MbDataCtrl*&      pcMbDataCtrl,
                                  ResizeParameters& rcResizeParameters,
                                  UInt              uiBaseLayerId )
{
  RNOK( m_apcLayerDecoder[uiBaseLayerId]->getBaseLayerData( rcELSH, pcFrame, pcResidual, pcMbDataCtrl, rcResizeParameters ) );
  return Err::m_nOK;
}

ErrVal
H264AVCDecoder::getBaseSliceHeader( SliceHeader*& rpcSliceHeader, UInt uiRefLayerDependencyId )
{
  RNOK( m_apcLayerDecoder[ uiRefLayerDependencyId ]->getBaseSliceHeader( rpcSliceHeader ) );
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END

