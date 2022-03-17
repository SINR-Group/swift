#include "TypeDef.h"
#if SVC_EXTENSION
#include "TComUpsampleFilter.h"

const Int TComUpsampleFilter::m_lumaFixedFilter[16][NTAPS_US_LUMA] =
{
  {  0, 0,   0, 64,  0,   0,  0,  0 },
  {  0, 1,  -3, 63,  4,  -2,  1,  0 },
  { -1, 2,  -5, 62,  8,  -3,  1,  0 },
  { -1, 3,  -8, 60, 13,  -4,  1,  0 },
  { -1, 4, -10, 58, 17,  -5,  1,  0 },
  { -1, 4, -11, 52, 26,  -8,  3, -1 }, // <-> actual phase shift 1/3, used for spatial scalability x1.5
  { -1, 3,  -9, 47, 31, -10,  4, -1 },
  { -1, 4, -11, 45, 34, -10,  4, -1 },
  { -1, 4, -11, 40, 40, -11,  4, -1 }, // <-> actual phase shift 1/2, equal to HEVC MC, used for spatial scalability x2
  { -1, 4, -10, 34, 45, -11,  4, -1 },
  { -1, 4, -10, 31, 47,  -9,  3, -1 },
  { -1, 3,  -8, 26, 52, -11,  4, -1 }, // <-> actual phase shift 2/3, used for spatial scalability x1.5
  {  0, 1,  -5, 17, 58, -10,  4, -1 },
  {  0, 1,  -4, 13, 60,  -8,  3, -1 },
  {  0, 1,  -3,  8, 62,  -5,  2, -1 },
  {  0, 1,  -2,  4, 63,  -3,  1,  0 }
};

const Int TComUpsampleFilter::m_chromaFixedFilter[16][NTAPS_US_CHROMA] =
{
  {  0, 64,  0,  0 },
  { -2, 62,  4,  0 },
  { -2, 58, 10, -2 },
  { -4, 56, 14, -2 },
  { -4, 54, 16, -2 }, // <-> actual phase shift 1/4,equal to HEVC MC, used for spatial scalability x1.5 (only for accurate Chroma alignement)
  { -6, 52, 20, -2 }, // <-> actual phase shift 1/3, used for spatial scalability x1.5
  { -6, 46, 28, -4 }, // <-> actual phase shift 3/8,equal to HEVC MC, used for spatial scalability x2 (only for accurate Chroma alignement)
  { -4, 42, 30, -4 },
  { -4, 36, 36, -4 }, // <-> actual phase shift 1/2,equal to HEVC MC, used for spatial scalability x2
  { -4, 30, 42, -4 }, // <-> actual phase shift 7/12, used for spatial scalability x1.5 (only for accurate Chroma alignement)
  { -4, 28, 46, -6 },
  { -2, 20, 52, -6 }, // <-> actual phase shift 2/3, used for spatial scalability x1.5
  { -2, 16, 54, -4 },
  { -2, 14, 56, -4 },
  { -2, 10, 58, -2 }, // <-> actual phase shift 7/8,equal to HEVC MC, used for spatial scalability x2 (only for accurate Chroma alignement)
  {  0,  4, 62, -2 }  // <-> actual phase shift 11/12, used for spatial scalability x1.5 (only for accurate Chroma alignement)
};

TComUpsampleFilter::TComUpsampleFilter(void)
{
}

TComUpsampleFilter::~TComUpsampleFilter(void)
{
}

