
#include "H264AVCDecoderLib.h"
#include "ControlMngH264AVCDecoder.h"


H264AVC_NAMESPACE_BEGIN

ControlMngH264AVCDecoder::ControlMngH264AVCDecoder()
: m_pcMbDataCtrl          ( NULL )
, m_pcUvlcReader          ( NULL )
, m_pcMbParser            ( NULL )
, m_pcMotionCompensation  ( NULL )
, m_pcCabacReader         ( NULL )
, m_pcSampleWeighting     ( NULL )
, m_uiCurrLayer           ( MSYS_UINT_MAX )
{
  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    m_auiMbXinFrame           [uiLayer] = 0;
    m_auiMbYinFrame           [uiLayer] = 0;
    m_apcLayerDecoder         [uiLayer] = NULL;
    m_apcYuvFullPelBufferCtrl [uiLayer] = NULL;
		m_uiInitialized           [uiLayer] = false;
  }
}


ControlMngH264AVCDecoder::~ControlMngH264AVCDecoder()
{
}


ErrVal
ControlMngH264AVCDecoder::init( UvlcReader*          pcUvlcReader,
                                MbParser*            pcMbParser,
                                MotionCompensation*  pcMotionCompensation,
                                YuvBufferCtrl*       apcYuvFullPelBufferCtrl [MAX_LAYERS],
                                CabacReader*         pcCabacReader,
                                SampleWeighting*     pcSampleWeighting,
                                LayerDecoder*        apcLayerDecoder         [MAX_LAYERS] )
{
  ROF( pcUvlcReader );
  ROF( pcMbParser );
  ROF( pcMotionCompensation );
  ROF( pcCabacReader );
  ROF( pcSampleWeighting );

  m_uiCurrLayer           = MSYS_UINT_MAX;
  m_pcUvlcReader          = pcUvlcReader;
  m_pcMbParser            = pcMbParser;
  m_pcMotionCompensation  = pcMotionCompensation;
  m_pcCabacReader         = pcCabacReader;
  m_pcSampleWeighting     = pcSampleWeighting;

  ROF( apcLayerDecoder );
  ROF( apcYuvFullPelBufferCtrl );

  for( UInt uiLayer = 0; uiLayer < MAX_LAYERS; uiLayer++ )
  {
    ROF( apcLayerDecoder        [uiLayer] );
    ROF( apcYuvFullPelBufferCtrl[uiLayer] );

    m_apcLayerDecoder         [uiLayer] = apcLayerDecoder         [uiLayer];
    m_apcYuvFullPelBufferCtrl [uiLayer] = apcYuvFullPelBufferCtrl [uiLayer];
  }

  return Err::m_nOK;
}


ErrVal
ControlMngH264AVCDecoder::uninit()
{
  return Err::m_nOK;
}


ErrVal
ControlMngH264AVCDecoder::create( ControlMngH264AVCDecoder*& rpcControlMngH264AVCDecoder )
{
  rpcControlMngH264AVCDecoder = new ControlMngH264AVCDecoder;
  ROF( rpcControlMngH264AVCDecoder );
  return Err::m_nOK;
}


ErrVal
ControlMngH264AVCDecoder::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal
ControlMngH264AVCDecoder::initMbForParsing( MbDataAccess*& rpcMbDataAccess, UInt uiMbIndex )
{
  ROF( m_uiCurrLayer < MAX_LAYERS );

  UInt uiMbY, uiMbX;

  uiMbY = uiMbIndex         / m_auiMbXinFrame[m_uiCurrLayer];
  uiMbX = uiMbIndex - uiMbY * m_auiMbXinFrame[m_uiCurrLayer];

  RNOK( m_pcMbDataCtrl                          ->initMb( rpcMbDataAccess, uiMbY, uiMbX ) );
  RNOK( m_apcYuvFullPelBufferCtrl[m_uiCurrLayer]->initMb(                  uiMbY, uiMbX, false ) );

  return Err::m_nOK;
}

ErrVal
ControlMngH264AVCDecoder::initMbForDecoding( MbDataAccess*& rpcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff )
{
  ROF( m_uiCurrLayer < MAX_LAYERS );

  RNOK( m_pcMbDataCtrl                          ->initMb( rpcMbDataAccess, uiMbY, uiMbX                   ) );
  RNOK( m_apcYuvFullPelBufferCtrl[m_uiCurrLayer]->initMb(                  uiMbY, uiMbX, bMbAff           ) );
  RNOK( m_pcMotionCompensation                  ->initMb(                  uiMbY, uiMbX, *rpcMbDataAccess ) ) ;

  return Err::m_nOK;
}

