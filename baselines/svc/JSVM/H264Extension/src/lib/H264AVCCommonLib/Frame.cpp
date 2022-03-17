
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/Frame.h"
#include "H264AVCCommonLib/MbDataCtrl.h"
#include "H264AVCCommonLib/QuarterPelFilter.h"
#include "DownConvert.h"

H264AVC_NAMESPACE_BEGIN

Frame::Frame( YuvBufferCtrl& rcYuvFullPelBufferCtrl, YuvBufferCtrl& rcYuvHalfPelBufferCtrl, PicType ePicType, Frame* pcFrame )
: m_cFullPelYuvBuffer     ( rcYuvFullPelBufferCtrl, ePicType ),
  m_cHalfPelYuvBuffer     ( rcYuvHalfPelBufferCtrl, ePicType ),
  m_ePicType              ( ePicType ),
  m_bHalfPel              ( false ),
  m_bExtended             ( false ),
	m_bPocIsSet             ( false ),
	m_iPoc                  ( 0 ),
  m_iTopFieldPoc          ( 0 ),
  m_iBotFieldPoc          ( 0 ),
  m_pcFrameTopField       ( NULL ),
  m_pcFrameBotField       ( NULL ),
  m_pcFrame               ( pcFrame ),
  m_pcDPBUnit             ( NULL ),
  m_piChannelDistortion   ( 0 )     // JVT-R057 LA-RDO
 ,m_bLongTerm( false )
 ,m_bUnvalid( false )
{
  if( m_pcFrame == 0 )
  {
    m_pcFrame = this;
  }
  else
  {
    m_cPicParameters    = m_pcFrame->m_cPicParameters;
    m_cPicParametersBot = m_pcFrame->m_cPicParametersBot;
  }
}

Frame::~Frame()
{
}

ErrVal Frame::create( Frame*& rpcIntFrame, YuvBufferCtrl& rcYuvFullPelBufferCtrl, YuvBufferCtrl& rcYuvHalfPelBufferCtrl, PicType ePicType, Frame* pcFrame  )
{
  rpcIntFrame = new Frame( rcYuvFullPelBufferCtrl, rcYuvHalfPelBufferCtrl, ePicType, pcFrame );
  ROT( NULL == rpcIntFrame );
  return Err::m_nOK;
}

ErrVal Frame::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal Frame::init()
{
  ASSERT( m_ePicType==FRAME );
  XPel* pData = 0;
  RNOK( getFullPelYuvBuffer()->init( pData ) );

	m_bPocIsSet = false;
  m_bExtended = false;
	m_bHalfPel  = false;
  return Err::m_nOK;
}

ErrVal Frame::initHalfPel( XPel*& rpucYuvBuffer )
{
  RNOK( getHalfPelYuvBuffer()->init( rpucYuvBuffer ) );
    m_bHalfPel = true;
  return Err::m_nOK;
}

ErrVal Frame::initHalfPel( )
  {
    XPel* pHPData = 0;
    return initHalfPel(pHPData);
  }


ErrVal Frame::uninit()
{
  if( m_ePicType==FRAME && NULL != m_pcFrameTopField )
  {
		// remove the default yuv memory from buffers
    RNOK( m_pcFrameTopField->uninit () );
		RNOK( m_pcFrameTopField->destroy() );
    m_pcFrameTopField = NULL;
  }

  if( m_ePicType==FRAME && NULL != m_pcFrameBotField )
  {
		// remove the default yuv memory from buffers
    RNOK( m_pcFrameBotField->uninit () );
		RNOK( m_pcFrameBotField->destroy() );
    m_pcFrameBotField = NULL;
  }

	// remove the default yuv memory from buffers
	RNOK( getFullPelYuvBuffer()->uninit() );
  RNOK( getHalfPelYuvBuffer()->uninit() );

	m_bPocIsSet = false;
  m_bHalfPel  = false;
  m_bExtended = false;

  return Err::m_nOK;
}

ErrVal Frame::uninitHalfPel()
{
  RNOK( getHalfPelYuvBuffer()->uninit() );

	m_bHalfPel  = false;
	m_bExtended = false;

  if( m_ePicType==FRAME && NULL != m_pcFrameTopField )
  {
    RNOK( m_pcFrameTopField->uninitHalfPel() );
  }
  if( m_ePicType==FRAME && NULL != m_pcFrameBotField )
  {
    RNOK( m_pcFrameBotField->uninitHalfPel() );
  }

	return Err::m_nOK;
}

