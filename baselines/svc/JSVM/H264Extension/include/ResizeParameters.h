
#ifndef _RESIZE_PARAMETERS_H_
#define _RESIZE_PARAMETERS_H_

#include "Typedefs.h"
#include "Macros.h"
#ifdef DOWN_CONVERT_STATIC
#include <iostream>
#include <assert.h>
#define ESS_NONE  0
#define ESS_SEQ   1
#define ESS_PICT  2
#else
#include "H264AVCCommonLib.h"

H264AVC_NAMESPACE_BEGIN

class SliceHeader;
class ResizeParameters;

#endif


#define SST_RATIO_1   0
#define SST_RATIO_2   1
#define SST_RATIO_3_2 2
#define SST_RATIO_X   3


__inline Int CeilLog2( Int i )
{
  Int s = 0; i--;
  while( i > 0 )
  {
    s++;
    i >>= 1;
  }
  return s;
}


class PictureParameters 
{
public:
  Void copy ( const PictureParameters& rcPicParam )
  {
    m_iScaledRefFrmWidth    = rcPicParam.m_iScaledRefFrmWidth;
    m_iScaledRefFrmHeight   = rcPicParam.m_iScaledRefFrmHeight;
    m_iLeftFrmOffset        = rcPicParam.m_iLeftFrmOffset;
    m_iTopFrmOffset         = rcPicParam.m_iTopFrmOffset;
    m_iRefLayerChromaPhaseX = rcPicParam.m_iRefLayerChromaPhaseX;
    m_iRefLayerChromaPhaseY = rcPicParam.m_iRefLayerChromaPhaseY;
  }
  const PictureParameters& operator = ( const PictureParameters& rcPicParam )
  {
    copy( rcPicParam );
    return *this;
  }

public:
  Int   m_iScaledRefFrmWidth;
  Int   m_iScaledRefFrmHeight;
  Int   m_iLeftFrmOffset;
  Int   m_iTopFrmOffset;
  Int   m_iRefLayerChromaPhaseX;
  Int   m_iRefLayerChromaPhaseY;
};


class ResizeParameters 
{
public:
    ResizeParameters()
      : m_iExtendedSpatialScalability ( ESS_NONE )
      , m_iLevelIdc                   ( 0 )
      , m_bFrameMbsOnlyFlag           ( true )
      , m_bFieldPicFlag               ( false )
      , m_bBotFieldFlag               ( false )
      , m_bIsMbAffFrame               ( false )
      , m_iFrameWidth                 ( 0 )
      , m_iFrameHeight                ( 0 )
      , m_iWidthInSamples             ( 0 )
      , m_iHeightInSamples            ( 0 )
      , m_iChromaPhaseX               ( 0 )
      , m_iChromaPhaseY               ( 0 )
      , m_iScaledRefFrmWidth          ( 0 )
      , m_iScaledRefFrmHeight         ( 0 )
      , m_iLeftFrmOffset              ( 0 )
      , m_iTopFrmOffset               ( 0 )
      , m_iRefLayerChromaPhaseX       ( 0 )
      , m_iRefLayerChromaPhaseY       ( 0 )
      , m_bRefLayerFrameMbsOnlyFlag   ( true )
      , m_bRefLayerFieldPicFlag       ( false )
      , m_bRefLayerBotFieldFlag       ( false )
      , m_bRefLayerIsMbAffFrame       ( false )
      , m_iRefLayerFrmWidth           ( 0 )
      , m_iRefLayerFrmHeight          ( 0 )
      , m_iRefLayerWidthInSamples     ( 0 )
      , m_iRefLayerHeightInSamples    ( 0 )
    { 
    };

#ifdef DOWN_CONVERT_STATIC
#else
    ErrVal  readPictureParameters     ( FILE* pFile, Bool bFrameMbsOnlyFlag );
    
    Void    updateCurrLayerParameters ( const SliceHeader&        rcSH );
    Void    updateRefLayerParameters  ( const SliceHeader&        rcSH );
    Void    updatePicParameters       ( const PictureParameters&  rcPP );

    Int     getLeftFrmOffset          ()  const { return m_iLeftFrmOffset; }
    Int     getTopFrmOffset           ()  const { return m_iTopFrmOffset; }
    Int     getRightFrmOffset         ()  const { return m_iFrameWidth  - m_iScaledRefFrmWidth  - m_iLeftFrmOffset; }
    Int     getBotFrmOffset           ()  const { return m_iFrameHeight - m_iScaledRefFrmHeight - m_iTopFrmOffset; }
#endif