ErrVal
ControlMngH264AVCDecoder::initMbForDecoding( MbDataAccess& rcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff )
{
  ROF( m_uiCurrLayer < MAX_LAYERS );

  RNOK( m_apcYuvFullPelBufferCtrl[m_uiCurrLayer]->initMb(                  uiMbY, uiMbX, bMbAff         ) );
  RNOK( m_pcMotionCompensation                  ->initMb( uiMbY, uiMbX, rcMbDataAccess ) ) ;

  return Err::m_nOK;
}

ErrVal
ControlMngH264AVCDecoder::initMbForFiltering( MbDataAccess*& rpcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff )
{
  ROF( m_uiCurrLayer < MAX_LAYERS );


  RNOK( m_pcMbDataCtrl                          ->initMb( rpcMbDataAccess, uiMbY, uiMbX           ) );
  RNOK( m_apcYuvFullPelBufferCtrl[m_uiCurrLayer]->initMb(                  uiMbY, uiMbX, bMbAff         ) );

  return Err::m_nOK;
}


ErrVal
ControlMngH264AVCDecoder::initMbForFiltering( MbDataAccess& rcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff  )
{
  ROF( m_uiCurrLayer < MAX_LAYERS );

  RNOK( m_apcYuvFullPelBufferCtrl[m_uiCurrLayer]->initMb( uiMbY, uiMbX, bMbAff ) );
  return Err::m_nOK;
}


ErrVal ControlMngH264AVCDecoder::initSlice0( SliceHeader *rcSH )
{
  UInt  uiLayer = rcSH->getDependencyId();

  ROTRS( m_uiInitialized[uiLayer], Err::m_nOK );
  m_auiMbXinFrame[uiLayer]  = rcSH->getSPS().getFrameWidthInMbs   ();
  m_auiMbYinFrame[uiLayer]  = rcSH->getSPS().getFrameHeightInMbs  ();

  UInt uiSizeX = rcSH->getSPS().getAllocFrameMbsX() << 4;
  UInt uiSizeY = rcSH->getSPS().getAllocFrameMbsY() << 4;
  RNOK( m_apcYuvFullPelBufferCtrl [uiLayer]->initSlice( uiSizeY, uiSizeX, YUV_Y_MARGIN, YUV_X_MARGIN ) );

	m_uiInitialized[uiLayer] = true;

  return Err::m_nOK;
}


ErrVal ControlMngH264AVCDecoder::initSPS( SequenceParameterSet& rcSequenceParameterSet, UInt  uiLayer )
{
  m_auiMbXinFrame[uiLayer]  = rcSequenceParameterSet.getFrameWidthInMbs   ();
  m_auiMbYinFrame[uiLayer]  = rcSequenceParameterSet.getFrameHeightInMbs  ();
  return Err::m_nOK;
}


ErrVal
ControlMngH264AVCDecoder::initSliceForReading( const SliceHeader& rcSH )
{
  m_uiCurrLayer   = rcSH.getDependencyId();

  MbSymbolReadIf* pcMbSymbolReadIf;

  if( rcSH.getPPS().getEntropyCodingModeFlag() )
  {
    pcMbSymbolReadIf = m_pcCabacReader;
  }
  else
  {
    pcMbSymbolReadIf = m_pcUvlcReader;
  }

	if ( rcSH.isTrueSlice())
	{
		RNOK( pcMbSymbolReadIf->startSlice( rcSH ) );
	}
  RNOK( m_pcMbParser->initSlice( pcMbSymbolReadIf ) );

  return Err::m_nOK;
}


ErrVal
ControlMngH264AVCDecoder::initSliceForDecoding( const SliceHeader& rcSH )
{
  m_uiCurrLayer   = rcSH.getDependencyId();

  RNOK( m_pcMotionCompensation->initSlice( rcSH ) );
  RNOK( m_pcSampleWeighting->initSlice( rcSH ) );

  return Err::m_nOK;
}


ErrVal
ControlMngH264AVCDecoder::initSliceForFiltering( const SliceHeader& rcSH )
{
  m_uiCurrLayer   = rcSH.getDependencyId();
  return Err::m_nOK;
}


ErrVal
ControlMngH264AVCDecoder::finishSlice( const SliceHeader& rcSH, Bool& rbPicDone, Bool& rbFrameDone )
{
  rbPicDone     = m_pcMbDataCtrl->isPicDone( rcSH );
  rbFrameDone   = m_pcMbDataCtrl->isFrameDone( rcSH );
  m_uiCurrLayer = MSYS_UINT_MAX;

  return Err::m_nOK;
}

H264AVC_NAMESPACE_END
