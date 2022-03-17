
#include "BStreamExtractor.h"
#include "Extractor.h"
#include "ScalableSEIModifyCode.h"
#include <math.h>


#define WARNING(x,t) {if(x) {::printf("\nWARNING: %s\n",t); } }
#define ERROR(x,t)   {if(x) {::printf("\nERROR:   %s\n",t); assert(0); return Err::m_nERR;} }

Extractor::Extractor()
: m_pcReadBitstream       ( 0 )
, m_pcWriteBitstream      ( 0 )
, m_pcExtractorParameter  ( 0 )
//{{Quality level estimation and modified truncation- JVTO044 and m12007
//France Telecom R&D-(nathalie.cammas@francetelecom.com)
, m_bInInputStreamQL      ( false )
//}}Quality level estimation and modified truncation- JVTO044 and m12007
// HS: packet trace
, m_pcTraceFile           ( 0 )
, m_pcExtractionTraceFile ( 0 )
, m_uiMaxSize             ( 0 )
, m_bQualityLevelInSEI    ( false )
//S051{
, m_bUseSIP(false)
,m_uiPreAndSuffixUnitEnable(0)//If we want to use sip with AVC compatiblity, the encoder and extractor should both set this to true
//S051}
{
  //{{Quality level estimation and modified truncation- JVTO044 and m12007
  //France Telecom R&D-(nathalie.cammas@francetelecom.com)
  UInt uiLayer;
  ::memset( m_aaauiScalableLayerId,-1, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  for( UInt uiTempLevel=0; uiTempLevel < MAX_TEMP_LEVELS; uiTempLevel++)
  {
    m_adFrameRate[uiTempLevel] = 0;
  }
  for( uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
  for( UInt uiTempLevel = 0; uiTempLevel < MAX_TEMP_LEVELS; uiTempLevel++ )
  {
    m_aadMinBitrate[uiLayer][uiTempLevel] = 0;
    for( UInt uiQualityLevel = 0; uiQualityLevel < MAX_QUALITY_LEVELS; uiQualityLevel++ )
    {
      m_aaadSingleBitrate[uiLayer][uiTempLevel][uiQualityLevel] = 0;
    }
  }
  }
  for( UInt uiScalableLayer= 0; uiScalableLayer < MAX_SCALABLE_LAYERS; uiScalableLayer++ )
  {
    m_adTotalBitrate[uiScalableLayer] = 0;
    m_auiDependencyId[uiScalableLayer] = 0;
    m_auiTempLevel[uiScalableLayer] = 0;
    m_auiQualityLevel[uiScalableLayer] = 0;
    m_auiDirDepLayerDelta[uiScalableLayer][0] = m_auiDirDepLayerDelta[uiScalableLayer][1] = MSYS_UINT_MAX;
  }

  m_uiTruncateLayer = MSYS_UINT_MAX;
  m_uiTruncateLevel = MSYS_UINT_MAX;
  m_uiTruncateFGSLayer = 1;
}



Extractor::~Extractor()
{
}



ErrVal
Extractor::create( Extractor*& rpcExtractor )
{
  rpcExtractor = new Extractor;
  ROT( NULL == rpcExtractor );
  return Err::m_nOK;
}



ErrVal
Extractor::init( ExtractorParameter *pcExtractorParameter )
{
  ROT( NULL == pcExtractorParameter );

  m_pcExtractorParameter  = pcExtractorParameter;
  m_pcExtractorParameter->setResult( -1 );

  ReadBitstreamFile*  pcReadBitstreamFile;
  RNOKS( ReadBitstreamFile::create( pcReadBitstreamFile ) );
  RNOKS( pcReadBitstreamFile->init( m_pcExtractorParameter->getInFile() ) );
  m_pcReadBitstream = (ReadBitstreamIf*)pcReadBitstreamFile;

  if( !m_pcExtractorParameter->getAnalysisOnly() )
  {
    WriteBitstreamToFile*  pcWriteBitstreamFile;
    RNOKS( WriteBitstreamToFile::create( pcWriteBitstreamFile ) );
    RNOKS( pcWriteBitstreamFile->init( m_pcExtractorParameter->getOutFile() ) );
    m_pcWriteBitstream = (WriteBitstreamIf*)pcWriteBitstreamFile;
  }
  else
  {
    m_pcWriteBitstream = NULL;
  }

  RNOK( h264::H264AVCPacketAnalyzer::create( m_pcH264AVCPacketAnalyzer ) );

  // HS: packet trace
  if( m_pcExtractorParameter->getTraceEnabled() )
  {
    m_pcTraceFile = ::fopen( m_pcExtractorParameter->getTraceFile().c_str(), "wt" );
    if (!m_pcTraceFile)
      fprintf( stderr, "\nCANNOT OPEN TRACE FILE \"%s\"\n\n", m_pcExtractorParameter->getTraceFile().c_str() );
    ROF( m_pcTraceFile );
  }
  else
  {
    m_pcTraceFile = 0;
  }
  if( m_pcExtractorParameter->getExtractTrace() )
  {
    m_pcExtractionTraceFile = ::fopen( m_pcExtractorParameter->getExtractTraceFile().c_str(), "rt" );
    if (!m_pcExtractionTraceFile)
      fprintf( stderr, "\nCANNOT OPEN TRACE FILE \"%s\"\n\n", m_pcExtractorParameter->getExtractTraceFile().c_str() );
    ROF( m_pcExtractionTraceFile );

    RNOK( m_cLargeFile.open( m_pcExtractorParameter->getInFile(), LargeFile::OM_READONLY ) );
  }
  else
  {
    m_pcExtractionTraceFile = 0;
  }


  m_aucStartCodeBuffer[0] = 0;
  m_aucStartCodeBuffer[1] = 0;
  m_aucStartCodeBuffer[2] = 0;
  m_aucStartCodeBuffer[3] = 1;
  m_cBinDataStartCode.reset();
  m_cBinDataStartCode.set( m_aucStartCodeBuffer, 4 );

  m_uiExtractNonRequiredPics = pcExtractorParameter->getExtractNonRequiredPics();

  //S051{
  m_bUseSIP=pcExtractorParameter->getUseSIP();
  m_uiPreAndSuffixUnitEnable=pcExtractorParameter->getPreAndSuffixUnitEnable();
  //S051}

  return Err::m_nOK;
}



ErrVal
Extractor::destroy()
{
  m_cBinDataStartCode.reset();

  if( NULL != m_pcH264AVCPacketAnalyzer )
  {
    RNOK( m_pcH264AVCPacketAnalyzer->destroy() );
  }

  if( NULL != m_pcReadBitstream )
  {
    RNOK( m_pcReadBitstream->uninit() );
    RNOK( m_pcReadBitstream->destroy() );
  }
//TMM_FIX
  if( NULL != m_pcWriteBitstream )
  {
    RNOK( m_pcWriteBitstream->uninit() );
    RNOK( m_pcWriteBitstream->destroy() );
  }
//TMM_FIX
  // HS: packet trace
  if( m_pcTraceFile )
  {
    ::fclose( m_pcTraceFile );
    m_pcTraceFile = 0;
  }
  if( m_pcExtractionTraceFile )
  {
    ::fclose( m_pcExtractionTraceFile );
    m_pcExtractionTraceFile = 0;
  }
  if( m_cLargeFile.is_open() )
  {
    RNOK( m_cLargeFile.close() );
  }

  UInt uiLayer, uiFGSLayer, uiPoint;
  for(uiLayer=0; uiLayer<MAX_LAYERS; uiLayer++)
  {
    delete []m_aaadMaxRate[uiLayer];
    delete []m_aaiLevelForFrame[uiLayer];
    delete []m_aadTargetByteForFrame[uiLayer];
    delete []m_aaiNumLevels[uiLayer];
    for(uiFGSLayer = 0; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++) //bug fix JV 02/11/06
    {
      delete []m_aaadTargetBytesFGS[uiLayer][uiFGSLayer];
      delete []m_aaadBytesForFrameFGS[uiLayer][uiFGSLayer];
    }
    for(uiPoint = 0; uiPoint < MAX_NUM_RD_LEVELS; uiPoint++)
    {
      delete []m_aaauiBytesForQualityLevel[uiLayer][uiPoint];
			delete []m_aaadPriorityLevel[uiLayer][uiPoint];//SEI changes update
    }
  }

  delete this;

  return Err::m_nOK;
}

ErrVal
Extractor::xPrimaryAnalyse()
{
  UInt                    uiLayer       = 0;
  UInt                    uiLevel       = 0;
  UInt                    uiFGSLayer    = 0;
  Bool                    bNewPicture   = false;
  Bool                    bApplyToNext  = false;
  Bool                    bEOS          = false;
  Bool                    bFirstPacket  = true;
  BinData*                pcBinData     = 0;
  h264::SEI::SEIMessage*  pcScalableSei = 0;
  h264::PacketDescription cPacketDescription;
  setBaseLayerAVCCompatible(true); //default value
//prefix unit{{
  Bool bDiscardable_prefix = false;
//prefix unit}}
  //Common information to dead substreams and quality levels
  //arrays initialization
  static UInt auiNumImage[MAX_LAYERS] =
  {
    0,0,0,0,0,0,0,0
  };

  RNOK( m_pcH264AVCPacketAnalyzer->init() );

  while( ! bEOS )
  {
    //===== get packet =====
    RNOK( m_pcReadBitstream->extractPacket( pcBinData, bEOS ) );

    if( bEOS )
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
      continue;
    }

    //===== get packet description =====
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSei ) );
    if(pcScalableSei)
    {
    delete pcScalableSei;
      bFirstPacket = true;
    }
    else
      bFirstPacket = false;
    while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
    {
      RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove zero at end
    }

    //==== get parameters =====
    if( ! bApplyToNext )
    {
//prefix unit{{
      if(cPacketDescription.NalUnitType == 1 || cPacketDescription.NalUnitType == 5 )
      {
        cPacketDescription.Level = uiLevel;
        cPacketDescription.bDiscardable = bDiscardable_prefix;
      }
      bDiscardable_prefix = cPacketDescription.bDiscardable;
//prefix unit}}
      uiLayer     = cPacketDescription.Layer;
      uiLevel     = cPacketDescription.Level;
      uiFGSLayer  = cPacketDescription.FGSLayer;
    }
    bApplyToNext  = cPacketDescription.ApplyToNext;
    bNewPicture   = ( ! cPacketDescription.ParameterSet && ! cPacketDescription.ApplyToNext && !bFirstPacket );

    if((cPacketDescription.NalUnitType == 20 || cPacketDescription.NalUnitType == 21 ) && cPacketDescription.Layer == 0 && cPacketDescription.FGSLayer == 0 && getBaseLayerAVCCompatible())
    {
      bNewPicture = false;
    }
//prefix unit{{
  if( cPacketDescription.NalUnitType == 14 )
    bNewPicture = false;
//prefix unit}}

    //==== update stream description =====
    //{{Quality level estimation and modified truncation- JVTO044 and m12007
    //France Telecom R&D-(nathalie.cammas@francetelecom.com)
    //count number of picture per layer in input stream
    if (bNewPicture && uiFGSLayer == 0 && cPacketDescription.NalUnitType != NAL_UNIT_SEI)
    {
      auiNumImage[uiLayer] ++;
      if( cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE ||
          cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_IDR )
        setBaseLayerAVCCompatible(true);
    }

    if(cPacketDescription.uiNumLevelsQL != 0)
    {
        //QL SEI packet
        bApplyToNext = false;
    }


    RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
    pcBinData = NULL;
  }

  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );

  //----- reset input file -----
  RNOKS( static_cast<ReadBitstreamFile*>(m_pcReadBitstream)->uninit() );
  RNOKS( static_cast<ReadBitstreamFile*>(m_pcReadBitstream)->init  ( m_pcExtractorParameter->getInFile() ) );

  //{{Quality level estimation and modified truncation- JVTO044 and m12007
  //France Telecom R&D-(nathalie.cammas@francetelecom.com)
  //save number of frames per layer
  for(uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++)
  {
    m_auiNbImages[uiLayer] = auiNumImage[uiLayer];
  }

  return Err::m_nOK;
}

Void Extractor::AllocateAndInitializeDatas()
{
  UInt uiLayer, uiFGSLayer, uiPoint, uiFrame;
  for(uiLayer=0; uiLayer<MAX_LAYERS; uiLayer++)
  {
    m_aaadMaxRate           [uiLayer] = new Double[m_auiNbImages[uiLayer]+1];
    m_aaiLevelForFrame      [uiLayer] = new Int   [m_auiNbImages[uiLayer]+1];
    m_aadTargetByteForFrame [uiLayer] = new Double[m_auiNbImages[uiLayer]+1];
    m_aaiNumLevels          [uiLayer] = new Int   [m_auiNbImages[uiLayer]+1];
    for(uiFGSLayer = 0; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++)
    {
      m_aaadTargetBytesFGS  [uiLayer][uiFGSLayer] = new Double[m_auiNbImages[uiLayer]+1];
      m_aaadBytesForFrameFGS[uiLayer][uiFGSLayer] = new Double[m_auiNbImages[uiLayer]+1];
    }
    for(uiPoint = 0; uiPoint < MAX_NUM_RD_LEVELS; uiPoint++)
    {
      m_aaauiBytesForQualityLevel [uiLayer][uiPoint] = new UInt   [m_auiNbImages[uiLayer]+1];
      m_aaadPriorityLevel         [uiLayer][uiPoint] = new Double [m_auiNbImages[uiLayer]+1];//SEI changes update
    }
  }

    for(uiLayer=0; uiLayer<MAX_LAYERS; uiLayer++)
    {
        for(uiFrame=0; uiFrame<=m_auiNbImages[uiLayer]; uiFrame++)
        {
            m_aaadMaxRate           [uiLayer][uiFrame] = 0;
            m_aaiLevelForFrame      [uiLayer][uiFrame] = -1;
            m_aadTargetByteForFrame [uiLayer][uiFrame] = 0;
            m_aaiNumLevels[uiLayer] [uiFrame] = 0;
          for(uiFGSLayer = 0; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++)
            {
                m_aaadTargetBytesFGS[uiLayer][uiFGSLayer][uiFrame] = 0;
                m_aaadBytesForFrameFGS[uiLayer][uiFGSLayer][uiFrame] = 0;
            }
            for(uiPoint = 0; uiPoint < MAX_NUM_RD_LEVELS; uiPoint++)
            {
                m_aaauiBytesForQualityLevel[uiLayer][uiPoint][uiFrame] = 0;
                m_aaadPriorityLevel[uiLayer][uiPoint][uiFrame] = 0;//SEI changes update
            }
        }
    }
}

ErrVal
Extractor::xAnalyse()
{
  UInt                    uiLayer       = 0;
  UInt                    uiLevel       = 0;
  UInt                    uiFGSLayer    = 0;
  Bool                    bNewPicture   = false;
  Bool                    bApplyToNext  = false;
  Bool                    bEOS          = false;
  BinData*                pcBinData     = 0;
  Bool                    bFirstPacket  = true;
  h264::SEI::SEIMessage*  pcScalableSei = 0;
  h264::PacketDescription cPacketDescription;
//prefix unit{{
  Bool bDiscardable_prefix = false;
//prefix unit}}
  //{{Quality level estimation and modified truncation- JVTO044 and m12007
  //France Telecom R&D-(nathalie.cammas@francetelecom.com)

  //Quality levels information
  UInt uiNumFrame;

  UInt uiMaxLayer = 0;
  UInt uiMaxTempLevel = 0;
  h264::SEI::ScalableSei*  pcTmpScalableSei = 0;

  //Common information to dead substreams and quality levels
  //arrays initialization
  static UInt auiNumImage[MAX_LAYERS] =
  {
    0,0,0,0,0,0,0,0
  };
  //}}Quality level estimation and modified truncation- JVTO044 and m12007

  UInt uiPId = 0;

  // HS: packet trace
  Int64                   i64StartPos   = 0;
  Int64                   i64EndPos     = 0;
  m_uiMaxSize                           = 0;

  //========== initialize (scalable SEI message shall be present in first access unit) ===========
  RNOK( m_pcH264AVCPacketAnalyzer->init() );
  while( true )
  {
    //--- get packet ---
    RNOK( m_pcReadBitstream->extractPacket( pcBinData, bEOS ) );

    if( bEOS )
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      return Err::m_nERR;
    }

    //--- analyse packet ---
    RNOK( m_pcH264AVCPacketAnalyzer ->process( pcBinData, cPacketDescription, pcScalableSei ) );
    if( !pcScalableSei )
    {
      if( cPacketDescription.NalUnitType != NAL_UNIT_CODED_SLICE          &&
          cPacketDescription.NalUnitType != NAL_UNIT_CODED_SLICE_IDR      &&
          cPacketDescription.NalUnitType != NAL_UNIT_CODED_SLICE_SCALABLE   )
      {
        RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
        continue;
      }
      printf("No scalability SEI messages found!\nExtractor exit.\n\n ");
      exit( 0 );
    }
    bFirstPacket = false;

    // HS: packet trace
    if( ! cPacketDescription.ApplyToNext )
    {
      i64EndPos     = static_cast<ReadBitstreamFile*>(m_pcReadBitstream)->getFilePos();
      UInt  uiStart = (UInt)( i64StartPos & 0xFFFFFFFF  );
      UInt  uiSize  = (UInt)( i64EndPos   - i64StartPos );
      i64StartPos   = i64EndPos;
      if( m_pcTraceFile )
      {
        ::fprintf( m_pcTraceFile, "Start-Pos.  Length  LId  TId  QId   Packet-Type  Discardable  Truncatable""\n" );
        ::fprintf( m_pcTraceFile, "==========  ======  ===  ===  ===  ============  ===========  ===========""\n" );
        ::fprintf( m_pcTraceFile, "0x%08x"   "%8d"   "%5d""%5d""%5d""  %s"        "         %s""         %s" "\n",
          uiStart,
          uiSize,
          cPacketDescription.Layer,
          cPacketDescription.Level,
          cPacketDescription.FGSLayer,
          "StreamHeader",
          " No", " No" );
      }
      m_uiMaxSize = gMax( m_uiMaxSize, uiSize );
    }

    //--- initialize stream description ----
    RNOK( m_cScalableStreamDescription.init( (h264::SEI::ScalableSei*)pcScalableSei ) );

    m_cScalableStreamDescription.setBaseLayerMode( getBaseLayerAVCCompatible() );
    pcTmpScalableSei    = (h264::SEI::ScalableSei* ) pcScalableSei;
    m_uiScalableNumLayersMinus1 = pcTmpScalableSei->getNumLayersMinus1();
    uiMaxLayer          = pcTmpScalableSei->getDependencyId( m_uiScalableNumLayersMinus1);
    uiMaxTempLevel      = pcTmpScalableSei->getTemporalId(m_uiScalableNumLayersMinus1);
    //SEI changes update {
    //JVT-W046 {
    //for(UInt ui=0; ui <= m_uiScalableNumLayersMinus1; ui++)
    //{
    //  m_bAvc_Layer_Conversion_Flag[ui] = pcTmpScalableSei->getAvcLayerConversionFlag(ui);
    //  if ( m_bAvc_Layer_Conversion_Flag[ui] )
    //  {
    //    m_uiAvc_Conversion_Type_Idc[ui] = pcTmpScalableSei->getAvcConversionTypeIdc(ui);
    //    for ( UInt uiType = 0; uiType < 2; uiType++ )
    //    {
    //      m_bAvc_Info_Flag[ui][uiType] = pcTmpScalableSei->getAvcInfoFlag(ui,uiType);
    //      if(m_bAvc_Info_Flag[ui][uiType])
    //      {
    //        m_uiAvc_Profile_Level_Idc[ui][uiType] = pcTmpScalableSei->getAvcProfileLevelIdc(ui,uiType);
    //        m_dAvc_Avg_Bitrate[ui][uiType] = pcTmpScalableSei->getAvcAvgBitrate(ui,uiType);
    //        m_dAvc_Max_Bitrate[ui][uiType] = pcTmpScalableSei->getAvcMaxBitrate(ui,uiType);
    //      }
    //    }
    //  }
    //}
		for(UInt ui=0; ui <= m_uiScalableNumLayersMinus1; ui++)
    {
      m_bLayer_Conversion_Flag[ui] = pcTmpScalableSei->getLayerConversionFlag(ui);
      if ( m_bLayer_Conversion_Flag[ui] )
      {
        m_uiConversion_Type_Idc[ui] = pcTmpScalableSei->getConversionTypeIdc(ui);
        for ( UInt uiType = 0; uiType < 2; uiType++ )
        {
          m_bRewriting_Info_Flag[ui][uiType] = pcTmpScalableSei->getRewritingInfoFlag(ui,uiType);
          if(m_bRewriting_Info_Flag[ui][uiType])
          {
            m_uiRewriting_Profile_Level_Idc[ui][uiType] = pcTmpScalableSei->getRewritingProfileLevelIdc(ui,uiType);
            m_dRewriting_Avg_Bitrate[ui][uiType] = pcTmpScalableSei->getRewritingAvgBitrateBPS(ui,uiType);
            m_dRewriting_Max_Bitrate[ui][uiType] = pcTmpScalableSei->getRewritingMaxBitrateBPS(ui,uiType);
          }
        }
      }
    }
		//SEI changes update }
    //JVT-W046 }
    if(pcBinData)
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
    }

    xSetROIParameters();

    break;
  }

  while( ! bEOS )
  {
    //===== get packet =====
    RNOK( m_pcReadBitstream->extractPacket( pcBinData, bEOS ) );
    if( bEOS )
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
      continue;
    }

    //===== get packet description =====
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSei ) );

    // *LMH (ADD) ->
    if ( m_pcExtractorParameter->getROIFlag() )
    {
        //-- ROI Extraction ICU/ETRI
        const MyList<ExtractorParameter::Point>&          rcExtList   = m_pcExtractorParameter->getExtractionList();
        ROT( rcExtList.size() != 1 );
        MyList<ExtractorParameter::Point>::const_iterator cIter       = rcExtList.begin ();
        MyList<ExtractorParameter::Point>::const_iterator cEnd        = rcExtList.end   ();
        const ExtractorParameter::Point&                  rcExtPoint  = *cIter;

        if (false == CurNalKeepingNeed(cPacketDescription, rcExtPoint))
        {
          if(pcBinData)
          {
            RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
            pcBinData = NULL;
          }
          continue;
        }
    }
    // *LMH (ADD) <-
    // JVT-S080 LMI {
    delete pcScalableSei;
    pcScalableSei = NULL;
    // JVT-S080 LMI }

    //===== set packet length =====
    while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
    {
      RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove zero at end
    }

    //==== get parameters =====
    UInt  uiPacketSize  = 4 + pcBinData->size();
    if( ! bApplyToNext )
    {
//prefix unit{{
      if(cPacketDescription.NalUnitType == 1 || cPacketDescription.NalUnitType == 5)
      {
        cPacketDescription.Level = uiLevel;
        cPacketDescription.bDiscardable = bDiscardable_prefix;
      }
      bDiscardable_prefix = cPacketDescription.bDiscardable;
//prefix unit}}
      uiLayer     = cPacketDescription.Layer;
      uiLevel     = cPacketDescription.Level;
      uiFGSLayer  = cPacketDescription.FGSLayer;
    }

    bApplyToNext  = cPacketDescription.ApplyToNext;
    bNewPicture   = ( ! cPacketDescription.ParameterSet && ! cPacketDescription.ApplyToNext && !bFirstPacket );
    if((cPacketDescription.NalUnitType == 20 || cPacketDescription.NalUnitType == 21 ) && cPacketDescription.Layer == 0 && cPacketDescription.FGSLayer == 0 && getBaseLayerAVCCompatible())
    {
      bNewPicture = false;
    }
//prefix unit{{
  if( cPacketDescription.NalUnitType == 14 )
    bNewPicture = false;