ErrVal Frame::load( PicBuffer* pcPicBuffer )
{
  ASSERT( m_ePicType==FRAME );
  RNOK( getFullPelYuvBuffer()->loadFromPicBuffer( pcPicBuffer ) );
  return Err::m_nOK;
}

ErrVal Frame::store( PicBuffer* pcPicBuffer, PicType ePicType )
{
  ASSERT( m_ePicType == FRAME );
  RNOK  ( getFullPelYuvBuffer()->storeToPicBuffer     ( pcPicBuffer ) );
  ROTRS ( ePicType   == FRAME, Err::m_nOK );
  RNOK  ( getFullPelYuvBuffer()->interpolatedPicBuffer( pcPicBuffer, ePicType == TOP_FIELD ) );
  return Err::m_nOK;
}

const Frame* Frame::getPic( PicType ePicType ) const
{
  ASSERT( m_ePicType==FRAME );
  switch( ePicType )
  {
  case FRAME:
    return this;
    break;
  case TOP_FIELD:
    ASSERT( m_pcFrameTopField != NULL );
    return m_pcFrameTopField;
    break;
  case BOT_FIELD:
    ASSERT( m_pcFrameBotField != NULL );
    return m_pcFrameBotField;
    break;
  default:
    return NULL;
    break;
  }
  return NULL;
}

Frame* Frame::getPic( PicType ePicType )
{
  ASSERT( m_ePicType==FRAME );
	switch( ePicType )
	{
	case FRAME:
		return this;
		break;
	case TOP_FIELD:
		ASSERT( m_pcFrameTopField != NULL );
		return m_pcFrameTopField;
		break;
	case BOT_FIELD:
		ASSERT( m_pcFrameBotField != NULL );
		return m_pcFrameBotField;
		break;
	default:
		return NULL;
		break;
	}
	return NULL;
}

ErrVal Frame::removeFieldBuffer( PicType ePicType )
{
  ASSERT( m_ePicType==FRAME );

  if( ePicType==TOP_FIELD )
  {
		if( NULL != m_pcFrameTopField )
    {
      // remove the default yuv memory from buffers
      RNOK( m_pcFrameTopField->uninit () );
      RNOK( m_pcFrameTopField->destroy() );
      m_pcFrameTopField = NULL;
    }
  }
  else if( ePicType==BOT_FIELD )
  {
    if( NULL != m_pcFrameBotField )
    {
      // remove the default yuv memory from buffers
      RNOK( m_pcFrameBotField->uninit () );
      RNOK( m_pcFrameBotField->destroy() );
      m_pcFrameBotField = NULL;
    }
  }

  return Err::m_nOK;
}

ErrVal Frame::removeFrameFieldBuffer()
{
  ASSERT( m_ePicType==FRAME );

  RNOK( removeFieldBuffer( TOP_FIELD ) );
  RNOK( removeFieldBuffer( BOT_FIELD ) );

  return Err::m_nOK;
}

