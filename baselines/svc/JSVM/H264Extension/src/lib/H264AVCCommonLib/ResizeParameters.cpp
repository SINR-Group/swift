
#include <stdio.h>
#include "ResizeParameters.h"
#include "H264AVCCommonLib/SliceHeader.h"

#define ROTREPORT(x,t) {if(x) {::printf("\n%s\n",t); assert(0); return Err::m_nInvalidParameter;} }

H264AVC_NAMESPACE_BEGIN

ErrVal
ResizeParameters::readPictureParameters( FILE* pFile, Bool bFrameMbsOnlyFlag )
{
  ROF( pFile );

  //===== read parameters =====
  Int iLeft = 0;
  Int iTop  = 0;
  Int iSW   = 0;
  Int iSH   = 0;
  if( fscanf( pFile, "%d,%d,%d,%d\n", &iLeft, &iTop, &iSW, &iSH ) != 4 )
  {
    printf("\nPictureParameters::read() : cannot read picture params\n");
    return Err::m_nERR;
  }

  //===== check parameters =====
  Int   iRight  = m_iFrameWidth  - iSW - iLeft;
  Int   iBot    = m_iFrameHeight - iSH - iTop;
  Int   iV      = ( bFrameMbsOnlyFlag ? 2 : 4 );
  Bool  bHor    = ( iLeft %  2 != 0 || iRight %  2 != 0 );
  Bool  bVer    = ( iTop  % iV != 0 || iBot   % iV != 0 );
  ROTREPORT( bHor,                       "Cropping window must be horizonzally aligned on a 2 pixel grid" );
  ROTREPORT( bVer &&  bFrameMbsOnlyFlag, "Cropping window must be vertically aligned on a 2 pixel grid" );
  ROTREPORT( bVer && !bFrameMbsOnlyFlag, "Cropping window must be vertically aligned on a 4 pixel grid for interlaced configurations" );
  ROTREPORT( iSW < m_iRefLayerFrmWidth,  "Cropping Window must be bigger than base layer" );
  ROTREPORT( iSH < m_iRefLayerFrmHeight, "Cropping Window must be bigger than base layer" );

  //===== adjust parameters =====
  if( m_iRefLayerFrmWidth != m_iRefLayerWidthInSamples )
  {
    Int iShift  = 1;
    Int iDiv    = m_iRefLayerWidthInSamples << iShift;
    iSW         = ( ( iSW * m_iRefLayerFrmWidth + ( iDiv >> 1 ) ) / iDiv ) << iShift;
  }
  if( m_iRefLayerFrmHeight != m_iRefLayerHeightInSamples )
  {
    Int iShift  = ( bFrameMbsOnlyFlag ? 1 : 2 );
    Int iDiv    = m_iRefLayerHeightInSamples << iShift;
    iSH         = ( ( iSH * m_iRefLayerFrmHeight + ( iDiv >> 1 ) ) / iDiv ) << iShift;
  }

  //===== set parameters =====
  m_iScaledRefFrmWidth  = iSW;
  m_iScaledRefFrmHeight = iSH;
  m_iLeftFrmOffset      = iLeft;
  m_iTopFrmOffset       = iTop;

  return Err::m_nOK;
}


Void
ResizeParameters::updateCurrLayerParameters( const SliceHeader& rcSH )
{
  m_iLevelIdc         = rcSH.getSPS().getLevelIdc();
  m_bFrameMbsOnlyFlag = rcSH.getSPS().getFrameMbsOnlyFlag();
  m_bFieldPicFlag     = rcSH.getFieldPicFlag();
  m_bBotFieldFlag     = rcSH.getBottomFieldFlag();
  m_bIsMbAffFrame     = rcSH.isMbaffFrame();
  m_iFrameWidth       = rcSH.getSPS().getFrameWidthInMbs () << 4;
  m_iFrameHeight      = rcSH.getSPS().getFrameHeightInMbs() << 4;
  m_iChromaPhaseX     = rcSH.getSPS().getChromaPhaseX();
  m_iChromaPhaseY     = rcSH.getSPS().getChromaPhaseY();
  if( rcSH.getQualityId() > 0 || rcSH.getNoInterLayerPredFlag() )
  {
    m_iExtendedSpatialScalability = ESS_NONE;
    m_iScaledRefFrmWidth          = m_iFrameWidth;
    m_iScaledRefFrmHeight         = m_iFrameHeight;
    m_iLeftFrmOffset              = 0;
    m_iTopFrmOffset               = 0;
    m_iRefLayerChromaPhaseX       = m_iChromaPhaseX;
    m_iRefLayerChromaPhaseY       = m_iChromaPhaseY;
  }
  else
  {
    m_iExtendedSpatialScalability = rcSH.getSPS().getExtendedSpatialScalability();
    m_iScaledRefFrmWidth          = m_iFrameWidth  - 2 * ( rcSH.getScaledRefLayerLeftOffset() + rcSH.getScaledRefLayerRightOffset () );
    m_iScaledRefFrmHeight         = m_iFrameHeight - 2 * ( rcSH.getScaledRefLayerTopOffset () + rcSH.getScaledRefLayerBottomOffset() );
    m_iLeftFrmOffset              = 2 * rcSH.getScaledRefLayerLeftOffset();
    m_iTopFrmOffset               = 2 * rcSH.getScaledRefLayerTopOffset ();
    m_iRefLayerChromaPhaseX       = rcSH.getRefLayerChromaPhaseX();
    m_iRefLayerChromaPhaseY       = rcSH.getRefLayerChromaPhaseY();
  }
}


Void
ResizeParameters::updateRefLayerParameters( const SliceHeader& rcSH )
{
  m_bRefLayerFrameMbsOnlyFlag   = rcSH.getSPS().getFrameMbsOnlyFlag();
  m_bRefLayerFieldPicFlag       = rcSH.getFieldPicFlag();
  m_bRefLayerBotFieldFlag       = rcSH.getBottomFieldFlag();
  m_bRefLayerIsMbAffFrame       = rcSH.isMbaffFrame();
  m_iRefLayerFrmWidth           = rcSH.getSPS().getFrameWidthInMbs () << 4;
  m_iRefLayerFrmHeight          = rcSH.getSPS().getFrameHeightInMbs() << 4;
}


Void
ResizeParameters::updatePicParameters( const PictureParameters& rcPP )
{
  m_iScaledRefFrmWidth    = rcPP.m_iScaledRefFrmWidth;
  m_iScaledRefFrmHeight   = rcPP.m_iScaledRefFrmHeight;
  m_iLeftFrmOffset        = rcPP.m_iLeftFrmOffset;
  m_iTopFrmOffset         = rcPP.m_iTopFrmOffset;
  m_iRefLayerChromaPhaseX = rcPP.m_iRefLayerChromaPhaseX;
  m_iRefLayerChromaPhaseY = rcPP.m_iRefLayerChromaPhaseY;
}


H264AVC_NAMESPACE_END