//prefix unit}}
    uiPId = cPacketDescription.uiPId;

    // HS: packet trace
    if( ! cPacketDescription.ApplyToNext )
    {
      i64EndPos     = static_cast<ReadBitstreamFile*>(m_pcReadBitstream)->getFilePos();
      UInt  uiStart = (UInt)( i64StartPos & 0xFFFFFFFF  );
      UInt  uiSize  = (UInt)( i64EndPos   - i64StartPos );
      i64StartPos   = i64EndPos;
      if( m_pcTraceFile )
      {
        ::fprintf( m_pcTraceFile, "0x%08x"   "%8d"   "%5d""%5d""%5d""  %s"        "         %s""         %s" "\n",
          uiStart,
          uiSize,
          cPacketDescription.Layer,
          cPacketDescription.Level,
          cPacketDescription.FGSLayer,
          cPacketDescription.ParameterSet ? "ParameterSet" : "   SliceData",
          cPacketDescription.ParameterSet || ( cPacketDescription.Level == 0 && cPacketDescription.FGSLayer == 0 )  ? " No" : "Yes",
          cPacketDescription.FGSLayer ? " No" : " No" );
      }
      m_uiMaxSize = gMax( m_uiMaxSize, uiSize );
    }

    //==== update stream description =====
    //{{Quality level estimation and modified truncation- JVTO044 and m12007
    //France Telecom R&D-(nathalie.cammas@francetelecom.com)
    //count number of picture per layer in input stream
    if (bNewPicture && uiFGSLayer == 0 && cPacketDescription.NalUnitType != NAL_UNIT_SEI)
    {
      auiNumImage[uiLayer] ++;
    }
     if(cPacketDescription.ParameterSet || cPacketDescription.NalUnitType == NAL_UNIT_SEI
         || cPacketDescription.NalUnitType == NAL_UNIT_PREFIX ) //prefix unit
     {
      //NonRequired JVT-Q066 (06-04-08){{
      if(m_pcH264AVCPacketAnalyzer->getNonRequiredSeiFlag() == 1 )
      {
        uiLayer = uiMaxLayer;
      }
      //NonRequired JVT-Q066 (06-04-08)}}
      uiNumFrame = auiNumImage[uiLayer];
      if( uiNumFrame > m_auiNbImages[uiLayer] ) // to be sure
      {
        uiNumFrame = m_auiNbImages[uiLayer];
      }
    }
    else
      uiNumFrame = auiNumImage[uiLayer]-1;

    if(cPacketDescription.uiNumLevelsQL != 0)
    {
      //QL SEI packet
      bApplyToNext = false;
    }

    if(m_pcExtractorParameter->getExtractUsingQL() == true)
    {
      m_bInInputStreamQL = true;

      //Saving of Quality Level SEI information
      if(cPacketDescription.uiNumLevelsQL != 0)
      {
        m_bQualityLevelInSEI = true;
        m_aaiNumLevels[uiLayer][uiNumFrame] = cPacketDescription.uiNumLevelsQL;
        for(UInt ui = 0; ui < (UInt)m_aaiNumLevels[uiLayer][uiNumFrame]; ui++)
        {
					m_aaadPriorityLevel[uiLayer][ui][uiNumFrame] = cPacketDescription.auiPriorityLevelPR[ui];//SEI changes update
        }
        bApplyToNext = false;
      }
      else
      {
        if(m_bQualityLevelInSEI == false)
        {
          //SEI changes update {
					if(uiFGSLayer != 0)
						m_aaadPriorityLevel[uiLayer][uiFGSLayer][uiNumFrame] = uiPId;
          else
            m_aaadPriorityLevel[uiLayer][uiFGSLayer][uiNumFrame] = 63;
					//SEI changes update }
        }
      }
    }
    //}}Quality level estimation and modified truncation- JVTO044 and m12007

    //S051{
    if(m_bUseSIP)
    {
      const MyList<ExtractorParameter::Point>&          rcExtList   = m_pcExtractorParameter->getExtractionList();
      ROT( rcExtList.size() != 1 );
      MyList<ExtractorParameter::Point>::const_iterator cIter       = rcExtList.begin ();
      const ExtractorParameter::Point&                  rcExtPoint  = *cIter;
      UInt                                              uiExtLayer  = MSYS_UINT_MAX;
      //----- layer -----
      for( UInt i = 0; i < m_cScalableStreamDescription.getNumberOfLayers(); i++ )
      {
        if( rcExtPoint.uiWidth  < m_cScalableStreamDescription.getFrameWidth (i) ||
          rcExtPoint.uiHeight < m_cScalableStreamDescription.getFrameHeight(i)    )
        {
          break;
        }
        uiExtLayer = i;
      }
      if(!cPacketDescription.bDiscardable||cPacketDescription.Layer>=uiExtLayer)
      {
        RNOK( m_cScalableStreamDescription.addPacket( uiPacketSize, uiLayer, uiLevel, uiFGSLayer, bNewPicture ) );
        RNOK( m_cScalableStreamDescription.addPic( cPacketDescription ) );
      }
      else
      {
        RNOK( m_cScalableStreamDescription.addPacketNoUse( uiPacketSize, uiLayer, uiLevel, uiFGSLayer, bNewPicture ) );
        RNOK( m_cScalableStreamDescription.addPic( cPacketDescription ) );
      }
      //DS_FIX_FT_09_2007
      UInt eNalUnitType = cPacketDescription.NalUnitType;
        if( (!cPacketDescription.bDiscardable || cPacketDescription.Layer == uiExtLayer )&& (eNalUnitType == NAL_UNIT_CODED_SLICE              ||
         eNalUnitType == NAL_UNIT_CODED_SLICE_IDR          ||
        eNalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE     )   )
    {
      if( eNalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE )
        m_cScalableStreamDescription.m_bSubsetSPSRequired[uiLayer][cPacketDescription.SPSid] = true;
      else
        m_cScalableStreamDescription.m_bSPSRequired[uiLayer][cPacketDescription.SPSid] = true;
      m_cScalableStreamDescription.m_bPPSRequired[uiLayer][cPacketDescription.PPSid] = true;
    }
    //~DS_FIX_FT_09_2007
    }
    else
    { //DS_FIX_FT_09_2007
    //S051}

    //JVT-P031
    if(!m_bUseSIP || !cPacketDescription.bDiscardable)
    {
      RNOK( m_cScalableStreamDescription.addPacket( uiPacketSize, uiLayer, uiLevel, uiFGSLayer, bNewPicture ) );
      RNOK( m_cScalableStreamDescription.addPic( cPacketDescription ) );
    }
    //~JVT-P031
    //DS_FIX_FT_09_2007
    UInt eNalUnitType = cPacketDescription.NalUnitType;
        if( eNalUnitType == NAL_UNIT_CODED_SLICE              ||
         eNalUnitType == NAL_UNIT_CODED_SLICE_IDR          ||
        eNalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE        )
    {
      if( eNalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE )
        m_cScalableStreamDescription.m_bSubsetSPSRequired[uiLayer][cPacketDescription.SPSid] = true;
      else
        m_cScalableStreamDescription.m_bSPSRequired[uiLayer][cPacketDescription.SPSid] = true;
      m_cScalableStreamDescription.m_bPPSRequired[uiLayer][cPacketDescription.PPSid] = true;
    }
    } //~DS_FIX_FT_09_2007

    //JVT-P031
    //add packet to calculate maxrate (rate before discardable stream
    if( !cPacketDescription.bDiscardable)
      m_aaadMaxRate[uiLayer][uiNumFrame] += uiPacketSize;

    m_aaauiBytesForQualityLevel[uiLayer][uiFGSLayer][uiNumFrame] += uiPacketSize; //JVT-W137

    //~JVT-P031

    //{{Quality level estimation and modified truncation- JVTO044 and m12007
    //France Telecom R&D-(nathalie.cammas@francetelecom.com)
    addPacket(uiLayer, uiNumFrame,uiFGSLayer,uiPacketSize);
    if(bNewPicture)
      setLevel(uiLayer, uiLevel,uiNumFrame);
    //}}Quality level estimation and modified truncation- JVTO044 and m12007


    if(pcBinData)
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
    }
  }

  RNOK( m_cScalableStreamDescription.analyse() );
  m_uiScalableNumLayersMinus1 =  pcTmpScalableSei->getNumLayersMinus1();
  for( UInt uiScalableLayerId = 0; uiScalableLayerId <= m_uiScalableNumLayersMinus1; uiScalableLayerId++ )
  {
    UInt uiActualLayerId = pcTmpScalableSei->getLayerId(uiScalableLayerId);

    UInt uiDependencyId =  pcTmpScalableSei->getDependencyId( uiScalableLayerId );
    UInt uiQualityLevel =  pcTmpScalableSei->getQualityId(uiScalableLayerId);
    UInt uiTempLevel    =  pcTmpScalableSei->getTemporalId(uiScalableLayerId);
    m_auiDependencyId[uiScalableLayerId] = uiDependencyId;
    m_auiTempLevel   [uiScalableLayerId] = uiTempLevel;
    m_auiQualityLevel[uiScalableLayerId] = uiQualityLevel;
    UInt uiBitrate      =  pcTmpScalableSei->getAvgBitrateBPS(uiScalableLayerId);
    m_auiFrmWidth[uiScalableLayerId] = (pcTmpScalableSei->getFrmWidthInMbsMinus1(uiScalableLayerId)+1) << 4;
    m_auiFrmHeight[uiScalableLayerId] = (pcTmpScalableSei->getFrmHeightInMbsMinus1(uiScalableLayerId)+1) << 4;
    m_adFramerate[uiScalableLayerId] = (pcTmpScalableSei->getAvgFrmRate(uiScalableLayerId))/256.0;
    m_aaauiScalableLayerId[uiDependencyId][uiTempLevel][uiQualityLevel] = uiScalableLayerId;

    m_adTotalBitrate[uiScalableLayerId] = (Double) uiBitrate;

    m_auiScalableLayerIdFromActualLayerId[uiActualLayerId] = uiScalableLayerId;
    if(pcTmpScalableSei->getNumDirectlyDependentLayers(uiScalableLayerId) >= 1)
    {
      UInt uiActualDepLayerId = uiActualLayerId - pcTmpScalableSei->getNumDirectlyDependentLayerIdDeltaMinus1(uiScalableLayerId, 0) - 1;
      UInt uiDepScalableLayerId = getScalableLayerIdFromActualLayerId(uiActualDepLayerId);
      m_auiDirDepLayerDelta[uiScalableLayerId][0] = uiScalableLayerId - uiDepScalableLayerId;
      if(pcTmpScalableSei->getNumDirectlyDependentLayers(uiScalableLayerId) == 2) {
        uiActualDepLayerId = uiActualLayerId - pcTmpScalableSei->getNumDirectlyDependentLayerIdDeltaMinus1(uiScalableLayerId, 1) - 1;
        uiDepScalableLayerId = getScalableLayerIdFromActualLayerId(uiActualDepLayerId);
        m_auiDirDepLayerDelta[uiScalableLayerId][1] = uiScalableLayerId - uiDepScalableLayerId;
      }
    }

    if(!m_adFrameRate[uiTempLevel])
      m_adFrameRate[uiTempLevel] = pcTmpScalableSei->getAvgFrmRate(uiScalableLayerId)/256.0;
  }
  uiMaxLayer          = pcTmpScalableSei->getDependencyId( m_uiScalableNumLayersMinus1);
  uiMaxTempLevel      = pcTmpScalableSei->getTemporalId(m_uiScalableNumLayersMinus1);

  //NS leak fix extractor begin
  if (pcTmpScalableSei)
  {
    pcScalableSei = (h264::SEI::ScalableSei*) pcTmpScalableSei;
    delete pcScalableSei;
    pcTmpScalableSei = NULL;
    pcScalableSei = NULL;
  }
  //NS leak fix extractor end

  //calculate dependent relationship among scalable layers with list
  for( UInt uiSId = 0; uiSId <= m_uiScalableNumLayersMinus1; uiSId++ )
  {
    if( m_auiDirDepLayerDelta[uiSId][0] != MSYS_UINT_MAX ) // with dependent layers
    {
      xMakeDepLayerList( uiSId-m_auiDirDepLayerDelta[uiSId][0], m_acDepLayerList[uiSId] );
    }
    if( m_auiDirDepLayerDelta[uiSId][1] != MSYS_UINT_MAX )
    {
      xMakeDepLayerList( uiSId-m_auiDirDepLayerDelta[uiSId][1], m_acDepLayerList[uiSId] );
    }
  }

  //calculate min_bitrate and single_bitrate for each scalable layer (representation)
  for( UInt uiDependencyId = 0; uiDependencyId <= uiMaxLayer; uiDependencyId++)
  {
    for( UInt uiTempLevel = 0; uiTempLevel <= uiMaxTempLevel; uiTempLevel++)
    {
      for( UInt uiQualityLevel = 0; uiQualityLevel < MAX_QUALITY_LEVELS; uiQualityLevel++)
      {
        UInt uiScalableLayerIdDes = getScalableLayer( uiDependencyId, uiTempLevel, uiQualityLevel );
        if(uiScalableLayerIdDes == MSYS_UINT_MAX) // No such scalable layers
          continue;
        if(m_auiDirDepLayerDelta[uiScalableLayerIdDes][0] == MSYS_UINT_MAX) // no direct dependent layer
        { //usually base layer: D=0,T=0,Q=0
          m_aadMinBitrate[uiDependencyId][uiTempLevel] = m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiQualityLevel] = m_adTotalBitrate[uiScalableLayerIdDes];
        }
        else//with direct dependent layer
        {
          UInt uiScalableLayerIdBas = uiScalableLayerIdDes - m_auiDirDepLayerDelta[uiScalableLayerIdDes][0];
          UInt uiDepLayer           = m_auiDependencyId[uiScalableLayerIdBas];
          UInt uiDepTL              = m_auiTempLevel[uiScalableLayerIdBas];

          if( uiQualityLevel ) //Q!=0
          {
          //ROF(uiScalableLayerIdBas == getScalableLayer( uiDependencyId, uiTempLevel, uiQualityLevel-1 )); // as suggested by Danny Hong
            m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiQualityLevel] =
              m_adTotalBitrate[uiScalableLayerIdDes] - m_adTotalBitrate[uiScalableLayerIdBas];
             if( uiTempLevel && m_auiDirDepLayerDelta[uiScalableLayerIdDes][1] != MSYS_UINT_MAX ) // T>0, Q>0
             {
               //In this case, uiQualityLevel within all TL shall be considered
               for( UInt uiTL = 0; uiTL < uiTempLevel; uiTL++ )
               {
                 Double dFactor = m_cScalableStreamDescription.getNumPictures( uiMaxLayer, uiTL )
                   /m_cScalableStreamDescription.getFrameRate(uiMaxLayer, uiTL )
                   *m_cScalableStreamDescription.getFrameRate(uiMaxLayer, uiTempLevel )
                   /m_cScalableStreamDescription.getNumPictures( uiMaxLayer, uiTempLevel ); //this factor represents the factor of different TL to current TL
                 m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiQualityLevel] -= m_aaadSingleBitrate[uiDependencyId][uiTL][uiQualityLevel] * dFactor;
               }
             }
          }
          else if( uiTempLevel ) // T != 0, Q = 0
          {
            Double dFactor = m_cScalableStreamDescription.getNumPictures( uiMaxLayer, uiDepTL )
              /m_cScalableStreamDescription.getFrameRate(uiMaxLayer, uiDepTL )
              *m_cScalableStreamDescription.getFrameRate(uiMaxLayer, uiTempLevel )
              /m_cScalableStreamDescription.getNumPictures( uiMaxLayer, uiTempLevel ); //this factor represents the factor of different TL to current TL
            // default value: for D=0, or base layer without current TL exist
            m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiQualityLevel] = m_adTotalBitrate[uiScalableLayerIdDes]
              - m_adTotalBitrate[m_aaauiScalableLayerId[uiDepLayer][uiDepTL][uiQualityLevel]] * dFactor;
            m_aadMinBitrate[uiDependencyId][uiTempLevel] = m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiQualityLevel] +
              m_aadMinBitrate[uiDepLayer][uiDepTL] * dFactor;
            if( uiDependencyId && m_auiDirDepLayerDelta[uiScalableLayerIdDes][1] != MSYS_UINT_MAX ) //D>0, T>0, Q=0
            {
              //In this case, pictures with TL=uiTempLevel in base layer exists
              UInt uiTmpScalableLayerIdBas = uiScalableLayerIdDes - m_auiDirDepLayerDelta[uiScalableLayerIdDes][1];
              UInt uiTmpDepLayer = m_auiDependencyId[uiTmpScalableLayerIdBas];
              //bitrate calculation
              //store tmp value for base layer
              m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiQualityLevel] =
                m_adTotalBitrate[uiScalableLayerIdDes] - m_adTotalBitrate[uiTmpScalableLayerIdBas];
              m_aadMinBitrate[uiDependencyId][uiTempLevel] = m_aadMinBitrate[uiTmpDepLayer][uiTempLevel];
              //consider the same layer
              for( UInt uiTL = 0; uiTL < uiTempLevel; uiTL++ )
              {
                dFactor = m_cScalableStreamDescription.getNumPictures( uiMaxLayer, uiTL )
                  /m_cScalableStreamDescription.getFrameRate(uiMaxLayer, uiTL )
                  *m_cScalableStreamDescription.getFrameRate(uiMaxLayer, uiTempLevel )
                  /m_cScalableStreamDescription.getNumPictures( uiMaxLayer, uiTempLevel );
                m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiQualityLevel] -= m_aaadSingleBitrate[uiDependencyId][uiTL][uiQualityLevel] * dFactor;
                m_aadMinBitrate[uiDependencyId][uiTempLevel] += m_aaadSingleBitrate[uiDependencyId][uiTL][uiQualityLevel] * dFactor;
              }
              m_aadMinBitrate[uiDependencyId][uiTempLevel] += m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiQualityLevel];
            }
          }
          else //D!=0, T=0, Q=0
          {
            m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiQualityLevel] =
              m_adTotalBitrate[uiScalableLayerIdDes] - m_adTotalBitrate[uiScalableLayerIdBas];
            m_aadMinBitrate[uiDependencyId][uiTempLevel]    = m_aadMinBitrate[uiDepLayer][uiTempLevel]
              + m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiQualityLevel];
          }
        }
      }
    }
  }

  xOutput( );

  //----- reset input file -----
  RNOKS( static_cast<ReadBitstreamFile*>(m_pcReadBitstream)->uninit() );
  RNOKS( static_cast<ReadBitstreamFile*>(m_pcReadBitstream)->init  ( m_pcExtractorParameter->getInFile() ) );

  //initialize max rate for each frame
  //if dead substream is present for the layer: max rate is equal to max rate before dead substream
  //else max rate is equal to the rate of the frame
  if(m_bInInputStreamQL)
  {
    setQualityLevel();
    UInt uiLayerDeb = 0;
      uiLayerDeb = m_cScalableStreamDescription.getNumberOfLayers()-1;
  for(uiLayer = uiLayerDeb; uiLayer < m_cScalableStreamDescription.getNumberOfLayers();uiLayer++)
  {
    CalculateMaxRate(uiLayer);
  }
  }
  //}}Quality level estimation and modified truncation- JVTO044 and m12007

  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );

  return Err::m_nOK;
}

UInt Extractor::addPIDToTable(UInt uiPID)
{
    //look if uiPID already in table
    UInt ui;
    for(ui=0;ui<m_uiNbPID;ui++)
    {
        if(m_auiPID[ui] == uiPID)
            return 0;
    }

    m_auiPID[m_uiNbPID] = uiPID;
    m_uiNbPID++;
    return 1;
}

ErrVal
Extractor::xSetParameters()
{
  RNOK( xGetExtParameters() );

  UInt   uiLayer, uiLevel, uiFGSLayer;
  UInt   uiExtLayer  = m_pcExtractorParameter->getLayer();
  UInt   uiExtLevel  = m_pcExtractorParameter->getLevel();
//JVT-T054{
  Bool  bQuit = false;
//JVT-T054}

  //=========== clear all ===========
  for( uiLayer = 0; uiLayer <  MAX_LAYERS;  uiLayer++ )
  for( uiLevel = 0; uiLevel <= MAX_DSTAGES; uiLevel++ )
  {
    m_aadTargetSNRLayer[uiLayer][uiLevel] = -1;
    m_aadTargetSNRLayerNoUse[uiLayer][uiLevel] = -1;
  }

  //===== get and set required base layer packets ======
  Double  dRemainingBytes     = m_pcExtractorParameter->getTargetRate();
  RNOK( GetAndCheckBaseLayerPackets( dRemainingBytes ) );
  if( dRemainingBytes < 0 )
    return Err::m_nOK;

  //===== set maximum possible bytes for included layers ======
  for( uiLayer = 0; uiLayer <  uiExtLayer; uiLayer++ )
  {
    for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
    {
      for( uiFGSLayer = 1; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++ )
      {
        Int64 i64NALUBytes = m_cScalableStreamDescription.getNALUBytes( uiLayer, uiLevel, uiFGSLayer );
        if( (Double)i64NALUBytes <= dRemainingBytes )
        {
          dRemainingBytes                      -= (Double)i64NALUBytes;
          m_aadTargetSNRLayer[uiLayer][uiLevel] = (Double)uiFGSLayer;
          m_pcExtractorParameter->setMaxFGSLayerKept(uiFGSLayer);
        }
        else
        {
//JVT-T054{
          dRemainingBytes                      -= (Double)i64NALUBytes;
          m_aadTargetSNRLayer[uiLayer][uiLevel] = (Double)uiFGSLayer;
          m_pcExtractorParameter->setMaxFGSLayerKept(uiFGSLayer);
          bQuit = true;
//JVT-T054}
        }
      }
    }
  }

//JVT-T054{
  if(bQuit)
  {
    m_uiTruncateLayer = uiExtLayer;
    m_uiTruncateLevel = uiExtLevel;
    m_uiTruncateFGSLayer = m_pcExtractorParameter->getMaxFGSLayerKept();

    //Then reset all above scalable layers' bitrate within the same layerId
    for( UInt uiTL = 0; uiTL < MAX_TEMP_LEVELS; uiTL++ )
    for( UInt uiQL = 1; uiQL < MAX_QUALITY_LEVELS; uiQL++ )
      m_aaadSingleBitrate[uiExtLayer][uiTL][uiQL] = 0;

    return Err::m_nOK;
  }
//JVT-T054}

  //===== set FGS layer for current layer =====
  for( uiFGSLayer = 1; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++ )
  {
    Int64 i64FGSLayerBytes = 0;
    for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
    {
      i64FGSLayerBytes += m_cScalableStreamDescription.getNALUBytes( uiExtLayer, uiLevel, uiFGSLayer );
    }
    if( (Double)i64FGSLayerBytes <= dRemainingBytes )
    {
      dRemainingBytes -= (Double)i64FGSLayerBytes;
      for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
      {
        m_aadTargetSNRLayer[uiExtLayer][uiLevel] = (Double)uiFGSLayer;
        m_pcExtractorParameter->setMaxFGSLayerKept(uiFGSLayer);
      }
    }
    else
    {
//JVT-T054{
      m_uiTruncateLayer = uiExtLayer;
      m_uiTruncateLevel = uiExtLevel;
      m_uiTruncateFGSLayer = uiFGSLayer;
      for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
      {
        i64FGSLayerBytes = m_cScalableStreamDescription.getNALUBytes( uiExtLayer, uiLevel, uiFGSLayer );
        if( (Double)i64FGSLayerBytes <= dRemainingBytes )
        {
          dRemainingBytes -= (Double)i64FGSLayerBytes;
          m_aadTargetSNRLayer[uiExtLayer][uiLevel] = (Double)uiFGSLayer;
          m_pcExtractorParameter->setMaxFGSLayerKept(uiFGSLayer);
        }
        else
        {
          UInt uiTL;
          m_uiTruncateLevel = uiLevel;
          //Then reset all above layers' bitrate
          for( uiTL = uiLevel; uiTL < MAX_TEMP_LEVELS; uiTL++ )
            m_aaadSingleBitrate[uiExtLayer][uiTL][uiFGSLayer] = 0;
          for( uiTL = 0; uiTL < MAX_TEMP_LEVELS; uiTL++ )
          for( UInt uiFGS = m_uiTruncateFGSLayer+1; uiFGS < MAX_QUALITY_LEVELS; uiFGS++ )
            m_aaadSingleBitrate[uiExtLayer][uiTL][uiFGS] = 0;
          return Err::m_nOK;
        }
      }
    }
  }

  //===== set maximum possible bytes for no use frames in included layers, for SIP ======
  for( uiLayer = 0; uiLayer <  uiExtLayer; uiLayer++ )
  {
    for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
    {
      for( uiFGSLayer = 0; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++ )
      {
        Int64 i64NALUBytes = m_cScalableStreamDescription.getNALUBytesNoUse( uiLayer, uiLevel, uiFGSLayer );
        if( (Double)i64NALUBytes <= dRemainingBytes )
        {
          dRemainingBytes                      -= (Double)i64NALUBytes;
          m_aadTargetSNRLayerNoUse[uiLayer][uiLevel] = (Double)uiFGSLayer;
        }
        else
        {
          //====== set fractional FGS layer and exit =====
          Double  dFGSLayer = dRemainingBytes / (Double)i64NALUBytes;
          m_aadTargetSNRLayerNoUse[uiLayer][uiLevel] += dFGSLayer;
          return Err::m_nOK;
        }
      }
    }
  }

  m_uiTruncateLayer = uiExtLayer;
  m_uiTruncateLevel = uiExtLevel;
  m_uiTruncateFGSLayer = MAX_QUALITY_LEVELS - 1;
  WARNING( dRemainingBytes>0.0, "Bit-rate overflow for extraction/inclusion point" );

  return Err::m_nOK;
}

ErrVal
Extractor::xReCalculateBr()
{
  UInt   uiLayer, uiTLevel, uiQLevel;

  UInt   uiExtLayer  = m_pcExtractorParameter->getLayer();
  UInt   uiExtLevel  = m_pcExtractorParameter->getLevel();

  for( UInt uiScalableLayer = 0; uiScalableLayer <= m_uiScalableNumLayersMinus1; uiScalableLayer++ )
  {
    uiLayer = m_auiDependencyId[uiScalableLayer];
    uiTLevel = m_auiTempLevel[uiScalableLayer];
    uiQLevel = m_auiQualityLevel[uiScalableLayer];
    if( uiLayer > uiExtLayer || uiTLevel > uiExtLevel || uiQLevel > m_aadTargetSNRLayer[uiLayer][uiTLevel] ) // scalable layer to be discarded
    {
      m_adTotalBitrate[uiScalableLayer] = 0;
    }
    else if( uiLayer == 0 && uiQLevel <= m_aadTargetSNRLayer[uiLayer][uiTLevel] ) // all contained layers remain
    {
      ; // remain unchanged
    }
    else // dependent layer changes, re-calculate
    {
      m_adTotalBitrate[uiScalableLayer]   = m_aaadSingleBitrate[uiLayer][uiTLevel][uiQLevel]; //initialize
      std::list<UInt>::iterator iIter     = m_acDepLayerList[uiScalableLayer].begin();
      std::list<UInt>::iterator iIterEnd  = m_acDepLayerList[uiScalableLayer].end();
      while ( iIter != iIterEnd )
      {
        UInt uiScalableLayerDep = (*iIter);
        UInt uiLayerDep = m_auiDependencyId[uiScalableLayerDep];
        UInt uiLevelDep = m_auiTempLevel[uiScalableLayerDep];
        UInt uiQLDep = m_auiQualityLevel[uiScalableLayerDep];
        Double dFactor = m_cScalableStreamDescription.getNumPictures( uiExtLayer, uiLevelDep )
         /m_cScalableStreamDescription.getFrameRate(uiExtLayer, uiLevelDep )
         *m_cScalableStreamDescription.getFrameRate(uiExtLayer, uiTLevel )
         /m_cScalableStreamDescription.getNumPictures( uiExtLayer, uiTLevel ); //this factor represents the factor of different TL to current TL

        m_adTotalBitrate[uiScalableLayer] += m_aaadSingleBitrate[uiLayerDep][uiLevelDep][uiQLDep] * dFactor;
        iIter++;
      }
    }
  }
  return Err::m_nOK;
}

Void
Extractor::xMakeDepLayerList( UInt uiScalableLayerId, std::list<UInt>& DepLayerList )
{
  //This function add all dependent layersId for "uiScalableLayerId" into list "DepLayerList"
  //First, push the current layer into list
  if( DepLayerList.size()==0 || std::find(DepLayerList.begin(), DepLayerList.end(), uiScalableLayerId) == DepLayerList.end() ) //doesn't exist
    DepLayerList.push_back( uiScalableLayerId );

  if( m_acDepLayerList[uiScalableLayerId].size() == 0 )
    return;
  else
  {
    std::list<UInt>::iterator iIterBgn = m_acDepLayerList[uiScalableLayerId].begin();
    std::list<UInt>::iterator iIterEnd = m_acDepLayerList[uiScalableLayerId].end();
    while( iIterBgn != iIterEnd )
    {
      UInt uiLayerId = (*iIterBgn);
      if( std::find(DepLayerList.begin(), DepLayerList.end(), uiLayerId ) == DepLayerList.end() ) //doesn't exist
        DepLayerList.push_back( uiLayerId );
      xMakeDepLayerList( uiLayerId, DepLayerList );
      iIterBgn++;
    }
  }
}


ErrVal
Extractor::go()
{
  RNOK ( xPrimaryAnalyse());

  AllocateAndInitializeDatas();

  if( m_pcExtractorParameter->minusRSpecified() )
  {
    xExtractMaxRate( m_pcExtractorParameter->getMaximumRate(), m_pcExtractorParameter->getDontTruncQLayer(), m_pcExtractorParameter->minusRPercentageMode() );
    return Err::m_nOK;
  }

  RNOK ( xAnalyse() );
  ROTRS( m_pcExtractorParameter->getAnalysisOnly(), Err::m_nOK );

  if( m_pcExtractionTraceFile ) // HS: packet trace
  {
    RNOK( xExtractTrace() ); // HS: packet trace
  }
  else if( m_pcExtractorParameter->getExtractionList().empty() )
  {
    RNOK( xExtractLayerLevel() );
  }
  else
  {
    //if there is R/D information in the input stream
    //with or without dead substream
    if(m_bInInputStreamQL)
    {
      go_QL();
      return Err::m_nOK;
    }

    //S051{
    if(m_bUseSIP)
    {
      go_SIP();
      return Err::m_nOK;
    }
    //S051}

    //default case: there is no dead substream, nor R/D information
    //in the input stream
    //}}Quality level estimation and modified truncation- JVTO044 and m12007
    RNOK( xSetParameters() );
    RNOK( xExtractPoints() );
  }

  return Err::m_nOK;
}



