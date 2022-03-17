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

#include "DownConvert.h"

#include <cmath>
#include <cstring>


#define  TMM_TABLE_SIZE          512


//=================================================
//
//   G E N E R A L   M A I N   F U N C T I O N S
//
//=================================================

DownConvert::DownConvert()
: m_iImageStride            ( 0 )
, m_paiImageBuffer          ( 0 )
, m_paiTmp1dBuffer          ( 0 )
{
}
  
DownConvert::~DownConvert()
{
  xDestroy();
}

bool
DownConvert::init( int iMaxWidth, int iMaxHeight, int iMaxMargin )
{
  xDestroy();

  iMaxWidth                += 2 * iMaxMargin;
  iMaxHeight               += 2 * iMaxMargin;
  int iPicSize              =   iMaxWidth * iMaxHeight;
  int iMaxDim               = ( iMaxWidth > iMaxHeight ? iMaxWidth : iMaxHeight );
  m_iImageStride            = iMaxWidth;
  m_paiImageBuffer          = new int [ iPicSize ];
  m_paiTmp1dBuffer          = new int [ iMaxDim ];

  ROFRS( m_paiImageBuffer,          true );
  ROFRS( m_paiTmp1dBuffer,          true );

  return false;
}

void
DownConvert::destroy()
{
  delete this;
}

//===========================================================================
//
//   M A I N   F U N C T I O N S   F O R   D O W N C O N V E R T   T O O L
//
//===========================================================================


void
DownConvert::downsamplingSVC( unsigned char*    pucBufferY,   int   iStrideY,
                              unsigned char*    pucBufferU,   int   iStrideU,
                              unsigned char*    pucBufferV,   int   iStrideV,
                              ResizeParameters* pcParameters, bool  bBotCoincided )
{
  int   iBaseW                  =   pcParameters->m_iFrameWidth;
  int   iBaseH                  =   pcParameters->m_iFrameHeight;
  int   iCurrW                  =   pcParameters->m_iRefLayerFrmWidth;
  int   iCurrH                  =   pcParameters->m_iRefLayerFrmHeight;
  bool  bTopAndBottomResampling = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag == false  &&
                                    pcParameters->m_bRefLayerFieldPicFlag     == false  &&
                                    pcParameters->m_bFrameMbsOnlyFlag         == false  &&
                                    pcParameters->m_bFieldPicFlag             == false    );
  bool  bVerticalDownsampling   = ( pcParameters->m_bFrameMbsOnlyFlag         == true   &&
                                    pcParameters->m_bRefLayerFieldPicFlag     == true     );
  bool  bCurrBotField           = ( pcParameters->m_bFieldPicFlag             == true   &&
                                    pcParameters->m_bBotFieldFlag             == true     );
  bool  bBotFieldFlag           = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ?  false
                                  : pcParameters->m_bFieldPicFlag             ?  pcParameters->m_bBotFieldFlag
                                  : pcParameters->m_bRefLayerFieldPicFlag     ?  pcParameters->m_bRefLayerBotFieldFlag
                                  : false );
  int   iBaseField              = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ?  0 : 1 );
  int   iCurrField              = ( pcParameters->m_bFieldPicFlag             ?  1 : 0 );
  int   iBaseBot                = ( bBotFieldFlag ? 1 : 0 );
  int   iCurrBot                = ( bCurrBotField ? 1 : 0 );

  //==== check bot field coincided parameter for interlaced to progressive resampling =====
  if( pcParameters->m_bRefLayerFrameMbsOnlyFlag && ! pcParameters->m_bFrameMbsOnlyFlag )
  {
    bBotFieldFlag = bBotCoincided;
  }

  //=======================
  //=====   L U M A   =====
  //=======================
  assert( bTopAndBottomResampling == false );
  if( !bTopAndBottomResampling )
  {
    unsigned char* pSrc = pucBufferY + iStrideY * iCurrBot;
    unsigned char* pDes = pucBufferY + iStrideY * iBaseBot;
    xCopyToImageBuffer  ( pSrc,         iCurrW, iCurrH >> iCurrField, iStrideY << iCurrField );
    xCompDownsampling   ( pcParameters, false,  bBotFieldFlag,        bVerticalDownsampling  );
    xCopyFromImageBuffer( pDes,         iBaseW, iBaseH >> iBaseField, iStrideY << iBaseField );
  }

  iBaseW >>= 1;
  iBaseH >>= 1;
  iCurrW >>= 1;
  iCurrH >>= 1;

  //===========================
  //=====   C H R O M A   =====
  //===========================
  if( !bTopAndBottomResampling )
  {
    //===== U =====
    unsigned char* pSrc = pucBufferU + iStrideU * iCurrBot;
    unsigned char* pDes = pucBufferU + iStrideU * iBaseBot;
    xCopyToImageBuffer  ( pSrc,         iCurrW, iCurrH >> iCurrField, iStrideU << iCurrField );
    xCompDownsampling   ( pcParameters, true,   bBotFieldFlag,        bVerticalDownsampling  );
    xCopyFromImageBuffer( pDes,         iBaseW, iBaseH >> iBaseField, iStrideU << iBaseField );

    //===== V =====
    pSrc = pucBufferV + iStrideV * iCurrBot;
    pDes = pucBufferV + iStrideV * iBaseBot;
    xCopyToImageBuffer  ( pSrc,         iCurrW, iCurrH >> iCurrField, iStrideV << iCurrField );
    xCompDownsampling   ( pcParameters, true,   bBotFieldFlag,        bVerticalDownsampling  );
    xCopyFromImageBuffer( pDes,         iBaseW, iBaseH >> iBaseField, iStrideV << iBaseField );
  }
}


