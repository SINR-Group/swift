
#include "H264AVCEncoderLib.h"
#include "ControlMngH264AVCEncoder.h"


H264AVC_NAMESPACE_BEGIN


ControlMngH264AVCEncoder::ControlMngH264AVCEncoder():
  m_pcSliceEncoder        ( NULL ),
  m_pcControlMng          ( NULL ),
  m_pcBitWriteBuffer      ( NULL ),
  m_pcNalUnitEncoder      ( NULL ),
  m_pcUvlcWriter          ( NULL ),
  m_pcUvlcTester          ( NULL ),
  m_pcMbCoder             ( NULL ),
  m_pcLoopFilter          ( NULL ),
  m_pcMbEncoder           ( NULL ),
  m_pcQuarterPelFilter    ( NULL ),
  m_pcCodingParameter     ( NULL ),
  m_pcParameterSetMng     ( NULL ),
  m_pcSampleWeighting     ( NULL ),
  m_pcCabacWriter         ( NULL ),
  m_pcXDistortion         ( NULL ),
  m_pcMotionEstimation    ( NULL ),
  m_pcRateDistortion      ( NULL ),
  m_pcMbDataCtrl          ( NULL ),
  m_pcMbSymbolWriteIf     ( NULL )
{
  ::memset( m_apcYuvFullPelBufferCtrl, 0x00, MAX_LAYERS*sizeof(Void*) );
  ::memset( m_apcYuvHalfPelBufferCtrl, 0x00, MAX_LAYERS*sizeof(Void*) );
  ::memset( m_apcPocCalculator,        0x00, MAX_LAYERS*sizeof(Void*) );
  ::memset( m_apcLayerEncoder,         0x00, MAX_LAYERS*sizeof(Void*) );
  ::memset( m_auiMbXinFrame,           0x00, MAX_LAYERS*sizeof(UInt ) );
  ::memset( m_auiMbYinFrame,           0x00, MAX_LAYERS*sizeof(UInt ) );
}

ControlMngH264AVCEncoder::~ControlMngH264AVCEncoder()
{
}


ErrVal
ControlMngH264AVCEncoder::initParameterSets( const SequenceParameterSet&  rcSPS,
                                             const PictureParameterSet&   rcPPS )
{
  UInt  uiLayer             = rcSPS.getDependencyId     ();
  UInt  uiMbX               = rcSPS.getFrameWidthInMbs  ();
  UInt  uiMbY               = rcSPS.getFrameHeightInMbs ();
  m_auiMbXinFrame[uiLayer]  = uiMbX;
  m_auiMbYinFrame[uiLayer]  = uiMbY;

  //===== initialize buffer controls and LayerEncoder =====
  UInt uiAllocX = rcSPS.getAllocFrameMbsX() << 4;
  UInt uiAllocY = rcSPS.getAllocFrameMbsY() << 4;
  RNOK( m_apcYuvFullPelBufferCtrl[uiLayer]->initSPS( uiAllocY, uiAllocX, YUV_Y_MARGIN, YUV_X_MARGIN    ) );
  RNOK( m_apcYuvHalfPelBufferCtrl[uiLayer]->initSPS( uiAllocY, uiAllocX, YUV_Y_MARGIN, YUV_X_MARGIN, 1 ) );
  if( ! m_bAVCMode )
  {
    RNOK( m_apcLayerEncoder      [uiLayer]->initParameterSets( rcSPS, rcPPS ) );
  }

  return Err::m_nOK;
}


ErrVal ControlMngH264AVCEncoder::create( ControlMngH264AVCEncoder*& rpcControlMngH264AVCEncoder )
{
  rpcControlMngH264AVCEncoder = new ControlMngH264AVCEncoder;

  ROT( NULL == rpcControlMngH264AVCEncoder );

  return Err::m_nOK;
}

ErrVal ControlMngH264AVCEncoder::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal ControlMngH264AVCEncoder::uninit()
{
  m_pcSliceEncoder = NULL;
  m_pcControlMng = NULL;
  m_pcBitWriteBuffer = NULL;
  m_pcBitCounter = NULL;
  m_pcNalUnitEncoder = NULL;
  m_pcUvlcWriter = NULL;
  m_pcUvlcTester = NULL;
  m_pcMbCoder = NULL;
  m_pcLoopFilter = NULL;
  m_pcMbEncoder = NULL;
  m_pcTransform = NULL;
  m_pcIntraPrediction = NULL;
  m_pcQuarterPelFilter = NULL;
  m_pcCodingParameter = NULL;
  m_pcParameterSetMng = NULL;
  m_pcSampleWeighting = NULL;
  m_pcCabacWriter = NULL;
  m_pcXDistortion = NULL;
  m_pcMotionEstimation = NULL;
  m_pcRateDistortion = NULL;


  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    m_apcYuvFullPelBufferCtrl [uiLayer] = 0;
    m_apcYuvHalfPelBufferCtrl [uiLayer] = 0;
    m_apcLayerEncoder         [uiLayer] = 0;
    m_apcPocCalculator        [uiLayer] = 0;
  }


  return Err::m_nOK;
}