ErrVal
Extractor::xExtractMaxRate( Double dMaxRate, Bool bDontTruncQLayer, Bool bPercentageMode )
{
  Bool    bQualityLayerPresent  = false;
  UInt    uiLayer               = MSYS_UINT_MAX;
  Double  dFrameRate            = 0.0;
  UInt    uiDiffRate            = 0;
  UInt    auiQLRate[MAX_QLAYERS+2];

  //----- get layer number -----
  for( Int i = MAX_LAYERS - 1; i >= 0; i--)
  {
    if( m_auiNbImages[i] > 0 )
    {
      uiLayer = (UInt)i;
      break;
    }
  }
  ROT( uiLayer == MSYS_UINT_MAX );

  //----- analyze -----
  RNOK( xAnalyse( uiLayer, dFrameRate, bQualityLayerPresent, auiQLRate, bDontTruncQLayer ) );
  ROF ( dFrameRate    >  0 );
  ROF ( auiQLRate[0]  >  0 );

  //----- get parameters -----
  Double  dScale    = 1000.0 / 8.0 * (Double)m_auiNbImages[uiLayer] / dFrameRate;
  UInt    uiTGRate;
  if( bPercentageMode )
  {
    uiTGRate  = auiQLRate[MAX_QLAYERS+1] + (UInt)floor( ( auiQLRate[0] - auiQLRate[MAX_QLAYERS+1] ) * dMaxRate );
  }
  else
  {
    uiTGRate  = (UInt)floor( dScale * dMaxRate );
  }
  Int     iExtPID   = MAX_QLAYERS;
  UInt    uiInLRate = 0;
  if(     uiTGRate  > auiQLRate[MAX_QLAYERS+1] )
  {
    for(  Int iQId  = MAX_QLAYERS; iQId >= 0; iQId-- )
    {
      if( uiTGRate  < auiQLRate[iQId] )
      {
        iExtPID     = iQId;
        uiInLRate   = uiTGRate - auiQLRate[iQId+1];
        break;
      }
      else if( iQId == 0 )
      {
        iExtPID     = -1;
        uiInLRate   = uiTGRate - auiQLRate[0];
      }
    }
  }
  else
  {
    uiDiffRate  = uiTGRate - auiQLRate[MAX_QLAYERS+1];
  }
  if( bDontTruncQLayer )
  {
    uiDiffRate  = uiInLRate;
    uiInLRate   = 0;
  }

  //---- extraction ----
  RNOK( xExtract( uiLayer, bQualityLayerPresent, iExtPID, uiInLRate, bDontTruncQLayer ) );
  UInt  uiOutRate = uiTGRate - uiInLRate - uiDiffRate;


#if 1
  printf("minimum rate = %8.2lf kbit/s\n", (Double)auiQLRate[MAX_QLAYERS+1] / dScale );
  printf("maximum rate = %8.2lf kbit/s\n", (Double)auiQLRate[            0] / dScale );
  printf("\n");
  if( iExtPID < 0 )
  {
    iExtPID = 0;
  }
  printf("   low limit = %8.2lf kbit/s\n", (Double)auiQLRate[    iExtPID+1] / dScale );
  printf("  high limit = %8.2lf kbit/s\n", (Double)auiQLRate[    iExtPID  ] / dScale );
  printf("\n");
  printf(" target rate = %8.2lf kbit/s\n", (Double) uiTGRate                / dScale );
  printf(" output rate = %8.2lf kbit/s\n", (Double)uiOutRate                / dScale );
  printf("\n");
#endif

  return Err::m_nOK;
}


ErrVal
Extractor::xAnalyse( UInt    uiTargetLayer,
                    Double& rdFrameRate,
                    Bool&   rbQualityLayerPresent,
                    UInt    auiQLRate[],
                    Bool    bNoSpecialFirstFrame
                    )
{
  Bool                    bEOS          = false;
  BinData*                pcBinData     = 0;
  UInt                    uiDId         = 0;
  UInt                    uiTId         = 0;
  UInt                    uiQId         = 0;
  UInt                    uiPId         = 0;
  UInt                    uiPacketSize  = 0;
  UInt                    uiNumAVCPics  = 0;
  UInt&                   ruiBLRate     = auiQLRate[MAX_QLAYERS+1];
  UInt&                   ruiFGSAU0Rate = auiQLRate[MAX_QLAYERS  ];
  h264::SEI::SEIMessage*  pcScalableSei = 0;
  h264::PacketDescription cPacketDescription;
  UInt                    aauiTLRate[MAX_TLAYERS][MAX_QUALITY_LEVELS];

  //===== clear arrays =====
  ::memset( auiQLRate,  0x00, sizeof( UInt ) * ( MAX_QLAYERS + 2 ) );
  ::memset( aauiTLRate, 0x00, sizeof( UInt ) * ( MAX_TLAYERS * MAX_QUALITY_LEVELS ) );


  //===== initialize and read scalable SEI message =====
  {
    //----- init -----
    RNOK( m_pcH264AVCPacketAnalyzer ->init          () );
    //----- read first packet, which must contain the scalable SEI -----
    RNOK( m_pcReadBitstream         ->extractPacket ( pcBinData, bEOS ) );
    ROT ( bEOS );
    RNOK( m_pcH264AVCPacketAnalyzer ->process       ( pcBinData, cPacketDescription, pcScalableSei ) );
    ROF ( pcScalableSei );
    //----- get SEI parameters and delete SEI -----
    h264::SEI::ScalableSei* pcSSEI  = static_cast<h264::SEI::ScalableSei*>( pcScalableSei );
    rdFrameRate     = (Double)pcSSEI->getAvgFrmRate( pcSSEI->getNumLayersMinus1() ) / 256.0;
    delete pcScalableSei;
    pcScalableSei   = 0;
    //----- get packet size -----
    while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
    {
      RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove zero at end
    }
    ruiBLRate       = 4 + pcBinData ->size();
    //----- release packet -----
    RNOK( m_pcReadBitstream         ->releasePacket ( pcBinData ) );
    pcBinData       = 0;
  }


  //===== MAIN LOOP OVER PACKETS =====
  while( ! bEOS )
  {
    //===== get packet =====
    RNOK  ( m_pcReadBitstream->extractPacket( pcBinData, bEOS ) );
    if    ( bEOS )
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
      continue;
    }

    //===== get packet description =====
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSei ) );
    ROT ( pcScalableSei );
    ROT ( cPacketDescription.ApplyToNext );


    //===== get packet size =====
    while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
    {
      RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove zero at end
    }


    //==== get parameters =====
    {
      uiPacketSize  = 4 + pcBinData->size();
      uiDId         = cPacketDescription.Layer;
//prefix unit{{
      if(cPacketDescription.NalUnitType != NAL_UNIT_CODED_SLICE && cPacketDescription.NalUnitType != NAL_UNIT_CODED_SLICE_IDR)
        uiTId         = cPacketDescription.Level;
//prefix unit}}
      uiQId         = cPacketDescription.FGSLayer;
      uiPId         = cPacketDescription.uiPId;
    }
    ROT( uiDId > uiTargetLayer );
    ROF( uiPId < MAX_QLAYERS   );

    //===== store data =====
    if( uiDId < uiTargetLayer || uiQId == 0 )
    {
      ruiBLRate                 += uiPacketSize;
    }
    else if( uiQId == 1 && uiNumAVCPics == 1 && ! bNoSpecialFirstFrame )
    {
      ruiFGSAU0Rate             += uiPacketSize;
    }
    else
    {
      aauiTLRate[uiTId][uiQId]  += uiPacketSize;
      auiQLRate [uiPId]         += uiPacketSize;
      rbQualityLayerPresent      = rbQualityLayerPresent || uiPId != 0;
    }

    //===== release packet =====
    if( pcBinData )
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
    }
  } // end of while( ! bEOS )


  //===== get quality level automatically =====
  if( ! rbQualityLayerPresent )
  {
    auiQLRate[0] = 0;
    for( uiTId = 0; uiTId < MAX_TLAYERS;        uiTId++ )
      for( uiQId = 1; uiQId < MAX_QUALITY_LEVELS; uiQId++ )
      {
        auiQLRate[ MAX_QLAYERS - 1 - ( ( uiQId - 1 ) << 3 ) - uiTId ] = aauiTLRate[uiTId][uiQId];
      }
  }


  //===== accumulate rate =====
  for( uiQId = MAX_QLAYERS + 1; uiQId > 0; uiQId-- )
  {
    auiQLRate[uiQId-1] += auiQLRate[uiQId];
  }

  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );

  //===== reset input file =====
  RNOKS( static_cast<ReadBitstreamFile*>( m_pcReadBitstream )->uninit() );
  RNOKS( static_cast<ReadBitstreamFile*>( m_pcReadBitstream )->init  ( m_pcExtractorParameter->getInFile() ) );

  return Err::m_nOK;
}


ErrVal
Extractor::xExtract( UInt    uiTargetLayer,
                    Bool    bQualityLayerPresent,
                    Int     iQualityLayer,
                    UInt&   ruiInLayerRate,
                    Bool    bNoSpecialFirstFrame )
{
  Bool                    bEOS          = false;
  BinData*                pcBinData     = 0;
  UInt                    uiDId         = 0;
  UInt                    uiTId         = 0;
  UInt                    uiQId         = 0;
  UInt                    uiPId         = 0;
  UInt                    uiPacketSize  = 0;
  UInt                    uiNumAVCPics  = 0;
  h264::SEI::SEIMessage*  pcScalableSei = 0;
  h264::PacketDescription cPacketDescription;


  //===== initialize and read scalable SEI message =====
  {
    //----- init -----
    RNOK( m_pcH264AVCPacketAnalyzer ->init          () );
    //----- read first packet, which must contain the scalable SEI -----
    RNOK( m_pcReadBitstream         ->extractPacket ( pcBinData, bEOS ) );
    ROT ( bEOS );
    RNOK( m_pcH264AVCPacketAnalyzer ->process       ( pcBinData, cPacketDescription, pcScalableSei ) );
    ROF ( pcScalableSei );
    //----- delete SEI -----
    delete pcScalableSei;
    pcScalableSei   = 0;
    //----- get packet size -----
    while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
    {
      RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove zero at end
    }
    //----- write and release packet -----
    RNOK( m_pcWriteBitstream        ->writePacket   ( &m_cBinDataStartCode ) );
    RNOK( m_pcWriteBitstream        ->writePacket   ( pcBinData ) );
    RNOK( m_pcReadBitstream         ->releasePacket ( pcBinData ) );
    pcBinData       = 0;
  }


  //===== MAIN LOOP OVER PACKETS =====
  while( ! bEOS )
  {
    //===== get packet =====
    RNOK  ( m_pcReadBitstream->extractPacket( pcBinData, bEOS ) );
    if    ( bEOS )
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
      continue;
    }

    //===== get packet description =====
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSei ) );
    ROT ( pcScalableSei );
    ROT ( cPacketDescription.ApplyToNext );


    //===== get packet size =====
    while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
    {
      RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove zero at end
    }


    //==== get parameters =====
    {
      uiPacketSize  = 4 + pcBinData->size();
      uiDId         = cPacketDescription.Layer;
//prefix unit{{
      if(cPacketDescription.NalUnitType != NAL_UNIT_CODED_SLICE && cPacketDescription.NalUnitType != NAL_UNIT_CODED_SLICE_IDR)
        uiTId         = cPacketDescription.Level;
//prefix unit}}
      uiQId         = cPacketDescription.FGSLayer;
      uiPId         = cPacketDescription.uiPId;

    }
    ROT( uiDId > uiTargetLayer );
    ROF( uiPId < MAX_QLAYERS   );


    //===== determine whether packet is kept =====
    Bool  bKeep = true;
    if(  uiDId == uiTargetLayer && uiQId > 0 )
    {
      //---- get quality layer when not present -----
      if( ! bQualityLayerPresent )
      {
        uiPId   = MAX_QLAYERS - 1 - ( ( uiQId - 1 ) << 3 ) - uiTId;
      }
      if( uiQId == 1 && uiNumAVCPics == 1 && ! bNoSpecialFirstFrame)
      {
        uiPId   = MAX_QLAYERS;
      }
      //---- check whether packet is discarded ----
      if( (Int)uiPId < iQualityLayer )
      {
        bKeep   = false;
      }
      else if( (Int)uiPId == iQualityLayer )
      {
        if( ruiInLayerRate >= uiPacketSize )
        {
          ruiInLayerRate -= uiPacketSize;
        }
        else
        {
          bKeep = false;
        }
      }
    }


    //===== write packet ====
    if( bKeep )
    {
      RNOK( m_pcWriteBitstream->writePacket( &m_cBinDataStartCode ) );
      RNOK( m_pcWriteBitstream->writePacket( pcBinData ) );
    }


    //===== release packet =====
    if( pcBinData )
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
    }
  } // end of while( ! bEOS )


  //===== uninit =====
  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );

  return Err::m_nOK;
}




// Keep ROI NAL ICU/ETRI DS
Int Extractor::CurNalKeepingNeed(h264::PacketDescription cPacketDescription
                 , const ExtractorParameter::Point& rcExtPoint)
{
  Bool bIsDataNal = false;
  if (
    (1 <= cPacketDescription.NalUnitType && cPacketDescription.NalUnitType <= 5)
    || (cPacketDescription.NalUnitType == 20 || cPacketDescription.NalUnitType == 21)
    )
  {
    bIsDataNal    = true;
  }


  Int iExtactionROINum    =   m_pcExtractorParameter->getExtractedNumROI();
  Bool      bROIFlag      =   m_pcExtractorParameter->getROIFlag();

  int keepingNAL = -1;
  if( bROIFlag == true && bIsDataNal == true )
  {
    for(int i=0; i<iExtactionROINum; i++)
    {
      Int SG_ID =-1;
      for(int sg_id=0;sg_id<8;sg_id++)
      {
        if(cPacketDescription.uiFirstMb == m_pcH264AVCPacketAnalyzer->uiaAddrFirstMBofROIs[cPacketDescription.PPSid][sg_id])
        {
          SG_ID=sg_id;
          break;
        }
      } // end for check sg (until 8)

      Int iROI_ID = getROI_ID(cPacketDescription.Layer,SG_ID);
      if(iROI_ID !=-1 &&rcExtPoint.uiROI[i] ==iROI_ID )
      {
        keepingNAL = 1;
      }
      if( keepingNAL == 1 )
        break;
    }  // end for
  }  // end if (bROIFlag == true.. )

  else
    return true;


  return (keepingNAL == 1);
}

ErrVal
Extractor::xExtractPoints()
{
  UInt  uiNumInput    = 0;
  UInt  uiNumKept     = 0;
  Bool  bKeep         = false;
  Bool  bApplyToNext  = false;
  Bool  bEOS          = false;

  UInt  uiLayer       = 0;
  UInt  uiLevel       = 0;
  UInt  uiFGSLayer    = 0;
  UInt  auiHighestRetainedQualityId[MAX_LAYERS];
  UInt  uiHighestRetainedDependencyId = 0;
  UInt  aaauiNumRetainedPerQualityId[MAX_LAYERS][MAX_QUALITY_LEVELS][2];
  ::memset( aaauiNumRetainedPerQualityId, 0, sizeof( aaauiNumRetainedPerQualityId ) );
  ::memset( auiHighestRetainedQualityId,  0, sizeof( auiHighestRetainedQualityId )  );

  UInt  uiPacketSize  = 0;
// JVT-T073 {
  Bool bNextSuffix = false;
// JVT-T073 }
  UInt uiWantedScalableLayer = GetWantedScalableLayer();
  UInt uiWantedLayer         = m_pcExtractorParameter->getLayer();
  UInt uiWantedLevel         = m_pcExtractorParameter->getLevel();
  Double dWantedFGSLayer     = (Double) m_uiTruncateFGSLayer;
  Bool  bNewPicture   = false;
  Bool  bFirstPacket  = true;

  UInt uiCurrFrame = 0; //for both QLAssigner and DeadSubstream

  //QLAssigner parameters
  Double totalPackets = 0;
  //Bool  bQualityLevelSEI  = false;//SEI changes update
	Bool  bPriorityLevelSEI  = false;//SEI changes update
  Double dTot = 0;
  UInt uiSEI = 0;
  UInt uiSEISize = 0;
  UInt uiTotalSEI = 0;
//prefix unit{{
  BinData*  pcBinData_prefix = NULL;
  Bool bDiscardable_prefix = false;
//prefix unit}}

  //DS parameters, count number of frame per layer
  Int currFrame[MAX_LAYERS];
  for(uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++)
  {
    currFrame[uiLayer] = 0;
  }

  RNOK( xReCalculateBr() );

  // consider ROI ICU/ETRI DS
  const MyList<ExtractorParameter::Point>&          rcExtList   = m_pcExtractorParameter->getExtractionList();
  ROT( rcExtList.size() != 1 );
  MyList<ExtractorParameter::Point>::const_iterator cIter       = rcExtList.begin ();
  MyList<ExtractorParameter::Point>::const_iterator cEnd        = rcExtList.end   ();
  const ExtractorParameter::Point&                  rcExtPoint  = *cIter;

  Int    Count = 0;


  printf("\n\n============Extraction Information======");
  printf("\nExtracted spatail layer  : %dx%d",   rcExtPoint.uiWidth, rcExtPoint.uiHeight);
  printf("\nExtracted temporal rate  : %2.0ff/s",   rcExtPoint.dFrameRate);


  RNOK( m_pcH264AVCPacketAnalyzer->init() );

  while( ! bEOS )
  {
    //=========== get packet ===========
    BinData*  pcBinData = NULL;
// JVT-S080 LMI {
    BinData * pcBinDataSEILysNotPreDepChange;
    ROT( NULL == ( pcBinDataSEILysNotPreDepChange = new BinData ) );
    Bool bWriteBinDataSEILysNotPreDepChange = false;
    Bool bWriteBinData = true;
// JVT-S080 LMI }
    RNOK( m_pcReadBitstream->extractPacket( pcBinData, bEOS ) );
    if( bEOS )
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
//TMM_FIX {
      RNOK( m_pcReadBitstream->releasePacket( pcBinDataSEILysNotPreDepChange ) );
      pcBinDataSEILysNotPreDepChange = NULL;
//TMM_FIX }
      continue;
    }
    //========== get packet description ==========
    h264::SEI::SEIMessage*  pcScalableSEIMessage = 0;
    h264::PacketDescription cPacketDescription;
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSEIMessage ) );
//prefix unit{{
  if(cPacketDescription.NalUnitType == 14)
  {
    if(pcBinData_prefix)
      RNOK( m_pcReadBitstream->releasePacket( pcBinData_prefix ) );
    bDiscardable_prefix = cPacketDescription.bDiscardable;
    pcBinData_prefix = pcBinData;
    uiLevel = cPacketDescription.Level;
    RNOK( m_pcReadBitstream->releasePacket( pcBinDataSEILysNotPreDepChange ) );
    pcBinDataSEILysNotPreDepChange = NULL;
    continue;
  }
//prefix unit}}
    if( pcScalableSEIMessage ) //re-write scalability SEI message
    {
      bFirstPacket = true;
// JVT-S080 LMI {
      if( pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI || pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI_LAYERS_NOT_PRESENT || pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI_DEPENDENCY_CHANGE )
      {
        if( pcScalableSEIMessage->getMessageType() != h264::SEI::SCALABLE_SEI )
            bWriteBinData = false;

         RNOK( xChangeScalableSEIMessage( pcBinData, pcBinDataSEILysNotPreDepChange, pcScalableSEIMessage, MSYS_UINT_MAX,//uiKeepScalableLayer,
                                           uiWantedScalableLayer, uiWantedLayer, uiWantedLevel, dWantedFGSLayer , MSYS_UINT_MAX ) );

        if( pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI )
        {
          h264::SEI::ScalableSeiLayersNotPresent* pcScalableSeiLayersNotPresent = ( h264::SEI::ScalableSeiLayersNotPresent*) pcScalableSEIMessage;
          bWriteBinDataSEILysNotPreDepChange = pcScalableSeiLayersNotPresent->getOutputFlag();
        }
        if( pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI_LAYERS_NOT_PRESENT )
        {
          h264::SEI::ScalableSeiLayersNotPresent* pcScalableSeiLayersNotPresent = ( h264::SEI::ScalableSeiLayersNotPresent*) pcScalableSEIMessage;
          bWriteBinDataSEILysNotPreDepChange = pcScalableSeiLayersNotPresent->getOutputFlag();
        }

        if( pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI_DEPENDENCY_CHANGE )
        {
          h264::SEI::ScalableSeiDependencyChange* pcScalableSeiDepChange = ( h264::SEI::ScalableSeiDependencyChange*) pcScalableSEIMessage;
          bWriteBinDataSEILysNotPreDepChange = pcScalableSeiDepChange->getOutputFlag();
        }
// JVT-S080 LMI }
      }
    }
    else
      bFirstPacket = false;
    delete pcScalableSEIMessage;

    // JVT-V068 {
    if ( cPacketDescription.bDiscardableHRDSEI )
    {
      uiNumInput++;
      Count++;
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      RNOK( m_pcReadBitstream->releasePacket( pcBinDataSEILysNotPreDepChange ) );
      pcBinDataSEILysNotPreDepChange = NULL;
      continue;
    }
    // JVT-V068 }

    // consider ROI Extraction ICU/ETRI DS
    if (false == CurNalKeepingNeed(cPacketDescription, rcExtPoint))
    {
      uiNumInput++;
      Count++;
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      RNOK( m_pcReadBitstream->releasePacket( pcBinDataSEILysNotPreDepChange ) );
      pcBinDataSEILysNotPreDepChange = NULL;
      continue;
    }


// JVT-S080 LMI {
    if( bWriteBinData )
    {
  // JVT-S080 LMI }
      //============ get packet size ===========
      while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
      {
        RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove zero at end
      }
      uiPacketSize  = 4 + pcBinData->size();
      //============ set parameters ===========
      if( ! bApplyToNext  )
      {
//prefix unit{{
        if(cPacketDescription.NalUnitType == 1 || cPacketDescription.NalUnitType == 5)
        {
          cPacketDescription.Level = uiLevel;
          cPacketDescription.bDiscardable = bDiscardable_prefix;
        }
//prefix unit}}
        uiLayer    = cPacketDescription.Layer;
        uiLevel    = cPacketDescription.Level;
        uiFGSLayer = cPacketDescription.FGSLayer;
      }
      if( m_bInInputStreamQL )
      {
        uiLayer    = cPacketDescription.Layer;
        uiLevel    = cPacketDescription.Level;
        uiFGSLayer = cPacketDescription.FGSLayer;
      }
#if 0 // debug
      printf("Input NAL %d D %d T %d Q %d \n", cPacketDescription.NalUnitType, uiLayer, uiLevel, uiFGSLayer);
#endif
      bApplyToNext = cPacketDescription.ApplyToNext;
      bNewPicture   = ( ! cPacketDescription.ParameterSet && ! cPacketDescription.ApplyToNext && !bFirstPacket );
      if((cPacketDescription.NalUnitType == 20 || cPacketDescription.NalUnitType == 21 ) && cPacketDescription.Layer == 0 && cPacketDescription.FGSLayer == 0 && getBaseLayerAVCCompatible())
      {
        bNewPicture = false;
      }
//prefix unit{{
      if( cPacketDescription.NalUnitType == 14 )
        bNewPicture = false;
//prefix unit}}
      bKeep = false;
      if( m_bInInputStreamQL  && !(m_pcExtractorParameter->getUseSIP() ) )
      {
        //bQualityLevelSEI = (cPacketDescription.uiNumLevelsQL != 0);//SEI changes update
        bPriorityLevelSEI = (cPacketDescription.uiNumLevelsQL != 0);//SEI changes update
        // update frame number
        if (bNewPicture && uiFGSLayer == 0)
        {
          currFrame[uiLayer]++;
        }
        //UInt uiCurrFrame = 0;
        if(cPacketDescription.ParameterSet || bFirstPacket )
        {
          uiCurrFrame = 0;
        }
        else
        {
          if(!bApplyToNext)
            uiCurrFrame = currFrame[uiLayer]-1;//[uiFGSLayer];
          else
            uiCurrFrame = currFrame[uiLayer];
        }

        //if(bQualityLevelSEI)//SEI changes update
        if(bPriorityLevelSEI)//SEI changes update
        {
          //RD SEI packet
          //look if packet is kept or not
          bKeep = ( m_aaiLevelForFrame[uiLayer][uiCurrFrame] <= (Int)m_pcExtractorParameter->getLevel()
                 && uiLayer <= m_pcExtractorParameter->getLayer() );
          uiSEISize = uiPacketSize;
          if(bKeep)
            m_aadTargetByteForFrame[uiLayer][uiCurrFrame] -= uiSEISize;
          uiTotalSEI += uiPacketSize;
          uiSEI++;
        }
        else if( m_bInInputStreamQL )
        {
          //look if parameter sets NAL Unit
          UInt eNalUnitType = cPacketDescription.NalUnitType;
          //if(eNalUnitType == NAL_UNIT_SPS || eNalUnitType == NAL_UNIT_PPS)
					if(eNalUnitType == NAL_UNIT_SPS || eNalUnitType == NAL_UNIT_PPS || eNalUnitType == NAL_UNIT_SUBSET_SPS )// SSPS
          {
            RNOK( GetPictureDataKeep( &cPacketDescription, 0, uiPacketSize, bKeep ) );
            m_aadTargetByteForFrame[uiLayer][uiCurrFrame] -= uiPacketSize;
          }
          else
          {
            //============ check packet ===========
            if (uiLayer>m_pcExtractorParameter->getLayer())
            {
              bKeep=false;
            }
            else
            {
              RNOK( GetPictureDataKeep( &cPacketDescription, m_aadTargetByteForFrame[uiLayer][uiCurrFrame], uiPacketSize,  bKeep ) );

              if( !bFirstPacket ) //should not consider the scalable SEI message
                m_aadTargetByteForFrame[uiLayer][uiCurrFrame] -= uiPacketSize;
            }
          }
        }
      }
      else //not dead substream or QLAssigner
      {
        if(cPacketDescription.uiNumLevelsQL != 0) // fix provided by Nathalie
        {
          //QL SEI packet
          bApplyToNext = false;
        }
        Double dSNRLayerDiff;
        //============ check packet ===========
        //modif Nathalie
        if(uiLayer > m_pcExtractorParameter->getLayer())
        {
          bKeep = false;
        }
        else
        {
        //~modif Nathalie
        if( m_pcExtractorParameter->getUseSIP() ) //SIP
        {
          if(!cPacketDescription.bDiscardable||uiLayer==m_pcExtractorParameter->getLayer() )
            dSNRLayerDiff= m_aadTargetSNRLayer[uiLayer][uiLevel] - (Double)uiFGSLayer;
          else
            dSNRLayerDiff= m_aadTargetSNRLayerNoUse[uiLayer][uiLevel] - (Double)uiFGSLayer;
        }
        else //not SIP
        {
          dSNRLayerDiff = m_aadTargetSNRLayer[uiLayer][uiLevel] - (Double)uiFGSLayer;
        }

        Double  dUpRound      = ceil  ( dSNRLayerDiff );
        Double  dDownRound    = floor ( dSNRLayerDiff );
        bKeep                 =       ( dUpRound   >= 0.0 );

        if( bKeep && dDownRound < 0.0 )
        {
          bKeep = false;
        }

        UInt eNalUnitType = cPacketDescription.NalUnitType;
        //if( eNalUnitType == NAL_UNIT_SPS || eNalUnitType == NAL_UNIT_PPS )
				if( eNalUnitType == NAL_UNIT_SPS || eNalUnitType == NAL_UNIT_PPS || eNalUnitType == NAL_UNIT_SUBSET_SPS )//SSPS
          RNOK( GetPictureDataKeep( &cPacketDescription, 0, 0, bKeep ) );
      //modif Nathalie
        }
        //~modif Nathalie
      } //else

      if( bKeep && m_bInInputStreamQL )
      {
        totalPackets += uiPacketSize;
        dTot += uiPacketSize;
      }
      uiNumInput++;
      if( bKeep )
        uiNumKept++;
  //JVT-T073 {
      if( bKeep && bNextSuffix )
        uiNumKept++; //consider next suffix NAL unit

      if(bNextSuffix)
        uiNumInput++;
  //JVT-T073 }

      Bool bCountStat = false;
      if( cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE              ||
          cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_IDR          ||
          cPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE      )
      {
        if( cPacketDescription.Level <= m_pcExtractorParameter->getLevel() && cPacketDescription.Layer <= m_pcExtractorParameter->getLayer() )
        {
          bCountStat = true;
        }
      }

      if( bCountStat )
      {
        aaauiNumRetainedPerQualityId[cPacketDescription.Layer][cPacketDescription.FGSLayer][0]++;
      }
      //============ write and release packet ============
      if( bKeep )
      {
#if 0 // debug
        printf("D %d T %d Q %d \n", uiLayer, uiLevel, uiFGSLayer);
#endif
        if( bCountStat )
        {
          auiHighestRetainedQualityId[cPacketDescription.Layer] = gMax( auiHighestRetainedQualityId[cPacketDescription.Layer], cPacketDescription.FGSLayer );
          uiHighestRetainedDependencyId                         = gMax( uiHighestRetainedDependencyId,                         cPacketDescription.Layer );
          aaauiNumRetainedPerQualityId[cPacketDescription.Layer][cPacketDescription.FGSLayer][1]++;
        }
//prefix unit{{
        if((cPacketDescription.NalUnitType == 1 || cPacketDescription.NalUnitType == 5) && pcBinData_prefix != NULL )
        {
          RNOK( m_pcWriteBitstream->writePacket( &m_cBinDataStartCode ) );
          RNOK( m_pcWriteBitstream->writePacket( pcBinData_prefix ) );
          RNOK( m_pcReadBitstream->releasePacket( pcBinData_prefix ) );
          pcBinData_prefix = NULL;
          uiNumKept++;
          uiNumInput++;
        }
//prefix unit}}
        RNOK( m_pcWriteBitstream->writePacket( &m_cBinDataStartCode ) );
        RNOK( m_pcWriteBitstream->writePacket( pcBinData ) );
      }
//prefix unit{{
      else
      {
        if((cPacketDescription.NalUnitType == 1 || cPacketDescription.NalUnitType == 5) && pcBinData_prefix != NULL )
        {
          RNOK( m_pcReadBitstream->releasePacket( pcBinData_prefix ) );
          pcBinData_prefix = NULL;
          uiNumInput++;
        }
      }
//prefix unit}}
    }
    RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
    pcBinData = NULL;

    // consider ROI Extraction ICU/ETRI DS
    Count++;
    // JVT-T073 {
    if( bNextSuffix )
    {
      RNOK( m_pcReadBitstream->extractPacket( pcBinData, bEOS ) );
      if( bEOS )
      {
        RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
        RNOK( m_pcReadBitstream->releasePacket( pcBinDataSEILysNotPreDepChange ) );
        pcBinDataSEILysNotPreDepChange = NULL;
        continue;
      }
      while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
      {
        RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove zero at end
      }
      if( bKeep && bWriteBinData )
      {
        RNOK( m_pcWriteBitstream->writePacket( &m_cBinDataStartCode ) );
        RNOK( m_pcWriteBitstream->writePacket( pcBinData ) );
        uiPacketSize = 4+pcBinData->size();
        if( m_bInInputStreamQL )
        {
          m_aadTargetByteForFrame[uiLayer][uiCurrFrame] -= uiPacketSize;
          totalPackets += uiPacketSize;
          dTot += uiPacketSize;
        }
      }
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
    }
