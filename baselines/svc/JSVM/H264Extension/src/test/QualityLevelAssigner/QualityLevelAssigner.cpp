
#include "ReadBitstreamFile.h"
#include "WriteBitstreamToFile.h"
#include "ReadYuvFile.h"
#include "WriteYuvToFile.h"
#include "QualityLevelParameter.h"
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/CommonDefs.h"
#include "QualityLevelAssigner.h"
#include <math.h>
#include <list>
#if WIN32
#if VC2005
#include <stdio.h>
FILE _iob[3] = {__iob_func()[0], __iob_func()[1], __iob_func()[2]};
#endif
#include <io.h>
#include <windows.h>
#endif


using namespace h264;



QualityLevelAssigner::QualityLevelAssigner()
: m_pcParameter             ( 0 )
, m_pcH264AVCPacketAnalyzer ( 0 )
, m_pcH264AVCDecoder        ( 0 )
, m_pcBitCounter            ( 0 )
, m_pcBitWriteBuffer        ( 0 )
, m_pcUvlcWriter            ( 0 )
, m_pcUvlcTester            ( 0 )
, m_pcNalUnitEncoder        ( 0 )
, m_uiNumLayers             ( 0 )
, m_bOutputReconstructions  ( false ) // for debugging
{
  ::memset( m_auiNumFGSLayers,      0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiNumFrames,         0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiGOPSize,           0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiNumTempLevel,      0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiFrameWidth,        0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiFrameHeight,       0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiSPSRequired,       0x00, 32                           *sizeof(UInt) );
  ::memset( m_auiSubsetSPSRequired, 0x00, 32                           *sizeof(UInt) );
  ::memset( m_auiPPSRequired,       0x00, 256                          *sizeof(UInt) );
  ::memset( m_aaadDeltaDist,        0x00, MAX_LAYERS*MAX_QUALITY_LEVELS*sizeof(Void*));
  ::memset( m_aaauiPacketSize,      0x00, MAX_LAYERS*MAX_QUALITY_LEVELS*sizeof(Void*));
  ::memset( m_aaauiQualityID,       0x00, MAX_LAYERS*MAX_QUALITY_LEVELS*sizeof(Void*));
  ::memset( m_aauiPicNumToFrmID,    0x00, MAX_LAYERS                   *sizeof(Void*));
  ::memset( m_aaauiNewPacketSize,   0x00, MAX_LAYERS*MAX_QUALITY_LEVELS*sizeof(Void*));
  ::memset( m_aauiPrFrameRate,      0x00, MAX_LAYERS*MAX_SIZE_PID*sizeof(UInt) );
  ::memset( m_aauiBaseIndex,        0x00, MAX_LAYERS*sizeof(UInt*) );
  m_uiPriorityLevelSEISize = 0;
}


QualityLevelAssigner::~QualityLevelAssigner()
{
}


ErrVal
QualityLevelAssigner::create( QualityLevelAssigner*& rpcQualityLevelAssigner )
{
  rpcQualityLevelAssigner = new QualityLevelAssigner;
  ROF( rpcQualityLevelAssigner );
  return Err::m_nOK;
}


ErrVal
QualityLevelAssigner::init( QualityLevelParameter* pcParameter )
{
  ROF( pcParameter );
  m_pcParameter = pcParameter;

  //--- create objects ---
  RNOK( H264AVCPacketAnalyzer::create( m_pcH264AVCPacketAnalyzer ) );
  RNOK( CreaterH264AVCDecoder::create( m_pcH264AVCDecoder ) );
  RNOK( BitCounter           ::create( m_pcBitCounter ) );
  RNOK( BitWriteBuffer       ::create( m_pcBitWriteBuffer ) );
  RNOK( UvlcWriter           ::create( m_pcUvlcWriter ) );
  RNOK( UvlcWriter           ::create( m_pcUvlcTester ) );
  RNOK( NalUnitEncoder       ::create( m_pcNalUnitEncoder ) );

  //--- initialize encoder objects ---
  RNOK( m_pcBitWriteBuffer  ->init() );
  RNOK( m_pcBitCounter      ->init() );
  RNOK( m_pcUvlcWriter      ->init( m_pcBitWriteBuffer ) );
  RNOK( m_pcUvlcTester      ->init( m_pcBitCounter ) );
  RNOK( m_pcNalUnitEncoder  ->init( m_pcBitWriteBuffer, m_pcUvlcWriter, m_pcUvlcTester ) );

  //--- get basic stream parameters ---
  RNOK( xInitStreamParameters() );

  //--- create arrays ---
  for( UInt uiLayer = 0; uiLayer <  m_uiNumLayers;              uiLayer++ )
  for( UInt uiFGS   = 0; uiFGS   <= m_auiNumFGSLayers[uiLayer]; uiFGS  ++ )
  {
    ROF( m_auiNumFrames[uiLayer] );
    ROFS( ( m_aaadDeltaDist       [uiLayer][uiFGS] = new Double[m_auiNumFrames[uiLayer]] ) );
    ROFS( ( m_aaauiPacketSize     [uiLayer][uiFGS] = new UInt  [m_auiNumFrames[uiLayer]] ) );
    ROFS( ( m_aaauiNewPacketSize  [uiLayer][uiFGS] = new UInt  [m_auiNumFrames[uiLayer]] ) );
    ROFS( ( m_aaauiQualityID      [uiLayer][uiFGS] = new UInt  [m_auiNumFrames[uiLayer]] ) );
    if( uiFGS == 0 )
    {
      ROFS( ( m_aauiPicNumToFrmID [uiLayer]        = new UInt  [m_auiNumFrames[uiLayer]] ) );
    }
  }

  //--- init start code ----
  m_aucStartCodeBuffer[0] = 0;
  m_aucStartCodeBuffer[1] = 0;
  m_aucStartCodeBuffer[2] = 0;
  m_aucStartCodeBuffer[3] = 1;
  m_cBinDataStartCode.reset();
  m_cBinDataStartCode.set( m_aucStartCodeBuffer, 4 );

  return Err::m_nOK;
}


ErrVal
QualityLevelAssigner::destroy()
{
  m_cBinDataStartCode.reset();

  if( m_pcH264AVCPacketAnalyzer )
  {
    RNOK( m_pcH264AVCPacketAnalyzer->destroy() );
  }
  if( m_pcH264AVCDecoder )
  {
    RNOK( m_pcH264AVCDecoder->destroy() );
  }
  if( m_pcBitCounter )
  {
    RNOK( m_pcBitCounter->uninit  () );
    RNOK( m_pcBitCounter->destroy () );
  }
  if( m_pcBitWriteBuffer )
  {
    RNOK( m_pcBitWriteBuffer->uninit  () );
    RNOK( m_pcBitWriteBuffer->destroy () );
  }
  if( m_pcUvlcTester )
  {
    RNOK( m_pcUvlcTester->uninit  () );
    RNOK( m_pcUvlcTester->destroy () );
  }
  if( m_pcUvlcWriter )
  {
    RNOK( m_pcUvlcWriter->uninit  () );
    RNOK( m_pcUvlcWriter->destroy () );
  }
  if( m_pcNalUnitEncoder )
  {
    RNOK( m_pcNalUnitEncoder->uninit  () );
    RNOK( m_pcNalUnitEncoder->destroy () )
  }


  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS;          uiLayer++ )
  for( UInt uiFGS   = 0; uiFGS   < MAX_QUALITY_LEVELS;  uiFGS  ++ )
  {
    delete [] m_aaadDeltaDist       [uiLayer][uiFGS];
    delete [] m_aaauiPacketSize     [uiLayer][uiFGS];
    delete [] m_aaauiNewPacketSize  [uiLayer][uiFGS];
    delete [] m_aaauiQualityID      [uiLayer][uiFGS];
    if( uiFGS == 0 )
    {
      delete [] m_aauiPicNumToFrmID [uiLayer];
    }
  }

  //===== delete picture buffers =====
  RNOK( xClearPicBufferLists() );

  delete this;
  return Err::m_nOK;
}


ErrVal
QualityLevelAssigner::go()
{
  //===== get rate and distortion values =====
  if( m_pcParameter->readDataFile() )
  {
    RNOK( xReadDataFile( m_pcParameter->getDataFileName() ) );
  }
  else
  {
    //JVT-S043
    Bool bMultiLayer = (m_pcParameter->getQLAssignerMode()==QLASSIGNERMODE_MLQL? true : false);

    RNOK( xInitRateAndDistortion(bMultiLayer) );

    if( m_pcParameter->writeDataFile() )
    {
      RNOK( xWriteDataFile( m_pcParameter->getDataFileName() ) );
    }
  }

  if( ! m_pcParameter->getOutputBitStreamName().empty() )
  {
    //===== determine quality levels =====
    if( m_pcParameter->getQLAssignerMode()==QLASSIGNERMODE_QL )
    {
        RNOK( xDetermineQualityIDs() );
    }
    //JVT-S043
    else if( m_pcParameter->getQLAssignerMode()==QLASSIGNERMODE_MLQL )
    {
       RNOK( xDetermineMultiLayerQualityIDs() );
    }


    //===== write output stream with quality levels =====
    //if( m_pcParameter->writeQualityLayerSEI() )//SEI changes update
    if( m_pcParameter->writePriorityLevelSEI() )//SEI changes update
    {
      RNOK( xWriteQualityLayerStreamSEI() );
      RNOK( xUpdateScalableSEI         () );
    }
    else
    {
      RNOK( xWriteQualityLayerStreamPID() );
    }
  }

  printf("\n");
  return Err::m_nOK;
}


ErrVal
QualityLevelAssigner::xGetNewPicBuffer( PicBuffer*& rpcPicBuffer,
                                        UInt        uiSize )
{
  if( m_cUnusedPicBufferList.empty() )
  {
    rpcPicBuffer = new PicBuffer( new UChar[ uiSize ] );
  }
  else
  {
    rpcPicBuffer = m_cUnusedPicBufferList.popFront();
  }

  m_cActivePicBufferList.push_back( rpcPicBuffer );
  return Err::m_nOK;
}


ErrVal
QualityLevelAssigner::xRemovePicBuffer( PicBufferList& rcPicBufferUnusedList )
{
  while( ! rcPicBufferUnusedList.empty() )
  {
    PicBuffer* pcBuffer = rcPicBufferUnusedList.popFront();

    if( NULL != pcBuffer )
    {
      PicBufferList::iterator  begin = m_cActivePicBufferList.begin();
      PicBufferList::iterator  end   = m_cActivePicBufferList.end  ();
      PicBufferList::iterator  iter  = std::find( begin, end, pcBuffer );

      ROT( iter == end ); // there is something wrong if the address is not in the active list

      AOT( pcBuffer->isUsed() )
      m_cUnusedPicBufferList.push_back( pcBuffer );
      m_cActivePicBufferList.erase    (  iter );
    }
  }
  return Err::m_nOK;
}


ErrVal
QualityLevelAssigner::xClearPicBufferLists()
{
  ROF( m_cActivePicBufferList.empty() );

  PicBufferList::iterator iter;
  for( iter = m_cUnusedPicBufferList.begin(); iter != m_cUnusedPicBufferList.end(); iter++ )
  {
    delete (*iter)->getBuffer();
    delete (*iter);
  }
  m_cUnusedPicBufferList.clear();

  for( iter = m_cActivePicBufferList.begin(); iter != m_cActivePicBufferList.end(); iter++ )
  {
    delete (*iter)->getBuffer();
    delete (*iter);
  }
  m_cActivePicBufferList.clear();

  return Err::m_nOK;
}


