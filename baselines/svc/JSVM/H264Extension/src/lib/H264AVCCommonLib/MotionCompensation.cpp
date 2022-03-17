
#include "H264AVCCommonLib.h"

#include "H264AVCCommonLib/SampleWeighting.h"
#include "H264AVCCommonLib/QuarterPelFilter.h"
#include "H264AVCCommonLib/Frame.h"
#include "H264AVCCommonLib/MotionVectorCalculation.h"
#include "H264AVCCommonLib/MotionCompensation.h"

H264AVC_NAMESPACE_BEGIN




MotionCompensation::MotionCompensation():
  m_pcQuarterPelFilter( NULL ),
  m_pcSampleWeighting( NULL ),
  m_uiMbInFrameY( 0 ),
  m_uiMbInFrameX( 0 )
{
}


MotionCompensation::~MotionCompensation()
{
}


ErrVal MotionCompensation::create( MotionCompensation*& rpcMotionCompensation )
{
  rpcMotionCompensation = new MotionCompensation;

  ROT( NULL == rpcMotionCompensation );

  return Err::m_nOK;
}


ErrVal MotionCompensation::destroy()
{
  delete this;

  return Err::m_nOK;
}



ErrVal MotionCompensation::init( QuarterPelFilter* pcQuarterPelFilter,
                                 Transform*        pcTransform,
                                 SampleWeighting* pcSampleWeighting )
{
  ROT( NULL == pcQuarterPelFilter );
  ROT( NULL == pcSampleWeighting );

  m_pcTransform = pcTransform;

  m_pcQuarterPelFilter  = pcQuarterPelFilter;
  m_pcSampleWeighting   = pcSampleWeighting;

  return Err::m_nOK;
}


ErrVal MotionCompensation::initSlice( const SliceHeader& rcSH )
{
  m_uiMbInFrameY = rcSH.getSPS().getFrameHeightInMbs();
  m_uiMbInFrameX = rcSH.getSPS().getFrameWidthInMbs();
  if( rcSH.getFieldPicFlag() )
  {
    m_uiMbInFrameY /= 2;
  }
//TMM_WP
      m_pcSampleWeighting->initSliceForWeighting(rcSH);
//TMM_WP

  RNOK( MotionVectorCalculation::initSlice( rcSH ) );

  return Err::m_nOK;
}


ErrVal MotionCompensation::uninit()
{
  RNOK( MotionVectorCalculation::uninit() );

  m_pcQuarterPelFilter  = NULL;
  m_pcSampleWeighting   = NULL;

  return Err::m_nOK;
}

Short MotionCompensation::xCorrectChromaMv( const MbDataAccess& rcMbDataAccess, PicType eRefPicType )
{
  PicType eCurrType = rcMbDataAccess.getMbPicType();

  if( eRefPicType == TOP_FIELD && eCurrType == BOT_FIELD )
  {
    return 2;
  }
  if( eRefPicType == BOT_FIELD && eCurrType == TOP_FIELD )
  {
    return -2;
  }
  return 0;
}



ErrVal MotionCompensation::compensateDirectBlock( MbDataAccess& rcMbDataAccess, YuvMbBuffer *pcRecBuffer, B8x8Idx c8x8Idx, RefFrameList& rcRefFrameListL0, RefFrameList& rcRefFrameListL1 )
{
  Par8x8 ePar8x8 = c8x8Idx.b8x8Index();
  MC8x8          cMC8x8D( ePar8x8 );
  YuvMbBuffer*  apcTarBuffer[2];

  Int       iRefIdx0    = rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( ePar8x8 );
  Int       iRefIdx1    = rcMbDataAccess.getMbMotionData( LIST_1 ).getRefIdx( ePar8x8 );
  Frame* pcRefFrame0 = ( iRefIdx0 > 0 ? rcRefFrameListL0[ iRefIdx0 ] : NULL );
  Frame* pcRefFrame1 = ( iRefIdx1 > 0 ? rcRefFrameListL1[ iRefIdx1 ] : NULL );

  if( rcMbDataAccess.getSH().getSPS().getDirect8x8InferenceFlag() )
  {
    xGetBlkPredData( rcMbDataAccess, pcRefFrame0, pcRefFrame1, cMC8x8D, BLK_8x8 );
    m_pcSampleWeighting->getTargetBuffers( apcTarBuffer, pcRecBuffer, cMC8x8D.m_apcPW[LIST_0], cMC8x8D.m_apcPW[LIST_1] );

    xPredLuma(   apcTarBuffer, 8, 8, cMC8x8D, SPART_4x4_0 );
    xPredChroma( apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_0 );
  }
  else
  {
    xGetBlkPredData( rcMbDataAccess, pcRefFrame0, pcRefFrame1, cMC8x8D, BLK_4x4 );
    m_pcSampleWeighting->getTargetBuffers( apcTarBuffer, pcRecBuffer, cMC8x8D.m_apcPW[LIST_0], cMC8x8D.m_apcPW[LIST_1] );

    xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_0 );
    xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_1 );
    xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_2 );
    xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_3 );
    xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_0 );
    xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_1 );
    xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_2 );
    xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_3 );

  }

  m_pcSampleWeighting->weightLumaSamples  ( pcRecBuffer, 8, 8, c8x8Idx, cMC8x8D.m_apcPW[LIST_0], cMC8x8D.m_apcPW[LIST_1] );
  m_pcSampleWeighting->weightChromaSamples( pcRecBuffer, 4, 4, c8x8Idx, cMC8x8D.m_apcPW[LIST_0], cMC8x8D.m_apcPW[LIST_1] );

  return Err::m_nOK;
}



ErrVal MotionCompensation::initMb( UInt uiMbY, UInt uiMbX, MbDataAccess& rcMbDataAccess )// TMM_INTERLACE
{
  UInt uiMbInFrameY = m_uiMbInFrameY;
  if( rcMbDataAccess.getMbData().getFieldFlag() )
  {
    uiMbInFrameY >>= 1;
    uiMbY        >>= 1;
  }
  m_cMin.setHor( (Short) gMax( (Int)MSYS_SHORT_MIN, (Int)((-(Int)uiMbX << 4) - (16+8) ) << 2 ) );
  m_cMin.setVer( (Short) gMax( (Int)MSYS_SHORT_MIN, (Int)((-(Int)uiMbY << 4) - (16+8) ) << 2 ) );

  m_cMax.setHor( (Short) gMin( (Int)MSYS_SHORT_MAX, (Int)(((m_uiMbInFrameX - uiMbX) << 4) + 8 ) << 2 ) );
  m_cMax.setVer( (Short) gMin( (Int)MSYS_SHORT_MAX, (Int)(((  uiMbInFrameY - uiMbY) << 4) + 8 ) << 2 ) );

  return Err::m_nOK;
}



Void MotionCompensation::xPredLuma( YuvMbBuffer* pcRecBuffer, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D )
{
  YuvMbBuffer* apcTarBuffer[2];
  m_pcSampleWeighting->getTargetBuffers( apcTarBuffer, pcRecBuffer, rcMc8x8D.m_apcPW[LIST_0], rcMc8x8D.m_apcPW[LIST_1] );

  for( Int n = 0; n < 2; n++ )
  {
    YuvPicBuffer* pcRefBuffer = rcMc8x8D.m_apcRefBuffer[n];
    if( NULL != pcRefBuffer )
    {
      const Mv cMv = rcMc8x8D.m_aacMv[n][0];
      m_pcQuarterPelFilter->predBlk( apcTarBuffer[n], pcRefBuffer, rcMc8x8D.m_cIdx, cMv, iSizeY, iSizeX );
    }
  }
  m_pcSampleWeighting->weightLumaSamples( pcRecBuffer, iSizeX, iSizeY, rcMc8x8D.m_cIdx, rcMc8x8D.m_apcPW[LIST_0], rcMc8x8D.m_apcPW[LIST_1] );
}

Void MotionCompensation::xPredLuma( YuvMbBuffer* apcTarBuffer[2], Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, SParIdx4x4 eSParIdx )
{
  B4x4Idx cIdx( rcMc8x8D.m_cIdx + eSParIdx );

  for( Int n = 0; n < 2; n++ )
  {
    YuvPicBuffer* pcRefBuffer = rcMc8x8D.m_apcRefBuffer[n];
    if( NULL != pcRefBuffer )
    {
      const Mv cMv = rcMc8x8D.m_aacMv[n][eSParIdx];
      m_pcQuarterPelFilter->predBlk( apcTarBuffer[n], pcRefBuffer, cIdx, cMv, iSizeY, iSizeX  );
    }
  }
}

