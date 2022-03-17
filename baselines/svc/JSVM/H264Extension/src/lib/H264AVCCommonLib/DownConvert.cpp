
#ifdef DOWN_CONVERT_STATIC
#else
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/MbData.h"
#include "H264AVCCommonLib/MbDataCtrl.h"
#include "H264AVCCommonLib/Frame.h"
#include "H264AVCCommonLib/ReconstructionBypass.h"
#endif
#include "DownConvert.h"
#include <math.h>
#include <string.h>


#ifdef DOWN_CONVERT_STATIC
#define  TMM_TABLE_SIZE          512
#define  TMM_FILTER_WINDOW_SIZE  3
#define  NFACT                   12
#define  VALFACT                 (1l<<NFACT)
#define  MASKFACT                (VALFACT-1)
#endif

#ifndef gMax
#define gMax(x,y) ((x)>(y)?(x):(y))
#define gMin(x,y) ((x)<(y)?(x):(y))
#endif


#ifdef DOWN_CONVERT_STATIC
#else
H264AVC_NAMESPACE_BEGIN
#endif



//=================================================
//
//   G E N E R A L   M A I N   F U N C T I O N S
//
//=================================================

DownConvert::DownConvert()
: m_iImageStride            ( 0 )
, m_paiImageBuffer		      ( 0 )
, m_paiTmp1dBuffer		      ( 0 )
#ifdef DOWN_CONVERT_STATIC
, m_padFilter			          ( 0 )
, m_aiTmp1dBufferInHalfpel  ( 0 )
, m_aiTmp1dBufferInQ1pel    ( 0 )
, m_aiTmp1dBufferInQ3pel    ( 0 )
, m_paiTmp1dBufferOut	      ( 0 )
#else
, m_iMbMapStride            ( 0 )
, m_paiTransBlkIdc          ( 0 )
, m_paeMbMapFrm             ( 0 )
, m_paeMbMapFld             ( 0 )
, m_pabIntraUpsAvailableFrm ( 0 )
, m_pabIntraUpsAvailableFld ( 0 )
#endif
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
#ifdef DOWN_CONVERT_STATIC
  m_padFilter               = new long[ TMM_TABLE_SIZE ];
  m_aiTmp1dBufferInHalfpel  = new int [ iMaxDim ];
  m_aiTmp1dBufferInQ1pel    = new int [ iMaxDim ];
  m_aiTmp1dBufferInQ3pel    = new int [ iMaxDim ];
  m_paiTmp1dBufferOut       = new int [ iMaxDim ];
#else
  m_iMbMapStride            = ( iMaxWidth + 15 ) >> 4;
  int iMapSize              = m_iMbMapStride * ( ( iMaxHeight + 15 ) >> 4 );
  m_paiTransBlkIdc          = new int       [ iPicSize ];
  m_paeMbMapFrm             = new MbMapEntry[ iMapSize ];
  m_paeMbMapFld             = new MbMapEntry[ iMapSize ];
  m_pabIntraUpsAvailableFrm = new bool      [ iMapSize ];
  m_pabIntraUpsAvailableFld = new bool      [ iMapSize ];
#endif

  ROFRS( m_paiImageBuffer,          true );
  ROFRS( m_paiTmp1dBuffer,          true );
#ifdef DOWN_CONVERT_STATIC
  ROFRS( m_padFilter,               true );
  ROFRS( m_aiTmp1dBufferInHalfpel,  true );
  ROFRS( m_aiTmp1dBufferInQ1pel,    true );
  ROFRS( m_aiTmp1dBufferInQ3pel,    true );
  ROFRS( m_paiTmp1dBufferOut,       true );
#else
  ROFRS( m_paiTransBlkIdc,          true );
  ROFRS( m_paeMbMapFrm,             true );
  ROFRS( m_paeMbMapFld,             true );
  ROFRS( m_pabIntraUpsAvailableFrm, true );
  ROFRS( m_pabIntraUpsAvailableFld, true );
#endif

#ifdef DOWN_CONVERT_STATIC
  xInitLanczosFilter();
#endif
  return false;
}

void
DownConvert::destroy()
{
  delete this;
}





#ifdef DOWN_CONVERT_STATIC

//===========================================================================
//
//   M A I N   F U N C T I O N S   F O R   D O W N C O N V E R T   T O O L
//
//===========================================================================

void
DownConvert::cropping( unsigned char*    pucBufferY, int iStrideY,
                       unsigned char*    pucBufferU, int iStrideU,
                       unsigned char*    pucBufferV, int iStrideV,
                       ResizeParameters* pcParameters )
{
  int             iOutWidth   = pcParameters->m_iScaledRefFrmWidth;
  int             iOutHeight  = pcParameters->m_iScaledRefFrmHeight;
  int             iPosX       = pcParameters->m_iLeftFrmOffset;
  int             iPosY       = pcParameters->m_iTopFrmOffset;
  int             iGlobWidth  = pcParameters->m_iFrameWidth;
  int             iGlobHeight = pcParameters->m_iFrameHeight;
  unsigned char   cValue      = 128; // mid gray
  unsigned char*  pCropRegion = 0;

  //===== luma =====
  pCropRegion = &pucBufferY[ iPosY * iStrideY + iPosX ];
  xCopyToImageBuffer  ( pCropRegion, iOutWidth,  iOutHeight,  iStrideY         );
  xInitializeWithValue( pucBufferY,  iGlobWidth, iGlobHeight, iStrideY, cValue );
  xCopyFromImageBuffer( pucBufferY,  iOutWidth,  iOutHeight,  iStrideY         );

  //===== parameters for chroma =====
  iOutWidth   >>= 1;
  iOutHeight  >>= 1;
  iPosX       >>= 1;
  iPosY       >>= 1;
  iGlobWidth  >>= 1;
  iGlobHeight >>= 1;

  //===== chroma cb =====
  pCropRegion = &pucBufferU[ iPosY * iStrideU + iPosX ];
  xCopyToImageBuffer  ( pCropRegion, iOutWidth,  iOutHeight,  iStrideU         );
  xInitializeWithValue( pucBufferU,  iGlobWidth, iGlobHeight, iStrideU, cValue );
  xCopyFromImageBuffer( pucBufferU,  iOutWidth,  iOutHeight,  iStrideU         );

  //===== chroma cr =====
  pCropRegion = &pucBufferV[ iPosY * iStrideV + iPosX ];
  xCopyToImageBuffer  ( pCropRegion, iOutWidth,  iOutHeight,  iStrideV         );
  xInitializeWithValue( pucBufferV,  iGlobWidth, iGlobHeight, iStrideV, cValue );
  xCopyFromImageBuffer( pucBufferV,  iOutWidth,  iOutHeight,  iStrideV         );
}


void
DownConvert::upsamplingDyadic( unsigned char*    pucBufferY,   int iStrideY,
                               unsigned char*    pucBufferU,   int iStrideU,
                               unsigned char*    pucBufferV,   int iStrideV,
                               ResizeParameters* pcParameters )
{
  int iWidth  = pcParameters->m_iRefLayerFrmWidth;
  int iHeight = pcParameters->m_iRefLayerFrmHeight;
  int iRatio  = pcParameters->m_iFrameWidth / pcParameters->m_iRefLayerFrmWidth;
  int iStages = 0;
  while( iRatio > 1 )
  {
    iStages++;
    iRatio >>= 1;
  }

  for( int i = iStages; i > 0; i-- )
  {
    //===== luma =====
    xCopyToImageBuffer    ( pucBufferY, iWidth,      iHeight,      iStrideY );
    xCompUpsamplingDyadic (             iWidth,      iHeight,      false    );
    xCopyFromImageBuffer  ( pucBufferY, iWidth << 1, iHeight << 1, iStrideY );
  	//===== chroma cb =====
    xCopyToImageBuffer    ( pucBufferU, iWidth >> 1, iHeight >> 1, iStrideU );
    xCompUpsamplingDyadic (             iWidth >> 1, iHeight >> 1, true     );
    xCopyFromImageBuffer  ( pucBufferU, iWidth,      iHeight,      iStrideU );
    //===== chroma cb =====
    xCopyToImageBuffer    ( pucBufferV, iWidth >> 1, iHeight >> 1, iStrideV );
    xCompUpsamplingDyadic (             iWidth >> 1, iHeight >> 1, true     );
    xCopyFromImageBuffer  ( pucBufferV, iWidth,      iHeight,      iStrideV );
    iWidth  <<= 1;
    iHeight <<= 1;
  }
}


void
DownConvert::upsamplingLanczos( unsigned char*    pucBufferY,   int iStrideY,
                                unsigned char*    pucBufferU,   int iStrideU,
                                unsigned char*    pucBufferV,   int iStrideV,
                                ResizeParameters* pcParameters )
{
  int iInWidth    = pcParameters->m_iRefLayerFrmWidth;
  int iInHeight   = pcParameters->m_iRefLayerFrmHeight;
  int iGlobWidth  = pcParameters->m_iFrameWidth;
  int iGlobHeight = pcParameters->m_iFrameHeight;

  //===== luma =====
  xCopyToImageBuffer    ( pucBufferY,   iInWidth,   iInHeight,   iStrideY );
  xCompUpsamplingLanczos( pcParameters, false );
  xCopyFromImageBuffer  ( pucBufferY,   iGlobWidth, iGlobHeight, iStrideY );
  //===== parameters for chroma =====
  iInWidth    >>= 1;
  iInHeight   >>= 1;
  iGlobWidth  >>= 1;
  iGlobHeight >>= 1;
  //===== chroma cb =====
  xCopyToImageBuffer    ( pucBufferU,   iInWidth,   iInHeight,   iStrideU );
  xCompUpsamplingLanczos( pcParameters, true );
  xCopyFromImageBuffer  ( pucBufferU,   iGlobWidth, iGlobHeight, iStrideU );
  //===== chroma cr =====
  xCopyToImageBuffer    ( pucBufferV,   iInWidth,   iInHeight,   iStrideV );
  xCompUpsamplingLanczos( pcParameters, true );
  xCopyFromImageBuffer  ( pucBufferV,   iGlobWidth, iGlobHeight, iStrideV );
}


void
DownConvert::upsampling6tapBilin( unsigned char*    pucBufferY,   int iStrideY,
                                  unsigned char*    pucBufferU,   int iStrideU,
                                  unsigned char*    pucBufferV,   int iStrideV,
                                  ResizeParameters* pcParameters )
{
  int iInWidth    = pcParameters->m_iRefLayerFrmWidth;
  int iInHeight   = pcParameters->m_iRefLayerFrmHeight;
  int iGlobWidth  = pcParameters->m_iFrameWidth;
  int iGlobHeight = pcParameters->m_iFrameHeight;

  //===== luma =====
  xCopyToImageBuffer      ( pucBufferY,   iInWidth,   iInHeight,   iStrideY );
  xCompUpsampling6tapBilin( pcParameters, false );
  xCopyFromImageBuffer    ( pucBufferY,   iGlobWidth, iGlobHeight, iStrideY );
  //===== parameters for chroma =====
  iInWidth    >>= 1;
  iInHeight   >>= 1;
  iGlobWidth  >>= 1;
  iGlobHeight >>= 1;
  //===== chroma cb =====
  xCopyToImageBuffer      ( pucBufferU,   iInWidth,   iInHeight,   iStrideU );
  xCompUpsampling6tapBilin( pcParameters, true );
  xCopyFromImageBuffer    ( pucBufferU,   iGlobWidth, iGlobHeight, iStrideU );
  //===== chroma cr =====
  xCopyToImageBuffer      ( pucBufferV,   iInWidth,   iInHeight,   iStrideV );
  xCompUpsampling6tapBilin( pcParameters, true );
  xCopyFromImageBuffer    ( pucBufferV,   iGlobWidth, iGlobHeight, iStrideV );
}


void
DownConvert::upsamplingSVC( unsigned char*    pucBufferY,   int   iStrideY,
                            unsigned char*    pucBufferU,   int   iStrideU,
                            unsigned char*    pucBufferV,   int   iStrideV,
                            ResizeParameters* pcParameters, bool  bBotCoincided )
{
  int   iBaseW                  =   pcParameters->m_iRefLayerFrmWidth;
  int   iBaseH                  =   pcParameters->m_iRefLayerFrmHeight;
  int   iCurrW                  =   pcParameters->m_iFrameWidth;
  int   iCurrH                  =   pcParameters->m_iFrameHeight;
  bool  bTopAndBottomResampling = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag == false  &&
                                    pcParameters->m_bRefLayerFieldPicFlag     == false  &&
                                    pcParameters->m_bFrameMbsOnlyFlag         == false  &&
                                    pcParameters->m_bFieldPicFlag             == false    );
  bool  bFrameBasedResampling   = ( pcParameters->m_bFrameMbsOnlyFlag         == true   &&
                                    pcParameters->m_bRefLayerFrameMbsOnlyFlag == true     );
  bool  bBotFieldFrameMbsOnly   = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag == true   &&
                                    pcParameters->m_bFieldPicFlag             == true   &&
                                    pcParameters->m_bBotFieldFlag             == true     );
  bool  bVerticalInterpolation  = ( bBotFieldFrameMbsOnly                     == true   ||
                                   (bFrameBasedResampling                     == false  &&
                                    pcParameters->m_bFieldPicFlag             == false   ));
  bool  bFrameMb                = ( bBotFieldFrameMbsOnly                     == false    );
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

  //==== check bot field coincided parameter for progressive to interlaced resampling =====
  if( pcParameters->m_bRefLayerFrameMbsOnlyFlag && ! pcParameters->m_bFrameMbsOnlyFlag )
  {
    bBotFieldFlag = bBotCoincided;
  }

  //=======================
  //=====   L U M A   =====
  //=======================
  if( bTopAndBottomResampling )
  {
    //===== top field =====
    unsigned char* pFld = pucBufferY;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideY << 1 );
    xCompIntraUpsampling( pcParameters, false,  false,       false, true   );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideY << 1 );

    //===== bottom field =====
    pFld += iStrideY;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideY << 1 );
    xCompIntraUpsampling( pcParameters, false,  true,        false, true   );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideY << 1 );
  }
  else
  {
    unsigned char* pSrc = pucBufferY + iStrideY * iBaseBot;
    unsigned char* pDes = pucBufferY + iStrideY * iCurrBot;
    xCopyToImageBuffer  ( pSrc,         iBaseW, iBaseH >> iBaseField, iStrideY << iBaseField           );
    xCompIntraUpsampling( pcParameters, false,  bBotFieldFlag,        bVerticalInterpolation, bFrameMb );
    xCopyFromImageBuffer( pDes,         iCurrW, iCurrH >> iCurrField, iStrideY << iCurrField           );
  }

  iBaseW >>= 1;
  iBaseH >>= 1;
  iCurrW >>= 1;
  iCurrH >>= 1;

  //===========================
  //=====   C H R O M A   =====
  //===========================
  if( bTopAndBottomResampling )
  {
    //===== top field (U) =====
    unsigned char* pFld = pucBufferU;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideU << 1 );
    xCompIntraUpsampling( pcParameters, true,   false,       false, true   );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideU << 1 );

    //===== bottom field (U) =====
    pFld += iStrideU;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideU << 1 );
    xCompIntraUpsampling( pcParameters, true,   true,        false, true   );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideU << 1 );

    //===== top field (V) =====
    pFld = pucBufferV;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideV << 1 );
    xCompIntraUpsampling( pcParameters, true,   false,       false, true   );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideV << 1 );

    //===== bottom field (V) =====
    pFld += iStrideV;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideV << 1 );
    xCompIntraUpsampling( pcParameters, true,   true,        false, true   );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideV << 1 );
  }
  else
  {
    //===== U =====
    unsigned char* pSrc = pucBufferU + iStrideU * iBaseBot;
    unsigned char* pDes = pucBufferU + iStrideU * iCurrBot;
    xCopyToImageBuffer  ( pSrc,         iBaseW, iBaseH >> iBaseField, iStrideU << iBaseField           );
    xCompIntraUpsampling( pcParameters, true,   bBotFieldFlag,        bVerticalInterpolation, bFrameMb );
    xCopyFromImageBuffer( pDes,         iCurrW, iCurrH >> iCurrField, iStrideU << iCurrField           );

    //===== V =====
    pSrc = pucBufferV + iStrideV * iBaseBot;
    pDes = pucBufferV + iStrideV * iCurrBot;
    xCopyToImageBuffer  ( pSrc,         iBaseW, iBaseH >> iBaseField, iStrideV << iBaseField           );
    xCompIntraUpsampling( pcParameters, true,   bBotFieldFlag,        bVerticalInterpolation, bFrameMb );
    xCopyFromImageBuffer( pDes,         iCurrW, iCurrH >> iCurrField, iStrideV << iCurrField           );
  }
}