// JVT-T073 }

// JVT-S080 LMI {
    if ( bWriteBinDataSEILysNotPreDepChange )
    {
      while( pcBinDataSEILysNotPreDepChange->data()[ pcBinDataSEILysNotPreDepChange->size() - 1 ] == 0x00 )
      {
        RNOK( pcBinDataSEILysNotPreDepChange->decreaseEndPos( 1 ) ); // remove zero at end
      }
      uiPacketSize  = 4 + pcBinDataSEILysNotPreDepChange->size();

      RNOK( m_pcWriteBitstream->writePacket( &m_cBinDataStartCode ) );
      RNOK( m_pcWriteBitstream->writePacket( pcBinDataSEILysNotPreDepChange ) );
    }
    RNOK( m_pcReadBitstream->releasePacket( pcBinDataSEILysNotPreDepChange ) );
    pcBinDataSEILysNotPreDepChange = NULL;
// JVT-S080 LMI }
  }

  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );
  if( m_bInInputStreamQL )
  {
    printf("\nTotalPackets %.2lf \n", totalPackets);
    //printf("Total QualityLevelSEI in bitstream: %d \n ",uiTotalSEI);//SEI changes update
    printf("Total PriorityLevelSEI in bitstream: %d \n ",uiTotalSEI);//SEI changes update
  }
  printf( "\n" );
  for( UInt uiDIdRet = 0; uiDIdRet <= uiHighestRetainedDependencyId; uiDIdRet++ )
  {
    printf( "\nquality_id statistics for dependency_id %d\n", uiDIdRet );
    printf( "===========================================\n" );
    for( UInt uiQLRet = 0; uiQLRet <= auiHighestRetainedQualityId[uiDIdRet]; uiQLRet++ )
    {
      printf( "quality_id%2d - total:%6d retained:%6d\n", uiQLRet, aaauiNumRetainedPerQualityId[uiDIdRet][uiQLRet][0], aaauiNumRetainedPerQualityId[uiDIdRet][uiQLRet][1] );
    }
  }
  printf( "\nNumber of input packets:     %d\nNumber of output packets:    %d\n\n", uiNumInput, uiNumKept );

  return Err::m_nOK;
}


ErrVal
Extractor::xWriteScalableSEIToBuffer(h264::SEI::SEIMessage* pcScalableSei, BinData* pcBinData )
{
  const UInt uiSEILength = 10000;
  UChar		pulStreamPacket[uiSEILength];
//TMM_FIX
  //pcBinData->reset();
  pcBinData->deleteData();
  pcBinData->set( new UChar[uiSEILength], uiSEILength );
  UChar *m_pucBuffer = pcBinData->data();

  ScalableSEIModifyCode *pcScalableTestCode;
  RNOK( ScalableSEIModifyCode::Create(pcScalableTestCode) );
  RNOK( pcScalableTestCode->init( (UInt*) pulStreamPacket, uiSEILength ) );
  switch( pcScalableSei->getMessageType() )
  {
  case h264::SEI::SCALABLE_SEI:
    {
      RNOK( pcScalableTestCode->SEICode( (h264::SEI::ScalableSei*)pcScalableSei, pcScalableTestCode ) );
      break;
    }
  case h264::SEI::SCALABLE_SEI_LAYERS_NOT_PRESENT:
    {
      RNOK( pcScalableTestCode->SEICode( (h264::SEI::ScalableSeiLayersNotPresent*)pcScalableSei, pcScalableTestCode ) );
      break;
    }
  case h264::SEI::SCALABLE_SEI_DEPENDENCY_CHANGE:
    {
      RNOK( pcScalableTestCode->SEICode( (h264::SEI::ScalableSeiDependencyChange*)pcScalableSei, pcScalableTestCode ) );
      break;
    }
  default:
    ROT(1);
    break;
  }
  UInt uiBits = pcScalableTestCode->getNumberOfWrittenBits();
  UInt uiSize = (uiBits+7)/8;
  RNOK( pcScalableTestCode->Uninit() );
  RNOK( pcScalableTestCode->Destroy() );

  ScalableSEIModifyCode *pcScalableSEIModifyCode;
  RNOK( ScalableSEIModifyCode::Create(pcScalableSEIModifyCode) );
  RNOK( pcScalableSEIModifyCode->init( (UInt*) pulStreamPacket, uiSEILength ) );

  RNOK( pcScalableSEIModifyCode->WriteFlag( 0 ) );
  RNOK( pcScalableSEIModifyCode->WriteCode( 0 ,2 ) );
  RNOK( pcScalableSEIModifyCode->WriteCode( NAL_UNIT_SEI, 5 ) );
  RNOK( pcScalableSEIModifyCode->WritePayloadHeader( pcScalableSei->getMessageType(), uiSize ) );
  switch( pcScalableSei->getMessageType() )
  {
  case h264::SEI::SCALABLE_SEI:
    {
      RNOK( pcScalableSEIModifyCode->SEICode( (h264::SEI::ScalableSei*)pcScalableSei, pcScalableSEIModifyCode ) );
      break;
    }
  case h264::SEI::SCALABLE_SEI_LAYERS_NOT_PRESENT:
    {
      RNOK( pcScalableSEIModifyCode->SEICode( (h264::SEI::ScalableSeiLayersNotPresent*)pcScalableSei, pcScalableSEIModifyCode ) );
      break;
    }
  case h264::SEI::SCALABLE_SEI_DEPENDENCY_CHANGE:
    {
      RNOK( pcScalableSEIModifyCode->SEICode( (h264::SEI::ScalableSeiDependencyChange*)pcScalableSei, pcScalableSEIModifyCode ) );
      break;
    }
  default:
    ROT(1);
    break;
  }
  uiBits = pcScalableSEIModifyCode->getNumberOfWrittenBits();
  uiSize = (uiBits+7)/8;
  UInt uiAlignedBits = 8 - (uiBits&7);
  if( uiAlignedBits != 0 && uiAlignedBits != 8 )
  {
    RNOK( pcScalableSEIModifyCode->WriteCode( 1 << (uiAlignedBits-1), uiAlignedBits ) );
  }
  RNOK ( pcScalableSEIModifyCode->WriteTrailingBits() );
  RNOK ( pcScalableSEIModifyCode->flushBuffer() );
  uiBits = pcScalableSEIModifyCode->getNumberOfWrittenBits();
  uiBits              = ( uiBits >> 3 ) + ( 0 != ( uiBits & 0x07 ) );
  uiSize = uiBits;
  RNOK( pcScalableSEIModifyCode->ConvertRBSPToPayload( m_pucBuffer, pulStreamPacket, uiSize, 2 ) );
  pcBinData->decreaseEndPos( uiSEILength - uiSize );
  RNOK( pcScalableSEIModifyCode->Destroy() );
  return Err::m_nOK;
}

// JVT-S080 LMI {
ErrVal
Extractor::xChangeScalableSEIMessage( BinData *pcBinData, BinData *pcBinDataSEILysNotPreDepChange, h264::SEI::SEIMessage* pcScalableSEIMessage,
            UInt uiKeepScalableLayer, UInt& uiWantedScalableLayer, UInt& uiMaxLayer, UInt& uiMaxTempLevel, Double& dMaxFGSLayer, UInt uiMaxBitrate)

