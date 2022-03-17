
#include "H264AVCEncoderLib.h"
#include "MotionEstimationCost.h"


H264AVC_NAMESPACE_BEGIN


MotionEstimationCost::MotionEstimationCost()
: m_pcRateDistortionIf    (  0 )
, m_puiComponentCostAlloc (  0 )
, m_puiComponentCost      (  0 )
, m_puiHorCost            (  0 )
, m_puiVerCost            (  0 )
, m_uiMvScaleShift        (  0 )
, m_uiCostFactor          (  0 )
, m_iSubPelSearchLimit    ( -1 )
{
}

MotionEstimationCost::~MotionEstimationCost()
{
  ANOK( xUninit() );
}

ErrVal
MotionEstimationCost::xInit( const Int iSubPelSearchLimit, RateDistortionIf* pcRateDistortionIf )
{
  ROF( iSubPelSearchLimit >= 0 );
  ROF( pcRateDistortionIf );
  m_pcRateDistortionIf = pcRateDistortionIf;
  if( m_iSubPelSearchLimit != iSubPelSearchLimit )
  {
    RNOK( xUninit() );
    m_iSubPelSearchLimit    = iSubPelSearchLimit;
    Int iNumPositionsDiv2   = ( iSubPelSearchLimit + 4 )  << 4;
    m_puiComponentCostAlloc = new UInt[ iNumPositionsDiv2 << 1 ];
    m_puiComponentCost      = m_puiComponentCostAlloc + iNumPositionsDiv2;
    for( Int iPos = -iNumPositionsDiv2; iPos < iNumPositionsDiv2; iPos++ )
    {
      m_puiComponentCost[ iPos ] = xGetComponentBits( iPos );
    }
  }
  return Err::m_nOK;
}

ErrVal 
MotionEstimationCost::xUninit()
{
  if( m_puiComponentCostAlloc )
  {
    delete [] m_puiComponentCostAlloc;
    m_puiComponentCostAlloc = 0;
  }
  return Err::m_nOK;
}

ErrVal
MotionEstimationCost::xSetMEPars( const UInt uiMvScaleShift, const Bool bSad )
{
  ROF( m_pcRateDistortionIf );
  m_uiMvScaleShift  = uiMvScaleShift;
  m_uiCostFactor    = m_pcRateDistortionIf->getMotionCostShift( bSad );
  return Err::m_nOK;
}

ErrVal
MotionEstimationCost::xSetPredictor( const Mv& rcMv )
{
  m_puiHorCost = m_puiComponentCost - rcMv.getHor();
  m_puiVerCost = m_puiComponentCost - rcMv.getVer();
  return Err::m_nOK;
}

UInt 
MotionEstimationCost::xGetComponentBits( Int iPos ) const
{
  UInt   uiCodeLength = 1;
  UInt   uiAbs        = UInt( iPos < 0 ? -iPos : iPos );
  UInt   uiTempVal    = ( uiAbs << 1 ) + 1;
  while( uiTempVal != 1 )
  {
    uiTempVal   >>= 1;
    uiCodeLength += 2;
  }
  return uiCodeLength;
}

H264AVC_NAMESPACE_END