void
DownConvert::downsamplingDyadic( unsigned char*    pucBufferY,   int iStrideY,
                                 unsigned char*    pucBufferU,   int iStrideU,
                                 unsigned char*    pucBufferV,   int iStrideV,
                                 ResizeParameters* pcParameters )
{
  int iWidth  = pcParameters->m_iRefLayerFrmWidth;
  int iHeight = pcParameters->m_iRefLayerFrmHeight;
  int iRatio  = pcParameters->m_iRefLayerFrmWidth / pcParameters->m_iFrameWidth;
  int iStages = 0;
  while( iRatio > 1 )
  {
    iStages++;
    iRatio >>= 1;
  }

  for( int i = iStages; i > 0; i-- )
  {
    //===== luma =====
    xCopyToImageBuffer      ( pucBufferY, iWidth,      iHeight,      iStrideY );
    xCompDownsamplingDyadic (             iWidth,      iHeight                );
    xCopyFromImageBuffer    ( pucBufferY, iWidth >> 1, iHeight >> 1, iStrideY );
    //===== chroma cb =====
    xCopyToImageBuffer      ( pucBufferU, iWidth >> 1, iHeight >> 1, iStrideU );
    xCompDownsamplingDyadic (             iWidth >> 1, iHeight >> 1           );
    xCopyFromImageBuffer    ( pucBufferU, iWidth >> 2, iHeight >> 2, iStrideU );
    //===== chroma cr =====
    xCopyToImageBuffer      ( pucBufferV, iWidth >> 1, iHeight >> 1, iStrideV );
    xCompDownsamplingDyadic (             iWidth >> 1, iHeight >> 1           );
    xCopyFromImageBuffer    ( pucBufferV, iWidth >> 2, iHeight >> 2, iStrideV );
    iWidth  >>= 1;
    iHeight >>= 1;
  }
}


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
  if( bTopAndBottomResampling )
  {
    //===== top field =====
    unsigned char* pFld = pucBufferY;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideY << 1 );
    xCompDownsampling   ( pcParameters, false,  false,       false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideY << 1 );

    //===== bottom field =====
    pFld += iStrideY;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideY << 1 );
    xCompDownsampling   ( pcParameters, false,  true,        false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideY << 1 );
  }
  else
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
  if( bTopAndBottomResampling )
  {
    //===== top field (U) =====
    unsigned char* pFld = pucBufferU;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideU << 1 );
    xCompDownsampling   ( pcParameters, true,   false,       false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideU << 1 );

    //===== bottom field (U) =====
    pFld += iStrideU;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideU << 1 );
    xCompDownsampling   ( pcParameters, true,   true,        false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideU << 1 );

    //===== top field (V) =====
    pFld = pucBufferV;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideV << 1 );
    xCompDownsampling   ( pcParameters, true,   false,       false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideV << 1 );

    //===== bottom field (V) =====
    pFld += iStrideV;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideV << 1 );
    xCompDownsampling   ( pcParameters, true,   true,        false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideV << 1 );
  }
  else
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

#else




//=========================================================================
//
//   M A I N   F U N C T I O N S   F O R   E N C O D E R / D E C O D E R
//
//=========================================================================

void
DownConvert::intraUpsampling( Frame*                pcFrame,
                              Frame*                pcBaseFrame,
                              Frame*                pcTempFrame,
                              Frame*                pcTempBaseFrame,
                              ResizeParameters*     pcParameters,
                              MbDataCtrl*           pcMbDataCtrlBase,
                              MbDataCtrl*           pcMbDataCtrlPredFrm,
                              MbDataCtrl*           pcMbDataCtrlPredFld,
                              ReconstructionBypass* pcReconstructionBypass,
                              Bool*                 pabBaseModeAllowedFlagArrayFrm,
                              Bool*                 pabBaseModeAllowedFlagArrayFld,
                              Bool                  bConstrainedIntraResamplingFlag )
{
  Bool    bResampling             = pcParameters->getSpatialResolutionChangeFlag();
  Bool    bConstrainedResampling  = ( bResampling && bConstrainedIntraResamplingFlag );
  Bool    bCropOnly               = ( ! bResampling && pcParameters->getCroppingFlag() );
  PicType ePicType                = ( pcParameters->m_bFieldPicFlag ? ( pcParameters->m_bBotFieldFlag ? BOT_FIELD : TOP_FIELD ) : FRAME );

  if( ! bConstrainedResampling )
  {
    xInitBaseModeAllowedFlags( pcParameters, pabBaseModeAllowedFlagArrayFrm, pabBaseModeAllowedFlagArrayFld );
    if( ! bResampling && ! bCropOnly )
    {
      pcFrame->copy( pcBaseFrame, ePicType );
      return;
    }
    if( bCropOnly )
    {
      xCrop( pcFrame, pcBaseFrame, pcParameters, 128 );
      return;
    }
    pcTempBaseFrame       ->copy        ( pcBaseFrame,     ePicType );
    pcReconstructionBypass->padRecFrame ( pcTempBaseFrame, pcMbDataCtrlBase, pcParameters );
    xIntraUpsampling( pcFrame, pcTempBaseFrame, pcParameters );
    return;
  }

  //===== constrained intra resampling =====
  MyList<unsigned int>  cSliceIdList;

  //--- initialization ---
  pcFrame->setZero();
  xInitBaseModeAllowedFlags ( pcParameters, pabBaseModeAllowedFlagArrayFrm, pabBaseModeAllowedFlagArrayFld );
  xInitIntraUpsAvailFlags   ( pcParameters );
  xInitSliceIdList          ( cSliceIdList, pcParameters, pcMbDataCtrlBase );

  //--- loop over slices ---
  while( !cSliceIdList.empty() )
  {
    unsigned int  uiSliceId = cSliceIdList.popFront();

    //--- basic resampling ---
    pcTempBaseFrame       ->copy        ( pcBaseFrame,     ePicType );
    pcReconstructionBypass->padRecFrame ( pcTempBaseFrame, pcMbDataCtrlBase, pcParameters, uiSliceId );
    xIntraUpsampling( pcTempFrame, pcTempBaseFrame, pcParameters );

    //--- generate mb maps for slice and update intra BL maps ---
    xGenerateMbMapsForSliceId   ( pcParameters, pcMbDataCtrlBase, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld, uiSliceId );
    xUpdateBaseModeAllowedFlags ( pcParameters, pabBaseModeAllowedFlagArrayFrm, pabBaseModeAllowedFlagArrayFld );
    xUpdateIntraUpsAvailFlags   ( pcParameters );

    //--- copy prediction data for current slice id ---
    xUpdateIntraPredFrame( pcFrame, pcTempFrame, pcParameters );
  }
  xUpdateBaseModeFlagsIntraUps( pcParameters, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld, pabBaseModeAllowedFlagArrayFrm, pabBaseModeAllowedFlagArrayFld );
}

void
DownConvert::residualUpsampling( Frame*             pcFrame,
                                 Frame*             pcBaseFrame,
                                 ResizeParameters*  pcParameters,
                                 MbDataCtrl*        pcMbDataCtrlBase )
{
  if( pcParameters->getSpatialResolutionChangeFlag() )
  {
    xResidualUpsampling ( pcFrame, pcBaseFrame, pcParameters, pcMbDataCtrlBase );
  }
  else if( pcParameters->getCroppingFlag() )
  {
    xCrop               ( pcFrame, pcBaseFrame, pcParameters, 0 );
  }
}

#endif




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
#ifdef DOWN_CONVERT_STATIC
  delete [] m_padFilter;
  delete [] m_aiTmp1dBufferInHalfpel;
  delete [] m_aiTmp1dBufferInQ1pel;
  delete [] m_aiTmp1dBufferInQ3pel;
  delete [] m_paiTmp1dBufferOut;
#else
  delete [] m_paiTransBlkIdc;
  delete [] m_paeMbMapFrm;
  delete [] m_paeMbMapFld;
  delete [] m_pabIntraUpsAvailableFrm;
  delete [] m_pabIntraUpsAvailableFld;
#endif
  m_paiImageBuffer          = 0;
  m_paiTmp1dBuffer          = 0;
#ifdef DOWN_CONVERT_STATIC
  m_padFilter               = 0;
  m_aiTmp1dBufferInHalfpel  = 0;
  m_aiTmp1dBufferInQ1pel    = 0;
  m_aiTmp1dBufferInQ3pel    = 0;
  m_paiTmp1dBufferOut       = 0;
#else
  m_paiTransBlkIdc          = 0;
  m_paeMbMapFrm             = 0;
  m_paeMbMapFld             = 0;
  m_pabIntraUpsAvailableFrm = 0;
  m_pabIntraUpsAvailableFld = 0;
#endif
}

int
DownConvert::xClip( int iValue, int imin, int imax )
{
  ROTRS( iValue < imin, imin );
  ROTRS( iValue > imax, imax );
  return iValue;
}


void
DownConvert::xCompIntraUpsampling( ResizeParameters* pcParameters, bool bChroma, bool bBotFlag, bool bVerticalInterpolation, bool bFrameMb, int iMargin )
{
  //===== set general parameters =====
  int   iBotField   = ( bBotFlag ? 1 : 0 );
  int   iFactor     = ( !bChroma ? 1 : 2 );
  int   iRefPhaseX  = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseX );
  int   iRefPhaseY  = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseY );
  int   iPhaseX     = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseX );
  int   iPhaseY     = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseY );
  int   iRefW       = pcParameters->m_iRefLayerFrmWidth   / iFactor;  // reference layer frame width
  int   iRefH       = pcParameters->m_iRefLayerFrmHeight  / iFactor;  // reference layer frame height
  int   iOutW       = pcParameters->m_iScaledRefFrmWidth  / iFactor;  // scaled reference layer frame width
  int   iOutH       = pcParameters->m_iScaledRefFrmHeight / iFactor;  // scaled reference layer frame height
  int   iGlobalW    = pcParameters->m_iFrameWidth         / iFactor;  // current frame width
  int   iGlobalH    = pcParameters->m_iFrameHeight        / iFactor;  // current frame height
  int   iLeftOffset = pcParameters->m_iLeftFrmOffset      / iFactor;  // current left frame offset
  int   iTopOffset  = pcParameters->m_iTopFrmOffset       / iFactor;  // current top  frame offset

  //===== set input/output size =====
  int   iBaseField  = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ? 0 : 1 );
  int   iCurrField  = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag && pcParameters->m_bFrameMbsOnlyFlag ? 0 : 1 );
  int   iBaseW      = iRefW;
  int   iBaseH      = iRefH      >> iBaseField;
  int   iCurrW      = iGlobalW;
  int   iCurrH      = iGlobalH   >> iCurrField;
  int   iLOffset    = iLeftOffset;
  int   iTOffset    = iTopOffset >> iCurrField;
  int   iROffset    = iCurrW - iLOffset -   iOutW;
  int   iBOffset    = iCurrH - iTOffset - ( iOutH >> iCurrField );
  int   iYBorder    = ( bVerticalInterpolation ? ( bChroma ? 1 : 2 ) : 0 );

  //===== set position calculation parameters =====
  int   iScaledW    = iOutW;
  int   iScaledH    = ( ! pcParameters->m_bRefLayerFrameMbsOnlyFlag || pcParameters->m_bFrameMbsOnlyFlag ? iOutH : iOutH / 2 );
  int   iShiftX     = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefW ) );
  int   iShiftY     = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefH ) );
  int   iScaleX     = ( ( (unsigned int)iRefW << iShiftX ) + ( iScaledW >> 1 ) ) / iScaledW;
  int   iScaleY     = ( ( (unsigned int)iRefH << iShiftY ) + ( iScaledH >> 1 ) ) / iScaledH;
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    if( pcParameters->m_bRefLayerFrameMbsOnlyFlag )
    {
      iPhaseY       = iPhaseY + 4 * iBotField + ( 3 - iFactor );
      iRefPhaseY    = 2 * iRefPhaseY + 2;
    }
    else
    {
      iPhaseY       = iPhaseY    + 4 * iBotField;
      iRefPhaseY    = iRefPhaseY + 4 * iBotField;
    }
  }
  Int   iOffsetX    = iLeftOffset;
  Int   iOffsetY    = iTopOffset;
  Int   iAddX       = ( ( ( iRefW * ( 2 + iPhaseX ) ) << ( iShiftX - 2 ) ) + ( iScaledW >> 1 ) ) / iScaledW + ( 1 << ( iShiftX - 5 ) );
  Int   iAddY       = ( ( ( iRefH * ( 2 + iPhaseY ) ) << ( iShiftY - 2 ) ) + ( iScaledH >> 1 ) ) / iScaledH + ( 1 << ( iShiftY - 5 ) );
  Int   iDeltaX     = 4 * ( 2 + iRefPhaseX );
  Int   iDeltaY     = 4 * ( 2 + iRefPhaseY );
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    iOffsetY        = iTopOffset / 2;
    iAddY           = ( ( ( iRefH * ( 2 + iPhaseY ) ) << ( iShiftY - 3 ) ) + ( iScaledH >> 1 ) ) / iScaledH + ( 1 << ( iShiftY - 5 ) );
    iDeltaY         = 2 * ( 2 + iRefPhaseY );
  }

  //===== basic interpolation of a frame or a field =====
  xBasicIntraUpsampling ( iBaseW,   iBaseH,   iCurrW,   iCurrH,
                          iLOffset, iTOffset, iROffset, iBOffset,
                          iShiftX,  iShiftY,  iScaleX,  iScaleY,
                          iOffsetX, iOffsetY, iAddX,    iAddY,
                          iDeltaX,  iDeltaY,  iYBorder, bChroma, iMargin );

  //===== vertical interpolation for second field =====
  if( bVerticalInterpolation )
  {
    xVertIntraUpsampling( iCurrW,   iCurrH,
                          iLOffset, iTOffset, iROffset, iBOffset,
                          iYBorder, bBotFlag, bFrameMb, bChroma );
  }
}


void
DownConvert::xVertIntraUpsampling( int  iBaseW,   int  iBaseH,
                                   int  iLOffset, int  iTOffset, int  iROffset, int  iBOffset,
                                   int  iYBorder, bool bBotFlag, bool bFrameMb, bool bChromaFilter )
{
  AOT( !bChromaFilter && iYBorder < 2 );
  AOT(  bChromaFilter && iYBorder < 1 );

  int iFrameMb      = ( bFrameMb ? 1 : 0 );
  int iBotField     = ( bBotFlag ? 1 : 0 );
  int iCurrW        = iBaseW;
  int iCurrH        = iBaseH   << iFrameMb;
  int iCurrLOffset  = iLOffset;
  int iCurrTOffset  = iTOffset << iFrameMb;
  int iCurrROffset  = iROffset;
  int iCurrBOffset  = iBOffset << iFrameMb;

  //========== vertical upsampling ===========
  for( int j = 0; j < iCurrW; j++ )
  {
    int* piSrc = &m_paiImageBuffer[j];

    //----- upsample column -----
    for( int i = 0; i < iCurrH; i++ )
    {
      if( j < iCurrLOffset || j >= iCurrW - iCurrROffset ||
          i < iCurrTOffset || i >= iCurrH - iCurrBOffset   )
      {
        m_paiTmp1dBuffer[i] = 128; // set to mid gray
        continue;
      }

      if( iFrameMb == 1 && ( i % 2 ) == iBotField )
      {
        int iSrc = ( ( i >> 1 ) + iYBorder ) * m_iImageStride;
        m_paiTmp1dBuffer[i] = piSrc[ iSrc ];
      }
      else
      {
        int iSrc = ( ( i >> iFrameMb ) + iYBorder - iBotField ) * m_iImageStride;
        if( bChromaFilter )
        {
          m_paiTmp1dBuffer[i]   = ( piSrc[ iSrc ] + piSrc[ iSrc + m_iImageStride ] + 1 ) >> 1;
        }
        else
        {
          m_paiTmp1dBuffer[i]   = 16;
          m_paiTmp1dBuffer[i]  += 19 * ( piSrc[ iSrc                  ] + piSrc[ iSrc +     m_iImageStride ] );
          m_paiTmp1dBuffer[i]  -=  3 * ( piSrc[ iSrc - m_iImageStride ] + piSrc[ iSrc + 2 * m_iImageStride ] );
          m_paiTmp1dBuffer[i] >>=  5;
          m_paiTmp1dBuffer[i]   = xClip( m_paiTmp1dBuffer[i], 0, 255 );
        }
      }
    }
    //----- copy back to image buffer -----
    for( int n = 0; n < iCurrH; n++ )
    {
      piSrc[n*m_iImageStride] = m_paiTmp1dBuffer[n];
    }
  }
}