{
  Bool bLayerNotPresent[MAX_SCALABLE_LAYERS];
  h264::SEI::ScalableSeiLayersNotPresent* pcNewScalableSeiLayersNotPresent;
  RNOK( h264::SEI::ScalableSeiLayersNotPresent::create(pcNewScalableSeiLayersNotPresent) );
  h264::SEI::ScalableSeiLayersNotPresent* pcOldScalableSeiLayersNotPresent = (h264::SEI::ScalableSeiLayersNotPresent* ) pcScalableSEIMessage;

  if(pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI)
  {
  ::memset( bLayerNotPresent, 1, MAX_SCALABLE_LAYERS*sizeof(Bool));
// JVT-S080 LMI }
  h264::SEI::ScalableSei* pcNewScalableSei;
  RNOK( h264::SEI::ScalableSei::create(pcNewScalableSei) );

  h264::SEI::ScalableSei* pcOldScalableSei = ( h264::SEI::ScalableSei*) pcScalableSEIMessage;
// JVT-U085 LMI
  pcNewScalableSei->setTlevelNestingFlag( pcOldScalableSei->getTlevelNestingFlag() );
  pcNewScalableSei->setPriorityIdSettingFlag( pcOldScalableSei->getPriorityIdSettingFlag() );//JVT-W053 wxwan
  pcNewScalableSei->setNumLayersMinus1( uiKeepScalableLayer-1);

  UInt tmpScaLayerId[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  ::memset( tmpScaLayerId, -1, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt));
  UInt   uiMaxFGSLayer    = (UInt)ceil( dMaxFGSLayer );
  Double dSNRLayerDiff    = uiMaxFGSLayer - dMaxFGSLayer;
  Double dUpRound         = ceil ( dSNRLayerDiff );
  Bool   bTruncate        = ( ( dUpRound > 0.0 && uiMaxFGSLayer < MAX_QUALITY_LEVELS ) || //-f
                              ( uiWantedScalableLayer != MSYS_UINT_MAX && uiMaxLayer != MSYS_UINT_MAX && uiMaxBitrate == MSYS_UINT_MAX ) ||//-e
                              ( uiMaxBitrate != MSYS_UINT_MAX && dUpRound > 0.0 ) ); //-b
//JVT-S036 lsj start
  Bool   bExactMatchFlag[MAX_LAYERS];
  for( UInt ui = 0; ui < MAX_LAYERS; ui++ )
    bExactMatchFlag[ui] = true;
//JVT-S036 lsj end
  UInt uiNumScalableLayer = 0;
  UInt uiScalableLayer = 0;
  for( uiScalableLayer = 0; uiScalableLayer <= pcOldScalableSei->getNumLayersMinus1(); uiScalableLayer++ )
  {
    //JVT-S036 lsj start
    if( uiWantedScalableLayer == MSYS_UINT_MAX ) //-l, -t, -f
    {
      if( pcOldScalableSei->getDependencyId( uiScalableLayer ) == uiMaxLayer    &&
          pcOldScalableSei->getQualityId   ( uiScalableLayer ) >  uiMaxFGSLayer &&
          !m_pcExtractorParameter->getKeepfExtraction() ) // -keepf is not in use
      {
        bExactMatchFlag[pcOldScalableSei->getDependencyId( uiScalableLayer )] = false;
      }
    }
    else if( uiMaxLayer != MSYS_UINT_MAX && uiMaxBitrate == MSYS_UINT_MAX )  // -e
    {
      if( pcOldScalableSei->getDependencyId( uiScalableLayer ) == uiMaxLayer &&
          pcOldScalableSei->getQualityId   ( uiScalableLayer ) >  uiMaxFGSLayer )
      {
        bExactMatchFlag[pcOldScalableSei->getDependencyId( uiScalableLayer )] = false;
      }
    }
    else if( uiMaxBitrate == MSYS_UINT_MAX ) //-sl
    {
      if( pcOldScalableSei->getDependencyId( uiWantedScalableLayer ) == pcOldScalableSei->getDependencyId( uiScalableLayer ) &&
          pcOldScalableSei->getQualityId   ( uiScalableLayer )       >  pcOldScalableSei->getQualityId   ( uiWantedScalableLayer ) )
      {
        bExactMatchFlag[pcOldScalableSei->getDependencyId( uiScalableLayer )] = false;
      }
    }
    else //-b
    {
      if( pcOldScalableSei->getDependencyId( uiWantedScalableLayer ) == pcOldScalableSei->getDependencyId( uiScalableLayer ) &&
          pcOldScalableSei->getQualityId   ( uiScalableLayer )       >  pcOldScalableSei->getQualityId   ( uiWantedScalableLayer ) )
      {
        bExactMatchFlag[pcOldScalableSei->getDependencyId( uiScalableLayer )] = false;
      }
    }
    //JVT-S036 lsj end
    if( uiWantedScalableLayer == MSYS_UINT_MAX ) //-l, -t, -f
    {
      if( m_pcExtractorParameter->getKeepfExtraction() ) // -keepf is in use
      {
        if( ( pcOldScalableSei->getTemporalId( uiScalableLayer ) > uiMaxTempLevel ) ||
            ( pcOldScalableSei->getDependencyId( uiScalableLayer ) > uiMaxLayer ) ||
            ( ( pcOldScalableSei->getDependencyId( uiScalableLayer ) == uiMaxLayer ) &&
              ( pcOldScalableSei->getQualityId( uiScalableLayer ) > uiMaxFGSLayer ) )
          )
          continue;
      }
      else // not -keepf
        if( pcOldScalableSei->getDependencyId( uiScalableLayer ) > uiMaxLayer    ||
            pcOldScalableSei->getQualityId( uiScalableLayer ) > uiMaxFGSLayer ||
            pcOldScalableSei->getTemporalId( uiScalableLayer ) > uiMaxTempLevel  )
          continue;
    }
    else if( uiMaxLayer != MSYS_UINT_MAX && uiMaxBitrate == MSYS_UINT_MAX ) // -e
    {
      if( pcOldScalableSei->getDependencyId ( uiScalableLayer ) > uiMaxLayer ||
          pcOldScalableSei->getTemporalId( uiScalableLayer ) > uiMaxTempLevel ||
          m_aaadSingleBitrate[pcOldScalableSei->getDependencyId ( uiScalableLayer )]
                             [pcOldScalableSei->getTemporalId( uiScalableLayer )]
                             [pcOldScalableSei->getQualityId( uiScalableLayer )]  <= 0 )
        continue;
    }
    else //-sl, -b
    {
      if( uiWantedScalableLayer != uiScalableLayer &&
        std::find( m_acDepLayerList[uiWantedScalableLayer].begin(), m_acDepLayerList[uiWantedScalableLayer].end(), uiScalableLayer )
        == m_acDepLayerList[uiWantedScalableLayer].end() ) //not in dependent layer list
        continue;
    }

    pcNewScalableSei->setLayerId( uiNumScalableLayer, uiNumScalableLayer );
// JVT S080 LMI {
    bLayerNotPresent[uiScalableLayer] = false;
// JVT S080 LMI }
//JVT-S036 lsj start

    //pcNewScalableSei->setSimplePriorityId(uiNumScalableLayer, pcOldScalableSei->getSimplePriorityId( uiScalableLayer ) );//SEI changes update
		pcNewScalableSei->setPriorityId(uiNumScalableLayer, pcOldScalableSei->getPriorityId( uiScalableLayer ) );//SEI changes update
    pcNewScalableSei->setDiscardableFlag(uiNumScalableLayer, pcOldScalableSei->getDiscardableFlag( uiScalableLayer) );
    pcNewScalableSei->setTemporalId(uiNumScalableLayer, pcOldScalableSei->getTemporalId( uiScalableLayer ) );
    pcNewScalableSei->setDependencyId(uiNumScalableLayer, pcOldScalableSei->getDependencyId( uiScalableLayer ) );
    pcNewScalableSei->setQualityLevel(uiNumScalableLayer, pcOldScalableSei->getQualityId( uiScalableLayer ) );
    tmpScaLayerId[pcOldScalableSei->getDependencyId ( uiScalableLayer )]
                 [pcOldScalableSei->getTemporalId( uiScalableLayer )]
                 [pcOldScalableSei->getQualityId ( uiScalableLayer )] =  uiNumScalableLayer;

    pcNewScalableSei->setSubPicLayerFlag( uiNumScalableLayer, pcOldScalableSei->getSubPicLayerFlag( uiScalableLayer ) );
    pcNewScalableSei->setSubRegionLayerFlag( uiNumScalableLayer, pcOldScalableSei->getSubRegionLayerFlag( uiScalableLayer ) );
    pcNewScalableSei->setIroiSliceDivisionInfoPresentFlag( uiNumScalableLayer, pcOldScalableSei->getIroiSliceDivisionInfoPresentFlag( uiScalableLayer ) );
    pcNewScalableSei->setProfileLevelInfoPresentFlag( uiNumScalableLayer, pcOldScalableSei->getProfileLevelInfoPresentFlag( uiScalableLayer ) );
  //JVT-S036 lsj change end
    pcNewScalableSei->setBitrateInfoPresentFlag( uiNumScalableLayer, pcOldScalableSei->getBitrateInfoPresentFlag( uiScalableLayer ) );
    pcNewScalableSei->setFrmRateInfoPresentFlag( uiNumScalableLayer, pcOldScalableSei->getFrmRateInfoPresentFlag( uiScalableLayer ) );
    pcNewScalableSei->setFrmSizeInfoPresentFlag( uiNumScalableLayer, pcOldScalableSei->getFrmSizeInfoPresentFlag( uiScalableLayer ) );
    pcNewScalableSei->setLayerDependencyInfoPresentFlag( uiNumScalableLayer, pcOldScalableSei->getLayerDependencyInfoPresentFlag( uiScalableLayer ) );
    //pcNewScalableSei->setInitParameterSetsInfoPresentFlag( uiNumScalableLayer, pcOldScalableSei->getInitParameterSetsInfoPresentFlag( uiScalableLayer ) );//SEI changes update
    pcNewScalableSei->setParameterSetsInfoPresentFlag( uiNumScalableLayer, pcOldScalableSei->getParameterSetsInfoPresentFlag( uiScalableLayer ) );//SEI changes update
		pcNewScalableSei->setExactInterlayerPredFlag( uiNumScalableLayer, pcOldScalableSei->getExactInterlayerPredFlag( uiScalableLayer) ); //JVT-S036 lsj
    pcNewScalableSei->setLayerOutputFlag( uiNumScalableLayer, pcOldScalableSei->getLayerOutputFlag(uiScalableLayer) );//JVT-W047 wxwan
    if( !bExactMatchFlag[pcOldScalableSei->getDependencyId(uiScalableLayer)] )
      pcNewScalableSei->setExactInterlayerPredFlag( uiNumScalableLayer, bExactMatchFlag[pcOldScalableSei->getDependencyId(uiScalableLayer)] ); //JVT-S036 lsj


    if(pcNewScalableSei->getProfileLevelInfoPresentFlag(uiNumScalableLayer))
    {
      pcNewScalableSei->setLayerProfileLevelIdc( uiNumScalableLayer, pcOldScalableSei->getLayerProfileLevelIdc( uiScalableLayer ) );
    }

		//else
    //{//JVT-S036 lsj
    //  pcNewScalableSei->setProfileLevelInfoSrcLayerIdDelta(uiNumScalableLayer, pcOldScalableSei->getProfileLevelInfoSrcLayerIdDelta( uiScalableLayer ) );
    //}
    //SEI changes update }
    if(pcNewScalableSei->getBitrateInfoPresentFlag(uiNumScalableLayer))
    {
      if( bTruncate ) // may be for -f,-e,-b option truncation
      {
        if( uiWantedScalableLayer == MSYS_UINT_MAX ) //-f
        {
          if( pcNewScalableSei->getQualityId( uiNumScalableLayer ) == uiMaxFGSLayer )
            pcNewScalableSei->setAvgBitrateBPS( uiNumScalableLayer, (Double)pcOldScalableSei->getAvgBitrateBPS( uiScalableLayer ) * ( 1.0 - dSNRLayerDiff ) );
          else
            pcNewScalableSei->setAvgBitrateBPS( uiNumScalableLayer, (Double)pcOldScalableSei->getAvgBitrateBPS( uiScalableLayer ) );
        }
        else if( uiMaxBitrate == MSYS_UINT_MAX )//-e
        {
//cleaning
          pcNewScalableSei->setAvgBitrateBPS( uiNumScalableLayer, m_adTotalBitrate[uiScalableLayer] );
//~cleaning
        }
        else //-b
        {
          if( pcNewScalableSei->getDependencyId( uiNumScalableLayer ) == m_auiDependencyId[uiWantedScalableLayer] &&
              pcNewScalableSei->getQualityId( uiNumScalableLayer ) == m_auiQualityLevel[uiWantedScalableLayer] )
            pcNewScalableSei->setAvgBitrateBPS( uiNumScalableLayer, (Double)pcOldScalableSei->getAvgBitrateBPS( uiScalableLayer ) * ( 1.0 - dSNRLayerDiff ) );
          else
            pcNewScalableSei->setAvgBitrateBPS( uiNumScalableLayer, (Double)pcOldScalableSei->getAvgBitrateBPS( uiScalableLayer ) );
        }
      }

      else
        pcNewScalableSei->setAvgBitrateBPS(uiNumScalableLayer, m_adTotalBitrate[uiScalableLayer] );
    //JVT-S036 lsj start
      pcNewScalableSei->setMaxBitrateLayerBPS(uiNumScalableLayer, (Double)pcOldScalableSei->getMaxBitrateLayerBPS( uiScalableLayer ) );
      pcNewScalableSei->setMaxBitrateDecodedPictureBPS(uiNumScalableLayer, (Double)pcOldScalableSei->getMaxBitrateDecodedPictureBPS( uiScalableLayer ) );
      pcNewScalableSei->setMaxBitrateCalcWindow(uiNumScalableLayer, pcOldScalableSei->getMaxBitrateCalcWindow( uiScalableLayer ) );
    //JVT-S036 lsj end
    }

    if(pcNewScalableSei->getFrmRateInfoPresentFlag(uiNumScalableLayer))
    {
      pcNewScalableSei->setConstantFrmRateIdc(uiNumScalableLayer, pcOldScalableSei->getConstantFrmRateIdc( uiScalableLayer ) );
      pcNewScalableSei->setAvgFrmRate(uiNumScalableLayer, pcOldScalableSei->getAvgFrmRate( uiScalableLayer ) );
    }
    //SEI changes update {
    //else
    //{//JVT-S036 lsj
    //  pcNewScalableSei->setFrmRateInfoSrcLayerIdDelta(uiNumScalableLayer, pcOldScalableSei->getFrmRateInfoSrcLayerIdDelta( uiScalableLayer) );
    //}

    //if(pcNewScalableSei->getFrmSizeInfoPresentFlag(uiNumScalableLayer))
    if(pcNewScalableSei->getFrmSizeInfoPresentFlag(uiNumScalableLayer) || pcNewScalableSei->getIroiSliceDivisionInfoPresentFlag(uiNumScalableLayer))
		//SEI changes update }
    {
      pcNewScalableSei->setFrmWidthInMbsMinus1(uiNumScalableLayer, pcOldScalableSei->getFrmWidthInMbsMinus1( uiScalableLayer ) );
      pcNewScalableSei->setFrmHeightInMbsMinus1(uiNumScalableLayer, pcOldScalableSei->getFrmHeightInMbsMinus1( uiScalableLayer ) );
    }
    //SEI changes update
		//else
    //{//JVT-S036 lsj
    //  pcNewScalableSei->setFrmSizeInfoSrcLayerIdDelta(uiNumScalableLayer, pcOldScalableSei->getFrmSizeInfoSrcLayerIdDelta( uiScalableLayer) );
    //}

    if(pcNewScalableSei->getSubRegionLayerFlag(uiNumScalableLayer))
    {
      pcNewScalableSei->setBaseRegionLayerId(uiNumScalableLayer, pcOldScalableSei->getBaseRegionLayerId( uiScalableLayer ) );
      pcNewScalableSei->setDynamicRectFlag(uiNumScalableLayer, pcOldScalableSei->getDynamicRectFlag( uiScalableLayer ) );
      if(pcNewScalableSei->getDynamicRectFlag(uiNumScalableLayer))
      {
        pcNewScalableSei->setHorizontalOffset(uiNumScalableLayer, pcOldScalableSei->getHorizontalOffset( uiScalableLayer ) );
        pcNewScalableSei->setVerticalOffset(uiNumScalableLayer, pcOldScalableSei->getVerticalOffset( uiScalableLayer ) );
        pcNewScalableSei->setRegionWidth(uiNumScalableLayer, pcOldScalableSei->getRegionWidth( uiScalableLayer ) );
        pcNewScalableSei->setRegionHeight(uiNumScalableLayer, pcOldScalableSei->getRegionHeight( uiScalableLayer ) );
      }
    }
    //SEI changes update
		//else
    //{//JVT-S036 lsj
    //  pcNewScalableSei->setSubRegionInfoSrcLayerIdDelta(uiNumScalableLayer, pcOldScalableSei->getSubRegionInfoSrcLayerIdDelta( uiScalableLayer) );
    //}

  //JVT-S036 lsj start
    if( pcNewScalableSei->getSubPicLayerFlag( uiNumScalableLayer ) )
    {
      pcNewScalableSei->setRoiId( uiNumScalableLayer, pcOldScalableSei->getRoiId( uiNumScalableLayer ) );
    }
    if( pcNewScalableSei->getIroiSliceDivisionInfoPresentFlag( uiNumScalableLayer ) )
    {
      //SEI changes update {
			//pcNewScalableSei->setIroiSliceDivisionType( uiNumScalableLayer, pcOldScalableSei->getIroiSliceDivisionType( uiNumScalableLayer ) );
			pcNewScalableSei->setIroiGridFlag( uiNumScalableLayer, pcOldScalableSei->getIroiGridFlag( uiNumScalableLayer ) );//SEI changes update
      //if( pcNewScalableSei->getIroiSliceDivisionType(uiNumScalableLayer) == 0 )
      //{
      //  pcNewScalableSei->setGridSliceWidthInMbsMinus1( uiNumScalableLayer, pcOldScalableSei->getGridSliceWidthInMbsMinus1( uiNumScalableLayer ) );
      //  pcNewScalableSei->setGridSliceHeightInMbsMinus1( uiNumScalableLayer, pcOldScalableSei->getGridSliceHeightInMbsMinus1( uiNumScalableLayer ) );
      //}
      //else if( pcNewScalableSei->getIroiSliceDivisionType(uiNumScalableLayer) == 1 )
      //{
      //  pcNewScalableSei->setNumSliceMinus1( uiNumScalableLayer, pcOldScalableSei->getNumSliceMinus1( uiNumScalableLayer ) );
      //  for (UInt nslice = 0; nslice <= pcNewScalableSei->getNumSliceMinus1( uiNumScalableLayer ) ; nslice ++ )
      //  {
      //    pcNewScalableSei->setFirstMbInSlice( uiNumScalableLayer, nslice, pcOldScalableSei->getFirstMbInSlice( uiNumScalableLayer, nslice ) );
      //    pcNewScalableSei->setSliceWidthInMbsMinus1( uiNumScalableLayer, nslice, pcOldScalableSei->getSliceWidthInMbsMinus1( uiNumScalableLayer, nslice ) );
      //    pcNewScalableSei->setSliceHeightInMbsMinus1( uiNumScalableLayer, nslice, pcOldScalableSei->getSliceHeightInMbsMinus1( uiNumScalableLayer, nslice ) );
      //  }
      //}
      //else if( pcNewScalableSei->getIroiSliceDivisionType(uiNumScalableLayer) == 2 )
      //{
      //  pcNewScalableSei->setNumSliceMinus1( uiNumScalableLayer, pcOldScalableSei->getNumSliceMinus1( uiNumScalableLayer ) );

      //  UInt uiFrameHeightInMb = pcNewScalableSei->getFrmHeightInMbsMinus1( uiNumScalableLayer ) + 1;
      //  UInt uiFrameWidthInMb  = pcNewScalableSei->getFrmWidthInMbsMinus1(uiNumScalableLayer ) + 1;
      //  UInt uiPicSizeInMbs = uiFrameHeightInMb * uiFrameWidthInMb;
      //  for( UInt j = 0; j < uiPicSizeInMbs; j++ )
      //  {
      //    pcNewScalableSei->setSliceId( uiNumScalableLayer, j, pcOldScalableSei->getSliceId( uiNumScalableLayer, j ) );
      //  }
      //}
			if( pcNewScalableSei->getIroiGridFlag(uiNumScalableLayer) )
      {
        pcNewScalableSei->setGridSliceWidthInMbsMinus1( uiNumScalableLayer, pcOldScalableSei->getGridSliceWidthInMbsMinus1( uiNumScalableLayer ) );
        pcNewScalableSei->setGridSliceHeightInMbsMinus1( uiNumScalableLayer, pcOldScalableSei->getGridSliceHeightInMbsMinus1( uiNumScalableLayer ) );
      }
      else
      {
        pcNewScalableSei->setNumSliceMinus1( uiNumScalableLayer, pcOldScalableSei->getNumSliceMinus1( uiNumScalableLayer ) );
        for (UInt nslice = 0; nslice <= pcNewScalableSei->getNumSliceMinus1( uiNumScalableLayer ) ; nslice ++ )
        {
          pcNewScalableSei->setFirstMbInSlice( uiNumScalableLayer, nslice, pcOldScalableSei->getFirstMbInSlice( uiNumScalableLayer, nslice ) );
          pcNewScalableSei->setSliceWidthInMbsMinus1( uiNumScalableLayer, nslice, pcOldScalableSei->getSliceWidthInMbsMinus1( uiNumScalableLayer, nslice ) );
          pcNewScalableSei->setSliceHeightInMbsMinus1( uiNumScalableLayer, nslice, pcOldScalableSei->getSliceHeightInMbsMinus1( uiNumScalableLayer, nslice ) );
        }
      }
    }
  //JVT-S036 lsj end
//SEI changes update }
    if(pcNewScalableSei->getLayerDependencyInfoPresentFlag(uiNumScalableLayer))
    {
      pcNewScalableSei->setNumDirectlyDependentLayers(uiNumScalableLayer, pcOldScalableSei->getNumDirectlyDependentLayers( uiScalableLayer ) );
      for( UInt j = 0; j < pcNewScalableSei->getNumDirectlyDependentLayers(uiNumScalableLayer); j++)
      {
        //change direct dependent layer info
        assert( j <= 2 );
      //UInt uiOldDepScaLayer  = uiScalableLayer - ( pcOldScalableSei->getNumDirectlyDependentLayerIdDeltaMinus1(uiScalableLayer, j)+1 );
        UInt uiOldDepScaLayer  = uiScalableLayer - ( m_auiDirDepLayerDelta[uiScalableLayer][j] ); // as suggested by Danny Hong
        UInt uiOldDependencyId = pcOldScalableSei->getDependencyId(uiOldDepScaLayer);
        UInt uiOldTempLevel    = pcOldScalableSei->getTemporalId(uiOldDepScaLayer);
        UInt uiOldQualityLevel = pcOldScalableSei->getQualityId(uiOldDepScaLayer);
        UInt uiNewDepScaLayer  = tmpScaLayerId[uiOldDependencyId][uiOldTempLevel][uiOldQualityLevel];
        if( m_bUseSIP && uiNewDepScaLayer == MSYS_UINT_MAX ) // this happens only when SIP is used and ALL packets of certain scalable layer is discarded
        {
          while(uiOldTempLevel && uiNewDepScaLayer == MSYS_UINT_MAX)
          {
            uiNewDepScaLayer = tmpScaLayerId[uiOldDependencyId][--uiOldTempLevel][uiOldQualityLevel];
          }
          if( !uiOldTempLevel && uiNewDepScaLayer == MSYS_UINT_MAX ) //not suitable dependent layer
            pcNewScalableSei->setNumDirectlyDependentLayers(uiNumScalableLayer, j );
        }
        if( j == 0 ) //direct dependent layer 0
        {
          if( pcOldScalableSei->getQualityId(uiScalableLayer) ) // Q != 0
          {
            assert( uiNumScalableLayer - uiNewDepScaLayer >= 0 );
          }
          else if( pcOldScalableSei->getTemporalId(uiScalableLayer) ) //TL != 0, Q = 0
          {
            assert( uiNumScalableLayer - uiNewDepScaLayer >= 0 );
          }
          else // D != 0, T = 0, Q = 0
          {
            Int iFGS;
            for( iFGS = (Int) uiOldQualityLevel; iFGS >=0; iFGS-- )
            {
              if(tmpScaLayerId[uiOldDependencyId][uiOldTempLevel][iFGS] != MSYS_UINT_MAX )
                break;
            }
            uiOldQualityLevel = ( iFGS >= 0 ) ? (UInt) iFGS : 0;
            uiNewDepScaLayer = tmpScaLayerId[uiOldDependencyId][uiOldTempLevel][uiOldQualityLevel];
          }

          if( uiNewDepScaLayer == MSYS_UINT_MAX ) //this should not happen
          {
            printf("No reasonable dependent layer exist!! Errror!!\n");
              exit(1);
          }
          pcNewScalableSei->setDirectlyDependentLayerIdDeltaMinus1(uiNumScalableLayer, j, uiNumScalableLayer - uiNewDepScaLayer - 1);
        }
        else if( j == 1 ) // j == 1, direct dependent layer 1
        {
          if( pcOldScalableSei->getQualityId(uiScalableLayer) ) // Q != 0, T != 0
        {
          Int iFGS;
          for( iFGS = (Int) uiOldQualityLevel; iFGS >= 0; iFGS-- )
          {
            if( tmpScaLayerId[uiOldDependencyId][uiOldTempLevel][iFGS] != MSYS_UINT_MAX )
            break;
          }
          uiOldQualityLevel = ( iFGS >= 0 ) ? (UInt) iFGS : 0;
          uiNewDepScaLayer = tmpScaLayerId[uiOldDependencyId][uiOldTempLevel][uiOldQualityLevel];
        }
        else if( pcOldScalableSei->getTemporalId( uiScalableLayer ) ) // D != 0, T != 0, Q = 0
        {
          Int iFGS;
          for( iFGS = (Int) uiOldQualityLevel; iFGS >= 0; iFGS-- )
          {
            if( tmpScaLayerId[uiOldDependencyId][uiOldTempLevel][iFGS] != MSYS_UINT_MAX )
            break;
          }
          uiOldQualityLevel = ( iFGS >= 0 ) ? (UInt) iFGS : 0;
          uiNewDepScaLayer = tmpScaLayerId[uiOldDependencyId][uiOldTempLevel][uiOldQualityLevel];
        }
        if( uiNewDepScaLayer == MSYS_UINT_MAX ) //this should not happen
        {
          printf("No reasonable dependent layer exist!! Errror!!\n");
          exit(1);
        }
        pcNewScalableSei->setDirectlyDependentLayerIdDeltaMinus1(uiNumScalableLayer, j, uiNumScalableLayer - uiNewDepScaLayer - 1);
        }
      }
    }
    else
    {//JVT-S036 lsj
      pcNewScalableSei->setLayerDependencyInfoSrcLayerIdDelta(uiNumScalableLayer, pcOldScalableSei->getLayerDependencyInfoSrcLayerIdDelta( uiScalableLayer ) );
    }

    //if(pcNewScalableSei->getInitParameterSetsInfoPresentFlag(uiNumScalableLayer))//SEI changes update
		if(pcNewScalableSei->getParameterSetsInfoPresentFlag(uiNumScalableLayer))//SEI changes update
    {
      pcNewScalableSei->setNumInitSeqParameterSetMinus1(uiNumScalableLayer, pcOldScalableSei->getNumInitSPSMinus1( uiScalableLayer ) );
      pcNewScalableSei->setNumInitSubsetSeqParameterSetMinus1(uiNumScalableLayer, pcOldScalableSei->getNumInitSSPSMinus1( uiScalableLayer ) );//SEI changes update
			pcNewScalableSei->setNumInitPicParameterSetMinus1(uiNumScalableLayer, pcOldScalableSei->getNumInitPPSMinus1( uiScalableLayer ) );
      UInt j;
      for( j = 0; j <= pcNewScalableSei->getNumInitSPSMinus1(uiNumScalableLayer); j++)
      {
        pcNewScalableSei->setInitSeqParameterSetIdDelta( uiNumScalableLayer, j, pcOldScalableSei->getInitSPSIdDelta(uiScalableLayer, j ) );
      }
      //SEI changes update {
      for( j = 0; j <= pcNewScalableSei->getNumInitSSPSMinus1(uiNumScalableLayer); j++)
      {
        pcNewScalableSei->setInitSubsetSeqParameterSetIdDelta( uiNumScalableLayer, j, pcOldScalableSei->getInitSSPSIdDelta(uiScalableLayer, j ) );
      }
			//SEI changes update }
      for( j = 0; j <= pcNewScalableSei->getNumInitPPSMinus1(uiNumScalableLayer); j++)
      {
        pcNewScalableSei->setInitPicParameterSetIdDelta( uiNumScalableLayer, j, pcOldScalableSei->getInitPPSIdDelta(uiScalableLayer, j ) );
      }
    }
    else
    {//JVT-S036 lsj
      pcNewScalableSei->setInitParameterSetsInfoSrcLayerIdDelta( uiNumScalableLayer, pcOldScalableSei->getInitParameterSetsInfoSrcLayerIdDelta( uiScalableLayer ) );
    }
    //SEI changes update {
    //JVT-W046 {
    //pcNewScalableSei->setAvcLayerConversionFlag(uiScalableLayer,pcOldScalableSei->getAvcLayerConversionFlag(uiScalableLayer));
    //if( pcOldScalableSei->getAvcLayerConversionFlag(uiScalableLayer) )
    //{
    //  pcNewScalableSei->setAvcConversionTypeIdc(uiScalableLayer,pcOldScalableSei->getAvcConversionTypeIdc(uiScalableLayer));
    //  for ( UInt uiType = 0; uiType < 2; uiType++ )
    //  {
    //    pcNewScalableSei->setAvcInfoFlag(uiScalableLayer,uiType,pcOldScalableSei->getAvcInfoFlag(uiScalableLayer,uiType));
    //    if( pcOldScalableSei->getAvcInfoFlag(uiScalableLayer, uiType) )
    //    {
    //      pcNewScalableSei->setAvcProfileLevelIdc(uiScalableLayer,uiType,pcOldScalableSei->getAvcProfileLevelIdc(uiScalableLayer,uiType));
    //      pcNewScalableSei->setAvcAvgBitrate(uiScalableLayer,uiType,pcOldScalableSei->getAvcAvgBitrate(uiScalableLayer,uiType));
    //      pcNewScalableSei->setAvcMaxBitrate(uiScalableLayer,uiType,pcOldScalableSei->getAvcMaxBitrate(uiScalableLayer,uiType));
    //    }
    //  }
    //}
		pcNewScalableSei->setLayerConversionFlag(uiScalableLayer,pcOldScalableSei->getLayerConversionFlag(uiScalableLayer));
    if( pcOldScalableSei->getLayerConversionFlag(uiScalableLayer) )
    {
      pcNewScalableSei->setConversionTypeIdc(uiScalableLayer,pcOldScalableSei->getConversionTypeIdc(uiScalableLayer));
      for ( UInt uiType = 0; uiType < 2; uiType++ )
      {
        pcNewScalableSei->setRewritingInfoFlag(uiScalableLayer,uiType,pcOldScalableSei->getRewritingInfoFlag(uiScalableLayer,uiType));
        if( pcOldScalableSei->getRewritingInfoFlag(uiScalableLayer, uiType) )
        {
          pcNewScalableSei->setRewritingProfileLevelIdc(uiScalableLayer,uiType,pcOldScalableSei->getRewritingProfileLevelIdc(uiScalableLayer,uiType));
          pcNewScalableSei->setRewritingAvgBitrateBPS(uiScalableLayer,uiType,(Double)pcOldScalableSei->getRewritingAvgBitrateBPS(uiScalableLayer,uiType));
          pcNewScalableSei->setRewritingMaxBitrateBPS(uiScalableLayer,uiType,(Double)pcOldScalableSei->getRewritingMaxBitrateBPS(uiScalableLayer,uiType));
        }
      }
    }
    //JVT-W046 }
		//SEI changes update }
    uiNumScalableLayer++;

  }

  //JVT-W053 wxwan
  if(pcNewScalableSei->getPriorityIdSettingFlag() )
  {
    UInt PriorityIdSettingUriIdx = 0;
    do{
      pcNewScalableSei->setPriorityIdSettingUri(PriorityIdSettingUriIdx, pcOldScalableSei->getPriorityIdSettingUri(PriorityIdSettingUriIdx) );
    }while( pcOldScalableSei->getPriorityIdSettingUri( PriorityIdSettingUriIdx++ )  !=  0 );
  }
  //JVT-W053 wxwan

  pcNewScalableSei->setNumLayersMinus1( uiNumScalableLayer-1);

  // JVT-S080 LMI

  UInt i, uiNumLayersNotPresent = 0;

  for( i = 0; i <= pcOldScalableSei->getNumLayersMinus1(); i++ )
  {
    if (bLayerNotPresent[pcOldScalableSei->getLayerId(i)]) {
      pcNewScalableSeiLayersNotPresent->setLayerId(uiNumLayersNotPresent, pcOldScalableSei->getLayerId(i) );
      uiNumLayersNotPresent++;
    }
  }
  pcNewScalableSeiLayersNotPresent->setNumLayers(uiNumLayersNotPresent);

  h264::SEI::ScalableSeiLayersNotPresent::m_uiLeftNumLayers = pcNewScalableSeiLayersNotPresent->getNumLayers();

  for ( i = 0; i < pcNewScalableSeiLayersNotPresent->getNumLayers(); i++)
    h264::SEI::ScalableSeiLayersNotPresent::m_auiLeftLayerId[i] = pcNewScalableSeiLayersNotPresent->getLayerId(i);
  //JVT-W051 {
  pcNewScalableSei->setPriorityLayerInfoPresentFlag(pcOldScalableSei->getPriorityLayerInfoPresentFlag());//SEI changes update

  for( uiScalableLayer = 0; uiScalableLayer <= pcOldScalableSei->getNumLayersMinus1(); uiScalableLayer++ )
  {
		pcNewScalableSei->setBitstreamRestrictionInfoPresentFlag(uiScalableLayer,pcOldScalableSei->getBitstreamRestrictionInfoPresentFlag(uiScalableLayer));//SEI changes update
    pcNewScalableSei->setMotionVectorsOverPicBoundariesFlag(uiScalableLayer,pcOldScalableSei->getMotionVectorsOverPicBoundariesFlag(uiScalableLayer));
    pcNewScalableSei->setMaxBytesPerPicDenom(uiScalableLayer,pcOldScalableSei->getMaxBytesPerPicDenom(uiScalableLayer));
    pcNewScalableSei->setMaxBitsPerMbDenom(uiScalableLayer,pcOldScalableSei->getMaxBitsPerMbDenom(uiScalableLayer));
    pcNewScalableSei->setLog2MaxMvLengthHorizontal(uiScalableLayer,pcOldScalableSei->getLog2MaxMvLengthHorizontal(uiScalableLayer));
    pcNewScalableSei->setLog2MaxMvLengthVertical(uiScalableLayer,pcOldScalableSei->getLog2MaxMvLengthVertical(uiScalableLayer));
    pcNewScalableSei->setNumReorderFrames(uiScalableLayer,pcOldScalableSei->getNumReorderFrames(uiScalableLayer));
    pcNewScalableSei->setMaxDecFrameBuffering(uiScalableLayer,pcOldScalableSei->getMaxDecFrameBuffering(uiScalableLayer));
  }
  //SEI changes update {
  if (pcOldScalableSei->getPriorityLayerInfoPresentFlag())
  {
    pcNewScalableSei->setPrNumdIdMinus1(pcOldScalableSei->getPrNumdIdMinus1());
    for ( i = 0; i <= pcOldScalableSei->getPrNumdIdMinus1(); i++ )
    {
      pcNewScalableSei->setPrDependencyId(i,pcOldScalableSei->getPrDependencyId(i));
      pcNewScalableSei->setPrNumMinus1(i,pcOldScalableSei->getPrNumMinus1(i));
      for ( UInt j = 0; j <= pcOldScalableSei->getPrNumMinus1(i); j++ )
      {
        pcNewScalableSei->setPrId(i,j,pcOldScalableSei->getPrId(i,j));
        pcNewScalableSei->setPrProfileLevelIdx(i,j,pcOldScalableSei->getPrProfileLevelIdc(i,j));
        pcNewScalableSei->setPrAvgBitrateBPS(i,j,(Double)pcOldScalableSei->getPrAvgBitrateBPS(i,j));
        pcNewScalableSei->setPrMaxBitrateBPS(i,j,(Double)pcOldScalableSei->getPrMaxBitrateBPS(i,j));
      }
    }
  }
  //JVT-W051 }
  //SEI changes update }

#if UPDATE_SCALABLE_SEI
  RNOK( xWriteScalableSEIToBuffer( (h264::SEI::SEIMessage*)pcNewScalableSei, pcBinData ) );
  // write pcNewScalableSei into bitstream pcBinData
#else
   // write the original SSEI followed by a layers_not_present SSEI message
  RNOK( xWriteScalableSEIToBuffer( (h264::SEI::SEIMessage*)pcOldScalableSei, pcBinData ) );
  if ( uiNumLayersNotPresent > 0 )
  {
    pcOldScalableSeiLayersNotPresent->setOutputFlag ( true );
    RNOK( xWriteScalableSEIToBuffer( (h264::SEI::SEIMessage*)pcNewScalableSeiLayersNotPresent, pcBinDataSEILysNotPreDepChange ) );
  }
  else
    pcOldScalableSeiLayersNotPresent->setOutputFlag ( false );
#endif

    pcNewScalableSei->destroy(); //TMM_FIX
  }

    // now deal with the layers_not_present SSEI message sent by the encoder
  if( pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI_LAYERS_NOT_PRESENT )
  {
     UInt i, j, uiScalableLayer, uiNumLayersNotPresent = 0;
       Bool bLayerNotPresentUpdate[MAX_SCALABLE_LAYERS];
     UInt uiOldNewLayerIdMap[MAX_SCALABLE_LAYERS];
     ::memset( bLayerNotPresent, 0, MAX_SCALABLE_LAYERS*sizeof(Bool));
     ::memset( bLayerNotPresentUpdate, 0, MAX_SCALABLE_LAYERS*sizeof(Bool));

     for( i = 0; i < h264::SEI::ScalableSeiLayersNotPresent::m_uiLeftNumLayers; i++ )
            bLayerNotPresent[h264::SEI::ScalableSeiLayersNotPresent::m_auiLeftLayerId[i]] = true;

       j = 0;
     for( i = 0; i < MAX_SCALABLE_LAYERS; i++ )
       if( !bLayerNotPresent[i] )
         uiOldNewLayerIdMap[i] = j++;

     for( i = 0; i < pcOldScalableSeiLayersNotPresent->getNumLayers(); i++ )
     {
         #if UPDATE_SCALABLE_SEI
       uiScalableLayer = pcOldScalableSeiLayersNotPresent->getLayerId(i);
         if( !bLayerNotPresent[uiScalableLayer] )
               bLayerNotPresentUpdate[uiOldNewLayerIdMap[uiScalableLayer]] = true ;
         #else
            bLayerNotPresent[pcOldScalableSeiLayersNotPresent->getDependencyId(i)] = true;
         #endif
     }

     for( uiScalableLayer = 0; uiScalableLayer < MAX_SCALABLE_LAYERS; uiScalableLayer++ )
       #if UPDATE_SCALABLE_SEI
        if ( bLayerNotPresentUpdate[uiScalableLayer] )
       #else
        if ( bLayerNotPresent[uiScalableLayer] )
       #endif
        {
          pcNewScalableSeiLayersNotPresent->setLayerId(uiNumLayersNotPresent,uiScalableLayer);
          uiNumLayersNotPresent++;
        }

     pcNewScalableSeiLayersNotPresent->setNumLayers(uiNumLayersNotPresent);

     if ( uiNumLayersNotPresent > 0 )
     {
          pcOldScalableSeiLayersNotPresent->setOutputFlag ( true );
          RNOK( xWriteScalableSEIToBuffer( (h264::SEI::SEIMessage*) pcNewScalableSeiLayersNotPresent, pcBinDataSEILysNotPreDepChange ) );
     }
     else
          pcOldScalableSeiLayersNotPresent->setOutputFlag ( false );
  }

    // now we deal with the dependency change SSEI
  if(pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI_DEPENDENCY_CHANGE)
  {
    UInt i, j, k, uiNumDireDepLyrs, uiLid;
    h264::SEI::ScalableSeiDependencyChange* pcNewScalableSeiDepChange;
    h264::SEI::ScalableSeiDependencyChange* pcOldScalableSeiDepChange = ( h264::SEI::ScalableSeiDependencyChange* ) pcScalableSEIMessage;
    RNOK( h264::SEI::ScalableSeiDependencyChange::create(pcNewScalableSeiDepChange) );
    ::memset( bLayerNotPresent, 0, MAX_SCALABLE_LAYERS*sizeof(Bool));

    for( i = 0; i < h264::SEI::ScalableSeiLayersNotPresent::m_uiLeftNumLayers; i++ )
         bLayerNotPresent[h264::SEI::ScalableSeiLayersNotPresent::m_auiLeftLayerId[i]] = true;

    j = 0;
    for( i = 0; i <= pcOldScalableSeiDepChange->getNumLayersMinus1(); i++ )
    {
      uiLid = pcOldScalableSeiDepChange->getDependencyId( i );
      if( ! bLayerNotPresent[uiLid] )
      {
        pcNewScalableSeiDepChange->setDependencyId( j, uiLid );
        pcNewScalableSeiDepChange->setLayerDependencyInfoPresentFlag( j, pcOldScalableSeiDepChange->getLayerDependencyInfoPresentFlag(i) );
        if( pcOldScalableSeiDepChange->getLayerDependencyInfoPresentFlag(i) )
        {
          uiNumDireDepLyrs = pcOldScalableSeiDepChange->getNumDirectDependentLayers(i);
          pcNewScalableSeiDepChange->setNumDirectDependentLayers( j, uiNumDireDepLyrs );
          for ( k = 0; k < uiNumDireDepLyrs; k++)
            pcNewScalableSeiDepChange->setDirectDependentLayerIdDeltaMinus1(j, k, pcOldScalableSeiDepChange->getDirectDependentLayerIdDeltaMinus1(i, k) );
        }
        else
          pcNewScalableSeiDepChange->setLayerDependencyInfoSrcLayerIdDeltaMinus1(j, pcOldScalableSeiDepChange->getLayerDependencyInfoSrcLayerIdDeltaMinus1(i));

        j++;
      }
    }

    if ( j > 0 )
    {
      pcOldScalableSeiDepChange->setOutputFlag ( true );
      pcNewScalableSeiDepChange->setNumLayersMinus1( j - 1 );
      RNOK( xWriteScalableSEIToBuffer( (h264::SEI::SEIMessage*) pcNewScalableSeiDepChange, pcBinDataSEILysNotPreDepChange ) );
    }
    else
      pcOldScalableSeiDepChange->setOutputFlag ( false );
  }
// JVT-S080 LMI }
  //TMM_FIX
  RNOK( pcNewScalableSeiLayersNotPresent->destroy() );
  //TMM_FIX
  return Err::m_nOK;
}