#if O0215_PHASE_ALIGNMENT
#if O0194_JOINT_US_BITSHIFT
Void TComUpsampleFilter::upsampleBasePic( TComSlice* currSlice, UInt refLayerIdc, TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic, Bool phaseAlignFlag )
#else
Void TComUpsampleFilter::upsampleBasePic( UInt refLayerIdc, TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic, const Window window, Bool phaseAlignFlag )
#endif
#else
#if O0194_JOINT_US_BITSHIFT
#if REF_REGION_OFFSET
Void TComUpsampleFilter::upsampleBasePic( TComSlice* currSlice, UInt refLayerIdc, TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic, const Window window, const Window altRefWindow )
#else
Void TComUpsampleFilter::upsampleBasePic( TComSlice* currSlice, UInt refLayerIdc, TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic, const Window window )
#endif
#else
Void TComUpsampleFilter::upsampleBasePic( UInt refLayerIdc, TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic, const Window window )
#endif
#endif
{
  assert ( NTAPS_US_LUMA == 8 );
  assert ( NTAPS_US_CHROMA == 4 );

  Int i, j;

#if O0194_JOINT_US_BITSHIFT
  UInt currLayerId = currSlice->getLayerId();
  UInt refLayerId  = currSlice->getVPS()->getRefLayerId( currLayerId, refLayerIdc );
#endif

#if MOVE_SCALED_OFFSET_TO_PPS
#if O0098_SCALED_REF_LAYER_ID
  const Window &scalEL = currSlice->getPPS()->getScaledRefLayerWindowForLayer(refLayerId);
#else
  const Window &scalEL = currSlice->getPPS()->getScaledRefLayerWindow(refLayerIdc);
#endif
#if REF_REGION_OFFSET
  const Window &windowRL = currSlice->getPPS()->getRefLayerWindow(refLayerIdc);
#endif
#else
#if O0098_SCALED_REF_LAYER_ID
  const Window &scalEL = currSlice->getSPS()->getScaledRefLayerWindowForLayer(refLayerId);
#else
  const Window &scalEL = currSlice->getSPS()->getScaledRefLayerWindow(refLayerIdc);
#endif
#endif

  //========== Y component upsampling ===========
  Int widthBL   = pcBasePic->getWidth ();
  Int heightBL  = pcBasePic->getHeight();
  Int strideBL  = pcBasePic->getStride();

  Int widthEL   = pcUsPic->getWidth () - scalEL.getWindowLeftOffset() - scalEL.getWindowRightOffset();
  Int heightEL  = pcUsPic->getHeight() - scalEL.getWindowTopOffset()  - scalEL.getWindowBottomOffset();
  Int strideEL  = pcUsPic->getStride();

#if Q0200_CONFORMANCE_BL_SIZE
  const Window &confBL = currSlice->getBaseColPic(refLayerIdc)->getConformanceWindow();
#endif
#if Q0200_CONFORMANCE_BL_SIZE || REF_REGION_OFFSET
  Int chromaFormatIdc = currSlice->getBaseColPic(refLayerIdc)->getSlice(0)->getChromaFormatIdc();
  Int xScal = TComSPS::getWinUnitX( chromaFormatIdc );
  Int yScal = TComSPS::getWinUnitY( chromaFormatIdc );
#endif
#if R0209_GENERIC_PHASE
  Int phaseHorLuma   = currSlice->getPPS()->getPhaseHorLuma(refLayerIdc);
  Int phaseVerLuma   = currSlice->getPPS()->getPhaseVerLuma(refLayerIdc);
  Int phaseHorChroma = currSlice->getPPS()->getPhaseHorChroma(refLayerIdc);
  Int phaseVerChroma;
  if (currSlice->getPPS()->getResamplePhaseSetPresentFlag(refLayerIdc))
  {
    phaseVerChroma = currSlice->getPPS()->getPhaseVerChroma(refLayerIdc);
  }
  else
  {
    Int refRegionHeight = heightBL - windowRL.getWindowTopOffset() - windowRL.getWindowBottomOffset();
    phaseVerChroma = (4 * heightEL + (refRegionHeight >> 1)) / refRegionHeight - 4;
  }
#endif
#if P0312_VERT_PHASE_ADJ
  Bool vertPhasePositionEnableFlag = scalEL.getVertPhasePositionEnableFlag();
  Bool vertPhasePositionFlag = currSlice->getVertPhasePositionFlag( refLayerIdc );
  if( vertPhasePositionFlag )
  {
    assert( vertPhasePositionEnableFlag );
  }
#endif

  Pel* piTempBufY = pcTempPic->getLumaAddr();
  Pel* piSrcBufY  = pcBasePic->getLumaAddr();
  Pel* piDstBufY  = pcUsPic->getLumaAddr();

  Pel* piSrcY;
  Pel* piDstY;

  Pel* piTempBufU = pcTempPic->getCbAddr();
  Pel* piSrcBufU  = pcBasePic->getCbAddr();
  Pel* piDstBufU  = pcUsPic->getCbAddr();

  Pel* piTempBufV = pcTempPic->getCrAddr();
  Pel* piSrcBufV  = pcBasePic->getCrAddr();
  Pel* piDstBufV  = pcUsPic->getCrAddr();

  Pel* piSrcU;
  Pel* piDstU;
  Pel* piSrcV;
  Pel* piDstV;

  Int scaleX = g_posScalingFactor[refLayerIdc][0];
  Int scaleY = g_posScalingFactor[refLayerIdc][1];

  // non-normative software optimization for certain simple resampling cases
  if( scaleX == 65536 && scaleY == 65536 ) // ratio 1x
  {
    piSrcY = piSrcBufY;
    piDstY = piDstBufY + scalEL.getWindowLeftOffset() + scalEL.getWindowTopOffset() * strideEL;

#if O0194_JOINT_US_BITSHIFT
    Int shift = g_bitDepthYLayer[currLayerId] - g_bitDepthYLayer[refLayerId];
#if Q0048_CGS_3D_ASYMLUT
    if( currSlice->getPPS()->getCGSFlag() )
    {
      shift = g_bitDepthYLayer[currLayerId] - currSlice->getPPS()->getCGSOutputBitDepthY();
    }
    assert( shift >= 0 );
#endif
#endif

    for( i = 0; i < heightBL; i++ )
    {
#if O0194_JOINT_US_BITSHIFT
      for( j = 0; j < widthBL; j++ )
      {
        piDstY[j] = piSrcY[j] << shift;
      }
#else
      memcpy( piDstY, piSrcY, sizeof(Pel) * widthBL );
#endif
      piSrcY += strideBL;
      piDstY += strideEL;
    }

    widthEL  >>= 1;
    heightEL >>= 1;

    widthBL  >>= 1;
    heightBL >>= 1;

    strideBL = pcBasePic->getCStride();
    strideEL = pcUsPic->getCStride();

    piSrcU = piSrcBufU;
    piSrcV = piSrcBufV;

    piDstU = piDstBufU + ( scalEL.getWindowLeftOffset() >> 1 ) + ( scalEL.getWindowTopOffset() >> 1 ) * strideEL;
    piDstV = piDstBufV + ( scalEL.getWindowLeftOffset() >> 1 ) + ( scalEL.getWindowTopOffset() >> 1 ) * strideEL;

#if O0194_JOINT_US_BITSHIFT
    shift = g_bitDepthCLayer[currLayerId] - g_bitDepthCLayer[refLayerId];
#if Q0048_CGS_3D_ASYMLUT
    if( currSlice->getPPS()->getCGSFlag() )
    {
      shift = g_bitDepthCLayer[currLayerId] - currSlice->getPPS()->getCGSOutputBitDepthC();
    }
#endif
#endif

    for( i = 0; i < heightBL; i++ )
    {
#if O0194_JOINT_US_BITSHIFT
      for( j = 0; j < widthBL; j++ )
      {
        piDstU[j] = piSrcU[j] << shift;
        piDstV[j] = piSrcV[j] << shift;
      }
#else
      memcpy( piDstU, piSrcU, sizeof(Pel) * widthBL );
      memcpy( piDstV, piSrcV, sizeof(Pel) * widthBL );
#endif
      piSrcU += strideBL;
      piSrcV += strideBL;
      piDstU += strideEL;
      piDstV += strideEL;
    }
  }
  else // general resampling process
  {
    Int refPos16 = 0;
    Int phase    = 0;
    Int refPos   = 0;
    Int* coeff = m_chromaFilter[phase];
    for ( i = 0; i < 16; i++)
    {
      memcpy(   m_lumaFilter[i],   m_lumaFixedFilter[i], sizeof(Int) * NTAPS_US_LUMA   );
      memcpy( m_chromaFilter[i], m_chromaFixedFilter[i], sizeof(Int) * NTAPS_US_CHROMA );
    }

    assert ( widthEL >= widthBL );
    assert ( heightEL >= heightBL );

    pcBasePic->setBorderExtension(false);
    pcBasePic->extendPicBorder(); // extend the border.

    Int   shiftX = 16;
    Int   shiftY = 16;

#if R0209_GENERIC_PHASE
    Int phaseX = phaseHorLuma;
    Int phaseY = phaseVerLuma;
    Int addX = ( ( phaseX * scaleX + 8 ) >> 4 ) -  (1 << ( shiftX - 5 ));
    Int addY = ( ( phaseY * scaleY + 8 ) >> 4 ) -  (1 << ( shiftX - 5 ));
#if REF_REGION_OFFSET
    Int refOffsetX = windowRL.getWindowLeftOffset() << 4;
    Int refOffsetY = windowRL.getWindowTopOffset()  << 4;
#else
#if Q0200_CONFORMANCE_BL_SIZE
    Int refOffsetX = ( confBL.getWindowLeftOffset() * xScal ) << 4;
    Int refOffsetY = ( confBL.getWindowTopOffset()  * yScal ) << 4;
#endif
#endif
#else
#if O0215_PHASE_ALIGNMENT //for Luma, if Phase 0, then both PhaseX  and PhaseY should be 0. If symmetric: both PhaseX and PhaseY should be 2
    Int   phaseX = 2*phaseAlignFlag;
#if P0312_VERT_PHASE_ADJ
#if Q0120_PHASE_CALCULATION
    Int   phaseY = 2*phaseAlignFlag;
#else
    Int   phaseY = vertPhasePositionEnableFlag ? ( vertPhasePositionFlag * 4 ) : ( 2 * phaseAlignFlag );
#endif
#else
    Int   phaseY = 2*phaseAlignFlag;
#endif
#else
    Int   phaseX = 0;
#if P0312_VERT_PHASE_ADJ
#if Q0120_PHASE_CALCULATION
    Int   phaseY = 0;
#else
    Int   phaseY = (vertPhasePositionEnableFlag?(vertPhasePositionFlag *4):(0));
#endif
#else
    Int   phaseY = 0;
#endif
#endif
 
    Int   addX = ( ( phaseX * scaleX + 2 ) >> 2 ) + ( 1 << ( shiftX - 5 ) );
    Int   addY = ( ( phaseY * scaleY + 2 ) >> 2 ) + ( 1 << ( shiftY - 5 ) );

#if Q0120_PHASE_CALCULATION
    Int   deltaX = (Int)phaseAlignFlag <<3;
    Int   deltaY = (((Int)phaseAlignFlag <<3)>>(Int)vertPhasePositionEnableFlag) + ((Int)vertPhasePositionFlag<<3);
#else
    Int   deltaX = 4 * phaseX;
    Int   deltaY = 4 * phaseY;
#endif

#if REF_REGION_OFFSET
    Int refOffsetX = windowRL.getWindowLeftOffset() << 4;
    Int refOffsetY = windowRL.getWindowTopOffset() << 4;
#else
#if Q0200_CONFORMANCE_BL_SIZE
    deltaX -= ( confBL.getWindowLeftOffset() * xScal ) << 4;
    deltaY -= ( confBL.getWindowTopOffset() * yScal ) << 4;
#endif
#endif
#endif

    Int shiftXM4 = shiftX - 4;
    Int shiftYM4 = shiftY - 4;

    widthEL  = pcUsPic->getWidth ();
    heightEL = pcUsPic->getHeight();

    widthBL  = pcBasePic->getWidth ();
    heightBL = min<Int>( pcBasePic->getHeight(), heightEL );

#if R0220_REMOVE_EL_CLIP
    Int phaseXL = scalEL.getWindowLeftOffset();
    Int phaseYL = scalEL.getWindowTopOffset();
    Int rlClipL = -(NTAPS_US_LUMA>>1);
    Int rlClipR = widthBL -1 + (NTAPS_US_LUMA>>1);
    Int rlClipT = -(NTAPS_US_LUMA>>1);
    Int rlClipB = heightBL - 1 + (NTAPS_US_LUMA>>1);
#else
    Int leftStartL = scalEL.getWindowLeftOffset();
    Int rightEndL  = pcUsPic->getWidth() - scalEL.getWindowRightOffset();
    Int topStartL  = scalEL.getWindowTopOffset();
    Int bottomEndL = pcUsPic->getHeight() - scalEL.getWindowBottomOffset();
    Int leftOffset = leftStartL > 0 ? leftStartL : 0;
#endif
#if O0194_JOINT_US_BITSHIFT
    // g_bitDepthY was set to EL bit-depth, but shift1 should be calculated using BL bit-depth
    Int shift1 = g_bitDepthYLayer[refLayerId] - 8;
#if Q0048_CGS_3D_ASYMLUT
    if( currSlice->getPPS()->getCGSFlag() )
    {
      shift1 = currSlice->getPPS()->getCGSOutputBitDepthY() - 8;
    }
#endif
#else
    Int shift1 = g_bitDepthY - 8;
#endif

    //========== horizontal upsampling ===========
    for( i = 0; i < widthEL; i++ )
    {
#if R0220_REMOVE_EL_CLIP
      Int x = i;
#if R0209_GENERIC_PHASE
      refPos16 = (((x - phaseXL)*scaleX - addX) >> shiftXM4) + refOffsetX;
#else
#if REF_REGION_OFFSET
      refPos16 = (((x - phaseXL)*scaleX + addX) >> shiftXM4) - deltaX + refOffsetX;
#else
      refPos16 = (((x - phaseXL)*scaleX + addX) >> shiftXM4) - deltaX;
#endif
#endif
#else
      Int x = Clip3( leftStartL, rightEndL - 1, i );
#if REF_REGION_OFFSET
      refPos16 = (((x - leftStartL)*scaleX + addX) >> shiftXM4) - deltaX + refOffsetX;
#else
      refPos16 = (((x - leftStartL)*scaleX + addX) >> shiftXM4) - deltaX;
#endif
#endif
      phase    = refPos16 & 15;
      refPos   = refPos16 >> 4;
#if R0220_REMOVE_EL_CLIP
      refPos   = Clip3( rlClipL, rlClipR, refPos );
#endif
      coeff = m_lumaFilter[phase];

      piSrcY = piSrcBufY + refPos -((NTAPS_US_LUMA>>1) - 1);
      piDstY = piTempBufY + i;

      for( j = 0; j < heightBL ; j++ )
      {
        *piDstY = sumLumaHor(piSrcY, coeff) >> shift1;
        piSrcY += strideBL;
        piDstY += strideEL;
      }
    }

    //========== vertical upsampling ===========
    pcTempPic->setBorderExtension(false);
    pcTempPic->setHeight(heightBL);
    pcTempPic->extendPicBorder   (); // extend the border.
    pcTempPic->setHeight(heightEL);

#if O0194_JOINT_US_BITSHIFT
    Int nShift = 20 - g_bitDepthYLayer[currLayerId];
#else
    Int nShift = US_FILTER_PREC*2 - shift1;
#endif
    Int iOffset = 1 << (nShift - 1);

    for( j = 0; j < pcTempPic->getHeight(); j++ )
    {
#if R0220_REMOVE_EL_CLIP
      Int y = j;
#if R0209_GENERIC_PHASE
      refPos16 = ((( y - phaseYL )*scaleY - addY) >> shiftYM4) + refOffsetY;
#else
#if REF_REGION_OFFSET
      refPos16 = ((( y - phaseYL )*scaleY + addY) >> shiftYM4) - deltaY + refOffsetY;
#else
      refPos16 = ((( y - pahseYL )*scaleY + addY) >> shiftYM4) - deltaY;
#endif
#endif
#else
      Int y = Clip3(topStartL, bottomEndL - 1, j);
#if REF_REGION_OFFSET
      refPos16 = ((( y - topStartL )*scaleY + addY) >> shiftYM4) - deltaY + refOffsetY;
#else
      refPos16 = ((( y - topStartL )*scaleY + addY) >> shiftYM4) - deltaY;
#endif
#endif
      phase    = refPos16 & 15;
      refPos   = refPos16 >> 4;
#if R0220_REMOVE_EL_CLIP
      refPos = Clip3( rlClipT, rlClipB, refPos );
#endif
      coeff = m_lumaFilter[phase];

      piSrcY = piTempBufY + (refPos -((NTAPS_US_LUMA>>1) - 1))*strideEL;
      Pel* piDstY0 = piDstBufY + j * strideEL;

#if R0220_REMOVE_EL_CLIP
      piDstY = piDstY0;

      for( i = pcTempPic->getWidth(); i > 0; i-- )
      {
        *piDstY = ClipY( (sumLumaVer(piSrcY, coeff, strideEL) + iOffset) >> (nShift));
        piSrcY++;
        piDstY++;
      }
#else
      piDstY = piDstY0 + leftOffset;
      piSrcY += leftOffset;

      for( i = min<Int>(rightEndL, pcTempPic->getWidth()) - max<Int>(0, leftStartL); i > 0; i-- )
      {
        *piDstY = ClipY( (sumLumaVer(piSrcY, coeff, strideEL) + iOffset) >> (nShift));
        piSrcY++;
        piDstY++;
      }

      for( i = rightEndL; i < pcTempPic->getWidth(); i++ )
      {
        *piDstY = piDstY0[rightEndL-1];
        piDstY++;
      }

      piDstY = piDstY0;
      for( i = 0; i < leftStartL; i++ )
      {
        *piDstY = piDstY0[leftStartL];
        piDstY++;
      }
#endif
    }

    widthBL   = pcBasePic->getWidth ();
    heightBL  = pcBasePic->getHeight();
    widthEL   = pcUsPic->getWidth () - scalEL.getWindowLeftOffset() - scalEL.getWindowRightOffset();
    heightEL  = pcUsPic->getHeight() - scalEL.getWindowTopOffset()  - scalEL.getWindowBottomOffset();

    //========== UV component upsampling ===========

    widthEL  >>= 1;
    heightEL >>= 1;

    widthBL  >>= 1;
    heightBL >>= 1;

    strideBL  = pcBasePic->getCStride();
    strideEL  = pcUsPic->getCStride();

#if R0220_REMOVE_EL_CLIP
    Int srlLOffsetC = scalEL.getWindowLeftOffset() >> 1;
    Int srlTOffsetC = scalEL.getWindowTopOffset() >> 1;
    rlClipL = -(NTAPS_US_CHROMA>>1);
    rlClipR = widthBL -1 + (NTAPS_US_CHROMA>>1);
    rlClipT = -(NTAPS_US_CHROMA>>1);
    rlClipB = heightBL - 1 + (NTAPS_US_CHROMA>>1);
#else
    Int leftStartC = scalEL.getWindowLeftOffset() >> 1;
    Int rightEndC  = (pcUsPic->getWidth() >> 1) - (scalEL.getWindowRightOffset() >> 1);
    Int topStartC  = scalEL.getWindowTopOffset() >> 1;
    Int bottomEndC = (pcUsPic->getHeight() >> 1) - (scalEL.getWindowBottomOffset() >> 1);
    leftOffset = leftStartC > 0 ? leftStartC : 0;
#endif
    shiftX = 16;
    shiftY = 16;

#if R0209_GENERIC_PHASE
    addX = ( ( phaseHorChroma * scaleX + 8 ) >> 4 ) -  (1 << ( shiftX - 5 ));
    addY = ( ( phaseVerChroma * scaleY + 8 ) >> 4 ) -  (1 << ( shiftX - 5 ));
    Int refOffsetXC = (windowRL.getWindowLeftOffset() / xScal) << 4;
    Int refOffsetYC = (windowRL.getWindowTopOffset()  / yScal) << 4;
#else
#if O0215_PHASE_ALIGNMENT
    Int phaseXC = phaseAlignFlag;
#if P0312_VERT_PHASE_ADJ
#if Q0120_PHASE_CALCULATION
    Int phaseYC = phaseAlignFlag + 1;
#else
    Int phaseYC = vertPhasePositionEnableFlag ? ( vertPhasePositionFlag * 4 ) : ( phaseAlignFlag + 1 );
#endif
#else
    Int phaseYC = phaseAlignFlag + 1;
#endif
#else
    Int phaseXC = 0;
#if P0312_VERT_PHASE_ADJ
#if Q0120_PHASE_CALCULATION
    Int phaseYC = 1;
#else
    Int phaseYC = vertPhasePositionEnableFlag ? (vertPhasePositionFlag * 4): 1;
#endif
#else
    Int phaseYC = 1;
#endif
#endif
    
    addX       = ( ( phaseXC * scaleX + 2 ) >> 2 ) + ( 1 << ( shiftX - 5 ) );
    addY       = ( ( phaseYC * scaleY + 2 ) >> 2 ) + ( 1 << ( shiftY - 5 ) );

#if Q0120_PHASE_CALCULATION
    deltaX     = (Int)phaseAlignFlag << 2;
    deltaY     = ((( (Int)phaseAlignFlag +1)<<2)>>(Int)vertPhasePositionEnableFlag)+((Int)vertPhasePositionFlag<<3);
#else
    deltaX     = 4 * phaseXC;
    deltaY     = 4 * phaseYC;
#endif

#if REF_REGION_OFFSET
    Int refOffsetXC = (windowRL.getWindowLeftOffset() / xScal) << 4;
    Int refOffsetYC  = (windowRL.getWindowTopOffset()  / yScal) << 4;
#else
#if Q0200_CONFORMANCE_BL_SIZE
    deltaX -= ( ( confBL.getWindowLeftOffset() * xScal ) >> 1 ) << 4;
    deltaY  -= ( ( confBL.getWindowTopOffset() * yScal ) >> 1 ) << 4;
#endif
#endif
#endif

    shiftXM4 = shiftX - 4;
    shiftYM4 = shiftY - 4;

    widthEL   = pcUsPic->getWidth () >> 1;
    heightEL  = pcUsPic->getHeight() >> 1;

    widthBL   = pcBasePic->getWidth () >> 1;
    heightBL  = min<Int>( pcBasePic->getHeight() >> 1, heightEL );

#if O0194_JOINT_US_BITSHIFT
    // g_bitDepthC was set to EL bit-depth, but shift1 should be calculated using BL bit-depth
    shift1 = g_bitDepthCLayer[refLayerId] - 8;
#if Q0048_CGS_3D_ASYMLUT
    if( currSlice->getPPS()->getCGSFlag() )
    {
      shift1 = currSlice->getPPS()->getCGSOutputBitDepthC() - 8;
    }
#endif
#else
    shift1 = g_bitDepthC - 8;
#endif

    //========== horizontal upsampling ===========
    for( i = 0; i < widthEL; i++ )
    {
#if R0220_REMOVE_EL_CLIP
      Int x = i;
#if R0209_GENERIC_PHASE
      refPos16 = (((x - srlLOffsetC)*scaleX - addX) >> shiftXM4) + refOffsetXC;
#else
#if REF_REGION_OFFSET
      refPos16 = (((x - srlLOffsetC)*scaleX + addX) >> shiftXM4) - deltaX + refOffsetXC;
#else
      refPos16 = (((x - srlLOffsetC)*scaleX + addX) >> shiftXM4) - deltaX;
#endif
#endif
#else
      Int x = Clip3(leftStartC, rightEndC - 1, i);
#if REF_REGION_OFFSET
      refPos16 = (((x - leftStartC)*scaleX + addX) >> shiftXM4) - deltaX + refOffsetXC;
#else
      refPos16 = (((x - leftStartC)*scaleX + addX) >> shiftXM4) - deltaX;
#endif
#endif
      phase    = refPos16 & 15;
      refPos   = refPos16 >> 4;
#if R0220_REMOVE_EL_CLIP
      refPos   = Clip3(rlClipL, rlClipR, refPos);
#endif
      coeff = m_chromaFilter[phase];

      piSrcU = piSrcBufU + refPos -((NTAPS_US_CHROMA>>1) - 1);
      piSrcV = piSrcBufV + refPos -((NTAPS_US_CHROMA>>1) - 1);
      piDstU = piTempBufU + i;
      piDstV = piTempBufV + i;

      for( j = 0; j < heightBL ; j++ )
      {
        *piDstU = sumChromaHor(piSrcU, coeff) >> shift1;
        *piDstV = sumChromaHor(piSrcV, coeff) >> shift1;

        piSrcU += strideBL;
        piSrcV += strideBL;
        piDstU += strideEL;
        piDstV += strideEL;
      }
    }

    //========== vertical upsampling ===========
    pcTempPic->setBorderExtension(false);
    pcTempPic->setHeight(heightBL << 1);
    pcTempPic->extendPicBorder   (); // extend the border.
    pcTempPic->setHeight(heightEL << 1);

#if O0194_JOINT_US_BITSHIFT
    nShift = 20 - g_bitDepthCLayer[currLayerId];
#else
    nShift = US_FILTER_PREC*2 - shift1;
#endif
    iOffset = 1 << (nShift - 1);

    for( j = 0; j < pcTempPic->getHeight() >> 1; j++ )
    {
#if R0220_REMOVE_EL_CLIP
      Int y = j;
#if R0209_GENERIC_PHASE
      refPos16 = (((y - srlTOffsetC)*scaleX - addY) >> shiftYM4) + refOffsetYC;
#else
#if REF_REGION_OFFSET
      refPos16 = (((y - srlTOffsetC)*scaleY + addY) >> shiftYM4) - deltaY + refOffsetYC;
#else
      refPos16 = (((y - srlTOffsetC)*scaleY + addY) >> shiftYM4) - deltaY;
#endif
#endif
#else
      Int y = Clip3(topStartC, bottomEndC - 1, j);
#if REF_REGION_OFFSET
      refPos16 = (((y - topStartC)*scaleY + addY) >> shiftYM4) - deltaY + refOffsetYC;
#else
      refPos16 = (((y - topStartC)*scaleY + addY) >> shiftYM4) - deltaY;
#endif
#endif
      phase    = refPos16 & 15;
      refPos   = refPos16 >> 4;
#if R0220_REMOVE_EL_CLIP
      refPos = Clip3(rlClipT, rlClipB, refPos);
#endif
      coeff = m_chromaFilter[phase];

      piSrcU = piTempBufU  + (refPos -((NTAPS_US_CHROMA>>1) - 1))*strideEL;
      piSrcV = piTempBufV  + (refPos -((NTAPS_US_CHROMA>>1) - 1))*strideEL;

      Pel* piDstU0 = piDstBufU + j*strideEL;
      Pel* piDstV0 = piDstBufV + j*strideEL;

#if R0220_REMOVE_EL_CLIP
      piDstU = piDstU0;
      piDstV = piDstV0;

      for( i = pcTempPic->getWidth() >> 1; i > 0; i-- )
      {
        *piDstU = ClipC( (sumChromaVer(piSrcU, coeff, strideEL) + iOffset) >> (nShift));
        *piDstV = ClipC( (sumChromaVer(piSrcV, coeff, strideEL) + iOffset) >> (nShift));
        piSrcU++;
        piSrcV++;
        piDstU++;
        piDstV++;
      }
#else
      piDstU = piDstU0 + leftOffset;
      piDstV = piDstV0 + leftOffset;
      piSrcU += leftOffset;
      piSrcV += leftOffset;

      for( i = min<Int>(rightEndC, pcTempPic->getWidth() >> 1) - max<Int>(0, leftStartC); i > 0; i-- )
      {
        *piDstU = ClipC( (sumChromaVer(piSrcU, coeff, strideEL) + iOffset) >> (nShift));
        *piDstV = ClipC( (sumChromaVer(piSrcV, coeff, strideEL) + iOffset) >> (nShift));
        piSrcU++;
        piSrcV++;
        piDstU++;
        piDstV++;
      }

      for( i = rightEndC; i < pcTempPic->getWidth() >> 1; i++ )
      {
        *piDstU = piDstU0[rightEndC-1];
        *piDstV = piDstV0[rightEndC-1];
        piDstU++;
        piDstV++;
      }

      piDstU = piDstU0;
      piDstV = piDstV0;
      for( i = 0; i < leftStartC; i++ )
      {
        *piDstU = piDstU0[leftStartC];
        *piDstV = piDstV0[leftStartC];
        piDstU++;
        piDstV++;
      }
#endif

    }
  }
    pcUsPic->setBorderExtension(false);
    pcUsPic->extendPicBorder   (); // extend the border.

    //Reset the Border extension flag
    pcUsPic->setBorderExtension(false);
    pcTempPic->setBorderExtension(false);
    pcBasePic->setBorderExtension(false);
}
#endif //SVC_EXTENSION