ErrVal
QualityLevelAssigner::xInitStreamParameters()
{
  printf( "analyse stream content ..." );

  Bool              bFirstPacket    = true;
  BinData*          pcBinData       = 0;
  SEI::SEIMessage*  pcScalableSEI   = 0;
  UInt              uiLastPrefixTL  = MSYS_UINT_MAX;
  PacketDescription cPacketDescription;

  m_uiNumLayers = 0;
  ::memset( m_auiNumFGSLayers,      0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiNumFrames,         0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiGOPSize,           0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiNumTempLevel,      0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiFrameWidth,        0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiFrameHeight,       0x00, MAX_LAYERS                   *sizeof(UInt) );
  ::memset( m_auiSPSRequired,       0x00, 32                           *sizeof(UInt) );
  ::memset( m_auiSubsetSPSRequired, 0x00, 32                           *sizeof(UInt) );
  ::memset( m_auiPPSRequired,       0x00, 256                          *sizeof(UInt) );

  //===== init =====
  RNOK( m_pcH264AVCPacketAnalyzer->init() );
  ReadBitstreamFile* pcReadBitStream = 0;
  RNOK( ReadBitstreamFile::create( pcReadBitStream ) );
  RNOK( pcReadBitStream->init( m_pcParameter->getInputBitStreamName() ) );


  //===== loop over packets =====
  while( true )
  {
    //----- read packet -----
    Bool bEOS = false;
    RNOK( pcReadBitStream->extractPacket( pcBinData, bEOS ) );
    if( bEOS )
    {
      //manu.mathew@samsung : memory leak fix
      RNOK( pcReadBitStream->releasePacket( pcBinData ) );
      pcBinData = NULL;
      //--
      break;
    }

    //----- get packet description -----
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSEI ) );
    ROT( bFirstPacket && !pcScalableSEI );
    if( pcScalableSEI )
    {
      Bool              bUncompleteInfo     = false;
      SEI::ScalableSei* pcScalSEI           = (SEI::ScalableSei*)pcScalableSEI;
      UInt              uiNumLayersInSEI    = pcScalSEI->getNumLayersMinus1() + 1;
      Bool              abUsed[MAX_LAYERS]  = { false, false, false, false, false, false, false, false };

      for( UInt ui = 0; ui < uiNumLayersInSEI; ui++ )
      {
        if(// ! pcScalSEI->getDecodingDependencyInfoPresentFlag ( ui ) ||  //JVT-S036 lsj
            ! pcScalSEI->getFrmSizeInfoPresentFlag            ( ui )   )
        {
          bUncompleteInfo = true;
          break;
        }

        UInt uiLayerId = pcScalSEI->getDependencyId( ui );
        if( abUsed[uiLayerId] )
        { // update information
          m_auiNumTempLevel [uiLayerId] = gMax( m_auiNumTempLevel[uiLayerId], pcScalSEI->getTemporalId( ui ) );
          m_auiGOPSize      [uiLayerId] = ( 1 << m_auiNumTempLevel[uiLayerId] );
        }
        else
        { // init information
          abUsed            [uiLayerId] = true;
          m_auiNumTempLevel [uiLayerId] = pcScalSEI->getTemporalId( ui );
          m_auiGOPSize      [uiLayerId] = ( 1 << m_auiNumTempLevel[uiLayerId] );
          m_auiFrameWidth   [uiLayerId] = ( pcScalSEI->getFrmWidthInMbsMinus1 ( ui ) + 1 ) << 4;
          m_auiFrameHeight  [uiLayerId] = ( pcScalSEI->getFrmHeightInMbsMinus1( ui ) + 1 ) << 4;
        }
      }

      ROT( bUncompleteInfo );

      delete pcScalableSEI;
      pcScalableSEI = 0;
      bFirstPacket  = false;
    }

    //----- analyse packets -----
    if(cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_IDR || cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE)
    {
      if( uiLastPrefixTL == MSYS_UINT_MAX )
      {
        fprintf( stderr, "\nMissing prefix NAL unit\n\n" );
        ROT(1);
      }
      cPacketDescription.Level = uiLastPrefixTL;
    }
    if( cPacketDescription.NalUnitType == 14 )
    {
      RNOK( pcReadBitStream->releasePacket( pcBinData ) );
      uiLastPrefixTL = cPacketDescription.Level;
      continue;
    }
    else
    {
      uiLastPrefixTL = MSYS_UINT_MAX;
    }
    if( cPacketDescription.FGSLayer )
    {
      if( cPacketDescription.Layer+1 > m_uiNumLayers)
      {
        m_uiNumLayers = cPacketDescription.Layer+1;
      }
      if( cPacketDescription.FGSLayer > m_auiNumFGSLayers[cPacketDescription.Layer] )
      {
        m_auiNumFGSLayers[cPacketDescription.Layer] = cPacketDescription.FGSLayer;
      }
      if( cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE )
        m_auiSubsetSPSRequired[cPacketDescription.SPSid] |= (1 << cPacketDescription.Layer);
      else
        m_auiSPSRequired[cPacketDescription.SPSid] |= (1 << cPacketDescription.Layer);
      m_auiPPSRequired[cPacketDescription.PPSid] |= (1 << cPacketDescription.Layer);
    }
    else if( ! cPacketDescription.ParameterSet && cPacketDescription.NalUnitType != NAL_UNIT_SEI &&
             ! cPacketDescription.FGSLayer     && cPacketDescription.uiFirstMb == 0 )
    {
      m_auiNumFrames[cPacketDescription.Layer]++;

      if( cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE )
        m_auiSubsetSPSRequired[cPacketDescription.SPSid] |= (1 << cPacketDescription.Layer);
      else
        m_auiSPSRequired[cPacketDescription.SPSid] |= (1 << cPacketDescription.Layer);
      m_auiPPSRequired[cPacketDescription.PPSid] |= (1 << cPacketDescription.Layer);
    }

    //----- delete bin data -----
    RNOK( pcReadBitStream->releasePacket( pcBinData ) );
  }


  //===== uninit =====
  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );
  RNOK( pcReadBitStream->uninit() );
  RNOK( pcReadBitStream->destroy() );

  printf("\n");
  return Err::m_nOK;
}



