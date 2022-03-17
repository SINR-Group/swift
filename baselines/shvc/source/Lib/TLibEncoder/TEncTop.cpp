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

/** \file     TEncTop.cpp
    \brief    encoder class
*/

#include "TLibCommon/CommonDef.h"
#include "TEncTop.h"
#include "TEncPic.h"
#if FAST_BIT_EST
#include "TLibCommon/ContextModel.h"
#endif

//! \ingroup TLibEncoder
//! \{
#if SVC_EXTENSION  
Int TEncTop::m_iSPSIdCnt = 0;
Int TEncTop::m_iPPSIdCnt = 0;
TComVPS TEncCfg::m_cVPS;
#endif

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncTop::TEncTop()
{
  m_iPOCLast          = -1;
  m_iNumPicRcvd       =  0;
  m_uiNumAllPicCoded  =  0;
  m_pppcRDSbacCoder   =  NULL;
  m_pppcBinCoderCABAC =  NULL;
  m_cRDGoOnSbacCoder.init( &m_cRDGoOnBinCoderCABAC );
#if ENC_DEC_TRACE
  g_hTrace = fopen( "TraceEnc.txt", "wb" );
  g_bJustDoIt = g_bEncDecTraceDisable;
  g_nSymbolCounter = 0;
#endif

  m_iMaxRefPicNum     = 0;

#if FAST_BIT_EST
  ContextModel::buildNextStateTable();
#endif

  m_pcSbacCoders           = NULL;
  m_pcBinCoderCABACs       = NULL;
  m_ppppcRDSbacCoders      = NULL;
  m_ppppcBinCodersCABAC    = NULL;
  m_pcRDGoOnSbacCoders     = NULL;
  m_pcRDGoOnBinCodersCABAC = NULL;
  m_pcBitCounters          = NULL;
  m_pcRdCosts              = NULL;
#if SVC_EXTENSION
  memset(m_cIlpPic, 0, sizeof(m_cIlpPic));
#if REF_IDX_MFM
  m_bMFMEnabledFlag = false;
#endif
  m_numScaledRefLayerOffsets = 0;
#if POC_RESET_FLAG || POC_RESET_IDC_ENCODER
  m_pocAdjustmentValue     = 0;
#endif
#if NO_CLRAS_OUTPUT_FLAG
  m_noClrasOutputFlag          = false;
  m_layerInitializedFlag       = false;
  m_firstPicInLayerDecodedFlag = false;
  m_noOutputOfPriorPicsFlags   = false;
#endif
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  m_pocDecrementedInDPBFlag    = false;
#endif
#endif //SVC_EXTENSION
}

TEncTop::~TEncTop()
{
#if ENC_DEC_TRACE
  fclose( g_hTrace );
#endif
}