//======================================================
//
//   G E N E R A L   H E L P E R    F U N C T I O N S
//
//======================================================

void
DownConvert::xDestroy()
{
  delete [] m_paiImageBuffer;
  delete [] m_paiTmp1dBuffer;
  m_paiImageBuffer          = 0;
  m_paiTmp1dBuffer          = 0;
}

int
DownConvert::xClip( int iValue, int imin, int imax )
{
  ROTRS( iValue < imin, imin );
  ROTRS( iValue > imax, imax );
  return iValue;
}


//===============================================================================
//
//   H E L P E R   F U N C T I O N S   F O R   D O W N C O N V E R T   T O O L
//
//===============================================================================

void
DownConvert::xCopyToImageBuffer( unsigned char* pucSrc, int iWidth, int iHeight, int iStride )
{
  int* piDes = m_paiImageBuffer;
  for( int j = 0; j < iHeight; j++ )
  {
    for( int i = 0; i < iWidth;  i++ )
    {
      piDes[i] = (int)pucSrc[i];
    }
    piDes   += m_iImageStride;
    pucSrc  += iStride;
  }
}

void
DownConvert::xCopyFromImageBuffer( unsigned char* pucDes, int iWidth, int iHeight, int iStride )
{
  int* piSrc = m_paiImageBuffer;
  for( int j = 0; j < iHeight; j++ )
  {
    for( int i = 0; i < iWidth;  i++ )
    {
      pucDes[i] = (unsigned char)piSrc[i];
    }
    pucDes  += iStride;
    piSrc   += m_iImageStride;
  }
}