ErrVal
QualityLevelAssigner::xInitRateAndDistortion(Bool bMultiLayer)
{
  RNOK( xInitRateValues() );

  //----- create temporarily distortion arrays -----
  UInt  uiLayer, uiFGS, uiTLevel, uiFrame;
  UInt* aaaauiDistortionDep[MAX_LAYERS][MAX_QUALITY_LEVELS][MAX_DSTAGES+1];
  UInt* aaaauiDistortionInd[MAX_LAYERS][MAX_QUALITY_LEVELS][MAX_DSTAGES+1];
  for( uiLayer  = 0;  uiLayer   <  m_uiNumLayers;              uiLayer  ++ )
  {
    //JVT-S043
    UInt uiTopLayer = (bMultiLayer? m_uiNumLayers-1 : uiLayer);

    for( uiFGS    = 0;  uiFGS     <= m_auiNumFGSLayers[uiLayer]; uiFGS    ++ )
    for( uiTLevel = 0;  uiTLevel  <= m_auiNumTempLevel[uiLayer]; uiTLevel ++ )
    {
      ROFRS( ( aaaauiDistortionDep[uiLayer][uiFGS][uiTLevel] = new UInt [m_auiNumFrames[uiTopLayer]] ), Err::m_nOK );
      ROFRS( ( aaaauiDistortionInd[uiLayer][uiFGS][uiTLevel] = new UInt [m_auiNumFrames[uiTopLayer]] ), Err::m_nOK );
    }
  }
  for( uiLayer  = 0;  uiLayer   <  m_uiNumLayers;              uiLayer  ++ )
  for( uiFrame  = 0;  uiFrame   <  m_auiNumFrames[uiLayer];    uiFrame  ++ )
  {
    m_aaadDeltaDist[uiLayer][0][uiFrame] = 100.0; // dummy value
  }


  Bool bDep = m_pcParameter->useDependentDistCalc   ();
  Bool bInd = m_pcParameter->useIndependentDistCalc ();
  ROF( bDep || bInd );
  if ( true )
  {
    bDep  = true;
    bInd  = false; // independent analysis is not possible
  }


  //----- determine picture distortions -----
  for( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer++ )
  {
    //JVT-S043
    UInt uiTopLayer = (bMultiLayer? m_uiNumLayers-1 : uiLayer);

    //----- get base layer distortion -----
    RNOK( xInitDistortion( aaaauiDistortionDep[uiLayer][0][0], uiTopLayer, uiLayer, 0 ) );

    memcpy(              aaaauiDistortionInd[uiLayer][0][0], aaaauiDistortionDep[uiLayer][0][0], m_auiNumFrames[uiTopLayer]*sizeof(UInt) );
    for( uiTLevel = 1; uiTLevel <= m_auiNumTempLevel[uiLayer]; uiTLevel++ )
    {
      memcpy(     aaaauiDistortionDep[uiLayer][0][uiTLevel], aaaauiDistortionDep[uiLayer][0][0], m_auiNumFrames[uiTopLayer]*sizeof(UInt) );
      memcpy(     aaaauiDistortionInd[uiLayer][0][uiTLevel], aaaauiDistortionInd[uiLayer][0][0], m_auiNumFrames[uiTopLayer]*sizeof(UInt) );
    }
    //----- get enhancement distortions -----
    for( uiFGS    = 1; uiFGS    <= m_auiNumFGSLayers[uiLayer]; uiFGS   ++ )
    for( uiTLevel = 0; uiTLevel <= m_auiNumTempLevel[uiLayer]; uiTLevel++ )
    {
      if( bDep )
      {
        RNOK( xInitDistortion( aaaauiDistortionDep[uiLayer][uiFGS][uiTLevel], uiTopLayer, uiLayer, uiFGS, uiTLevel, false ) );
      }
      if( bInd )
      {
        RNOK( xInitDistortion( aaaauiDistortionInd[uiLayer][uiFGS][uiTLevel], uiTopLayer, uiLayer, uiFGS, uiTLevel, true  ) );
      }
    }
    RNOK( xClearPicBufferLists() ); // spatial resolution can be changed
  }


  //----- init delta distortion -----
  printf( "determine delta distortions ..." );
  for( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer++ )
  {
    //JVT-S043
    UInt uiTopLayer = (bMultiLayer? m_uiNumLayers-1 : uiLayer);
    UInt* puiPic2FNum = m_aauiPicNumToFrmID[uiLayer];

    //----- determine delta distortions -----
    for( uiFGS = 1; uiFGS <= m_auiNumFGSLayers[uiLayer]; uiFGS++ )
    {
      //----- key pictures (that's a bit tricky) -----
      {
        UInt  uiMaxTLevel     = m_auiNumTempLevel[uiLayer];
        Bool  bLastBaseRep = false;
        for( UInt uiBaseRepCount = 0; ! bLastBaseRep; uiBaseRepCount++ )
        {
          Double dDistortionBaseDep  = 0;
          Double dDistortionBaseInd  = 0;
          Double dDistortionEnhDep   = 0;
          Double dDistortionEnhInd   = 0;
          bLastBaseRep         = ( ( ( m_auiNumFrames[uiLayer] - 1 ) / m_auiGOPSize[uiLayer] ) == uiBaseRepCount );

          UInt   uiPicNum         = uiBaseRepCount * m_auiGOPSize[uiLayer];
          UInt   uiTopLayerPicNum = uiBaseRepCount * m_auiGOPSize[uiTopLayer];
          // UInt   uiStepSize2      = m_auiGOPSize[uiLayer] >> 1;  // unused variable mwi060625
          UInt   uiTopLayerStepSize2 = m_auiGOPSize[uiTopLayer] >> 1;

          //---- preceding level 1 picture -----
          if( uiBaseRepCount )
          {
            dDistortionBaseDep += xLog( aaaauiDistortionDep[uiLayer][uiFGS-1][uiMaxTLevel][uiTopLayerPicNum-uiTopLayerStepSize2] ) / 2;
            dDistortionEnhDep  += xLog( aaaauiDistortionDep[uiLayer][uiFGS  ][0          ][uiTopLayerPicNum-uiTopLayerStepSize2] ) / 2;

            dDistortionBaseInd += xLog( aaaauiDistortionInd[uiLayer][uiFGS-1][0          ][uiTopLayerPicNum-uiTopLayerStepSize2] ) / 2;
            dDistortionEnhInd  += xLog( aaaauiDistortionInd[uiLayer][uiFGS  ][0          ][uiTopLayerPicNum-uiTopLayerStepSize2] ) / 2;
          }
          //---- normal pictures -----
          Int iStartPicNum  = ( uiBaseRepCount   ?  uiTopLayerPicNum - uiTopLayerStepSize2 + 1 : 0 );
          Int iEndPicNum    = ( bLastBaseRep ? m_auiNumFrames[uiTopLayer] - 1 : uiTopLayerPicNum + uiTopLayerStepSize2 - 1 );
          for( Int iCheckPicNum = iStartPicNum; iCheckPicNum <= iEndPicNum; iCheckPicNum++ )
          {
            dDistortionBaseDep += xLog( aaaauiDistortionDep[uiLayer][uiFGS-1][uiMaxTLevel][iCheckPicNum] );
            dDistortionEnhDep  += xLog( aaaauiDistortionDep[uiLayer][uiFGS  ][0          ][iCheckPicNum] );

            dDistortionBaseInd += xLog( aaaauiDistortionInd[uiLayer][uiFGS-1][0          ][iCheckPicNum] );
            dDistortionEnhInd  += xLog( aaaauiDistortionInd[uiLayer][uiFGS  ][0          ][iCheckPicNum] );
          }
          //---- following level 1 picture -----
          if( ! bLastBaseRep )
          {
            dDistortionBaseDep += xLog( aaaauiDistortionDep[uiLayer][uiFGS-1][uiMaxTLevel][uiTopLayerPicNum+uiTopLayerStepSize2] ) / 2;
            dDistortionEnhDep  += xLog( aaaauiDistortionDep[uiLayer][uiFGS  ][0          ][uiTopLayerPicNum+uiTopLayerStepSize2] ) / 2;

            dDistortionBaseInd += xLog( aaaauiDistortionInd[uiLayer][uiFGS-1][0          ][uiTopLayerPicNum+uiTopLayerStepSize2] ) / 2;
            dDistortionEnhInd  += xLog( aaaauiDistortionInd[uiLayer][uiFGS  ][0          ][uiTopLayerPicNum+uiTopLayerStepSize2] ) / 2;
          }

          m_aaadDeltaDist[uiLayer][uiFGS][puiPic2FNum[uiPicNum]] = 0;
          m_aaadDeltaDist[uiLayer][uiFGS][puiPic2FNum[uiPicNum]]+= ( bDep ? dDistortionBaseDep - dDistortionEnhDep : 0 );
          m_aaadDeltaDist[uiLayer][uiFGS][puiPic2FNum[uiPicNum]]+= ( bInd ? dDistortionBaseInd - dDistortionEnhInd : 0 );
        }
      }

      //----- non-key pictures -----
      UInt uiTemporalScaleFactor = m_auiGOPSize[uiTopLayer] / m_auiGOPSize[uiLayer];

      for( uiTLevel = 1; uiTLevel <= m_auiNumTempLevel[uiLayer]; uiTLevel++ )
      {
        UInt uiStepSize2   = ( 1 << ( m_auiNumTempLevel[uiLayer] - uiTLevel ) );
        UInt uiTopLayerStepSize2   = ( 1 << ( m_auiNumTempLevel[uiTopLayer] - uiTLevel ) );

        for( UInt uiPicNum = uiStepSize2; uiPicNum < m_auiNumFrames[uiLayer]; uiPicNum += (uiStepSize2<<1) )
        {
          UInt uiTopLayerPicNum = uiPicNum * uiTemporalScaleFactor;

          Double dDistortionBaseDep  = 0;
          Double dDistortionBaseInd  = 0;
          Double dDistortionEnhDep   = 0;
          Double dDistortionEnhInd   = 0;
          UInt    uiStartPicNum   = uiTopLayerPicNum - uiTopLayerStepSize2 + 1;
          UInt    uiEndPicNum     = gMin( uiTopLayerPicNum + uiTopLayerStepSize2, m_auiNumFrames[uiTopLayer] ) - 1;
          for( UInt uiCheckPicNum = uiStartPicNum; uiCheckPicNum <= uiEndPicNum; uiCheckPicNum++ )
          {
            dDistortionBaseDep += xLog( aaaauiDistortionDep[uiLayer][uiFGS  ][uiTLevel-1][uiCheckPicNum] );
            dDistortionEnhDep  += xLog( aaaauiDistortionDep[uiLayer][uiFGS  ][uiTLevel  ][uiCheckPicNum] );

            dDistortionBaseInd += xLog( aaaauiDistortionInd[uiLayer][uiFGS-1][uiTLevel  ][uiCheckPicNum] );
            dDistortionEnhInd  += xLog( aaaauiDistortionInd[uiLayer][uiFGS  ][uiTLevel  ][uiCheckPicNum] );
          }
          m_aaadDeltaDist[uiLayer][uiFGS][puiPic2FNum[uiPicNum]] = 0;
          m_aaadDeltaDist[uiLayer][uiFGS][puiPic2FNum[uiPicNum]]+= ( bDep ? dDistortionBaseDep - dDistortionEnhDep : 0 );
          m_aaadDeltaDist[uiLayer][uiFGS][puiPic2FNum[uiPicNum]]+= ( bInd ? dDistortionBaseInd - dDistortionEnhInd : 0 );
        }
      }
    }
  }
  printf("\n");


  //----- delete temporarily distortion arrays -----
  for( uiLayer  = 0;  uiLayer   <  m_uiNumLayers;              uiLayer  ++ )
  for( uiFGS    = 0;  uiFGS     <= m_auiNumFGSLayers[uiLayer]; uiFGS    ++ )
  for( uiTLevel = 0;  uiTLevel  <= m_auiNumTempLevel[uiLayer]; uiTLevel ++ )
  {
    delete [] aaaauiDistortionDep[uiLayer][uiFGS][uiTLevel];
    delete [] aaaauiDistortionInd[uiLayer][uiFGS][uiTLevel];
  }

  return Err::m_nOK;
}



ErrVal
QualityLevelAssigner::xInitRateValues()
{
  printf( "determine packet sizes ..." );

  Int64             i64StartPos     = 0;
  BinData*          pcBinData       = 0;
  SEI::SEIMessage*  pcScalableSEI   = 0;
  UInt              uiLastPrefixTL  = MSYS_UINT_MAX;
  PacketDescription cPacketDescription;
  UInt              auiFrameNum[MAX_LAYERS] = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX };

  //===== init =====
  RNOK( m_pcH264AVCPacketAnalyzer->init() );
  ReadBitstreamFile* pcReadBitStream = 0;
  RNOK( ReadBitstreamFile::create( pcReadBitStream ) );
  RNOK( pcReadBitStream->init( m_pcParameter->getInputBitStreamName() ) );


  //===== init values ======
  for( UInt uiLayer = 0; uiLayer <  m_uiNumLayers;               uiLayer ++ )
  for( UInt uiFGS   = 0; uiFGS   <= m_auiNumFGSLayers[uiLayer];  uiFGS   ++ )
  for( UInt uiFrame = 0; uiFrame <  m_auiNumFrames   [uiLayer];  uiFrame ++ )
  {
    m_aaauiPacketSize     [uiLayer][uiFGS][uiFrame] = 0;
    if( uiFGS == 0 )
    {
      m_aauiPicNumToFrmID [uiLayer]       [uiFrame] = MSYS_UINT_MAX;
    }
  }

  //===== loop over packets =====
  while( true )
  {
    //----- read packet -----
    Bool bEOS = false;
    RNOK( pcReadBitStream->extractPacket( pcBinData, bEOS ) );
    if( bEOS )
    {
      //manu.mathew@samsung : memory leak fix
      RNOK( pcReadBitStream->releasePacket( pcBinData ) );
      pcBinData = NULL;
      //--
      break;
    }

    //----- get packet description -----
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSEI ) );
    delete pcScalableSEI; pcScalableSEI = 0;

    //----- get packet size -----
    Int64 i64EndPos     = pcReadBitStream->getFilePos();
    UInt  uiPacketSize  = (UInt)( i64EndPos - i64StartPos );
    i64StartPos         = i64EndPos;

    //----- analyse packets -----
    if( ! cPacketDescription.ParameterSet && cPacketDescription.NalUnitType != NAL_UNIT_SEI )
    {
      if( cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_IDR || cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE )
      {
        if( uiLastPrefixTL == MSYS_UINT_MAX )
        {
          fprintf( stderr, "\nMissing prefix NAL unit\n\n" );
          ROT(1);
        }
        cPacketDescription.Level = uiLastPrefixTL;
      }
      if( cPacketDescription.NalUnitType == 14 )
      {
        RNOK( pcReadBitStream->releasePacket( pcBinData ) );
        uiLastPrefixTL = cPacketDescription.Level;
        continue;
      }
      else
      {
        uiLastPrefixTL = MSYS_UINT_MAX;
      }

      if( cPacketDescription.FGSLayer == 0 && cPacketDescription.uiFirstMb == 0 )
      {
        auiFrameNum[cPacketDescription.Layer]++;
        {
          Bool  bSet        = false;
          UInt  uiTXLevel   = m_auiNumTempLevel[cPacketDescription.Layer] - cPacketDescription.Level;
          UInt  uiOffset    = ( 1 << uiTXLevel );
          UInt  uiStepSize  = ( uiOffset  << 1 );
          if( cPacketDescription.Level == 0 )
          {
            uiStepSize  = uiOffset;
            uiOffset    = 0;
          }
          for( UInt uiPicNum = uiOffset; uiPicNum < m_auiNumFrames[cPacketDescription.Layer] && !bSet; uiPicNum += uiStepSize )
          {
            if( m_aauiPicNumToFrmID[cPacketDescription.Layer][ uiPicNum ] == MSYS_UINT_MAX )
            {
              m_aauiPicNumToFrmID[cPacketDescription.Layer][ uiPicNum ] = auiFrameNum[cPacketDescription.Layer];
              bSet = true;
            }
          }
          ROF( bSet );
        }
      }
      m_aaauiPacketSize[cPacketDescription.Layer][cPacketDescription.FGSLayer][auiFrameNum[cPacketDescription.Layer]] += uiPacketSize;
    }
    else
    {
      uiLastPrefixTL = MSYS_UINT_MAX;
    }

    //----- delete bin data -----
    RNOK( pcReadBitStream->releasePacket( pcBinData ) );
  }


  //===== uninit =====
  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );
  RNOK( pcReadBitStream->uninit() );
  RNOK( pcReadBitStream->destroy() );

  printf("\n");
  return Err::m_nOK;
}