__inline Void MotionCompensation::xPredChromaPel( XPel* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Mv cMv, Int iSizeY, Int iSizeX )
{
  Int xF1 = cMv.getHor();
  Int yF1 = cMv.getVer();

  xF1 &= 0x7;
  yF1 &= 0x7;

  Int x, y;
  if( 0 == xF1 )
  {
    if( 0 == yF1 )
    {
      for( y = 0; y < iSizeY; y++ )
      {
        for( x = 0; x < iSizeX; x++ )
        {
          pucDest[x  ] = pucSrc[x  ];
        }
        pucDest += iDestStride;
        pucSrc  += iSrcStride;
      }
      return;
    }

    Int yF0 = 8 - yF1;

    for( y = 0; y < iSizeY; y++ )
    {
      for( x = 0; x < iSizeX; x++ )
      {
        pucDest[x  ] = (yF0 * pucSrc[x  ] + yF1 * pucSrc[x+iSrcStride  ] + 4) >> 3;
      }
      pucDest += iDestStride;
      pucSrc  += iSrcStride;
    }
    return;
  }


  if( 0 == yF1 )
  {
    Int xF0 = 8 - xF1;

    for( y = 0; y < iSizeY; y++ )
    {
      for( x = 0; x < iSizeX; x++ )
      {
        pucDest[x] = (xF0 * pucSrc[x] + xF1 * pucSrc[x+1] + 4) >> 3;
      }
      pucDest += iDestStride;
      pucSrc  += iSrcStride;
    }
    return;
  }

  Int xF0 = 8 - xF1;
  Int yF0 = 8 - yF1;
  for( y = 0; y < iSizeY; y++ )
  {
    Int a = xF0* ( yF0 * pucSrc[0] + yF1 * pucSrc[iSrcStride]);
    for( x = 0; x < iSizeX; x++ )
    {
      Int b = yF0 * pucSrc[x+1] + yF1 * pucSrc[iSrcStride+x+1];
      Int c = xF1 * b;
      pucDest[x]   = (a + c + 0x20) >> 6;
      a = (b<<3) - c;
    }
    pucDest += iDestStride;
    pucSrc  += iSrcStride;
  }
}

__inline Void MotionCompensation::xPredChroma( YuvMbBuffer* pcDesBuffer, YuvPicBuffer* pcSrcBuffer, LumaIdx cIdx, Mv cMv, Int iSizeY, Int iSizeX)
{
  const Int iDesStride  = pcDesBuffer->getCStride();
  const Int iSrcStride  = pcSrcBuffer->getCStride();

  cMv.limitComponents( m_cMin, m_cMax );

  Int iOffset = (cMv.getHor() >> 3) + (cMv.getVer() >> 3) * iSrcStride;

  xPredChromaPel( pcDesBuffer->getUBlk( cIdx ),          iDesStride,
                  pcSrcBuffer->getUBlk( cIdx )+ iOffset, iSrcStride,
                  cMv, iSizeY, iSizeX );

  xPredChromaPel( pcDesBuffer->getVBlk( cIdx ),          iDesStride,
                  pcSrcBuffer->getVBlk( cIdx )+ iOffset, iSrcStride,
                  cMv, iSizeY, iSizeX );
}

Void MotionCompensation::xPredChroma( YuvMbBuffer* pcRecBuffer, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D )
{
  YuvMbBuffer* apcTarBuffer[2];
  m_pcSampleWeighting->getTargetBuffers( apcTarBuffer, pcRecBuffer, rcMc8x8D.m_apcPW[LIST_0], rcMc8x8D.m_apcPW[LIST_1] );

  for( Int n = 0; n < 2; n++ )
  {
    YuvPicBuffer* pcRefBuffer = rcMc8x8D.m_apcRefBuffer[n];
    if( NULL != pcRefBuffer )
    {
      Mv cMv = rcMc8x8D.m_aacMv[n][0];
	    cMv.setVer( cMv.getVer() + rcMc8x8D.m_sChromaOffset[n] );
      xPredChroma( apcTarBuffer[n], pcRefBuffer, rcMc8x8D.m_cIdx, cMv, iSizeY, iSizeX );
    }
  }
  m_pcSampleWeighting->weightChromaSamples( pcRecBuffer, iSizeX, iSizeY, rcMc8x8D.m_cIdx, rcMc8x8D.m_apcPW[LIST_0], rcMc8x8D.m_apcPW[LIST_1] );
}

Void MotionCompensation::xPredChroma( YuvMbBuffer* apcTarBuffer[2], Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, SParIdx4x4 eSParIdx )
{
  B4x4Idx cIdx( rcMc8x8D.m_cIdx + eSParIdx );

  for( Int n = 0; n < 2; n++ )
  {
    YuvPicBuffer* pcRefBuffer = rcMc8x8D.m_apcRefBuffer[n];
    if( NULL != pcRefBuffer )
    {
      Mv cMv = rcMc8x8D.m_aacMv[n][eSParIdx];
			cMv.setVer( cMv.getVer() + rcMc8x8D.m_sChromaOffset[n] );
      xPredChroma( apcTarBuffer[n], pcRefBuffer, cIdx, cMv, iSizeY, iSizeX );
    }
  }
}



ErrVal MotionCompensation::updateSubMb( B8x8Idx         c8x8Idx,
                                        MbDataAccess&   rcMbDataAccess,
                                        Frame*       pcMCFrame,
                                        Frame*       pcPrdFrame,
                                        ListIdx         eListPrd )
{
  m_curMbX = rcMbDataAccess.getMbX();
  m_curMbY = rcMbDataAccess.getMbY();
  xUpdateMb8x8Mode( c8x8Idx, rcMbDataAccess, pcMCFrame, pcPrdFrame, eListPrd );

  return Err::m_nOK;
}



Void MotionCompensation::xUpdateMb8x8Mode(    B8x8Idx         c8x8Idx,
                                              MbDataAccess&   rcMbDataAccess,
                                              Frame*       pcMCFrame,
                                              Frame*       pcPrdFrame,
                                              ListIdx         eListPrd )
{
  Par8x8  ePar8x8   = c8x8Idx.b8x8Index();
  BlkMode eBlkMode  = rcMbDataAccess.getMbData().getBlkMode( ePar8x8 );

  MC8x8 cMC8x8D( ePar8x8 );
  if(eListPrd == LIST_0)
    xGetBlkPredData( rcMbDataAccess, pcMCFrame, NULL, cMC8x8D, eBlkMode );
  else
    xGetBlkPredData( rcMbDataAccess, NULL, pcMCFrame, cMC8x8D, eBlkMode );

  switch( eBlkMode )
  {
    case BLK_SKIP:
    {
      if( rcMbDataAccess.getSH().getSPS().getDirect8x8InferenceFlag() )
      {
        xUpdateBlk(   pcPrdFrame, 8, 8, cMC8x8D, SPART_4x4_0 );
      }
      else
      {
        xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_0 );
        xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_1 );
        xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_2 );
        xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_3 );
      }
      break;
    }
    case BLK_8x8:
    {
      xUpdateBlk(   pcPrdFrame, 8, 8, cMC8x8D, SPART_4x4_0 );
      break;
    }
    case BLK_8x4:
    {
      xUpdateBlk(   pcPrdFrame, 8, 4, cMC8x8D, SPART_4x4_0 );
      xUpdateBlk(   pcPrdFrame, 8, 4, cMC8x8D, SPART_4x4_2 );
      break;
    }
    case BLK_4x8:
    {
      xUpdateBlk(   pcPrdFrame, 4, 8, cMC8x8D, SPART_4x4_0 );
      xUpdateBlk(   pcPrdFrame, 4, 8, cMC8x8D, SPART_4x4_1 );
      break;
    }
    case BLK_4x4:
    {
      xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_0 );
      xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_1 );
      xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_2 );
      xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_3 );
      break;
    }
    default:
    {
      AF();
      break;
    }
  }
}