void
DownConvert::xCompDownsampling( ResizeParameters* pcParameters, bool bChroma, bool bBotFlag, bool bVerticalDownsampling )
{
  //===== set general parameters =====
  int   iBotField   = ( bBotFlag ? 1 : 0 );
  int   iFactor     = ( !bChroma ? 1 : 2 );
  int   iRefPhaseX  = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseX );
  int   iRefPhaseY  = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseY );
  int   iPhaseX     = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseX );
  int   iPhaseY     = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseY );
  int   iRefW       = pcParameters->m_iFrameWidth         / iFactor;  // reference layer frame width
  int   iRefH       = pcParameters->m_iFrameHeight        / iFactor;  // reference layer frame height
  int   iOutW       = pcParameters->m_iScaledRefFrmWidth  / iFactor;  // scaled reference layer frame width
  int   iOutH       = pcParameters->m_iScaledRefFrmHeight / iFactor;  // scaled reference layer frame height
  int   iGlobalW    = pcParameters->m_iRefLayerFrmWidth   / iFactor;  // current frame width
  int   iGlobalH    = pcParameters->m_iRefLayerFrmHeight  / iFactor;  // current frame height
  int   iLeftOffset = pcParameters->m_iLeftFrmOffset      / iFactor;  // current left frame offset
  int   iTopOffset  = pcParameters->m_iTopFrmOffset       / iFactor;  // current  top frame offset

  //===== set input/output size =====
  int   iBaseField  = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ? 0 : 1 );
  int   iCurrField  = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag && pcParameters->m_bFrameMbsOnlyFlag ? 0 : 1 );
  int   iBaseW      = iRefW;
  int   iBaseH      = iRefH       >> iBaseField;
  int   iCurrW      = iGlobalW;
  int   iCurrH      = iGlobalH    >> iCurrField;
  int   iLOffset    = iLeftOffset;
  int   iTOffset    = iTopOffset  >> iCurrField;
  int   iROffset    = iCurrW - iLOffset -   iOutW;
  int   iBOffset    = iCurrH - iTOffset - ( iOutH >> iCurrField );

  //===== set position calculation parameters =====
  int   iScaledW    = iOutW;
  int   iScaledH    = ( ! pcParameters->m_bRefLayerFrameMbsOnlyFlag || pcParameters->m_bFrameMbsOnlyFlag ? iOutH : iOutH / 2 );
  int   iShiftX     = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iScaledW ) );
  int   iShiftY     = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iScaledH ) );
  int   iScaleX     = ( ( (unsigned int)iScaledW << iShiftX ) + ( iRefW >> 1 ) ) / iRefW;
  int   iScaleY     = ( ( (unsigned int)iScaledH << iShiftY ) + ( iRefH >> 1 ) ) / iRefH;
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    if( pcParameters->m_bRefLayerFrameMbsOnlyFlag )
    {
      iPhaseY       = iPhaseY + 4 * iBotField + ( 3 - iFactor );
#if ZERO_PHASE
      iRefPhaseY    = 2 * iRefPhaseY + 0;
#else
      iRefPhaseY    = 2 * iRefPhaseY + 2;
#endif
    }
    else
    {
      iPhaseY       = iPhaseY    + 4 * iBotField;
      iRefPhaseY    = iRefPhaseY + 4 * iBotField;
    }
  }
#if ZERO_PHASE
  int   iAddX       = ( ( ( iScaledW * ( 0 + iRefPhaseX ) ) << ( iShiftX - 2 ) ) + ( iRefW >> 1 ) ) / iRefW + ( 1 << ( iShiftX - 5 ) );
  int   iAddY       = ( ( ( iScaledH * ( 0 + iRefPhaseY ) ) << ( iShiftY - 2 ) ) + ( iRefH >> 1 ) ) / iRefH + ( 1 << ( iShiftY - 5 ) );
  int   iDeltaX     = 4 * ( 0 + iPhaseX ) - ( iLeftOffset << 4 );
  int   iDeltaY     = 4 * ( 0 + iPhaseY ) - ( iTopOffset  << 4 );
#else
  int   iAddX       = ( ( ( iScaledW * ( 2 + iRefPhaseX ) ) << ( iShiftX - 2 ) ) + ( iRefW >> 1 ) ) / iRefW + ( 1 << ( iShiftX - 5 ) );
  int   iAddY       = ( ( ( iScaledH * ( 2 + iRefPhaseY ) ) << ( iShiftY - 2 ) ) + ( iRefH >> 1 ) ) / iRefH + ( 1 << ( iShiftY - 5 ) );
  int   iDeltaX     = 4 * ( 2 + iPhaseX ) - ( iLeftOffset << 4 );
  int   iDeltaY     = 4 * ( 2 + iPhaseY ) - ( iTopOffset  << 4 );
#endif
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
#if ZERO_PHASE
    iAddY           = ( ( ( iScaledH * ( 0 + iRefPhaseY ) ) << ( iShiftY - 3 ) ) + ( iRefH >> 1 ) ) / iRefH + ( 1 << ( iShiftY - 5 ) );
    iDeltaY         = 2 * ( 0 + iPhaseY ) - ( iTopOffset  << 3 );
#else
    iAddY           = ( ( ( iScaledH * ( 2 + iRefPhaseY ) ) << ( iShiftY - 3 ) ) + ( iRefH >> 1 ) ) / iRefH + ( 1 << ( iShiftY - 5 ) );
    iDeltaY         = 2 * ( 2 + iPhaseY ) - ( iTopOffset  << 3 );