    Bool  getCroppingChangeFlag() const
    {
      return ( m_iExtendedSpatialScalability == ESS_PICT );
    }
    Bool  getCroppingFlag() const
    {
      ROTRS( m_iExtendedSpatialScalability == ESS_PICT,   true );
      ROFRS( m_iLeftFrmOffset == 0,                       true );
      ROFRS( m_iTopFrmOffset  == 0,                       true );
      ROFRS( m_iFrameWidth    == m_iScaledRefFrmWidth,    true );
      ROFRS( m_iFrameHeight   == m_iScaledRefFrmHeight,   true );
      return false;
    }
    Bool  getSpatialResolutionChangeFlag() const
    {
      ROTRS( m_iExtendedSpatialScalability == ESS_PICT,                                     true );
      ROFRS( m_bFieldPicFlag         == m_bRefLayerFieldPicFlag,                            true );
      ROFRS( m_bIsMbAffFrame         == m_bRefLayerIsMbAffFrame,                            true );
      ROFRS( ( m_iLeftFrmOffset %                                             16   ) == 0,  true );
      ROFRS( ( m_iTopFrmOffset  % ( m_bFieldPicFlag || m_bIsMbAffFrame ? 32 : 16 ) ) == 0,  true );
      ROFRS( m_iRefLayerFrmWidth     == m_iScaledRefFrmWidth,                               true );
      ROFRS( m_iRefLayerFrmHeight    == m_iScaledRefFrmHeight,                              true );
      ROFRS( m_iRefLayerChromaPhaseX == m_iChromaPhaseX,                                    true );
      ROFRS( m_iRefLayerChromaPhaseY == m_iChromaPhaseY,                                    true );
      return false;
    }
    Bool  getRestrictedSpatialResolutionChangeFlag() const
    {
      ROFRS( getSpatialResolutionChangeFlag(),                                                                    true  );
      ROFRS( m_bFieldPicFlag          == m_bRefLayerFieldPicFlag,                                                 false );
      ROFRS( m_bIsMbAffFrame          == false,                                                                   false );
      ROFRS( m_bRefLayerIsMbAffFrame  == false,                                                                   false );
      ROFRS( ( m_iLeftFrmOffset %                          16   ) == 0,                                           false );
      ROFRS( ( m_iTopFrmOffset  % ( m_bFieldPicFlag ? 32 : 16 ) ) == 0,                                           false );
      ROFRS( m_iRefLayerFrmHeight == m_iScaledRefFrmHeight || 2 * m_iRefLayerFrmHeight == m_iScaledRefFrmHeight,  false );
      ROFRS( m_iRefLayerFrmWidth  == m_iScaledRefFrmWidth  || 2 * m_iRefLayerFrmWidth  == m_iScaledRefFrmWidth,   false );
      return true;
    }

public:
  //===== parameters of current layer =====
  Int   m_iExtendedSpatialScalability;
  Int   m_iLevelIdc;
  Bool  m_bFrameMbsOnlyFlag;
  Bool  m_bFieldPicFlag;
  Bool  m_bBotFieldFlag;
  Bool  m_bIsMbAffFrame;
  Int   m_iFrameWidth;
  Int   m_iFrameHeight;
  Int   m_iWidthInSamples;
  Int   m_iHeightInSamples;
  Int   m_iChromaPhaseX;
  Int   m_iChromaPhaseY;
  Int   m_iScaledRefFrmWidth;     // also in PictureParameters
  Int   m_iScaledRefFrmHeight;    // also in PictureParameters
  Int   m_iLeftFrmOffset;         // also in PictureParameters
  Int   m_iTopFrmOffset;          // also in PictureParameters
  Int   m_iRefLayerChromaPhaseX;  // also in PictureParameters
  Int   m_iRefLayerChromaPhaseY;  // also in PictureParameters

  //===== parameters for base layer =====
  Bool  m_bRefLayerFrameMbsOnlyFlag;
  Bool  m_bRefLayerFieldPicFlag;
  Bool  m_bRefLayerBotFieldFlag;
  Bool  m_bRefLayerIsMbAffFrame;
  Int   m_iRefLayerFrmWidth;
  Int   m_iRefLayerFrmHeight;
  Int   m_iRefLayerWidthInSamples;
  Int   m_iRefLayerHeightInSamples;
};


#ifdef DOWN_CONVERT_STATIC
#else
H264AVC_NAMESPACE_END
#endif

#endif 