ErrVal MotionCompensation::updateMb(MbDataAccess&   rcMbDataAccess,
                                    Frame*       pcMCFrame,
                                    Frame*       pcPrdFrame,
                                    ListIdx         eListPrd,
                                    Int             iRefIdx)
{
  MbMode eMbMode  = rcMbDataAccess.getMbData().getMbMode();

  m_curMbX = rcMbDataAccess.getMbX();
  m_curMbY = rcMbDataAccess.getMbY();

  switch( eMbMode )
  {
  case MODE_16x16:
    {
      MC8x8 cMC8x8D( B_8x8_0 );

      if( rcMbDataAccess.getMbData().getMbMotionData(eListPrd).getRefIdx() == iRefIdx )
      {
        if(eListPrd == LIST_0)
          xGetMbPredData( rcMbDataAccess, pcMCFrame, NULL, cMC8x8D );
        else
          xGetMbPredData( rcMbDataAccess, NULL, pcMCFrame, cMC8x8D );

        xUpdateBlk(   pcPrdFrame, 16, 16, cMC8x8D);
      }
    }
    break;

  case MODE_16x8:
    {

      MC8x8 cMC8x8D0( B_8x8_0 );
      MC8x8 cMC8x8D1( B_8x8_2 );

      if( rcMbDataAccess.getMbData().getMbMotionData(eListPrd).getRefIdx(PART_16x8_0) == iRefIdx )
      {
        if(eListPrd == LIST_0)
          xGetMbPredData( rcMbDataAccess, pcMCFrame, NULL, cMC8x8D0 );
        else
          xGetMbPredData( rcMbDataAccess, NULL, pcMCFrame, cMC8x8D0 );

        xUpdateBlk(   pcPrdFrame, 16, 8, cMC8x8D0 );
      }

      if( rcMbDataAccess.getMbData().getMbMotionData(eListPrd).getRefIdx(PART_16x8_1) == iRefIdx )
      {
        if(eListPrd == LIST_0)
          xGetMbPredData( rcMbDataAccess, pcMCFrame, NULL, cMC8x8D1 );
        else
          xGetMbPredData( rcMbDataAccess, NULL, pcMCFrame, cMC8x8D1 );

        xUpdateBlk(   pcPrdFrame, 16, 8, cMC8x8D1 );
      }
    }
    break;

  case MODE_8x16:
    {
      MC8x8 cMC8x8D0( B_8x8_0 );
      MC8x8 cMC8x8D1( B_8x8_1 );

      if( rcMbDataAccess.getMbData().getMbMotionData(eListPrd).getRefIdx(PART_8x16_0) == iRefIdx )
      {
        if(eListPrd == LIST_0)
          xGetMbPredData( rcMbDataAccess, pcMCFrame, NULL, cMC8x8D0 );
        else
          xGetMbPredData( rcMbDataAccess, NULL, pcMCFrame, cMC8x8D0 );

        xUpdateBlk(   pcPrdFrame, 8, 16, cMC8x8D0 );
      }

      if( rcMbDataAccess.getMbData().getMbMotionData(eListPrd).getRefIdx(PART_8x16_1) == iRefIdx )
      {
        if(eListPrd == LIST_0)
          xGetMbPredData( rcMbDataAccess, pcMCFrame, NULL, cMC8x8D1 );
        else
          xGetMbPredData( rcMbDataAccess, NULL, pcMCFrame, cMC8x8D1 );

        xUpdateBlk(   pcPrdFrame, 8, 16, cMC8x8D1 );
      }
    }
    break;

  case MODE_SKIP:
    {
      if( rcMbDataAccess.getSH().isBSlice() )
      {
        B8x8Idx c8x8Idx;
        RNOK( updateDirectBlock( rcMbDataAccess, pcMCFrame, pcPrdFrame, eListPrd, iRefIdx, c8x8Idx ) ); c8x8Idx++;
        RNOK( updateDirectBlock( rcMbDataAccess, pcMCFrame, pcPrdFrame, eListPrd, iRefIdx, c8x8Idx ) ); c8x8Idx++;
        RNOK( updateDirectBlock( rcMbDataAccess, pcMCFrame, pcPrdFrame, eListPrd, iRefIdx, c8x8Idx ) ); c8x8Idx++;
        RNOK( updateDirectBlock( rcMbDataAccess, pcMCFrame, pcPrdFrame, eListPrd, iRefIdx, c8x8Idx ) ); ;

      }
      else
      {
        MC8x8 cMC8x8D( B_8x8_0 );

        if(rcMbDataAccess.getMbMotionData(eListPrd).getRefIdx(PART_8x16_0) != iRefIdx)
          return Err::m_nOK;

        if(eListPrd == LIST_0)
          xGetMbPredData( rcMbDataAccess, pcMCFrame, NULL, cMC8x8D );
        else
          xGetMbPredData( rcMbDataAccess, NULL, pcMCFrame, cMC8x8D );

        xUpdateBlk(   pcPrdFrame, 16, 16, cMC8x8D );
        return Err::m_nOK;
      }
    }
    break;

  case MODE_8x8:
  case MODE_8x8ref0:
    printf("function not defined for the case\n");
    RERR();
    break;

  default:
    break;
  }

  return Err::m_nOK;
}


ErrVal MotionCompensation::updateDirectBlock( MbDataAccess&   rcMbDataAccess,
                                              Frame*       pcMCFrame,
                                              Frame*       pcPrdFrame,
                                              ListIdx         eListPrd,
                                              Int             iRefIdx,
                                              B8x8Idx         c8x8Idx )
{
  Par8x8 ePar8x8 = c8x8Idx.b8x8Index();
  MC8x8          cMC8x8D( ePar8x8 );

  if(rcMbDataAccess.getMbData().getMbMotionData(eListPrd).getRefIdx(ePar8x8) != iRefIdx)
    return Err::m_nOK;

  if( rcMbDataAccess.getSH().getSPS().getDirect8x8InferenceFlag() )
  {
    if(eListPrd == LIST_0)
      xGetBlkPredData( rcMbDataAccess, pcMCFrame, NULL, cMC8x8D, BLK_8x8 );
    else
      xGetBlkPredData( rcMbDataAccess, NULL, pcMCFrame, cMC8x8D, BLK_8x8 );

    xUpdateBlk(  pcPrdFrame, 8, 8, cMC8x8D, SPART_4x4_0 );
  }
  else
  {
    if(eListPrd == LIST_0)
      xGetBlkPredData( rcMbDataAccess, pcMCFrame, NULL, cMC8x8D, BLK_4x4 );
    else
      xGetBlkPredData( rcMbDataAccess, NULL, pcMCFrame, cMC8x8D, BLK_4x4 );

    xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_0 );
    xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_1 );
    xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_2 );
    xUpdateBlk(   pcPrdFrame, 4, 4, cMC8x8D, SPART_4x4_3 );
  }

  return Err::m_nOK;
}


Void MotionCompensation::xUpdateBlk( Frame* pcPrdFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D)
{
  UShort weight;
  xUpdateLuma( pcPrdFrame, iSizeX, iSizeY, rcMc8x8D, &weight);
  xUpdateChroma(pcPrdFrame, iSizeX/2, iSizeY/2, rcMc8x8D, &weight);
}

Void MotionCompensation::xUpdateBlk( Frame* pcPrdFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, SParIdx4x4 eSParIdx )
{
  UShort weight;
  xUpdateLuma( pcPrdFrame, iSizeX, iSizeY, rcMc8x8D, eSParIdx, &weight);
  xUpdateChroma(pcPrdFrame, iSizeX/2, iSizeY/2, rcMc8x8D, eSParIdx, &weight);
}


Void MotionCompensation::xUpdateLuma( Frame* pcPrdFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, UShort *usWeight)
{
  for( Int n = 0; n < 2; n++ )
  {
    YuvPicBuffer* pcRefBuffer = rcMc8x8D.m_apcRefBuffer[n];
    if( NULL != pcRefBuffer )
    {
      const Mv cMv = rcMc8x8D.m_aacMv[n][0];
      const Mv dMv = rcMc8x8D.m_aacMvd[n][0];

      if( dMv.getAbsHor() + dMv.getAbsVer() >= DMV_THRES)
        continue;

      updateBlkAdapt( pcPrdFrame->getFullPelYuvBuffer(), pcRefBuffer, rcMc8x8D.m_cIdx, cMv, iSizeY, iSizeX, usWeight);
    }
  }
}


Void MotionCompensation::xUpdateLuma( Frame* pcPrdFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, SParIdx4x4 eSParIdx, UShort *usWeight )
{
  B4x4Idx cIdx( rcMc8x8D.m_cIdx + eSParIdx );

  for( Int n = 0; n < 2; n++ )
  {
    YuvPicBuffer* pcRefBuffer = rcMc8x8D.m_apcRefBuffer[n];
    if( NULL != pcRefBuffer )
    {
      const Mv cMv = rcMc8x8D.m_aacMv[n][eSParIdx];
      const Mv dMv = rcMc8x8D.m_aacMvd[n][eSParIdx];

      if( dMv.getAbsHor() + dMv.getAbsVer() >= DMV_THRES)
        continue;

      updateBlkAdapt( pcPrdFrame->getFullPelYuvBuffer(), pcRefBuffer, cIdx, cMv, iSizeY, iSizeX, usWeight  );
    }
  }
}