ErrVal
Extractor::xExtractLayerLevel() // this function for extracting using "-sl, -l, -t, -f, -b" and its permittable combination
{
  UInt uiWantedScalableLayer = m_pcExtractorParameter->getScalableLayer();
  UInt uiMaxLayer            = m_pcExtractorParameter->getLayer();
  UInt uiMaxTempLevel        = m_pcExtractorParameter->getLevel();
  UInt uiMaxFGSLayer         = m_pcExtractorParameter->getFGSLayer();
  Double dMaxFGSLayer        = uiMaxFGSLayer;
  UInt uiMaxBitrate          = (UInt) m_pcExtractorParameter->getBitrate();
  if( uiMaxBitrate != MSYS_UINT_MAX )
    uiMaxBitrate *= 1000; //change to bps
  UInt uiKeepScalableLayer   = 0;
  UInt uiDecreaseBitrate     = MSYS_UINT_MAX;
// JVT-T073 {
  Bool bNextSuffix = false;
// JVT-T073 }

  h264::SEI::NonRequiredSei* pcNonRequiredDescription = NULL;
  if( uiMaxLayer != MSYS_UINT_MAX || uiMaxFGSLayer != 1000 || uiMaxTempLevel != MSYS_UINT_MAX )
  {
    //-l, -t, -f
    uiKeepScalableLayer = 0;
    if( m_pcExtractorParameter->getKeepfExtraction() ) //keepf is in use, re-calculation is not needed
    {
      for( UInt uiScalableLayer = 0; uiScalableLayer < m_cScalableStreamDescription.getNumOfScalableLayers(); uiScalableLayer++ )
      {
        if( ( m_cScalableStreamDescription.getTempLevel( uiScalableLayer )    <= uiMaxTempLevel )
          &&( m_cScalableStreamDescription.getDependencyId( uiScalableLayer ) <= uiMaxLayer ||
            ( m_cScalableStreamDescription.getDependencyId( uiScalableLayer ) == uiMaxLayer &&
              m_cScalableStreamDescription.getFGSLevel( uiScalableLayer )     <= uiMaxFGSLayer )
            )
          )
         uiKeepScalableLayer++;
      }
    }
    else // -l -t -f without -keepf, need to re-calculate bitrate
    {
      Bool *bUnchanged = 0;
      if( uiMaxFGSLayer < MAX_QUALITY_LEVELS ) //using -f option, need to re-calculate bitrate
        bUnchanged = new Bool [m_cScalableStreamDescription.getNumOfScalableLayers()];
      for ( UInt uiScalableLayer = 0; uiScalableLayer < m_cScalableStreamDescription.getNumOfScalableLayers(); uiScalableLayer++ )
      {
        if( uiMaxFGSLayer < MAX_QUALITY_LEVELS )
          bUnchanged[uiScalableLayer] = false; //default value
        UInt uiLayer = m_auiDependencyId[uiScalableLayer]; //m_cScalableStreamDescription.getDependencyId( uiScalableLayer );
        UInt uiTempLevel = m_auiTempLevel[uiScalableLayer]; //m_cScalableStreamDescription.getTempLevel( uiScalableLayer );
        UInt uiQualityLevel = m_auiQualityLevel[uiScalableLayer]; //m_cScalableStreamDescription.getFGSLevel( uiScalableLayer );
        if( uiLayer <= uiMaxLayer && uiQualityLevel <= uiMaxFGSLayer && uiTempLevel <= uiMaxTempLevel   )
        {
          uiKeepScalableLayer++;
          if( uiMaxFGSLayer < MAX_QUALITY_LEVELS ) // re-calculate
          {
            bUnchanged[uiScalableLayer] = true; //keep current scalable layer
            if( m_acDepLayerList[uiScalableLayer].size() ) //check dependent layers
            {
              Double dTmpBR = 0; //store re-calculated bitrate
              std::list<UInt>::iterator iIter     = m_acDepLayerList[uiScalableLayer].begin();
              std::list<UInt>::iterator iIterEnd  = m_acDepLayerList[uiScalableLayer].end();
              while( iIter != iIterEnd )
              {
                UInt uiTmpSL = (*iIter);
                if( bUnchanged[uiTmpSL] == true ) //dependent layer not  exist
                {
                  UInt uiL = m_auiDependencyId[uiTmpSL]; //m_cScalableStreamDescription.getDependencyId( uiTmpSL );
                  UInt uiTL = m_auiTempLevel[uiTmpSL]; //m_cScalableStreamDescription.getTempLevel( uiTmpSL );
                  UInt uiQL = m_auiQualityLevel[uiTmpSL]; //m_cScalableStreamDescription; //??????????????
                  Double dFactor = m_cScalableStreamDescription.getNumPictures( uiMaxLayer, uiTL )
                   /m_cScalableStreamDescription.getFrameRate(uiMaxLayer, uiTL )
                   *m_cScalableStreamDescription.getFrameRate(uiMaxLayer, uiTempLevel )
                   /m_cScalableStreamDescription.getNumPictures( uiMaxLayer, uiTempLevel ); //this factor represents the factor of different TL to current TL
                  dTmpBR += m_aaadSingleBitrate[uiL][uiTL][uiQL] * dFactor;
                } //bUnchanged
                iIter++;
              } // while
              m_adTotalBitrate[uiScalableLayer] = dTmpBR + m_aaadSingleBitrate[uiLayer][uiTempLevel][uiQualityLevel];
            } //if list size
          } // if uiMaxFGSLayer
        } //if all <=
      } //for
      if( uiMaxFGSLayer != 1000 )
        delete[] bUnchanged;
    } //else
  }
  else
  {
    // -sl, -b
    uiKeepScalableLayer = 0;
    if( uiWantedScalableLayer != MSYS_UINT_MAX && uiWantedScalableLayer >= m_cScalableStreamDescription.getNumOfScalableLayers() )
    {
      //specific scalable layer
      uiWantedScalableLayer = m_cScalableStreamDescription.getNumOfScalableLayers()-1;
    }
    else if( uiMaxBitrate != MSYS_UINT_MAX )
    {
      if( uiMaxBitrate < (UInt)(m_adTotalBitrate[0]+0.5) )
      {
        WARNING( true, "Bit-rate underflow for allowable minimum bitrate! Scalable layer 0 is determined to be output.\n" );
        uiWantedScalableLayer = 0;
      }
      else
      {
        for( UInt uiScalableLayer = 0; uiScalableLayer < m_cScalableStreamDescription.getNumOfScalableLayers(); uiScalableLayer++ )
        {
          if( uiMaxBitrate < (UInt)(m_adTotalBitrate[uiScalableLayer]+0.5) )
          {
            uiWantedScalableLayer = uiScalableLayer-1;
            break;
          }
        }
      }
    }
    else if( m_uiExtractNonRequiredPics != MSYS_UINT_MAX ) // non-required
      uiWantedScalableLayer = m_cScalableStreamDescription.getNumOfScalableLayers() - 1;
    uiKeepScalableLayer = (UInt)m_acDepLayerList[uiWantedScalableLayer].size()+1;
  }
  if( uiKeepScalableLayer == 0 )
  {
    printf(" The command leads to no scalable layers extracted!\n"
      " Extraction failed. Exit! \n\n" );
    exit(1);
  }

  Bool bTruncated = ( uiDecreaseBitrate != 0 && uiDecreaseBitrate != MSYS_UINT_MAX );
  if( bTruncated ) //-b
  {
    dMaxFGSLayer = uiDecreaseBitrate/( m_adTotalBitrate[uiWantedScalableLayer] - m_adTotalBitrate[uiWantedScalableLayer-1] );
    dMaxFGSLayer += m_auiQualityLevel[uiWantedScalableLayer-1];
  }

  UInt uiNumInput          = 0;
  UInt uiNumKept           = 0;
  UInt uiCropped           = 0;
  Bool bKeep               = false;
  Bool bApplyToNext        = false;
  Bool bEOS                = false;
  UInt uiLayer             = 0;
  UInt uiTempLevel         = 0;
  UInt uiFGSLayer          = 0;
  UInt uiPacketSize        = 0;
//prefix unit{{
  BinData*  pcBinData_prefix = NULL;
  Bool bDiscardable_prefix = false;
//prefix unit}}

  RNOK( m_pcH264AVCPacketAnalyzer->init() );

  BinData*  pcBinDataFrags[MAX_NUM_PD_FRAGMENTS];
  UInt      ui;
  for( ui = 0; ui < MAX_NUM_PD_FRAGMENTS; ui ++ )
    pcBinDataFrags[ui] = 0;

  while( ! bEOS )
  {
    UInt uiScalableLayer = 0;
    //========== get packet ==============
    BinData * pcBinData;
// JVT-S080 LMI {
    BinData * pcBinDataSEILysNotPreDepChange;
    ROT( NULL == ( pcBinDataSEILysNotPreDepChange = new BinData ) );
    Bool bWriteBinDataSEILysNotPreDepChange = false;
    Bool bWriteBinData = true;
// JVT-S080 LMI }

    RNOK( m_pcReadBitstream->extractPacket( pcBinData, bEOS ) );
    if( bEOS )
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
      continue;
    }
    //===== get packet description ======
    h264::SEI::SEIMessage*  pcScalableSEIMessage = 0;
    h264::PacketDescription cPacketDescription;
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSEIMessage ) );
//prefix unit{{
  if(cPacketDescription.NalUnitType == 14)
  {
    if(pcBinData_prefix)
      RNOK( m_pcReadBitstream->releasePacket( pcBinData_prefix ) );

    bDiscardable_prefix = cPacketDescription.bDiscardable;
    pcBinData_prefix = pcBinData;
    uiTempLevel = cPacketDescription.Level;
    continue;
  }
//prefix unit}}
    if( pcScalableSEIMessage )
    {
// JVT-S080 LMI {
      if( pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI || pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI_LAYERS_NOT_PRESENT || pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI_DEPENDENCY_CHANGE )
      {
        if( pcScalableSEIMessage->getMessageType() != h264::SEI::SCALABLE_SEI )
           bWriteBinData = false;
          RNOK( xChangeScalableSEIMessage( pcBinData, pcBinDataSEILysNotPreDepChange, pcScalableSEIMessage, uiKeepScalableLayer, uiWantedScalableLayer,
            uiMaxLayer, uiMaxTempLevel, dMaxFGSLayer, uiMaxBitrate ) );

        if( pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI )
        {
                  h264::SEI::ScalableSeiLayersNotPresent* pcScalableSeiLayersNotPresent = ( h264::SEI::ScalableSeiLayersNotPresent*) pcScalableSEIMessage;
          bWriteBinDataSEILysNotPreDepChange = pcScalableSeiLayersNotPresent->getOutputFlag();
        }
        if( pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI_LAYERS_NOT_PRESENT )
        {
                  h264::SEI::ScalableSeiLayersNotPresent* pcScalableSeiLayersNotPresent = ( h264::SEI::ScalableSeiLayersNotPresent*) pcScalableSEIMessage;
          bWriteBinDataSEILysNotPreDepChange = pcScalableSeiLayersNotPresent->getOutputFlag();
        }

        if( pcScalableSEIMessage->getMessageType() == h264::SEI::SCALABLE_SEI_DEPENDENCY_CHANGE )
        {
                  h264::SEI::ScalableSeiDependencyChange* pcScalableSeiDepChange = ( h264::SEI::ScalableSeiDependencyChange*) pcScalableSEIMessage;
          bWriteBinDataSEILysNotPreDepChange = pcScalableSeiDepChange->getOutputFlag();
        }

// JVT-S080 LMI }
        if( uiWantedScalableLayer != MSYS_UINT_MAX ) // -sl, -b
        {
          uiMaxLayer   = m_cScalableStreamDescription.getDependencyId( uiWantedScalableLayer );
          uiMaxTempLevel = m_cScalableStreamDescription.getTempLevel( uiWantedScalableLayer );
          uiMaxFGSLayer  = m_cScalableStreamDescription.getFGSLevel ( uiWantedScalableLayer );
        }
      }
    }

    delete pcScalableSEIMessage;
    pcNonRequiredDescription = m_pcH264AVCPacketAnalyzer->getNonRequiredSEI();
// JVT-S080 LMI {
    if( bWriteBinData )
    {
// JVT-S080 LMI }
      //============ get packet size ===========
      while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
      {
        RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove zero at end
      }


      //============ set parameters ===========
      if( ! bApplyToNext  )
      {
//prefix unit{{
        if(cPacketDescription.NalUnitType == 1 || cPacketDescription.NalUnitType == 5)
        {
          cPacketDescription.Level = uiTempLevel;
          cPacketDescription.bDiscardable = bDiscardable_prefix;
        }
//prefix unit}}
        uiLayer          = cPacketDescription.Layer;
        uiTempLevel      = cPacketDescription.Level;
        uiFGSLayer      = cPacketDescription.FGSLayer;
        uiScalableLayer = m_cScalableStreamDescription.getNumberOfScalableLayers( uiLayer,uiTempLevel, uiFGSLayer );
      }
      bApplyToNext = cPacketDescription.ApplyToNext;

      //============ check packet ===========
  //JVT-T054{
      if(m_pcExtractorParameter->getKeepfExtraction())
      {
        if( uiWantedScalableLayer == MSYS_UINT_MAX )  //input: -t,-f
        bKeep = ( (uiLayer < uiMaxLayer || (uiLayer == uiMaxLayer && uiFGSLayer <= uiMaxFGSLayer)) && uiTempLevel <= uiMaxTempLevel );
        else  //input: -sl,-b
        {
          if( uiTempLevel <= uiMaxTempLevel)
          {
            if( ( uiLayer == uiMaxLayer && uiFGSLayer <= uiMaxFGSLayer ) || uiLayer < uiMaxLayer )
              bKeep = true;
            else
              bKeep = false;
          }
          else
            bKeep = false;
        }
      }
      else
      {
        if( uiWantedScalableLayer == MSYS_UINT_MAX )  //input: -t,-f
          bKeep = ( uiLayer <= uiMaxLayer && uiFGSLayer <= uiMaxFGSLayer && uiTempLevel <= uiMaxTempLevel );
        else  //input: -sl,-b
        {
          if( uiTempLevel <= uiMaxTempLevel)
          {
            if( ( uiLayer == uiMaxLayer && uiFGSLayer <= uiMaxFGSLayer ) || uiLayer < uiMaxLayer )
              bKeep = true;
            else
              bKeep = false;
          }
          else
            bKeep = false;
        }
      }
  //JVT-T054}
      if(m_uiExtractNonRequiredPics != MSYS_UINT_MAX)
      {
        if(m_pcH264AVCPacketAnalyzer->getNonRequiredSeiFlag() == 1)
          bKeep = 0;
        if( m_uiExtractNonRequiredPics == 1 && pcNonRequiredDescription &&
          cPacketDescription.NalUnitType != NAL_UNIT_PPS &&
          cPacketDescription.NalUnitType != NAL_UNIT_SPS &&
          cPacketDescription.NalUnitType != NAL_UNIT_SUBSET_SPS &&
          cPacketDescription.NalUnitType != NAL_UNIT_SEI )
        {
          for(UInt i = 0; i <= pcNonRequiredDescription->getNumInfoEntriesMinus1(); i++)
          {
            if(pcNonRequiredDescription->getEntryDependencyId(i))  // it should be changed to if(DenpendencyId == LayerId of the shown picture)
            {
              for(UInt j = 0; j <= pcNonRequiredDescription->getNumNonRequiredPicsMinus1(i); j++)
              {
                if(cPacketDescription.Layer == pcNonRequiredDescription->getNonRequiredPicDependencyId(i,j) &&
                  cPacketDescription.FGSLayer == pcNonRequiredDescription->getNonRequiredPicQulityLevel(i,j))  // it should be add something about FragmentFlag
                {
                  bKeep = 0;
                }
              }
            }
          }
        }
      }

      UInt eNalUnitType = cPacketDescription.NalUnitType;
      Bool bRequired = false;
      if(  eNalUnitType == NAL_UNIT_SPS )
      {
        for( UInt layer = 0; layer <= uiMaxLayer; layer ++ )
        {
          if( m_cScalableStreamDescription.m_bSPSRequired[layer][cPacketDescription.SPSid] )
          {
            bRequired = true;
            break;
          }
        }
        bKeep = bRequired;
      }
      else if( eNalUnitType == NAL_UNIT_SUBSET_SPS )
      {
        for( UInt layer = 0; layer <= uiMaxLayer; layer ++ )
        {
          if( m_cScalableStreamDescription.m_bSubsetSPSRequired[layer][cPacketDescription.SPSid] )
          {
            bRequired = true;
            break;
          }
        }
        bKeep = bRequired;
      }
      else if( eNalUnitType == NAL_UNIT_PPS )
      {
        for( UInt layer = 0; layer <= uiMaxLayer; layer ++ )
        {
          if( m_cScalableStreamDescription.m_bPPSRequired[layer][cPacketDescription.PPSid] )
          {
            bRequired = true;
            break;
          }
        }
        bKeep = bRequired;
      }

      uiNumInput++;
      if( bKeep )   uiNumKept++;
  //JVT-T073 {
      if( bKeep && bNextSuffix ) uiNumKept++; //consider next suffix NAL unit
      if(bNextSuffix) uiNumInput++;
  //JVT-T073 }

      //============ write and release packet ============
      if( bKeep )
      {
//prefix unit{{
        if((cPacketDescription.NalUnitType == 1 || cPacketDescription.NalUnitType == 5) && pcBinData_prefix != NULL )
        {
          RNOK( m_pcWriteBitstream->writePacket( &m_cBinDataStartCode ) );
          RNOK( m_pcWriteBitstream->writePacket( pcBinData_prefix ) );
          uiPacketSize  += 4 + pcBinData_prefix->size();
          RNOK( m_pcReadBitstream->releasePacket( pcBinData_prefix ) );
          pcBinData_prefix = NULL;
          uiNumKept++;
          uiNumInput++;
        }
//prefix unit}}
  //JVT-T054{
        if(uiFGSLayer > 0 && bTruncated )
        {
          bTruncated = false;
          bKeep = false;
        }
  //JVT-T054}
        //first check if truncated FGS layer
        if( cPacketDescription.NalUnitType != NAL_UNIT_PPS &&
            cPacketDescription.NalUnitType != NAL_UNIT_SPS &&
						cPacketDescription.NalUnitType != NAL_UNIT_SUBSET_SPS && //SSPS
            cPacketDescription.NalUnitType != NAL_UNIT_SEI )
        if( bTruncated && uiLayer == uiMaxLayer && uiFGSLayer == uiMaxFGSLayer )
        {
          Double dTempBitrate = m_adTotalBitrate[uiWantedScalableLayer] - m_adTotalBitrate[uiWantedScalableLayer-1];
          Double dTemp = ( (Double)uiMaxBitrate-m_adTotalBitrate[uiWantedScalableLayer-1] )/dTempBitrate;
          UInt uiSize  = (UInt)floor(pcBinData->size() * dTemp);
          uiSize = ( uiSize >= 25 ) ? uiSize : 25;
          pcBinData->decreaseEndPos( pcBinData->size() - uiSize );
          pcBinData->data()[pcBinData->size()-1]  |= 0x01; //trailing one
          uiCropped++;
        }

        RNOK( m_pcWriteBitstream->writePacket( &m_cBinDataStartCode ) );
        RNOK( m_pcWriteBitstream->writePacket( pcBinData ) );
        uiPacketSize  += 4 + pcBinData->size();
      }
//prefix unit{{
      else
      {
        if((cPacketDescription.NalUnitType == 1 || cPacketDescription.NalUnitType == 5) && pcBinData_prefix != NULL )
        {
          RNOK( m_pcReadBitstream->releasePacket( pcBinData_prefix ) );
          pcBinData_prefix = NULL;
          uiNumInput++;
        }
      }
//prefix unit}}

    }
    RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
    pcBinData = NULL;
// JVT-T073 {
    if( bNextSuffix )
    {
      RNOK( m_pcReadBitstream->extractPacket( pcBinData, bEOS ) );
      if( bEOS )
        continue;
      while( pcBinData->data()[ pcBinData->size() - 1 ] == 0x00 )
      {
        RNOK( pcBinData->decreaseEndPos( 1 ) ); // remove zero at end
      }
      if( bKeep && bWriteBinData )
      {
        RNOK( m_pcWriteBitstream->writePacket( &m_cBinDataStartCode ) );
        RNOK( m_pcWriteBitstream->writePacket( pcBinData ) );
        uiPacketSize += 4+pcBinData->size();
      }
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
    }
// JVT-T073 }

// JVT-S080 LMI {
    if( bWriteBinDataSEILysNotPreDepChange )
    {
      while( pcBinDataSEILysNotPreDepChange->data()[ pcBinDataSEILysNotPreDepChange->size() - 1 ] == 0x00 )
      {
        RNOK( pcBinDataSEILysNotPreDepChange->decreaseEndPos( 1 ) ); // remove zero at end
      }
      uiPacketSize  = 4 + pcBinDataSEILysNotPreDepChange->size();

      RNOK( m_pcWriteBitstream->writePacket( &m_cBinDataStartCode ) );
      RNOK( m_pcWriteBitstream->writePacket( pcBinDataSEILysNotPreDepChange ) );
    }
    RNOK( m_pcReadBitstream->releasePacket( pcBinDataSEILysNotPreDepChange ) );
    pcBinDataSEILysNotPreDepChange = NULL;
// JVT-S080 LMI }
  }

  for( ui = 0; ui < MAX_NUM_PD_FRAGMENTS; ui ++ )
  {
    if( pcBinDataFrags[ui] )
    {
      delete  pcBinDataFrags[ui];
      pcBinDataFrags[ui] = 0;
    }
  }


  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );
  printf("Total Packet Size: %d\n", uiPacketSize );

  printf("\n\nNumber of input packets :  %d\n"
             "Number of output packets:  %d\n", uiNumInput, uiNumKept );

  if( bTruncated )
  {
    printf( "The scalable layer %d is truncated.( Cropped packets: %d ) \n\n", uiWantedScalableLayer, uiCropped );
  }

  return Err::m_nOK;
}


// HS: packet trace
ErrVal
Extractor::xReadLineExtractTrace( const Char* pcFormatString,
                                  UInt*       puiStart,
                                  UInt*       puiLength )
{
  if( NULL != puiStart && NULL != puiLength )
  {
    //--- don't ask me why ----
    ROTR( 0 >= fscanf( m_pcExtractionTraceFile, pcFormatString, puiStart, puiLength ), Err::m_nInvalidParameter );
  }

  for( Int n = 0; n < 0x400; n++ )
  {
    ROTRS( '\n' == fgetc( m_pcExtractionTraceFile ), Err::m_nOK );
  }

  return Err::m_nERR;
}


// HS: packet trace
ErrVal
Extractor::xExtractTrace()
{
  Bool    bEOS            = false;
  Bool    bFirstPacket    = true;
  Int64   i64StartPos     = 0;
  Int64   i64EndPos       = 0;
  UInt    uiNextStart     = 0;
  UInt    uiNextLength    = 0;

  UInt    uiNumDiscarded  = 0;
  UInt    uiNumTruncated  = 0;
  UInt    uiNumKept       = 0;

  UChar*  pucPacketBuffer = new UChar[ m_uiMaxSize + 1 ];
  ROF( pucPacketBuffer );

  RNOK( m_pcH264AVCPacketAnalyzer->init() );

  RNOK( xReadLineExtractTrace( "",      NULL,          NULL ) );  // skip first line
  RNOK( xReadLineExtractTrace( "",      NULL,          NULL ) );  // skip second line
  RNOK( xReadLineExtractTrace( "%x %d", &uiNextStart,  &uiNextLength ) );

  while( ! bEOS )
  {
    //=========== get packet ===========
    BinData*  pcBinData;
    RNOK( m_pcReadBitstream->extractPacket( pcBinData, bEOS ) );
    if( bEOS )
    {
      RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
      pcBinData = NULL;
      continue;
    }

    //========== get packet description ==========
    h264::SEI::SEIMessage*  pcScalableSEIMessage = 0;
    h264::PacketDescription cPacketDescription;
    RNOK( m_pcH264AVCPacketAnalyzer->process( pcBinData, cPacketDescription, pcScalableSEIMessage ) );
    if( ! pcScalableSEIMessage )
    {
      if( bFirstPacket &&
          cPacketDescription.NalUnitType != NAL_UNIT_CODED_SLICE          &&
          cPacketDescription.NalUnitType != NAL_UNIT_CODED_SLICE_IDR      &&
          cPacketDescription.NalUnitType != NAL_UNIT_CODED_SLICE_SCALABLE   )
      {
        RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
        continue;
      }
    }
    else
    {
      bFirstPacket  = false;
      delete pcScalableSEIMessage;
    }

    if( ! cPacketDescription.ApplyToNext )
    {
      i64EndPos       = static_cast<ReadBitstreamFile*>( m_pcReadBitstream )->getFilePos();
      UInt  uiStart   = (UInt)( i64StartPos & 0xFFFFFFFF  );
      UInt  uiSize    = (UInt)( i64EndPos   - i64StartPos );
      i64StartPos     = i64EndPos;

      printf("PACKET 0x%08x (%6d)     ", uiStart, uiSize );

      //////////////////////////////////////////////////////////////////////////
      if( uiStart == uiNextStart )
      {
        //===== read packet =====
        ROT ( uiSize > m_uiMaxSize );
        RNOK( m_cLargeFile.seek( uiStart, SEEK_SET ) );
        UInt  uiReadSize = 0;
        RNOK( m_cLargeFile.read( pucPacketBuffer, uiSize, uiReadSize ) );
        ROF ( uiSize == uiReadSize );

        //===== modify last bytes if necessary ====
        if( uiSize < uiNextLength )
        {
          fprintf( stderr, "\nERROR: The packet at start pos. 0x%08x is shorter than %d bytes\n", uiNextStart, uiNextLength );
          RERR();
        }
        else if( uiSize > uiNextLength )
        {
          if( true /*cPacketDescription.FGSLayer == 0*/ )
          {
            fprintf( stderr, "\nERROR: The packet at start pos. 0x%08x is not truncatable\n", uiNextStart );
            RERR();
          }

          //===== truncate packet =====
          if( pcBinData->size() - uiSize + uiNextLength < 25 )
          {
            uiNextLength = 25 + uiSize - pcBinData->size();

            fprintf( stderr, "\nWARNING: The size of the packet at start pos. 0x%08x was increased to %d bytes\n", uiNextStart, uiNextLength );
          }

          pucPacketBuffer[uiNextLength-1] |= 0x01; // trailing one

          printf("truncated to %d bytes\n", uiNextLength );
          uiNumTruncated++;
        }
        else
        {
          printf("kept\n");
          uiNumKept++;
        }

        //===== write packet =====
        static_cast<WriteBitstreamToFile*>( m_pcWriteBitstream )->writePacket( pucPacketBuffer, uiNextLength );

        //===== get next traget packet ====
        if( xReadLineExtractTrace( "%x %d", &uiNextStart,  &uiNextLength ) != Err::m_nOK )
        {
          uiNextStart   = 0xFFFFFFFF;
          uiNextLength  = 1;
        }
      }
      else if( uiStart > uiNextStart )
      {
        fprintf( stderr, "\nERROR: It exists no packet with start pos. 0x%08x\n", uiNextStart );
        RERR();
      }
      else
      {
        printf("discarded\n");
        uiNumDiscarded++;
      }
    }

    RNOK( m_pcReadBitstream->releasePacket( pcBinData ) );
  }

  RNOK( m_pcH264AVCPacketAnalyzer->uninit() );

  delete pucPacketBuffer;

  printf("\n\n\n");
  printf("%d packets kept (%d truncated)\n", uiNumKept+uiNumTruncated, uiNumTruncated );
  printf("%d packets discarded\n", uiNumDiscarded );
  printf("\n");

  return Err::m_nOK;
}

ErrVal
Extractor::go_QL()
{
  //determine layer, level and rateTarget for output stream
  RNOK( xGetExtParameters() );

  //JVT-S043
  Bool bOrderedTopLayerTruncation = ( m_pcExtractorParameter->getQLExtractionMode()==ExtractorParameter::QL_EXTRACTOR_MODE_ORDERED? true : false );

  //search optimal quality for target rate
  QualityLevelSearch(bOrderedTopLayerTruncation);

  //extract NALs for optimal quality
  RNOK( xExtractPoints() );
  //RNOK(ExtractPointsFromRate());
  return Err::m_nOK;
}

Void Extractor::setQualityLevel()
{
  UInt uiLevel;
  UInt uiLayer;
  UInt uiNumImage;
  UInt uiRef = 0;
  UInt uiPID = 0;
  m_uiNbPID = 0;
  for(uiPID = 0; uiPID < MAX_SIZE_PID; uiPID++)
      m_auiPID[uiPID] = 0;


 // if(m_bQualityLevelInSEI == false) //JVT-W137
  {
    for(uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++)
    {
      for(uiNumImage = 0; uiNumImage < m_auiNbImages[uiLayer]; uiNumImage++)
      {
        for(uiLevel = 0; uiLevel < MAX_QUALITY_LEVELS; uiLevel++)
        {
          m_aaauiBytesForQualityLevel[uiLayer][uiLevel][uiNumImage] = 0;
        }
      }
    }
    for(uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++)
    {
      for(uiNumImage = 0; uiNumImage < m_auiNbImages[uiLayer]; uiNumImage++)
      {
        m_aaiNumLevels[uiLayer][uiNumImage] = 0;
        uiRef = 0;
        for(uiLevel = 0; uiLevel < MAX_QUALITY_LEVELS; uiLevel++)
        {
          if(m_aaadBytesForFrameFGS[uiLayer][uiLevel][uiNumImage]!=0)
            m_aaiNumLevels[uiLayer][uiNumImage]++;
          else
            break;
          m_aaauiBytesForQualityLevel[uiLayer][uiLevel][uiNumImage] = uiRef+(UInt)m_aaadBytesForFrameFGS[uiLayer][uiLevel][uiNumImage];
          uiRef += (UInt)m_aaadBytesForFrameFGS[uiLayer][uiLevel][uiNumImage];
          addPIDToTable((UInt)m_aaadPriorityLevel[uiLayer][uiLevel][uiNumImage]);//SEI changes update
        }
      }
    }
  }

  //ordering in decreasing order PID
  UInt uiTempPID;
  UInt uiEnd;
  for(uiEnd = m_uiNbPID; uiEnd > 0; uiEnd--)
  {
    for(uiPID = 1; uiPID < uiEnd; uiPID++)
    {
      if(m_auiPID[uiPID] > m_auiPID[uiPID-1])
      {
        uiTempPID = m_auiPID[uiPID-1];
        m_auiPID[uiPID-1] = m_auiPID[uiPID];
        m_auiPID[uiPID] = uiTempPID;
      }
    }
  }
}

UInt Extractor::GetWantedScalableLayer()
{
  UInt uiWantedScalableLayer = 0;
  for( Int iFGS = (Int)m_uiTruncateFGSLayer; iFGS >= 0; iFGS-- )
  {
    if( ( uiWantedScalableLayer = getScalableLayer( m_uiTruncateLayer, m_uiTruncateLevel, (UInt) iFGS ) ) != MSYS_UINT_MAX )
    {
      m_uiTruncateFGSLayer = (UInt) iFGS;
      break;
    }
  }
  return uiWantedScalableLayer;
}