ErrVal ControlMngH264AVCEncoder::init( LayerEncoder*              apcLayerEncoder          [MAX_LAYERS],
                                       SliceEncoder*              pcSliceEncoder,
                                       ControlMngH264AVCEncoder*  pcControlMng,
                                       BitWriteBuffer*            pcBitWriteBuffer,
                                       BitCounter*                pcBitCounter,
                                       NalUnitEncoder*            pcNalUnitEncoder,
                                       UvlcWriter*                pcUvlcWriter,
                                       UvlcWriter*                pcUvlcTester,
                                       MbCoder*                   pcMbCoder,
                                       LoopFilter*                pcLoopFilter,
                                       MbEncoder*                 pcMbEncoder,
                                       Transform*                 pcTransform,
                                       IntraPredictionSearch*     pcIntraPrediction,
                                       YuvBufferCtrl*             apcYuvFullPelBufferCtrl [MAX_LAYERS],
                                       YuvBufferCtrl*             apcYuvHalfPelBufferCtrl [MAX_LAYERS],
                                       QuarterPelFilter*          pcQuarterPelFilter,
                                       CodingParameter*           pcCodingParameter,
                                       ParameterSetMng*           pcParameterSetMng,
                                       PocCalculator*             apcPocCalculator        [MAX_LAYERS],
                                       SampleWeighting*           pcSampleWeighting,
                                       CabacWriter*               pcCabacWriter,
                                       XDistortion*               pcXDistortion,
                                       MotionEstimation*          pcMotionEstimation,
                                       RateDistortion*            pcRateDistortion )
{
  ROT( NULL == pcSliceEncoder);
  ROT( NULL == pcControlMng);
  ROT( NULL == pcBitWriteBuffer);
  ROT( NULL == pcBitCounter);
  ROT( NULL == pcNalUnitEncoder);
  ROT( NULL == pcUvlcWriter);
  ROT( NULL == pcUvlcTester);
  ROT( NULL == pcMbCoder);
  ROT( NULL == pcLoopFilter);
  ROT( NULL == pcMbEncoder);
  ROT( NULL == pcTransform);
  ROT( NULL == pcIntraPrediction);
  ROT( NULL == pcQuarterPelFilter);
  ROT( NULL == pcCodingParameter);
  ROT( NULL == pcParameterSetMng);
  ROT( NULL == pcSampleWeighting);
  ROT( NULL == pcCabacWriter);
  ROT( NULL == pcXDistortion);
  ROT( NULL == pcMotionEstimation);
  ROT( NULL == pcRateDistortion);


  m_pcSliceEncoder      = pcSliceEncoder;
  m_pcControlMng        = pcControlMng;
  m_pcBitWriteBuffer    = pcBitWriteBuffer;
  m_pcBitCounter        = pcBitCounter;
  m_pcNalUnitEncoder    = pcNalUnitEncoder;
  m_pcUvlcWriter        = pcUvlcWriter;
  m_pcUvlcTester        = pcUvlcTester;
  m_pcMbCoder           = pcMbCoder;
  m_pcLoopFilter        = pcLoopFilter;
  m_pcMbEncoder         = pcMbEncoder;
  m_pcTransform         = pcTransform;
  m_pcIntraPrediction   = pcIntraPrediction;
  m_pcQuarterPelFilter  = pcQuarterPelFilter;
  m_pcCodingParameter   = pcCodingParameter;
  m_pcParameterSetMng   = pcParameterSetMng;
  m_pcSampleWeighting   = pcSampleWeighting;
  m_pcCabacWriter       = pcCabacWriter;
  m_pcXDistortion       = pcXDistortion;
  m_pcMotionEstimation  = pcMotionEstimation;
  m_pcRateDistortion    = pcRateDistortion;


  ROT( NULL == apcLayerEncoder );
  ROT( NULL == apcPocCalculator );
  ROT( NULL == apcYuvFullPelBufferCtrl );
  ROT( NULL == apcYuvHalfPelBufferCtrl );

  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    ROT( NULL == apcLayerEncoder         [uiLayer] );
    ROT( NULL == apcPocCalculator       [uiLayer] );
    ROT( NULL == apcYuvFullPelBufferCtrl[uiLayer] );
    ROT( NULL == apcYuvHalfPelBufferCtrl[uiLayer] );

    m_apcLayerEncoder         [uiLayer] = apcLayerEncoder         [uiLayer];
    m_apcPocCalculator        [uiLayer] = apcPocCalculator        [uiLayer];
    m_apcYuvFullPelBufferCtrl [uiLayer] = apcYuvFullPelBufferCtrl [uiLayer];
    m_apcYuvHalfPelBufferCtrl [uiLayer] = apcYuvHalfPelBufferCtrl [uiLayer];
  }

  m_bAVCMode = ( pcCodingParameter->getAVCmode() != 0 );

  return Err::m_nOK;
}