Void MotionCompensation::updateBlkAdapt( YuvPicBuffer* pcSrcBuffer, YuvPicBuffer* pcDesBuffer, LumaIdx cIdx, Mv cMv, Int iSizeY, Int iSizeX,
                                      UShort *usWeight)
{
  XPel* pucDes    = pcDesBuffer->getYBlk( cIdx );
  XPel* pucSrc    = pcSrcBuffer->getYBlk( cIdx );
  Int iDesStride  = pcDesBuffer->getLStride();
  Int iSrcStride  = pcSrcBuffer->getLStride();
  Short mvHor = cMv.getHor(), mvVer = cMv.getVer();

  Int iOffsetDes     = (mvHor >>2 ) + (mvVer >>2 ) * iDesStride;
  pucDes += iOffsetDes;

  m_pcQuarterPelFilter->weightOnEnergy(usWeight, pucSrc, iSrcStride, iSizeY, iSizeX);

  Int iDx = (mvHor & 3) ;
  Int iDy = (mvVer & 3) ;

  xUpdAdapt( pucDes, pucSrc, iDesStride, iSrcStride, iDx, iDy, iSizeY, iSizeX, *usWeight, 16);
}



#define BUF_W   19
#define BUF_H   19
Void MotionCompensation::xUpdAdapt( XPel* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy,
                                    UInt uiSizeY, UInt uiSizeX, UShort weight, UShort wMax )
{
  Int methodId;

  Int interpBuf[BUF_W*BUF_H]; // interpolation buffer
  Int* pBuf = interpBuf;

  XPel* pDest  = 0;
  Int updSizeX = 0, updSizeY = 0;
  Int bitShift = 0;


  if(weight > wMax/2)
  {
    methodId = 2;
  }
  else if(weight > 0)
  {
    methodId = 1;
  }
  else
    return;

  // initialize buffer
  memset(pBuf, 0, sizeof(Int)*BUF_W*BUF_H);

  // INTERPOLATION
  if(methodId==1)
  {
    // method 1: bi-linear for sub-pel samples
    m_pcQuarterPelFilter->xUpdInterpBlnr(pBuf, pucSrc, BUF_W, iSrcStride, iDx, iDy, uiSizeY, uiSizeX);

    updSizeX = uiSizeX + 1;
    updSizeY = uiSizeY + 1;
    pDest = pucDest;
    bitShift = 4;
  }
  else if(methodId == 2)
  {
    // method 2: 4 tap filter for sub-pel samples
    m_pcQuarterPelFilter->xUpdInterp4Tap(pBuf, pucSrc, BUF_W, iSrcStride, iDx, iDy, uiSizeY, uiSizeX);

    updSizeX = uiSizeX + 3;
    updSizeY = uiSizeY + 3;
    pDest = pucDest - iDestStride - 1;
    bitShift = 8;
  }

  pBuf = interpBuf;
  int Th = gMax(0, (int)weight/2 - 1 ) ;
  for( Int y = 0; y < updSizeY; y++)
  {
    for( Int x = 0; x < updSizeX; x++)
    {
      pBuf [x] = (pBuf[x] + (1 << (bitShift-1))) >> bitShift;
      pBuf [x] = gMax(-Th, gMin(Th, pBuf[x]));
      pDest[x] = (XPel)gClip( pDest[x] + ((pBuf[x] + (pBuf[x]>0? 1:-1))/4) );
    }
    pBuf += BUF_W;
    pDest += iDestStride;
  }
}


__inline Void MotionCompensation::xUpdateChroma( YuvPicBuffer* pcSrcBuffer, YuvPicBuffer* pcDesBuffer,  LumaIdx cIdx, Mv cMv,
                                                Int iSizeY, Int iSizeX, UShort *usWeight)
{
  const Int iDesStride  = pcDesBuffer->getCStride();
  const Int iSrcStride  = pcSrcBuffer->getCStride();

  cMv.limitComponents( m_cMin, m_cMax );

  Short mvHor = cMv.getHor(), mvVer = cMv.getVer();

  const Int iOffsetDes = (mvHor>>3) + (mvVer>>3) * iDesStride;
  const Int iOffsetSrc = 0;

  xUpdateChromaPel( pcDesBuffer->getUBlk( cIdx )+ iOffsetDes, iDesStride,
                    pcSrcBuffer->getUBlk( cIdx )+ iOffsetSrc, iSrcStride,
                    cMv, iSizeY, iSizeX, *usWeight );

  xUpdateChromaPel( pcDesBuffer->getVBlk( cIdx )+ iOffsetDes, iDesStride,
                    pcSrcBuffer->getVBlk( cIdx )+ iOffsetSrc, iSrcStride,
                    cMv, iSizeY, iSizeX, *usWeight );
}

Void MotionCompensation::xUpdateChroma( Frame* pcSrcFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, UShort *usWeight )
{
  for( Int n = 0; n < 2; n++ )
  {
    YuvPicBuffer* pcRefBuffer = rcMc8x8D.m_apcRefBuffer[n];
    if( NULL != pcRefBuffer )
    {
      Mv cMv = rcMc8x8D.m_aacMv[n][0];
      Mv dMv = rcMc8x8D.m_aacMvd[n][0];

      if( dMv.getAbsHor() + dMv.getAbsVer() >= DMV_THRES)
        return;

      xUpdateChroma( pcSrcFrame->getFullPelYuvBuffer(), pcRefBuffer, rcMc8x8D.m_cIdx, cMv, iSizeY, iSizeX, usWeight );
    }
  }
}

Void MotionCompensation::xUpdateChroma( Frame* pcSrcFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, SParIdx4x4 eSParIdx, UShort *usWeight )
{
  B4x4Idx cIdx( rcMc8x8D.m_cIdx + eSParIdx );

  for( Int n = 0; n < 2; n++ )
  {
    YuvPicBuffer* pcRefBuffer = rcMc8x8D.m_apcRefBuffer[n];
    if( NULL != pcRefBuffer )
    {
      Mv cMv = rcMc8x8D.m_aacMv[n][eSParIdx];
      Mv dMv = rcMc8x8D.m_aacMvd[n][eSParIdx];

      if( dMv.getAbsHor() + dMv.getAbsVer() >= DMV_THRES)
        return;

      xUpdateChroma( pcSrcFrame->getFullPelYuvBuffer(), pcRefBuffer, cIdx, cMv, iSizeY, iSizeX, usWeight );
    }
  }
}

__inline Void MotionCompensation::xUpdateChromaPel( XPel* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Mv cMv, Int iSizeY, Int iSizeX, UShort weight )
{
  Int interpBuf[BUF_W*BUF_H]; // interpolation buffer
  Int* pBuf = interpBuf;

  // initialize buffer
  memset(pBuf, 0, sizeof(Int)*BUF_W*BUF_H);

  m_pcQuarterPelFilter->xUpdInterpChroma(pBuf, BUF_W, pucSrc, iSrcStride, cMv, iSizeY, iSizeX);

  pBuf = interpBuf;
  int Th = gMax(0, (int)weight/2 - 1 ) ;
  for( Int y = 0; y < iSizeY + 1; y++)
  {
    for( Int x = 0; x < iSizeX + 1; x++)
    {
      pBuf   [x] = (pBuf[x] + 32) >> 6;
      pBuf   [x] = gMax(-Th, gMin(Th, pBuf[x]));
      pucDest[x] = (XPel)gClip( pucDest[x] + ((pBuf[x] + (pBuf[x]>0? 2:-2))/4) );
    }
    pBuf += BUF_W;
    pucDest += iDestStride;
  }
}


ErrVal MotionCompensation::compensateSubMb( B8x8Idx         c8x8Idx,
                                            MbDataAccess&   rcMbDataAccess,
                                            RefFrameList&   rcRefFrameList0,
                                            RefFrameList&   rcRefFrameList1,
                                            YuvMbBuffer* pcRecBuffer,
                                            Bool            bCalcMv,
                                            Bool            bFaultTolerant )
{
  if( bCalcMv )
  {
    xCalc8x8( c8x8Idx, rcMbDataAccess, NULL, bFaultTolerant );
  }

  Int       iRefIdx0    = rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( c8x8Idx.b8x8() );
  Int       iRefIdx1    = rcMbDataAccess.getMbMotionData( LIST_1 ).getRefIdx( c8x8Idx.b8x8() );
  Frame* pcRefFrame0 = ( iRefIdx0 > 0 ? rcRefFrameList0[ iRefIdx0 ] : NULL );
  Frame* pcRefFrame1 = ( iRefIdx1 > 0 ? rcRefFrameList1[ iRefIdx1 ] : NULL );

  xPredMb8x8Mode( c8x8Idx, rcMbDataAccess, pcRefFrame0, pcRefFrame1, pcRecBuffer );

  return Err::m_nOK;
}