ErrVal
Extractor::GetAndCheckBaseLayerPackets( Double& dRemainingBytes )
{
  UInt uiExtLayer = m_pcExtractorParameter->getLayer();
  UInt uiExtLevel = m_pcExtractorParameter->getLevel();
  //set base layer packets for all layers
  for( UInt uiLayer = 0; uiLayer <= uiExtLayer; uiLayer++ )
  for( UInt uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
  {
    Int64 i64NALUBytes                    = m_cScalableStreamDescription.getNALUBytes( uiLayer, uiLevel, 0 );
    //FIX_FRAG_CAVLC
    if (dRemainingBytes<(Double)i64NALUBytes)
    {
      // J.Reichel -> CGS and FGS supports (note this will work only if the uiLevel for the framerate doesn't change for the different layer)
      // not enough bit for a layer, if the previous layer was a CGS, then it should become the new max layer
      if( uiLayer > 0 &&
          m_pcExtractorParameter->getFrameWidth()  == m_cScalableStreamDescription.getFrameWidth (uiLayer-1) &&
          m_pcExtractorParameter->getFrameHeight() == m_cScalableStreamDescription.getFrameHeight(uiLayer-1)    )
      {
        uiExtLayer=uiLayer-1;
        m_pcExtractorParameter->setLayer( uiExtLayer );
        break;
      }
    }
    //~FIX_FRAG_CAVLC
    dRemainingBytes                      -= (Double)i64NALUBytes;
    m_aadTargetSNRLayer[uiLayer][uiLevel] = 0;
    m_pcExtractorParameter->setMaxFGSLayerKept(0);
    for(UInt uiNFrames = 0; uiNFrames < m_auiNbImages[uiLayer]; uiNFrames++)
    {
      if(m_aaiLevelForFrame[uiLayer][uiNFrames] <= (Int)uiExtLevel)
      {
          m_aaadTargetBytesFGS[uiLayer][0][uiNFrames] = m_aaadBytesForFrameFGS[uiLayer][0][uiNFrames];
      }
    }
  }
  if( dRemainingBytes < 0.0 )
  {
    WARNING( true, "Bit-rate underflow for extraction/inclusion point" );
    m_uiTruncateLayer = 0;
    m_uiTruncateLevel = 0;
    m_uiTruncateFGSLayer = 0; // 0 means no truncation of all FGS layers
    RNOK( xResetSLFGSBitrate( m_uiTruncateLayer, m_uiTruncateLevel, m_uiTruncateFGSLayer, 0 ) );
  }
  return Err::m_nOK;
}

ErrVal
Extractor::xGetExtParameters()
{
  UInt uiLayer,uiLevel;
  const MyList<ExtractorParameter::Point>&          rcExtList   = m_pcExtractorParameter->getExtractionList();
  ROT( rcExtList.size() != 1 );
  MyList<ExtractorParameter::Point>::const_iterator cIter       = rcExtList.begin ();
  MyList<ExtractorParameter::Point>::const_iterator cEnd        = rcExtList.end   ();
  const ExtractorParameter::Point&                  rcExtPoint  = *cIter;
  UInt                                              uiExtLayer  = MSYS_UINT_MAX;
  UInt                                              uiExtLevel  = MSYS_UINT_MAX;
  //----- layer -----
  //modif Nathalie
  for( uiLayer = m_cScalableStreamDescription.getNumberOfLayers()-1; (Int)uiLayer >= 0; uiLayer-- )
  {
    if( rcExtPoint.uiWidth  == m_cScalableStreamDescription.getFrameWidth (uiLayer) &&
        rcExtPoint.uiHeight == m_cScalableStreamDescription.getFrameHeight(uiLayer)    )
    {
      uiExtLayer = uiLayer;
      break;
    }
  } //~modif Nathalie
  ERROR( uiExtLayer==MSYS_UINT_MAX, "Spatial resolution of extraction/inclusion point not supported" );
  m_pcExtractorParameter->setLayer(uiExtLayer);
  m_pcExtractorParameter->setFrameWidth ( rcExtPoint.uiWidth  );
  m_pcExtractorParameter->setFrameHeight( rcExtPoint.uiHeight );
  //--- level ---
  for( uiLevel = 0; uiLevel <= MAX_DSTAGES; uiLevel++ )
  {
    if( rcExtPoint.dFrameRate == m_cScalableStreamDescription.getFrameRate(uiExtLayer, uiLevel) )
    {
      uiExtLevel = uiLevel;
      break;
    }
  }
  m_pcExtractorParameter->setLevel(uiLevel);
  ERROR( uiExtLevel==MSYS_UINT_MAX, "Temporal resolution of extraction/inclusion point not supported" );
  ERROR( uiExtLevel>m_cScalableStreamDescription.getMaxLevel(uiExtLayer), "Spatio-temporal resolution of extraction/inclusion point not supported" );
   //--- target number of bytes -----
  Double  dTargetNumExtBytes  = rcExtPoint.dBitRate / 8.0 * 1000.0 / m_cScalableStreamDescription.getFrameRate(uiExtLayer,uiExtLevel)  * (Double)m_cScalableStreamDescription.getNumPictures(uiExtLayer,uiExtLevel);
  m_pcExtractorParameter->setTargetRate(dTargetNumExtBytes);

  return Err::m_nOK;
}

ErrVal Extractor::QualityLevelSearch(Bool bOrderedTopLayerTrunc)
{
  UInt uiNFrames;
  UInt uiNumPictures;
  UInt uiLayer;
  Double rate;

  UInt uiExtLayer = m_pcExtractorParameter->getLayer();
  UInt uiExtLevel = m_pcExtractorParameter->getLevel();
  Double dRateConstraint = m_pcExtractorParameter->getTargetRate();
  UInt uiMaxLayers = uiExtLayer+1;

  // Getting min and max QualityLevel
  Double QualityLevelMin = 0;
  Double QualityLevelMax = 63;

  printf("Rate target: %.2lf\n", dRateConstraint);

  Double dInclLayersRate = CalculateSizeOfIncludedLayers(uiExtLevel, uiExtLayer);
  Double dTotBQRate = CalculateSizeOfBQLayers(uiExtLevel, uiExtLayer);
  Double dTotalRate = CalculateSizeOfMaxQuality(uiExtLevel, uiExtLayer);

  printf ("  - BQ Rate(of all Layers): %.2lf   Total Rate(of all Layers): %.2lf\n", dTotBQRate, dTotalRate);

  if( dRateConstraint <= dTotBQRate )
  {
    //Target rate <= BQ rate. We can't do any truncation!
    for( uiLayer = 0; uiLayer <=  uiExtLayer; uiLayer++ )
    {
      uiNumPictures = m_auiNbImages[uiLayer];
      for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
      {
        if(m_aaiLevelForFrame[uiLayer][uiNFrames] <= (Int)uiExtLevel)
        {
          m_aadTargetByteForFrame[uiLayer][uiNFrames] = m_aaauiBytesForQualityLevel[uiLayer][0][uiNFrames];
        }
      }
    }
    m_uiTruncateLayer = 0;
    m_uiTruncateLevel = 0;
    m_uiTruncateFGSLayer = 0;
    RNOK( xResetSLFGSBitrate( m_uiTruncateLayer, m_uiTruncateLevel, m_uiTruncateFGSLayer, 0 ) );
  }
  else if( dRateConstraint >= dTotalRate )
  {
    //Target rate >= Maximum rate. We can't do any truncation!
    for( uiLayer = 0; uiLayer <=  uiExtLayer; uiLayer++ )
    {
      uiNumPictures = m_auiNbImages[uiLayer];
      for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
      {
        if(m_aaiLevelForFrame[uiLayer][uiNFrames] <= (Int)uiExtLevel)
        {
          m_aadTargetByteForFrame[uiLayer][uiNFrames] = m_aaauiBytesForQualityLevel[uiLayer][ m_aaiNumLevels[uiLayer][uiNFrames]-1 ][uiNFrames];
        }
      }
    }
    m_uiTruncateLayer = uiExtLayer;
    m_uiTruncateLevel = uiExtLevel;
    m_uiTruncateFGSLayer = MAX_QUALITY_LEVELS - 1;
  }
  //Low Bitrate truncation performance seems to be poor!
  //Restrict the QL truncation to BR > BQ&Included layers
  else if(dInclLayersRate < dRateConstraint || uiExtLayer == 0)
  {

    //By default, consider all Quality levels of layers for truncation.
    UInt uiMinTruncLayer=0, uiMaxTruncLayer=uiExtLayer;

    //JVT-S043
    //If ordered truncation is being used, find the layer to be truncated.
    //FGS packets of layers above this are discarded. FGS packets of layers below this layer are kept.
    if(bOrderedTopLayerTrunc)
    {
      Double dMinRate = 0;
      Double dMaxRate = 0;
      UInt uiTruncLayer=0;
      for(uiTruncLayer = 0; uiTruncLayer<=uiExtLayer; uiTruncLayer++)
      {
        dMinRate = GetTotalRateForQualityLevel(QualityLevelMax, uiExtLevel, uiExtLayer, uiTruncLayer, uiTruncLayer);
        dMaxRate = GetTotalRateForQualityLevel(QualityLevelMin, uiExtLevel, uiExtLayer, uiTruncLayer, uiTruncLayer);

        if( dMinRate <= dRateConstraint && dMaxRate >= dRateConstraint )
        {
          break;
        }
      }
      uiMinTruncLayer = uiMaxTruncLayer = uiTruncLayer;

      printf ("  - Truncating FGS packets in Layer %d\n", uiMinTruncLayer);
      printf ("  - Min Rate of the Layer: %.2lf   Max Rate of the Layer: %.2lf\n", dMinRate, dMaxRate);
    }


    Double minRate = GetTotalRateForQualityLevel(QualityLevelMax, uiExtLevel, uiExtLayer, uiMinTruncLayer, uiMaxTruncLayer);
    Double maxRate = GetTotalRateForQualityLevel(QualityLevelMin, uiExtLevel, uiExtLayer, uiMinTruncLayer, uiMaxTruncLayer);

    // iteration loop
    Int iter;
    Int iterMax = 10;
    Double midQualityLevel;
    Double midRate;
    for(iter=0; iter<iterMax; iter++)
    {
      midQualityLevel = (QualityLevelMin+QualityLevelMax)/2;
      midRate = GetTotalRateForQualityLevel(midQualityLevel, uiExtLevel, uiExtLayer, uiMinTruncLayer, uiMaxTruncLayer);
      if (midRate > dRateConstraint)
      {
        QualityLevelMin = midQualityLevel;
        maxRate = midRate;
      }
      else
      {
        QualityLevelMax = midQualityLevel;
        minRate = midRate;
      }
    }

    printf("  - Rate generated: %.2lf   QualityLevel: %.2lf\n", minRate, QualityLevelMax);

    //truncation of NAL with prevPID > midQualityLevel > PID
    Double dTruncRate = GetTruncatedRate(QualityLevelMax, uiExtLevel, uiExtLayer, uiMinTruncLayer, uiMaxTruncLayer);
    Double dRatio = 1.0;
    if(dTruncRate) dRatio = (dRateConstraint - minRate)/(dTruncRate);
    printf("  - Trunc rate: %.2lf   Ratio: %.2lf\n",dTruncRate, dRatio);

    Double totalRate = 0.0;
    // set the rate for each frames
    for(uiLayer = 0; uiLayer < uiMaxLayers; uiLayer++)
    {
      uiNumPictures = m_auiNbImages[uiLayer];
      for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
      {
        if(m_aaiLevelForFrame[uiLayer][uiNFrames] <= (Int)uiExtLevel)
        {
          rate = GetImageRateForQualityLevelActual(uiLayer, uiNFrames,QualityLevelMax, dRatio, uiMinTruncLayer, uiMaxTruncLayer);
          totalRate += rate;
          m_aadTargetByteForFrame[uiLayer][uiNFrames] = rate;
        }
      }
    }

    m_uiQualityId = (UInt)QualityLevelMax;

    printf("  - Actual Rate generated: %.2lf   QualityLevel: %d\n", totalRate, m_uiQualityId);
  }
  //Do a temporal level based truncation for low bitrates.
  else
  {
    UInt uiLevel;
    UInt uiFGSLayer;

    //Calculate sum of level
    Double uiBytesOfLevelPerFGS[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
    Double uiSumBytesOfLevelPerFGS[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
    for( uiLayer = 0; uiLayer <  MAX_LAYERS; uiLayer++ )
    {
      for( uiLevel = 0; uiLevel < MAX_TEMP_LEVELS; uiLevel++ )
      {
        for( uiFGSLayer = 0; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++ )
        {
          uiSumBytesOfLevelPerFGS[uiLayer][uiLevel][uiFGSLayer] = 0;
          uiBytesOfLevelPerFGS[uiLayer][uiLevel][uiFGSLayer] = 0;
        }
      }
    }

    for( uiLayer = 0; uiLayer <  uiExtLayer; uiLayer++ )
    {
      uiNumPictures = m_auiNbImages[uiLayer];
      for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
      {
        for( uiFGSLayer = 1; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++ )
        {
          uiBytesOfLevelPerFGS[uiLayer][uiLevel][uiFGSLayer] = 0;
          uiSumBytesOfLevelPerFGS[uiLayer][uiLevel][uiFGSLayer] = 0;
          for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
          {
            if(m_aaiLevelForFrame[uiLayer][uiNFrames] == (Int)uiLevel)
            {
              Double uiRate = m_aaadBytesForFrameFGS[uiLayer][uiFGSLayer][uiNFrames];
              uiBytesOfLevelPerFGS[uiLayer][uiLevel][uiFGSLayer]+=uiRate;
              uiSumBytesOfLevelPerFGS[uiLayer][uiLevel][uiFGSLayer] += m_aaauiBytesForQualityLevel[uiLayer][uiFGSLayer][uiNFrames];
            }
          }
        }
      }
    }


    Double dRemainingBytes = dRateConstraint;
    for( uiLayer = 0; uiLayer <=  uiExtLayer; uiLayer++ )
    {
      uiNumPictures = m_auiNbImages[uiLayer];
      for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
      {
        if(m_aaiLevelForFrame[uiLayer][uiNFrames] <= (Int)uiExtLevel)
        {
          m_aadTargetByteForFrame[uiLayer][uiNFrames] = m_aaauiBytesForQualityLevel[uiLayer][0][uiNFrames];
          dRemainingBytes -= m_aaauiBytesForQualityLevel[uiLayer][0][uiNFrames];
        }
      }
    }
    if(dRemainingBytes > 0)
    {
      try{

        for( uiLayer = 0; uiLayer <  uiExtLayer; uiLayer++ )
        {
          uiNumPictures = m_auiNbImages[uiLayer];
          for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
          {
            for( uiFGSLayer = 1; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++ )
            {
              Int64 i64NALUBytes = (Int64)uiBytesOfLevelPerFGS[uiLayer][uiLevel][uiFGSLayer];

              if(i64NALUBytes > 0)
              {
                if( (Double)i64NALUBytes <= dRemainingBytes)
                {
                  dRemainingBytes                      -= (Double)i64NALUBytes;
                  for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
                  {
                    if(m_aaiLevelForFrame[uiLayer][uiNFrames] == (Int)uiLevel)
                    {
                      rate = m_aaauiBytesForQualityLevel[uiLayer][uiFGSLayer][uiNFrames];
                      if(rate != 0)
                        m_aadTargetByteForFrame[uiLayer][uiNFrames] = rate;
                      printf("  - Layer %d   Frame %d   Level %d   Rate %.2lf \n",uiLayer,uiNFrames,uiLevel,m_aadTargetByteForFrame[uiLayer][uiNFrames]);
                    }
                  }
                }
                else
                {
                  Double  dFGSLayer = dRemainingBytes / (Double)uiBytesOfLevelPerFGS[uiLayer][uiLevel][uiFGSLayer];
                  for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
                  {
                    if(m_aaiLevelForFrame[uiLayer][uiNFrames] == (Int)uiLevel)
                    {
                      rate = m_aaauiBytesForQualityLevel[uiLayer][uiFGSLayer-1][uiNFrames];
                      UInt uiTempRate = m_aaauiBytesForQualityLevel[uiLayer][uiFGSLayer][uiNFrames] - m_aaauiBytesForQualityLevel[uiLayer][uiNFrames][uiFGSLayer-1];
                      rate += dFGSLayer*uiTempRate;
                      m_aadTargetByteForFrame[uiLayer][uiNFrames] = rate;
                      printf("  - Layer %d   Frame %d   Level %d   Rate %.2lf \n",uiLayer,uiNFrames,uiLevel,rate);
                    }
                  }
                  throw ExtractStop();

                }
              }
            }
          }
        }

      }
      catch( ExtractStop ){}
    }
  }

  return Err::m_nOK;
}

Double Extractor::CalculateSizeOfBQLayers(UInt uiExtLevel, UInt uiExtLayer)
{
  Double dRate = 0;
  UInt uiLayer,uiNumPictures,uiNFrames;
  UInt uiQualityLevel = 0;

  for(uiLayer = 0; uiLayer <= uiExtLayer; uiLayer++)
  {
    uiNumPictures = m_auiNbImages[uiLayer];
    for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
    {
      if(m_aaiLevelForFrame[uiLayer][uiNFrames] <= (Int)uiExtLevel)
      {
        dRate += m_aaauiBytesForQualityLevel[uiLayer][uiQualityLevel][uiNFrames];
      }
    }
  }
  return dRate;
}
Double Extractor::CalculateSizeOfMaxQuality(UInt uiExtLevel, UInt uiExtLayer)
{
  Double dRate = 0;
  UInt uiLayer,uiNumPictures,uiNFrames;
  UInt uiQualityLevel = 0;

  for(uiLayer = 0; uiLayer <= uiExtLayer; uiLayer++)
  {
    uiNumPictures = m_auiNbImages[uiLayer];
    for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
    {
      uiQualityLevel = m_aaiNumLevels[uiLayer][uiNFrames]-1;

      if(m_aaiLevelForFrame[uiLayer][uiNFrames] <= (Int)uiExtLevel)
      {
        dRate += m_aaauiBytesForQualityLevel[uiLayer][uiQualityLevel][uiNFrames];
      }
    }
  }
  return dRate;
}
Double Extractor::CalculateSizeOfIncludedLayers(UInt uiExtLevel, UInt uiExtLayer)
{
  Double dRate = 0;
  UInt uiLayer,uiNumPictures,uiNFrames;
  UInt uiQualityLevel = 0;

  for(uiLayer = 0; uiLayer <= uiExtLayer; uiLayer++)
  {
    uiNumPictures = m_auiNbImages[uiLayer];
    for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
    {
      if(uiLayer < uiExtLayer) uiQualityLevel = m_aaiNumLevels[uiLayer][uiNFrames]-1;
      else                     uiQualityLevel = 0;

      if(m_aaiLevelForFrame[uiLayer][uiNFrames] <= (Int)uiExtLevel)
      {
        dRate += m_aaauiBytesForQualityLevel[uiLayer][uiQualityLevel][uiNFrames];
      }
    }
  }
  return dRate;
}

Double Extractor::GetTruncatedRate(Double dQuality, UInt uiExtLevel, UInt uiExtLayer, UInt uiMinTruncLayer, UInt uiMaxTruncLayer)
{
  UInt uiNFrames;
  UInt uiNumPictures;
  UInt uiLayer;
  Double dRate = 0;
  UInt uiPID, uiPrevPID;
  UInt uiPIDIndexInFrame;
  //calculate rate of nal with prevPID > quality > pid

  for(uiLayer = uiMinTruncLayer; uiLayer <= uiMaxTruncLayer; uiLayer++)
  {
    uiNumPictures = m_auiNbImages[uiLayer];
    for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
    {
      if(m_aaiLevelForFrame[uiLayer][uiNFrames] <= (Int)uiExtLevel)
      {
        uiPIDIndexInFrame = GetNearestPIDForQualityLevel(uiLayer,uiNFrames,dQuality);
        uiPID = (UInt)m_aaadPriorityLevel[uiLayer][uiPIDIndexInFrame][uiNFrames];//SEI changes update
        UInt uiIndex = getPIDIndex(uiPID);
        if(uiIndex >0 && uiPIDIndexInFrame > 0)
        {
          uiPrevPID = m_auiPID[uiIndex-1];
          if(uiPrevPID > dQuality && uiPID < dQuality)
          {
            dRate += (m_aaauiBytesForQualityLevel[uiLayer][uiPIDIndexInFrame][uiNFrames] -
              m_aaauiBytesForQualityLevel[uiLayer][uiPIDIndexInFrame-1][uiNFrames]);
          }
          else if( uiPrevPID < dQuality && uiPID < dQuality) // this layer not needed, reset it
          {
            m_uiTruncateLayer = uiLayer;
            m_uiTruncateLevel = m_aaiLevelForFrame[uiLayer][uiNFrames];
            m_uiTruncateFGSLayer = uiPIDIndexInFrame;
            m_aaadSingleBitrate[m_uiTruncateLayer][m_uiTruncateLevel][m_uiTruncateFGSLayer] = 0;
          }
          m_uiTruncateLayer = uiLayer;
          m_uiTruncateLevel = m_aaiLevelForFrame[uiLayer][uiNFrames];
          m_uiTruncateFGSLayer = uiPIDIndexInFrame;
          for( UInt uiFGS = m_uiTruncateFGSLayer+1; uiFGS < MAX_QUALITY_LEVELS; uiFGS++ )
            m_aaadSingleBitrate[m_uiTruncateLayer][m_uiTruncateLevel][uiFGS] = 0;
         }
      }
    }
  }

  return dRate;
}


Double Extractor::GetImageRateForQualityLevelActual(UInt uiLayer, UInt uiNumImage, Double QualityLevel,
                                                     Double dRatio,
                                                     UInt uiMinTruncLayer, UInt uiMaxTruncLayer)
{
  Double dRate = 0;
  UInt uiPID, uiPrevPID;
  UInt uiPIDIndexInFrame;

  dRate = GetImageRateForQualityLevel(uiLayer,uiNumImage,QualityLevel,
                                      uiMinTruncLayer, uiMaxTruncLayer);

  if(uiLayer >= uiMinTruncLayer && uiLayer<=uiMaxTruncLayer)
  {
    uiPIDIndexInFrame = GetNearestPIDForQualityLevel(uiLayer,uiNumImage,QualityLevel);
    uiPID = (UInt)m_aaadPriorityLevel[uiLayer][uiPIDIndexInFrame][uiNumImage];//SEI changes update
    UInt uiIndex = getPIDIndex(uiPID);
    if(uiIndex >0 && uiPIDIndexInFrame > 0)
    {
      uiPrevPID = m_auiPID[uiIndex-1];
      if(uiPrevPID > QualityLevel && uiPID < QualityLevel)
      {
        dRate = m_aaauiBytesForQualityLevel[uiLayer][uiPIDIndexInFrame-1][uiNumImage];
        dRate += dRatio*(m_aaauiBytesForQualityLevel[uiLayer][uiPIDIndexInFrame][uiNumImage] -
          m_aaauiBytesForQualityLevel[uiLayer][uiPIDIndexInFrame-1][uiNumImage]);
        m_uiTruncateLayer = uiLayer;
        m_uiTruncateLevel = m_aaiLevelForFrame[uiLayer][uiNumImage];
        m_uiTruncateFGSLayer = uiPIDIndexInFrame;
        Double dTime = m_cScalableStreamDescription.getNumPictures(m_uiTruncateLayer, m_uiTruncateLevel) /
          m_cScalableStreamDescription.getFrameRate( m_uiTruncateLayer, m_uiTruncateLevel);
        Double dDecBitrate = (1-dRatio) * (m_aaauiBytesForQualityLevel[uiLayer][uiPIDIndexInFrame][uiNumImage] -
          m_aaauiBytesForQualityLevel[uiLayer][uiPIDIndexInFrame-1][uiNumImage] ) * 8 / dTime;
        m_aaadSingleBitrate[m_uiTruncateLayer][m_uiTruncateLevel][m_uiTruncateFGSLayer] -= dDecBitrate;
      }
    }
  }

  return dRate;
}


Double Extractor::GetTotalRateForQualityLevel(double QualityLevel, UInt uiExtLevel, UInt uiExtLayer,
                                                     UInt uiMinTruncLayer, UInt uiMaxTruncLayer)
{
  UInt uiNFrames;
  UInt uiNumPictures;
  UInt uiLayer;

  Double sum=0;

  for(uiLayer = 0; uiLayer <= uiExtLayer; uiLayer++)
  {
    uiNumPictures = m_auiNbImages[uiLayer];
    for(uiNFrames = 0; uiNFrames < uiNumPictures; uiNFrames++)
    {
      if(m_aaiLevelForFrame[uiLayer][uiNFrames] <= (Int)uiExtLevel)
      {
        sum += GetImageRateForQualityLevel(uiLayer, uiNFrames, QualityLevel,
                                           uiMinTruncLayer, uiMaxTruncLayer);
      }
    }
  }

  return sum;
}


UInt Extractor::GetNearestPIDForQualityLevel(UInt uiLayer, UInt uiNumImage, Double QualityLevel)
{
    Int i = 0;
  Bool stop = true;
  while (i<m_aaiNumLevels[uiLayer][uiNumImage] && stop)
    {
			if(m_aaadPriorityLevel[uiLayer][i][uiNumImage]<=QualityLevel)//SEI changes update
        stop = false;
        i++;
    }
    i--;
    return i;
}


Double Extractor::GetImageRateForQualityLevel(UInt uiLayer, UInt uiNumImage, Double QualityLevel,
                                              UInt uiMinTruncLayer, UInt uiMaxTruncLayer)
{
  //JVT-S043
  if( uiLayer < uiMinTruncLayer ) QualityLevel = 0;      //Total bitrate for layers < uiMinTruncLayer
  else if( uiLayer > uiMaxTruncLayer ) QualityLevel = 63;//Bitrate corresponding to Base Quality (BQ) Level for Layer > uiMaxTruncLayer
  //else                                                 //Bitrate corresponding to the QualityLevel

  Int i = 0;
  //minimal rate for the frame (BL)
  Double rate = m_aaauiBytesForQualityLevel[uiLayer][0][uiNumImage];
  Bool stop = true;


  while (i<m_aaiNumLevels[uiLayer][uiNumImage] && stop)
    {
		if (m_aaadPriorityLevel[uiLayer][i][uiNumImage]>QualityLevel)//SEI changes update
    rate = m_aaauiBytesForQualityLevel[uiLayer][i][uiNumImage];
		if(m_aaadPriorityLevel[uiLayer][i][uiNumImage]<=QualityLevel)//SEI changes update
    stop = false;
    i++;
   }
  i--;
  //if target lambda is lower than min lambda of the frame, max rate of the frame is returned
	if(QualityLevel < m_aaadPriorityLevel[uiLayer][m_aaiNumLevels[uiLayer][uiNumImage]-1][uiNumImage])//SEI changes update
  {
    rate = m_aaauiBytesForQualityLevel[uiLayer][m_aaiNumLevels[uiLayer][uiNumImage]-1][uiNumImage];
  }

  if(rate > m_aaadMaxRate[uiLayer][uiNumImage])
    rate = m_aaadMaxRate[uiLayer][uiNumImage];

  return rate;
    }


UInt Extractor::getPIDIndex(UInt uiPID)
{
    UInt ui;
    for(ui=0; ui<m_uiNbPID; ui++)
    {
        if(m_auiPID[ui] == uiPID)
              return ui;
    }

    return MSYS_UINT_MAX;
}

Void Extractor::CalculateMaxRate(UInt uiLayer)
{
  UInt uiNumImage;
  UInt ui;
    for(uiNumImage = 0; uiNumImage < m_auiNbImages[uiLayer]; uiNumImage++)
    {
      UInt maxRate = 0;
      for(ui = 0; ui < MAX_QUALITY_LEVELS; ui++) //bug fix JV 02/11/06
      {
        maxRate += (UInt)m_aaadBytesForFrameFGS[uiLayer][ui][uiNumImage];
      }
      m_aaadMaxRate[uiLayer][uiNumImage] = maxRate;
    }
}

Void Extractor::setLevel(UInt                    uiLayer,
                      UInt                    uiLevel,
                      UInt                    uiNumImage)
{
     m_aaiLevelForFrame[uiLayer][uiNumImage] = uiLevel;
}

Void Extractor::setMaxRateDS(UInt          uiMaxRate,
                      UInt                    uiLayer,
                            UInt          uiNumImage)
{
     m_aaadMaxRate[uiLayer][uiNumImage] = (Double)uiMaxRate;

}

Void Extractor::addPacket(UInt                    uiLayer,
                      UInt                    uiNumImage,
                      UInt                    uiFGSLayer,
                      UInt                    uiNumBytes)
{
  m_aaadBytesForFrameFGS[uiLayer][uiFGSLayer][uiNumImage] += uiNumBytes;
}



ErrVal Extractor::go_SIP()
{
  RNOK( xCalcSIPBitrate() );
  RNOK( xSetParameters_SIP() );
  RNOK( xExtractPoints() );
  //RNOK(xExtractPoints_SIP());
  return Err::m_nOK;
}

ErrVal
Extractor::xCalcSIPBitrate()
{
  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  for( UInt uiLevel = 0; uiLevel <= MAX_DSTAGES; uiLevel++ )
  {
    Double dTime = m_cScalableStreamDescription.getNumPictures( uiLayer, uiLevel ) /
      m_cScalableStreamDescription.getFrameRate( uiLayer, uiLevel );
    for( UInt uiFGS = 0; uiFGS < MAX_QUALITY_LEVELS; uiFGS++ )
    {
      Double dDecBitrate = (Double)(Int64)m_cScalableStreamDescription.getNALUBytesNoUse(uiLayer, uiLevel, uiFGS ) * 8 / dTime;
      if( ( m_aaadSingleBitrate[uiLayer][uiLevel][uiFGS]-dDecBitrate ) < 0.05 * m_aaadSingleBitrate[uiLayer][uiLevel][uiFGS] )
        m_aaadSingleBitrate[uiLayer][uiLevel][uiFGS] = 0; //force the layer non-existing
      else
        m_aaadSingleBitrate[uiLayer][uiLevel][uiFGS] -= dDecBitrate;
    }
  }
  return Err::m_nOK;
}
ErrVal
Extractor::xSetParameters_SIP()
{
  RNOK( xGetExtParameters() );

  UInt   uiLayer, uiLevel, uiFGSLayer;
  UInt   uiExtLayer         = m_pcExtractorParameter->getLayer();
  UInt   uiExtLevel         = m_pcExtractorParameter->getLevel();
//JVT-T054{
  Bool  bQuit = false;
//JVT-T054}

  //=========== clear all ===========
  for( uiLayer = 0; uiLayer <  MAX_LAYERS;  uiLayer++ )
  for( uiLevel = 0; uiLevel <= MAX_DSTAGES; uiLevel++ )
  {
    m_aadTargetSNRLayer[uiLayer][uiLevel] = -1;
    m_aadTargetSNRLayerNoUse[uiLayer][uiLevel] = -1;
  }

  //===== get and set required base layer packets ======
  Double  dRemainingBytes     = m_pcExtractorParameter->getTargetRate();
  RNOK( GetAndCheckBaseLayerPackets( dRemainingBytes ) );
  if( dRemainingBytes < 0 )
    return Err::m_nOK;

  //===== set maximum possible bytes for included layers ======
  for( uiLayer = 0; uiLayer <  uiExtLayer; uiLayer++ )
  {
    for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
    {
      for( uiFGSLayer = 1; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++ )
      {
        Int64 i64NALUBytes = m_cScalableStreamDescription.getNALUBytes( uiLayer, uiLevel, uiFGSLayer );
        if( (Double)i64NALUBytes <= dRemainingBytes )
        {
          dRemainingBytes                      -= (Double)i64NALUBytes;
          m_aadTargetSNRLayer[uiLayer][uiLevel] = (Double)uiFGSLayer;
          m_pcExtractorParameter->setMaxFGSLayerKept(uiFGSLayer);
        }
        else
        {
          dRemainingBytes                      -= (Double)i64NALUBytes;
          m_aadTargetSNRLayer[uiLayer][uiLevel] = (Double)uiFGSLayer;
          m_pcExtractorParameter->setMaxFGSLayerKept(uiFGSLayer);
          bQuit = true;
        }
      }
    }
  }

//JVT-T054{
  if(bQuit)
  {
    m_uiTruncateLayer = uiExtLayer;
    m_uiTruncateLevel = uiExtLevel;
    m_uiTruncateFGSLayer = m_pcExtractorParameter->getMaxFGSLayerKept();

    //Then reset all above scalable layers' bitrate within the same layerId
    for( UInt uiTL = 0; uiTL < MAX_TEMP_LEVELS; uiTL++ )
    for( UInt uiQL = 1; uiQL < MAX_QUALITY_LEVELS; uiQL++ )
      m_aaadSingleBitrate[uiExtLayer][uiTL][uiQL] = 0;

    return Err::m_nOK;
  }
//JVT-T054}

  //===== set FGS layer for current layer =====
  for( uiFGSLayer = 1; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++ )
  {
    Int64 i64FGSLayerBytes = 0;
    for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
    {
      i64FGSLayerBytes += m_cScalableStreamDescription.getNALUBytes( uiExtLayer, uiLevel, uiFGSLayer );
    }
    if( (Double)i64FGSLayerBytes <= dRemainingBytes )
    {
      dRemainingBytes -= (Double)i64FGSLayerBytes;
      for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
      {
        m_aadTargetSNRLayer[uiExtLayer][uiLevel] = (Double)uiFGSLayer;
        m_pcExtractorParameter->setMaxFGSLayerKept(uiFGSLayer);
      }
    }
    else
    {
      for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
      {
        i64FGSLayerBytes = m_cScalableStreamDescription.getNALUBytes( uiExtLayer, uiLevel, uiFGSLayer );
        if( (Double)i64FGSLayerBytes <= dRemainingBytes )
        {
          dRemainingBytes -= (Double)i64FGSLayerBytes;
          m_aadTargetSNRLayer[uiExtLayer][uiLevel] = (Double)uiFGSLayer;
          m_pcExtractorParameter->setMaxFGSLayerKept(uiFGSLayer);
        }
        else
        {
          m_uiTruncateLayer = uiExtLayer;
          m_uiTruncateLevel = uiLevel;
          m_uiTruncateFGSLayer = uiFGSLayer;

          UInt uiTL;
          //reset all above layers' bitrate
          for( uiTL = uiLevel; uiTL < MAX_TEMP_LEVELS; uiTL++ )
            m_aaadSingleBitrate[uiExtLayer][uiTL][uiFGSLayer] = 0;
          for( uiTL = 0; uiTL < MAX_TEMP_LEVELS; uiTL++ )
          for( UInt uiFGS = m_uiTruncateFGSLayer+1; uiFGS < MAX_QUALITY_LEVELS; uiFGS++ )
            m_aaadSingleBitrate[uiExtLayer][uiTL][uiFGS] = 0;
          return Err::m_nOK;
        }
      }
    }
  }

  //===== set maximum possible bytes for no use frames in included layers, for SIP ======
  for( uiLayer = 0; uiLayer <  uiExtLayer; uiLayer++ )
  {
    for( uiLevel = 0; uiLevel <= uiExtLevel; uiLevel++ )
    {
      for( uiFGSLayer = 0; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++ )
      {
        Int64 i64NALUBytes = m_cScalableStreamDescription.getNALUBytesNoUse( uiLayer, uiLevel, uiFGSLayer );
        if( (Double)i64NALUBytes <= dRemainingBytes )
        {
          dRemainingBytes                      -= (Double)i64NALUBytes;
          m_aadTargetSNRLayerNoUse[uiLayer][uiLevel] = (Double)uiFGSLayer;
        }
        else
        {
          //====== set fractional FGS layer and exit =====
          Double  dFGSLayer = dRemainingBytes / (Double)i64NALUBytes;
          m_aadTargetSNRLayerNoUse[uiLayer][uiLevel] += dFGSLayer;
          // >>>>> fix >>>>>
          m_uiTruncateLayer = uiExtLayer;
          m_uiTruncateLevel = uiLevel;
          m_uiTruncateFGSLayer = uiFGSLayer;
          // <<<<< fix <<<<<
          return Err::m_nOK;
        }
      }
    }
  }

  m_uiTruncateLayer = uiExtLayer;
  m_uiTruncateLevel = uiExtLevel;
  m_uiTruncateFGSLayer = MAX_QUALITY_LEVELS - 1;
  WARNING( dRemainingBytes>0.0, "Bit-rate overflow for extraction/inclusion point" );

  return Err::m_nOK;
}

ScalableStreamDescription::ScalableStreamDescription()
: m_bInit     ( false )
, m_bAnalyzed ( false )
{
}

ScalableStreamDescription::~ScalableStreamDescription()
{
}

ErrVal
ScalableStreamDescription::init( h264::SEI::ScalableSei* pcScalableSei )
{
  ROT( m_bInit );

  //::memset( m_aaaauiPictureNALUBytes, 0x00, MAX_NUM_PICTURES * MAX_LAYERS * MAX_QUALITY_LEVELS * sizeof(UInt) );

  ::memset( m_aaaui64NumNALUBytes,  0x00, MAX_LAYERS*(MAX_DSTAGES+1)*MAX_QUALITY_LEVELS*sizeof(UInt64) );
  ::memset( m_aaaui64NumNALUBytesNoUse, 0x00, MAX_LAYERS*(MAX_DSTAGES+1)*MAX_QUALITY_LEVELS*sizeof(UInt64) );
  ::memset( m_aauiNumPictures,      0x00, MAX_LAYERS*(MAX_DSTAGES+1)                   *sizeof(UInt)   );

  ::memset( m_auiBitrate,           0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_auiTempLevel,         0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_auiDependencyId,      0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_auiQualityLevel,      0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_adFramerate,          0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_auiFrmWidth,          0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_auiFrmHeight,         0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );

  ::memset( m_aaauiScalableLayerId, 0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_aaauiBitrate,         0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_aaauiTempLevel,       0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_aaauiDependencyId,    0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_aaauiQualityLevel,    0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_aaadFramerate,        0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_aaauiFrmWidth,        0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );
  ::memset( m_aaauiFrmHeight,       0x00, MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS*sizeof(UInt)   );

  m_uiScalableNumLayersMinus1 = pcScalableSei->getNumLayersMinus1();
  UInt m_uiMaxWidth;
  UInt m_uiMaxHeight;
  //if( pcScalableSei->getDecodingDependencyInfoPresentFlag( m_uiScalableNumLayersMinus1 ) )  //JVT-S036 lsj
  //{
    m_uiNumLayers     = pcScalableSei->getDependencyId( m_uiScalableNumLayersMinus1 ) + 1;
    m_uiMaxDecStages  = pcScalableSei->getTemporalId( m_uiScalableNumLayersMinus1 ) + 1;
  //}
  //if( pcScalableSei->getFrmSizeInfoPresentFlag( m_uiScalableNumLayersMinus1 ) )
  if( pcScalableSei->getFrmSizeInfoPresentFlag( m_uiScalableNumLayersMinus1 ) || pcScalableSei->getIroiSliceDivisionInfoPresentFlag(m_uiScalableNumLayersMinus1) )//SEI changes update
	{
    m_uiMaxWidth      = ( pcScalableSei->getFrmWidthInMbsMinus1 ( m_uiScalableNumLayersMinus1 ) + 1 ) << 4;
    m_uiMaxHeight     = ( pcScalableSei->getFrmHeightInMbsMinus1( m_uiScalableNumLayersMinus1 ) + 1 ) << 4;
  }

  m_bAVCBaseLayer = true;

  UInt uiLayer = 0;
  UInt uiLevel = 0;
  UInt uiFGS   = 0;
  for ( UInt uiScalableLayer = 0; uiScalableLayer <= m_uiScalableNumLayersMinus1; uiScalableLayer++ )
  {
    uiLayer = pcScalableSei->getDependencyId ( uiScalableLayer );
    uiLevel = pcScalableSei->getTemporalId( uiScalableLayer );
    uiFGS   = pcScalableSei->getQualityId ( uiScalableLayer );

    m_auiFrameWidth [ uiLayer ] = pcScalableSei->getFrmSizeInfoPresentFlag( uiScalableLayer ) ?
                                ( pcScalableSei->getFrmWidthInMbsMinus1    (uiScalableLayer)+1 ) << 4 : 0;
    m_auiFrameHeight[ uiLayer ] = pcScalableSei->getFrmSizeInfoPresentFlag( uiScalableLayer ) ?
                                ( pcScalableSei->getFrmHeightInMbsMinus1  (uiScalableLayer)+1 ) << 4 : 0;

    if(uiFGS == 0) //MGS_FIX_FT_09_2007
    m_auiDecStages  [ uiLayer ] = pcScalableSei->getTemporalId( uiScalableLayer );//JVT-S036 lsj
    m_aaauiScalableLayerId[uiLayer][uiLevel][uiFGS] = uiScalableLayer;
    m_aaauiTempLevel      [uiLayer][uiLevel][uiFGS] = uiLevel;
    m_aaauiDependencyId   [uiLayer][uiLevel][uiFGS] = uiLayer;
    m_aaauiQualityLevel   [uiLayer][uiLevel][uiFGS] = uiFGS;

    m_aaadFramerate        [uiLayer][uiLevel][uiFGS] = pcScalableSei->getFrmRateInfoPresentFlag( uiScalableLayer ) ?
                                        pcScalableSei->getAvgFrmRate( uiScalableLayer )/256.0 : 0;
    m_aaauiBitrate         [uiLayer][uiLevel][uiFGS] = pcScalableSei->getBitrateInfoPresentFlag( uiScalableLayer ) ?
                                        pcScalableSei->getAvgBitrateBPS( uiScalableLayer ) : 0;
    m_aaauiFrmWidth        [uiLayer][uiLevel][uiFGS] = pcScalableSei->getFrmSizeInfoPresentFlag( uiScalableLayer ) ?
                                      ( pcScalableSei->getFrmWidthInMbsMinus1    ( uiScalableLayer ) + 1 ) << 4 : 0;
    m_aaauiFrmHeight       [uiLayer][uiLevel][uiFGS] = pcScalableSei->getFrmSizeInfoPresentFlag( uiScalableLayer ) ?
                                      ( pcScalableSei->getFrmHeightInMbsMinus1   ( uiScalableLayer ) + 1 ) << 4 : 0;

    m_adFramerate      [ uiScalableLayer ] = m_aaadFramerate     [uiLayer][uiLevel][uiFGS];
    m_auiBitrate       [ uiScalableLayer ] = m_aaauiBitrate      [uiLayer][uiLevel][uiFGS];
    m_auiFrmWidth      [ uiScalableLayer ] = m_aaauiFrmWidth     [uiLayer][uiLevel][uiFGS];
    m_auiFrmHeight     [ uiScalableLayer ] = m_aaauiFrmHeight    [uiLayer][uiLevel][uiFGS];
    m_auiTempLevel     [ uiScalableLayer ] = m_aaauiTempLevel    [uiLayer][uiLevel][uiFGS];
    m_auiDependencyId  [ uiScalableLayer ] = m_aaauiDependencyId [uiLayer][uiLevel][uiFGS];
    m_auiQualityLevel  [ uiScalableLayer ] = m_aaauiQualityLevel [uiLayer][uiLevel][uiFGS];
  }

  UInt uiNum, uiIndex;
  for( uiIndex = 0; uiIndex <= uiLayer; uiIndex++ )
  {
    for( uiNum = 0; uiNum < 32; uiNum ++ )
      m_bSPSRequired[uiIndex][uiNum] = false;
    for( uiNum = 0; uiNum < 32; uiNum ++ )
      m_bSubsetSPSRequired[uiIndex][uiNum] = false;
    for( uiNum = 0; uiNum < 256; uiNum ++ )
      m_bPPSRequired[uiIndex][uiNum] = false;
  }

  m_bInit = true;
  m_bAnalyzed = false;
  return Err::m_nOK;
}

ErrVal
ScalableStreamDescription::uninit()
{
  m_bInit     = false;
  m_bAnalyzed = false;

  return Err::m_nOK;
}

ErrVal
ScalableStreamDescription::addPacket( UInt  uiNumBytes,
                                      UInt  uiLayer,
                                      UInt  uiLevel,
                                      UInt  uiFGSLayer,
                                      Bool  bNewPicture )
{
  ROF( m_bInit      );
  ROT( m_bAnalyzed  );
  ROF( uiLayer    <  MAX_LAYERS         );
  ROF( uiLevel    <= MAX_DSTAGES        );
  ROF( uiFGSLayer <  MAX_QUALITY_LEVELS );


  static UInt uiPictureIdx = 0;

  //m_aaaauiPictureNALUBytes[uiPictureIdx][uiLayer][uiFGSLayer] += uiNumBytes;
  if( bNewPicture && uiFGSLayer == 0 )
    uiPictureIdx ++;

  m_aaaui64NumNALUBytes[uiLayer][uiLevel][uiFGSLayer] += uiNumBytes;

  return Err::m_nOK;
}

//S051{
ErrVal
ScalableStreamDescription::addPacketNoUse( UInt  uiNumBytes,
                                      UInt  uiLayer,
                                      UInt  uiLevel,
                                      UInt  uiFGSLayer,
                                      Bool  bNewPicture )
{
  ROF( m_bInit      );
  ROT( m_bAnalyzed  );
  ROF( uiLayer    <  MAX_LAYERS         );
  ROF( uiLevel    <= MAX_DSTAGES        );
  ROF( uiFGSLayer <  MAX_QUALITY_LEVELS );

  m_aaaui64NumNALUBytesNoUse[uiLayer][uiLevel][uiFGSLayer] += uiNumBytes;

  return Err::m_nOK;
}
//S051}


ErrVal ScalableStreamDescription::addPic( h264::PacketDescription& rcPacketDescription )
{
  if((rcPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE ||
      rcPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_IDR ) &&
      rcPacketDescription.uiFirstMb   == 0 )
  {
    m_aauiNumPictures[0][rcPacketDescription.Level]++;
  }
  if( rcPacketDescription.NalUnitType == NAL_UNIT_CODED_SLICE_SCALABLE &&
      rcPacketDescription.FGSLayer    == 0 &&
      rcPacketDescription.uiFirstMb   == 0 )
  {
    m_aauiNumPictures[rcPacketDescription.Layer][rcPacketDescription.Level]++;
  }
  return Err::m_nOK;
}

ErrVal
ScalableStreamDescription::analyse()
{
  ROF( m_bInit );

  UInt uiLayer, uiLevel;
  for(uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++)
  {
      for(uiLevel = 1; uiLevel <=MAX_DSTAGES; uiLevel++)
      {
          m_aauiNumPictures[uiLayer][uiLevel] += m_aauiNumPictures[uiLayer][uiLevel-1];
      }
  }
  m_bAnalyzed = true;

  return Err::m_nOK;
}

Void
Extractor::xOutput( )
{
  printf("\nContained Layers:");
  printf("\n====================\n\n");

  printf( "       Layer" "   Resolution" "   Framerate" "   Bitrate"" MinBitrate" "      DTQ\n" );
    for ( UInt uiScalableLayer = 0; uiScalableLayer <= m_uiScalableNumLayersMinus1; uiScalableLayer++ )
  {
    UInt uiDependencyId     = m_auiDependencyId         [uiScalableLayer];
    UInt uiTempLevel        = m_auiTempLevel            [uiScalableLayer];
    UInt uiQualityLevel     = m_auiQualityLevel         [uiScalableLayer];
    UInt uiFrameWidth       = m_auiFrmWidth             [uiScalableLayer];
    UInt uiFrameHeight      = m_auiFrmHeight            [uiScalableLayer];
    Double dFrameRate       = m_adFramerate             [uiScalableLayer];
    Double dBitrate         = m_adTotalBitrate          [uiScalableLayer];

    if( uiQualityLevel)
      printf( "       %3d     %3dx%3d      %7.4lf   %8.2lf               (%d,%d,%d) \n",
      uiScalableLayer,uiFrameWidth, uiFrameHeight, dFrameRate, dBitrate/1000.0, uiDependencyId, uiTempLevel, uiQualityLevel );
    else //Q = 0
      printf( "       %3d     %3dx%3d      %7.4lf   %8.2lf  %10.2lf   (%d,%d,%d) \n",
      uiScalableLayer,uiFrameWidth, uiFrameHeight, dFrameRate, dBitrate/1000.0, m_aadMinBitrate[uiDependencyId][uiTempLevel]/1000.0, uiDependencyId, uiTempLevel, uiQualityLevel );

    //JVT-W046 {
		//SEI changes update {
    //Bool   bAvcLayerConvFlag  =  m_bAvc_Layer_Conversion_Flag[uiScalableLayer];
    //if ( bAvcLayerConvFlag )
    //{
    //  UInt   uiAvcConvType     =  m_uiAvc_Conversion_Type_Idc[uiScalableLayer];
    //  for ( UInt uiType = 0; uiType < 2; uiType++ )
    //  {
    //    Bool   bAvcInfoFlag      =  m_bAvc_Info_Flag[uiScalableLayer][uiType];
    //    if(bAvcInfoFlag)
    //    {
    //      UInt  uiAvcProfileLevel  =  m_uiAvc_Profile_Level_Idc[uiScalableLayer][uiType];
    //      UInt   dAvcAvgBitRate    =  m_dAvc_Avg_Bitrate[uiScalableLayer][uiType];
    //      UInt   dAvcMaxBitRate    =  m_dAvc_Max_Bitrate[uiScalableLayer][uiType];
    //      printf( "\n AvcConvType" "   AvcProfileLevel" "   AvcAvgBitrate" "   AvcMaxBitrate\n");
    //      printf( "      %3d                %7d                 %11d                %11d\n",
    //        uiAvcConvType, uiAvcProfileLevel, dAvcAvgBitRate, dAvcMaxBitRate );
    //    }
    //  }
    //}
		Bool   bLayerConvFlag  =  m_bLayer_Conversion_Flag[uiScalableLayer];
    if ( bLayerConvFlag )
    {
      UInt   uiConvType     =  m_uiConversion_Type_Idc[uiScalableLayer];
      for ( UInt uiType = 0; uiType < 2; uiType++ )
      {
        Bool   bRewritingInfoFlag      =  m_bRewriting_Info_Flag[uiScalableLayer][uiType];
        if(bRewritingInfoFlag)
        {
          UInt  uiRewritingProfileLevel  =  m_uiRewriting_Profile_Level_Idc[uiScalableLayer][uiType];
          UInt   dRewritingAvgBitRate    =  m_dRewriting_Avg_Bitrate[uiScalableLayer][uiType]/1000;
          UInt   dRewritingMaxBitRate    =  m_dRewriting_Max_Bitrate[uiScalableLayer][uiType]/1000;
          printf( "\n ConvType" "   RewritingProfileLevel" "   RewritingAvgBitrate" "   RewritingMaxBitrate\n");
          printf( "      %3d                  %7d                      %11d                      %11d\n",
            uiConvType, uiRewritingProfileLevel, dRewritingAvgBitRate, dRewritingMaxBitRate );
        }
      }
    }

    //JVT-W046 }
  }
  //SEI changes update }
}

Void
ScalableStreamDescription::output( )
{
  printf("\nContained Layers:");
  printf("\n====================\n\n");

  printf( "       Layer" "   Resolution" "   Framerate" "   Bitrate" "      DTQ\n" );
for ( UInt uiScalableLayer = 0; uiScalableLayer <= m_uiScalableNumLayersMinus1; uiScalableLayer++ )
  {
    UInt uiFrameWidth    = m_auiFrmWidth             [uiScalableLayer];
    UInt uiFrameHeight   = m_auiFrmHeight            [uiScalableLayer];
    Double dFrameRate    = m_adFramerate             [uiScalableLayer];
    Double dBitrate      = (Double)m_auiBitrate      [uiScalableLayer];
    UInt uiDependencyId  = m_auiDependencyId         [uiScalableLayer];
    UInt uiTempLevel     = m_auiTempLevel            [uiScalableLayer];
    UInt uiQualityLevel  = m_auiQualityLevel         [uiScalableLayer];

    printf( "       %3d     %3dx%3d      %7.4lf   %8.2lf     (%d,%d,%d) \n",
      uiScalableLayer,uiFrameWidth, uiFrameHeight, dFrameRate, dBitrate/1000.0, uiDependencyId, uiTempLevel, uiQualityLevel );
  }
  printf( "\n\n" );
}

//<-- Set ROI Parameters to extract ROI
void
Extractor::xSetROIParameters()
{
  init_ROI_ID();

  UInt m_uiNum_layers = m_pcH264AVCPacketAnalyzer->m_uiNum_layers;

  for(UInt ui=0; ui< m_uiNum_layers; ui++)
  {
    Int roi_id = m_pcH264AVCPacketAnalyzer->m_ID_ROI[ui];
    UInt dependency_Id = m_pcH264AVCPacketAnalyzer->m_ID_Dependency[ui];

    Int m_slice_group_id = m_pcH264AVCPacketAnalyzer->m_silceIDOfSubPicLayer[ui];

    if(m_slice_group_id !=-1)
      setROI_ID(dependency_Id,m_slice_group_id,roi_id );
  }
}

ErrVal
Extractor::GetPictureDataKeep( h264::PacketDescription* pcPacketDescription, Double dRemainingBytes, Double dCurrPacketBytes, Bool& bKeep )
{
  bKeep = false;
  UInt NalUnitType = pcPacketDescription->NalUnitType;
  UInt uiFGSLayer  = pcPacketDescription->FGSLayer;

  if( NalUnitType == NAL_UNIT_SEI )
  {
    bKeep = true;
    return Err::m_nOK;
  }
  if( NalUnitType == NAL_UNIT_SPS )
  {
    for( UInt layer = 0; layer <= m_pcExtractorParameter->getLayer(); layer ++ )
    {
      if( m_cScalableStreamDescription.m_bSPSRequired[layer][pcPacketDescription->SPSid] )
      {
        bKeep = true;
        break;
      }
    }
    return Err::m_nOK;
  }
  else if( NalUnitType == NAL_UNIT_SUBSET_SPS )
  {
    for( UInt layer = 0; layer <= m_pcExtractorParameter->getLayer(); layer ++ )
    {
      if( m_cScalableStreamDescription.m_bSubsetSPSRequired[layer][pcPacketDescription->SPSid] )
      {
        bKeep = true;
        break;
      }
    }
    return Err::m_nOK;
  }
  else if( NalUnitType == NAL_UNIT_PPS )
  {
    for( UInt layer = 0; layer <= m_pcExtractorParameter->getLayer(); layer ++ )
    {
      if( m_cScalableStreamDescription.m_bPPSRequired[layer][pcPacketDescription->PPSid] )
      {
        bKeep = true;
        break;
      }
    }
    return Err::m_nOK;
  }
  else	//not SPS, PPS, SEI
  {
    if( dRemainingBytes <= 0 )
    {
      bKeep = false;
    }
    else if( dRemainingBytes >= dCurrPacketBytes )
    {
      bKeep = true;
    }
    else if( uiFGSLayer > 0 )
    {
      bKeep = false;
    }
  }

  return Err::m_nOK;
}


ErrVal
Extractor::xResetSLFGSBitrate( UInt uiDependencyId, UInt uiTempLevel, UInt uiFGSLayer, Double dDecBitrate )
{
  if( uiFGSLayer == 0 ) //no truncation of all FGS layers
  {
    for( UInt uiDID=uiDependencyId; uiDID < MAX_LAYERS; uiDID++ )
    for( UInt uiTL=uiTempLevel; uiTL < MAX_TEMP_LEVELS; uiTL++)
    for( UInt uiFGS=1; uiFGS < MAX_QUALITY_LEVELS; uiFGS++ )
      m_aaadSingleBitrate[uiDID][uiTL][uiFGS] = 0;
    return Err::m_nOK;
  }
  m_aaadSingleBitrate[uiDependencyId][uiTempLevel][uiFGSLayer] -= dDecBitrate;

  //Then reset all following FGS layer bitrate
  for( UInt uiTL=uiTempLevel; uiTL < MAX_TEMP_LEVELS; uiTL++)
  for( UInt uiFGS=uiFGSLayer+1; uiFGS < MAX_QUALITY_LEVELS; uiFGS++ )
    m_aaadSingleBitrate[uiDependencyId][uiTL][uiFGS] = 0;
  for( UInt uiDID=uiDependencyId+1; uiDID < MAX_LAYERS; uiDID++ )
  for( UInt uiTL=0; uiTL < MAX_TEMP_LEVELS; uiTL++)
  for( UInt uiFGS=1; uiFGS < MAX_QUALITY_LEVELS; uiFGS++ )
    m_aaadSingleBitrate[uiDID][uiTL][uiFGS] = 0;

  return Err::m_nOK;
}


#undef WARNING
#undef ERROR

//-->