Void TEncTop::create ()
{
#if !SVC_EXTENSION
  // initialize global variables
  initROM();
#endif

  // create processing unit classes
#if SVC_EXTENSION
  m_cGOPEncoder.        create( m_layerId );
#else
  m_cGOPEncoder.        create();
#endif
#if AUXILIARY_PICTURES
  m_cSliceEncoder.      create( getSourceWidth(), getSourceHeight(), m_chromaFormatIDC, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#else
  m_cSliceEncoder.      create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#endif
  m_cCuEncoder.         create( g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight );
  if (m_bUseSAO)
  {
#if AUXILIARY_PICTURES
    m_cEncSAO.create( getSourceWidth(), getSourceHeight(), m_chromaFormatIDC, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#else
    m_cEncSAO.create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#endif
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
    m_cEncSAO.createEncData(getSaoLcuBoundary());
#else
    m_cEncSAO.createEncData();
#endif
  }
#if ADAPTIVE_QP_SELECTION
  if (m_bUseAdaptQpSelect)
  {
    m_cTrQuant.initSliceQpDelta();
  }
#endif
  m_cLoopFilter.        create( g_uiMaxCUDepth );
  
  if ( m_RCEnableRateControl )
  {
    m_cRateCtrl.init( m_framesToBeEncoded, m_RCTargetBitrate, m_iFrameRate, m_iGOPSize, m_iSourceWidth, m_iSourceHeight,
                      g_uiMaxCUWidth, g_uiMaxCUHeight, m_RCKeepHierarchicalBit, m_RCUseLCUSeparateModel, m_GOPList );
  }

  m_pppcRDSbacCoder = new TEncSbac** [g_uiMaxCUDepth+1];
#if FAST_BIT_EST
  m_pppcBinCoderCABAC = new TEncBinCABACCounter** [g_uiMaxCUDepth+1];
#else
  m_pppcBinCoderCABAC = new TEncBinCABAC** [g_uiMaxCUDepth+1];
#endif

  for ( Int iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
  {
    m_pppcRDSbacCoder[iDepth] = new TEncSbac* [CI_NUM];
#if FAST_BIT_EST
    m_pppcBinCoderCABAC[iDepth] = new TEncBinCABACCounter* [CI_NUM];
#else
    m_pppcBinCoderCABAC[iDepth] = new TEncBinCABAC* [CI_NUM];
#endif

    for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
    {
      m_pppcRDSbacCoder[iDepth][iCIIdx] = new TEncSbac;
#if FAST_BIT_EST
      m_pppcBinCoderCABAC [iDepth][iCIIdx] = new TEncBinCABACCounter;
#else
      m_pppcBinCoderCABAC [iDepth][iCIIdx] = new TEncBinCABAC;
#endif
      m_pppcRDSbacCoder   [iDepth][iCIIdx]->init( m_pppcBinCoderCABAC [iDepth][iCIIdx] );
    }
  }

#if LAYER_CTB
  memcpy(g_auiLayerZscanToRaster[m_layerId], g_auiZscanToRaster, sizeof( g_auiZscanToRaster ) );
  memcpy(g_auiLayerRasterToZscan[m_layerId], g_auiRasterToZscan, sizeof( g_auiRasterToZscan ) );
  memcpy(g_auiLayerRasterToPelX[m_layerId],  g_auiRasterToPelX,  sizeof( g_auiRasterToPelX ) );
  memcpy(g_auiLayerRasterToPelY[m_layerId],  g_auiRasterToPelY,  sizeof( g_auiRasterToPelY ) );
#endif
}

/**
 - Allocate coders required for wavefront for the nominated number of substreams.
 .
 \param iNumSubstreams Determines how much information to allocate.
 */
Void TEncTop::createWPPCoders(Int iNumSubstreams)
{
  if (m_pcSbacCoders != NULL)
  {
    return; // already generated.
  }

  m_iNumSubstreams         = iNumSubstreams;
  m_pcSbacCoders           = new TEncSbac       [iNumSubstreams];
  m_pcBinCoderCABACs       = new TEncBinCABAC   [iNumSubstreams];
  m_pcRDGoOnSbacCoders     = new TEncSbac       [iNumSubstreams];
  m_pcRDGoOnBinCodersCABAC = new TEncBinCABAC   [iNumSubstreams];
  m_pcBitCounters          = new TComBitCounter [iNumSubstreams];
  m_pcRdCosts              = new TComRdCost     [iNumSubstreams];

  for ( UInt ui = 0 ; ui < iNumSubstreams; ui++ )
  {
    m_pcRDGoOnSbacCoders[ui].init( &m_pcRDGoOnBinCodersCABAC[ui] );
    m_pcSbacCoders[ui].init( &m_pcBinCoderCABACs[ui] );
  }

  m_ppppcRDSbacCoders      = new TEncSbac***    [iNumSubstreams];
  m_ppppcBinCodersCABAC    = new TEncBinCABAC***[iNumSubstreams];
  for ( UInt ui = 0 ; ui < iNumSubstreams ; ui++ )
  {
    m_ppppcRDSbacCoders[ui]  = new TEncSbac** [g_uiMaxCUDepth+1];
    m_ppppcBinCodersCABAC[ui]= new TEncBinCABAC** [g_uiMaxCUDepth+1];

    for ( Int iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
    {
      m_ppppcRDSbacCoders[ui][iDepth]  = new TEncSbac*     [CI_NUM];
      m_ppppcBinCodersCABAC[ui][iDepth]= new TEncBinCABAC* [CI_NUM];

      for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
      {
        m_ppppcRDSbacCoders  [ui][iDepth][iCIIdx] = new TEncSbac;
        m_ppppcBinCodersCABAC[ui][iDepth][iCIIdx] = new TEncBinCABAC;
        m_ppppcRDSbacCoders  [ui][iDepth][iCIIdx]->init( m_ppppcBinCodersCABAC[ui][iDepth][iCIIdx] );
      }
    }
  }
}

Void TEncTop::destroy ()
{
  // destroy processing unit classes
  m_cGOPEncoder.        destroy();
  m_cSliceEncoder.      destroy();
  m_cCuEncoder.         destroy();
  if (m_cSPS.getUseSAO())
  {
    m_cEncSAO.destroyEncData();
    m_cEncSAO.destroy();
  }
  m_cLoopFilter.        destroy();
  m_cRateCtrl.          destroy();

  Int iDepth;
  for ( iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
  {
    for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
    {
      delete m_pppcRDSbacCoder[iDepth][iCIIdx];
      delete m_pppcBinCoderCABAC[iDepth][iCIIdx];
    }
  }

  for ( iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
  {
    delete [] m_pppcRDSbacCoder[iDepth];
    delete [] m_pppcBinCoderCABAC[iDepth];
  }

  delete [] m_pppcRDSbacCoder;
  delete [] m_pppcBinCoderCABAC;

  for ( UInt ui = 0; ui < m_iNumSubstreams; ui++ )
  {
    for ( iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
    {
      for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
      {
        delete m_ppppcRDSbacCoders  [ui][iDepth][iCIIdx];
        delete m_ppppcBinCodersCABAC[ui][iDepth][iCIIdx];
      }
    }

    for ( iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
    {
      delete [] m_ppppcRDSbacCoders  [ui][iDepth];
      delete [] m_ppppcBinCodersCABAC[ui][iDepth];
    }
    delete[] m_ppppcRDSbacCoders  [ui];
    delete[] m_ppppcBinCodersCABAC[ui];
  }
  delete[] m_ppppcRDSbacCoders;
  delete[] m_ppppcBinCodersCABAC;
  delete[] m_pcSbacCoders;
  delete[] m_pcBinCoderCABACs;
  delete[] m_pcRDGoOnSbacCoders;  
  delete[] m_pcRDGoOnBinCodersCABAC;
  delete[] m_pcBitCounters;
  delete[] m_pcRdCosts;
  
#if SVC_EXTENSION
  for(Int i=0; i<MAX_NUM_REF; i++)
  {
    if(m_cIlpPic[i])
    {
      m_cIlpPic[i]->destroy();
      delete m_cIlpPic[i];
      m_cIlpPic[i] = NULL;
    }
  }
#else
  // destroy ROM
  destroyROM();
#endif
  return;
}

Void TEncTop::init(Bool isFieldCoding)
{
  // initialize SPS
  xInitSPS();
  
  /* set the VPS profile information */
  *m_cVPS.getPTL() = *m_cSPS.getPTL();
#if VPS_VUI_BSP_HRD_PARAMS
  m_cVPS.getTimingInfo()->setTimingInfoPresentFlag       ( true );
#else
  m_cVPS.getTimingInfo()->setTimingInfoPresentFlag       ( false );
#endif
  // initialize PPS
  m_cPPS.setSPS(&m_cSPS);
  xInitPPS();
  xInitRPS(isFieldCoding);

  xInitPPSforTiles();

  // initialize processing unit classes
  m_cGOPEncoder.  init( this );
  m_cSliceEncoder.init( this );
  m_cCuEncoder.   init( this );
  
  // initialize transform & quantization class
  m_pcCavlcCoder = getCavlcCoder();
  
  m_cTrQuant.init( 1 << m_uiQuadtreeTULog2MaxSize,
                  m_useRDOQ, 
                  m_useRDOQTS,
                  true 
                  ,m_useTransformSkipFast
#if ADAPTIVE_QP_SELECTION                  
                  , m_bUseAdaptQpSelect
#endif
                  );
  
  // initialize encoder search class
  m_cSearch.init( this, &m_cTrQuant, m_iSearchRange, m_bipredSearchRange, m_iFastSearch, 0, &m_cEntropyCoder, &m_cRdCost, getRDSbacCoder(), getRDGoOnSbacCoder() );

  m_iMaxRefPicNum = 0;
#if SVC_EXTENSION
  m_iSPSIdCnt ++;
  m_iPPSIdCnt ++;
  xInitILRP();
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncTop::deletePicBuffer()
{
  TComList<TComPic*>::iterator iterPic = m_cListPic.begin();
  Int iSize = Int( m_cListPic.size() );
  
  for ( Int i = 0; i < iSize; i++ )
  {
    TComPic* pcPic = *(iterPic++);
    
    pcPic->destroy();
    delete pcPic;
    pcPic = NULL;
  }
}

/**
 - Application has picture buffer list with size of GOP + 1
 - Picture buffer list acts like as ring buffer
 - End of the list has the latest picture
 .
 \param   flush               cause encoder to encode a partial GOP
 \param   pcPicYuvOrg         original YUV picture
 \retval  rcListPicYuvRecOut  list of reconstruction YUV pictures
 \retval  rcListBitstreamOut  list of output bitstreams
 \retval  iNumEncoded         number of encoded pictures
 */
#if SVC_EXTENSION
Void TEncTop::encode( TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut, Int iPicIdInGOP )
{
  // compress GOP
#if !RC_SHVC_HARMONIZATION
  if ( m_RCEnableRateControl )
  {
    m_cRateCtrl.initRCGOP( m_iNumPicRcvd );
  }
#endif

  // compress GOP
  m_cGOPEncoder.compressGOP(iPicIdInGOP, m_iPOCLast, m_iNumPicRcvd, m_cListPic, rcListPicYuvRecOut, accessUnitsOut, false, false);

#if !RC_SHVC_HARMONIZATION
  if ( m_RCEnableRateControl )
  {
    m_cRateCtrl.destroyRCGOP();
  }
#endif
  
  m_uiNumAllPicCoded ++;
}

Void TEncTop::encodePrep( TComPicYuv* pcPicYuvOrg )
{
  if (pcPicYuvOrg) 
  {
    // get original YUV
    TComPic* pcPicCurr = NULL;
    xGetNewPicBuffer( pcPicCurr );
    pcPicYuvOrg->copyToPic( pcPicCurr->getPicYuvOrg() );

    // compute image characteristics
    if ( getUseAdaptiveQP() )
    {
      m_cPreanalyzer.xPreanalyze( dynamic_cast<TEncPic*>( pcPicCurr ) );
    }
  }
}
#else
Void TEncTop::encode(Bool flush, TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut, Int& iNumEncoded )
{
  if (pcPicYuvOrg) {
    // get original YUV
    TComPic* pcPicCurr = NULL;
    xGetNewPicBuffer( pcPicCurr );
    pcPicYuvOrg->copyToPic( pcPicCurr->getPicYuvOrg() );

    // compute image characteristics
    if ( getUseAdaptiveQP() )
    {
      m_cPreanalyzer.xPreanalyze( dynamic_cast<TEncPic*>( pcPicCurr ) );
    }
  }
  
  if (!m_iNumPicRcvd || (!flush && m_iPOCLast != 0 && m_iNumPicRcvd != m_iGOPSize && m_iGOPSize))
  {
    iNumEncoded = 0;
    return;
  }
  
  if ( m_RCEnableRateControl )
  {
    m_cRateCtrl.initRCGOP( m_iNumPicRcvd );
  }

  // compress GOP

  m_cGOPEncoder.compressGOP(m_iPOCLast, m_iNumPicRcvd, m_cListPic, rcListPicYuvRecOut, accessUnitsOut, false, false);

  if ( m_RCEnableRateControl )
  {
    m_cRateCtrl.destroyRCGOP();
  }
  
  iNumEncoded         = m_iNumPicRcvd;
  m_iNumPicRcvd       = 0;
  m_uiNumAllPicCoded += iNumEncoded;
}
#endif

/**------------------------------------------------
 Separate interlaced frame into two fields
 -------------------------------------------------**/
void separateFields(Pel* org, Pel* dstField, UInt stride, UInt width, UInt height, Bool isTop)
{
  if (!isTop)
  {
    org += stride;
  }
  for (Int y = 0; y < height>>1; y++)
  {
    for (Int x = 0; x < width; x++)
    {
      dstField[x] = org[x];
    }
    
    dstField += stride;
    org += stride*2;
  }
  
}

#if SVC_EXTENSION
Void TEncTop::encodePrep( TComPicYuv* pcPicYuvOrg, Bool isTff )
{
  if (pcPicYuvOrg)
  {
    /* -- TOP FIELD -- */
    /* -- Top field initialization -- */
    
    TComPic *pcTopField;
    xGetNewPicBuffer( pcTopField );
    pcTopField->setReconMark (false);
    
    pcTopField->getSlice(0)->setPOC( m_iPOCLast );
    pcTopField->getPicYuvRec()->setBorderExtension(false);
    pcTopField->setTopField(isTff);
    
    Int nHeight = pcPicYuvOrg->getHeight();
    Int nWidth = pcPicYuvOrg->getWidth();
    Int nStride = pcPicYuvOrg->getStride();
    Int nPadLuma = pcPicYuvOrg->getLumaMargin();
    Int nPadChroma = pcPicYuvOrg->getChromaMargin();
    
    // Get pointers
    Pel * PicBufY = pcPicYuvOrg->getBufY();
    Pel * PicBufU = pcPicYuvOrg->getBufU();
    Pel * PicBufV = pcPicYuvOrg->getBufV();
    
    Pel * pcTopFieldY =  pcTopField->getPicYuvOrg()->getLumaAddr();
    Pel * pcTopFieldU =  pcTopField->getPicYuvOrg()->getCbAddr();
    Pel * pcTopFieldV =  pcTopField->getPicYuvOrg()->getCrAddr();

    // compute image characteristics
    if ( getUseAdaptiveQP() )
    {
      m_cPreanalyzer.xPreanalyze( dynamic_cast<TEncPic*>( pcTopField ) );
    }    

    /* -- Defield -- */
    
    Bool isTop = isTff;
    
    separateFields(PicBufY + nPadLuma + nStride*nPadLuma, pcTopFieldY, nStride, nWidth, nHeight, isTop);
    separateFields(PicBufU + nPadChroma + (nStride >> 1)*nPadChroma, pcTopFieldU, nStride >> 1, nWidth >> 1, nHeight >> 1, isTop);
    separateFields(PicBufV + nPadChroma + (nStride >> 1)*nPadChroma, pcTopFieldV, nStride >> 1, nWidth >> 1, nHeight >> 1, isTop);

    /* -- BOTTOM FIELD -- */
    /* -- Bottom field initialization -- */
    
    TComPic* pcBottomField;
    xGetNewPicBuffer( pcBottomField );
    pcBottomField->setReconMark (false);
   
    pcBottomField->getSlice(0)->setPOC( m_iPOCLast);
    pcBottomField->getPicYuvRec()->setBorderExtension(false);
    pcBottomField->setTopField(!isTff);
    
    nHeight = pcPicYuvOrg->getHeight();
    nWidth = pcPicYuvOrg->getWidth();
    nStride = pcPicYuvOrg->getStride();
    nPadLuma = pcPicYuvOrg->getLumaMargin();
    nPadChroma = pcPicYuvOrg->getChromaMargin();
    
    // Get pointers
    PicBufY = pcPicYuvOrg->getBufY();
    PicBufU = pcPicYuvOrg->getBufU();
    PicBufV = pcPicYuvOrg->getBufV();
    
    Pel * pcBottomFieldY =  pcBottomField->getPicYuvOrg()->getLumaAddr();
    Pel * pcBottomFieldU =  pcBottomField->getPicYuvOrg()->getCbAddr();
    Pel * pcBottomFieldV =  pcBottomField->getPicYuvOrg()->getCrAddr();

    // compute image characteristics
    if ( getUseAdaptiveQP() )
    {
      m_cPreanalyzer.xPreanalyze( dynamic_cast<TEncPic*>( pcBottomField ) );
    }       

    /* -- Defield -- */
    
    isTop = !isTff;
    
    separateFields(PicBufY + nPadLuma + nStride*nPadLuma, pcBottomFieldY, nStride, nWidth, nHeight, isTop);
    separateFields(PicBufU + nPadChroma + (nStride >> 1)*nPadChroma, pcBottomFieldU, nStride >> 1, nWidth >> 1, nHeight >> 1, isTop);
    separateFields(PicBufV + nPadChroma + (nStride >> 1)*nPadChroma, pcBottomFieldV, nStride >> 1, nWidth >> 1, nHeight >> 1, isTop);
    
  }
}

Void TEncTop::encode( TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut, Int iPicIdInGOP, Bool isTff )
{
  // compress GOP
  if (m_iPOCLast == 0) // compress field 0
  {
    m_cGOPEncoder.compressGOP(iPicIdInGOP, m_iPOCLast, m_iNumPicRcvd, m_cListPic, rcListPicYuvRecOut, accessUnitsOut, true, isTff);
  }

  if (pcPicYuvOrg)
  {
    TComPicYuv* rpcPicYuvRec = new TComPicYuv;
    if ( rcListPicYuvRecOut.size() == (UInt)m_iGOPSize )
    {
      rpcPicYuvRec = rcListPicYuvRecOut.popFront();
    }
    else
    {
#if AUXILIARY_PICTURES
      rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, m_chromaFormatIDC, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#else
      rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#endif
    }
    rcListPicYuvRecOut.pushBack( rpcPicYuvRec );
  }
  
  // compress GOP
  m_cGOPEncoder.compressGOP(iPicIdInGOP, m_iPOCLast, m_iNumPicRcvd, m_cListPic, rcListPicYuvRecOut, accessUnitsOut, true, isTff);

  m_uiNumAllPicCoded ++;
}
#else
Void TEncTop::encode(Bool flush, TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut, Int& iNumEncoded, Bool isTff)
{
  /* -- TOP FIELD -- */
  
  if (pcPicYuvOrg)
  {
    
    /* -- Top field initialization -- */
    
    TComPic *pcTopField;
    xGetNewPicBuffer( pcTopField );
    pcTopField->setReconMark (false);
    
    pcTopField->getSlice(0)->setPOC( m_iPOCLast );
    pcTopField->getPicYuvRec()->setBorderExtension(false);
    pcTopField->setTopField(isTff);
    
    Int nHeight = pcPicYuvOrg->getHeight();
    Int nWidth = pcPicYuvOrg->getWidth();
    Int nStride = pcPicYuvOrg->getStride();
    Int nPadLuma = pcPicYuvOrg->getLumaMargin();
    Int nPadChroma = pcPicYuvOrg->getChromaMargin();
    
    // Get pointers
    Pel * PicBufY = pcPicYuvOrg->getBufY();
    Pel * PicBufU = pcPicYuvOrg->getBufU();
    Pel * PicBufV = pcPicYuvOrg->getBufV();
    
    Pel * pcTopFieldY =  pcTopField->getPicYuvOrg()->getLumaAddr();
    Pel * pcTopFieldU =  pcTopField->getPicYuvOrg()->getCbAddr();
    Pel * pcTopFieldV =  pcTopField->getPicYuvOrg()->getCrAddr();
    
    // compute image characteristics
    if ( getUseAdaptiveQP() )
    {
      m_cPreanalyzer.xPreanalyze( dynamic_cast<TEncPic*>( pcTopField ) );
    }
    
    /* -- Defield -- */
    
    Bool isTop = isTff;
    
    separateFields(PicBufY + nPadLuma + nStride*nPadLuma, pcTopFieldY, nStride, nWidth, nHeight, isTop);
    separateFields(PicBufU + nPadChroma + (nStride >> 1)*nPadChroma, pcTopFieldU, nStride >> 1, nWidth >> 1, nHeight >> 1, isTop);
    separateFields(PicBufV + nPadChroma + (nStride >> 1)*nPadChroma, pcTopFieldV, nStride >> 1, nWidth >> 1, nHeight >> 1, isTop);
    
  }
  
  if (m_iPOCLast == 0) // compress field 0
  {
    m_cGOPEncoder.compressGOP(m_iPOCLast, m_iNumPicRcvd, m_cListPic, rcListPicYuvRecOut, accessUnitsOut, true, isTff);
  }
  
  /* -- BOTTOM FIELD -- */
  
  if (pcPicYuvOrg)
  {
    
    /* -- Bottom field initialization -- */
    
    TComPic* pcBottomField;
    xGetNewPicBuffer( pcBottomField );
    pcBottomField->setReconMark (false);
    
    TComPicYuv* rpcPicYuvRec;
    if ( rcListPicYuvRecOut.size() == (UInt)m_iGOPSize )
    {
      rpcPicYuvRec = rcListPicYuvRecOut.popFront();
    }
    else
    {
      rpcPicYuvRec = new TComPicYuv;
      rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
    }
    rcListPicYuvRecOut.pushBack( rpcPicYuvRec );
    
    pcBottomField->getSlice(0)->setPOC( m_iPOCLast);
    pcBottomField->getPicYuvRec()->setBorderExtension(false);
    pcBottomField->setTopField(!isTff);
    
    int nHeight = pcPicYuvOrg->getHeight();
    int nWidth = pcPicYuvOrg->getWidth();
    int nStride = pcPicYuvOrg->getStride();
    int nPadLuma = pcPicYuvOrg->getLumaMargin();
    int nPadChroma = pcPicYuvOrg->getChromaMargin();
    
    // Get pointers
    Pel * PicBufY = pcPicYuvOrg->getBufY();
    Pel * PicBufU = pcPicYuvOrg->getBufU();
    Pel * PicBufV = pcPicYuvOrg->getBufV();
    
    Pel * pcBottomFieldY =  pcBottomField->getPicYuvOrg()->getLumaAddr();
    Pel * pcBottomFieldU =  pcBottomField->getPicYuvOrg()->getCbAddr();
    Pel * pcBottomFieldV =  pcBottomField->getPicYuvOrg()->getCrAddr();
    
    // Compute image characteristics
    if ( getUseAdaptiveQP() )
    {
      m_cPreanalyzer.xPreanalyze( dynamic_cast<TEncPic*>( pcBottomField ) );
    }
    
    /* -- Defield -- */
    
    Bool isTop = !isTff;
    
    separateFields(PicBufY + nPadLuma + nStride*nPadLuma, pcBottomFieldY, nStride, nWidth, nHeight, isTop);
    separateFields(PicBufU + nPadChroma + (nStride >> 1)*nPadChroma, pcBottomFieldU, nStride >> 1, nWidth >> 1, nHeight >> 1, isTop);
    separateFields(PicBufV + nPadChroma + (nStride >> 1)*nPadChroma, pcBottomFieldV, nStride >> 1, nWidth >> 1, nHeight >> 1, isTop);
    
  }
  
  if ( ( !(m_iNumPicRcvd) || (!flush && m_iPOCLast != 1 && m_iNumPicRcvd != m_iGOPSize && m_iGOPSize)) )
  {
    iNumEncoded = 0;
    return;
  }
  
  // compress GOP
  m_cGOPEncoder.compressGOP(m_iPOCLast, m_iNumPicRcvd, m_cListPic, rcListPicYuvRecOut, accessUnitsOut, true, isTff);
  
  iNumEncoded = m_iNumPicRcvd;
  m_iNumPicRcvd = 0;
  m_uiNumAllPicCoded += iNumEncoded;
}
#endif

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - Application has picture buffer list with size of GOP + 1
 - Picture buffer list acts like as ring buffer
 - End of the list has the latest picture
 .
 \retval rpcPic obtained picture buffer
 */
Void TEncTop::xGetNewPicBuffer ( TComPic*& rpcPic )
{
  TComSlice::sortPicList(m_cListPic);
  
  if (m_cListPic.size() >= (UInt)(m_iGOPSize + getMaxDecPicBuffering(MAX_TLAYER-1) + 2) )
  {
    TComList<TComPic*>::iterator iterPic  = m_cListPic.begin();
    Int iSize = Int( m_cListPic.size() );
    for ( Int i = 0; i < iSize; i++ )
    {
      rpcPic = *(iterPic++);
      if(rpcPic->getSlice(0)->isReferenced() == false)
      {
        break;
      }
    }
  }
  else
  {
    if ( getUseAdaptiveQP() )
    {
      TEncPic* pcEPic = new TEncPic;

#if SVC_EXTENSION //Temporal solution, should be modified
      if(m_layerId > 0)
      {
        for(UInt i = 0; i < m_cVPS.getNumDirectRefLayers( m_layerId ); i++ )
        {
#if MOVE_SCALED_OFFSET_TO_PPS
#if O0098_SCALED_REF_LAYER_ID
          const Window scalEL = getPPS()->getScaledRefLayerWindowForLayer(m_cVPS.getRefLayerId(m_layerId, i));
#else
          const Window scalEL = getPPS()->getScaledRefLayerWindow(i);
#endif
#else
#if O0098_SCALED_REF_LAYER_ID
          const Window scalEL = getSPS()->getScaledRefLayerWindowForLayer(m_cVPS.getRefLayerId(m_layerId, i));
#else
          const Window scalEL = getSPS()->getScaledRefLayerWindow(i);
#endif
#endif
#if REF_REGION_OFFSET
          const Window altRL  = getPPS()->getRefLayerWindowForLayer(m_cVPS.getRefLayerId(m_layerId, i));
#if RESAMPLING_FIX
          Bool equalOffsets = scalEL.hasEqualOffset(altRL);
#if R0209_GENERIC_PHASE
          Bool zeroPhase = getPPS()->hasZeroResamplingPhase(m_cVPS.getRefLayerId(m_layerId, i));
#endif
#else
          Bool zeroOffsets = ( scalEL.getWindowLeftOffset() == 0 && scalEL.getWindowRightOffset() == 0 && scalEL.getWindowTopOffset() == 0 && scalEL.getWindowBottomOffset() == 0
                               && altRL.getWindowLeftOffset() == 0 && altRL.getWindowRightOffset() == 0 && altRL.getWindowTopOffset() == 0 && altRL.getWindowBottomOffset() == 0);
#endif
#else
          Bool zeroOffsets = ( scalEL.getWindowLeftOffset() == 0 && scalEL.getWindowRightOffset() == 0 && scalEL.getWindowTopOffset() == 0 && scalEL.getWindowBottomOffset() == 0 );
#endif

#if VPS_EXTN_DIRECT_REF_LAYERS
          TEncTop *pcEncTopBase = (TEncTop *)getRefLayerEnc( i );
#else
          TEncTop *pcEncTopBase = (TEncTop *)getLayerEnc( m_layerId-1 );
#endif
#if O0194_DIFFERENT_BITDEPTH_EL_BL
          UInt refLayerId = m_cVPS.getRefLayerId(m_layerId, i);
          Bool sameBitDepths = ( g_bitDepthYLayer[m_layerId] == g_bitDepthYLayer[refLayerId] ) && ( g_bitDepthCLayer[m_layerId] == g_bitDepthCLayer[refLayerId] );

#if REF_IDX_MFM
          if( m_iSourceWidth == pcEncTopBase->getSourceWidth() && m_iSourceHeight == pcEncTopBase->getSourceHeight() && equalOffsets && zeroPhase )
          {
            pcEPic->setEqualPictureSizeAndOffsetFlag( i, true );
          }

          if( !pcEPic->equalPictureSizeAndOffsetFlag(i) || !sameBitDepths 
#else
          if( m_iSourceWidth != pcEncTopBase->getSourceWidth() || m_iSourceHeight != pcEncTopBase->getSourceHeight() || !sameBitDepths 
#if REF_REGION_OFFSET && RESAMPLING_FIX
            || !equalOffsets
#if R0209_GENERIC_PHASE
            || !zeroPhase
#endif
#else
            || !zeroOffsets
#endif
#endif
#if Q0048_CGS_3D_ASYMLUT
            || m_cPPS.getCGSFlag() > 0
#endif
#if LAYER_CTB
            || pcEncTopBase->getSPS()->getMaxCUWidth() != m_cSPS.getMaxCUWidth() || pcEncTopBase->getSPS()->getMaxCUHeight() != m_cSPS.getMaxCUHeight() || pcEncTopBase->getSPS()->getMaxCUDepth() != m_cSPS.getMaxCUDepth()
#endif
            )
#else
          if(m_iSourceWidth != pcEncTopBase->getSourceWidth() || m_iSourceHeight != pcEncTopBase->getSourceHeight()
#if REF_REGION_OFFSET && RESAMPLING_FIX
            || !equalOffsets
#if R0209_GENERIC_PHASE
            || !zeroPhase
#endif
#else
            || !zeroOffsets
#endif
          )
#endif
          {
            pcEPic->setSpatialEnhLayerFlag( i, true );

            //only for scalable extension
            assert( m_cVPS.getScalabilityMask( SCALABILITY_ID ) == true );
          }
        }
      }
#endif

#if SVC_EXTENSION
#if AUXILIARY_PICTURES
      pcEPic->create( m_iSourceWidth, m_iSourceHeight, m_chromaFormatIDC, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, m_cPPS.getMaxCuDQPDepth()+1 ,
                      m_conformanceWindow, m_defaultDisplayWindow, m_numReorderPics, &m_cSPS);
#else
      pcEPic->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, m_cPPS.getMaxCuDQPDepth()+1 ,
                      m_conformanceWindow, m_defaultDisplayWindow, m_numReorderPics, &m_cSPS);
#endif
#else  //SVC_EXTENSION
      pcEPic->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, m_cPPS.getMaxCuDQPDepth()+1 ,
                      m_conformanceWindow, m_defaultDisplayWindow, m_numReorderPics);
#endif //SVC_EXTENSION
      rpcPic = pcEPic;
    }
    else
    {
      rpcPic = new TComPic;

#if SVC_EXTENSION //Temporal solution, should be modified
      if(m_layerId > 0)
      {
        for(UInt i = 0; i < m_cVPS.getNumDirectRefLayers( m_layerId ); i++ )
        {
#if MOVE_SCALED_OFFSET_TO_PPS
#if O0098_SCALED_REF_LAYER_ID
          const Window scalEL = getPPS()->getScaledRefLayerWindowForLayer(m_cVPS.getRefLayerId(m_layerId, i));
#else
          const Window scalEL = getPPS()->getScaledRefLayerWindow(i);
#endif
#else
#if O0098_SCALED_REF_LAYER_ID
          const Window scalEL = getSPS()->getScaledRefLayerWindowForLayer(m_cVPS.getRefLayerId(m_layerId, i));
#else
          const Window scalEL = getSPS()->getScaledRefLayerWindow(i);
#endif
#endif
#if REF_REGION_OFFSET
          const Window altRL  = getPPS()->getRefLayerWindowForLayer(m_cVPS.getRefLayerId(m_layerId, i));
#if RESAMPLING_FIX
          Bool equalOffsets = scalEL.hasEqualOffset(altRL);
#if R0209_GENERIC_PHASE
          Bool zeroPhase = getPPS()->hasZeroResamplingPhase(m_cVPS.getRefLayerId(m_layerId, i));
#endif
#else
          Bool zeroOffsets = ( scalEL.getWindowLeftOffset() == 0 && scalEL.getWindowRightOffset() == 0 && scalEL.getWindowTopOffset() == 0 && scalEL.getWindowBottomOffset() == 0
                               && altRL.getWindowLeftOffset() == 0 && altRL.getWindowRightOffset() == 0 && altRL.getWindowTopOffset() == 0 && altRL.getWindowBottomOffset() == 0);
#endif
#else
          Bool zeroOffsets = ( scalEL.getWindowLeftOffset() == 0 && scalEL.getWindowRightOffset() == 0 && scalEL.getWindowTopOffset() == 0 && scalEL.getWindowBottomOffset() == 0 );
#endif

#if VPS_EXTN_DIRECT_REF_LAYERS
          TEncTop *pcEncTopBase = (TEncTop *)getRefLayerEnc( i );
#else
          TEncTop *pcEncTopBase = (TEncTop *)getLayerEnc( m_layerId-1 );
#endif
#if O0194_DIFFERENT_BITDEPTH_EL_BL
          UInt refLayerId = m_cVPS.getRefLayerId(m_layerId, i);
          Bool sameBitDepths = ( g_bitDepthYLayer[m_layerId] == g_bitDepthYLayer[refLayerId] ) && ( g_bitDepthCLayer[m_layerId] == g_bitDepthCLayer[refLayerId] );

          if( m_iSourceWidth != pcEncTopBase->getSourceWidth() || m_iSourceHeight != pcEncTopBase->getSourceHeight() || !sameBitDepths 
#if REF_REGION_OFFSET && RESAMPLING_FIX
            || !equalOffsets 
#if R0209_GENERIC_PHASE
            || !zeroPhase
#endif
#else
            || !zeroOffsets 
#endif
#if Q0048_CGS_3D_ASYMLUT
            || m_cPPS.getCGSFlag() > 0
#endif
#if LAYER_CTB
            || pcEncTopBase->getSPS()->getMaxCUWidth() != m_cSPS.getMaxCUWidth() || pcEncTopBase->getSPS()->getMaxCUHeight() != m_cSPS.getMaxCUHeight() || pcEncTopBase->getSPS()->getMaxCUDepth() != m_cSPS.getMaxCUDepth()
#endif
)
#else
          if(m_iSourceWidth != pcEncTopBase->getSourceWidth() || m_iSourceHeight != pcEncTopBase->getSourceHeight()
#if REF_REGION_OFFSET && RESAMPLING_FIX
            || !equalOffsets 
#if R0209_GENERIC_PHASE
            || !zeroPhase
#endif
#else
            || !zeroOffsets 
#endif
          )
#endif
          {
            rpcPic->setSpatialEnhLayerFlag( i, true );

            //only for scalable extension
            assert( m_cVPS.getScalabilityMask( SCALABILITY_ID ) == true );
          }
        }
      }
#endif

#if SVC_EXTENSION
#if AUXILIARY_PICTURES
      rpcPic->create( m_iSourceWidth, m_iSourceHeight, m_chromaFormatIDC, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, 
                      m_conformanceWindow, m_defaultDisplayWindow, m_numReorderPics, &m_cSPS);
#else
      rpcPic->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, 
                      m_conformanceWindow, m_defaultDisplayWindow, m_numReorderPics, &m_cSPS);
#endif
#else  //SVC_EXTENSION
      rpcPic->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, 
                      m_conformanceWindow, m_defaultDisplayWindow, m_numReorderPics);
#endif //SVC_EXTENSION
    }
    m_cListPic.pushBack( rpcPic );
  }
  rpcPic->setReconMark (false);
  
  m_iPOCLast++;
  m_iNumPicRcvd++;
  
  rpcPic->getSlice(0)->setPOC( m_iPOCLast );
  // mark it should be extended
  rpcPic->getPicYuvRec()->setBorderExtension(false);
}

Void TEncTop::xInitSPS()
{
#if SVC_EXTENSION
  m_cSPS.setExtensionFlag( m_layerId > 0 ? true : false );
#if R0042_PROFILE_INDICATION
  m_cSPS.setNumDirectRefLayers(m_numAddLayerSets);
#endif
#if Q0078_ADD_LAYER_SETS
  if( !m_numDirectRefLayers && m_numAddLayerSets )
  {
    m_cSPS.setLayerId(0); // layer ID 0 for independent layers
  }
  else
  {
    m_cSPS.setLayerId(m_layerId);
  }
#else
  m_cSPS.setLayerId(m_layerId);
#endif
#if !MOVE_SCALED_OFFSET_TO_PPS
  m_cSPS.setNumScaledRefLayerOffsets(m_numScaledRefLayerOffsets);
  for(Int i = 0; i < m_cSPS.getNumScaledRefLayerOffsets(); i++)
  {
#if O0098_SCALED_REF_LAYER_ID
    m_cSPS.setScaledRefLayerId(i, m_scaledRefLayerId[i]);
#endif
    m_cSPS.getScaledRefLayerWindow(i) = m_scaledRefLayerWindow[i];
#if P0312_VERT_PHASE_ADJ
    m_cSPS.setVertPhasePositionEnableFlag( m_scaledRefLayerId[i], m_scaledRefLayerWindow[i].getVertPhasePositionEnableFlag() );
#endif
  }
#endif
#endif //SVC_EXTENSION
  ProfileTierLevel& profileTierLevel = *m_cSPS.getPTL()->getGeneralPTL();
  profileTierLevel.setLevelIdc(m_level);
  profileTierLevel.setTierFlag(m_levelTier);
  profileTierLevel.setProfileIdc(m_profile);
  profileTierLevel.setProfileCompatibilityFlag(m_profile, 1);
  profileTierLevel.setProgressiveSourceFlag(m_progressiveSourceFlag);
  profileTierLevel.setInterlacedSourceFlag(m_interlacedSourceFlag);
  profileTierLevel.setNonPackedConstraintFlag(m_nonPackedConstraintFlag);
  profileTierLevel.setFrameOnlyConstraintFlag(m_frameOnlyConstraintFlag);
  
  if (m_profile == Profile::MAIN10 && g_bitDepthY == 8 && g_bitDepthC == 8)
  {
    /* The above constraint is equal to Profile::MAIN */
    profileTierLevel.setProfileCompatibilityFlag(Profile::MAIN, 1);
  }
  if (m_profile == Profile::MAIN)
  {
    /* A Profile::MAIN10 decoder can always decode Profile::MAIN */
    profileTierLevel.setProfileCompatibilityFlag(Profile::MAIN10, 1);
  }
  /* XXX: should Main be marked as compatible with still picture? */
  /* XXX: may be a good idea to refactor the above into a function
   * that chooses the actual compatibility based upon options */

  m_cSPS.setPicWidthInLumaSamples         ( m_iSourceWidth      );
  m_cSPS.setPicHeightInLumaSamples        ( m_iSourceHeight     );
  m_cSPS.setConformanceWindow             ( m_conformanceWindow );
  m_cSPS.setMaxCUWidth    ( g_uiMaxCUWidth      );
  m_cSPS.setMaxCUHeight   ( g_uiMaxCUHeight     );
  m_cSPS.setMaxCUDepth    ( g_uiMaxCUDepth      );
#if AUXILIARY_PICTURES
  m_cSPS.setChromaFormatIdc( m_chromaFormatIDC);
#endif

  Int minCUSize = m_cSPS.getMaxCUWidth() >> ( m_cSPS.getMaxCUDepth()-g_uiAddCUDepth );
  Int log2MinCUSize = 0;
  while(minCUSize > 1)
  {
    minCUSize >>= 1;
    log2MinCUSize++;
  }

  m_cSPS.setLog2MinCodingBlockSize(log2MinCUSize);
  m_cSPS.setLog2DiffMaxMinCodingBlockSize(m_cSPS.getMaxCUDepth()-g_uiAddCUDepth);
#if SVC_EXTENSION
  m_cSPS.setSPSId         ( m_iSPSIdCnt       );
#endif
  
  m_cSPS.setPCMLog2MinSize (m_uiPCMLog2MinSize);
  m_cSPS.setUsePCM        ( m_usePCM           );
  m_cSPS.setPCMLog2MaxSize( m_pcmLog2MaxSize  );

  m_cSPS.setQuadtreeTULog2MaxSize( m_uiQuadtreeTULog2MaxSize );
  m_cSPS.setQuadtreeTULog2MinSize( m_uiQuadtreeTULog2MinSize );
  m_cSPS.setQuadtreeTUMaxDepthInter( m_uiQuadtreeTUMaxDepthInter    );
  m_cSPS.setQuadtreeTUMaxDepthIntra( m_uiQuadtreeTUMaxDepthIntra    );
  
  m_cSPS.setTMVPFlagsPresent(false);

  m_cSPS.setMaxTrSize   ( 1 << m_uiQuadtreeTULog2MaxSize );
  
  Int i;
  
  for (i = 0; i < g_uiMaxCUDepth-g_uiAddCUDepth; i++ )
  {
    m_cSPS.setAMPAcc( i, m_useAMP );
    //m_cSPS.setAMPAcc( i, 1 );
  }

  m_cSPS.setUseAMP ( m_useAMP );

  for (i = g_uiMaxCUDepth-g_uiAddCUDepth; i < g_uiMaxCUDepth; i++ )
  {
    m_cSPS.setAMPAcc(i, 0);
  }

#if REPN_FORMAT_IN_VPS
  m_cSPS.setBitDepthY( m_cVPS.getVpsRepFormat( m_cVPS.getVpsRepFormatIdx( m_layerId ) )->getBitDepthVpsLuma() );
  m_cSPS.setBitDepthC( m_cVPS.getVpsRepFormat( m_cVPS.getVpsRepFormatIdx( m_layerId ) )->getBitDepthVpsChroma()  );

  m_cSPS.setQpBDOffsetY ( 6*(m_cVPS.getVpsRepFormat( m_cVPS.getVpsRepFormatIdx( m_layerId ) )->getBitDepthVpsLuma()  - 8) );
  m_cSPS.setQpBDOffsetC ( 6*(m_cVPS.getVpsRepFormat( m_cVPS.getVpsRepFormatIdx( m_layerId ) )->getBitDepthVpsChroma()  - 8) );
#else
  m_cSPS.setBitDepthY( g_bitDepthY );
  m_cSPS.setBitDepthC( g_bitDepthC );

  m_cSPS.setQpBDOffsetY ( 6*(g_bitDepthY - 8) );
  m_cSPS.setQpBDOffsetC ( 6*(g_bitDepthC - 8) );
#endif

  m_cSPS.setUseSAO( m_bUseSAO );

  m_cSPS.setMaxTLayers( m_maxTempLayer );
  m_cSPS.setTemporalIdNestingFlag( ( m_maxTempLayer == 1 ) ? true : false );
  for ( i = 0; i < min(m_cSPS.getMaxTLayers(),(UInt) MAX_TLAYER); i++ )
  {
    m_cSPS.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
    m_cSPS.setNumReorderPics(m_numReorderPics[i], i);
  }
  m_cSPS.setPCMBitDepthLuma (g_uiPCMBitDepthLuma);
  m_cSPS.setPCMBitDepthChroma (g_uiPCMBitDepthChroma);
  m_cSPS.setPCMFilterDisableFlag  ( m_bPCMFilterDisableFlag );

  m_cSPS.setScalingListFlag ( (m_useScalingListId == 0) ? 0 : 1 );

  m_cSPS.setUseStrongIntraSmoothing( m_useStrongIntraSmoothing );

  m_cSPS.setVuiParametersPresentFlag(getVuiParametersPresentFlag());
  if (m_cSPS.getVuiParametersPresentFlag())
  {
    TComVUI* pcVUI = m_cSPS.getVuiParameters();
    pcVUI->setAspectRatioInfoPresentFlag(getAspectRatioInfoPresentFlag());
    pcVUI->setAspectRatioIdc(getAspectRatioIdc());
    pcVUI->setSarWidth(getSarWidth());
    pcVUI->setSarHeight(getSarHeight());
    pcVUI->setOverscanInfoPresentFlag(getOverscanInfoPresentFlag());
    pcVUI->setOverscanAppropriateFlag(getOverscanAppropriateFlag());
    pcVUI->setVideoSignalTypePresentFlag(getVideoSignalTypePresentFlag());
    pcVUI->setVideoFormat(getVideoFormat());
    pcVUI->setVideoFullRangeFlag(getVideoFullRangeFlag());
    pcVUI->setColourDescriptionPresentFlag(getColourDescriptionPresentFlag());
    pcVUI->setColourPrimaries(getColourPrimaries());
    pcVUI->setTransferCharacteristics(getTransferCharacteristics());
    pcVUI->setMatrixCoefficients(getMatrixCoefficients());
    pcVUI->setChromaLocInfoPresentFlag(getChromaLocInfoPresentFlag());
    pcVUI->setChromaSampleLocTypeTopField(getChromaSampleLocTypeTopField());
    pcVUI->setChromaSampleLocTypeBottomField(getChromaSampleLocTypeBottomField());
    pcVUI->setNeutralChromaIndicationFlag(getNeutralChromaIndicationFlag());
    pcVUI->setDefaultDisplayWindow(getDefaultDisplayWindow());
    pcVUI->setFrameFieldInfoPresentFlag(getFrameFieldInfoPresentFlag());
    pcVUI->setFieldSeqFlag(false);
    pcVUI->setHrdParametersPresentFlag(false);
    pcVUI->getTimingInfo()->setPocProportionalToTimingFlag(getPocProportionalToTimingFlag());
    pcVUI->getTimingInfo()->setNumTicksPocDiffOneMinus1   (getNumTicksPocDiffOneMinus1()   );
    pcVUI->setBitstreamRestrictionFlag(getBitstreamRestrictionFlag());
    pcVUI->setTilesFixedStructureFlag(getTilesFixedStructureFlag());
    pcVUI->setMotionVectorsOverPicBoundariesFlag(getMotionVectorsOverPicBoundariesFlag());
    pcVUI->setMinSpatialSegmentationIdc(getMinSpatialSegmentationIdc());
    pcVUI->setMaxBytesPerPicDenom(getMaxBytesPerPicDenom());
    pcVUI->setMaxBitsPerMinCuDenom(getMaxBitsPerMinCuDenom());
    pcVUI->setLog2MaxMvLengthHorizontal(getLog2MaxMvLengthHorizontal());
    pcVUI->setLog2MaxMvLengthVertical(getLog2MaxMvLengthVertical());
  }
}

Void TEncTop::xInitPPS()
{
  m_cPPS.setConstrainedIntraPred( m_bUseConstrainedIntraPred );
  Bool bUseDQP = (getMaxCuDQPDepth() > 0)? true : false;

  if((getMaxDeltaQP() != 0 )|| getUseAdaptiveQP())
  {
    bUseDQP = true;
  }

  if(bUseDQP)
  {
    m_cPPS.setUseDQP(true);
    m_cPPS.setMaxCuDQPDepth( m_iMaxCuDQPDepth );
    m_cPPS.setMinCuDQPSize( m_cPPS.getSPS()->getMaxCUWidth() >> ( m_cPPS.getMaxCuDQPDepth()) );
  }
  else
  {
    m_cPPS.setUseDQP(false);
    m_cPPS.setMaxCuDQPDepth( 0 );
    m_cPPS.setMinCuDQPSize( m_cPPS.getSPS()->getMaxCUWidth() >> ( m_cPPS.getMaxCuDQPDepth()) );
  }

  if ( m_RCEnableRateControl )
  {
    m_cPPS.setUseDQP(true);
    m_cPPS.setMaxCuDQPDepth( 0 );
    m_cPPS.setMinCuDQPSize( m_cPPS.getSPS()->getMaxCUWidth() >> ( m_cPPS.getMaxCuDQPDepth()) );
  } 

  m_cPPS.setChromaCbQpOffset( m_chromaCbQpOffset );
  m_cPPS.setChromaCrQpOffset( m_chromaCrQpOffset );

  m_cPPS.setNumSubstreams(m_iWaveFrontSubstreams);
  m_cPPS.setEntropyCodingSyncEnabledFlag( m_iWaveFrontSynchro > 0 );
  m_cPPS.setTilesEnabledFlag( (m_iNumColumnsMinus1 > 0 || m_iNumRowsMinus1 > 0) );
  m_cPPS.setUseWP( m_useWeightedPred );
  m_cPPS.setWPBiPred( m_useWeightedBiPred );
  m_cPPS.setOutputFlagPresentFlag( false );
  m_cPPS.setSignHideFlag(getSignHideFlag());
  if ( getDeblockingFilterMetric() )
  {
    m_cPPS.setDeblockingFilterControlPresentFlag (true);
    m_cPPS.setDeblockingFilterOverrideEnabledFlag(true);
    m_cPPS.setPicDisableDeblockingFilterFlag(false);
    m_cPPS.setDeblockingFilterBetaOffsetDiv2(0);
    m_cPPS.setDeblockingFilterTcOffsetDiv2(0);
  } 
  else
  {
  m_cPPS.setDeblockingFilterControlPresentFlag (m_DeblockingFilterControlPresent );
  }
  m_cPPS.setLog2ParallelMergeLevelMinus2   (m_log2ParallelMergeLevelMinus2 );
  m_cPPS.setCabacInitPresentFlag(CABAC_INIT_PRESENT_FLAG);
  m_cPPS.setLoopFilterAcrossSlicesEnabledFlag( m_bLFCrossSliceBoundaryFlag );
  Int histogram[MAX_NUM_REF + 1];
  for( Int i = 0; i <= MAX_NUM_REF; i++ )
  {
    histogram[i]=0;
  }
  for( Int i = 0; i < getGOPSize(); i++ )
  {
    assert(getGOPEntry(i).m_numRefPicsActive >= 0 && getGOPEntry(i).m_numRefPicsActive <= MAX_NUM_REF);
    histogram[getGOPEntry(i).m_numRefPicsActive]++;
  }
  Int maxHist=-1;
  Int bestPos=0;
  for( Int i = 0; i <= MAX_NUM_REF; i++ )
  {
    if(histogram[i]>maxHist)
    {
      maxHist=histogram[i];
      bestPos=i;
    }
  }
  assert(bestPos <= 15);
  m_cPPS.setNumRefIdxL0DefaultActive(bestPos);
  m_cPPS.setNumRefIdxL1DefaultActive(bestPos);
  m_cPPS.setTransquantBypassEnableFlag(getTransquantBypassEnableFlag());
  m_cPPS.setUseTransformSkip( m_useTransformSkip );
  if (m_sliceSegmentMode)
  {
    m_cPPS.setDependentSliceSegmentsEnabledFlag( true );
  }
  if( m_cPPS.getDependentSliceSegmentsEnabledFlag() )
  {
    Int NumCtx = m_cPPS.getEntropyCodingSyncEnabledFlag()?2:1;
    m_cSliceEncoder.initCtxMem( NumCtx );
    for ( UInt st = 0; st < NumCtx; st++ )
    {
      TEncSbac* ctx = NULL;
      ctx = new TEncSbac;
      ctx->init( &m_cBinCoderCABAC );
      m_cSliceEncoder.setCtxMem( ctx, st );
    }
  }
#if SVC_EXTENSION
#if SCALINGLIST_INFERRING
  m_cPPS.setLayerId( m_layerId );
#endif

#if Q0078_ADD_LAYER_SETS
  if( !m_numDirectRefLayers && m_numAddLayerSets )
  {
    m_cPPS.setLayerId(0); // layer ID 0 for independent layers
  }
#endif

  if( m_layerId > 0 )
  {
    m_cPPS.setListsModificationPresentFlag(true);
    m_cPPS.setExtensionFlag(true);
  }
  else
  {
    m_cPPS.setListsModificationPresentFlag(false);
    m_cPPS.setExtensionFlag(false);
  }

  m_cPPS.setPPSId( m_iPPSIdCnt );
  m_cPPS.setSPSId( m_iSPSIdCnt );
#if POC_RESET_FLAG
  m_cPPS.setNumExtraSliceHeaderBits( 2 );
#endif
#if O0149_CROSS_LAYER_BLA_FLAG
  if (m_crossLayerBLAFlag)
  {
    m_cPPS.setNumExtraSliceHeaderBits( 3 );
  }
#endif
#if MOVE_SCALED_OFFSET_TO_PPS
  m_cPPS.setNumScaledRefLayerOffsets(m_numScaledRefLayerOffsets);
  for(Int i = 0; i < m_cPPS.getNumScaledRefLayerOffsets(); i++)
  {
#if O0098_SCALED_REF_LAYER_ID
    m_cPPS.setScaledRefLayerId(i, m_scaledRefLayerId[i]);
#endif
    m_cPPS.getScaledRefLayerWindow(i) = m_scaledRefLayerWindow[i];
#if REF_REGION_OFFSET
    m_cPPS.getRefLayerWindow(i) = m_refLayerWindow[i];
    m_cPPS.setScaledRefLayerOffsetPresentFlag( m_scaledRefLayerId[i], m_scaledRefLayerOffsetPresentFlag[i] );
    m_cPPS.setRefRegionOffsetPresentFlag( m_scaledRefLayerId[i], m_refRegionOffsetPresentFlag[i] );
#endif
#if R0209_GENERIC_PHASE
    m_cPPS.setResamplePhaseSetPresentFlag( m_scaledRefLayerId[i], m_resamplePhaseSetPresentFlag[i] );
    m_cPPS.setPhaseHorLuma( m_scaledRefLayerId[i], m_phaseHorLuma[i] );
    m_cPPS.setPhaseVerLuma( m_scaledRefLayerId[i], m_phaseVerLuma[i] );
    m_cPPS.setPhaseHorChroma( m_scaledRefLayerId[i], m_phaseHorChroma[i] );
    m_cPPS.setPhaseVerChroma( m_scaledRefLayerId[i], m_phaseVerChroma[i] );
#endif
#if P0312_VERT_PHASE_ADJ
    m_cPPS.setVertPhasePositionEnableFlag( m_scaledRefLayerId[i], m_scaledRefLayerWindow[i].getVertPhasePositionEnableFlag() );
#endif
  }
#endif
#if Q0048_CGS_3D_ASYMLUT
  m_cPPS.setCGSFlag( m_nCGSFlag );
#endif
#if POC_RESET_IDC_ENCODER
  m_cPPS.setPocResetInfoPresentFlag( true );
  m_cPPS.setExtensionFlag( true );
  m_cPPS.setSliceHeaderExtensionPresentFlag( true );
#endif
#endif //SVC_EXTENSION
}

//Function for initializing m_RPSList, a list of TComReferencePictureSet, based on the GOPEntry objects read from the config file.
Void TEncTop::xInitRPS(Bool isFieldCoding)
{
  TComReferencePictureSet*      rps;
  
  m_cSPS.createRPSList(getGOPSize()+m_extraRPSs+1);
  TComRPSList* rpsList = m_cSPS.getRPSList();

  for( Int i = 0; i < getGOPSize()+m_extraRPSs; i++) 
  {
    GOPEntry ge = getGOPEntry(i);
    rps = rpsList->getReferencePictureSet(i);
    rps->setNumberOfPictures(ge.m_numRefPics);
    rps->setNumRefIdc(ge.m_numRefIdc);
    Int numNeg = 0;
    Int numPos = 0;
    for( Int j = 0; j < ge.m_numRefPics; j++)
    {
      rps->setDeltaPOC(j,ge.m_referencePics[j]);
      rps->setUsed(j,ge.m_usedByCurrPic[j]);
      if(ge.m_referencePics[j]>0)
      {
        numPos++;
      }
      else
      {
        numNeg++;
      }
    }
    rps->setNumberOfNegativePictures(numNeg);
    rps->setNumberOfPositivePictures(numPos);

    // handle inter RPS intialization from the config file.
#if AUTO_INTER_RPS
    rps->setInterRPSPrediction(ge.m_interRPSPrediction > 0);  // not very clean, converting anything > 0 to true.
    rps->setDeltaRIdxMinus1(0);                               // index to the Reference RPS is always the previous one.
    TComReferencePictureSet*     RPSRef = rpsList->getReferencePictureSet(i-1);  // get the reference RPS

    if (ge.m_interRPSPrediction == 2)  // Automatic generation of the inter RPS idc based on the RIdx provided.
    {
      Int deltaRPS = getGOPEntry(i-1).m_POC - ge.m_POC;  // the ref POC - current POC
      Int numRefDeltaPOC = RPSRef->getNumberOfPictures();

      rps->setDeltaRPS(deltaRPS);           // set delta RPS
      rps->setNumRefIdc(numRefDeltaPOC+1);  // set the numRefIdc to the number of pictures in the reference RPS + 1.
      Int count=0;
      for (Int j = 0; j <= numRefDeltaPOC; j++ ) // cycle through pics in reference RPS.
      {
        Int RefDeltaPOC = (j<numRefDeltaPOC)? RPSRef->getDeltaPOC(j): 0;  // if it is the last decoded picture, set RefDeltaPOC = 0
        rps->setRefIdc(j, 0);
        for (Int k = 0; k < rps->getNumberOfPictures(); k++ )  // cycle through pics in current RPS.
        {
          if (rps->getDeltaPOC(k) == ( RefDeltaPOC + deltaRPS))  // if the current RPS has a same picture as the reference RPS. 
          {
              rps->setRefIdc(j, (rps->getUsed(k)?1:2));
              count++;
              break;
          }
        }
      }
      if (count != rps->getNumberOfPictures())
      {
        printf("Warning: Unable fully predict all delta POCs using the reference RPS index given in the config file.  Setting Inter RPS to false for this RPS.\n");
        rps->setInterRPSPrediction(0);
      }
    }
    else if (ge.m_interRPSPrediction == 1)  // inter RPS idc based on the RefIdc values provided in config file.
    {
      rps->setDeltaRPS(ge.m_deltaRPS);
      rps->setNumRefIdc(ge.m_numRefIdc);
      for (Int j = 0; j < ge.m_numRefIdc; j++ )
      {
        rps->setRefIdc(j, ge.m_refIdc[j]);
      }
#if WRITE_BACK
      // the folowing code overwrite the deltaPOC and Used by current values read from the config file with the ones
      // computed from the RefIdc.  A warning is printed if they are not identical.
      numNeg = 0;
      numPos = 0;
      TComReferencePictureSet      RPSTemp;  // temporary variable

      for (Int j = 0; j < ge.m_numRefIdc; j++ )
      {
        if (ge.m_refIdc[j])
        {
          Int deltaPOC = ge.m_deltaRPS + ((j < RPSRef->getNumberOfPictures())? RPSRef->getDeltaPOC(j) : 0);
          RPSTemp.setDeltaPOC((numNeg+numPos),deltaPOC);
          RPSTemp.setUsed((numNeg+numPos),ge.m_refIdc[j]==1?1:0);
          if (deltaPOC<0)
          {
            numNeg++;
          }
          else
          {
            numPos++;
          }
        }
      }
      if (numNeg != rps->getNumberOfNegativePictures())
      {
        printf("Warning: number of negative pictures in RPS is different between intra and inter RPS specified in the config file.\n");
        rps->setNumberOfNegativePictures(numNeg);
        rps->setNumberOfPictures(numNeg+numPos);
      }
      if (numPos != rps->getNumberOfPositivePictures())
      {
        printf("Warning: number of positive pictures in RPS is different between intra and inter RPS specified in the config file.\n");
        rps->setNumberOfPositivePictures(numPos);
        rps->setNumberOfPictures(numNeg+numPos);
      }
      RPSTemp.setNumberOfPictures(numNeg+numPos);
      RPSTemp.setNumberOfNegativePictures(numNeg);
      RPSTemp.sortDeltaPOC();     // sort the created delta POC before comparing
      // check if Delta POC and Used are the same 
      // print warning if they are not.
      for (Int j = 0; j < ge.m_numRefIdc; j++ )
      {
        if (RPSTemp.getDeltaPOC(j) != rps->getDeltaPOC(j))
        {
          printf("Warning: delta POC is different between intra RPS and inter RPS specified in the config file.\n");
          rps->setDeltaPOC(j,RPSTemp.getDeltaPOC(j));
        }
        if (RPSTemp.getUsed(j) != rps->getUsed(j))
        {
          printf("Warning: Used by Current in RPS is different between intra and inter RPS specified in the config file.\n");
          rps->setUsed(j,RPSTemp.getUsed(j));
        }
      }
#endif
    }
#else
    rps->setInterRPSPrediction(ge.m_interRPSPrediction);
    if (ge.m_interRPSPrediction)
    {
      rps->setDeltaRIdxMinus1(0);
      rps->setDeltaRPS(ge.m_deltaRPS);
      rps->setNumRefIdc(ge.m_numRefIdc);
      for (Int j = 0; j < ge.m_numRefIdc; j++ )
      {
        rps->setRefIdc(j, ge.m_refIdc[j]);
      }
#if WRITE_BACK
      // the folowing code overwrite the deltaPOC and Used by current values read from the config file with the ones
      // computed from the RefIdc.  This is not necessary if both are identical. Currently there is no check to see if they are identical.
      numNeg = 0;
      numPos = 0;
      TComReferencePictureSet*     RPSRef = m_RPSList.getReferencePictureSet(i-1);

      for (Int j = 0; j < ge.m_numRefIdc; j++ )
      {
        if (ge.m_refIdc[j])
        {
          Int deltaPOC = ge.m_deltaRPS + ((j < RPSRef->getNumberOfPictures())? RPSRef->getDeltaPOC(j) : 0);
          rps->setDeltaPOC((numNeg+numPos),deltaPOC);
          rps->setUsed((numNeg+numPos),ge.m_refIdc[j]==1?1:0);
          if (deltaPOC<0)
          {
            numNeg++;
          }
          else
          {
            numPos++;
          }
        }
      }
      rps->setNumberOfNegativePictures(numNeg);
      rps->setNumberOfPositivePictures(numPos);
      rps->sortDeltaPOC();
#endif
    }
#endif //INTER_RPS_AUTO
  }
  //In case of field coding, we need to set special parameters for the first bottom field of the sequence, since it is not specified in the cfg file. 
  //The position = GOPSize + extraRPSs which is (a priori) unused is reserved for this field in the RPS. 
  if (isFieldCoding) 
  {
    rps = rpsList->getReferencePictureSet(getGOPSize()+m_extraRPSs);
    rps->setNumberOfPictures(1);
    rps->setNumberOfNegativePictures(1);
    rps->setNumberOfPositivePictures(0);
    rps->setNumberOfLongtermPictures(0);
    rps->setDeltaPOC(0,-1);
    rps->setPOC(0,0);
    rps->setUsed(0,true);
    rps->setInterRPSPrediction(false);
    rps->setDeltaRIdxMinus1(0);
    rps->setDeltaRPS(0);
    rps->setNumRefIdc(0);
}
}

   // This is a function that 
   // determines what Reference Picture Set to use 
   // for a specific slice (with POC = POCCurr)
Void TEncTop::selectReferencePictureSet(TComSlice* slice, Int POCCurr, Int GOPid )
{
  slice->setRPSidx(GOPid);
  for(Int extraNum=m_iGOPSize; extraNum<m_extraRPSs+m_iGOPSize; extraNum++)
  {    
    if(m_uiIntraPeriod > 0 && getDecodingRefreshType() > 0)
    {
      Int POCIndex = POCCurr%m_uiIntraPeriod;
      if(POCIndex == 0)
      {
        POCIndex = m_uiIntraPeriod;
      }
      if(POCIndex == m_GOPList[extraNum].m_POC)
      {
        slice->setRPSidx(extraNum);
      }
    }
    else
    {
      if(POCCurr==m_GOPList[extraNum].m_POC)
      {
        slice->setRPSidx(extraNum);
      }
    }
  }

  if(POCCurr == 1 && slice->getPic()->isField())
  {
    slice->setRPSidx(m_iGOPSize+m_extraRPSs);
  }

  slice->setRPS(getSPS()->getRPSList()->getReferencePictureSet(slice->getRPSidx()));
  slice->getRPS()->setNumberOfPictures(slice->getRPS()->getNumberOfNegativePictures()+slice->getRPS()->getNumberOfPositivePictures());
}

Int TEncTop::getReferencePictureSetIdxForSOP(TComSlice* slice, Int POCCurr, Int GOPid )
{
  int rpsIdx = GOPid;

  for(Int extraNum=m_iGOPSize; extraNum<m_extraRPSs+m_iGOPSize; extraNum++)
  {    
    if(m_uiIntraPeriod > 0 && getDecodingRefreshType() > 0)
    {
      Int POCIndex = POCCurr%m_uiIntraPeriod;
      if(POCIndex == 0)
      {
        POCIndex = m_uiIntraPeriod;
      }
      if(POCIndex == m_GOPList[extraNum].m_POC)
      {
        rpsIdx = extraNum;
      }
    }
    else
    {
      if(POCCurr==m_GOPList[extraNum].m_POC)
      {
        rpsIdx = extraNum;
      }
    }
  }

  return rpsIdx;
}

Void  TEncTop::xInitPPSforTiles()
{
  m_cPPS.setTileUniformSpacingFlag( m_tileUniformSpacingFlag );
  m_cPPS.setNumTileColumnsMinus1( m_iNumColumnsMinus1 );
  m_cPPS.setNumTileRowsMinus1( m_iNumRowsMinus1 );
  if( !m_tileUniformSpacingFlag )
  {
    m_cPPS.setTileColumnWidth( m_tileColumnWidth );
    m_cPPS.setTileRowHeight( m_tileRowHeight );
  }
  m_cPPS.setLoopFilterAcrossTilesEnabledFlag( m_loopFilterAcrossTilesEnabledFlag );

  // # substreams is "per tile" when tiles are independent.
  if (m_iWaveFrontSynchro )
  {
    m_cPPS.setNumSubstreams(m_iWaveFrontSubstreams * (m_iNumColumnsMinus1+1));
  }
}

Void  TEncCfg::xCheckGSParameters()
{
  Int   iWidthInCU = ( m_iSourceWidth%g_uiMaxCUWidth ) ? m_iSourceWidth/g_uiMaxCUWidth + 1 : m_iSourceWidth/g_uiMaxCUWidth;
  Int   iHeightInCU = ( m_iSourceHeight%g_uiMaxCUHeight ) ? m_iSourceHeight/g_uiMaxCUHeight + 1 : m_iSourceHeight/g_uiMaxCUHeight;
  UInt  uiCummulativeColumnWidth = 0;
  UInt  uiCummulativeRowHeight = 0;

  //check the column relative parameters
  if( m_iNumColumnsMinus1 >= (1<<(LOG2_MAX_NUM_COLUMNS_MINUS1+1)) )
  {
    printf( "The number of columns is larger than the maximum allowed number of columns.\n" );
    exit( EXIT_FAILURE );
  }

  if( m_iNumColumnsMinus1 >= iWidthInCU )
  {
    printf( "The current picture can not have so many columns.\n" );
    exit( EXIT_FAILURE );
  }

  if( m_iNumColumnsMinus1 && !m_tileUniformSpacingFlag )
  {
    for(Int i=0; i<m_iNumColumnsMinus1; i++)
    {
      uiCummulativeColumnWidth += m_tileColumnWidth[i];
    }

    if( uiCummulativeColumnWidth >= iWidthInCU )
    {
      printf( "The width of the column is too large.\n" );
      exit( EXIT_FAILURE );
    }
  }

  //check the row relative parameters
  if( m_iNumRowsMinus1 >= (1<<(LOG2_MAX_NUM_ROWS_MINUS1+1)) )
  {
    printf( "The number of rows is larger than the maximum allowed number of rows.\n" );
    exit( EXIT_FAILURE );
  }

  if( m_iNumRowsMinus1 >= iHeightInCU )
  {
    printf( "The current picture can not have so many rows.\n" );
    exit( EXIT_FAILURE );
  }

  if( m_iNumRowsMinus1 && !m_tileUniformSpacingFlag )
  {
    for(Int i=0; i<m_iNumRowsMinus1; i++)
      uiCummulativeRowHeight += m_tileRowHeight[i];

    if( uiCummulativeRowHeight >= iHeightInCU )
    {
      printf( "The height of the row is too large.\n" );
      exit( EXIT_FAILURE );
    }
  }
}

#if SVC_EXTENSION
#if VPS_EXTN_DIRECT_REF_LAYERS
TEncTop* TEncTop::getRefLayerEnc( UInt refLayerIdc )
{
  if( m_ppcTEncTop[m_layerId]->getNumDirectRefLayers() <= 0 )
  {
    return (TEncTop *)getLayerEnc( 0 );
  }

  return (TEncTop *)getLayerEnc( m_cVPS.getRefLayerId( m_layerId, refLayerIdc ) );
}
#endif

#if !REPN_FORMAT_IN_VPS
Void TEncTop::xInitILRP()
{
  if(m_layerId>0)
  {
    g_bitDepthY     = m_cSPS.getBitDepthY();
    g_bitDepthC     = m_cSPS.getBitDepthC();
    g_uiMaxCUWidth  = m_cSPS.getMaxCUWidth();
    g_uiMaxCUHeight = m_cSPS.getMaxCUHeight();
    g_uiMaxCUDepth  = m_cSPS.getMaxCUDepth();
    g_uiAddCUDepth  = max (0, m_cSPS.getLog2MinCodingBlockSize() - (Int)m_cSPS.getQuadtreeTULog2MinSize() );

    Int  numReorderPics[MAX_TLAYER];
    Window &conformanceWindow = m_cSPS.getConformanceWindow();
    Window defaultDisplayWindow = m_cSPS.getVuiParametersPresentFlag() ? m_cSPS.getVuiParameters()->getDefaultDisplayWindow() : Window();

    if (m_cIlpPic[0] == NULL)
    {
      for (Int j=0; j < m_numLayer; j++) // consider to set to NumDirectRefLayers[LayerIdInVps[nuh_layer_id]]
      {
        m_cIlpPic[j] = new  TComPic;
#if AUXILIARY_PICTURES
        m_cIlpPic[j]->create(m_iSourceWidth, m_iSourceHeight, m_chromaFormatIDC, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, conformanceWindow, defaultDisplayWindow, numReorderPics, &m_cSPS, true);
#else
        m_cIlpPic[j]->create(m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, conformanceWindow, defaultDisplayWindow, numReorderPics, &m_cSPS, true);
#endif
        for (Int i=0; i<m_cIlpPic[j]->getPicSym()->getNumberOfCUsInFrame(); i++)
        {
          m_cIlpPic[j]->getPicSym()->getCU(i)->initCU(m_cIlpPic[j], i);
        }
      }
    }
  }
}
#else
Void TEncTop::xInitILRP()
{
#if O0096_REP_FORMAT_INDEX
  RepFormat *repFormat = m_cVPS.getVpsRepFormat( m_cVPS.getVpsRepFormatIdx( m_cSPS.getUpdateRepFormatFlag() ? m_cSPS.getUpdateRepFormatIndex() : m_layerId ) );
#else
  RepFormat *repFormat = m_cVPS.getVpsRepFormat( m_cVPS.getVpsRepFormatIdx( m_layerId ) );
#endif
  Int bitDepthY,bitDepthC,picWidth,picHeight;

#if O0096_REP_FORMAT_INDEX
  bitDepthY   = repFormat->getBitDepthVpsLuma();
  bitDepthC   = repFormat->getBitDepthVpsChroma();
  picWidth    = repFormat->getPicWidthVpsInLumaSamples();
  picHeight   = repFormat->getPicHeightVpsInLumaSamples();
#else
  if( m_cSPS.getUpdateRepFormatFlag() )
  {
    bitDepthY   = m_cSPS.getBitDepthY();
    bitDepthC   = m_cSPS.getBitDepthC();
    picWidth    = m_cSPS.getPicWidthInLumaSamples();
    picHeight   = m_cSPS.getPicHeightInLumaSamples();
  }
  else
  {
    bitDepthY   = repFormat->getBitDepthVpsLuma();
    bitDepthC   = repFormat->getBitDepthVpsChroma();
    picWidth    = repFormat->getPicWidthVpsInLumaSamples();
    picHeight   = repFormat->getPicHeightVpsInLumaSamples();
  }
#endif
  
  if(m_layerId > 0)
  {
    g_bitDepthY     = bitDepthY;
    g_bitDepthC     = bitDepthC;
    g_uiMaxCUWidth  = m_cSPS.getMaxCUWidth();
    g_uiMaxCUHeight = m_cSPS.getMaxCUHeight();
    g_uiMaxCUDepth  = m_cSPS.getMaxCUDepth();
    g_uiAddCUDepth  = max (0, m_cSPS.getLog2MinCodingBlockSize() - (Int)m_cSPS.getQuadtreeTULog2MinSize() );

    Int  numReorderPics[MAX_TLAYER];
#if R0156_CONF_WINDOW_IN_REP_FORMAT
    Window &conformanceWindow = repFormat->getConformanceWindowVps();
#else
    Window &conformanceWindow = m_cSPS.getConformanceWindow();
#endif
    Window defaultDisplayWindow = m_cSPS.getVuiParametersPresentFlag() ? m_cSPS.getVuiParameters()->getDefaultDisplayWindow() : Window();

    if (m_cIlpPic[0] == NULL)
    {
      for (Int j=0; j < m_numDirectRefLayers; j++)
      {
        m_cIlpPic[j] = new  TComPic;
#if AUXILIARY_PICTURES
        m_cIlpPic[j]->create(picWidth, picHeight, m_chromaFormatIDC, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, conformanceWindow, defaultDisplayWindow, numReorderPics, &m_cSPS, true);
#else
        m_cIlpPic[j]->create(picWidth, picHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, conformanceWindow, defaultDisplayWindow, numReorderPics, &m_cSPS, true);
#endif
        for (Int i=0; i<m_cIlpPic[j]->getPicSym()->getNumberOfCUsInFrame(); i++)
        {
          m_cIlpPic[j]->getPicSym()->getCU(i)->initCU(m_cIlpPic[j], i);
        }
      }
    }
  }
}
#endif

#if O0098_SCALED_REF_LAYER_ID
Window& TEncTop::getScaledRefLayerWindowForLayer(Int layerId)
{
  static Window win;

  for (Int i = 0; i < m_numScaledRefLayerOffsets; i++)
  {
    if (layerId == m_scaledRefLayerId[i])
    {
      return m_scaledRefLayerWindow[i];
    }
  }

  win.resetWindow();  // scaled reference layer offsets are inferred to be zero when not present
  return win;
}
#if REF_REGION_OFFSET
Window& TEncTop::getRefLayerWindowForLayer(Int layerId)
{
  static Window win;

  for (Int i = 0; i < m_numScaledRefLayerOffsets; i++)
  {
    if (layerId == m_refLayerId[i])
    {
      return m_refLayerWindow[i];
    }
  }

  win.resetWindow();  // reference offsets are inferred to be zero when not present
  return win;
}
#endif
#endif

#endif //SVC_EXTENSION
//! \}