Void MotionCompensation::xPredMb8x8Mode(       B8x8Idx         c8x8Idx,
                                               MbDataAccess&   rcMbDataAccess,
                                         const Frame*       pcRefFrame0,
                                         const Frame*       pcRefFrame1,
                                               YuvMbBuffer* pcRecBuffer  )
{
  YuvMbBuffer* apcTarBuffer[2];
  Par8x8  ePar8x8   = c8x8Idx.b8x8Index();
  BlkMode eBlkMode  = rcMbDataAccess.getMbData().getBlkMode( ePar8x8 );

  MC8x8 cMC8x8D( ePar8x8 );
  xGetBlkPredData( rcMbDataAccess, pcRefFrame0, pcRefFrame1, cMC8x8D, eBlkMode );
  m_pcSampleWeighting->getTargetBuffers( apcTarBuffer, pcRecBuffer, cMC8x8D.m_apcPW[LIST_0], cMC8x8D.m_apcPW[LIST_1] );

  switch( eBlkMode )
  {
    case BLK_SKIP:
    {
      if( rcMbDataAccess.getSH().getSPS().getDirect8x8InferenceFlag() )
      {
        xPredLuma(   apcTarBuffer, 8, 8, cMC8x8D, SPART_4x4_0 );
        xPredChroma( apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_0 );
      }
      else
      {
        xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_0 );
        xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_1 );
        xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_2 );
        xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_3 );
        xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_0 );
        xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_1 );
        xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_2 );
        xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_3 );
      }
      break;
    }
    case BLK_8x8:
    {
      xPredLuma(   apcTarBuffer, 8, 8, cMC8x8D, SPART_4x4_0 );
      xPredChroma( apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_0 );
      break;
    }
    case BLK_8x4:
    {
      xPredLuma(   apcTarBuffer, 8, 4, cMC8x8D, SPART_4x4_0 );
      xPredLuma(   apcTarBuffer, 8, 4, cMC8x8D, SPART_4x4_2 );
      xPredChroma( apcTarBuffer, 4, 2, cMC8x8D, SPART_4x4_0 );
      xPredChroma( apcTarBuffer, 4, 2, cMC8x8D, SPART_4x4_2 );

      break;
    }
    case BLK_4x8:
    {
      xPredLuma(   apcTarBuffer, 4, 8, cMC8x8D, SPART_4x4_0 );
      xPredLuma(   apcTarBuffer, 4, 8, cMC8x8D, SPART_4x4_1 );
      xPredChroma( apcTarBuffer, 2, 4, cMC8x8D, SPART_4x4_0 );
      xPredChroma( apcTarBuffer, 2, 4, cMC8x8D, SPART_4x4_1 );

      break;
    }
    case BLK_4x4:
    {
      xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_0 );
      xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_1 );
      xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_2 );
      xPredLuma(   apcTarBuffer, 4, 4, cMC8x8D, SPART_4x4_3 );
      xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_0 );
      xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_1 );
      xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_2 );
      xPredChroma( apcTarBuffer, 2, 2, cMC8x8D, SPART_4x4_3 );
      break;
    }
    default:
    {
      AF();
      break;
    }
  }

  m_pcSampleWeighting->weightLumaSamples  ( pcRecBuffer, 8, 8, c8x8Idx, cMC8x8D.m_apcPW[LIST_0], cMC8x8D.m_apcPW[LIST_1] );
  m_pcSampleWeighting->weightChromaSamples( pcRecBuffer, 4, 4, c8x8Idx, cMC8x8D.m_apcPW[LIST_0], cMC8x8D.m_apcPW[LIST_1] );
}

ErrVal MotionCompensation::compensateMbBLSkipIntra( MbDataAccess& rcMbDataAccess,
                                                    YuvMbBuffer*  pcRecBuffer,
                                                    Frame*        pcBaseLayerRec )
{
  ROFRS( rcMbDataAccess.getMbData().getBLSkipFlag(),                        Err::m_nOK );
  ROFRS( m_pcResizeParameters,                                              Err::m_nOK );
  ROTRS( m_pcResizeParameters->m_bRefLayerIsMbAffFrame,                     Err::m_nOK );
  ROTRS( m_pcResizeParameters->m_bIsMbAffFrame,                             Err::m_nOK );
  ROTRS( m_pcResizeParameters->getRestrictedSpatialResolutionChangeFlag(),  Err::m_nOK );

  Int iMbX      = rcMbDataAccess.getMbX() * 16;
  Int iMbY      = rcMbDataAccess.getMbY() * 16;
  Int iRefW     = m_pcResizeParameters->m_iRefLayerFrmWidth;
  Int iRefH     = m_pcResizeParameters->m_iRefLayerFrmHeight;
  Int iScaledW  = m_pcResizeParameters->m_iScaledRefFrmWidth;
  Int iScaledH  = m_pcResizeParameters->m_iScaledRefFrmHeight;
  Int iLeftOff  = m_pcResizeParameters->m_iLeftFrmOffset;
  Int iTopOff   = m_pcResizeParameters->m_iTopFrmOffset;
  if( m_pcResizeParameters->m_bFieldPicFlag )
  {
    iScaledH   /= 2;
    iTopOff    /= 2;
  }
  if( m_pcResizeParameters->m_bRefLayerFieldPicFlag )
  {
    iRefH      /= 2;
  }
  Int iShiftX   = ( m_pcResizeParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefW ) );
  Int iShiftY   = ( m_pcResizeParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefH ) );
  Int iScaleX   = ( ( (UInt)iRefW << iShiftX ) + ( iScaledW >> 1 ) ) / iScaledW;
  Int iScaleY   = ( ( (UInt)iRefH << iShiftY ) + ( iScaledH >> 1 ) ) / iScaledH;
  Int iBasePosX0= (Int)( (UInt)( ( iMbX - iLeftOff ) * iScaleX + ( 1 << ( iShiftX - 1 ) ) ) >> iShiftX );
  Int iBasePosY0= (Int)( (UInt)( ( iMbY - iTopOff  ) * iScaleY + ( 1 << ( iShiftY - 1 ) ) ) >> iShiftY );
  iBasePosX0    = gMin( iBasePosX0, iRefW - 1 );
  iBasePosY0    = gMin( iBasePosY0, iRefH - 1 );
  Int iBaseMbX0 = iBasePosX0 >> 4;
  Int iBaseMbY0 = iBasePosY0 >> 4;
  Int iCX       = 0;
  Int iCY       = 0;

  //----- determine first location that points to a different macroblock (not efficient implementation!) -----
  for( ; iCX < 16; iCX++ )
  {
    Int iBasePosX = (Int)( (UInt)( ( iMbX + iCX - iLeftOff ) * iScaleX + ( 1 << ( iShiftX - 1 ) ) ) >> iShiftX );
    iBasePosX     = gMin( iBasePosX, iRefW - 1 );
    Int iBaseMbX  = iBasePosX >> 4;
    if( iBaseMbX  > iBaseMbX0 )
    {
      break;
    }
  }
  for( ; iCY < 16; iCY++ )
  {
    Int iBasePosY = (Int)( (UInt)( ( iMbY + iCY - iTopOff  ) * iScaleY + ( 1 << ( iShiftY - 1 ) ) ) >> iShiftY );
    iBasePosY     = gMin( iBasePosY, iRefH - 1 );
    Int iBaseMbY  = iBasePosY >> 4;
    if( iBaseMbY > iBaseMbY0 )
    {
      break;
    }
  }

  //----- load intra data -----
  YuvMbBuffer cBaseMbRec;
  cBaseMbRec.loadBuffer( pcBaseLayerRec->getFullPelYuvBuffer() );

  //----- copy intra data -----
  MbData& rcMbData = rcMbDataAccess.getMbDataAccessBase()->getMbData();
  if( rcMbData.isBaseIntra( 0, 0 ) )
  {
    RNOK( copyMbBuffer( &cBaseMbRec, pcRecBuffer, 0, 0, iCX, iCY ) );
  }
  if( rcMbData.isBaseIntra( 1, 0 ) && iCX < 16 )
  {
    RNOK( copyMbBuffer( &cBaseMbRec, pcRecBuffer, iCX, 0, 16, iCY ) );
  }
  if( rcMbData.isBaseIntra( 0, 1 ) && iCY < 16 )
  {
    RNOK( copyMbBuffer( &cBaseMbRec, pcRecBuffer, 0, iCY, iCX, 16 ) );
  }
  if( rcMbData.isBaseIntra( 1, 1 ) && iCX < 16 && iCY < 16 )
  {
    RNOK( copyMbBuffer( &cBaseMbRec, pcRecBuffer, iCX, iCY, 16, 16 ) );
  }
  return Err::m_nOK;
}