ErrVal
QualityLevelAssigner::xGetNextValidPacket( BinData*&          rpcBinData,
                                           ReadBitstreamFile* pcReadBitStream,
                                           UInt               uiTopLayer,
                                           UInt               uiLayer,
                                           UInt               uiFGSLayer,
                                           UInt               uiLevel,
                                           Bool               bIndependent,
                                           Bool&              rbEOS,
                                           UInt*              auiFrameNum )
{
  Bool              bValid          = false;
  SEI::SEIMessage*  pcScalableSEI   = 0;
  static UInt       uiLastPrefixTL  = MSYS_UINT_MAX;
  PacketDescription cPacketDescription;

  while( !bValid )
  {
    //===== get next packet =====
    RNOK( pcReadBitStream->extractPacket( rpcBinData, rbEOS ) );
    if( rbEOS )
    {
      break;
    }

    //===== analyze packet =====
    RNOK( m_pcH264AVCPacketAnalyzer->process( rpcBinData, cPacketDescription, pcScalableSEI ) );
    delete pcScalableSEI; pcScalableSEI = 0;


    //===== check whether packet is required =====
    if( cPacketDescription.NalUnitType == NAL_UNIT_SEI )
    {
      uiLastPrefixTL  = MSYS_UINT_MAX;
      bValid          = true;
    }
		else if( cPacketDescription.NalUnitType == NAL_UNIT_SPS )
    {
      uiLastPrefixTL  = MSYS_UINT_MAX;
      bValid          = false;
      for( UInt ui = 0; ui <= uiTopLayer; ui++ )
      {
        if( m_auiSPSRequired[cPacketDescription.SPSid] & (1<<ui) )
        {
          bValid  = true;
          break;
        }
      }
    }
    else if( cPacketDescription.NalUnitType == NAL_UNIT_SUBSET_SPS )
    {
      uiLastPrefixTL  = MSYS_UINT_MAX;
      bValid          = false;
      for( UInt ui = 0; ui <= uiTopLayer; ui++ )
      {
        if( m_auiSubsetSPSRequired[cPacketDescription.SPSid] & (1<<ui) )
        {
          bValid  = true;
          break;
        }
      }
    }
    else if( cPacketDescription.NalUnitType == NAL_UNIT_PPS )
    {
      uiLastPrefixTL  = MSYS_UINT_MAX;
      bValid          = false;
      for( UInt ui = 0; ui <= uiTopLayer; ui++ )
      {
        if( m_auiPPSRequired[cPacketDescription.PPSid] & (1<<ui) )
        {
          bValid  = true;
          break;
        }
      }
    }
    else // slice data
    {
      if( cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_IDR || cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE )
      {
        if( uiLastPrefixTL == MSYS_UINT_MAX )
        {
          fprintf( stderr, "\nMissing prefix NAL unit\n\n" );
          ROT(1);
        }
        cPacketDescription.Level = uiLastPrefixTL;
      }
      if( cPacketDescription.NalUnitType == 14 )
      {
        uiLastPrefixTL  = cPacketDescription.Level;
        bValid          = true;
        continue;
      }
      else
      {
        uiLastPrefixTL  = MSYS_UINT_MAX;
      }
      //===== update frame num =====
      if( ! cPacketDescription.FGSLayer && cPacketDescription.uiFirstMb == 0 )
      {
        auiFrameNum[cPacketDescription.Layer]++;
      }

      //===== get valid status =====
      //JVT-S043
      //For cPacketDescription.Layer > uiLayer,  only Base Quality Level(Discrete layer) is selected!
      //For cPacketDescription.Layer <= uiLayer, the algorithm selects the required FGS layers and Temporal Levels.
      //--
      if( bIndependent )
      {
        bValid      = ( cPacketDescription.Layer    <= uiLayer
                      //JVT-S043
                      || ( cPacketDescription.Layer  <= uiTopLayer && cPacketDescription.FGSLayer == 0 )
                      );
        if( cPacketDescription.Layer == uiLayer )
        {
          bValid    = ( cPacketDescription.Level    == uiLevel &&
                        cPacketDescription.FGSLayer <= uiFGSLayer ) || ( cPacketDescription.FGSLayer == 0 );
        }
      }
      else
      {
        bValid      = ( cPacketDescription.Layer <= uiLayer
                      //JVT-S043
                      || ( cPacketDescription.Layer  <= uiTopLayer && cPacketDescription.FGSLayer == 0 )
                      );
        if( cPacketDescription.Layer == uiLayer )
        {
          bValid    = ( cPacketDescription.FGSLayer <= uiFGSLayer );
          if( cPacketDescription.FGSLayer == uiFGSLayer )
          {
            bValid  = ( cPacketDescription.Level <= uiLevel );
          }
        }
      }

    }

    if( !bValid )
    {
      RNOK( pcReadBitStream->releasePacket( rpcBinData ) );
    }
  }

  return Err::m_nOK;
}

ErrVal
QualityLevelAssigner::xGetDistortion( UInt&         ruiDistortion,
                                      const UChar*  pucReconstruction,
                                      const UChar*  pucReference,
                                      UInt          uiHeight,
                                      UInt          uiWidth,
                                      UInt          uiStride )
{
  ruiDistortion = 0;
  for( UInt y = 0; y < uiHeight; y++ )
  {
    for( UInt x = 0; x < uiWidth; x++ )
    {
      Int iDiff      = ( pucReconstruction[x] - pucReference[x] );
      ruiDistortion += (UInt)( iDiff * iDiff );
    }
    pucReconstruction += uiStride;
    pucReference      += uiStride;
  }
  return Err::m_nOK;
}





ErrVal
QualityLevelAssigner::xInitDistortion( UInt*  auiDistortion,
                                       UInt   uiTopLayer,
                                       UInt   uiLayer,
                                       UInt   uiFGSLayer,
                                       UInt   uiLevel,
                                       Bool   bIndependent )
{
  ROT( m_pcParameter->getOriginalFileName( uiTopLayer ).empty() );

  if( uiLevel == MSYS_UINT_MAX )
    printf( "determine distortion (layer %d - FGS %d - base layer  ) ...", uiLayer, uiFGSLayer );
  else
    printf( "determine distortion (layer %d - FGS %d - lev%2d - %s ) ...", uiLayer, uiFGSLayer, uiLevel, bIndependent?"ind":"dep" );

#if WIN32
  Char              tmp_file_name[]   = "decout.tmp";
#endif
  UInt              uiFrame           = 0;
  UInt              uiMbX             = 0;
  UInt              uiMbY             = 0;
  UInt              uiSize            = 0;
  UInt              auiCropping[4];

  UInt              uiLumOffset       = 0;
  UInt              uiCbOffset        = 0;
  UInt              uiCrOffset        = 0;
  UInt              uiLumWidth        = 0;
  UInt              uiLumHeight       = 0;
  UInt              uiLumStride       = 0;
  PicBuffer*        pcPicBuffer       = 0;
  PicBuffer*        pcPicBufferOrig   = 0;
  WriteYuvToFile*   pcWriteYuv        = 0;
  PicBufferList     cPicBufferOutputList;
  PicBufferList     cPicBufferUnusedList;
  UInt              auiFrameNumAnalysis[MAX_LAYERS] = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX };

  //===== init =====
  RNOK( m_pcH264AVCPacketAnalyzer->init() );
  RNOK( m_pcH264AVCDecoder       ->init( true ) );
  ReadBitstreamFile*  pcReadBitStream = 0;
  ReadYuvFile*        pcReadYuv       = 0;
  RNOK( ReadBitstreamFile ::create( pcReadBitStream ) );
  RNOK( ReadYuvFile       ::create( pcReadYuv       ) );
  RNOK( pcReadBitStream ->init( m_pcParameter->getInputBitStreamName() ) );
  RNOK( pcReadYuv       ->init( m_pcParameter->getOriginalFileName  ( uiTopLayer ), m_auiFrameHeight[uiTopLayer], m_auiFrameWidth[uiTopLayer] ) );

  if( m_bOutputReconstructions )
  {
    Char  acName[1024];
    sprintf( acName, "rec_Layer%d_FGS%d_Level%d_Mode%d", uiLayer, uiFGSLayer, uiLevel, (bIndependent?0:1) );
    RNOK( WriteYuvToFile::create( pcWriteYuv ) );
    RNOK( pcWriteYuv->init( std::string( acName ) ) );
  }

  //-----------------------------------------------------------
  //=====                                                 =====
  //=====   L O O P   O V E R   A C C E S S   U N I T S   =====
  //=====                                                 =====
  //-----------------------------------------------------------
  Bool              bFirstAccessUnit = true;
  h264::AccessUnit  cAccessUnit;
  while( ! cAccessUnit.isEndOfStream() )
  {
    //===== read next access unit =====
    while( ! cAccessUnit.isComplete() )
    {
      BinData*  pcBinData = 0;
      Bool      bEOS      = false; // dummy
      RNOK( xGetNextValidPacket( pcBinData, pcReadBitStream, uiTopLayer, uiLayer, uiFGSLayer, uiLevel, bIndependent, bEOS, auiFrameNumAnalysis ) );
      RNOK( m_pcH264AVCDecoder->initNALUnit ( pcBinData, cAccessUnit ) );
      RNOK( pcReadBitStream->releasePacket  ( pcBinData ) );
    }

    //===== set dimensions =====
    if( bFirstAccessUnit )
    {
      const h264::SliceDataNALUnit* pcSliceDataNalUnit = cAccessUnit.getLastVCLNalUnit();
      ROF( pcSliceDataNalUnit );
      uiMbX             = pcSliceDataNalUnit->getFrameWidthInMb  ();
      uiMbY             = pcSliceDataNalUnit->getFrameHeightInMb ();
      UInt uiChromaSize = ( ( uiMbX << 3 ) + YUV_X_MARGIN     ) * ( ( uiMbY << 3 ) + YUV_Y_MARGIN );
      uiLumOffset       = ( ( uiMbX << 4 ) + YUV_X_MARGIN * 2 ) * YUV_Y_MARGIN     + YUV_X_MARGIN;
      uiCbOffset        = ( ( uiMbX << 3 ) + YUV_X_MARGIN     ) * YUV_Y_MARGIN / 2 + YUV_X_MARGIN / 2 + uiChromaSize * 4;
      uiCrOffset        = ( ( uiMbX << 3 ) + YUV_X_MARGIN     ) * YUV_Y_MARGIN / 2 + YUV_X_MARGIN / 2 + uiChromaSize * 5;
      uiLumHeight       =   ( uiMbY << 4 );
      uiLumWidth        =   ( uiMbX << 4 );
      uiLumStride       =   ( uiMbX << 4 ) + YUV_X_MARGIN * 2;
      uiSize            = 6 * uiChromaSize;
      auiCropping[0]    = pcSliceDataNalUnit->getCroppingRectangle ()[0];
      auiCropping[1]    = pcSliceDataNalUnit->getCroppingRectangle ()[1];
      auiCropping[2]    = pcSliceDataNalUnit->getCroppingRectangle ()[2];
      auiCropping[3]    = pcSliceDataNalUnit->getCroppingRectangle ()[3];
      bFirstAccessUnit  = false;
      RNOK( xGetNewPicBuffer( pcPicBufferOrig, uiSize ) );
    }

    //===== process access unit =====
    while( cAccessUnit.isComplete() )
    {
      //----- decode NAL unit -----
      BinDataList     cBinDataList; // dummy
      h264::NALUnit*  pcNalUnit  = 0;
      pcPicBuffer                = 0;
      RNOK( cAccessUnit.getAndRemoveNextNalUnit( pcNalUnit ) );
      RNOK( xGetNewPicBuffer( pcPicBuffer, uiSize ) );
#if WIN32 // for linux, this has to be slightly re-formulated
      // re-direct stdout
      Int   orig_stdout     = _dup(1);
      FILE* stdout_copy     = freopen( tmp_file_name, "wt", stdout );
#endif
      RNOK( m_pcH264AVCDecoder->processNALUnit( pcPicBuffer, cPicBufferOutputList, cPicBufferUnusedList, cBinDataList, *pcNalUnit ) );
#if WIN32 // for linux, this have to be slightly re-formulated
      // restore stdout
      fclose( stdout );
      _dup2( orig_stdout, 1 );
      _iob[1] = *fdopen( 1, "wt" );
      fclose(  fdopen( orig_stdout, "w" ) );
#endif

      //----- determine distortion (and output for debugging) -----
      while( ! cPicBufferOutputList.empty() )
      {
        PicBuffer* pcPicBufferTmp = cPicBufferOutputList.popFront();
        if( pcPicBufferTmp )
        {
          //----- output (for debugging) -----
          if( pcWriteYuv )
          {
            RNOK( pcWriteYuv->writeFrame( *pcPicBufferTmp + uiLumOffset,
                                          *pcPicBufferTmp + uiCbOffset,
                                          *pcPicBufferTmp + uiCrOffset,
                                          uiLumHeight, uiLumWidth, uiLumStride, auiCropping ) );
          }
          //----- read in reference picture ------
          RNOK( pcReadYuv->readFrame    ( *pcPicBufferOrig + uiLumOffset,
                                          *pcPicBufferOrig + uiCbOffset,
                                          *pcPicBufferOrig + uiCrOffset,
                                          uiLumHeight, uiLumWidth, uiLumStride ) );

          //----- get distortion -----
          RNOK( xGetDistortion          ( auiDistortion[uiFrame],
                                          *pcPicBufferTmp  + uiLumOffset,
                                          *pcPicBufferOrig + uiLumOffset,
                                          uiLumHeight, uiLumWidth, uiLumStride ) );

          //----- increment output picture number -----
          uiFrame++;
          if( uiLevel == MSYS_UINT_MAX )
            printf( "\rdetermine distortion (layer %d - FGS %d - base layer  ) --> frame %d completed", uiLayer, uiFGSLayer,                                    uiFrame );
          else
            printf( "\rdetermine distortion (layer %d - FGS %d - lev%2d - %s ) --> frame %d completed", uiLayer, uiFGSLayer, uiLevel, bIndependent?"ind":"dep", uiFrame );
        }
      }

      //----- free buffers and delete slice data NAL units -----
      RNOK( xRemovePicBuffer( cPicBufferUnusedList  ) );
      delete pcNalUnit;
    }
  }


  //----- remove original pic buffer -----
  PicBufferList cPicBufferListOrig; cPicBufferListOrig.push_back( pcPicBufferOrig );
  RNOK( xRemovePicBuffer( cPicBufferListOrig ) );
