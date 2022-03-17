
#include <time.h>
#include "H264AVCEncoderLibTest.h"
#include "H264AVCEncoderTest.h"
#include "EncoderCodingParameter.h"
#include <cstdio>
// JVT-W043
#include "RateCtlBase.h"
#include "RateCtlQuadratic.h"
// JVT-V068 {
#include "H264AVCCommonLib/SequenceParameterSet.h"
// JVT-V068 }

H264AVCEncoderTest::H264AVCEncoderTest() :
  m_pcH264AVCEncoder        ( NULL ),
  m_pcWriteBitstreamToFile  ( NULL ),
  m_pcEncoderCodingParameter( NULL )
{
  ::memset( m_apcReadYuv,   0x00, MAX_LAYERS*sizeof(Void*) );
  ::memset( m_apcWriteYuv,  0x00, MAX_LAYERS*sizeof(Void*) );
  ::memset( m_auiLumOffset, 0x00, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_auiCbOffset,  0x00, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_auiCrOffset,  0x00, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_auiHeight,    0x00, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_auiWidth,     0x00, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_auiStride,    0x00, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_aauiCropping, 0x00, MAX_LAYERS*sizeof(UInt)*4);
}


H264AVCEncoderTest::~H264AVCEncoderTest()
{
}


ErrVal
H264AVCEncoderTest::create( H264AVCEncoderTest*& rpcH264AVCEncoderTest )
{
  rpcH264AVCEncoderTest = new H264AVCEncoderTest;

  ROT( NULL == rpcH264AVCEncoderTest );

  return Err::m_nOK;
}



ErrVal H264AVCEncoderTest::init( Int    argc,
                                 Char** argv )
{
  //===== create and read encoder parameters =====
  RNOK( EncoderCodingParameter::create( m_pcEncoderCodingParameter ) );
  if( Err::m_nOK != m_pcEncoderCodingParameter->init( argc, argv, m_cEncoderIoParameter.cBitstreamFilename ) )
  {
    m_pcEncoderCodingParameter->printHelp();
    return -3;
  }
  m_cEncoderIoParameter.nResult = -1;


  //===== init instances for reading and writing yuv data =====
  UInt uiNumberOfLayers = m_pcEncoderCodingParameter->getAVCmode() ? 1 : m_pcEncoderCodingParameter->getNumberOfLayers();
  for( UInt uiLayer = 0; uiLayer < uiNumberOfLayers; uiLayer++ )
  {
    h264::LayerParameters&  rcLayer = m_pcEncoderCodingParameter->getLayerParameters( uiLayer );

    RNOKS( WriteYuvToFile::create( m_apcWriteYuv[uiLayer] ) );
    RNOKS( m_apcWriteYuv[uiLayer]->init( rcLayer.getOutputFilename() ) );
    RNOKS( ReadYuvFile   ::create( m_apcReadYuv [uiLayer] ) );

    ReadYuvFile::FillMode eFillMode = ( rcLayer.isInterlaced() ? ReadYuvFile::FILL_FIELD : ReadYuvFile::FILL_FRAME );
#if DOLBY_ENCMUX_ENABLE
    if(m_pcEncoderCodingParameter->getMuxMethod() && uiNumberOfLayers>1)
    {
      h264::LayerParameters&  rcLayer0 = m_pcEncoderCodingParameter->getLayerParameters( 0 );
      RNOKS( m_apcReadYuv[uiLayer]->init( rcLayer.getInputFilename        (),
                                          rcLayer0.getFrameHeightInSamples (),
                                          rcLayer0.getFrameWidthInSamples  (), 0, MSYS_UINT_MAX, eFillMode ) );
    }
    else
    {
      RNOKS( m_apcReadYuv[uiLayer]->init( rcLayer.getInputFilename        (),
                                          rcLayer.getFrameHeightInSamples (),
                                          rcLayer.getFrameWidthInSamples  (), 0, MSYS_UINT_MAX, eFillMode ) );
    }
#else
    RNOKS( m_apcReadYuv[uiLayer]->init( rcLayer.getInputFilename        (),
                                        rcLayer.getFrameHeightInSamples (),
                                        rcLayer.getFrameWidthInSamples  (), 0, MSYS_UINT_MAX, eFillMode ) );
#endif
  }


  //===== init bitstream writer =====
  if( m_pcEncoderCodingParameter->getAVCmode() )
  {
    RNOKS( WriteBitstreamToFile::create   ( m_pcWriteBitstreamToFile ) );
    RNOKS( m_pcWriteBitstreamToFile->init ( m_cEncoderIoParameter.cBitstreamFilename ) );
  }
  else
  {
    m_cWriteToBitFileTempName                 = m_cEncoderIoParameter.cBitstreamFilename + ".temp";
    m_cWriteToBitFileName                     = m_cEncoderIoParameter.cBitstreamFilename;
    m_cEncoderIoParameter.cBitstreamFilename  = m_cWriteToBitFileTempName;
    RNOKS( WriteBitstreamToFile::create   ( m_pcWriteBitstreamToFile ) );
    RNOKS( m_pcWriteBitstreamToFile->init ( m_cEncoderIoParameter.cBitstreamFilename ) );
  }

  //===== create encoder instance =====
  RNOK( h264::CreaterH264AVCEncoder::create( m_pcH264AVCEncoder ) );


  //===== set start code =====
  m_aucStartCodeBuffer[0] = 0;
  m_aucStartCodeBuffer[1] = 0;
  m_aucStartCodeBuffer[2] = 0;
  m_aucStartCodeBuffer[3] = 1;
  m_cBinDataStartCode.reset ();
  m_cBinDataStartCode.set   ( m_aucStartCodeBuffer, 4 );

  // Extended NAL unit priority is enabled by default, since 6-bit short priority
  // is incompatible with extended 4CIF Palma test set.  Change value to false
  // to enable short ID.

  // Example priority ID assignment: (a) spatial, (b) temporal, (c) quality
  // Other priority assignments can be created by adjusting the mapping table.
  // (J. Ridge, Nokia)
  return Err::m_nOK;
}