ErrVal 
MotionCompensation::updateMbBLSkipResidual( MbDataAccess& rcMbDataAccess,
                                            YuvMbBuffer&  rcMbResBuffer )
{
  ROFRS( rcMbDataAccess.getMbData().getBLSkipFlag(),                        Err::m_nOK );
  ROFRS( m_pcResizeParameters,                                              Err::m_nOK );
  ROTRS( m_pcResizeParameters->m_bRefLayerIsMbAffFrame,                     Err::m_nOK );
  ROTRS( m_pcResizeParameters->m_bIsMbAffFrame,                             Err::m_nOK );
  ROTRS( m_pcResizeParameters->getRestrictedSpatialResolutionChangeFlag(),  Err::m_nOK );

  Int iMbX      = rcMbDataAccess.getMbX() * 16;
  Int iMbY      = rcMbDataAccess.getMbY() * 16;
  Int iRefW     = m_pcResizeParameters->m_iRefLayerFrmWidth;
  Int iRefH     = m_pcResizeParameters->m_iRefLayerFrmHeight;
  Int iScaledW  = m_pcResizeParameters->m_iScaledRefFrmWidth;
  Int iScaledH  = m_pcResizeParameters->m_iScaledRefFrmHeight;
  Int iLeftOff  = m_pcResizeParameters->m_iLeftFrmOffset;
  Int iTopOff   = m_pcResizeParameters->m_iTopFrmOffset;
  if( m_pcResizeParameters->m_bFieldPicFlag )
  {
    iScaledH   /= 2;
    iTopOff    /= 2;
  }
  if( m_pcResizeParameters->m_bRefLayerFieldPicFlag )
  {
    iRefH      /= 2;
  }
  Int iShiftX   = ( m_pcResizeParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefW ) );
  Int iShiftY   = ( m_pcResizeParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefH ) );
  Int iScaleX   = ( ( (UInt)iRefW << iShiftX ) + ( iScaledW >> 1 ) ) / iScaledW;
  Int iScaleY   = ( ( (UInt)iRefH << iShiftY ) + ( iScaledH >> 1 ) ) / iScaledH;
  Int iBasePosX0= (Int)( (UInt)( ( iMbX - iLeftOff ) * iScaleX + ( 1 << ( iShiftX - 1 ) ) ) >> iShiftX );
  Int iBasePosY0= (Int)( (UInt)( ( iMbY - iTopOff  ) * iScaleY + ( 1 << ( iShiftY - 1 ) ) ) >> iShiftY );
  iBasePosX0    = gMin( iBasePosX0, iRefW - 1 );
  iBasePosY0    = gMin( iBasePosY0, iRefH - 1 );
  Int iBaseMbX0 = iBasePosX0 >> 4;
  Int iBaseMbY0 = iBasePosY0 >> 4;
  Int iCX       = 0;
  Int iCY       = 0;

  //----- determine first location that points to a different macroblock (not efficient implementation!) -----
  for( ; iCX < 16; iCX++ )
  {
    Int iBasePosX = (Int)( (UInt)( ( iMbX + iCX - iLeftOff ) * iScaleX + ( 1 << ( iShiftX - 1 ) ) ) >> iShiftX );
    iBasePosX     = gMin( iBasePosX, iRefW - 1 );
    Int iBaseMbX  = iBasePosX >> 4;
    if( iBaseMbX > iBaseMbX0 )
    {
      break;
    }
  }
  for( ; iCY < 16; iCY++ )
  {
    Int iBasePosY = (Int)( (UInt)( ( iMbY + iCY - iTopOff  ) * iScaleY + ( 1 << ( iShiftY - 1 ) ) ) >> iShiftY );
    iBasePosY     = gMin( iBasePosY, iRefH - 1 );
    Int iBaseMbY  = iBasePosY >> 4;
    if( iBaseMbY > iBaseMbY0 )
    {
      break;
    }
  }

  //----- copy intra data -----
  MbData& rcMbData = rcMbDataAccess.getMbDataAccessBase()->getMbData();
  if( rcMbData.isBaseIntra( 0, 0 ) )
  {
    RNOK( clearMbBuffer( rcMbResBuffer, 0, 0, iCX, iCY ) );
  }
  if( rcMbData.isBaseIntra( 1, 0 ) && iCX < 16 )
  {
    RNOK( clearMbBuffer( rcMbResBuffer, iCX, 0, 16, iCY ) );
  }
  if( rcMbData.isBaseIntra( 0, 1 ) && iCY < 16 )
  {
    RNOK( clearMbBuffer( rcMbResBuffer, 0, iCY, iCX, 16 ) );
  }
  if( rcMbData.isBaseIntra( 1, 1 ) && iCX < 16 && iCY < 16 )
  {
    RNOK( clearMbBuffer( rcMbResBuffer, iCX, iCY, 16, 16 ) );
  }
  return Err::m_nOK;
}


ErrVal MotionCompensation::copyMbBuffer(  YuvMbBuffer*    pcMbBufSrc,
                                          YuvMbBuffer*    pcMbBufDes,
                                          Int sX, Int sY, Int eX, Int eY)
{
  Int x,y;

  XPel* pSrc;
  XPel* pDes;
  Int   iSrcStride;
  Int   iDesStride;

  if(sX == eX || sY == eY)
    return Err::m_nOK;

  pSrc = pcMbBufSrc->getMbLumAddr();
  pDes = pcMbBufDes->getMbLumAddr();
  iDesStride = pcMbBufDes->getLStride();
  iSrcStride = pcMbBufSrc->getLStride();

  pSrc += sY*iSrcStride;
  pDes += sY*iDesStride;
  for( y = sY; y < eY; y++ )
  {
    for( x = sX; x< eX; x++)
    {
      pDes[x] = pSrc[x];
    }
    pDes += iDesStride;
    pSrc += iSrcStride;
  }

  sX = ( sX + 1 ) >> 1;
  sY = ( sY + 1 ) >> 1;
  eX = ( eX + 1 ) >> 1;
  eY = ( eY + 1 ) >> 1;

  iDesStride = pcMbBufDes->getCStride ();
  iSrcStride = pcMbBufSrc->getCStride ();
  pSrc       = pcMbBufSrc->getMbCbAddr();
  pDes       = pcMbBufDes->getMbCbAddr();

  pSrc += sY * iSrcStride;
  pDes += sY * iDesStride;
  for( y = sY; y < eY; y++ )
  {
    for( x = sX; x < eX; x++ )
    {
      pDes[x] = pSrc[x];
    }
    pDes += iDesStride;
    pSrc += iSrcStride;
  }

  pSrc = pcMbBufSrc->getMbCrAddr();
  pDes = pcMbBufDes->getMbCrAddr();

  pSrc += sY * iSrcStride;
  pDes += sY * iDesStride;
  for( y = sY; y < eY; y++ )
  {
    for( x = sX; x < eX; x++ )
    {
      pDes[x] = pSrc[x];
    }
    pDes += iDesStride;
    pSrc += iSrcStride;
  }

  return Err::m_nOK;
}


ErrVal
MotionCompensation::clearMbBuffer( YuvMbBuffer& rcMbBuffer, Int sX, Int sY, Int eX, Int eY )
{
  ROTRS( sX == eX || sY == eY, Err::m_nOK );

  Int   iStride = rcMbBuffer.getLStride  ();
  XPel* pBuf    = rcMbBuffer.getMbLumAddr() + sY * iStride;
  for( Int y = sY; y < eY; y++, pBuf += iStride )
  {
    for( Int x = sX; x < eX; x++ )
    {
      pBuf[x] = 0;
    }
  }

  sX      = ( sX + 1 ) >> 1;
  sY      = ( sY + 1 ) >> 1;
  eX      = ( eX + 1 ) >> 1;
  eY      = ( eY + 1 ) >> 1;
  iStride = rcMbBuffer.getCStride ();
  pBuf    = rcMbBuffer.getMbCbAddr() + sY * iStride;
  for( Int y = sY; y < eY; y++, pBuf += iStride )
  {
    for( Int x = sX; x < eX; x++ )
    {
      pBuf[x] = 0;
    }
  }

  pBuf    = rcMbBuffer.getMbCrAddr() + sY * iStride;
  for( Int y = sY; y < eY; y++, pBuf += iStride )
  {
    for( Int x = sX; x < eX; x++ )
    {
      pBuf[x] = 0;
    }
  }

  return Err::m_nOK;
}