#if WIN32
  remove( tmp_file_name );
#endif


  //===== uninit =====
  RNOK( m_pcH264AVCPacketAnalyzer ->uninit  () );
  RNOK( m_pcH264AVCDecoder        ->uninit  ( true ) );
  RNOK( pcReadBitStream           ->uninit  () );
  RNOK( pcReadYuv                 ->uninit  () );
  RNOK( pcReadBitStream           ->destroy () );
  RNOK( pcReadYuv                 ->destroy () );
  if( pcWriteYuv )
  {
    RNOK( pcWriteYuv->destroy() );
  }

  //---- re-create decoder (there's something wrong) -----
  RNOK( m_pcH264AVCDecoder->destroy() );
  RNOK( CreaterH264AVCDecoder::create( m_pcH264AVCDecoder ) );

  printf("\n");

  return Err::m_nOK;
}


ErrVal
QualityLevelAssigner::xWriteDataFile( const std::string&  cFileName )
{
  printf( "write data to file \"%s\" ...", cFileName.c_str() );

  FILE* pFile = fopen( cFileName.c_str(), "wt" );
  if( !pFile )
  {
    fprintf( stderr, "\nERROR: Cannot open file \"%s\" for writing!\n\n", cFileName.c_str() );
    return Err::m_nERR;
  }

  for( UInt uiLayer = 0; uiLayer <  m_uiNumLayers;               uiLayer ++ )
  for( UInt uiFGS   = 0; uiFGS   <= m_auiNumFGSLayers[uiLayer];  uiFGS   ++ )
  for( UInt uiFrame = 0; uiFrame <  m_auiNumFrames   [uiLayer];  uiFrame ++ )
  {
    fprintf( pFile,
             "%d  %d  %5d  %6d  %lf\n",
              uiLayer, uiFGS, uiFrame,
              m_aaauiPacketSize [uiLayer][uiFGS][uiFrame],
              m_aaadDeltaDist   [uiLayer][uiFGS][uiFrame] );

  }

  fclose( pFile );

  printf("\n");
  return Err::m_nOK;
}


ErrVal
QualityLevelAssigner::xReadDataFile( const std::string&  cFileName )
{
  printf( "read data from file \"%s\" ...", cFileName.c_str() );

  FILE* pFile = fopen( cFileName.c_str(), "rt" );
  if( !pFile )
  {
    fprintf( stderr, "\nERROR: Cannot open file \"%s\" for reading!\n\n", cFileName.c_str() );
    return Err::m_nERR;
  }

  Bool    bEOS    = false;
  UInt    uiLayer, uiFGS, uiFrame, uiPacketSize, uiNumPackets;
  Double  dDeltaDist;
  for( uiNumPackets = 0; !bEOS; uiNumPackets++ )
  {
    Int iNumRead = fscanf( pFile,
                           " %d %d %d %d %lf",
                           &uiLayer, &uiFGS, &uiFrame, &uiPacketSize, &dDeltaDist );
    if( iNumRead == 5 )
    {
      ROF( uiLayer <  m_uiNumLayers );
      ROF( uiFGS   <= m_auiNumFGSLayers[uiLayer] );
      ROF( uiFrame <  m_auiNumFrames   [uiLayer] );
      m_aaauiPacketSize [uiLayer][uiFGS][uiFrame]  = uiPacketSize;
      m_aaadDeltaDist   [uiLayer][uiFGS][uiFrame]  = dDeltaDist;
    }
    else
    {
      bEOS = true;
      uiNumPackets--;
    }
  }

  //----- check number of elements -----
  UInt uiTargetPackets = 0;
  for( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer++ )
  {
    uiTargetPackets += ( 1 + m_auiNumFGSLayers[uiLayer] ) * m_auiNumFrames[uiLayer];
  }
  if( uiTargetPackets != uiNumPackets )
  {
    fprintf( stderr, "\nERROR: File \"%s\" contains incomplete data!\n\n", cFileName.c_str() );
    return Err::m_nERR;
  }

  fclose( pFile );

  printf("\n");
  return Err::m_nOK;
}



ErrVal
QualityLevelAssigner::xDetermineQualityIDs()
{
  printf( "determine quality levels ..." );

  //===== determine minimum and maximum quality level id's =====
  UInt  auiMinQualityLevel[MAX_LAYERS];
  UInt  auiMaxQualityLevel[MAX_LAYERS];
  {
    for( Int iLayer = (Int)m_uiNumLayers-1; iLayer >= 0; iLayer-- )
    {
      UInt  uiMinQLLayer          = ( iLayer == (Int)m_uiNumLayers-1 ? 0 : auiMaxQualityLevel[iLayer+1]+1 );
      UInt  uiNumQLInLayer        = ( 63 - uiMinQLLayer ) / ( iLayer + 1 );
      auiMinQualityLevel[iLayer]  = uiMinQLLayer;
      auiMaxQualityLevel[iLayer]  = uiMinQLLayer + uiNumQLInLayer - 1;
    }
  }

  //===== determine optimized quality levels per layer =====
  for( UInt uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer++ )
  {
    //----- create quality estimation object -----
    QualityLevelEstimation  cQualityLevelEstimation;
    RNOK( cQualityLevelEstimation.init( m_uiNumLayers, m_auiNumFGSLayers, m_auiNumFrames ) );

    //----- initialize with packets -----
    {
      for( UInt uiFGSLayer = 1; uiFGSLayer <= m_auiNumFGSLayers[uiLayer]; uiFGSLayer++ )
      for( UInt uiFrame    = 0; uiFrame    <  m_auiNumFrames   [uiLayer]; uiFrame   ++ )
      {
        RNOK( cQualityLevelEstimation.addPacket( uiLayer, uiFGSLayer, uiFrame,
                                                 m_aaauiPacketSize [uiLayer][uiFGSLayer][uiFrame],
                                                 m_aaadDeltaDist   [uiLayer][uiFGSLayer][uiFrame] ) );
      }
    }

    //----- determine quality levels -----
    RNOK( cQualityLevelEstimation.optimizeQualityLevel( uiLayer, uiLayer, auiMinQualityLevel[uiLayer], auiMaxQualityLevel[uiLayer] ) );

    //----- assign quality levels -----
    {
      for( UInt uiFGSLayer = 0; uiFGSLayer <= m_auiNumFGSLayers[uiLayer]; uiFGSLayer++ )
      for( UInt uiFrame    = 0; uiFrame    <  m_auiNumFrames   [uiLayer]; uiFrame   ++ )
      {
        m_aaauiQualityID[uiLayer][uiFGSLayer][uiFrame] = ( uiFGSLayer ? cQualityLevelEstimation.getQualityId( uiLayer, uiFGSLayer, uiFrame ) : 63 );
      }
    }
  }

  printf("\n");
  return Err::m_nOK;
}

//JVT-S043
ErrVal
QualityLevelAssigner::xDetermineMultiLayerQualityIDs()
{
  printf( "determine ML quality levels ..." );

  //===== determine minimum and maximum quality level id's =====
  UInt  uiMinQualityLevel = 0;
  UInt  uiMaxQualityLevel = 62;

  UInt uiLayer;

 //----- create quality estimation object -----
  QualityLevelEstimation  cQualityLevelEstimation;
  RNOK( cQualityLevelEstimation.init( m_uiNumLayers, m_auiNumFGSLayers, m_auiNumFrames ) );

  //===== determine optimized quality levels per layer =====
  for( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer++ )
  {
    //----- initialize with packets -----
    {
      for( UInt uiFGSLayer = 1; uiFGSLayer <= m_auiNumFGSLayers[uiLayer]; uiFGSLayer++ )
      for( UInt uiFrame    = 0; uiFrame    <  m_auiNumFrames   [uiLayer]; uiFrame   ++ )
      {
        RNOK( cQualityLevelEstimation.addPacket( uiLayer, uiFGSLayer, uiFrame,
                                                 m_aaauiPacketSize [uiLayer][uiFGSLayer][uiFrame],
                                                 m_aaadDeltaDist   [uiLayer][uiFGSLayer][uiFrame] ) );
      }
    }
  }


  //----- determine quality levels -----
  RNOK( cQualityLevelEstimation.optimizeQualityLevel( m_uiNumLayers-1, 0, uiMinQualityLevel, uiMaxQualityLevel ) );

  for( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer++ )
  {
    //----- assign quality levels -----
    {
      for( UInt uiFGSLayer = 0; uiFGSLayer <= m_auiNumFGSLayers[uiLayer]; uiFGSLayer++ )
      for( UInt uiFrame    = 0; uiFrame    <  m_auiNumFrames   [uiLayer]; uiFrame   ++ )
      {
        m_aaauiQualityID[uiLayer][uiFGSLayer][uiFrame] = ( uiFGSLayer ? cQualityLevelEstimation.getQualityId( uiLayer, uiFGSLayer, uiFrame ) : 63 );
      }
    }
  }

  printf("\n");
  return Err::m_nOK;
}