#endif
  }

  //===== vertical downsampling to generate a field signal from a progressive frame =====
  if( bVerticalDownsampling )
  {
    xVertDownsampling( iCurrW, iCurrH, bBotFlag );
  }

  //===== basic downsampling of a frame or field =====
  xBasicDownsampling( iBaseW,   iBaseH,   iCurrW,   iCurrH,
                      iLOffset, iTOffset, iROffset, iBOffset,
                      iShiftX,  iShiftY,  iScaleX,  iScaleY,
                      iAddX,    iAddY,    iDeltaX,  iDeltaY );
}

void
DownConvert::xVertDownsampling( int   iBaseW,
                                int   iBaseH,
                                bool  bBotFlag )
{
  int aiVertFilter[13]  = { 2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2 };
  int iBotField         = ( bBotFlag ? 1 : 0 );
  int iCurrW            = iBaseW;
  int iCurrH            = iBaseH << 1;

  //===== vertical downsampling =====
  for( int j = 0; j < iCurrW; j++ )
  {
    int* piSrc = &m_paiImageBuffer[j];
    for( int i = 0; i < iBaseH; i++ )
    {
      m_paiTmp1dBuffer[i] = 0;
      for( int k = 0; k < 13; k++ )
      {
        int m = xClip( 2 * i + iBotField + k - 6, 0, iCurrH - 1 );
        m_paiTmp1dBuffer[i] += aiVertFilter[k] * piSrc[m*m_iImageStride];
      }
      m_paiTmp1dBuffer[i] = ( m_paiTmp1dBuffer[i] + 32 ) >> 6;
    }
    //--- clip and copy back to image buffer ---
    for( int n = 0; n < iBaseH; n++ )
    {
      piSrc[n*m_iImageStride] = xClip( m_paiTmp1dBuffer[n], 0, 255 );
    }
  }
}