ErrVal MotionCompensation::compensateMb( MbDataAccess&    rcMbDataAccess,
                                         RefFrameList&    rcRefFrameList0,
                                         RefFrameList&    rcRefFrameList1,
                                         YuvMbBuffer*  pcRecBuffer,
                                         Bool             bCalcMv  )
{
  MbMode eMbMode  = rcMbDataAccess.getMbData().getMbMode();


  switch( eMbMode )
  {
  case MODE_16x16:
    {

      if( bCalcMv )
      {
        xCalc16x16( rcMbDataAccess, NULL );
      }

      MC8x8 cMC8x8D( B_8x8_0 );

      Int       iRefIdx0    = rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx();
      Int       iRefIdx1    = rcMbDataAccess.getMbMotionData( LIST_1 ).getRefIdx();
      Frame* pcRefFrame0 = ( iRefIdx0 > 0 ? rcRefFrameList0[ iRefIdx0 ] : NULL );
      Frame* pcRefFrame1 = ( iRefIdx1 > 0 ? rcRefFrameList1[ iRefIdx1 ] : NULL );
      xGetMbPredData( rcMbDataAccess, pcRefFrame0, pcRefFrame1, cMC8x8D );
      xPredLuma(   pcRecBuffer, 16, 16, cMC8x8D );
      xPredChroma( pcRecBuffer,  8,  8, cMC8x8D );
    }
    break;

  case MODE_16x8:
    {
      if( bCalcMv )
      {
        xCalc16x8( rcMbDataAccess, NULL );
      }

      MC8x8 cMC8x8D0( B_8x8_0 );
      MC8x8 cMC8x8D1( B_8x8_2 );

      Int       iRefIdx0    = rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( PART_16x8_0 );
      Int       iRefIdx1    = rcMbDataAccess.getMbMotionData( LIST_1 ).getRefIdx( PART_16x8_0 );
      Frame* pcRefFrame0 = ( iRefIdx0 > 0 ? rcRefFrameList0[ iRefIdx0 ] : NULL );
      Frame* pcRefFrame1 = ( iRefIdx1 > 0 ? rcRefFrameList1[ iRefIdx1 ] : NULL );
      xGetMbPredData( rcMbDataAccess, pcRefFrame0, pcRefFrame1, cMC8x8D0 );

      iRefIdx0    = rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( PART_16x8_1 );
      iRefIdx1    = rcMbDataAccess.getMbMotionData( LIST_1 ).getRefIdx( PART_16x8_1 );
      pcRefFrame0 = ( iRefIdx0 > 0 ? rcRefFrameList0[ iRefIdx0 ] : NULL );
      pcRefFrame1 = ( iRefIdx1 > 0 ? rcRefFrameList1[ iRefIdx1 ] : NULL );
      xGetMbPredData( rcMbDataAccess, pcRefFrame0, pcRefFrame1, cMC8x8D1 );

      xPredLuma(   pcRecBuffer, 16, 8, cMC8x8D0 );
      xPredLuma(   pcRecBuffer, 16, 8, cMC8x8D1 );
      xPredChroma( pcRecBuffer,  8, 4, cMC8x8D0 );
      xPredChroma( pcRecBuffer,  8, 4, cMC8x8D1 );
    }
    break;

  case MODE_8x16:
    {
      if( bCalcMv )
      {
        xCalc8x16( rcMbDataAccess, NULL );
      }

      MC8x8 cMC8x8D0( B_8x8_0 );
      MC8x8 cMC8x8D1( B_8x8_1 );

      Int       iRefIdx0    = rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( PART_8x16_0 );
      Int       iRefIdx1    = rcMbDataAccess.getMbMotionData( LIST_1 ).getRefIdx( PART_8x16_0 );
      Frame* pcRefFrame0 = ( iRefIdx0 > 0 ? rcRefFrameList0[ iRefIdx0 ] : NULL );
      Frame* pcRefFrame1 = ( iRefIdx1 > 0 ? rcRefFrameList1[ iRefIdx1 ] : NULL );
      xGetMbPredData( rcMbDataAccess, pcRefFrame0, pcRefFrame1, cMC8x8D0 );

      iRefIdx0    = rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( PART_8x16_1 );
      iRefIdx1    = rcMbDataAccess.getMbMotionData( LIST_1 ).getRefIdx( PART_8x16_1 );
      pcRefFrame0 = ( iRefIdx0 > 0 ? rcRefFrameList0[ iRefIdx0 ] : NULL );
      pcRefFrame1 = ( iRefIdx1 > 0 ? rcRefFrameList1[ iRefIdx1 ] : NULL );
      xGetMbPredData( rcMbDataAccess, pcRefFrame0, pcRefFrame1, cMC8x8D1 );

      xPredLuma(   pcRecBuffer, 8, 16, cMC8x8D0 );
      xPredLuma(   pcRecBuffer, 8, 16, cMC8x8D1 );
      xPredChroma( pcRecBuffer, 4,  8, cMC8x8D0 );
      xPredChroma( pcRecBuffer, 4,  8, cMC8x8D1 );
    }
    break;

  case MODE_SKIP:
    {
      if( rcMbDataAccess.getSH().isBSlice() )
      {
        if( bCalcMv )
        {
          RefFrameList* pcL0  = rcMbDataAccess.getSH().getRefFrameList( rcMbDataAccess.getMbPicType(), LIST_0 );
          RefFrameList* pcL1  = rcMbDataAccess.getSH().getRefFrameList( rcMbDataAccess.getMbPicType(), LIST_1 );
          if( rcMbDataAccess.getSH().isH264AVCCompatible() )
          {
            B8x8Idx c8x8Idx1;
            Bool    bOneMv;
            AOF( rcMbDataAccess.getMvPredictorDirect( c8x8Idx1.b8x8(), bOneMv, false, pcL0, pcL1 ) ); c8x8Idx1++;
            AOF( rcMbDataAccess.getMvPredictorDirect( c8x8Idx1.b8x8(), bOneMv, false, pcL0, pcL1 ) ); c8x8Idx1++;
            AOF( rcMbDataAccess.getMvPredictorDirect( c8x8Idx1.b8x8(), bOneMv, false, pcL0, pcL1 ) ); c8x8Idx1++;
            AOF( rcMbDataAccess.getMvPredictorDirect( c8x8Idx1.b8x8(), bOneMv, false, pcL0, pcL1 ) );
          }
          else
          {
            RNOK( rcMbDataAccess.setSVCDirectModeMvAndRef( *pcL0, *pcL1 ) );
          }
        }

        B8x8Idx c8x8Idx;
        RNOK( compensateDirectBlock( rcMbDataAccess, pcRecBuffer, c8x8Idx, rcRefFrameList0, rcRefFrameList1 ) ); c8x8Idx++;
        RNOK( compensateDirectBlock( rcMbDataAccess, pcRecBuffer, c8x8Idx, rcRefFrameList0, rcRefFrameList1 ) ); c8x8Idx++;
        RNOK( compensateDirectBlock( rcMbDataAccess, pcRecBuffer, c8x8Idx, rcRefFrameList0, rcRefFrameList1 ) ); c8x8Idx++;
        RNOK( compensateDirectBlock( rcMbDataAccess, pcRecBuffer, c8x8Idx, rcRefFrameList0, rcRefFrameList1 ) ); ;
      }
      else
      {
//	TMM_EC {{
		if ( rcMbDataAccess.getSH().getErrorConcealMode() == EC_TEMPORAL_DIRECT)
			bCalcMv	=	true;
//  TMM_EC }}
        if( bCalcMv )
        {
          Mv cMvPred;
          rcMbDataAccess.getMvPredictorSkipMode ( cMvPred );
          rcMbDataAccess.getMbMotionData( LIST_0 ).setRefIdx   ( 1 );
          rcMbDataAccess.getMbMotionData( LIST_0 ).setAllMv    ( cMvPred );
          rcMbDataAccess.getMbMotionData( LIST_1 ).setRefIdx   ( 0 );
          rcMbDataAccess.getMbMotionData( LIST_1 ).setAllMv    ( Mv::ZeroMv() );
        }

        MC8x8 cMC8x8D( B_8x8_0 );

        Int       iRefIdx0    = rcMbDataAccess.getMbMotionData( LIST_0 ).getRefIdx( PART_8x16_0 );
        Int       iRefIdx1    = rcMbDataAccess.getMbMotionData( LIST_1 ).getRefIdx( PART_8x16_0 );
        Frame* pcRefFrame0 = ( iRefIdx0 > 0 ? rcRefFrameList0[ iRefIdx0 ] : NULL );
        Frame* pcRefFrame1 = ( iRefIdx1 > 0 ? rcRefFrameList1[ iRefIdx1 ] : NULL );

        xGetMbPredData( rcMbDataAccess, pcRefFrame0, pcRefFrame1, cMC8x8D );

        xPredLuma(   pcRecBuffer, 16, 16, cMC8x8D );
        xPredChroma( pcRecBuffer,  8,  8, cMC8x8D );
        return Err::m_nOK;
      }
    }
    break;

  case MODE_8x8:
  case MODE_8x8ref0:
    printf("function not defined for the case\n");
    RERR();
    break;

  default:
    break;
  }

  return Err::m_nOK;
}