ErrVal
QualityLevelAssigner::xWriteQualityLayerStreamPID()
{
  printf( "write stream with quality layer (PID) \"%s\" ...", m_pcParameter->getOutputBitStreamName().c_str() );

  BinData*          pcBinData       = 0;
  SEI::SEIMessage*  pcScalableSEI   = 0;
  UInt              uiLastPrefixTL  = MSYS_UINT_MAX;
  PacketDescription cPacketDescription;
  UInt              auiFrameNum[MAX_LAYERS] = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX };
  //===== init =====
  RNOK( m_pcH264AVCPacketAnalyzer->init() );
  ReadBitstreamFile*    pcReadBitStream   = 0;
  WriteBitstreamToFile* pcWriteBitStream  = 0;
  RNOK( ReadBitstreamFile   ::create( pcReadBitStream  ) );
  RNOK( WriteBitstreamToFile::create( pcWriteBitStream ) );
  RNOK( pcReadBitStream ->init( m_pcParameter->getInputBitStreamName  () ) );
  RNOK( pcWriteBitStream->init( m_pcParameter->getOutputBitStreamName () ) );


  //===== loop over packets =====
  while( true )
  {
    //----- read packet -----
    Bool bEOS = false;
    RNOK( pcReadBitStream->extractPacket( pcBinData, bEOS ) );
    if( bEOS )
    {
      //manu.mathew@samsung : memory leak fix
      RNOK( pcReadBitStream ->releasePacket ( pcBinData ) );
      pcBinData = NULL;
      //--
      break;
    }

    //----- get packet description -----
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSEI ) );
    delete pcScalableSEI; pcScalableSEI = 0;

    //----- set packet size -----
    while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
    {
      RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove trailing zeros
    }

    //----- analyse packets -----
    if( cPacketDescription.FGSLayer )
    {
      // JVT-T083: modify the Position of SimplePriorityId
      // clear previous value
      pcBinData->data()[1] &= 0xC0;
      // write new value of priority ID
      pcBinData->data()[1] |= m_aaauiQualityID[cPacketDescription.Layer][cPacketDescription.FGSLayer][auiFrameNum[cPacketDescription.Layer]];
    }
    else
    {
      if( cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_IDR || cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE )
      {
        if( uiLastPrefixTL == MSYS_UINT_MAX )
        {
          fprintf( stderr, "\nMissing prefix NAL unit\n\n" );
          ROT(1);
        }
        cPacketDescription.Level = uiLastPrefixTL;
      }
      if( ! cPacketDescription.ParameterSet && cPacketDescription.NalUnitType != NAL_UNIT_SEI &&
          ! cPacketDescription.FGSLayer  && cPacketDescription.NalUnitType != NAL_UNIT_PREFIX && cPacketDescription.uiFirstMb == 0 )
      {
        auiFrameNum[cPacketDescription.Layer]++;
      }
    }

    if( cPacketDescription.NalUnitType == 14 )
	  {
      uiLastPrefixTL = cPacketDescription.Level;
  	}
    else
    {
      uiLastPrefixTL = MSYS_UINT_MAX;
    }

    //----- write and delete bin data -----
    RNOK( pcWriteBitStream->writePacket   ( &m_cBinDataStartCode ) );
    RNOK( pcWriteBitStream->writePacket   ( pcBinData ) );
    RNOK( pcReadBitStream ->releasePacket ( pcBinData ) );
  }


  //===== uninit =====
  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );
  RNOK( pcReadBitStream ->uninit  () );
  RNOK( pcWriteBitStream->uninit  () );
  RNOK( pcReadBitStream ->destroy () );
  RNOK( pcWriteBitStream->destroy () );

  printf("\n");
  return Err::m_nOK;
}

Void CalMaxBitrate( std::list<Double>& BitsList, UInt uiFrameRate, Double& dMaxBitrate )
{
  if ( BitsList.empty() ) return;
  if ( uiFrameRate == 0 ) return;
  
  std::list<Double>::iterator iter;
  Double dTotalBits = 0;
  if ( BitsList.size() > uiFrameRate )
  {
    BitsList.pop_front();
    for ( iter = BitsList.begin(); iter != BitsList.end(); iter ++ )
    {
      dTotalBits += *iter;
    }
    Double dBitrate = dTotalBits*uiFrameRate/BitsList.size();
    if ( dMaxBitrate < dBitrate )
    {
      dMaxBitrate = dBitrate;
    }
  }
  else
  {
    for ( iter = BitsList.begin(); iter != BitsList.end(); iter ++ )
    {
      dTotalBits += *iter;
    }
    Double dBitrate = dTotalBits*uiFrameRate/BitsList.size();
    dMaxBitrate = dBitrate;
  }
}