ErrVal
H264AVCEncoderTest::destroy()
{
  m_cBinDataStartCode.reset();

  if( m_pcH264AVCEncoder )
  {
    RNOK( m_pcH264AVCEncoder->uninit() );
    RNOK( m_pcH264AVCEncoder->destroy() );
  }

  for( UInt ui = 0; ui < MAX_LAYERS; ui++ )
  {
    if( m_apcWriteYuv[ui] )
    {
      RNOK( m_apcWriteYuv[ui]->uninit () );
      RNOK( m_apcWriteYuv[ui]->destroy() );
    }

    if( m_apcReadYuv[ui] )
    {
      RNOK( m_apcReadYuv[ui]->uninit() );
      RNOK( m_apcReadYuv[ui]->destroy() );
    }
  }


  RNOK( m_pcEncoderCodingParameter->destroy());

  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    AOF( m_acActivePicBufferList[uiLayer].empty() );

    //===== delete picture buffer =====
    PicBufferList::iterator iter;
    for( iter = m_acUnusedPicBufferList[uiLayer].begin(); iter != m_acUnusedPicBufferList[uiLayer].end(); iter++ )
    {
      delete [] (*iter)->getBuffer();
      delete    (*iter);
    }
    for( iter = m_acActivePicBufferList[uiLayer].begin(); iter != m_acActivePicBufferList[uiLayer].end(); iter++ )
    {
      delete [] (*iter)->getBuffer();
      delete    (*iter);
    }
  }

  delete this;
  return Err::m_nOK;
}


ErrVal
H264AVCEncoderTest::xGetNewPicBuffer ( PicBuffer*&  rpcPicBuffer,
                                       UInt         uiLayer,
                                       UInt         uiSize )
{
  if( m_acUnusedPicBufferList[uiLayer].empty() )
  {
	  UChar* pcBuffer = new UChar[ uiSize ];
    ::memset( pcBuffer,  0x00, uiSize*sizeof(UChar) );
    rpcPicBuffer = new PicBuffer( pcBuffer );
  }
  else
  {
    rpcPicBuffer = m_acUnusedPicBufferList[uiLayer].popFront();
  }

  m_acActivePicBufferList[uiLayer].push_back( rpcPicBuffer );

  return Err::m_nOK;
}


ErrVal
H264AVCEncoderTest::xRemovePicBuffer( PicBufferList&  rcPicBufferUnusedList,
                                      UInt            uiLayer )
{
  while( ! rcPicBufferUnusedList.empty() )
  {
    PicBuffer* pcBuffer = rcPicBufferUnusedList.popFront();

    if( NULL != pcBuffer )
    {
      PicBufferList::iterator begin = m_acActivePicBufferList[uiLayer].begin();
      PicBufferList::iterator end   = m_acActivePicBufferList[uiLayer].end  ();
      PicBufferList::iterator iter  = std::find( begin, end, pcBuffer );

      ROT( iter == end ); // there is something wrong if the address is not in the active list

      AOT_DBG( (*iter)->isUsed() );
      m_acUnusedPicBufferList[uiLayer].push_back( *iter );
      m_acActivePicBufferList[uiLayer].erase    (  iter );
    }
  }
  return Err::m_nOK;
}


ErrVal
H264AVCEncoderTest::xWrite( PicBufferList&  rcPicBufferList,
                            UInt            uiLayer )
{
  while( ! rcPicBufferList.empty() )
  {
    PicBuffer* pcBuffer = rcPicBufferList.popFront();

    Pel* pcBuf = pcBuffer->getBuffer();
    RNOK( m_apcWriteYuv[uiLayer]->writeFrame( pcBuf + m_auiLumOffset[uiLayer],
                                              pcBuf + m_auiCbOffset [uiLayer],
                                              pcBuf + m_auiCrOffset [uiLayer],
                                              m_auiHeight           [uiLayer],
                                              m_auiWidth            [uiLayer],
                                              m_auiStride           [uiLayer], m_aauiCropping[uiLayer] ) );
  }
  return Err::m_nOK;
}


ErrVal
H264AVCEncoderTest::xRelease( PicBufferList&  rcPicBufferList,
                              UInt            uiLayer )
{
  RNOK( xRemovePicBuffer( rcPicBufferList, uiLayer ) );
  return Err::m_nOK;
}