__inline Void MotionCompensation::xGetMbPredData(       MbDataAccess& rcMbDataAccess,
                                                  const Frame*     pcRefFrame0,
                                                  const Frame*     pcRefFrame1,
                                                        MC8x8&    rcMC8x8D )
{
  rcMC8x8D.clear();

  Int iPredCount = 0;

  const SliceHeader& rcSH = rcMbDataAccess.getSH();
  const Frame* apcFrame[2];

  for (Int n = 0; n < 2; n++)
  {
    const Frame* pcRefFrame = n ? pcRefFrame1 : pcRefFrame0;
    ListIdx eLstIdx            = ListIdx(n);

    const MbMvData&     rcMbMvdData = rcMbDataAccess.getMbMvdData( eLstIdx );
    Mv3D& rcMv3D               = rcMC8x8D.m_aacMv[eLstIdx][0];
    rcMbDataAccess.getMbMotionData( eLstIdx ).getMv3D( rcMv3D, rcMC8x8D.m_cIdx );

    rcMC8x8D.m_aacMvd[eLstIdx][SPART_4x4_0] = rcMbMvdData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_0 );
    if( pcRefFrame != NULL )
    {
      iPredCount++;
      rcMv3D.limitComponents( m_cMin, m_cMax );
			rcMC8x8D.m_sChromaOffset[eLstIdx] = xCorrectChromaMv( rcMbDataAccess, pcRefFrame->getPicType() );
      rcMC8x8D.m_apcRefBuffer [eLstIdx] = const_cast<Frame*>(pcRefFrame)->getFullPelYuvBuffer();
      apcFrame[n]                       = pcRefFrame;
      rcMC8x8D.m_apcPW[eLstIdx]         = &rcSH.getPredWeight( eLstIdx, rcMv3D.getRef(), rcMbDataAccess.getMbData().getFieldFlag() ); // TMM_INTERLACE
    }
  }
  if( ( 2 == iPredCount ) && ( 2 == rcSH.getPPS().getWeightedBiPredIdc() ) )
  {
    Int iScale = rcSH.getDistScaleFactorWP( apcFrame[0], apcFrame[1] );
    rcMC8x8D.m_acPW[LIST_1].scaleL1Weight( iScale );
    rcMC8x8D.m_acPW[LIST_0].scaleL0Weight( rcMC8x8D.m_acPW[LIST_1] );
    rcMC8x8D.m_apcPW[LIST_1] = &rcMC8x8D.m_acPW[LIST_1];
    rcMC8x8D.m_apcPW[LIST_0] = &rcMC8x8D.m_acPW[LIST_0];
  }
}



__inline Void MotionCompensation::xGetBlkPredData(       MbDataAccess& rcMbDataAccess,
                                                   const Frame*     pcRefFrame0,
                                                   const Frame*     pcRefFrame1,
                                                         MC8x8&    rcMC8x8D,
                                                         BlkMode       eBlkMode )
{
  rcMC8x8D.clear();

  Int iPredCount = 0;

  const SliceHeader& rcSH = rcMbDataAccess.getSH();
  const Frame* apcFrame[2];

  for( Int n = 0; n < 2; n++)
  {
    const Frame* pcRefFrame = n ? pcRefFrame1 : pcRefFrame0;
    ListIdx eLstIdx            = ListIdx( n );

    Mv3D& rcMv3D                       = rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_0];
    const MbMotionData& rcMbMotionData = rcMbDataAccess.getMbMotionData( eLstIdx );

    const MbMvData&     rcMbMvdData = rcMbDataAccess.getMbMvdData( eLstIdx );
    rcMbMotionData.getMv3D( rcMv3D, rcMC8x8D.m_cIdx + SPART_4x4_0 );
    rcMv3D.limitComponents( m_cMin, m_cMax );

    rcMC8x8D.m_aacMvd[eLstIdx][SPART_4x4_0] = rcMbMvdData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_0 );
    if( pcRefFrame != NULL )
    {
      iPredCount++;
			rcMC8x8D.m_sChromaOffset[eLstIdx] = xCorrectChromaMv( rcMbDataAccess, pcRefFrame->getPicType() );
      rcMC8x8D.m_apcRefBuffer[eLstIdx]  = const_cast<Frame*>(pcRefFrame)->getFullPelYuvBuffer();
      apcFrame[n]                       = pcRefFrame;
      rcMC8x8D.m_apcPW[eLstIdx]         = &rcSH.getPredWeight( eLstIdx, rcMv3D.getRef(), rcMbDataAccess.getMbData().getFieldFlag() ); // TMM_INTERLACE

      switch( eBlkMode )
      {
      case BLK_8x8:
        break;
      case BLK_8x4:
        {
          rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_2] = rcMbMotionData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_2 );
          rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_2].limitComponents( m_cMin, m_cMax );
          rcMC8x8D.m_aacMvd[eLstIdx][SPART_4x4_2] = rcMbMvdData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_2 );
        }
        break;
      case BLK_4x8:
        {
          rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_1] = rcMbMotionData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_1 );
          rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_1].limitComponents( m_cMin, m_cMax );
          rcMC8x8D.m_aacMvd[eLstIdx][SPART_4x4_1] = rcMbMvdData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_1 );
        }
        break;
      case BLK_SKIP:
      case BLK_4x4:
        {
          rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_1] = rcMbMotionData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_1 );
          rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_1].limitComponents( m_cMin, m_cMax );
          rcMC8x8D.m_aacMvd[eLstIdx][SPART_4x4_1] = rcMbMvdData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_1 );

          rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_2] = rcMbMotionData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_2 );
          rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_2].limitComponents( m_cMin, m_cMax );
          rcMC8x8D.m_aacMvd[eLstIdx][SPART_4x4_2] = rcMbMvdData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_2 );

          rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_3] = rcMbMotionData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_3 );
          rcMC8x8D.m_aacMv[eLstIdx][SPART_4x4_3].limitComponents( m_cMin, m_cMax );
          rcMC8x8D.m_aacMvd[eLstIdx][SPART_4x4_3] = rcMbMvdData.getMv( rcMC8x8D.m_cIdx + SPART_4x4_3 );
        }
        break;
      default:
        {
          AF();
        }
      }
    }
  }
  if( ( 2 == iPredCount ) && ( 2 == rcSH.getPPS().getWeightedBiPredIdc() ) )
  {
    Int iScale = rcSH.getDistScaleFactorWP( apcFrame[0], apcFrame[1] );
    rcMC8x8D.m_acPW[LIST_1].scaleL1Weight( iScale );
    rcMC8x8D.m_acPW[LIST_0].scaleL0Weight( rcMC8x8D.m_acPW[LIST_1] );
    rcMC8x8D.m_apcPW[LIST_1] = &rcMC8x8D.m_acPW[LIST_1];
    rcMC8x8D.m_apcPW[LIST_0] = &rcMC8x8D.m_acPW[LIST_0];
  }
}


// scaled by 10-bits
TCoeff aiNormMatrix4x4[16]=
{
  512, 410, 512, 410,
  410, 328, 410, 328,
  512, 410, 512, 410,
  410, 328, 410, 328,
};


TCoeff aiNormMatrix8x8[64] =
{
	512, 454, 819, 454, 512, 454, 819, 454,
	454, 402, 726, 402, 454, 402, 726, 402,
	819, 726, 1311, 726, 819, 726, 1311, 726,
	454, 402, 726, 402, 454, 402, 726, 402,
	512, 454, 819, 454, 512, 454, 819, 454,
	454, 402, 726, 402, 454, 402, 726, 402,
	819, 726, 1311, 726, 819, 726, 1311, 726,
	454, 402, 726, 402, 454, 402, 726, 402,
};


H264AVC_NAMESPACE_END