ErrVal Frame::addFieldBuffer( PicType ePicType )
{
  ASSERT( m_ePicType==FRAME );

  if( ePicType==FRAME )
  {
    return Err::m_nOK;
  }
  if( ePicType == TOP_FIELD )
  {
    if( NULL != m_pcFrameTopField )
		{
			RNOK( m_pcFrameTopField->uninit() );
		}
		if( NULL == m_pcFrameTopField )
    {
			YuvBufferCtrl& rcYuvFullPelBufferCtrl = getFullPelYuvBuffer()->getBufferCtrl();
			YuvBufferCtrl& rcYuvHalfPelBufferCtrl = getHalfPelYuvBuffer()->getBufferCtrl();

			RNOK( Frame::create( m_pcFrameTopField, rcYuvFullPelBufferCtrl, rcYuvHalfPelBufferCtrl, TOP_FIELD, this ) );
		}

    // creates private full pel buffer
    XPel* pData = getFullPelYuvBuffer()->getBuffer();
    RNOK( m_pcFrameTopField->getFullPelYuvBuffer()->init( pData ) );
    RNOK( m_pcFrameTopField->getFullPelYuvBuffer()->fillMargin() );

    m_pcFrameTopField->setPoc( m_iTopFieldPoc );
    m_pcFrameTopField->setLongTerm( m_bLongTerm );
  }
  else if( ePicType == BOT_FIELD )
  {
		if( NULL != m_pcFrameBotField )
		{
			RNOK( m_pcFrameBotField->uninit() );
		}
		if( NULL == m_pcFrameBotField )
    {
			YuvBufferCtrl& rcYuvFullPelBufferCtrl = getFullPelYuvBuffer()->getBufferCtrl();
			YuvBufferCtrl& rcYuvHalfPelBufferCtrl = getHalfPelYuvBuffer()->getBufferCtrl();

			RNOK( Frame::create( m_pcFrameBotField, rcYuvFullPelBufferCtrl, rcYuvHalfPelBufferCtrl, BOT_FIELD, this ) );
		}

    // creates private full pel buffer
    XPel* pData = getFullPelYuvBuffer()->getBuffer();
    RNOK( m_pcFrameBotField->getFullPelYuvBuffer()->init( pData ) );
    RNOK( m_pcFrameBotField->getFullPelYuvBuffer()->fillMargin() );

    m_pcFrameBotField->setPoc( m_iBotFieldPoc );
    m_pcFrameBotField->setLongTerm( m_bLongTerm );
  }

  return Err::m_nOK;
}

ErrVal Frame::addFrameFieldBuffer()
{
  ASSERT( m_ePicType==FRAME );

  RNOK( addFieldBuffer( TOP_FIELD ) );
  RNOK( addFieldBuffer( BOT_FIELD ) );

  return Err::m_nOK;
}

ErrVal Frame::extendFrame( QuarterPelFilter* pcQuarterPelFilter, PicType ePicType, Bool bFrameMbsOnlyFlag )
{
	ASSERT( m_ePicType==FRAME );

	const Bool bNoHalfPel = ( NULL == pcQuarterPelFilter );

	if( NULL != m_pcFrameTopField || NULL != m_pcFrameBotField )
  {
    RNOK( removeFrameFieldBuffer() );
  }

	// perform border padding on the full pel buffer
  RNOK( getFullPelYuvBuffer()->fillMargin( ) );
  m_bExtended     = true;

	if( ! bFrameMbsOnlyFlag )
	{
		if( ePicType==FRAME )
		{
			if( NULL == m_pcFrameTopField || NULL == m_pcFrameBotField )
			{
				ROT( NULL != m_pcFrameTopField );
        ROT( NULL != m_pcFrameBotField );
				YuvBufferCtrl& rcYuvFullPelBufferCtrl = getFullPelYuvBuffer()->getBufferCtrl();
				YuvBufferCtrl& rcYuvHalfPelBufferCtrl = getHalfPelYuvBuffer()->getBufferCtrl();

				RNOK( Frame::create( m_pcFrameTopField, rcYuvFullPelBufferCtrl, rcYuvHalfPelBufferCtrl, TOP_FIELD, this ) );
				RNOK( Frame::create( m_pcFrameBotField, rcYuvFullPelBufferCtrl, rcYuvHalfPelBufferCtrl, BOT_FIELD, this ) );

				// creates private full pel buffer
				XPel* pData = NULL;
				RNOK( m_pcFrameTopField->getFullPelYuvBuffer()->init( pData ) );
//TMM {
        m_pcFrameTopField->setPoc     ( m_iTopFieldPoc );
        m_pcFrameTopField->setLongTerm( m_bLongTerm );
//TMM }
        pData = NULL;
				RNOK( m_pcFrameBotField->getFullPelYuvBuffer()->init( pData ) );
//TMM {
        m_pcFrameBotField->setPoc     ( m_iBotFieldPoc );
        m_pcFrameBotField->setLongTerm( m_bLongTerm );
//TMM }
			}
		}
		else
		{
			RNOK( addFrameFieldBuffer() );
		}

  // perform border padding on the full pel buffer
		RNOK( m_pcFrameTopField->getFullPelYuvBuffer()->loadBufferAndFillMargin( getFullPelYuvBuffer() ) );
		RNOK( m_pcFrameBotField->getFullPelYuvBuffer()->loadBufferAndFillMargin( getFullPelYuvBuffer() ) );
		m_pcFrameTopField->setExtended();
		m_pcFrameBotField->setExtended();

		if( ! bNoHalfPel )
		{
			XPel* pHPData = NULL;
			RNOK( m_pcFrameTopField->initHalfPel( pHPData ) );
      pHPData = NULL;
			RNOK( m_pcFrameBotField->initHalfPel( pHPData ) );
		}
	}

  // if cond is true no sub pel buffer is used
  ROTRS( bNoHalfPel, Err::m_nOK );

  // create half pel samples
  RNOK( pcQuarterPelFilter->filterFrame(                         getFullPelYuvBuffer(),                       getHalfPelYuvBuffer() ) );

	if( ! bFrameMbsOnlyFlag )
	{
		RNOK( pcQuarterPelFilter->filterFrame( m_pcFrameTopField->getFullPelYuvBuffer(), m_pcFrameTopField->getHalfPelYuvBuffer() ) );
		RNOK( pcQuarterPelFilter->filterFrame( m_pcFrameBotField->getFullPelYuvBuffer(), m_pcFrameBotField->getHalfPelYuvBuffer() ) );
	}

  return Err::m_nOK;
}