void
DownConvert::xBasicIntraUpsampling( int  iBaseW,   int  iBaseH,   int  iCurrW,   int  iCurrH,
                                    int  iLOffset, int  iTOffset, int  iROffset, int  iBOffset,
                                    int  iShiftX,  int  iShiftY,  int  iScaleX,  int  iScaleY,
                                    int  iOffsetX, int  iOffsetY, int  iAddX,    int  iAddY,
                                    int  iDeltaX,  int  iDeltaY,  int  iYBorder, bool bChromaFilter, int iMargin )
{
  assert( iMargin >= 0 );

  int filter16_luma[16][4] =
  {
    {  0, 32,  0,  0 },
    { -1, 32,  2, -1 },
    { -2, 31,  4, -1 },
    { -3, 30,  6, -1 },
    { -3, 28,  8, -1 },
    { -4, 26, 11, -1 },
    { -4, 24, 14, -2 },
    { -3, 22, 16, -3 },
    { -3, 19, 19, -3 },
    { -3, 16, 22, -3 },
    { -2, 14, 24, -4 },
    { -1, 11, 26, -4 },
    { -1,  8, 28, -3 },
    { -1,  6, 30, -3 },
    { -1,  4, 31, -2 },
    { -1,  2, 32, -1 }
  };
  int filter16_chroma[16][2] =
  {
    { 32,  0 },
    { 30,  2 },
    { 28,  4 },
    { 26,  6 },
    { 24,  8 },
    { 22, 10 },
    { 20, 12 },
    { 18, 14 },
    { 16, 16 },
    { 14, 18 },
    { 12, 20 },
    { 10, 22 },
    {  8, 24 },
    {  6, 26 },
    {  4, 28 },
    {  2, 30 }
  };

  int iShiftXM4 = iShiftX - 4;
  int iShiftYM4 = iShiftY - 4;

  //========== horizontal upsampling ===========
  {
    for( int j = 0; j < iBaseH + 2 * iMargin; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j*m_iImageStride];
      for( int i = 0; i < iCurrW; i++ )
      {
        if( i < iLOffset || i >= iCurrW - iROffset )
        {
          m_paiTmp1dBuffer[i] = 128; // set to mid gray
          continue;
        }

        m_paiTmp1dBuffer[i] = 0;

        int iRefPos16 = (int)( (unsigned int)( ( i - iOffsetX ) * iScaleX + iAddX ) >> iShiftXM4 ) - iDeltaX;
        int iPhase    = iRefPos16 & 15;
        int iRefPos   = iRefPos16 >> 4;

        if( bChromaFilter )
        {
          for( int k = 0; k < 2; k++ )
          {
            int m = xClip( iRefPos + k, -iMargin, iBaseW - 1 + iMargin ) + iMargin;
            m_paiTmp1dBuffer[i] += filter16_chroma[iPhase][k] * piSrc[m];
          }
        }
        else
        {
          for( int k = 0; k < 4; k++ )
          {
            int m = xClip( iRefPos + k - 1, -iMargin, iBaseW - 1 + iMargin ) + iMargin;
            m_paiTmp1dBuffer[i] += filter16_luma[iPhase][k] * piSrc[m];
          }
        }
      }
      //----- copy row back to image buffer -----
      memcpy( piSrc, m_paiTmp1dBuffer, iCurrW*sizeof(int) );
    }
  }

  //========== vertical upsampling ===========
  {
    for( int i = 0; i < iCurrW; i++ )
    {
      int* piSrc = &m_paiImageBuffer[i];
      for( int j = -iYBorder; j < iCurrH+iYBorder; j++ )
      {
        if( i < iLOffset            || i >= iCurrW - iROffset           ||
            j < iTOffset - iYBorder || j >= iCurrH - iBOffset + iYBorder  )
        {
          m_paiTmp1dBuffer[j+iYBorder] = 128; // set to mid gray
          continue;
        }

        m_paiTmp1dBuffer[j+iYBorder] = 0;

        int iPreShift   = ( j - iOffsetY ) * iScaleY + iAddY;
        int iPostShift  = ( j >= iOffsetY ? (int)( (unsigned int)iPreShift >> iShiftYM4 ) : ( iPreShift >> iShiftYM4 ) );
        int iRefPos16   = iPostShift - iDeltaY;
        int iPhase      = iRefPos16 & 15;
        int iRefPos     = iRefPos16 >> 4;

        if( bChromaFilter )
        {
          for( int k = 0; k < 2; k++ )
          {
            int m = xClip( iRefPos + k, -iMargin, iBaseH - 1 + iMargin ) + iMargin;
            m_paiTmp1dBuffer[j+iYBorder] += filter16_chroma[iPhase][k] * piSrc[m*m_iImageStride];
          }
        }
        else
        {
          for( int k = 0; k < 4; k++ )
          {
            int m = xClip( iRefPos + k - 1, -iMargin, iBaseH - 1 + iMargin ) + iMargin;
            m_paiTmp1dBuffer[j+iYBorder] += filter16_luma[iPhase][k] * piSrc[m*m_iImageStride];
          }
        }
        m_paiTmp1dBuffer[j+iYBorder] = ( m_paiTmp1dBuffer[j+iYBorder] + 512 ) >> 10;
      }
      //----- clip and copy back to image buffer -----
      for( int n = 0; n < iCurrH+2*iYBorder; n++ )
      {
        piSrc[n*m_iImageStride] = xClip( m_paiTmp1dBuffer[n], 0, 255 );
      }
    }
  }
}





#ifdef DOWN_CONVERT_STATIC

//===============================================================================
//
//   H E L P E R   F U N C T I O N S   F O R   D O W N C O N V E R T   T O O L
//
//===============================================================================

void
DownConvert::xInitLanczosFilter()
{
  const double pi = 3.14159265359;
  m_padFilter[0]  = VALFACT;
  for( int i = 1; i < TMM_TABLE_SIZE; i++ )
  {
    double  x       = ( (double)i / TMM_TABLE_SIZE ) * TMM_FILTER_WINDOW_SIZE;
    double  pix     = pi * x;
    double  pixw    = pix / TMM_FILTER_WINDOW_SIZE;
    m_padFilter[i]  = (long)( sin( pix ) / pix * sin( pixw ) / pixw * VALFACT );
  }
}

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
DownConvert::xInitializeWithValue( unsigned char* pucBuffer, int iWidth, int iHeight, int iStride, unsigned char cValue )
{
  for( int y = 0; y < iHeight; y++, pucBuffer += iStride )
  {
    ::memset( pucBuffer, cValue, iWidth );
  }
}

void
DownConvert::xCompUpsamplingDyadic( int iBaseW, int iBaseH, bool bChroma )
{
  int aiLumaFilter[6] = { 1, -5, 20, 20, -5, 1 };

  //========== vertical upsampling ===========
  {
    for( int j = 0; j < iBaseW; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j];
      //----- upsample column -----
      for( int i = 0; i < iBaseH; i++)
      {
        m_paiTmp1dBuffer[2*i  ] = piSrc[i*m_iImageStride] << 5;
        m_paiTmp1dBuffer[2*i+1] = 0;

        if( bChroma )
        {
          int m1 = i;
          int m2 = gMin( i + 1, iBaseH - 1 );
          m_paiTmp1dBuffer[2*i+1] += piSrc[m1*m_iImageStride] << 4;
          m_paiTmp1dBuffer[2*i+1] += piSrc[m2*m_iImageStride] << 4;
        }
        else
        {
          for( int k = 0; k < 6; k++ )
          {
            int m = xClip( i + k - 2, 0, iBaseH - 1 );
            m_paiTmp1dBuffer[2*i+1] += aiLumaFilter[k] * piSrc[m*m_iImageStride];
          }
        }
      }
      //----- copy back to image buffer -----
      for( int n = 0; n < 2*iBaseH; n++ )
      {
        piSrc[n*m_iImageStride] = m_paiTmp1dBuffer[n];
      }
    }
  }

  //========== horizontal upsampling ==========
  {
    for( int j = 0; j < 2*iBaseH; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j*m_iImageStride];
      //----- upsample row -----
      for( int i = 0; i < iBaseW; i++)
      {
        m_paiTmp1dBuffer[2*i  ] = piSrc[i] << 5;
        m_paiTmp1dBuffer[2*i+1] = 0;

        if( bChroma )
        {
          int m1 = i;
          int m2 = gMin( i + 1, iBaseW - 1 );
          m_paiTmp1dBuffer[2*i+1] += piSrc[m1] << 4;
          m_paiTmp1dBuffer[2*i+1] += piSrc[m2] << 4;
        }
        else
        {
          for( int k = 0; k < 6; k++ )
          {
            int m = xClip( i + k - 2, 0, iBaseW - 1 );
            m_paiTmp1dBuffer[2*i+1] += aiLumaFilter[k] * piSrc[m];
          }
        }
      }
      //----- round, clip, and copy back to image buffer -----
      for( int n = 0; n < 2*iBaseW; n++ )
      {
        int iS    = ( m_paiTmp1dBuffer[n] + 512 ) >> 10;
        piSrc[n]  = xClip( iS, 0, 255 );
      }
    }
  }
}

void
DownConvert::xCompUpsamplingLanczos( ResizeParameters* pcParameters, bool bChroma )

{
  int   iShift        = ( bChroma ? 1 : 0 );
  int   iInWidth      = pcParameters->m_iRefLayerFrmWidth    >> iShift;
  int   iInHeight     = pcParameters->m_iRefLayerFrmHeight   >> iShift;
  int   iOutWidth     = pcParameters->m_iScaledRefFrmWidth   >> iShift;
  int   iOutHeight    = pcParameters->m_iScaledRefFrmHeight  >> iShift;
  int   iNumerator    = 1;
  int   iDenominator  = 1;
  long  spos          = 0;

  //===== vertical upsampling =====
  xGetNumDenomLanczos( iInHeight, iOutHeight, iNumerator, iDenominator );
  spos = ( 1 << NFACT ) * iDenominator / iNumerator;
  for( int xin = 0; xin < iInWidth; xin++ )
  {
    int* piSrc = &m_paiImageBuffer[xin];
    for( int yin = 0; yin < iInHeight; yin++ )
    {
      m_paiTmp1dBuffer[yin] = piSrc[yin * m_iImageStride];
    }
    xUpsamplingDataLanczos( iInHeight, iOutHeight, spos );
    for( int yout = 0; yout < iOutHeight; yout++ )
    {
      piSrc[yout*m_iImageStride] = m_paiTmp1dBufferOut[yout];
    }
  }

  //===== horizontal upsampling =====
  xGetNumDenomLanczos( iInWidth, iOutWidth, iNumerator, iDenominator );
  spos = ( 1 << NFACT ) * iDenominator / iNumerator;
  for( int yout = 0; yout < iOutHeight; yout++ )
  {
    int* piSrc = &m_paiImageBuffer[yout * m_iImageStride];
    for( int xin = 0; xin < iInWidth; xin++ )
    {
      m_paiTmp1dBuffer[xin] = piSrc[xin];
    }
    xUpsamplingDataLanczos( iInWidth, iOutWidth, spos );
    memcpy( piSrc, m_paiTmp1dBufferOut, iOutWidth*sizeof(int) );
  }
}

void
DownConvert::xUpsamplingDataLanczos( int iInLength, int iOutLength, long spos )
{
  long dpos0 = -spos;
  for( int iout = 0; iout < iOutLength; iout++ )
  {
    dpos0      += spos;
    long  rpos0 = dpos0  & MASKFACT;
    int   ipos0 = dpos0 >> NFACT;
    if(   rpos0 == 0 )
    {
      m_paiTmp1dBufferOut[iout] = m_paiTmp1dBuffer[ipos0];
      continue;
    }
    int   end   = ipos0 + TMM_FILTER_WINDOW_SIZE;
    int   begin = end   - TMM_FILTER_WINDOW_SIZE * 2;
    long  sval  = 0;
    long  posi  = ( ( begin - ipos0 ) << NFACT ) - rpos0;
    for( int i = begin; i <= end; i++, posi += VALFACT )
    {
      long  fact  = xGetFilterLanczos( posi );
      int   m     = xClip( i, 0, iInLength - 1 );
      int   val   = m_paiTmp1dBuffer[m];
      sval       += val * fact;
    }
    m_paiTmp1dBufferOut[iout] = xClip( sval >> NFACT, 0, 255 );
  }
}

void
DownConvert::xGetNumDenomLanczos( int iInWidth, int iOutWidth, int& riNumerator, int& riDenominator )
{
  int iA = 1;
  int iB = iOutWidth;
  int iC = iInWidth;
  while (iC != 0)
  {
    iA = iB;
    iB = iC;
    iC = iA % iB;
  }
  riNumerator   = iOutWidth / iB;
  riDenominator = iInWidth  / iB;
}

long
DownConvert::xGetFilterLanczos( long x )
{
  x      = ( x < 0 ? -x : x );
  int i  = (int)( ( x / TMM_FILTER_WINDOW_SIZE ) * TMM_TABLE_SIZE ) >> NFACT;
  if( i >= TMM_TABLE_SIZE )
  {
    return 0;
  }
  return m_padFilter[ i ];
}

void
DownConvert::xCompUpsampling6tapBilin( ResizeParameters* pcParameters, bool bChroma )
{
  int iShift      = ( bChroma ? 1 : 0 );
  int iInWidth    = pcParameters->m_iRefLayerFrmWidth    >> iShift;
  int iInHeight   = pcParameters->m_iRefLayerFrmHeight   >> iShift;
  int iOutWidth   = pcParameters->m_iScaledRefFrmWidth   >> iShift;
  int iOutHeight  = pcParameters->m_iScaledRefFrmHeight  >> iShift;

  //===== vertical upsampling =====
  for( int xin = 0; xin < iInWidth; xin++ )
  {
    int* piSrc = &m_paiImageBuffer[xin];
    for( int yin = 0; yin < iInHeight; yin++ )
    {
      m_paiTmp1dBuffer[yin] = piSrc[yin * m_iImageStride];
    }
    xUpsamplingData6tapBilin( iInHeight, iOutHeight );
    for( int yout = 0; yout < iOutHeight; yout++ )
    {
      piSrc[yout*m_iImageStride] = m_paiTmp1dBufferOut[yout];
    }
  }

  // ===== horizontal upsampling =====
  for( int yout = 0; yout < iOutHeight; yout++ )
  {
    int* piSrc = &m_paiImageBuffer[yout * m_iImageStride];
    for( int xin = 0; xin < iInWidth; xin++ )
    {
      m_paiTmp1dBuffer[xin] = piSrc[xin];
    }
    xUpsamplingData6tapBilin( iInWidth, iOutWidth );
    for( int i = 0; i < iOutWidth; i++ )
    {
      piSrc[i] = xClip( ( m_paiTmp1dBufferOut[i] + 512 ) >> 10, 0, 255 );
    }
  }
}