ErrVal 
QualityLevelAssigner::xUpdateScalableSEI()
{
  //================================initlization=============================
  UInt    uiPr_num_dId_minus1;
  UInt    auiPr_dependency_id[MAX_LAYERS];
  UInt    auiPr_num_minus1[MAX_LAYERS];
  UInt    aauiPr_id[MAX_LAYERS][MAX_SIZE_PID];
  UInt    aaiPr_profile_level_idc[MAX_LAYERS][MAX_SIZE_PID];
  Double  aadPr_avg_bitrate[MAX_LAYERS][MAX_SIZE_PID];
  Double  aadPr_max_bitrate[MAX_LAYERS][MAX_SIZE_PID];
  uiPr_num_dId_minus1 = 0;
  ::memset( auiPr_dependency_id, 0, MAX_LAYERS*sizeof(UInt) );
  ::memset( auiPr_num_minus1, 0, MAX_LAYERS*sizeof(UInt) );
  ::memset( aauiPr_id, 0, MAX_LAYERS*MAX_SIZE_PID*sizeof(UInt) );
  ::memset( aaiPr_profile_level_idc, 0, MAX_LAYERS*MAX_SIZE_PID*sizeof(UInt) );
  ::memset( aadPr_avg_bitrate, 0, MAX_LAYERS*MAX_SIZE_PID*sizeof(Double) );
  ::memset( aadPr_max_bitrate, 0, MAX_LAYERS*MAX_SIZE_PID*sizeof(Double) );

  //=============================================================================
  UInt uiLayer, uiNumLayers, uiFrameNum, ui, uiPrLayer;
  Bool bPIdExist[MAX_SIZE_PID];
  //==========================reverse the priority level ========================
  for ( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer ++ )
  {
    for ( uiFrameNum = 0; uiFrameNum < m_auiNumFrames[uiLayer]; uiFrameNum ++ )
    {
      for( uiNumLayers = 0; uiNumLayers <= m_auiNumFGSLayers[uiLayer] && m_aaauiQualityID  [uiLayer][uiNumLayers][uiFrameNum] != MSYS_UINT_MAX; uiNumLayers ++ )
      {
        m_aaauiQualityID[uiLayer][uiNumLayers][uiFrameNum] = 63 - m_aaauiQualityID[uiLayer][uiNumLayers][uiFrameNum];
      }
    }
  }

  //============================================================================
  uiPr_num_dId_minus1 = m_uiNumLayers - 1;
  for ( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer ++ )
  {
    auiPr_dependency_id[uiLayer] = uiLayer;
    for ( ui = 0; ui < MAX_SIZE_PID; ui ++ )
    {
      bPIdExist[ui] = false;
    }
    auiPr_num_minus1[uiLayer] = 0;
    for ( uiFrameNum = 0; uiFrameNum < m_auiNumFrames[uiLayer]; uiFrameNum ++ )
    {
      for( uiNumLayers = 0; uiNumLayers <= m_auiNumFGSLayers[uiLayer] && m_aaauiQualityID  [uiLayer][uiNumLayers][uiFrameNum] != MSYS_UINT_MAX; uiNumLayers ++ )
      {
        bPIdExist[m_aaauiQualityID[uiLayer][uiNumLayers][uiFrameNum]] = true;
      }
    }
    for ( ui = 0; ui < MAX_SIZE_PID; ui ++ )
    {
      if ( bPIdExist[ui] )
      {
        auiPr_num_minus1[uiLayer] ++;
      }
    }
    auiPr_num_minus1[uiLayer] --;
    UInt uiPtr = 0;
    for ( ui = 0; ui < MAX_SIZE_PID; ui ++ )
    {
      if ( bPIdExist[ui] )
      {
        aauiPr_id[uiLayer][uiPtr++] = ui;
      }
    }
  }

  //==============================================================================
  for ( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer ++ )
  {
    if ( uiLayer )
    {
      UInt uiBasePrId;
      for ( uiPrLayer = 0; uiPrLayer < MAX_SIZE_PID; uiPrLayer ++ )
      {
        uiBasePrId = MSYS_UINT_MAX;
        for ( ui = auiPr_num_minus1[uiLayer-1]; ui >= 0; ui -- )
        {
          //==========find the first base layer PrId=============
          if ( aauiPr_id[uiLayer-1][ui] <= uiPrLayer )
          {
            uiBasePrId = ui;
            break;
          }
        }
        if ( uiBasePrId != MSYS_UINT_MAX && m_aauiPrFrameRate[uiLayer][uiPrLayer] < m_aauiPrFrameRate[uiLayer-1][aauiPr_id[uiLayer-1][uiBasePrId]] )
        {
          m_aauiPrFrameRate[uiLayer][uiPrLayer] = m_aauiPrFrameRate[uiLayer-1][aauiPr_id[uiLayer-1][uiBasePrId]];
        }
      }
    }
    for ( ui = 0; ui < MAX_SIZE_PID; ui ++ )
    {
      if ( ui )
      {
        if ( m_aauiPrFrameRate[uiLayer][ui] < m_aauiPrFrameRate[uiLayer][ui-1] )
        {
          m_aauiPrFrameRate[uiLayer][ui] = m_aauiPrFrameRate[uiLayer][ui-1];
        }
      }
    }
  }

  //==============================calculate bitrate===============================
  UInt aauiPrAUNum[MAX_LAYERS][MAX_SIZE_PID];
  ::memset( aauiPrAUNum, 0, MAX_LAYERS*MAX_SIZE_PID*sizeof(UInt) );

  Double **aaadPrPacketSize[MAX_LAYERS];
  ::memset( aaadPrPacketSize, 0, MAX_LAYERS*sizeof(Double**) );

  std::list<Double> PrBitsList[MAX_LAYERS][MAX_SIZE_PID];

  for ( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer ++ )
  {
    aaadPrPacketSize[uiLayer] = new Double*[m_auiNumFrames[uiLayer]];
    ::memset( aaadPrPacketSize[uiLayer], 0, m_auiNumFrames[uiLayer]*sizeof(Double*) );
    for ( uiFrameNum = 0; uiFrameNum < m_auiNumFrames[uiLayer]; uiFrameNum ++ )
    {
      aaadPrPacketSize[uiLayer][uiFrameNum] = new Double[MAX_SIZE_PID];
      ::memset( aaadPrPacketSize[uiLayer][uiFrameNum], 0, MAX_SIZE_PID*sizeof(Double) ); 
      if ( uiLayer )
      {
        UInt uiBasePrId;
        for ( uiPrLayer = 0; uiPrLayer < MAX_SIZE_PID; uiPrLayer ++ )
        {
          uiBasePrId = MSYS_UINT_MAX;
          for ( ui = auiPr_num_minus1[uiLayer-1]; ui >= 0; ui -- )
          {
            //==========find the first base layer PrId=============
            if ( aauiPr_id[uiLayer-1][ui] <= uiPrLayer )
            {
              uiBasePrId = ui;
              break;
            }
          }
          if ( uiBasePrId != MSYS_UINT_MAX && m_aauiBaseIndex[uiLayer][uiFrameNum] != MSYS_UINT_MAX )
          {
            aaadPrPacketSize[uiLayer][uiFrameNum][uiPrLayer] += aaadPrPacketSize[uiLayer-1][m_aauiBaseIndex[uiLayer][uiFrameNum]][aauiPr_id[uiLayer-1][uiBasePrId]];
          }
        }
      }
      for( uiNumLayers = 0; uiNumLayers <= m_auiNumFGSLayers[uiLayer] && m_aaauiQualityID  [uiLayer][uiNumLayers][uiFrameNum] != MSYS_UINT_MAX; uiNumLayers ++ )
      {
        for ( uiPrLayer = m_aaauiQualityID[uiLayer][uiNumLayers][uiFrameNum]; uiPrLayer < MAX_SIZE_PID; uiPrLayer ++ )
        {
          aaadPrPacketSize[uiLayer][uiFrameNum][uiPrLayer] += (Double)m_aaauiNewPacketSize[uiLayer][uiNumLayers][uiFrameNum]*8;
        }
      }

      for ( uiPrLayer = 0; uiPrLayer <= auiPr_num_minus1[uiLayer]; uiPrLayer ++ )
      {
        UInt uiPrId = aauiPr_id[uiLayer][uiPrLayer];
        aadPr_avg_bitrate[uiLayer][uiPrLayer] += aaadPrPacketSize[uiLayer][uiFrameNum][uiPrId];
        if ( aaadPrPacketSize[uiLayer][uiFrameNum][uiPrId] )
        {
          aauiPrAUNum[uiLayer][uiPrLayer] ++;
          PrBitsList[uiLayer][uiPrLayer].push_back( aaadPrPacketSize[uiLayer][uiFrameNum][uiPrId] );
        }
        CalMaxBitrate( PrBitsList[uiLayer][uiPrLayer], m_aauiPrFrameRate[uiLayer][uiPrId], aadPr_max_bitrate[uiLayer][uiPrLayer] );
      }
    }
  }

  //==================================================================================================
  for ( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer ++ )
  {
    for ( uiPrLayer = 0; uiPrLayer <= auiPr_num_minus1[uiLayer]; uiPrLayer ++ )
    {
      AOF( aauiPrAUNum[uiLayer][uiPrLayer] );
      aadPr_avg_bitrate[uiLayer][uiPrLayer] *= m_aauiPrFrameRate[uiLayer][aauiPr_id[uiLayer][uiPrLayer]];
      aadPr_avg_bitrate[uiLayer][uiPrLayer] /= aauiPrAUNum[uiLayer][uiPrLayer];
    }
  }

  //===============================resume the priority level ===========================================
  for ( uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer ++ )
  {
    for ( uiFrameNum = 0; uiFrameNum < m_auiNumFrames[uiLayer]; uiFrameNum ++ )
    {
      for( uiNumLayers = 0; uiNumLayers <= m_auiNumFGSLayers[uiLayer] && m_aaauiQualityID  [uiLayer][uiNumLayers][uiFrameNum] != MSYS_UINT_MAX; uiNumLayers ++ )
      {
        m_aaauiQualityID[uiLayer][uiNumLayers][uiFrameNum] = 63 - m_aaauiQualityID[uiLayer][uiNumLayers][uiFrameNum];
      }
    }
  }

  //=========================================free memory================================================
  for ( uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer ++ )
  {
    if ( m_aauiBaseIndex[uiLayer] )
    {
      delete[] m_aauiBaseIndex[uiLayer];
      m_aauiBaseIndex[uiLayer] = NULL;
    }

    if ( aaadPrPacketSize[uiLayer] )
    {
      for ( uiFrameNum = 0; uiFrameNum < m_auiNumFrames[uiLayer]; uiFrameNum ++ )
      {
        if ( aaadPrPacketSize[uiLayer][uiFrameNum] )
        {
          delete[] aaadPrPacketSize[uiLayer][uiFrameNum];
          aaadPrPacketSize[uiLayer][uiFrameNum] = NULL;
        }
      }
      delete[] aaadPrPacketSize[uiLayer];
      aaadPrPacketSize[uiLayer] = NULL;
    }
  }

  //=====================================================================================================
  BinData*          pcBinData;
  SEI::SEIMessage*  pcScalableSEI     = 0;
  PacketDescription cPacketDescription;
  //===== init =====
  RNOK( m_pcH264AVCPacketAnalyzer->init() );
  ReadBitstreamFile*    pcReadBitStream   = 0;
  WriteBitstreamToFile* pcWriteBitStream  = 0;
  RNOK( ReadBitstreamFile   ::create( pcReadBitStream  ) );
  RNOK( WriteBitstreamToFile::create( pcWriteBitStream ) );
  std::string tempfile(m_pcParameter->getOutputBitStreamName  ());
  tempfile += ".temp";
  RNOK( pcReadBitStream ->init( m_pcParameter->getOutputBitStreamName  () ) );
  RNOK( pcWriteBitStream->init( tempfile ) );

  //===== loop over packets =====
  while( true )
  {
    //----- read packet -----
    Bool bEOS = false;
    RNOK( pcReadBitStream->extractPacket( pcBinData, bEOS ) );
    if( bEOS )
    {
      //manu.mathew@samsung : memory leak fix
      RNOK( pcReadBitStream ->releasePacket ( pcBinData ) );
      pcBinData = NULL;
      //--
      break;
    }

    //----- get packet description -----
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSEI ) );
    if ( pcScalableSEI && pcScalableSEI->getMessageType() == SEI::SCALABLE_SEI )
    {
      //=============================reset the priority layer information=================================
      SEI::ScalableSei* pcSSEI = (SEI::ScalableSei*)pcScalableSEI;
      pcSSEI->setPriorityLayerInfoPresentFlag( true );
      pcSSEI->setPrNumdIdMinus1( uiPr_num_dId_minus1 );
      for ( UInt i = 0; i <= pcSSEI->getPrNumdIdMinus1(); i ++ )
      {
        pcSSEI->setPrDependencyId( i, auiPr_dependency_id[i] );
        pcSSEI->setPrNumMinus1( i, auiPr_num_minus1[i] );
        for ( UInt j = 0; j <= pcSSEI->getPrNumMinus1(i); j ++ )
        {
          pcSSEI->setPrId( i, j, aauiPr_id[i][j] );
          pcSSEI->setPrProfileLevelIdx( i, j, aaiPr_profile_level_idc[i][j] );
          pcSSEI->setPrAvgBitrateBPS( i, j, (Double)aadPr_avg_bitrate[i][j] );
          pcSSEI->setPrMaxBitrateBPS( i, j, (Double)aadPr_max_bitrate[i][j] );
        }
      }

      const UInt          uiPRSEIMessageBufferSize = 4096;
      UChar               aucPRSEIMessageBuffer[uiPRSEIMessageBufferSize];
      BinData             cBinData;
      ExtBinDataAccessor  cExtBinDataAccessor;
      cBinData.reset      ();
      cBinData.set        ( aucPRSEIMessageBuffer, uiPRSEIMessageBufferSize );
      cBinData.setMemAccessor ( cExtBinDataAccessor );

      SEI::MessageList      cSEIMessageList;
      UInt uiBits = 0; 
      cSEIMessageList.push_back( pcScalableSEI );
      RNOK( m_pcNalUnitEncoder->initNalUnit ( &cExtBinDataAccessor ) );
      RNOK( m_pcNalUnitEncoder->write       ( cSEIMessageList ) );
      RNOK( m_pcNalUnitEncoder->closeNalUnit( uiBits ) );
      RNOK( pcWriteBitStream->writePacket( &m_cBinDataStartCode ) );
      RNOK( pcWriteBitStream->writePacket( &cExtBinDataAccessor ) );
      cBinData.reset();
      RNOK( pcReadBitStream->releasePacket( pcBinData ) );
      pcBinData = NULL;
    }
    else
    {
      RNOK( pcWriteBitStream->writePacket( &m_cBinDataStartCode ) );
      RNOK( pcWriteBitStream->writePacket( pcBinData ) );
      RNOK( pcReadBitStream->releasePacket( pcBinData ) );
      pcBinData = NULL;
    }
  }
  if ( pcScalableSEI )
  {
    delete pcScalableSEI;
    pcScalableSEI = 0;
  }


  //===== uninit =====
  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );
  RNOK( pcReadBitStream ->uninit  () );
  RNOK( pcWriteBitStream->uninit  () );
  RNOK( pcReadBitStream ->destroy () );
  RNOK( pcWriteBitStream->destroy () );

  RNOK( remove( m_pcParameter->getOutputBitStreamName  ().c_str() ) );
  RNOK( rename( tempfile.c_str(), m_pcParameter->getOutputBitStreamName  ().c_str() ) );

  return Err::m_nOK;
}