ErrVal
Frame::intraUpsampling( Frame*                pcBaseFrame,
                        Frame*                pcTempBaseFrame,
                        Frame*                pcTempFrame,
                        DownConvert&          rcDownConvert,
                        ResizeParameters*     pcParameters,
                        MbDataCtrl*           pcMbDataCtrlBase,
                        MbDataCtrl*           pcMbDataCtrlPredFrm,
                        MbDataCtrl*           pcMbDataCtrlPredFld,
                        ReconstructionBypass* pcReconstructionBypass,
                        Bool                  bConstrainedIntraUpsamplingFlag,
                        Bool*                 pabBaseModeAllowedFlagArrayFrm,
                        Bool*                 pabBaseModeAllowedFlagArrayFld )
{
  AOF ( m_ePicType == FRAME );
  rcDownConvert.intraUpsampling( this, pcBaseFrame, pcTempFrame, pcTempBaseFrame, pcParameters,
                                 pcMbDataCtrlBase, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld,
                                 pcReconstructionBypass, pabBaseModeAllowedFlagArrayFrm, pabBaseModeAllowedFlagArrayFld, bConstrainedIntraUpsamplingFlag );
  return Err::m_nOK;
}

ErrVal
Frame::residualUpsampling( Frame*             pcBaseFrame,
                           DownConvert&       rcDownConvert,
                           ResizeParameters*  pcParameters,
                           MbDataCtrl*        pcMbDataCtrlBase )
{
  AOF ( m_ePicType == FRAME );
  rcDownConvert.residualUpsampling( this, pcBaseFrame, pcParameters, pcMbDataCtrlBase );
  return Err::m_nOK;
}


Void Frame::initChannelDistortion()
{
	if(!m_piChannelDistortion)
	{
		UInt  uiMbY  = getFullPelYuvBuffer()->getLHeight()/4;
		UInt  uiMbX  = getFullPelYuvBuffer()->getLWidth()/4;
		UInt  uiSize = uiMbX*uiMbY;
		m_piChannelDistortion= new UInt[uiSize];
	}
}

Void Frame::copyChannelDistortion(Frame*p1)
{
	UInt  uiMbY  = getFullPelYuvBuffer()->getLHeight()/16;
	UInt  uiMbX  = getFullPelYuvBuffer()->getLWidth()/16;
	for(UInt y=0;y<uiMbY*4;y++)
	{
		for(UInt x=0;x<uiMbX*4;x++)
		{
			m_piChannelDistortion[y*(uiMbX*4)+x]=p1->m_piChannelDistortion[y*(uiMbX*4)+x];
		}
	}
}

Void Frame::zeroChannelDistortion()
{
	UInt  uiMbY  = getFullPelYuvBuffer()->getLHeight()/16;
	UInt  uiMbX  = getFullPelYuvBuffer()->getLWidth()/16;
	for(UInt y=0;y<uiMbY*4;y++)
	{
		for(UInt x=0;x<uiMbX*4;x++)
		{
			m_piChannelDistortion[y*(uiMbX*4)+x]=0;
		}
	}
}