void
DownConvert::xUpsamplingData6tapBilin( int iInLength, int iOutLength )
{
  int*  Tmp1dBufferInHalfpel  = m_aiTmp1dBufferInHalfpel;
  int*  Tmp1dBufferInQ1pel    = m_aiTmp1dBufferInQ1pel;
  int*  Tmp1dBufferInQ3pel    = m_aiTmp1dBufferInQ3pel;
  int   x, y, iTemp;

  // half pel samples (6-tap: 1 -5 20 20 -5 1)
  for(  x = 0; x < iInLength; x++ )
  {
    y      = x;
    iTemp  = m_paiTmp1dBuffer[y];
    y      = ( x+1 < iInLength ? x+1 : iInLength-1 );
    iTemp += m_paiTmp1dBuffer[y];
    iTemp  = iTemp << 2;
    y      = ( x-1 >= 0 ? x-1 : 0 );
    iTemp -= m_paiTmp1dBuffer[y];
    y      = ( x+2 < iInLength ? x+2 : iInLength-1 );
    iTemp -= m_paiTmp1dBuffer[y];
    iTemp += iTemp << 2;
    y      = ( x-2 >= 0 ? x-2 : 0);
    iTemp += m_paiTmp1dBuffer[y];
    y      = ( x+3 < iInLength ? x+3 : iInLength-1 );
    iTemp += m_paiTmp1dBuffer[y];
    Tmp1dBufferInHalfpel[x] = iTemp;
  }

  // 1/4 pel samples
  for( x = 0; x < iInLength-1; x++ )
  {
    Tmp1dBufferInQ1pel[x] = ( ( m_paiTmp1dBuffer[x  ] << 5 ) + Tmp1dBufferInHalfpel[x] + 1 ) >> 1;
    Tmp1dBufferInQ3pel[x] = ( ( m_paiTmp1dBuffer[x+1] << 5 ) + Tmp1dBufferInHalfpel[x] + 1 ) >> 1;
  }
  Tmp1dBufferInQ1pel[iInLength-1] = ( ( m_paiTmp1dBuffer[iInLength-1] << 5 ) + Tmp1dBufferInHalfpel[iInLength-1] + 1 ) >> 1;
  Tmp1dBufferInQ3pel[iInLength-1] = Tmp1dBufferInHalfpel[iInLength-1];

  // generic interpolation to nearest 1/4 pel position
  for( int iout = 0; iout < iOutLength; iout++ )
  {
    double  dpos0   = ( (double)iout * iInLength / iOutLength );
    int     ipos0   = (int)dpos0;
    double  rpos0   = dpos0 - ipos0;
    int     iIndex  = (int)( 8 * rpos0 );
    switch( iIndex )
    {
    case 0:
      m_paiTmp1dBufferOut[iout] =  m_paiTmp1dBuffer     [ipos0] << 5; // original pel value
      break;
    case 1:
    case 2:
      m_paiTmp1dBufferOut[iout] =  Tmp1dBufferInQ1pel   [ipos0];      // 1/4 pel value
      break;
    case 3:
    case 4:
      m_paiTmp1dBufferOut[iout] =  Tmp1dBufferInHalfpel [ipos0];      // half pel value
      break;
    case 5:
    case 6:
      m_paiTmp1dBufferOut[iout] =  Tmp1dBufferInQ3pel   [ipos0];      // 1/4 pel value
      break;
    case 7:
      int ipos1                 = ( ipos0 + 1 < iInLength ? ipos0 + 1 : ipos0 );
      m_paiTmp1dBufferOut[iout] =  m_paiTmp1dBuffer     [ipos1] << 5; // original pel value
      break;
    }
  }
}

void
DownConvert::xCompDownsamplingDyadic( int iCurrW, int iCurrH )
{
  int iBaseW        = iCurrW >> 1;
  int iBaseH        = iCurrH >> 1;
  int aiFilter[13]  = { 2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2 };

  //========== horizontal downsampling ===========
  {
    for( int j = 0; j < iCurrH; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j*m_iImageStride];
      //----- down sample row -----
      for( int i = 0; i < iBaseW; i++ )
      {
        m_paiTmp1dBuffer[i] = 0;
        for( int k = 0; k < 13; k++ )
        {
          int m = xClip( 2*i + k - 6, 0, iCurrW - 1 );
          m_paiTmp1dBuffer[i] += aiFilter[k] * piSrc[m];
        }
      }
      //----- copy row back to image buffer -----
      memcpy( piSrc, m_paiTmp1dBuffer, iBaseW*sizeof(int) );
    }
  }

  //=========== vertical downsampling ===========
  {
    for( int j = 0; j < iBaseW; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j];
      //----- down sample column -----
      for( int i = 0; i < iBaseH; i++ )
      {
        m_paiTmp1dBuffer[i] = 0;
        for( int k = 0; k < 13; k++ )
        {
          int m = xClip( 2*i + k - 6, 0, iCurrH - 1 );
          m_paiTmp1dBuffer[i] += aiFilter[k] * piSrc[m*m_iImageStride];
        }
      }
      //----- scale, clip, and copy back to image buffer -----
      for( int n = 0; n < iBaseH; n++ )
      {
        int iS                  = ( m_paiTmp1dBuffer[n] + 2048 ) >> 12;
        piSrc[n*m_iImageStride] = xClip( iS, 0, 255 );
      }
    }
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
      iRefPhaseY    = 2 * iRefPhaseY + 2;
    }
    else
    {
      iPhaseY       = iPhaseY    + 4 * iBotField;
      iRefPhaseY    = iRefPhaseY + 4 * iBotField;
    }
  }
  int   iAddX       = ( ( ( iScaledW * ( 2 + iRefPhaseX ) ) << ( iShiftX - 2 ) ) + ( iRefW >> 1 ) ) / iRefW + ( 1 << ( iShiftX - 5 ) );
  int   iAddY       = ( ( ( iScaledH * ( 2 + iRefPhaseY ) ) << ( iShiftY - 2 ) ) + ( iRefH >> 1 ) ) / iRefH + ( 1 << ( iShiftY - 5 ) );
  int   iDeltaX     = 4 * ( 2 + iPhaseX ) - ( iLeftOffset << 4 );
  int   iDeltaY     = 4 * ( 2 + iPhaseY ) - ( iTopOffset  << 4 );
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    iAddY           = ( ( ( iScaledH * ( 2 + iRefPhaseY ) ) << ( iShiftY - 3 ) ) + ( iRefH >> 1 ) ) / iRefH + ( 1 << ( iShiftY - 5 ) );
    iDeltaY         = 2 * ( 2 + iPhaseY ) - ( iTopOffset  << 3 );
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
      {   2,   0, -10,   0,  40,  64,  40,   0, -10,   0,   2,   0 },
      {   2,   1,  -9,  -2,  37,  64,  42,   2, -10,  -1,   2,   0 },
      {   2,   1,  -9,  -3,  34,  64,  44,   4, -10,  -1,   2,   0 },
      {   2,   1,  -8,  -5,  31,  63,  47,   6, -10,  -2,   3,   0 },
      {   1,   2,  -8,  -6,  29,  62,  49,   8, -10,  -2,   3,   0 },
      {   1,   2,  -7,  -7,  26,  61,  52,  10, -10,  -3,   3,   0 },
      {   1,   2,  -6,  -8,  23,  60,  54,  13, -10,  -4,   3,   0 },
      {   1,   2,  -6,  -9,  20,  59,  56,  15, -10,  -4,   3,   1 },
      {   1,   2,  -5,  -9,  18,  57,  57,  18,  -9,  -5,   2,   1 },
      {   1,   3,  -4, -10,  15,  56,  59,  20,  -9,  -6,   2,   1 },
      {   0,   3,  -4, -10,  13,  54,  60,  23,  -8,  -6,   2,   1 },
      {   0,   3,  -3, -10,  10,  52,  61,  26,  -7,  -7,   2,   1 },
      {   0,   3,  -2, -10,   8,  49,  62,  29,  -6,  -8,   2,   1 },
      {   0,   3,  -2, -10,   6,  47,  63,  31,  -5,  -8,   1,   2 },
      {   0,   2,  -1, -10,   4,  44,  64,  34,  -3,  -9,   1,   2 },
      {   0,   2,  -1, -10,   2,  42,  64,  37,  -2,  -9,   1,   2 }
    },
    { // D = 2.5
      {   0,  -4,  -7,  11,  38,  52,  38,  11,  -7,  -4,   0,   0 },
      {   0,  -4,  -7,   9,  37,  51,  40,  13,  -6,  -7,   2,   0 },
      {   0,  -3,  -7,   8,  35,  51,  41,  14,  -5,  -7,   1,   0 },
      {   0,  -2,  -8,   6,  33,  51,  42,  16,  -5,  -7,   2,   0 },
      {   0,  -2,  -8,   5,  32,  50,  43,  18,  -4,  -8,   2,   0 },
      {   0,  -2,  -8,   4,  30,  50,  45,  19,  -3,  -8,   1,   0 },
      {   0,  -1,  -8,   2,  28,  49,  46,  21,  -2,  -8,   1,   0 },
      {   0,  -1,  -8,   1,  26,  49,  47,  23,  -1,  -8,   0,   0 },
      {   0,   0,  -8,   0,  24,  48,  48,  24,   0,  -8,   0,   0 },
      {   0,   0,  -8,  -1,  23,  47,  49,  26,   1,  -8,  -1,   0 },
      {   0,   1,  -8,  -2,  21,  46,  49,  28,   2,  -8,  -1,   0 },
      {   0,   1,  -8,  -3,  19,  45,  50,  30,   4,  -8,  -2,   0 },
      {   0,   2,  -8,  -4,  18,  43,  50,  32,   5,  -8,  -2,   0 },
      {   0,   2,  -7,  -5,  16,  42,  51,  33,   6,  -8,  -2,   0 },
      {   0,   1,  -7,  -5,  14,  41,  51,  35,   8,  -7,  -3,   0 },
      {   0,   2,  -7,  -6,  13,  40,  51,  37,   9,  -7,  -4,   0 }
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

#else




//==============================================================================
//
//   H E L P E R    F U N C T I O N S   F O R   E N C O D E R / D E C O D E R
//
//==============================================================================

void
DownConvert::xCopyToImageBuffer( const short* psSrc, int iWidth, int iHeight, int iStride, int iMargin )
{
  iWidth    += iMargin * 2;
  iHeight   += iMargin * 2;
  psSrc     -= iMargin * iStride + iMargin;
  int* piDes = m_paiImageBuffer;
  for( int j = 0; j < iHeight; j++ )
  {
    for( int i = 0; i < iWidth;  i++ )
    {
      piDes[i] = (int)psSrc[i];
    }
    piDes += m_iImageStride;
    psSrc += iStride;
  }
}

void
DownConvert::xCopyFromImageBuffer( short* psDes, int iWidth, int iHeight, int iStride )
{
  int* piSrc = m_paiImageBuffer;
  for( int j = 0; j < iHeight; j++ )
  {
    for( int i = 0; i < iWidth;  i++ )
    {
      psDes[i] = (short)piSrc[i];
    }
    psDes += iStride;
    piSrc += m_iImageStride;
  }
}

void
DownConvert::xInitializeWithValue( short* psBuffer, int iWidth, int iHeight, int iStride, short iValue )
{
  for( int y = 0; y < iHeight; y++, psBuffer += iStride )
  {
    for( int x = 0; x < iWidth; x++ )
    {
      psBuffer[x] = iValue;
    }
  }
}

void
DownConvert::xCrop( Frame*            pcFrame,
                    Frame*            pcBaseFrame,
                    ResizeParameters* pcParameters,
                    short             iValue )
{
  pcFrame    ->getFullPelYuvBuffer()->getYuvBufferCtrl().initMb();
  pcBaseFrame->getFullPelYuvBuffer()->getYuvBufferCtrl().initMb();

  short*        psDesY      = pcFrame->getFullPelYuvBuffer()->getMbLumAddr();
  short*        psDesU      = pcFrame->getFullPelYuvBuffer()->getMbCbAddr ();
  short*        psDesV      = pcFrame->getFullPelYuvBuffer()->getMbCrAddr ();
  int           iDesStrideY = pcFrame->getFullPelYuvBuffer()->getLStride  ();
  int           iDesStrideC = pcFrame->getFullPelYuvBuffer()->getCStride  ();
  const short*  psSrcY      = pcBaseFrame->getFullPelYuvBuffer()->getMbLumAddr();
  const short*  psSrcU      = pcBaseFrame->getFullPelYuvBuffer()->getMbCbAddr ();
  const short*  psSrcV      = pcBaseFrame->getFullPelYuvBuffer()->getMbCrAddr ();
  int           iSrcStrideY = pcBaseFrame->getFullPelYuvBuffer()->getLStride  ();
  int           iSrcStrideC = pcBaseFrame->getFullPelYuvBuffer()->getCStride  ();

  int           iSrcPosX    = gMax(0, -pcParameters->m_iLeftFrmOffset);
  int           iSrcPosY    = gMax(0, -pcParameters->m_iTopFrmOffset);
  int           iPosX       = gMax(0,  pcParameters->m_iLeftFrmOffset);
  int           iPosY       = gMax(0,  pcParameters->m_iTopFrmOffset);
  int           iOutWidth   = gMin(pcParameters->m_iScaledRefFrmWidth  - iSrcPosX, pcParameters->m_iFrameWidth  - iPosX);
  int           iOutHeight  = gMin(pcParameters->m_iScaledRefFrmHeight - iSrcPosY, pcParameters->m_iFrameHeight - iPosY);
  int           iGlobWidth  = pcParameters->m_iFrameWidth;
  int           iGlobHeight = pcParameters->m_iFrameHeight;

  //===== luma =====
  psSrcY += iSrcPosY * iSrcStrideY + iSrcPosX;
  xCopyToImageBuffer  ( psSrcY, iOutWidth,  iOutHeight,  iSrcStrideY         );
  xInitializeWithValue( psDesY, iGlobWidth, iGlobHeight, iDesStrideY, iValue );
  psDesY += iPosY * iDesStrideY + iPosX;
  xCopyFromImageBuffer( psDesY, iOutWidth,  iOutHeight,  iDesStrideY         );

  //===== parameters for chroma =====
  iOutWidth   >>= 1;
  iOutHeight  >>= 1;
  iSrcPosX    >>= 1;
  iSrcPosY    >>= 1;
  iPosX       >>= 1;
  iPosY       >>= 1;
  iGlobWidth  >>= 1;
  iGlobHeight >>= 1;

  //===== chroma cb =====
  psSrcU += iSrcPosY * iSrcStrideC + iSrcPosX;
  xCopyToImageBuffer  ( psSrcU, iOutWidth,  iOutHeight,  iSrcStrideC         );
  xInitializeWithValue( psDesU, iGlobWidth, iGlobHeight, iDesStrideC, iValue );
  psDesU += iPosY * iDesStrideC + iPosX;
  xCopyFromImageBuffer( psDesU, iOutWidth,  iOutHeight,  iDesStrideC         );

  //===== chroma cr =====
  psSrcV += iSrcPosY * iSrcStrideC + iSrcPosX;
  xCopyToImageBuffer  ( psSrcV, iOutWidth,  iOutHeight,  iSrcStrideC         );
  xInitializeWithValue( psDesV, iGlobWidth, iGlobHeight, iDesStrideC, iValue );
  psDesV += iPosY * iDesStrideC + iPosX;
  xCopyFromImageBuffer( psDesV, iOutWidth,  iOutHeight,  iDesStrideC         );
}


void
DownConvert::xIntraUpsampling( Frame*             pcFrame,
                               Frame*             pcBaseFrame,
                               ResizeParameters*  pcParameters )
{
  pcFrame    ->getFullPelYuvBuffer()->getYuvBufferCtrl().initMb();
  pcBaseFrame->getFullPelYuvBuffer()->getYuvBufferCtrl().initMb();

  short*        psDesY                  =   pcFrame->getFullPelYuvBuffer()->getMbLumAddr();
  short*        psDesU                  =   pcFrame->getFullPelYuvBuffer()->getMbCbAddr ();
  short*        psDesV                  =   pcFrame->getFullPelYuvBuffer()->getMbCrAddr ();
  int           iDesStrideY             =   pcFrame->getFullPelYuvBuffer()->getLStride  ();
  int           iDesStrideC             =   pcFrame->getFullPelYuvBuffer()->getCStride  ();
  const short*  psSrcY                  =   pcBaseFrame->getFullPelYuvBuffer()->getMbLumAddr();
  const short*  psSrcU                  =   pcBaseFrame->getFullPelYuvBuffer()->getMbCbAddr ();
  const short*  psSrcV                  =   pcBaseFrame->getFullPelYuvBuffer()->getMbCrAddr ();
  int           iSrcStrideY             =   pcBaseFrame->getFullPelYuvBuffer()->getLStride  ();
  int           iSrcStrideC             =   pcBaseFrame->getFullPelYuvBuffer()->getCStride  ();

  int           iBaseW                  =   pcParameters->m_iRefLayerFrmWidth;
  int           iBaseH                  =   pcParameters->m_iRefLayerFrmHeight;
  int           iCurrW                  =   pcParameters->m_iFrameWidth;
  int           iCurrH                  =   pcParameters->m_iFrameHeight;
  bool          bTopAndBottomResampling = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag == false  &&
                                            pcParameters->m_bRefLayerFieldPicFlag     == false  &&
                                            pcParameters->m_bFrameMbsOnlyFlag         == false  &&
                                            pcParameters->m_bFieldPicFlag             == false    );
  bool          bFrameBasedResampling   = ( pcParameters->m_bFrameMbsOnlyFlag         == true   &&
                                            pcParameters->m_bRefLayerFrameMbsOnlyFlag == true     );
  bool          bBotFieldFrameMbsOnly   = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag == true   &&
                                            pcParameters->m_bFieldPicFlag             == true   &&
                                            pcParameters->m_bBotFieldFlag             == true     );
  bool          bVerticalInterpolation  = ( bBotFieldFrameMbsOnly                     == true   ||
                                           (bFrameBasedResampling                     == false  &&
                                            pcParameters->m_bFieldPicFlag             == false   ));
  bool          bFrameMb                = ( bBotFieldFrameMbsOnly                     == false    );
  bool          bCurrBotField           = ( pcParameters->m_bFieldPicFlag             == true   &&
                                            pcParameters->m_bBotFieldFlag             == true     );
  bool          bBotFieldFlag           = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ?  false
                                          : pcParameters->m_bFieldPicFlag             ?  pcParameters->m_bBotFieldFlag
                                          : pcParameters->m_bRefLayerFieldPicFlag     ?  pcParameters->m_bRefLayerBotFieldFlag
                                          : false );
  int           iBaseField              = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ?  0 : 1 );
  int           iCurrField              = ( pcParameters->m_bFieldPicFlag             ?  1 : 0 );
  int           iBaseBot                = ( bBotFieldFlag ? 1 : 0 );
  int           iCurrBot                = ( bCurrBotField ? 1 : 0 );
  int           iMargin                 = 16;

  //=======================
  //=====   L U M A   =====
  //=======================
  if( bTopAndBottomResampling )
  {
    //===== top field =====
    const short*  pSrcFld = psSrcY;
    short*        pDesFld = psDesY;
    xCopyToImageBuffer  ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideY << 1, iMargin );
    xCompIntraUpsampling( pcParameters, false,  false,       false, true,      iMargin );
    xCopyFromImageBuffer( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideY << 1 );

    //===== bottom field =====
    pSrcFld += iSrcStrideY;
    pDesFld += iDesStrideY;
    xCopyToImageBuffer  ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideY << 1, iMargin );
    xCompIntraUpsampling( pcParameters, false,  true,        false, true,      iMargin );
    xCopyFromImageBuffer( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideY << 1 );
  }
  else
  {
    const short*  pSrc = psSrcY + iSrcStrideY * iBaseBot;
    short*        pDes = psDesY + iDesStrideY * iCurrBot;
    xCopyToImageBuffer  ( pSrc,         iBaseW, iBaseH >> iBaseField, iSrcStrideY << iBaseField,        iMargin );
    xCompIntraUpsampling( pcParameters, false,  bBotFieldFlag,        bVerticalInterpolation, bFrameMb, iMargin );
    xCopyFromImageBuffer( pDes,         iCurrW, iCurrH >> iCurrField, iDesStrideY << iCurrField );
  }

  iBaseW  >>= 1;
  iBaseH  >>= 1;
  iCurrW  >>= 1;
  iCurrH  >>= 1;
  iMargin >>= 1;

  //===========================
  //=====   C H R O M A   =====
  //===========================
  if( bTopAndBottomResampling )
  {
    //===== top field (U) =====
    const short*  pSrcFld = psSrcU;
    short*        pDesFld = psDesU;
    xCopyToImageBuffer  ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideC << 1, iMargin );
    xCompIntraUpsampling( pcParameters, true,   false,       false, true,      iMargin );
    xCopyFromImageBuffer( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideC << 1 );

    //===== bottom field (U) =====
    pSrcFld += iSrcStrideC;
    pDesFld += iDesStrideC;
    xCopyToImageBuffer  ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideC << 1, iMargin );
    xCompIntraUpsampling( pcParameters, true,   true,        false, true,      iMargin );
    xCopyFromImageBuffer( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideC << 1 );

    //===== top field (V) =====
    pSrcFld = psSrcV;
    pDesFld = psDesV;
    xCopyToImageBuffer  ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideC << 1, iMargin );
    xCompIntraUpsampling( pcParameters, true,   false,       false, true,      iMargin );
    xCopyFromImageBuffer( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideC << 1 );

    //===== bottom field (V) =====
    pSrcFld += iSrcStrideC;
    pDesFld += iDesStrideC;
    xCopyToImageBuffer  ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideC << 1, iMargin );
    xCompIntraUpsampling( pcParameters, true,   true,        false, true,      iMargin );
    xCopyFromImageBuffer( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideC << 1 );
  }
  else
  {
    //===== U =====
    const short*  pSrc = psSrcU + iSrcStrideC * iBaseBot;
    short*        pDes = psDesU + iDesStrideC * iCurrBot;
    xCopyToImageBuffer  ( pSrc,         iBaseW, iBaseH >> iBaseField, iSrcStrideC << iBaseField,        iMargin );
    xCompIntraUpsampling( pcParameters, true,   bBotFieldFlag,        bVerticalInterpolation, bFrameMb, iMargin );
    xCopyFromImageBuffer( pDes,         iCurrW, iCurrH >> iCurrField, iDesStrideC << iCurrField );

    //===== V =====
    pSrc = psSrcV + iSrcStrideC * iBaseBot;
    pDes = psDesV + iDesStrideC * iCurrBot;
    xCopyToImageBuffer  ( pSrc,         iBaseW, iBaseH >> iBaseField, iSrcStrideC << iBaseField,        iMargin );
    xCompIntraUpsampling( pcParameters, true,   bBotFieldFlag,        bVerticalInterpolation, bFrameMb, iMargin );
    xCopyFromImageBuffer( pDes,         iCurrW, iCurrH >> iCurrField, iDesStrideC << iCurrField );
  }
}