ErrVal
H264AVCEncoderTest::xWrite( ExtBinDataAccessorList& rcList,
                            UInt&                   ruiBytesInFrame )
{
  while( rcList.size() )
  {
    ruiBytesInFrame += rcList.front()->size() + 4;
    RNOK( m_pcWriteBitstreamToFile->writePacket( &m_cBinDataStartCode ) );
    RNOK( m_pcWriteBitstreamToFile->writePacket( rcList.front() ) );
    delete[] rcList.front()->data();
    delete   rcList.front();
    rcList.pop_front();
  }
  return Err::m_nOK;
}


ErrVal
H264AVCEncoderTest::xRelease( ExtBinDataAccessorList& rcList )
{
  while( rcList.size() )
  {
    delete[] rcList.front()->data();
    delete   rcList.front();
    rcList.pop_front();
  }
  return Err::m_nOK;
}


ErrVal
H264AVCEncoderTest::go()
{
  UInt                    uiWrittenBytes          = 0;
  const UInt              uiMaxFrame              = m_pcEncoderCodingParameter->getTotalFrames();
  UInt                    uiNumLayers             = ( m_pcEncoderCodingParameter->getAVCmode() ? 1 : m_pcEncoderCodingParameter->getNumberOfLayers() );
  UInt                    uiFrame;
  UInt                    uiLayer;
  UInt                    auiMbX                  [MAX_LAYERS];
  UInt                    auiMbY                  [MAX_LAYERS];
  UInt                    auiPicSize              [MAX_LAYERS];
  PicBuffer*              apcOriginalPicBuffer    [MAX_LAYERS];
  PicBuffer*              apcReconstructPicBuffer [MAX_LAYERS];
  PicBufferList           acPicBufferOutputList   [MAX_LAYERS];
  PicBufferList           acPicBufferUnusedList   [MAX_LAYERS];
  ExtBinDataAccessorList  cOutExtBinDataAccessorList;
  Bool                    bMoreSets;
#if DOLBY_ENCMUX_ENABLE
  PicBuffer*              apcMuxPicBuffer[MAX_LAYERS];
#endif
  
  //===== initialization =====
  RNOK( m_pcH264AVCEncoder->init( m_pcEncoderCodingParameter ) );

  // JVT-W043 {
  bRateControlEnable = (Bool)((m_pcH264AVCEncoder->getCodingParameter()->m_uiRateControlEnable) > 0 ? true : false );
  if ( bRateControlEnable )
  {
    Int   iGOPSize     = m_pcEncoderCodingParameter->getGOPSize();
    Int   iFullGOPs    = uiMaxFrame / iGOPSize;
    Double dMaximumFrameRate = m_pcEncoderCodingParameter->getMaximumFrameRate();
    iGOPSize =  (Int)floor( 0.5F + iGOPSize / (dMaximumFrameRate / (Float) (m_pcH264AVCEncoder->getCodingParameter()->getLayerParameters(0).m_dOutputFrameRate)) );
    iGOPSize = gMax( iGOPSize, 1 );
    Int uiLocalMaxFrame =  (Int)floor( 0.5F + uiMaxFrame / (dMaximumFrameRate / (Float) (m_pcH264AVCEncoder->getCodingParameter()->getLayerParameters(0).m_dOutputFrameRate)) );
    if ( uiLocalMaxFrame % iGOPSize == 0 )
      iFullGOPs--;
    Int   iNb          = iFullGOPs * (iGOPSize - 1);

    pcJSVMParams  = new jsvm_parameters;
    pcGenericRC   = new rc_generic( pcJSVMParams );
    pcQuadraticRC = new rc_quadratic( pcGenericRC, pcJSVMParams );

    // parameter initialization
    pcQuadraticRC->m_iDDquant       = (Int) m_pcH264AVCEncoder->getCodingParameter()->m_uiMaxQpChange;
    pcQuadraticRC->m_iPMaxQpChange  = (Int) m_pcH264AVCEncoder->getCodingParameter()->m_uiMaxQpChange;
    pcJSVMParams->bit_rate          = (Float) (m_pcH264AVCEncoder->getCodingParameter()->m_uiBitRate);
    pcJSVMParams->BasicUnit         = m_pcH264AVCEncoder->getCodingParameter()->m_uiBasicUnit;
    pcJSVMParams->basicunit         = m_pcH264AVCEncoder->getCodingParameter()->m_uiBasicUnit;
    pcJSVMParams->RCMaxQP           = m_pcH264AVCEncoder->getCodingParameter()->m_uiRCMaxQP;
    pcJSVMParams->RCMinQP           = m_pcH264AVCEncoder->getCodingParameter()->m_uiRCMinQP;
    pcJSVMParams->SetInitialQP      = m_pcH264AVCEncoder->getCodingParameter()->m_uiInitialQp;
    pcJSVMParams->m_uiIntraPeriod   = m_pcEncoderCodingParameter->getIntraPeriod();
    pcJSVMParams->FrameRate         = (Float) (m_pcH264AVCEncoder->getCodingParameter()->getLayerParameters(0).m_dOutputFrameRate);
    pcJSVMParams->height            = m_pcH264AVCEncoder->getCodingParameter()->getLayerParameters(0).m_uiFrameHeightInSamples;
    pcJSVMParams->width             = m_pcH264AVCEncoder->getCodingParameter()->getLayerParameters(0).m_uiFrameWidthInSamples;
    pcJSVMParams->no_frames         = uiMaxFrame;
    pcJSVMParams->successive_Bframe = iGOPSize - 1;
    pcJSVMParams->m_uiLayerId       = 0;

    if ( m_pcH264AVCEncoder->getCodingParameter()->m_uiAdaptInitialQP )
      pcGenericRC->adaptInitialQP();

    switch( iGOPSize )
    {
    case 1:
      pcJSVMParams->HierarchicalLevels = 0;
      break;
    case 2:
      pcJSVMParams->HierarchicalLevels = 1;
      break;
    case 4:
      pcJSVMParams->HierarchicalLevels = 2;
      break;
    case 8:
      pcJSVMParams->HierarchicalLevels = 3;
      break;
    case 16:
      pcJSVMParams->HierarchicalLevels = 4;
      break;
    case 32:
    default:
      fprintf(stderr, "\n RC not stable for large number of hierarchical levels...exiting...\n" );
      assert(0); // some compilers do not compile assert in release/Ox mode
      exit(1);
      break;
    }

    // rate control initialization functions
    pcQuadraticRC->init();
    pcQuadraticRC->rc_init_seq();
    pcQuadraticRC->rc_init_GOP(uiMaxFrame - iNb - 1, iNb);
    pcGenericRC->generic_alloc();
  }
  // JVT-W043 }

  //===== write parameter sets =====
  for( bMoreSets = true; bMoreSets;  )
  {
    UChar   aucParameterSetBuffer[1000];
    BinData cBinData;
    cBinData.reset();
    cBinData.set( aucParameterSetBuffer, 1000 );

    ExtBinDataAccessor cExtBinDataAccessor;
    cBinData.setMemAccessor( cExtBinDataAccessor );

    h264::SequenceParameterSet* pcAVCSPS = NULL;
    RNOK( m_pcH264AVCEncoder->writeParameterSets( &cExtBinDataAccessor, pcAVCSPS, bMoreSets ) );
		if( m_pcH264AVCEncoder->getScalableSeiMessage() )
		{
      RNOK( m_pcWriteBitstreamToFile->writePacket       ( &m_cBinDataStartCode ) );
      RNOK( m_pcWriteBitstreamToFile->writePacket       ( &cExtBinDataAccessor ) );

      uiWrittenBytes += 4 + cExtBinDataAccessor.size();
		}
    cBinData.reset();
    // JVT-V068 {
/* luodan */
    if (pcAVCSPS)
    {
      cBinData.set( aucParameterSetBuffer, 1000 );

      ExtBinDataAccessor cExtBinDataAccessorl;
      cBinData.setMemAccessor( cExtBinDataAccessorl );

      RNOK( m_pcH264AVCEncoder->writeAVCCompatibleHRDSEI( &cExtBinDataAccessorl, pcAVCSPS ) );

      RNOK( m_pcWriteBitstreamToFile->writePacket       ( &m_cBinDataStartCode ) );
      RNOK( m_pcWriteBitstreamToFile->writePacket       ( &cExtBinDataAccessorl ) );

      uiWrittenBytes += 4 + cExtBinDataAccessorl.size();
      cBinData.reset();
    }
/* luodan */
    // JVT-V068 }
  }

  //===== determine parameters for required frame buffers =====
  UInt  uiAllocMbX = 0;
  UInt  uiAllocMbY = 0;
  for( uiLayer = 0; uiLayer < uiNumLayers; uiLayer++ )
  {
    h264::LayerParameters& rcLayer  = m_pcEncoderCodingParameter->getLayerParameters( uiLayer );
    auiMbX        [uiLayer]         = rcLayer.getFrameWidthInMbs ();
    auiMbY        [uiLayer]         = rcLayer.getFrameHeightInMbs();
    uiAllocMbX                      = gMax( uiAllocMbX, auiMbX[uiLayer] );
    uiAllocMbY                      = gMax( uiAllocMbY, auiMbY[uiLayer] );
    m_aauiCropping[uiLayer][0]      = 0;
    m_aauiCropping[uiLayer][1]      = rcLayer.getHorPadding      ();
    m_aauiCropping[uiLayer][2]      = 0;
    m_aauiCropping[uiLayer][3]      = rcLayer.getVerPadding      ();
    m_auiHeight   [uiLayer]         = auiMbY[uiLayer]<<4;
    m_auiWidth    [uiLayer]         = auiMbX[uiLayer]<<4;
    UInt  uiSize                    = ((uiAllocMbY<<4)+2*YUV_Y_MARGIN)*((uiAllocMbX<<4)+2*YUV_X_MARGIN);
    auiPicSize    [uiLayer]         = ((uiAllocMbX<<4)+2*YUV_X_MARGIN)*((uiAllocMbY<<4)+2*YUV_Y_MARGIN)*3/2;
    m_auiLumOffset[uiLayer]         = ((uiAllocMbX<<4)+2*YUV_X_MARGIN)* YUV_Y_MARGIN   + YUV_X_MARGIN;
    m_auiCbOffset [uiLayer]         = ((uiAllocMbX<<3)+  YUV_X_MARGIN)* YUV_Y_MARGIN/2 + YUV_X_MARGIN/2 + uiSize;
    m_auiCrOffset [uiLayer]         = ((uiAllocMbX<<3)+  YUV_X_MARGIN)* YUV_Y_MARGIN/2 + YUV_X_MARGIN/2 + 5*uiSize/4;
    m_auiStride   [uiLayer]         =  (uiAllocMbX<<4)+ 2*YUV_X_MARGIN;
  }

#if DOLBY_ENCMUX_ENABLE
  if(m_pcEncoderCodingParameter->getMuxMethod() && uiNumLayers >1)
  {
    for(uiLayer=0; uiLayer<2; uiLayer++)
    {
      apcMuxPicBuffer[uiLayer] = NULL;
	    UChar* pcBuffer = new UChar[ auiPicSize[uiLayer] ];
      ::memset( pcBuffer,  0x00, auiPicSize[uiLayer] *sizeof(UChar) );
      apcMuxPicBuffer[uiLayer] = new PicBuffer( pcBuffer );
    }
  }
  else
  {
    for(uiLayer=0; uiLayer<2; uiLayer++)
    {
      apcMuxPicBuffer[uiLayer] = NULL;
    }
  }
#endif

  // start time measurement
  clock_t start = clock();

  //===== loop over frames =====
  for( uiFrame = 0; uiFrame < uiMaxFrame; uiFrame++ )
  {
    //===== get picture buffers and read original pictures =====
    for( uiLayer = 0; uiLayer < uiNumLayers; uiLayer++ )
    {
      UInt  uiSkip = ( 1 << m_pcEncoderCodingParameter->getLayerParameters( uiLayer ).getTemporalResolution() );

      if( uiFrame % uiSkip == 0 )
      {
        RNOK( xGetNewPicBuffer( apcReconstructPicBuffer [uiLayer], uiLayer, auiPicSize[uiLayer] ) );
        RNOK( xGetNewPicBuffer( apcOriginalPicBuffer    [uiLayer], uiLayer, auiPicSize[uiLayer] ) );

#if DOLBY_ENCMUX_ENABLE
        if( (m_pcEncoderCodingParameter->getMuxMethod() && uiNumLayers >1) )
        {
          RNOK( m_apcReadYuv[uiLayer]->readFrame( *apcOriginalPicBuffer[uiLayer] + m_auiLumOffset[0],
                                                  *apcOriginalPicBuffer[uiLayer] + m_auiCbOffset [0],
                                                  *apcOriginalPicBuffer[uiLayer] + m_auiCrOffset [0],
                                                  m_auiHeight [0],
                                                  m_auiWidth  [0],
                                                  m_auiStride [0] ) );
        }
        else
        {
          RNOK( m_apcReadYuv[uiLayer]->readFrame( *apcOriginalPicBuffer[uiLayer] + m_auiLumOffset[uiLayer],
                                                  *apcOriginalPicBuffer[uiLayer] + m_auiCbOffset [uiLayer],
                                                  *apcOriginalPicBuffer[uiLayer] + m_auiCrOffset [uiLayer],
                                                  m_auiHeight [uiLayer],
                                                  m_auiWidth  [uiLayer],
                                                  m_auiStride [uiLayer] ) );
        }
#else
        RNOK( m_apcReadYuv[uiLayer]->readFrame( *apcOriginalPicBuffer[uiLayer] + m_auiLumOffset[uiLayer],
                                                *apcOriginalPicBuffer[uiLayer] + m_auiCbOffset [uiLayer],
                                                *apcOriginalPicBuffer[uiLayer] + m_auiCrOffset [uiLayer],
                                                m_auiHeight [uiLayer],
                                                m_auiWidth  [uiLayer],
                                                m_auiStride [uiLayer] ) );
#endif
      }
      else
      {
        apcReconstructPicBuffer [uiLayer] = 0;
        apcOriginalPicBuffer    [uiLayer] = 0;
      }
    }

#if DOLBY_ENCMUX_ENABLE
    if( (m_pcEncoderCodingParameter->getMuxMethod() && uiNumLayers >1) && (apcOriginalPicBuffer[0] && apcOriginalPicBuffer[1]))
    {
      //mux process;
      if(m_pcEncoderCodingParameter->getMuxMethod() == 1)
      {
        h264::LayerParameters&  rcLayer0 = m_pcEncoderCodingParameter->getLayerParameters( 0 );
        UInt uiWidth = rcLayer0.getFrameWidthInSamples();
        UInt uiHeight = rcLayer0.getFrameHeightInSamples();

        sbsMux(*apcMuxPicBuffer[0] + m_auiLumOffset[0], m_auiStride[0], *apcOriginalPicBuffer[0] + m_auiLumOffset[0],  *apcOriginalPicBuffer[1] + m_auiLumOffset[0], m_auiStride[0],
          uiWidth, uiHeight, m_pcEncoderCodingParameter->getMuxOffset(0), m_pcEncoderCodingParameter->getMuxOffset(1), m_pcEncoderCodingParameter->getMuxFilter());
        sbsMux(*apcMuxPicBuffer[0] + m_auiCbOffset[0], (m_auiStride[0]>>1), *apcOriginalPicBuffer[0] + m_auiCbOffset[0],  *apcOriginalPicBuffer[1] + m_auiCbOffset[0], (m_auiStride[0]>>1),
          (uiWidth>>1), (uiHeight>>1), m_pcEncoderCodingParameter->getMuxOffset(0), m_pcEncoderCodingParameter->getMuxOffset(1), m_pcEncoderCodingParameter->getMuxFilter());
        sbsMux(*apcMuxPicBuffer[0] + m_auiCrOffset[0], (m_auiStride[0]>>1), *apcOriginalPicBuffer[0] + m_auiCrOffset[0],  *apcOriginalPicBuffer[1] + m_auiCrOffset[0], (m_auiStride[0]>>1),
          (uiWidth>>1), (uiHeight>>1), m_pcEncoderCodingParameter->getMuxOffset(0), m_pcEncoderCodingParameter->getMuxOffset(1), m_pcEncoderCodingParameter->getMuxFilter());

        //padding;
        RNOK(padBuf(*apcMuxPicBuffer[0] + m_auiLumOffset[0], m_auiStride[0], uiWidth, uiHeight, m_auiWidth[0], m_auiHeight[0], m_apcReadYuv[0]->getFillMode()));
        RNOK(padBuf(*apcMuxPicBuffer[0] + m_auiCbOffset[0], (m_auiStride[0]>>1), (uiWidth>>1), (uiHeight>>1), (m_auiWidth[0]>>1), (m_auiHeight[0]>>1), m_apcReadYuv[0]->getFillMode()));
        RNOK(padBuf(*apcMuxPicBuffer[0] + m_auiCrOffset[0], (m_auiStride[0]>>1), (uiWidth>>1), (uiHeight>>1), (m_auiWidth[0]>>1), (m_auiHeight[0]>>1), m_apcReadYuv[0]->getFillMode()));

        sbsMuxFR(*apcMuxPicBuffer[1] + m_auiLumOffset[1], m_auiStride[1], *apcOriginalPicBuffer[0] + m_auiLumOffset[0],  *apcOriginalPicBuffer[1] + m_auiLumOffset[0], m_auiStride[0],
          uiWidth, uiHeight);
        sbsMuxFR(*apcMuxPicBuffer[1] + m_auiCbOffset[1], (m_auiStride[1]>>1), *apcOriginalPicBuffer[0] + m_auiCbOffset[0],  *apcOriginalPicBuffer[1] + m_auiCbOffset[0], (m_auiStride[0]>>1),
          (uiWidth>>1), (uiHeight>>1));
        sbsMuxFR(*apcMuxPicBuffer[1] + m_auiCrOffset[1], (m_auiStride[1]>>1), *apcOriginalPicBuffer[0] + m_auiCrOffset[0],  *apcOriginalPicBuffer[1] + m_auiCrOffset[0], (m_auiStride[0]>>1),
          (uiWidth>>1), (uiHeight>>1));

        //padding;
        uiWidth = m_pcEncoderCodingParameter->getLayerParameters( 1 ).getFrameWidthInSamples();
        uiHeight = m_pcEncoderCodingParameter->getLayerParameters( 1 ).getFrameHeightInSamples();
        RNOK(padBuf(*apcMuxPicBuffer[1] + m_auiLumOffset[1], m_auiStride[1], uiWidth, uiHeight, m_auiWidth[1], m_auiHeight[1], m_apcReadYuv[1]->getFillMode()));
        RNOK(padBuf(*apcMuxPicBuffer[1] + m_auiCbOffset[1], (m_auiStride[1]>>1), (uiWidth>>1), (uiHeight>>1), (m_auiWidth[1]>>1), (m_auiHeight[1]>>1), m_apcReadYuv[1]->getFillMode()));
        RNOK(padBuf(*apcMuxPicBuffer[1] + m_auiCrOffset[1], (m_auiStride[1]>>1), (uiWidth>>1), (uiHeight>>1), (m_auiWidth[1]>>1), (m_auiHeight[1]>>1), m_apcReadYuv[1]->getFillMode()));
      }
      else
      {
        h264::LayerParameters&  rcLayer0 = m_pcEncoderCodingParameter->getLayerParameters( 0 );
        UInt uiWidth = rcLayer0.getFrameWidthInSamples();
        UInt uiHeight = rcLayer0.getFrameHeightInSamples();

        tabMux(*apcMuxPicBuffer[0] + m_auiLumOffset[0], m_auiStride[0], *apcOriginalPicBuffer[0] + m_auiLumOffset[0],  *apcOriginalPicBuffer[1] + m_auiLumOffset[0], m_auiStride[0],
          uiWidth, uiHeight, m_pcEncoderCodingParameter->getMuxOffset(0), m_pcEncoderCodingParameter->getMuxOffset(1), m_pcEncoderCodingParameter->getMuxFilter());
        tabMux(*apcMuxPicBuffer[0] + m_auiCbOffset[0], (m_auiStride[0]>>1), *apcOriginalPicBuffer[0] + m_auiCbOffset[0],  *apcOriginalPicBuffer[1] + m_auiCbOffset[0], (m_auiStride[0]>>1),
          (uiWidth>>1), (uiHeight>>1), m_pcEncoderCodingParameter->getMuxOffset(0), m_pcEncoderCodingParameter->getMuxOffset(1), m_pcEncoderCodingParameter->getMuxFilter());
        tabMux(*apcMuxPicBuffer[0] + m_auiCrOffset[0], (m_auiStride[0]>>1), *apcOriginalPicBuffer[0] + m_auiCrOffset[0],  *apcOriginalPicBuffer[1] + m_auiCrOffset[0], (m_auiStride[0]>>1),
          (uiWidth>>1), (uiHeight>>1), m_pcEncoderCodingParameter->getMuxOffset(0), m_pcEncoderCodingParameter->getMuxOffset(1), m_pcEncoderCodingParameter->getMuxFilter());

        //padding;
        RNOK(padBuf(*apcMuxPicBuffer[0] + m_auiLumOffset[0], m_auiStride[0], uiWidth, uiHeight, m_auiWidth[0], m_auiHeight[0], m_apcReadYuv[0]->getFillMode()));
        RNOK(padBuf(*apcMuxPicBuffer[0] + m_auiCbOffset[0], (m_auiStride[0]>>1), (uiWidth>>1), (uiHeight>>1), (m_auiWidth[0]>>1), (m_auiHeight[0]>>1), m_apcReadYuv[0]->getFillMode()));
        RNOK(padBuf(*apcMuxPicBuffer[0] + m_auiCrOffset[0], (m_auiStride[0]>>1), (uiWidth>>1), (uiHeight>>1), (m_auiWidth[0]>>1), (m_auiHeight[0]>>1), m_apcReadYuv[0]->getFillMode()));

        tabMuxFR(*apcMuxPicBuffer[1] + m_auiLumOffset[1], m_auiStride[1], *apcOriginalPicBuffer[0] + m_auiLumOffset[0],  *apcOriginalPicBuffer[1] + m_auiLumOffset[0], m_auiStride[0],
          uiWidth, uiHeight);
        tabMuxFR(*apcMuxPicBuffer[1] + m_auiCbOffset[1], (m_auiStride[1]>>1), *apcOriginalPicBuffer[0] + m_auiCbOffset[0],  *apcOriginalPicBuffer[1] + m_auiCbOffset[0], (m_auiStride[0]>>1),
          (uiWidth>>1), (uiHeight>>1));
        tabMuxFR(*apcMuxPicBuffer[1] + m_auiCrOffset[1], (m_auiStride[1]>>1), *apcOriginalPicBuffer[0] + m_auiCrOffset[0],  *apcOriginalPicBuffer[1] + m_auiCrOffset[0], (m_auiStride[0]>>1),
          (uiWidth>>1), (uiHeight>>1));

        //padding;
        uiWidth = m_pcEncoderCodingParameter->getLayerParameters( 1 ).getFrameWidthInSamples();
        uiHeight = m_pcEncoderCodingParameter->getLayerParameters( 1 ).getFrameHeightInSamples();
        RNOK(padBuf(*apcMuxPicBuffer[1] + m_auiLumOffset[1], m_auiStride[1], uiWidth, uiHeight, m_auiWidth[1], m_auiHeight[1], m_apcReadYuv[1]->getFillMode()));
        RNOK(padBuf(*apcMuxPicBuffer[1] + m_auiCbOffset[1], (m_auiStride[1]>>1), (uiWidth>>1), (uiHeight>>1), (m_auiWidth[1]>>1), (m_auiHeight[1]>>1), m_apcReadYuv[1]->getFillMode()));
        RNOK(padBuf(*apcMuxPicBuffer[1] + m_auiCrOffset[1], (m_auiStride[1]>>1), (uiWidth>>1), (uiHeight>>1), (m_auiWidth[1]>>1), (m_auiHeight[1]>>1), m_apcReadYuv[1]->getFillMode()));
      }

      //swap the buffer;
      for( uiLayer = 0; uiLayer < 2; uiLayer++ )
      {
        PicBuffer *pTmp = m_acActivePicBufferList[uiLayer].popBack();
        apcOriginalPicBuffer[uiLayer] = apcMuxPicBuffer[uiLayer];
        m_acActivePicBufferList[uiLayer].pushBack(apcOriginalPicBuffer[uiLayer]);
        apcMuxPicBuffer[uiLayer] = pTmp;
      }
    }
#endif

    //===== call encoder =====
    RNOK( m_pcH264AVCEncoder->process( cOutExtBinDataAccessorList,
                                       apcOriginalPicBuffer,
                                       apcReconstructPicBuffer,
                                       acPicBufferOutputList,
                                       acPicBufferUnusedList ) );

    //===== write and release NAL unit buffers =====
    UInt  uiBytesUsed = 0;
    RNOK( xWrite  ( cOutExtBinDataAccessorList, uiBytesUsed ) );
    uiWrittenBytes   += uiBytesUsed;

    //===== write and release reconstructed pictures =====
    for( uiLayer = 0; uiLayer < uiNumLayers; uiLayer++ )
    {
      RNOK( xWrite  ( acPicBufferOutputList[uiLayer], uiLayer ) );
      RNOK( xRelease( acPicBufferUnusedList[uiLayer], uiLayer ) );
    }
  }

  // stop time measurement
  clock_t end = clock();

  //===== finish encoding =====
  UInt  uiNumCodedFrames = 0;
  Double  dHighestLayerOutputRate = 0.0;
  RNOK( m_pcH264AVCEncoder->finish( cOutExtBinDataAccessorList,
                                    acPicBufferOutputList,
                                    acPicBufferUnusedList,
                                    uiNumCodedFrames,
                                    dHighestLayerOutputRate ) );

  //===== write and release NAL unit buffers =====
  RNOK( xWrite  ( cOutExtBinDataAccessorList, uiWrittenBytes ) );

  //===== write and release reconstructed pictures =====
  for( uiLayer = 0; uiLayer < uiNumLayers; uiLayer++ )
  {
    RNOK( xWrite  ( acPicBufferOutputList[uiLayer], uiLayer ) );
    RNOK( xRelease( acPicBufferUnusedList[uiLayer], uiLayer ) );
  }


  //===== set parameters and output summary =====
  m_cEncoderIoParameter.nFrames = uiFrame;
  m_cEncoderIoParameter.nResult = 0;

  if( ! m_pcEncoderCodingParameter->getAVCmode() )
	{
    UChar   aucParameterSetBuffer[10000];
    BinData cBinData;
    cBinData.reset();
    cBinData.set( aucParameterSetBuffer, 10000 );

		ExtBinDataAccessor cExtBinDataAccessor;
		cBinData.setMemAccessor( cExtBinDataAccessor );
		m_pcH264AVCEncoder->SetVeryFirstCall();

  	h264::SequenceParameterSet* pcAVCSPS = NULL;
  	RNOK( m_pcH264AVCEncoder->writeParameterSets( &cExtBinDataAccessor, pcAVCSPS, bMoreSets ) );
    RNOK( m_pcWriteBitstreamToFile->writePacket       ( &m_cBinDataStartCode ) );
		RNOK( m_pcWriteBitstreamToFile->writePacket       ( &cExtBinDataAccessor ) );
		uiWrittenBytes += 4 + cExtBinDataAccessor.size();
		cBinData.reset();
	}

  if( m_pcWriteBitstreamToFile )
  {
    RNOK( m_pcWriteBitstreamToFile->uninit() );
    RNOK( m_pcWriteBitstreamToFile->destroy() );
  }

  if( ! m_pcEncoderCodingParameter->getAVCmode() )
  {
    RNOK	( ScalableDealing() );
  }

  // JVT-W043 {
  if ( bRateControlEnable )
  {
    delete pcJSVMParams;
    delete pcGenericRC;
    delete pcQuadraticRC;
  }
  // JVT-W043 }

#if DOLBY_ENCMUX_ENABLE
  for(uiLayer=0; uiLayer<2; uiLayer++)
  {
    if(apcMuxPicBuffer[uiLayer])
    {
      delete [] (*apcMuxPicBuffer[uiLayer]);
      delete apcMuxPicBuffer[uiLayer];
    }
  }
#endif

  // output encoding time
  fprintf(stdout, "Encoding speed: %.3lf ms/frame, Time:%.3lf ms, Frames: %d\n", (double)(end-start)*1000/CLOCKS_PER_SEC/uiMaxFrame, (double)(end-start)*1000/CLOCKS_PER_SEC, uiMaxFrame);

  return Err::m_nOK;
}