ErrVal
QualityLevelAssigner::xWriteQualityLayerStreamSEI()
{
  printf( "write stream with quality layer (SEI) \"%s\" ...", m_pcParameter->getOutputBitStreamName().c_str() );

  UInt              uiNumAccessUnits  = 0;
  UInt              uiPrevLayer       = 0;
  UInt              uiPrevLevel       = 0;
  UInt              uiLastPrefixTL    = MSYS_UINT_MAX;
  BinData*          pcBinData         = 0;
  SEI::SEIMessage*  pcScalableSEI     = 0;
  PacketDescription cPacketDescription;
  UInt              auiFrameNum[MAX_LAYERS] = { MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX, MSYS_UINT_MAX };
  //===== init =====
  RNOK( m_pcH264AVCPacketAnalyzer->init() );
  ReadBitstreamFile*    pcReadBitStream   = 0;
  WriteBitstreamToFile* pcWriteBitStream  = 0;
  RNOK( ReadBitstreamFile   ::create( pcReadBitStream  ) );
  RNOK( WriteBitstreamToFile::create( pcWriteBitStream ) );
  RNOK( pcReadBitStream ->init( m_pcParameter->getInputBitStreamName  () ) );
  RNOK( pcWriteBitStream->init( m_pcParameter->getOutputBitStreamName () ) );

  Bool bPrevNALUnitWasPrefix = false; //JVT-W137
  UInt auiFrameRate[MAX_SCALABLE_LAYERS];
  ::memset( auiFrameRate, 0x00, MAX_SCALABLE_LAYERS*sizeof(UInt) );
  UInt aaauiDTQToScalableId[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  ::memset( aaauiDTQToScalableId, 0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt) );
  
  for ( UInt uiLayer = 0; uiLayer < m_uiNumLayers; uiLayer ++ )
  {
    m_aauiBaseIndex[uiLayer] = new UInt[m_auiNumFrames[uiLayer]];
    ::memset( m_aauiBaseIndex[uiLayer], MSYS_UINT_MAX, m_auiNumFrames[uiLayer]*sizeof(UInt) );
    for ( UInt uiFrameNum = 0; uiFrameNum < m_auiNumFrames[uiLayer]; uiFrameNum ++ )
    {
      for( UInt uiNumLayers = 0; uiNumLayers <= m_auiNumFGSLayers[uiLayer] && m_aaauiQualityID  [uiLayer][uiNumLayers][uiFrameNum] != MSYS_UINT_MAX; uiNumLayers ++ )
      {
        m_aaauiNewPacketSize[uiLayer][uiNumLayers][uiFrameNum] = m_aaauiPacketSize[uiLayer][uiNumLayers][uiFrameNum];
      }
    }
  }
  UInt uiPreD, uiPreT, uiPreQ;
  UInt uiCurD, uiCurT, uiCurQ;
  uiPreD = uiPreT = uiPreQ = MSYS_UINT_MAX;
  uiCurD = uiCurT = uiCurQ = MSYS_UINT_MAX;

  //===== loop over packets =====
  while( true )
  {
    //----- read packet -----
    Bool bEOS = false;
    RNOK( pcReadBitStream->extractPacket( pcBinData, bEOS ) );
    if( bEOS )
    {
      //manu.mathew@samsung : memory leak fix
      RNOK( pcReadBitStream ->releasePacket ( pcBinData ) );
      pcBinData = NULL;
      //--
      break;
    }

    //----- get packet description -----
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSEI ) );
    if ( pcScalableSEI )
    {
      if ( pcScalableSEI->getMessageType() == SEI::SCALABLE_SEI )
      {
        SEI::ScalableSei* pcSSEI = (SEI::ScalableSei*)pcScalableSEI;
        for ( UInt uiLayer = 0; uiLayer <= pcSSEI->getNumLayersMinus1(); uiLayer ++ )
        {
          if ( pcSSEI->getFrmRateInfoPresentFlag(uiLayer) )
          {
            auiFrameRate[uiLayer] = pcSSEI->getAvgFrmRate(uiLayer)/256;
          }
          UInt uiD, uiT, uiQ;
          uiD = pcSSEI->getDependencyId( uiLayer );
          uiT = pcSSEI->getTemporalId( uiLayer );
          uiQ = pcSSEI->getQualityId( uiLayer );
          aaauiDTQToScalableId[uiD][uiT][uiQ] = uiLayer;
        }
      }
    }
    delete pcScalableSEI; pcScalableSEI = 0;

    //----- set packet size -----
    while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
    {
      RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove trailing zeros
    }

    //----- detect first slice data of access unit -----
    Bool bNewAccessUnit = ( !cPacketDescription.ParameterSet                &&
                            !cPacketDescription.ApplyToNext                 &&
                            cPacketDescription.NalUnitType != NAL_UNIT_SEI &&
                            cPacketDescription.FGSLayer    == 0U );
    if(cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_IDR || cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE)
    {
      if( uiLastPrefixTL == MSYS_UINT_MAX )
      {
        fprintf( stderr, "\nMissing prefix NAL unit\n\n" );
        ROT(1);
      }
      cPacketDescription.Level = uiLastPrefixTL;
      bNewAccessUnit = bNewAccessUnit && (!bPrevNALUnitWasPrefix); //JVT-W137
      if(bPrevNALUnitWasPrefix)
        bPrevNALUnitWasPrefix = false; //JVT-W137
    }
    if( cPacketDescription.NalUnitType == 14 )
    {
      uiLastPrefixTL  = cPacketDescription.Level;
      bPrevNALUnitWasPrefix = true;
    }
    else
    {
      uiLastPrefixTL  = MSYS_UINT_MAX;
    }
    if(  bNewAccessUnit )
    {
      bNewAccessUnit  =                   ( cPacketDescription.Layer == 0 );
      bNewAccessUnit  = bNewAccessUnit || ( cPacketDescription.Layer >= uiPrevLayer && cPacketDescription.Level >  uiPrevLevel );
      bNewAccessUnit  = bNewAccessUnit || ( cPacketDescription.Layer == uiPrevLayer && cPacketDescription.Level == uiPrevLevel );
      bNewAccessUnit  = bNewAccessUnit || ( cPacketDescription.Layer <  uiPrevLayer );
    }

    //----- insert quality layer SEI and increase frame number -----
    if( bNewAccessUnit )
    {
      for( UInt uiLayer = cPacketDescription.Layer; uiLayer < m_uiNumLayers; uiLayer++ )
      {
        auiFrameNum[uiLayer]++;
        xInsertPriorityLevelSEI( pcWriteBitStream, uiLayer, auiFrameNum[uiLayer] );//SEI changes update
        m_aaauiNewPacketSize[uiLayer][0][auiFrameNum[uiLayer]] += m_uiPriorityLevelSEISize/8;
      }
      uiNumAccessUnits++;
    }

    if ( cPacketDescription.NalUnitType == NAL_UNIT_SPS || cPacketDescription.NalUnitType == NAL_UNIT_PPS ||
       cPacketDescription.NalUnitType == NAL_UNIT_SEI || cPacketDescription.NalUnitType == NAL_UNIT_SUBSET_SPS)
    {
      m_aaauiNewPacketSize[0][0][0] += pcBinData->size();
    }
    if ( cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE ||
      cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE || cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_IDR )
    {
      UInt uiLayer, uiLevel, uiFGSLayer, uiFrameNum, uiScalableId;
      uiLayer = cPacketDescription.Layer;
      uiLevel = cPacketDescription.Level;
      uiFGSLayer = cPacketDescription.FGSLayer;
      uiFrameNum = auiFrameNum[uiLayer];
      uiScalableId = aaauiDTQToScalableId[uiLayer][uiLevel][uiFGSLayer];
      if ( auiFrameRate[uiScalableId] > m_aauiPrFrameRate[uiLayer][63-m_aaauiQualityID[uiLayer][uiFGSLayer][auiFrameNum[uiLayer]]] )
      {
        m_aauiPrFrameRate[uiLayer][63-m_aaauiQualityID[uiLayer][uiFGSLayer][auiFrameNum[uiLayer]]] = auiFrameRate[uiScalableId];
      }
      //=====================base layer relationship=====================
      uiPreD = uiCurD; uiPreT = uiCurT; uiPreQ = uiCurQ;
      uiCurD = uiLayer;uiCurT = uiLevel;uiCurQ = uiFGSLayer;
      if ( m_uiNumLayers > 1 )
      {
        if ( uiCurD == (uiPreD + 1) && uiCurT == uiPreT && uiCurQ == 0 )//next D in the same AU
        {
          m_aauiBaseIndex[uiCurD][auiFrameNum[uiCurD]] = auiFrameNum[uiPreD];
        }
      }
    }

    //----- write and delete bin data -----
    RNOK( pcWriteBitStream->writePacket   ( &m_cBinDataStartCode ) );
    RNOK( pcWriteBitStream->writePacket   ( pcBinData ) );
    RNOK( pcReadBitStream ->releasePacket ( pcBinData ) );

    //----- update previous layer information -----
    uiPrevLayer = cPacketDescription.Layer;
    uiPrevLevel = cPacketDescription.Level;
  }


  //===== uninit =====
  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );
  RNOK( pcReadBitStream ->uninit  () );
  RNOK( pcWriteBitStream->uninit  () );
  RNOK( pcReadBitStream ->destroy () );
  RNOK( pcWriteBitStream->destroy () );


  printf(" (%d AU's)\n", uiNumAccessUnits );
  return Err::m_nOK;
}



//SEI changes update {
//ErrVal
//QualityLevelAssigner::xInsertQualityLayerSEI( WriteBitstreamToFile* pcWriteBitStream,
//                                              UInt                  uiLayer,
//                                              UInt                  uiFrameNum )
//{
//  //===== init binary data =====
//  const UInt          uiQLSEIMessageBufferSize = 1024;
//  UChar               aucQLSEIMessageBuffer[uiQLSEIMessageBufferSize];
//  BinData             cBinData;
//  ExtBinDataAccessor  cExtBinDataAccessor;
//  cBinData.reset          ();
//  cBinData.set            ( aucQLSEIMessageBuffer, uiQLSEIMessageBufferSize );
//  cBinData.setMemAccessor ( cExtBinDataAccessor );
//
//  //===== create SEI message =====
//  SEI::QualityLevelSEI* pcQualityLevelSEI;
//  SEI::MessageList      cSEIMessageList;
//  RNOK( SEI::QualityLevelSEI::create( pcQualityLevelSEI ) );
//  cSEIMessageList.push_back( pcQualityLevelSEI );
//
//  //===== set content of SEI message =====
//  UInt uiNumLayers;
//  for( uiNumLayers = 0; uiNumLayers <= m_auiNumFGSLayers[uiLayer] && m_aaauiQualityID  [uiLayer][uiNumLayers][uiFrameNum] != MSYS_UINT_MAX; uiNumLayers++ )
//  {
//    pcQualityLevelSEI->setQualityLevel          ( uiNumLayers,       m_aaauiQualityID  [uiLayer][uiNumLayers][uiFrameNum] );
//    //pcQualityLevelSEI->setDeltaBytesRateOfLevel ( uiNumLayers,       m_aaauiPacketSize [uiLayer][uiNumLayers][uiFrameNum] ); //JVT-W137
//  }
//  pcQualityLevelSEI->setDependencyId            ( uiLayer );
//  pcQualityLevelSEI->setNumLevel                ( uiNumLayers );
//
//  //===== encode SEI message =====
//  UInt uiBits = 0;
//  RNOK( m_pcNalUnitEncoder->initNalUnit ( &cExtBinDataAccessor ) );
//  RNOK( m_pcNalUnitEncoder->write       ( cSEIMessageList ) );
//  RNOK( m_pcNalUnitEncoder->closeNalUnit( uiBits ) );
//
//  //===== write SEI message =====
//  RNOK( pcWriteBitStream->writePacket( &m_cBinDataStartCode ) );
//  RNOK( pcWriteBitStream->writePacket( &cExtBinDataAccessor ) );
//
//  //===== reset =====
//  cBinData.reset();
//
//  return Err::m_nOK;
//}
ErrVal
QualityLevelAssigner::xInsertPriorityLevelSEI( WriteBitstreamToFile* pcWriteBitStream,
                                              UInt                  uiLayer,
                                              UInt                  uiFrameNum )
{
  //===== init binary data =====
  const UInt          uiPRSEIMessageBufferSize = 1024;
  UChar               aucPRSEIMessageBuffer[uiPRSEIMessageBufferSize];
  BinData             cBinData;
  ExtBinDataAccessor  cExtBinDataAccessor;
  cBinData.reset          ();
  cBinData.set            ( aucPRSEIMessageBuffer, uiPRSEIMessageBufferSize );
  cBinData.setMemAccessor ( cExtBinDataAccessor );

  //===== create SEI message =====
  SEI::PriorityLevelSEI* pcPriorityLevelSEI;
  SEI::MessageList      cSEIMessageList;
  RNOK( SEI::PriorityLevelSEI::create( pcPriorityLevelSEI ) );
  cSEIMessageList.push_back( pcPriorityLevelSEI );

  //===== set content of SEI message =====
  UInt uiNumLayers;
  for( uiNumLayers = 0; uiNumLayers <= m_auiNumFGSLayers[uiLayer] && m_aaauiQualityID  [uiLayer][uiNumLayers][uiFrameNum] != MSYS_UINT_MAX; uiNumLayers++ )
  {
		pcPriorityLevelSEI->setAltPriorityId          ( uiNumLayers,       m_aaauiQualityID  [uiLayer][uiNumLayers][uiFrameNum] );
  }
	pcPriorityLevelSEI->setPrDependencyId           ( uiLayer );
	pcPriorityLevelSEI->setNumPriorityIds           ( uiNumLayers );

  //===== encode SEI message =====
  UInt uiBits = 0;
  RNOK( m_pcNalUnitEncoder->initNalUnit ( &cExtBinDataAccessor ) );
  RNOK( m_pcNalUnitEncoder->write       ( cSEIMessageList ) );
  RNOK( m_pcNalUnitEncoder->closeNalUnit( uiBits ) );
  m_uiPriorityLevelSEISize = uiBits;

  //===== write SEI message =====
  RNOK( pcWriteBitStream->writePacket( &m_cBinDataStartCode ) );
  RNOK( pcWriteBitStream->writePacket( &cExtBinDataAccessor ) );

  //===== reset =====
  cBinData.reset();

  return Err::m_nOK;
}
//SEI changes update }