void
DownConvert::xInitSliceIdList( MyList<unsigned int>& rcSliceIdList, ResizeParameters* pcParameters, MbDataCtrl* pcMbDataCtrl )
{
  rcSliceIdList.clear();

  int iField          = ( pcParameters->m_bRefLayerFieldPicFlag ? 1 : 0 );
  int iBotField       = ( pcParameters->m_bRefLayerBotFieldFlag ? 1 : 0 );
  int iPicWidthInMbs  = ( pcParameters->m_iRefLayerFrmWidth  >> 4 );
  int iPicHeightInMbs = ( pcParameters->m_iRefLayerFrmHeight >> 4 ) >> iField;
  int iMbStride       = iPicWidthInMbs << iField;
  int iMbOffset       = iPicWidthInMbs  * iBotField;

  for( int iMbY = 0; iMbY < iPicHeightInMbs; iMbY++ )
  for( int iMbX = 0; iMbX < iPicWidthInMbs;  iMbX++ )
  {
    int           iMbIdx    = iMbY * iMbStride + iMbX + iMbOffset;
    MbData&       rcMbData  = pcMbDataCtrl->getMbDataByIndex( (unsigned int)iMbIdx );
    unsigned int  uiSliceId = rcMbData.getSliceId();
    bool          bPresent  = false;

    MyList<unsigned int>::iterator  cIter = rcSliceIdList.begin();
    MyList<unsigned int>::iterator  cEnd  = rcSliceIdList.end  ();
    for( ; ! bPresent && cIter != cEnd; cIter++ )
    {
      bPresent = ( (*cIter) == uiSliceId );
    }

    if( ! bPresent )
    {
      rcSliceIdList.push_back( uiSliceId );
    }
  }
}

void
DownConvert::xInitBaseModeAllowedFlags( ResizeParameters* pcParameters,
                                        bool*             pabBaseModeAllowedFlagArrayFrm,
                                        bool*             pabBaseModeAllowedFlagArrayFld )
{
  int iFrmSizeInMbs = ( pcParameters->m_iFrameWidth * pcParameters->m_iFrameHeight ) >> 8;
  int iField        = ( pcParameters->m_bFieldPicFlag ? 1 : 0 );

  if( pabBaseModeAllowedFlagArrayFrm )
  {
    for( int iMbAddr = 0; iMbAddr < iFrmSizeInMbs; iMbAddr++ )
    {
      pabBaseModeAllowedFlagArrayFrm[ iMbAddr ] = ! pcParameters->m_bFieldPicFlag;
    }
  }
  if( pabBaseModeAllowedFlagArrayFld )
  {
    if( ! pcParameters->m_bFieldPicFlag )
    {
      for( int iMbAddr = 0; iMbAddr < iFrmSizeInMbs; iMbAddr++ )
      {
        pabBaseModeAllowedFlagArrayFld[ iMbAddr ] = pcParameters->m_bIsMbAffFrame;
      }
    }
    else
    {
      for( int iMbAddr = 0; iMbAddr < iFrmSizeInMbs; iMbAddr++ )
      {
        pabBaseModeAllowedFlagArrayFld[ iMbAddr ] = ( iMbAddr < ( iFrmSizeInMbs >> iField ) );
      }
    }
  }
}


void
DownConvert::xUpdateBaseModeAllowedFlags( ResizeParameters* pcParameters,
                                          bool*             pabBaseModeAllowedFlagArrayFrm,
                                          bool*             pabBaseModeAllowedFlagArrayFld )
{
  int iFrmSizeInMbs = ( pcParameters->m_iFrameWidth * pcParameters->m_iFrameHeight ) >> 8;
  int iMbStride     = ( pcParameters->m_iFrameWidth >> 4 );
  int iField        = ( pcParameters->m_bFieldPicFlag ? 1 : 0 );
  int iBotField     = ( pcParameters->m_bBotFieldFlag ? 1 : 0 );

  if( pabBaseModeAllowedFlagArrayFrm && ! pcParameters->m_bFieldPicFlag )
  {
    for( int iMbIdx = 0; iMbIdx < iFrmSizeInMbs; iMbIdx++ )
    {
      int iMbX      = ( iMbIdx % iMbStride );
      int iMbY      = ( iMbIdx / iMbStride );
      int iMbMapIdx = iMbY * m_iMbMapStride + iMbX;
      int iMbAddr   = iMbIdx;
      if( pcParameters->m_bIsMbAffFrame )
      {
        iMbAddr = ( ( ( iMbY >> 1 ) * iMbStride + iMbX ) << 1 ) + ( iMbY % 2 );
      }
      if( ( m_paeMbMapFrm[ iMbMapIdx ] & BASE_MODE_ALLOWED ) == INVALID_ENTRY )
      {
        pabBaseModeAllowedFlagArrayFrm[ iMbAddr ] = false;
      }
    }
  }

  if( pabBaseModeAllowedFlagArrayFld && ( pcParameters->m_bFieldPicFlag || pcParameters->m_bIsMbAffFrame ) )
  {
    for( int iMbIdx = 0; iMbIdx < ( iFrmSizeInMbs >> iField ); iMbIdx++ )
    {
      int iMbX      =   ( iMbIdx % iMbStride );
      int iMbY      = ( ( iMbIdx / iMbStride ) << iField ) + iBotField;
      int iMbMapIdx = iMbY * m_iMbMapStride + iMbX;
      int iMbAddr   = iMbIdx;
      if( pcParameters->m_bIsMbAffFrame )
      {
        iMbAddr = ( ( ( iMbY >> 1 ) * iMbStride + iMbX ) << 1 ) + ( iMbY % 2 );
      }
      if( ( m_paeMbMapFld[ iMbMapIdx ] & BASE_MODE_ALLOWED ) == INVALID_ENTRY )
      {
        pabBaseModeAllowedFlagArrayFld[ iMbAddr ] = false;
      }
    }
  }
}


void
DownConvert::xInitIntraUpsAvailFlags( ResizeParameters* pcParameters )
{
  int iFrmSizeInMbs = ( pcParameters->m_iFrameWidth * pcParameters->m_iFrameHeight ) >> 8;
  for( int iMbAddr  = 0; iMbAddr < iFrmSizeInMbs; iMbAddr++ )
  {
    m_pabIntraUpsAvailableFrm[ iMbAddr ] = false;
    m_pabIntraUpsAvailableFld[ iMbAddr ] = false;
  }
}


void
DownConvert::xUpdateIntraUpsAvailFlags( ResizeParameters* pcParameters )
{
  int iFrmSizeInMbs = ( pcParameters->m_iFrameWidth * pcParameters->m_iFrameHeight ) >> 8;
  int iMbStride     = ( pcParameters->m_iFrameWidth >> 4 );
  int iField        = ( pcParameters->m_bFieldPicFlag ? 1 : 0 );
  int iBotField     = ( pcParameters->m_bBotFieldFlag ? 1 : 0 );

  if( ! pcParameters->m_bFieldPicFlag )
  {
    for( int iMbIdx = 0; iMbIdx < iFrmSizeInMbs; iMbIdx++ )
    {
      int iMbX      = ( iMbIdx % iMbStride );
      int iMbY      = ( iMbIdx / iMbStride );
      int iMbMapIdx = iMbY * m_iMbMapStride + iMbX;
      int iMbAddr   = iMbIdx;
      if( pcParameters->m_bIsMbAffFrame )
      {
        iMbAddr = ( ( ( iMbY >> 1 ) * iMbStride + iMbX ) << 1 ) + ( iMbY % 2 );
      }
      if( ( m_paeMbMapFrm[ iMbMapIdx ] & INTRA_UPS_ALLOWED ) == INTRA_UPS_ALLOWED )
      {
        AOT( m_pabIntraUpsAvailableFrm[ iMbAddr ] ); // something wrong
        m_pabIntraUpsAvailableFrm[ iMbAddr ] = true;
      }
    }
  }

  if( pcParameters->m_bFieldPicFlag || pcParameters->m_bIsMbAffFrame )
  {
    for( int iMbIdx = 0; iMbIdx < ( iFrmSizeInMbs >> iField ); iMbIdx++ )
    {
      int iMbX      =   ( iMbIdx % iMbStride );
      int iMbY      = ( ( iMbIdx / iMbStride ) << iField ) + iBotField;
      int iMbMapIdx = iMbY * m_iMbMapStride + iMbX;
      int iMbAddr   = iMbIdx;
      if( pcParameters->m_bIsMbAffFrame )
      {
        iMbAddr = ( ( ( iMbY >> 1 ) * iMbStride + iMbX ) << 1 ) + ( iMbY % 2 );
      }
      if( ( m_paeMbMapFld[ iMbMapIdx ] & INTRA_UPS_ALLOWED ) == INTRA_UPS_ALLOWED )
      {
        AOT( m_pabIntraUpsAvailableFld[ iMbAddr ] ); // something wrong
        m_pabIntraUpsAvailableFld[ iMbAddr ] = true;
      }
    }
  }
}