ErrVal
H264AVCEncoderTest::ScalableDealing()
{
  FILE *ftemp = fopen( m_cWriteToBitFileTempName.c_str(), "rb" );
  FILE *f     = fopen( m_cWriteToBitFileName.c_str    (), "wb" );

	UChar pvBuffer[4];

	fseek( ftemp, SEEK_SET, SEEK_END );
	long lFileLength = ftell( ftemp );

	long lpos = 0;
	long loffset = -5;	//start offset from end of file
	Bool bMoreSets = true;
	do
  {
		fseek( ftemp, loffset, SEEK_END);
		ROF( 4 == fread( pvBuffer, 1, 4, ftemp ) );
		if( pvBuffer[0] == 0 && pvBuffer[1] == 0 && pvBuffer[2] == 0 && pvBuffer[3] == 1)
		{
			bMoreSets = false;
			lpos = abs( loffset );
		}
		else
		{
			loffset --;
		}
	} while( bMoreSets );

	fseek( ftemp, loffset, SEEK_END );

	UChar *pvChar = new UChar[lFileLength];
	ROF( lpos == fread( pvChar, 1, lpos, ftemp ) );
	fseek( ftemp, 0, SEEK_SET );
	ROF( lFileLength-lpos == fread( pvChar+lpos, 1, lFileLength-lpos, ftemp) );
  fflush(ftemp);
	fclose(ftemp);
	ROF( lFileLength == fwrite( pvChar, 1, lFileLength, f ) );
	delete [] pvChar;
  fflush(f);
	fclose(f);
	RNOK( remove( m_cWriteToBitFileTempName.c_str() ) );

	return Err::m_nOK;
}