ErrVal ControlMngH264AVCEncoder::finishSlice( const SliceHeader& rcSH, Bool& rbPicDone, Bool& rbFrameDone )
{
  ROT( NULL == m_pcMbSymbolWriteIf ) ;

  RNOK( m_pcMbSymbolWriteIf->finishSlice() );

  m_pcMbSymbolWriteIf = NULL;

  rbPicDone   = m_pcMbDataCtrl->isPicDone( rcSH );
  rbFrameDone = m_pcMbDataCtrl->isFrameDone( rcSH );

  return Err::m_nOK;
}



ErrVal ControlMngH264AVCEncoder::initSliceForCoding( const SliceHeader& rcSH )
{
  m_uiCurrLayer   = rcSH.getSPS().getDependencyId();

  Bool bCabac = rcSH.getPPS().getEntropyCodingModeFlag();
  if( bCabac )
  {
    m_pcMbSymbolWriteIf = m_pcCabacWriter;
  }
  else
  {
    m_pcMbSymbolWriteIf = m_pcUvlcWriter;
  }

  RNOK( m_pcMbSymbolWriteIf   ->startSlice( rcSH ) );
  MbSymbolWriteIf *pcCurrentWriter = m_pcMbSymbolWriteIf;
  for( UInt uiMGSFragments = 0; rcSH.getSPS().getMGSCoeffStop( uiMGSFragments ) < 16; uiMGSFragments++ )
  {
    pcCurrentWriter = pcCurrentWriter->getSymbolWriteIfNextSlice();
    RNOK( pcCurrentWriter->startSlice( rcSH ) );
  }
  RNOK( m_pcMbEncoder         ->initSlice ( rcSH ) )
  RNOK( m_pcMbCoder           ->initSlice ( rcSH, m_pcMbSymbolWriteIf, m_pcRateDistortion ) );
  RNOK( m_pcMotionEstimation  ->initSlice ( rcSH ) );
  RNOK( m_pcSampleWeighting   ->initSlice ( rcSH ) );

  setFMO( rcSH.getFMO());

  return Err::m_nOK;
}


ErrVal ControlMngH264AVCEncoder::initSliceForFiltering( const SliceHeader& rcSH )
{
  m_uiCurrLayer   = rcSH.getSPS().getDependencyId();

  return Err::m_nOK;
}


ErrVal ControlMngH264AVCEncoder::initMbForCoding( MbDataAccess& rcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff, Bool bFieldFlag )
{
  ROF( m_uiCurrLayer < MAX_LAYERS );

  if( bMbAff )
  {
    rcMbDataAccess.setFieldMode( bFieldFlag );
  }
  else
  {
    rcMbDataAccess.getMbMotionData( LIST_0 ).setFieldMode( false );
    rcMbDataAccess.getMbMotionData( LIST_1 ).setFieldMode( false );
  }

  RNOK( m_apcYuvFullPelBufferCtrl[m_uiCurrLayer]->initMb( uiMbY, uiMbX, bMbAff ) );
  RNOK( m_apcYuvHalfPelBufferCtrl[m_uiCurrLayer]->initMb( uiMbY, uiMbX, bMbAff ) );

  RNOK( m_pcMotionEstimation->initMb( uiMbY, uiMbX, rcMbDataAccess ) );

  return Err::m_nOK;
}

ErrVal ControlMngH264AVCEncoder::initMbForFiltering( MbDataAccess*& rpcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff )
{
  ROF( m_uiCurrLayer < MAX_LAYERS );

  m_pcMbDataCtrl->initMb( rpcMbDataAccess, uiMbY, uiMbX );

  RNOK( m_apcYuvFullPelBufferCtrl[m_uiCurrLayer]->initMb( uiMbY, uiMbX, bMbAff ) );

  return Err::m_nOK;
}

ErrVal ControlMngH264AVCEncoder::initMbForFiltering( MbDataAccess& rcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff )
{
  ROF( m_uiCurrLayer < MAX_LAYERS );

  RNOK( m_apcYuvFullPelBufferCtrl[m_uiCurrLayer]->initMb( uiMbY, uiMbX, bMbAff ) );

  return Err::m_nOK;
}


ErrVal ControlMngH264AVCEncoder::initSliceForWeighting ( const SliceHeader& rcSH)
{
   return m_pcSampleWeighting->initSlice( rcSH );
}

H264AVC_NAMESPACE_END