void
DownConvert::xUpdateBaseModeFlagsIntraUps( ResizeParameters* pcParameters,
                                           MbDataCtrl*       pcMbDataCtrlPredFrm,
                                           MbDataCtrl*       pcMbDataCtrlPredFld,
                                           bool*             pabBaseModeAllowedFlagArrayFrm,
                                           bool*             pabBaseModeAllowedFlagArrayFld )
{
  ROTVS( pcParameters->m_bIsMbAffFrame );
  ROTVS( pcParameters->m_bRefLayerIsMbAffFrame );
  ROTVS( pcParameters->getRestrictedSpatialResolutionChangeFlag() );

  int iFrmSizeInMbs = ( pcParameters->m_iFrameWidth * pcParameters->m_iFrameHeight ) >> 8;
  int iField        = ( pcParameters->m_bFieldPicFlag ? 1 : 0 );

  if( pabBaseModeAllowedFlagArrayFrm && ! pcParameters->m_bFieldPicFlag )
  {
    for( int iMbIdx = 0; iMbIdx < iFrmSizeInMbs; iMbIdx++ )
    {
      if( pabBaseModeAllowedFlagArrayFrm[ iMbIdx ] && !m_pabIntraUpsAvailableFrm[ iMbIdx ] )
      {
        const MbData& rcMbPredData = pcMbDataCtrlPredFrm->getMbData( iMbIdx );
        if( rcMbPredData.getInCropWindowFlag() &&
            ( rcMbPredData.isBaseIntra( 0, 0 ) ||
              rcMbPredData.isBaseIntra( 0, 1 ) ||
              rcMbPredData.isBaseIntra( 1, 0 ) ||
              rcMbPredData.isBaseIntra( 1, 1 )  ) )
        {
          pabBaseModeAllowedFlagArrayFrm[ iMbIdx ] = false;
        }
      }
    }
  }

  if( pabBaseModeAllowedFlagArrayFld && ( pcParameters->m_bFieldPicFlag || pcParameters->m_bIsMbAffFrame ) )
  {
    for( int iMbIdx = 0; iMbIdx < ( iFrmSizeInMbs >> iField ); iMbIdx++ )
    {
      if( pabBaseModeAllowedFlagArrayFld[ iMbIdx ] && !m_pabIntraUpsAvailableFld[ iMbIdx ] )
      {
        const MbData& rcMbPredData = pcMbDataCtrlPredFld->getMbData( iMbIdx );
        if( rcMbPredData.getInCropWindowFlag() &&
            ( rcMbPredData.isBaseIntra( 0, 0 ) ||
              rcMbPredData.isBaseIntra( 0, 1 ) ||
              rcMbPredData.isBaseIntra( 1, 0 ) ||
              rcMbPredData.isBaseIntra( 1, 1 )  ) )
        {
          pabBaseModeAllowedFlagArrayFld[ iMbIdx ] = false;
        }
      }
    }
  }
}


void
DownConvert::xGenerateMbMapsForSliceId( ResizeParameters* pcParameters,
                                        MbDataCtrl*       pcMbDataCtrlBase,
                                        MbDataCtrl*       pcMbDataCtrlPredFrm,
                                        MbDataCtrl*       pcMbDataCtrlPredFld,
                                        unsigned int      uiCurrentSliceId )
{
  if( pcParameters->m_bFieldPicFlag )
  {
    xInitMbMaps           ( pcParameters, false, ! pcParameters->m_bBotFieldFlag, pcParameters->m_bBotFieldFlag );
    xUpdateMbMapForSliceId( pcParameters, false, true,  pcMbDataCtrlBase, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld, uiCurrentSliceId );
    xUpdateMbMapForSliceId( pcParameters, true,  true,  pcMbDataCtrlBase, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld, uiCurrentSliceId );
  }
  else if( pcParameters->m_bIsMbAffFrame )
  {
    xInitMbMaps           ( pcParameters, true,  true,  true );
    xUpdateMbMapForSliceId( pcParameters, false, false, pcMbDataCtrlBase, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld, uiCurrentSliceId );
    xUpdateMbMapForSliceId( pcParameters, true,  false, pcMbDataCtrlBase, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld, uiCurrentSliceId );
    xUpdateMbMapForSliceId( pcParameters, false, true,  pcMbDataCtrlBase, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld, uiCurrentSliceId );
    xUpdateMbMapForSliceId( pcParameters, true,  true,  pcMbDataCtrlBase, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld, uiCurrentSliceId );
  }
  else
  {
    xInitMbMaps           ( pcParameters, true,  false, false );
    xUpdateMbMapForSliceId( pcParameters, false, false, pcMbDataCtrlBase, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld, uiCurrentSliceId );
    xUpdateMbMapForSliceId( pcParameters, true,  false, pcMbDataCtrlBase, pcMbDataCtrlPredFrm, pcMbDataCtrlPredFld, uiCurrentSliceId );
  }
}

void
DownConvert::xInitMbMaps( ResizeParameters* pcParameters, bool bFrm, bool bTop, bool bBot )
{
  MbMapEntry  eFrm            = MbMapEntry( bFrm ? ( BASE_MODE_ALLOWED | INTRA_UPS_ALLOWED ) : INVALID_ENTRY );
  MbMapEntry  eTop            = MbMapEntry( bTop ? ( BASE_MODE_ALLOWED | INTRA_UPS_ALLOWED ) : INVALID_ENTRY );
  MbMapEntry  eBot            = MbMapEntry( bBot ? ( BASE_MODE_ALLOWED | INTRA_UPS_ALLOWED ) : INVALID_ENTRY );
  int         iFrmWidthInMbs  = ( pcParameters->m_iFrameWidth  >> 4 );
  int         iFrmHeightInMbs = ( pcParameters->m_iFrameHeight >> 4 );

  for( int iMbY = 0; iMbY < iFrmHeightInMbs; iMbY++ )
  for( int iMbX = 0; iMbX < iFrmWidthInMbs;  iMbX++ )
  {
    int iIdx            = iMbY * m_iMbMapStride + iMbX;
    m_paeMbMapFrm[iIdx] = eFrm;
    m_paeMbMapFld[iIdx] = ( ( iMbY % 2 ) ? eBot : eTop );
  }
}

void
DownConvert::xUpdateMbMapForSliceId( ResizeParameters*  pcParameters,
                                     bool               bChroma,
                                     bool               bFieldMb,
                                     MbDataCtrl*        pcMbDataCtrlBase,
                                     MbDataCtrl*        pcMbDataCtrlPredFrm,
                                     MbDataCtrl*        pcMbDataCtrlPredFld,
                                     unsigned int       uiCurrentSliceId )
{
  //===== general parameters =====
  int         iCShift             = ( bChroma ? 1 : 0 );
  int         iMbW                = ( 16 >> iCShift );
  int         iMbH                = ( 16 >> iCShift );
  int         iFieldMb            = ( bFieldMb                      ? 1 : 0 );
  int         iFieldPic           = ( pcParameters->m_bFieldPicFlag ? 1 : 0 );
  int         iBotField           = ( pcParameters->m_bBotFieldFlag ? 1 : 0 );
  int         iFldMbInFrm         = ( iFieldMb - iFieldPic );
  int         iExtraYShift        = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag && pcParameters->m_bFrameMbsOnlyFlag ? 0 : 1 - iFieldMb );
  int         iPicWidthInMbs      = ( pcParameters->m_iFrameWidth  >> 4 );
  int         iPicHeightInMbs     = ( pcParameters->m_iFrameHeight >> 4 ) >> iFieldPic;
  int         iMbMapStride        = ( m_iMbMapStride << iFieldPic );
  int         iMbMapOffset        = ( m_iMbMapStride  * iBotField );
  MbMapEntry* paeMbMap            = ( bFieldMb ? m_paeMbMapFld       : m_paeMbMapFrm );
  MbDataCtrl* pcMbDataCtrlPred    = ( bFieldMb ? pcMbDataCtrlPredFld : pcMbDataCtrlPredFrm );
  bool        bInterIntraAllowed  = ( ! pcParameters->m_bIsMbAffFrame && ! pcParameters->m_bRefLayerIsMbAffFrame );

  int         iRefPicWidthInMbs   = ( pcParameters->m_iRefLayerFrmWidth    >> 4 );
  int         iRefCompWidth       = ( pcParameters->m_iRefLayerFrmWidth    >> iCShift );
  int         iRefCompHeight      = ( pcParameters->m_iRefLayerFrmHeight   >> iCShift ) >> ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ? 0 : 1 );
  int         iRefFieldPic        = ( pcParameters->m_bRefLayerFieldPicFlag ? 1 : 0 );
  int         iRefBotField        = ( pcParameters->m_bRefLayerBotFieldFlag ? 1 : 0 );
  int         iRefFldCompInFrm    = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag || pcParameters->m_bRefLayerFieldPicFlag ? 0 : 1 );
  int         iMbIdxStride        = ( iRefPicWidthInMbs << iRefFieldPic );
  int         iMbIdxOffset        = ( iRefPicWidthInMbs  * iRefBotField );

  bool        bAdaptiveBotFlag    = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag == false &&
                                      pcParameters->m_bFieldPicFlag             == false &&
                                      pcParameters->m_bRefLayerFieldPicFlag     == false &&
                                      bFieldMb                                  == true    );
  int         iFixedBotFlag       = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ? 0            :
                                      pcParameters->m_bFieldPicFlag             ? iBotField    :
                                      pcParameters->m_bRefLayerFieldPicFlag     ? iRefBotField : 0 );

  //===== determine position calculation parameters =====
  int   iFactor         = ( !bChroma ? 1 : 2 );
  int   iRefPhaseX      = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseX );
  int   iRefPhaseY      = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseY );
  int   iPhaseX         = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseX );
  int   iPhaseY         = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseY );
  int   iRefW           = pcParameters->m_iRefLayerFrmWidth    / iFactor;  // reference layer frame width
  int   iRefH           = pcParameters->m_iRefLayerFrmHeight   / iFactor;  // reference layer frame height
  int   iOutW           = pcParameters->m_iScaledRefFrmWidth   / iFactor;  // scaled reference layer frame width
  int   iOutH           = pcParameters->m_iScaledRefFrmHeight  / iFactor;  // scaled reference layer frame height

  int   iScaledW        = iOutW;
  int   iScaledH        = ( ! pcParameters->m_bRefLayerFrameMbsOnlyFlag || pcParameters->m_bFrameMbsOnlyFlag ? iOutH : iOutH / 2 );
  int   iShiftX         = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefW ) );
  int   iShiftY         = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefH ) );
  int   iScaleX         = ( ( (unsigned int)iRefW << iShiftX ) + ( iScaledW >> 1 ) ) / iScaledW;
  int   iScaleY         = ( ( (unsigned int)iRefH << iShiftY ) + ( iScaledH >> 1 ) ) / iScaledH;
  int   iOffsetX        = pcParameters->m_iLeftFrmOffset / iFactor;
  int   iOffsetY        = pcParameters->m_iTopFrmOffset  / iFactor;
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    iOffsetY            = iOffsetY / 2;
  }
  int   iAddX           = ( ( ( iRefW * ( 2 + iPhaseX ) ) << ( iShiftX - 2 ) ) + ( iScaledW >> 1 ) ) / iScaledW + ( 1 << ( iShiftX - 5 ) );
  int   iDeltaX         = 4 * ( 2 + iRefPhaseX );
  int   aiAddY  [2]     = { 0, 0 };
  int   aiDeltaY[2]     = { 0, 0 };
  for( int iBot = 0; iBot <= 1; iBot++ )
  {
    int iTempPhaseY     = iPhaseY;
    int iTempRefPhaseY  = iRefPhaseY;

    if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
    {
      if( pcParameters->m_bRefLayerFrameMbsOnlyFlag )
      {
        iTempPhaseY     = iTempPhaseY + 4 * iBotField + ( 3 - iFactor );
        iTempRefPhaseY  = 2 * iTempRefPhaseY + 2;
      }
      else
      {
        iTempPhaseY     = iTempPhaseY    + 4 * iBot;
        iTempRefPhaseY  = iTempRefPhaseY + 4 * iBot;
      }
    }
    if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
    {
      aiAddY  [iBot]    = ( ( ( iRefH * ( 2 + iTempPhaseY ) ) << ( iShiftY - 3 ) ) + ( iScaledH >> 1 ) ) / iScaledH + ( 1 << ( iShiftY - 5 ) );
      aiDeltaY[iBot]    = 2 * ( 2 + iTempRefPhaseY );
    }
    else
    {
      aiAddY  [iBot]    = ( ( ( iRefH * ( 2 + iTempPhaseY ) ) << ( iShiftY - 2 ) ) + ( iScaledH >> 1 ) ) / iScaledH + ( 1 << ( iShiftY - 5 ) );
      aiDeltaY[iBot]    = 4 * ( 2 + iTempRefPhaseY );
    }
  }

  int iShiftXM4 = iShiftX - 4;
  int iShiftYM4 = iShiftY - 4;

  //===== loop over macroblocks =====
  for( int iMbY = 0; iMbY < iPicHeightInMbs; iMbY++ )
  for( int iMbX = 0; iMbX < iPicWidthInMbs;  iMbX++ )
  {
    MbMapEntry& reMapEntry  = paeMbMap[ iMbMapOffset + iMbY * iMbMapStride + iMbX ];
    int         iMbIdxCurr  = iMbY * ( iPicWidthInMbs << iFieldPic ) + iMbX + ( iPicWidthInMbs * iBotField );
    MbData&     rcMbDataPrd = pcMbDataCtrlPred->getMbDataByIndex( (unsigned int)iMbIdxCurr );
    bool        bInCropWnd  = rcMbDataPrd.getInCropWindowFlag();
    bool        bIsPredIBL  = rcMbDataPrd.isIntraBL();

    if( ! bInCropWnd )
    {
      // outside cropping window
      reMapEntry = MbMapEntry( reMapEntry & ( ~( INTRA_UPS_ALLOWED | BASE_MODE_ALLOWED ) ) );
      continue;
    }

    if( ! bInterIntraAllowed && ! bIsPredIBL )
    {
      // intra upsampling never required in this case, base_mode_flag always allowed
      reMapEntry = MbMapEntry( reMapEntry & ( ~INTRA_UPS_ALLOWED ) );
      continue;
    }

    int   iXLumMbInComp = (   iMbX                  << 4 ) >> iCShift;
    int   iYLumMbInComp = ( ( iMbY >> iFldMbInFrm ) << 4 ) >> iCShift;
    int   iXCurrMin     = ( 0        + iXLumMbInComp );
    int   iXCurrMax     = ( iMbW - 1 + iXLumMbInComp );
    int   iYCurrMin     = ( 0        + iYLumMbInComp ) >> iExtraYShift;
    int   iYCurrMax     = ( iMbH - 1 + iYLumMbInComp ) >> iExtraYShift;

    int   iB            = ( ! bAdaptiveBotFlag ? iFixedBotFlag : iMbY % 2 );
    int   iXRefMin      = ( (int)( (unsigned int)( ( iXCurrMin - iOffsetX ) * iScaleX +  iAddX     ) >> iShiftXM4 ) -  iDeltaX          ) >> 4;
    int   iXRefMax      = ( (int)( (unsigned int)( ( iXCurrMax - iOffsetX ) * iScaleX +  iAddX     ) >> iShiftXM4 ) -  iDeltaX     + 15 ) >> 4;
    int   iYRefMin      = ( (int)( (unsigned int)( ( iYCurrMin - iOffsetY ) * iScaleY + aiAddY[iB] ) >> iShiftYM4 ) - aiDeltaY[iB]      ) >> 4;
    int   iYRefMax      = ( (int)( (unsigned int)( ( iYCurrMax - iOffsetY ) * iScaleY + aiAddY[iB] ) >> iShiftYM4 ) - aiDeltaY[iB] + 15 ) >> 4;

    bool  bIsIntraCurr  = false;
    bool  bIsIntraDiff  = false;

    for( int iYRef = iYRefMin + 1; iYRef < iYRefMax && ! ( bIsIntraCurr && bIsIntraDiff ); iYRef++ )
    for( int iXRef = iXRefMin + 1; iXRef < iXRefMax && ! ( bIsIntraCurr && bIsIntraDiff ); iXRef++ )
    {
      int     iRefMbX   = xClip( iXRef, 0, iRefCompWidth  - 1 ) /   iMbW;
      int     iRefMbY   = xClip( iYRef, 0, iRefCompHeight - 1 ) / ( iMbH >> iRefFldCompInFrm );
      int     iMbIdx    = iMbIdxOffset + iRefMbY * iMbIdxStride + iRefMbX;
      MbData& rcMbData  = pcMbDataCtrlBase->getMbDataByIndex( (unsigned int)iMbIdx );
      if( ! bIsIntraCurr )
      {
        bIsIntraCurr    = rcMbData.isIntraInSlice( uiCurrentSliceId );
      }
      if( ! bIsIntraDiff )
      {
        bIsIntraDiff    = rcMbData.isIntra() && ! rcMbData.isIntraInSlice( uiCurrentSliceId );
      }
    }

    if( bInterIntraAllowed )
    {
      if( ! bIsIntraCurr || bIsIntraDiff )
      {
        reMapEntry = MbMapEntry( reMapEntry & ( ~INTRA_UPS_ALLOWED ) );
      }
      if( bIsIntraCurr && bIsIntraDiff )
      {
        reMapEntry = MbMapEntry( reMapEntry & ( ~BASE_MODE_ALLOWED ) );
      }
    }
    else // bInterIntraAllowed == false && prediction mode is IBL
    {
      if( bIsIntraDiff )
      {
        reMapEntry = MbMapEntry( reMapEntry & ( ~( INTRA_UPS_ALLOWED | BASE_MODE_ALLOWED ) ) );
      }
    }
  }
}

