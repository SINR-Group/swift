/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2014, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DOWN_CONVERT_
#define _DOWN_CONVERT_

#define ZERO_PHASE        1

#define ESS_NONE      0

#include <cassert>
#include "../../Lib/TLibCommon/TypeDef.h"

#ifndef  gMax
#define  gMax(x,y)   ((x)>(y)?(x):(y))
#define  gMin(x,y)   ((x)<(y)?(x):(y))

#define ROFRS( exp, retVal )  \
{                             \
  if( !( exp ) )              \
  {                           \
  return retVal;            \
  }                           \
}

#define ROTRS( exp, retVal )  \
{                             \
  if( ( exp ) )               \
  {                           \
  return retVal;            \
  }                           \
}

#endif

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


class DownConvert
{
public:
  //========================
  // general main functions
  //========================
  DownConvert   ();
  ~DownConvert  ();
  bool  init    ( int iMaxWidth, int iMaxHeight, int iMaxMargin = 0 );
  void  destroy ();

  //=====================================
  // main functions for DownConvert Tool
  //=====================================
  void  downsamplingSVC             ( unsigned char*          pucBufferY,    int   iStrideY,
                                      unsigned char*          pucBufferU,    int   iStrideU,
                                      unsigned char*          pucBufferV,    int   iStrideV,
                                      ResizeParameters*       pcParameters,  bool  bBotCoincided = false );

  //==========================
  // general helper functions
  //==========================
  //--- delete buffers ---
  void  xDestroy                    ();
  //--- general clipping ---
  int   xClip                       ( int                     iValue,
                                      int                     imin,
                                      int                     imax );
  //=======================================
  // helper functions for DownConvert Tool
  //=======================================
  //--- place to and get from image buffer ---
  void  xCopyToImageBuffer          ( unsigned char*        pucSrc,
                                      int                   iWidth,
                                      int                   iHeight,
                                      int                   iStride );
  void  xCopyFromImageBuffer        ( unsigned char*        pucDes,
                                      int                   iWidth,
                                      int                   iHeight,
                                      int                   iStride );
  //--- SVC non-normative downsampling ---
  void  xCompDownsampling           ( ResizeParameters*     pcParameters,
                                      bool                  bChroma,
                                      bool                  bBotFlag,
                                      bool                  bVerticalDownsampling );
  void  xVertDownsampling           ( int                   iBaseW,
                                      int                   iBaseH,
                                      bool                  bBotFlag );
  void  xBasicDownsampling          ( int  iBaseW,   int  iBaseH,   int  iCurrW,   int  iCurrH,
                                      int  iLOffset, int  iTOffset, int  iROffset, int  iBOffset,
                                      int  iShiftX,  int  iShiftY,  int  iScaleX,  int  iScaleY,
                                      int  iAddX,    int  iAddY,    int  iDeltaX,  int  iDeltaY );
private:
  //===== member variables =====
  int         m_iImageStride;
  int*        m_paiImageBuffer;
  int*        m_paiTmp1dBuffer;
};


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


#endif // _DOWN_CONVERT_