void
DownConvert::xBasicDownsampling( int iBaseW,   int iBaseH,   int iCurrW,   int iCurrH,
                                 int iLOffset, int iTOffset, int iROffset, int iBOffset,
                                 int iShiftX,  int iShiftY,  int iScaleX,  int iScaleY,
                                 int iAddX,    int iAddY,    int iDeltaX,  int iDeltaY )
{
  const int filter16[8][16][12] =
  {
    { // D = 1
      {   0,   0,   0,   0,   0, 128,   0,   0,   0,   0,   0,   0 },
      {   0,   0,   0,   2,  -6, 127,   7,  -2,   0,   0,   0,   0 },
      {   0,   0,   0,   3, -12, 125,  16,  -5,   1,   0,   0,   0 },
      {   0,   0,   0,   4, -16, 120,  26,  -7,   1,   0,   0,   0 },
      {   0,   0,   0,   5, -18, 114,  36, -10,   1,   0,   0,   0 },
      {   0,   0,   0,   5, -20, 107,  46, -12,   2,   0,   0,   0 },
      {   0,   0,   0,   5, -21,  99,  57, -15,   3,   0,   0,   0 },
      {   0,   0,   0,   5, -20,  89,  68, -18,   4,   0,   0,   0 },
      {   0,   0,   0,   4, -19,  79,  79, -19,   4,   0,   0,   0 },
      {   0,   0,   0,   4, -18,  68,  89, -20,   5,   0,   0,   0 },
      {   0,   0,   0,   3, -15,  57,  99, -21,   5,   0,   0,   0 },
      {   0,   0,   0,   2, -12,  46, 107, -20,   5,   0,   0,   0 },
      {   0,   0,   0,   1, -10,  36, 114, -18,   5,   0,   0,   0 },
      {   0,   0,   0,   1,  -7,  26, 120, -16,   4,   0,   0,   0 },
      {   0,   0,   0,   1,  -5,  16, 125, -12,   3,   0,   0,   0 },
      {   0,   0,   0,   0,  -2,   7, 127,  -6,   2,   0,   0,   0 }
    },
    { // D = 1.5
      {   0,   2,   0, -14,  33,  86,  33, -14,   0,   2,   0,   0 },
      {   0,   1,   1, -14,  29,  85,  38, -13,  -1,   2,   0,   0 },
      {   0,   1,   2, -14,  24,  84,  43, -12,  -2,   2,   0,   0 },
      {   0,   1,   2, -13,  19,  83,  48, -11,  -3,   2,   0,   0 },
      {   0,   0,   3, -13,  15,  81,  53, -10,  -4,   3,   0,   0 },
      {   0,   0,   3, -12,  11,  79,  57,  -8,  -5,   3,   0,   0 },
      {   0,   0,   3, -11,   7,  76,  62,  -5,  -7,   3,   0,   0 },
      {   0,   0,   3, -10,   3,  73,  65,  -2,  -7,   3,   0,   0 },
      {   0,   0,   3,  -9,   0,  70,  70,   0,  -9,   3,   0,   0 },
      {   0,   0,   3,  -7,  -2,  65,  73,   3, -10,   3,   0,   0 },
      {   0,   0,   3,  -7,  -5,  62,  76,   7, -11,   3,   0,   0 },
      {   0,   0,   3,  -5,  -8,  57,  79,  11, -12,   3,   0,   0 },
      {   0,   0,   3,  -4, -10,  53,  81,  15, -13,   3,   0,   0 },
      {   0,   0,   2,  -3, -11,  48,  83,  19, -13,   2,   1,   0 },
      {   0,   0,   2,  -2, -12,  43,  84,  24, -14,   2,   1,   0 },
      {   0,   0,   2,  -1, -13,  38,  85,  29, -14,   1,   1,   0 }
    },
    { // D = 2
      {   0,   5,   -6,  -10,  37,  76,   37,  -10,  -6,    5,  0,   0}, //0
      {   0,   5,   -4,  -11,  33,  76,   40,  -9,    -7,    5,  0,   0}, //1
      //{   0,   5,   -3,  -12,  28,  75,   44,  -7,    -8,    5,  1,   0}, //2
      {  -1,   5,   -3,  -12,  29,  75,   45,  -7,    -8,   5,  0,   0}, //2 new coefficients in m24499
      {  -1,   4,   -2,  -13,  25,  75,   48,  -5,    -9,    5,  1,   0}, //3
      {  -1,   4,   -1,  -13,  22,  73,   52,  -3,    -10,  4,  1,   0}, //4
      {  -1,   4,   0,    -13,  18,  72,   55,  -1,    -11,  4,  2,  -1}, //5
      {  -1,   4,   1,    -13,  14,  70,   59,  2,    -12,  3,  2,  -1}, //6
      {  -1,   3,   1,    -13,  11,  68,   62,  5,    -12,  3,  2,  -1}, //7
      {  -1,   3,   2,    -13,  8,  65,   65,  8,    -13,  2,  3,  -1}, //8
      {  -1,   2,   3,    -12,  5,  62,   68,  11,    -13,  1,  3,  -1}, //9
      {  -1,   2,   3,    -12,  2,  59,   70,  14,    -13,  1,  4,  -1}, //10
      {  -1,   2,   4,    -11,  -1,  55,   72,  18,    -13,  0,  4,  -1}, //11
      {   0,   1,   4,    -10,  -3,  52,   73,  22,    -13,  -1,  4,  -1}, //12
      {   0,   1,   5,    -9,    -5,  48,   75,  25,    -13,  -2,  4,  -1}, //13
      //{   0,   1,   5,    -8,    -7,  44,   75,  28,    -12,  -3,  5,   0}, //14
      {    0,   0,   5,    -8,   -7,  45,   75,  29,    -12,  -3,  5,  -1}  , //14 new coefficients in m24499  
      {   0,   0,   5,    -7,    -9,  40,   76,  33,    -11,  -4,  5,   0}, //15
    },
    { // D = 2.5
      {   2,  -3,   -9,  6,   39,  58,   39,  6,   -9,  -3,    2,    0}, // 0
      {   2,  -3,   -9,  4,   38,  58,   43,  7,   -9,  -4,    1,    0}, // 1
      {   2,  -2,   -9,  2,   35,  58,   44,  9,   -8,  -4,    1,    0}, // 2
      {   1,  -2,   -9,  1,   34,  58,   46,  11,   -8,  -5,    1,    0}, // 3
      //{   1,  -1,   -8,  -1,   31,  57,   48,  13,   -8,  -5,    1,    0}, // 4
      {   1,  -1,   -8,  -1,   31,  57,   47,  13,   -7,  -5,    1,    0},  // 4 new coefficients in m24499  
      {   1,  -1,   -8,  -2,   29,  56,   49,  15,   -7,  -6,    1,    1}, // 5
      {   1,  0,   -8,  -3,   26,  55,   51,  17,   -7,  -6,    1,    1}, // 6
      {   1,  0,   -7,  -4,   24,  54,   52,  19,   -6,  -7,    1,    1}, // 7
      {   1,  0,   -7,  -5,   22,  53,   53,  22,   -5,  -7,    0,    1}, // 8
      {   1,  1,   -7,  -6,   19,  52,   54,  24,   -4,  -7,    0,    1}, // 9
      {   1,  1,   -6,  -7,   17,  51,   55,  26,   -3,  -8,    0,    1}, // 10
      {   1,  1,   -6,  -7,   15,  49,   56,  29,   -2,  -8,    -1,    1}, // 11
      //{   0,  1,   -5,  -8,   13,  48,   57,  31,   -1,  -8,    -1,    1}, // 12 new coefficients in m24499
      {   0,  1,   -5,  -7,   13,  47,  57,  31,  -1,    -8,   -1,    1}, // 12   
      {   0,  1,   -5,  -8,   11,  46,   58,  34,   1,    -9,    -2,    1}, // 13
      {   0,  1,   -4,  -8,   9,    44,   58,  35,   2,    -9,    -2,    2}, // 14
      {   0,  1,   -4,  -9,   7,    43,   58,  38,   4,    -9,    -3,    2}, // 15
    },
    { // D = 3
      {  -2,  -7,   0,  17,  35,  43,  35,  17,   0,  -7,  -5,   2 },
      {  -2,  -7,  -1,  16,  34,  43,  36,  18,   1,  -7,  -5,   2 },
      {  -1,  -7,  -1,  14,  33,  43,  36,  19,   1,  -6,  -5,   2 },
      {  -1,  -7,  -2,  13,  32,  42,  37,  20,   3,  -6,  -5,   2 },
      {   0,  -7,  -3,  12,  31,  42,  38,  21,   3,  -6,  -5,   2 },
      {   0,  -7,  -3,  11,  30,  42,  39,  23,   4,  -6,  -6,   1 },
      {   0,  -7,  -4,  10,  29,  42,  40,  24,   5,  -6,  -6,   1 },
      {   1,  -7,  -4,   9,  27,  41,  40,  25,   6,  -5,  -6,   1 },
      {   1,  -6,  -5,   7,  26,  41,  41,  26,   7,  -5,  -6,   1 },
      {   1,  -6,  -5,   6,  25,  40,  41,  27,   9,  -4,  -7,   1 },
      {   1,  -6,  -6,   5,  24,  40,  42,  29,  10,  -4,  -7,   0 },
      {   1,  -6,  -6,   4,  23,  39,  42,  30,  11,  -3,  -7,   0 },
      {   2,  -5,  -6,   3,  21,  38,  42,  31,  12,  -3,  -7,   0 },
      {   2,  -5,  -6,   3,  20,  37,  42,  32,  13,  -2,  -7,  -1 },
      {   2,  -5,  -6,   1,  19,  36,  43,  33,  14,  -1,  -7,  -1 },
      {   2,  -5,  -7,   1,  18,  36,  43,  34,  16,  -1,  -7,  -2 }
    },
    { // D = 3.5
      {  -6,  -3,   5,  19,  31,  36,  31,  19,   5,  -3,  -6,   0 },
      {  -6,  -4,   4,  18,  31,  37,  32,  20,   6,  -3,  -6,  -1 },
      {  -6,  -4,   4,  17,  30,  36,  33,  21,   7,  -3,  -6,  -1 },
      {  -5,  -5,   3,  16,  30,  36,  33,  22,   8,  -2,  -6,  -2 },
      {  -5,  -5,   2,  15,  29,  36,  34,  23,   9,  -2,  -6,  -2 },
      {  -5,  -5,   2,  15,  28,  36,  34,  24,  10,  -2,  -6,  -3 },
      {  -4,  -5,   1,  14,  27,  36,  35,  24,  10,  -1,  -6,  -3 },
      {  -4,  -5,   0,  13,  26,  35,  35,  25,  11,   0,  -5,  -3 },
      {  -4,  -6,   0,  12,  26,  36,  36,  26,  12,   0,  -6,  -4 },
      {  -3,  -5,   0,  11,  25,  35,  35,  26,  13,   0,  -5,  -4 },
      {  -3,  -6,  -1,  10,  24,  35,  36,  27,  14,   1,  -5,  -4 },
      {  -3,  -6,  -2,  10,  24,  34,  36,  28,  15,   2,  -5,  -5 },
      {  -2,  -6,  -2,   9,  23,  34,  36,  29,  15,   2,  -5,  -5 },
      {  -2,  -6,  -2,   8,  22,  33,  36,  30,  16,   3,  -5,  -5 },
      {  -1,  -6,  -3,   7,  21,  33,  36,  30,  17,   4,  -4,  -6 },
      {  -1,  -6,  -3,   6,  20,  32,  37,  31,  18,   4,  -4,  -6 }
    },
    { // D = 4
      {  -9,   0,   9,  20,  28,  32,  28,  20,   9,   0,  -9,   0 },
      {  -9,   0,   8,  19,  28,  32,  29,  20,  10,   0,  -4,  -5 },
      {  -9,  -1,   8,  18,  28,  32,  29,  21,  10,   1,  -4,  -5 },
      {  -9,  -1,   7,  18,  27,  32,  30,  22,  11,   1,  -4,  -6 },
      {  -8,  -2,   6,  17,  27,  32,  30,  22,  12,   2,  -4,  -6 },
      {  -8,  -2,   6,  16,  26,  32,  31,  23,  12,   2,  -4,  -6 },
      {  -8,  -2,   5,  16,  26,  31,  31,  23,  13,   3,  -3,  -7 },
      {  -8,  -3,   5,  15,  25,  31,  31,  24,  14,   4,  -3,  -7 },
      {  -7,  -3,   4,  14,  25,  31,  31,  25,  14,   4,  -3,  -7 },
      {  -7,  -3,   4,  14,  24,  31,  31,  25,  15,   5,  -3,  -8 },
      {  -7,  -3,   3,  13,  23,  31,  31,  26,  16,   5,  -2,  -8 },
      {  -6,  -4,   2,  12,  23,  31,  32,  26,  16,   6,  -2,  -8 },
      {  -6,  -4,   2,  12,  22,  30,  32,  27,  17,   6,  -2,  -8 },
      {  -6,  -4,   1,  11,  22,  30,  32,  27,  18,   7,  -1,  -9 },
      {  -5,  -4,   1,  10,  21,  29,  32,  28,  18,   8,  -1,  -9 },
      {  -5,  -4,   0,  10,  20,  29,  32,  28,  19,   8,   0,  -9 }
    },
    { // D = 5.5
      {  -8,   7,  13,  18,  22,  24,  22,  18,  13,   7,   2, -10 },
      {  -8,   7,  13,  18,  22,  23,  22,  19,  13,   7,   2, -10 },
      {  -8,   6,  12,  18,  22,  23,  22,  19,  14,   8,   2, -10 },
      {  -9,   6,  12,  17,  22,  23,  23,  19,  14,   8,   3, -10 },
      {  -9,   6,  12,  17,  21,  23,  23,  19,  14,   9,   3, -10 },
      {  -9,   5,  11,  17,  21,  23,  23,  20,  15,   9,   3, -10 },
      {  -9,   5,  11,  16,  21,  23,  23,  20,  15,   9,   4, -10 },
      {  -9,   5,  10,  16,  21,  23,  23,  20,  15,  10,   4, -10 },
      { -10,   5,  10,  16,  20,  23,  23,  20,  16,  10,   5, -10 },
      { -10,   4,  10,  15,  20,  23,  23,  21,  16,  10,   5,  -9 },
      { -10,   4,   9,  15,  20,  23,  23,  21,  16,  11,   5,  -9 },
      { -10,   3,   9,  15,  20,  23,  23,  21,  17,  11,   5,  -9 },
      { -10,   3,   9,  14,  19,  23,  23,  21,  17,  12,   6,  -9 },
      { -10,   3,   8,  14,  19,  23,  23,  22,  17,  12,   6,  -9 },
      { -10,   2,   8,  14,  19,  22,  23,  22,  18,  12,   6,  -8 },
      { -10,   2,   7,  13,  19,  22,  23,  22,  18,  13,   7,  -8 }
    }
  };

  //===== determine filter sets =====
  int iCropW      = iCurrW - iLOffset - iROffset;
  int iCropH      = iCurrH - iTOffset - iBOffset;
  int iVerFilter  = 0;
  int iHorFilter  = 0;
  if      (  4 * iCropH > 15 * iBaseH )   iVerFilter  = 7;
  else if (  7 * iCropH > 20 * iBaseH )   iVerFilter  = 6;
  else if (  2 * iCropH >  5 * iBaseH )   iVerFilter  = 5;
  else if (  1 * iCropH >  2 * iBaseH )   iVerFilter  = 4;
  else if (  3 * iCropH >  5 * iBaseH )   iVerFilter  = 3;
  else if (  4 * iCropH >  5 * iBaseH )   iVerFilter  = 2;
  else if ( 19 * iCropH > 20 * iBaseH )   iVerFilter  = 1;
  if      (  4 * iCropW > 15 * iBaseW )   iHorFilter  = 7;
  else if (  7 * iCropW > 20 * iBaseW )   iHorFilter  = 6;
  else if (  2 * iCropW >  5 * iBaseW )   iHorFilter  = 5;
  else if (  1 * iCropW >  2 * iBaseW )   iHorFilter  = 4;
  else if (  3 * iCropW >  5 * iBaseW )   iHorFilter  = 3;
  else if (  4 * iCropW >  5 * iBaseW )   iHorFilter  = 2;
  else if ( 19 * iCropW > 20 * iBaseW )   iHorFilter  = 1;

  int iShiftXM4 = iShiftX - 4;
  int iShiftYM4 = iShiftY - 4;

  //===== horizontal downsampling =====
  {
    for( int j = 0; j < iCurrH; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j*m_iImageStride];
      for( int i = 0; i < iBaseW; i++ )
      {
        int iRefPos16 = (int)( (unsigned int)( i * iScaleX + iAddX ) >> iShiftXM4 ) - iDeltaX;
        int iPhase    = iRefPos16  & 15;
        int iRefPos   = iRefPos16 >>  4;

        m_paiTmp1dBuffer[i] = 0;
        for( int k = 0; k < 12; k++ )
        {
          int m = xClip( iRefPos + k - 5, 0, iCurrW - 1 );
          m_paiTmp1dBuffer[i] += filter16[iHorFilter][iPhase][k] * piSrc[m];
        }
      }
      //--- copy row back to image buffer ---
      memcpy( piSrc, m_paiTmp1dBuffer, iBaseW*sizeof(int) );
    }
  }

  //===== vertical downsampling =====
  {
    for( int i = 0; i < iBaseW; i++ )
    {
      int* piSrc = &m_paiImageBuffer[i];
      for( int j = 0; j < iBaseH; j++ )
      {
        int iRefPos16 = (int)( (unsigned int)( j * iScaleY + iAddY ) >> iShiftYM4 ) - iDeltaY;
        int iPhase    = iRefPos16  & 15;
        int iRefPos   = iRefPos16 >>  4;

        m_paiTmp1dBuffer[j] = 0;
        for( int k = 0; k < 12; k++ )
        {
          int m = xClip( iRefPos + k - 5, 0, iCurrH - 1 );
          m_paiTmp1dBuffer[j] += filter16[iVerFilter][iPhase][k] * piSrc[m*m_iImageStride];
        }
        m_paiTmp1dBuffer[j] = ( m_paiTmp1dBuffer[j] + 8192 ) >> 14;
      }
      //--- clip and copy back to image buffer ---
      for( int n = 0; n < iBaseH; n++ )
      {
        piSrc[n*m_iImageStride] = xClip( m_paiTmp1dBuffer[n], 0, 255 );
      }
    }
  }
}