void
DownConvert::xUpdateIntraPredFrame( Frame* pcDesFrame, Frame* pcSrcFrame, ResizeParameters* pcParameters )
{
  pcDesFrame->getFullPelYuvBuffer()->getYuvBufferCtrl().initMb();
  pcSrcFrame->getFullPelYuvBuffer()->getYuvBufferCtrl().initMb();

  int     iFrmWidthInMbs  = ( pcParameters->m_iFrameWidth  >> 4 );
  int     iFrmHeightInMbs = ( pcParameters->m_iFrameHeight >> 4 );
  short*  psDesOriginY    = pcDesFrame->getFullPelYuvBuffer()->getMbLumAddr();
  short*  psDesOriginU    = pcDesFrame->getFullPelYuvBuffer()->getMbCbAddr ();
  short*  psDesOriginV    = pcDesFrame->getFullPelYuvBuffer()->getMbCrAddr ();
  short*  psSrcOriginY    = pcSrcFrame->getFullPelYuvBuffer()->getMbLumAddr();
  short*  psSrcOriginU    = pcSrcFrame->getFullPelYuvBuffer()->getMbCbAddr ();
  short*  psSrcOriginV    = pcSrcFrame->getFullPelYuvBuffer()->getMbCrAddr ();
  int     iDesStrideY     = pcDesFrame->getFullPelYuvBuffer()->getLStride  ();
  int     iDesStrideC     = pcDesFrame->getFullPelYuvBuffer()->getCStride  ();
  int     iSrcStrideY     = pcSrcFrame->getFullPelYuvBuffer()->getLStride  ();
  int     iSrcStrideC     = pcSrcFrame->getFullPelYuvBuffer()->getCStride  ();

  for( int iMbY = 0; iMbY < iFrmHeightInMbs; iMbY++ )
  for( int iMbX = 0; iMbX < iFrmWidthInMbs;  iMbX++ )
  {
    int   iMbYTop   = ( iMbY >> 1 ) << 1;
    int   iMbYBot   = iMbYTop + 1;
    bool  bUpdateMb = ( ( m_paeMbMapFrm[ iMbY    * m_iMbMapStride + iMbX ] & INTRA_UPS_ALLOWED ) == INTRA_UPS_ALLOWED );
    bUpdateMb       = ( ( m_paeMbMapFld[ iMbYTop * m_iMbMapStride + iMbX ] & INTRA_UPS_ALLOWED ) == INTRA_UPS_ALLOWED ) || bUpdateMb;
    bUpdateMb       = ( ( m_paeMbMapFld[ iMbYBot * m_iMbMapStride + iMbX ] & INTRA_UPS_ALLOWED ) == INTRA_UPS_ALLOWED ) || bUpdateMb;
    if( ! bUpdateMb )
    {
      continue;
    }

    short*  pDesY   = psDesOriginY + ( iMbY << 4 ) * iDesStrideY + ( iMbX << 4 );
    short*  pDesU   = psDesOriginU + ( iMbY << 3 ) * iDesStrideC + ( iMbX << 3 );
    short*  pDesV   = psDesOriginV + ( iMbY << 3 ) * iDesStrideC + ( iMbX << 3 );
    short*  pSrcY   = psSrcOriginY + ( iMbY << 4 ) * iDesStrideY + ( iMbX << 4 );
    short*  pSrcU   = psSrcOriginU + ( iMbY << 3 ) * iDesStrideC + ( iMbX << 3 );
    short*  pSrcV   = psSrcOriginV + ( iMbY << 3 ) * iDesStrideC + ( iMbX << 3 );
    for( int iYL = 0; iYL < 16; iYL++, pDesY += iDesStrideY, pSrcY += iSrcStrideY )
    for( int iXL = 0; iXL < 16; iXL++ )
    {
      pDesY[iXL] = pSrcY[iXL];
    }
    for( int iYC = 0; iYC < 8; iYC++, pDesU += iDesStrideC, pDesV += iDesStrideC, pSrcU += iSrcStrideC, pSrcV += iSrcStrideC )
    for( int iXC = 0; iXC < 8; iXC++ )
    {
      pDesU[iXC] = pSrcU[iXC];
      pDesV[iXC] = pSrcV[iXC];
    }
  }
}


void
DownConvert::xResidualUpsampling( Frame*            pcFrame,
                                  Frame*            pcBaseFrame,
                                  ResizeParameters* pcParameters,
                                  MbDataCtrl*       pcMbDataCtrlBase )
{
  pcFrame    ->getFullPelYuvBuffer()->getYuvBufferCtrl().initMb();
  pcBaseFrame->getFullPelYuvBuffer()->getYuvBufferCtrl().initMb();

  short*        psDesY                  =   pcFrame->getFullPelYuvBuffer()->getMbLumAddr();
  short*        psDesU                  =   pcFrame->getFullPelYuvBuffer()->getMbCbAddr ();
  short*        psDesV                  =   pcFrame->getFullPelYuvBuffer()->getMbCrAddr ();
  int           iDesStrideY             =   pcFrame->getFullPelYuvBuffer()->getLStride  ();
  int           iDesStrideC             =   pcFrame->getFullPelYuvBuffer()->getCStride  ();
  const short*  psSrcY                  =   pcBaseFrame->getFullPelYuvBuffer()->getMbLumAddr();
  const short*  psSrcU                  =   pcBaseFrame->getFullPelYuvBuffer()->getMbCbAddr ();
  const short*  psSrcV                  =   pcBaseFrame->getFullPelYuvBuffer()->getMbCrAddr ();
  int           iSrcStrideY             =   pcBaseFrame->getFullPelYuvBuffer()->getLStride  ();
  int           iSrcStrideC             =   pcBaseFrame->getFullPelYuvBuffer()->getCStride  ();

  int           iBaseW                  =   pcParameters->m_iRefLayerFrmWidth;
  int           iBaseH                  =   pcParameters->m_iRefLayerFrmHeight;
  int           iCurrW                  =   pcParameters->m_iFrameWidth;
  int           iCurrH                  =   pcParameters->m_iFrameHeight;
  bool          bTopAndBottomResampling = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag == false  &&
                                            pcParameters->m_bRefLayerFieldPicFlag     == false  &&
                                            pcParameters->m_bFrameMbsOnlyFlag         == false  &&
                                            pcParameters->m_bFieldPicFlag             == false    );
  bool          bFrameBasedResampling   = ( pcParameters->m_bFrameMbsOnlyFlag         == true   &&
                                            pcParameters->m_bRefLayerFrameMbsOnlyFlag == true     );
  bool          bVerticalInterpolation  = ( bFrameBasedResampling                     == false  &&
                                            pcParameters->m_bFieldPicFlag             == false    );
  bool          bCurrBotField           = ( pcParameters->m_bFieldPicFlag             == true   &&
                                            pcParameters->m_bBotFieldFlag             == true     );
  bool          bBotFieldFlag           = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ?  false
                                          : pcParameters->m_bFieldPicFlag             ?  pcParameters->m_bBotFieldFlag
                                          : pcParameters->m_bRefLayerFieldPicFlag     ?  pcParameters->m_bRefLayerBotFieldFlag
                                          : false );
  int           iBaseField              = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ?  0 : 1 );
  int           iCurrField              = ( pcParameters->m_bFieldPicFlag             ?  1 : 0 );
  int           iBaseBot                = ( bBotFieldFlag ? 1 : 0 );
  int           iCurrBot                = ( bCurrBotField ? 1 : 0 );

  //=======================
  //=====   L U M A   =====
  //=======================
  if( bTopAndBottomResampling )
  {
    //===== top field =====
    const short*  pSrcFld = psSrcY;
    short*        pDesFld = psDesY;
    xCopyToImageBuffer      ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideY << 1                  );
    xCompResidualUpsampling ( pcParameters, false,  false,       false,           pcMbDataCtrlBase );
    xCopyFromImageBuffer    ( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideY << 1                  );

    //===== bottom field =====
    pSrcFld += iSrcStrideY;
    pDesFld += iDesStrideY;
    xCopyToImageBuffer      ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideY << 1                  );
    xCompResidualUpsampling ( pcParameters, false,  true,        false,           pcMbDataCtrlBase );
    xCopyFromImageBuffer    ( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideY << 1                  );
  }
  else
  {
    const short*  pSrc = psSrcY + iSrcStrideY * iBaseBot;
    short*        pDes = psDesY + iDesStrideY * iCurrBot;
    xCopyToImageBuffer      ( pSrc,         iBaseW, iBaseH >> iBaseField, iSrcStrideY << iBaseField                   );
    xCompResidualUpsampling ( pcParameters, false,  bBotFieldFlag,        bVerticalInterpolation,    pcMbDataCtrlBase );
    xCopyFromImageBuffer    ( pDes,         iCurrW, iCurrH >> iCurrField, iDesStrideY << iCurrField                   );
  }

  iBaseW >>= 1;
  iBaseH >>= 1;
  iCurrW >>= 1;
  iCurrH >>= 1;

  //===========================
  //=====   C H R O M A   =====
  //===========================
  if( bTopAndBottomResampling )
  {
    //===== top field (U) =====
    const short*  pSrcFld = psSrcU;
    short*        pDesFld = psDesU;
    xCopyToImageBuffer      ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideC << 1                  );
    xCompResidualUpsampling ( pcParameters, true,   false,       false,           pcMbDataCtrlBase );
    xCopyFromImageBuffer    ( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideC << 1                  );

    //===== bottom field (U) =====
    pSrcFld += iSrcStrideC;
    pDesFld += iDesStrideC;
    xCopyToImageBuffer      ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideC << 1                  );
    xCompResidualUpsampling ( pcParameters, true,   true,        false,           pcMbDataCtrlBase );
    xCopyFromImageBuffer    ( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideC << 1                  );

    //===== top field (V) =====
    pSrcFld = psSrcV;
    pDesFld = psDesV;
    xCopyToImageBuffer      ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideC << 1                  );
    xCompResidualUpsampling ( pcParameters, true,   false,       false,           pcMbDataCtrlBase );
    xCopyFromImageBuffer    ( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideC << 1                  );

    //===== bottom field (V) =====
    pSrcFld += iSrcStrideC;
    pDesFld += iDesStrideC;
    xCopyToImageBuffer      ( pSrcFld,      iBaseW, iBaseH >> 1, iSrcStrideC << 1                  );
    xCompResidualUpsampling ( pcParameters, true,   true,        false,           pcMbDataCtrlBase );
    xCopyFromImageBuffer    ( pDesFld,      iCurrW, iCurrH >> 1, iDesStrideC << 1                  );
  }
  else
  {
    //===== U =====
    const short*  pSrc = psSrcU + iSrcStrideC * iBaseBot;
    short*        pDes = psDesU + iDesStrideC * iCurrBot;
    xCopyToImageBuffer      ( pSrc,         iBaseW, iBaseH >> iBaseField, iSrcStrideC << iBaseField                   );
    xCompResidualUpsampling ( pcParameters, true,   bBotFieldFlag,        bVerticalInterpolation,    pcMbDataCtrlBase );
    xCopyFromImageBuffer    ( pDes,         iCurrW, iCurrH >> iCurrField, iDesStrideC << iCurrField                   );

    //===== V =====
    pSrc = psSrcV + iSrcStrideC * iBaseBot;
    pDes = psDesV + iDesStrideC * iCurrBot;
    xCopyToImageBuffer      ( pSrc,         iBaseW, iBaseH >> iBaseField, iSrcStrideC << iBaseField                   );
    xCompResidualUpsampling ( pcParameters, true,   bBotFieldFlag,        bVerticalInterpolation,    pcMbDataCtrlBase );
    xCopyFromImageBuffer    ( pDes,         iCurrW, iCurrH >> iCurrField, iDesStrideC << iCurrField                   );
  }
}