const PictureParameters&
Frame::getPicParameters() const
{
  return getPicParameters( m_ePicType );
}

const PictureParameters&
Frame::getPicParameters( PicType ePicType ) const
{
  if( ePicType == BOT_FIELD )
  {
    return m_cPicParametersBot;
  }
  return m_cPicParameters;
}

ErrVal
Frame::setPicParameters( const ResizeParameters& rcRP, const SliceHeader* pcSH )
{
  PictureParameters cPP;
  cPP.m_iScaledRefFrmWidth    = rcRP.m_iScaledRefFrmWidth;
  cPP.m_iScaledRefFrmHeight   = rcRP.m_iScaledRefFrmHeight;
  cPP.m_iLeftFrmOffset        = rcRP.m_iLeftFrmOffset;
  cPP.m_iTopFrmOffset         = rcRP.m_iTopFrmOffset;
  cPP.m_iRefLayerChromaPhaseX = rcRP.m_iRefLayerChromaPhaseX;
  cPP.m_iRefLayerChromaPhaseY = rcRP.m_iRefLayerChromaPhaseY;

  PicType ePicType = FRAME;
  if( pcSH && pcSH->getFieldPicFlag() )
  {
    ePicType = ( pcSH->getBottomFieldFlag() ? BOT_FIELD : TOP_FIELD );
  }

  RNOK( setPicParameters( cPP, ePicType ) );
  return Err::m_nOK;
}

ErrVal
Frame::setPicParameters( const PictureParameters& rcPP, PicType ePicType )
{
  //===== set in current pictures =====
  if( ePicType == FRAME || ePicType == TOP_FIELD )
  {
    m_cPicParameters    = rcPP;
  }
  if( ePicType == FRAME || ePicType == BOT_FIELD )
  {
    m_cPicParametersBot = rcPP;
  }
  RNOK( xUpdatePicParameters() );
  return Err::m_nOK;
}

ErrVal
Frame::copyPicParameters( const Frame& rcFrame, PicType ePicType )
{
  //===== set in current pictures =====
  if( ePicType == FRAME || ePicType == TOP_FIELD )
  {
    m_cPicParameters    = rcFrame.m_cPicParameters;
  }
  if( ePicType == FRAME || ePicType == BOT_FIELD )
  {
    m_cPicParametersBot = rcFrame.m_cPicParametersBot;
  }
  RNOK( xUpdatePicParameters() );
  return Err::m_nOK;
}

ErrVal
Frame::xUpdatePicParameters()
{
  //===== set in associated frame or top and bot field =====
  if( m_ePicType == FRAME )
  {
    if( m_pcFrameTopField )
    {
      m_pcFrameTopField->m_cPicParameters     = m_cPicParameters;
      m_pcFrameTopField->m_cPicParametersBot  = m_cPicParametersBot;
    }
    if( m_pcFrameBotField )
    {
      m_pcFrameBotField->m_cPicParameters     = m_cPicParameters;
      m_pcFrameBotField->m_cPicParametersBot  = m_cPicParametersBot;
    }
  }
  else if( m_ePicType == TOP_FIELD )
  {
    ROF( m_pcFrame );
    m_pcFrame->m_cPicParameters     = m_cPicParameters;
    m_pcFrame->m_cPicParametersBot  = m_cPicParametersBot;
    if( m_pcFrame->m_pcFrameBotField )
    {
      m_pcFrameBotField->m_cPicParameters     = m_cPicParameters;
      m_pcFrameBotField->m_cPicParametersBot  = m_cPicParametersBot;
    }
  }
  else if( m_ePicType == BOT_FIELD )
  {
    ROF( m_pcFrame );
    m_pcFrame->m_cPicParameters     = m_cPicParameters;
    m_pcFrame->m_cPicParametersBot  = m_cPicParametersBot;
    if( m_pcFrame->m_pcFrameTopField )
    {
      m_pcFrameTopField->m_cPicParameters     = m_cPicParameters;
      m_pcFrameTopField->m_cPicParametersBot  = m_cPicParametersBot;
    }
  }
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END