void
DownConvert::xDetermineTransBlkIdcs( int iBaseW, int iBaseH, bool bChroma, bool bBotField, ResizeParameters* pcRP, MbDataCtrl* pcMbDataCtrlBase )
{
  if( ! pcRP->m_bRefLayerIsMbAffFrame )
  {
    //===== progressive frame, field picture, or field of non-Mbaff frame =====
    int iYShift         = ( pcRP->m_bRefLayerFrameMbsOnlyFlag || pcRP->m_bRefLayerFieldPicFlag ? 0 : 1 );
    int iMbSizeX        = ( bChroma ? 8 : 16 );
    int iMbSizeY        = iMbSizeX >> iYShift;
    int iPicWidthInMbs  = iBaseW / iMbSizeX;
    int iPicHeightInMbs = iBaseH / iMbSizeY;
    int iMbIdxOffset    = ( pcRP->m_bRefLayerBotFieldFlag ? iPicWidthInMbs      : 0 );
    int iMbIdxStride    = ( pcRP->m_bRefLayerFieldPicFlag ? iPicWidthInMbs << 1 : iPicWidthInMbs );
    for( int yMb = 0; yMb < iPicHeightInMbs; yMb++ )
    for( int xMb = 0; xMb < iPicWidthInMbs;  xMb++ )
    {
      int     iMbAddr   = yMb * iPicWidthInMbs + xMb;
      int     iMbIdx    = yMb * iMbIdxStride   + xMb + iMbIdxOffset;
      MbData& rcMbData  = pcMbDataCtrlBase->getMbDataByIndex( (unsigned int)iMbIdx );
      bool    b8x8      = ( bChroma ? false : rcMbData.isTransformSize8x8() );
      int*    pDes      = m_paiTransBlkIdc + yMb * iMbSizeY * m_iImageStride + xMb * iMbSizeX;
      if( b8x8 )
      {
        for( int y = 0; y < iMbSizeY; y++, pDes += m_iImageStride )
        for( int x = 0; x < iMbSizeX; x++ )
        {
          pDes[x]   = ( ( y >> ( 3 - iYShift ) ) << 1 ) + ( x >> 3 );   // 8x8 block index inside MB
          pDes[x]  += iMbAddr << 2;                                     // macroblock address
          pDes[x]   = ( pDes[x] << 1 ) + 1;                             // 8x8 indication
        }
      }
      else
      {
        for( int y = 0; y < iMbSizeY; y++, pDes += m_iImageStride )
        for( int x = 0; x < iMbSizeX; x++ )
        {
          pDes[x]   = ( ( y >> ( 2 - iYShift ) ) << 2 ) + ( x >> 2 );   // 4x4 block index inside MB
          pDes[x]  += iMbAddr << 4;                                     // macroblock address
          pDes[x]   = ( pDes[x] << 1 );                                 // 4x4 indication
        }
      }
    }
  }
  else
  {
    //===== field of MBaff frame =====
    int iMbSizeX              = ( bChroma ? 8 : 16 );
    int iMapUnitSizeY         = iMbSizeX;
    int iPicWidthInMbs        = iBaseW / iMbSizeX;
    int iPicHeightInMapUnits  = iBaseH / iMapUnitSizeY;
    int iMapUnitStride        = iPicWidthInMbs << 1;
    for( int yMbPair = 0; yMbPair < iPicHeightInMapUnits; yMbPair++ )
    for( int xMb     = 0; xMb     < iPicWidthInMbs;       xMb    ++ )
    {
      int     iTopAddr  = yMbPair * iMapUnitStride + ( xMb << 1 );
      int     iTopIdx   = yMbPair * iMapUnitStride +   xMb;
      MbData& rcTopData = pcMbDataCtrlBase->getMbDataByIndex( (unsigned int)iTopIdx );
      bool    bFieldMb  = rcTopData.getFieldFlag();
      if( bFieldMb )
      {
        //----- ref samples for map unit correspond to one field macroblock -----
        int     iMbSizeY  = iMapUnitSizeY;
        int     iBotField = ( bBotField ? 1 : 0 );
        int     iMbAddr   = iTopAddr + iBotField;
        int     iMbIdx    = iTopIdx  + iBotField * ( iMapUnitStride >> 1 );
        MbData& rcMbData  = pcMbDataCtrlBase->getMbDataByIndex( (unsigned int)iMbIdx );
        bool    b8x8      = ( bChroma ? false : rcMbData.isTransformSize8x8() );
        int*    pDes      = m_paiTransBlkIdc + yMbPair * iMapUnitSizeY * m_iImageStride + xMb * iMbSizeX;
        if( b8x8 )
        {
          for( int y = 0; y < iMbSizeY; y++, pDes += m_iImageStride )
          for( int x = 0; x < iMbSizeX; x++ )
          {
            pDes[x]   = ( ( y >> 3 ) << 1 ) + ( x >> 3 );   // 8x8 block index inside MB
            pDes[x]  += iMbAddr << 2;                       // macroblock address
            pDes[x]   = ( pDes[x] << 1 ) + 1;               // 8x8 indication
          }
        }
        else
        {
          for( int y = 0; y < iMbSizeY; y++, pDes += m_iImageStride )
          for( int x = 0; x < iMbSizeX; x++ )
          {
            pDes[x]   = ( ( y >> 2 ) << 2 ) + ( x >> 2 );   // 4x4 block index inside MB
            pDes[x]  += iMbAddr << 4;                       // macroblock address
            pDes[x]   = ( pDes[x] << 1 );                   // 4x4 indication
          }
        }
      }
      else
      {
        //----- ref samples for map unit correspond to two frame macroblocks -----
        int     iMbSizeY  = iMapUnitSizeY >> 1;
        int     iBotAddr  = iTopAddr + 1;
        int     iBotIdx   = iTopIdx  + ( iMapUnitStride >> 1 );
        MbData& rcBotData = pcMbDataCtrlBase->getMbDataByIndex( (unsigned int)iBotIdx );
        bool    b8x8Top   = ( bChroma ? false : rcTopData.isTransformSize8x8() );
        bool    b8x8Bot   = ( bChroma ? false : rcBotData.isTransformSize8x8() );
        int*    pDes      = m_paiTransBlkIdc + yMbPair * iMapUnitSizeY * m_iImageStride + xMb * iMbSizeX;
        for( int yMb = 0; yMb < 2; yMb++ )
        {
          int         iMbAddr   = ( yMb ? iBotAddr : iTopAddr );
          int         b8x8      = ( yMb ? b8x8Bot  : b8x8Top  );
          if( b8x8 )
          {
            for( int y = 0; y < iMbSizeY; y++, pDes += m_iImageStride )
            for( int x = 0; x < iMbSizeX; x++ )
            {
              pDes[x]   = ( ( y >> 2 ) << 1 ) + ( x >> 3 );   // 8x8 block index inside MB
              pDes[x]  += iMbAddr << 2;                       // macroblock address
              pDes[x]   = ( pDes[x] << 1 ) + 1;               // 8x8 indication
            }
          }
          else
          {
            for( int y = 0; y < iMbSizeY; y++, pDes += m_iImageStride )
            for( int x = 0; x < iMbSizeX; x++ )
            {
              pDes[x]   = ( ( y >> 1 ) << 2 ) + ( x >> 2 );   // 4x4 block index inside MB
              pDes[x]  += iMbAddr << 4;                       // macroblock address
              pDes[x]   = ( pDes[x] << 1 );                   // 4x4 indication
            }
          }
        }
      }
    }
  }
}

void
DownConvert::xCompResidualUpsampling( ResizeParameters* pcParameters, bool bChroma, bool bBotFlag, bool bVerticalInterpolation, MbDataCtrl* pcMbDataCtrlBase )
{
  //===== set general parameters =====
  int   iBotField   = ( bBotFlag ? 1 : 0 );
  int   iFactor     = ( !bChroma ? 1 : 2 );
  int   iRefPhaseX  = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseX );
  int   iRefPhaseY  = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseY );
  int   iPhaseX     = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseX );
  int   iPhaseY     = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseY );
  int   iRefW       = pcParameters->m_iRefLayerFrmWidth   / iFactor;  // reference layer frame width
  int   iRefH       = pcParameters->m_iRefLayerFrmHeight  / iFactor;  // reference layer frame height
  int   iOutW       = pcParameters->m_iScaledRefFrmWidth  / iFactor;  // scaled reference layer frame width
  int   iOutH       = pcParameters->m_iScaledRefFrmHeight / iFactor;  // scaled reference layer frame height
  int   iGlobalW    = pcParameters->m_iFrameWidth         / iFactor;  // current frame width
  int   iGlobalH    = pcParameters->m_iFrameHeight        / iFactor;  // current frame height
  int   iLeftOffset = pcParameters->m_iLeftFrmOffset      / iFactor;  // current left frame offset
  int   iTopOffset  = pcParameters->m_iTopFrmOffset       / iFactor;  // current top  frame offset

  //===== set input/output size =====
  int   iBaseField  = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ? 0 : 1 );
  int   iCurrField  = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag && pcParameters->m_bFrameMbsOnlyFlag ? 0 : 1 );
  int   iBaseW      = iRefW;
  int   iBaseH      = iRefH      >> iBaseField;
  int   iCurrW      = iGlobalW;
  int   iCurrH      = iGlobalH   >> iCurrField;
  int   iLOffset    = iLeftOffset;
  int   iTOffset    = iTopOffset >> iCurrField;
  int   iROffset    = iCurrW - iLOffset -   iOutW;
  int   iBOffset    = iCurrH - iTOffset - ( iOutH >> iCurrField );
  int   iYBorder    = ( bVerticalInterpolation ? ( bChroma ? 1 : 2 ) : 0 );

  //===== set position calculation parameters =====
  int   iScaledW    = iOutW;
  int   iScaledH    = ( ! pcParameters->m_bRefLayerFrameMbsOnlyFlag || pcParameters->m_bFrameMbsOnlyFlag ? iOutH : iOutH / 2 );
  int   iShiftX     = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefW ) );
  int   iShiftY     = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefH ) );
  int   iScaleX     = ( ( (unsigned int)iRefW << iShiftX ) + ( iScaledW >> 1 ) ) / iScaledW;
  int   iScaleY     = ( ( (unsigned int)iRefH << iShiftY ) + ( iScaledH >> 1 ) ) / iScaledH;
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    if( pcParameters->m_bRefLayerFrameMbsOnlyFlag )
    {
      iPhaseY       = iPhaseY + 4 * iBotField + ( 3 - iFactor );
      iRefPhaseY    = 2 * iRefPhaseY + 2;
    }
    else
    {
      iPhaseY       = iPhaseY    + 4 * iBotField;
      iRefPhaseY    = iRefPhaseY + 4 * iBotField;
    }
  }
  Int   iOffsetX    = iLeftOffset;
  Int   iOffsetY    = iTopOffset;
  Int   iAddX       = ( ( ( iRefW * ( 2 + iPhaseX ) ) << ( iShiftX - 2 ) ) + ( iScaledW >> 1 ) ) / iScaledW + ( 1 << ( iShiftX - 5 ) );
  Int   iAddY       = ( ( ( iRefH * ( 2 + iPhaseY ) ) << ( iShiftY - 2 ) ) + ( iScaledH >> 1 ) ) / iScaledH + ( 1 << ( iShiftY - 5 ) );
  Int   iDeltaX     = 4 * ( 2 + iRefPhaseX );
  Int   iDeltaY     = 4 * ( 2 + iRefPhaseY );
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    iOffsetY        = iTopOffset / 2;
    iAddY           = ( ( ( iRefH * ( 2 + iPhaseY ) ) << ( iShiftY - 3 ) ) + ( iScaledH >> 1 ) ) / iScaledH + ( 1 << ( iShiftY - 5 ) );
    iDeltaY         = 2 * ( 2 + iRefPhaseY );
  }

  //===== set transform block indications =====
  xDetermineTransBlkIdcs    ( iBaseW,   iBaseH,   bChroma,  bBotFlag,
                              pcParameters,       pcMbDataCtrlBase );

  //===== basic interpolation of a frame or a field =====
  xBasicResidualUpsampling  ( iBaseW,   iBaseH,   iCurrW,   iCurrH,
                              iLOffset, iTOffset, iROffset, iBOffset,
                              iShiftX,  iShiftY,  iScaleX,  iScaleY,
                              iOffsetX, iOffsetY, iAddX,    iAddY,
                              iDeltaX,  iDeltaY,  iYBorder );

  //===== vertical interpolation for second field =====
  if( bVerticalInterpolation )
  {
    xVertResidualUpsampling ( iCurrW,   iCurrH,
                              iLOffset, iTOffset, iROffset, iBOffset,
                              iYBorder, bBotFlag );
  }
}


void
DownConvert::xVertResidualUpsampling( int iBaseW, int iBaseH, int iLOffset, int iTOffset, int iROffset, int iBOffset, int iYBorder, bool bBotFlag )
{
  AOT( iYBorder < 1 );

  int iBotField     = ( bBotFlag ? 1 : 0 );
  int iCurrW        = iBaseW;
  int iCurrH        = iBaseH   << 1;
  int iCurrLOffset  = iLOffset;
  int iCurrTOffset  = iTOffset << 1;
  int iCurrROffset  = iROffset;
  int iCurrBOffset  = iBOffset << 1;

  //========== vertical upsampling ===========
  for( int j = 0; j < iCurrW; j++ )
  {
    int* piSrc = &m_paiImageBuffer[j];

    //----- upsample column -----
    for( int i = 0; i < iCurrH; i++ )
    {
      if( j < iCurrLOffset || j >= iCurrW - iCurrROffset ||
          i < iCurrTOffset || i >= iCurrH - iCurrBOffset   )
      {
        m_paiTmp1dBuffer[i] = 0; // set to zero
        continue;
      }

      if( ( i % 2 ) == iBotField )
      {
        int iSrc = ( ( i >> 1 ) + iYBorder ) * m_iImageStride;
        m_paiTmp1dBuffer[i] = piSrc[ iSrc ];
      }
      else
      {
        int iSrc = ( ( i >> 1 ) + iYBorder - iBotField ) * m_iImageStride;
        m_paiTmp1dBuffer[i] = ( piSrc[ iSrc ] + piSrc[ iSrc + m_iImageStride ] + 1 ) >> 1;
      }
    }
    //----- copy back to image buffer -----
    for( int n = 0; n < iCurrH; n++ )
    {
      piSrc[n*m_iImageStride] = m_paiTmp1dBuffer[n];
    }
  }
}


void
DownConvert::xBasicResidualUpsampling( int iBaseW,   int iBaseH,   int iCurrW,   int iCurrH,
                                       int iLOffset, int iTOffset, int iROffset, int iBOffset,
                                       int iShiftX,  int iShiftY,  int iScaleX,  int iScaleY,
                                       int iOffsetX, int iOffsetY, int iAddX,    int iAddY,
                                       int iDeltaX,  int iDeltaY,  int iYBorder )
{
  int iShiftXM4 = iShiftX - 4;
  int iShiftYM4 = iShiftY - 4;

  //========== horizontal upsampling ===========
  {
    for( int j = 0; j < iBaseH; j++ )
    {
      int   iRefPosY  = j;
      int*  piSrc     = &m_paiImageBuffer[j*m_iImageStride];
      for( int i = 0; i < iCurrW; i++ )
      {
        if( i < iLOffset || i >= iCurrW - iROffset )
        {
          m_paiTmp1dBuffer[i] = 0; // set to zero
          continue;
        }

        int iRefPosX16  = (int)( (unsigned int)( ( i - iOffsetX ) * iScaleX + iAddX ) >> iShiftXM4 ) - iDeltaX;
        int iPhase      = iRefPosX16 & 15;
        int iRefPosX    = iRefPosX16 >> 4;
        int iRefPosX0   = xClip( iRefPosX,     0, iBaseW - 1 );
        int iRefPosX1   = xClip( iRefPosX + 1, 0, iBaseW - 1 );
        int iTBlkIdc0   = m_paiTransBlkIdc[ iRefPosY * m_iImageStride + iRefPosX0 ];
        int iTBlkIdc1   = m_paiTransBlkIdc[ iRefPosY * m_iImageStride + iRefPosX1 ];

        if( iTBlkIdc0 == iTBlkIdc1 )
        {
          m_paiTmp1dBuffer[i] = ( 16 - iPhase ) * piSrc[iRefPosX0] + iPhase * piSrc[iRefPosX1];
        }
        else
        {
          m_paiTmp1dBuffer[i] = piSrc[ iPhase < 8 ? iRefPosX0 : iRefPosX1 ] << 4;
        }
      }
      //----- copy row back to image buffer -----
      memcpy( piSrc, m_paiTmp1dBuffer, iCurrW*sizeof(int) );
    }
  }

  //========== vertical upsampling ===========
  {
    for( int i = 0; i < iCurrW; i++ )
    {
      int   iRefPosX16  = (int)( (unsigned int)( ( i - iOffsetX ) * iScaleX + iAddX ) >> iShiftXM4 ) - iDeltaX;
      int   iRndPosX    = ( iRefPosX16 >> 4 ) + ( ( iRefPosX16 & 15 ) >> 3 );
      int   iRefPosX    = xClip( iRndPosX, 0, iBaseW - 1 );
      int*  piSrc       = &m_paiImageBuffer[i];
      for( int j = -iYBorder; j < iCurrH+iYBorder; j++ )
      {
        if( i < iLOffset            || i >= iCurrW - iROffset           ||
            j < iTOffset - iYBorder || j >= iCurrH - iBOffset + iYBorder  )
        {
          m_paiTmp1dBuffer[j+iYBorder] = 0; // set to zero
          continue;
        }

        int iPreShift   = ( j - iOffsetY ) * iScaleY + iAddY;
        int iPostShift  = ( j >= iOffsetY ? (int)( (unsigned int)iPreShift >> iShiftYM4 ) : ( iPreShift >> iShiftYM4 ) );
        int iRefPosY16  = iPostShift - iDeltaY;
        int iPhase      = iRefPosY16 & 15;
        int iRefPosY    = iRefPosY16 >> 4;
        int iRefPosY0   = xClip( iRefPosY,     0, iBaseH - 1 );
        int iRefPosY1   = xClip( iRefPosY + 1, 0, iBaseH - 1 );
        int iTBlkIdc0   = m_paiTransBlkIdc[ iRefPosY0 * m_iImageStride + iRefPosX ];
        int iTBlkIdc1   = m_paiTransBlkIdc[ iRefPosY1 * m_iImageStride + iRefPosX ];

        if( iTBlkIdc0 == iTBlkIdc1 )
        {
          m_paiTmp1dBuffer[j+iYBorder] = ( ( 16 - iPhase ) * piSrc[iRefPosY0*m_iImageStride] + iPhase * piSrc[iRefPosY1*m_iImageStride] + 128 ) >> 8;
        }
        else
        {
          m_paiTmp1dBuffer[j+iYBorder] = ( piSrc[ ( iPhase < 8 ? iRefPosY0 : iRefPosY1 ) * m_iImageStride ] + 8 ) >> 4;
        }
      }
      //----- copy back to image buffer -----
      for( int n = 0; n < iCurrH+2*iYBorder; n++ )
      {
        piSrc[n*m_iImageStride] = m_paiTmp1dBuffer[n];
      }
    }
  }
}

#endif


#ifdef DOWN_CONVERT_STATIC
#else
H264AVC_NAMESPACE_END
#endif


