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

/** \file     TAppEncTop.cpp
    \brief    Encoder application class
*/

#include <list>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "TAppEncTop.h"
#include "TLibEncoder/AnnexBwrite.h"

using namespace std;

//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppEncTop::TAppEncTop()
{
  m_iFrameRcvd = 0;
  m_totalBytes = 0;
  m_essentialBytes = 0;
#if SVC_EXTENSION
  for(UInt layer=0; layer < MAX_LAYERS; layer++)
  {
    m_apcTEncTop[layer] = &m_acTEncTop[layer];
  }
#endif
}

TAppEncTop::~TAppEncTop()
{
}

#if SVC_EXTENSION
Void TAppEncTop::xInitLibCfg()
{
  TComVPS* vps = m_acTEncTop[0].getVPS();

#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  vps->setVpsPocLsbAlignedFlag(false);
#endif
  vps->setMaxTLayers                       ( m_maxTempLayer );
  if (m_maxTempLayer == 1)
  {
    vps->setTemporalNestingFlag(true);
  }
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    vps->setNumReorderPics                 ( m_numReorderPics[i], i );
    vps->setMaxDecPicBuffering             ( m_maxDecPicBuffering[i], i );
  }

#if REPN_FORMAT_IN_VPS  
  Int maxRepFormatIdx = -1;
  Int formatIdx = -1;
  for(UInt layer=0; layer < m_numLayers; layer++)
  {
    // Auto generation of the format index
    if( m_acLayerCfg[layer].getRepFormatIdx() == -1 )
    {      
      Bool found = false;
      for( UInt idx = 0; idx < layer; idx++ )
      {
        if( m_acLayerCfg[layer].getSourceWidth() == m_acLayerCfg[idx].getSourceWidth() && m_acLayerCfg[layer].getSourceHeight() == m_acLayerCfg[idx].getSourceHeight()
#if AUXILIARY_PICTURES
          && m_acLayerCfg[layer].getChromaFormatIDC() == m_acLayerCfg[idx].getChromaFormatIDC()
#endif
#if O0194_DIFFERENT_BITDEPTH_EL_BL
          && m_acLayerCfg[layer].m_internalBitDepthY == m_acLayerCfg[idx].m_internalBitDepthY && m_acLayerCfg[layer].m_internalBitDepthC == m_acLayerCfg[idx].m_internalBitDepthC
#endif
          )
        {
          found = true;
          break;
        }
      }
      if( !found )
      {
        formatIdx++;
      }

      m_acLayerCfg[layer].setRepFormatIdx( formatIdx );
    }

    assert( m_acLayerCfg[layer].getRepFormatIdx() != -1 && "RepFormatIdx not assigned for a layer" );

    vps->setVpsRepFormatIdx( layer, m_acLayerCfg[layer].getRepFormatIdx() );

    maxRepFormatIdx = std::max( m_acLayerCfg[layer].getRepFormatIdx(), maxRepFormatIdx );
  }

  assert( vps->getVpsRepFormatIdx( 0 ) == 0 );  // Base layer should point to the first one.

  Int* mapIdxToLayer = new Int[maxRepFormatIdx + 1];

  // Check that all the indices from 0 to maxRepFormatIdx are used in the VPS
  for(Int i = 0; i <= maxRepFormatIdx; i++)
  {
    mapIdxToLayer[i] = -1;
    UInt layer;
    for(layer=0; layer < m_numLayers; layer++)
    {
      if( vps->getVpsRepFormatIdx(layer) == i )
      {
        mapIdxToLayer[i] = layer;
        break;
      }
    }
    assert( layer != m_numLayers );   // One of the VPS Rep format indices not set
  }

  vps->setVpsNumRepFormats( maxRepFormatIdx + 1 );

#if Q0195_REP_FORMAT_CLEANUP
  // When not present, the value of rep_format_idx_present_flag is inferred to be equal to 0
  vps->setRepFormatIdxPresentFlag( vps->getVpsNumRepFormats() > 1 ? true : false );
#else
  vps->setRepFormatIdxPresentFlag( true );
#endif

  for(UInt idx=0; idx < vps->getVpsNumRepFormats(); idx++)
  {
    RepFormat *repFormat = vps->getVpsRepFormat( idx );
#if REPN_FORMAT_CONTROL_FLAG
    repFormat->setChromaAndBitDepthVpsPresentFlag( true ); 
    if (idx==0)
    {
      assert(repFormat->getChromaAndBitDepthVpsPresentFlag() == true); 
    }
#endif 
    repFormat->setPicWidthVpsInLumaSamples  ( m_acLayerCfg[mapIdxToLayer[idx]].getSourceWidth()   );
    repFormat->setPicHeightVpsInLumaSamples ( m_acLayerCfg[mapIdxToLayer[idx]].getSourceHeight()  );
#if AUXILIARY_PICTURES
    repFormat->setChromaFormatVpsIdc        ( m_acLayerCfg[mapIdxToLayer[idx]].getChromaFormatIDC() );
#else
    repFormat->setChromaFormatVpsIdc        ( 1                                             );  // Need modification to change for each layer - corresponds to 420
#endif
    repFormat->setSeparateColourPlaneVpsFlag( 0                                             );  // Need modification to change for each layer
#if O0194_DIFFERENT_BITDEPTH_EL_BL
    repFormat->setBitDepthVpsLuma           ( getInternalBitDepthY(mapIdxToLayer[idx])      );  // Need modification to change for each layer
    repFormat->setBitDepthVpsChroma         ( getInternalBitDepthC(mapIdxToLayer[idx])      );  // Need modification to change for each layer
#else
    repFormat->setBitDepthVpsLuma           ( getInternalBitDepthY()                        );  // Need modification to change for each layer
    repFormat->setBitDepthVpsChroma         ( getInternalBitDepthC()                        );  // Need modification to change for each layer
#endif

#if R0156_CONF_WINDOW_IN_REP_FORMAT
    repFormat->getConformanceWindowVps().setWindow(
      m_acLayerCfg[mapIdxToLayer[idx]].m_confWinLeft, 
      m_acLayerCfg[mapIdxToLayer[idx]].m_confWinRight, 
      m_acLayerCfg[mapIdxToLayer[idx]].m_confWinTop,
      m_acLayerCfg[mapIdxToLayer[idx]].m_confWinBottom );
#endif

#if HIGHER_LAYER_IRAP_SKIP_FLAG
    m_acTEncTop[mapIdxToLayer[idx]].setSkipPictureAtArcSwitch( m_skipPictureAtArcSwitch );
#endif
  }
  delete [] mapIdxToLayer;
#endif

  assert( m_numLayers <= MAX_LAYERS );

  for(UInt layer=0; layer<m_numLayers; layer++)
  {
#if O0194_DIFFERENT_BITDEPTH_EL_BL
    //1
    g_bitDepthY = m_acLayerCfg[layer].m_internalBitDepthY;
    g_bitDepthC = m_acLayerCfg[layer].m_internalBitDepthC;

    g_uiPCMBitDepthLuma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthY : m_acLayerCfg[layer].m_internalBitDepthY;
    g_uiPCMBitDepthChroma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthC : m_acLayerCfg[layer].m_internalBitDepthC;

    // Set this to be used in Upsampling filter in function "TComUpsampleFilter::upsampleBasePic"
    g_bitDepthYLayer[layer] = g_bitDepthY;
    g_bitDepthCLayer[layer] = g_bitDepthC;

#if O0194_WEIGHTED_PREDICTION_CGS
    m_acTEncTop[layer].setInterLayerWeightedPredFlag( m_useInterLayerWeightedPred );
#endif
#endif
    //m_acTEncTop[layer].setVPS(&vps);
    m_acTEncTop[layer].setFrameRate                    ( m_acLayerCfg[layer].getFrameRate() );
    m_acTEncTop[layer].setFrameSkip                    ( m_FrameSkip );
    m_acTEncTop[layer].setSourceWidth                  ( m_acLayerCfg[layer].getSourceWidth() );
    m_acTEncTop[layer].setSourceHeight                 ( m_acLayerCfg[layer].getSourceHeight() );
    m_acTEncTop[layer].setConformanceWindow            ( m_acLayerCfg[layer].m_confWinLeft, m_acLayerCfg[layer].m_confWinRight, m_acLayerCfg[layer].m_confWinTop, m_acLayerCfg[layer].m_confWinBottom );
    m_acTEncTop[layer].setFramesToBeEncoded            ( m_framesToBeEncoded );

    m_acTEncTop[layer].setProfile(m_profile);
    m_acTEncTop[layer].setLevel(m_levelTier, m_level);
    m_acTEncTop[layer].setProgressiveSourceFlag(m_progressiveSourceFlag);
    m_acTEncTop[layer].setInterlacedSourceFlag(m_interlacedSourceFlag);
    m_acTEncTop[layer].setNonPackedConstraintFlag(m_nonPackedConstraintFlag);
    m_acTEncTop[layer].setFrameOnlyConstraintFlag(m_frameOnlyConstraintFlag);

#if REF_IDX_MFM
#if AVC_BASE
#if VPS_AVC_BL_FLAG_REMOVAL
    m_acTEncTop[layer].setMFMEnabledFlag(layer == 0 ? false : ( m_nonHEVCBaseLayerFlag ? false : true ) && m_acLayerCfg[layer].getNumMotionPredRefLayers());
#else
    m_acTEncTop[layer].setMFMEnabledFlag(layer == 0 ? false : ( m_avcBaseLayerFlag ? false : true ) && m_acLayerCfg[layer].getNumMotionPredRefLayers());
#endif
#else
    m_acTEncTop[layer].setMFMEnabledFlag(layer == 0 ? false : ( m_acLayerCfg[layer].getNumMotionPredRefLayers() > 0 ) );
#endif
#endif
    // set layer ID
    m_acTEncTop[layer].setLayerId ( layer );
    m_acTEncTop[layer].setNumLayer ( m_numLayers );
    m_acTEncTop[layer].setLayerEnc(m_apcTEncTop);

    //====== Coding Structure ========
    m_acTEncTop[layer].setIntraPeriod                  ( m_acLayerCfg[layer].m_iIntraPeriod );
    m_acTEncTop[layer].setDecodingRefreshType          ( m_iDecodingRefreshType );
    m_acTEncTop[layer].setGOPSize                      ( m_iGOPSize );
#if Q0108_TSA_STSA
    m_acTEncTop[layer].setGopList                      ( layer ? m_EhGOPList[layer] : m_GOPList );
#else
    m_acTEncTop[layer].setGopList                      ( m_GOPList );
#endif

#if !Q0108_TSA_STSA
    m_acTEncTop[layer].setExtraRPSs                    ( m_extraRPSs );
#else
    m_acTEncTop[layer].setExtraRPSs                    ( m_extraRPSs[layer] );
#endif
    for(Int i = 0; i < MAX_TLAYER; i++)
    {
      m_acTEncTop[layer].setNumReorderPics             ( m_numReorderPics[i], i );
      m_acTEncTop[layer].setMaxDecPicBuffering         ( m_maxDecPicBuffering[i], i );
    }
    for( UInt uiLoop = 0; uiLoop < MAX_TLAYER; ++uiLoop )
    {
      m_acTEncTop[layer].setLambdaModifier( uiLoop, m_adLambdaModifier[ uiLoop ] );
    }
    m_acTEncTop[layer].setQP                           ( m_acLayerCfg[layer].getIntQP() );

    m_acTEncTop[layer].setPad                          ( m_acLayerCfg[layer].getPad() );
#if !Q0108_TSA_STSA
    m_acTEncTop[layer].setMaxTempLayer                 ( m_maxTempLayer );
#else
    if (layer== 0)
    {
      m_acTEncTop[layer].setMaxTempLayer                 ( m_maxTempLayer );
    }
    else
    {
      m_acTEncTop[layer].setMaxTempLayer                 ( m_EhMaxTempLayer[layer] );
    }
#endif
    m_acTEncTop[layer].setUseAMP( m_enableAMP );

    if( layer < m_numLayers - 1 )
    {
       m_acTEncTop[layer].setMaxTidIlRefPicsPlus1 ( m_acLayerCfg[layer].getMaxTidIlRefPicsPlus1());
    }

#if VPS_EXTN_DIRECT_REF_LAYERS
    if(layer)
    {
      for(Int i = 0; i < MAX_VPS_LAYER_ID_PLUS1; i++)
      {
        m_acTEncTop[layer].setSamplePredEnabledFlag(i, false);
        m_acTEncTop[layer].setMotionPredEnabledFlag(i, false);
      }
      if(m_acLayerCfg[layer].getNumSamplePredRefLayers() == -1)
      {
        // Not included in the configuration file; assume that each layer depends on previous layer
        m_acTEncTop[layer].setNumSamplePredRefLayers   (1);      // One sample pred ref. layer
        m_acTEncTop[layer].setSamplePredRefLayerId     (0, layer - 1);   // Previous layer
        m_acTEncTop[layer].setSamplePredEnabledFlag    (layer - 1, true);
      }
      else
      {
        m_acTEncTop[layer].setNumSamplePredRefLayers   ( m_acLayerCfg[layer].getNumSamplePredRefLayers() );
        for(Int i = 0; i < m_acTEncTop[layer].getNumSamplePredRefLayers(); i++)
        {
          m_acTEncTop[layer].setSamplePredRefLayerId   ( i, m_acLayerCfg[layer].getSamplePredRefLayerId(i));
          m_acTEncTop[layer].setSamplePredEnabledFlag  (m_acLayerCfg[layer].getSamplePredRefLayerId(i), true);
        }
      }
      if(m_acLayerCfg[layer].getNumMotionPredRefLayers() == -1)
      {
        // Not included in the configuration file; assume that each layer depends on previous layer
        m_acTEncTop[layer].setNumMotionPredRefLayers   (1);      // One motion pred ref. layer
        m_acTEncTop[layer].setMotionPredRefLayerId     (0, layer - 1);   // Previous layer
        m_acTEncTop[layer].setMotionPredEnabledFlag    (layer - 1, true);
      }
      else
      {
        m_acTEncTop[layer].setNumMotionPredRefLayers   ( m_acLayerCfg[layer].getNumMotionPredRefLayers() );
        for(Int i = 0; i < m_acTEncTop[layer].getNumMotionPredRefLayers(); i++)
        {
          m_acTEncTop[layer].setMotionPredRefLayerId   ( i, m_acLayerCfg[layer].getMotionPredRefLayerId(i));
          m_acTEncTop[layer].setMotionPredEnabledFlag  (m_acLayerCfg[layer].getMotionPredRefLayerId(i), true);
        }
      }
      Int numDirectRefLayers = 0;

      assert( layer < MAX_LAYERS );

      for (Int i = 0; i < layer; i++)
      {
        if (m_acTEncTop[layer].getSamplePredEnabledFlag(i) || m_acTEncTop[layer].getMotionPredEnabledFlag(i))
        {
          m_acTEncTop[layer].setRefLayerId(numDirectRefLayers, i);
          numDirectRefLayers++;
        }
      }
      m_acTEncTop[layer].setNumDirectRefLayers(numDirectRefLayers);

      if(m_acLayerCfg[layer].getNumActiveRefLayers() == -1)
      {
        m_acTEncTop[layer].setNumActiveRefLayers( m_acTEncTop[layer].getNumDirectRefLayers() );
        for( Int i = 0; i < m_acTEncTop[layer].getNumActiveRefLayers(); i++ )
        {
          m_acTEncTop[layer].setPredLayerId(i, i);
        }
      }
      else
      {
        m_acTEncTop[layer].setNumActiveRefLayers       ( m_acLayerCfg[layer].getNumActiveRefLayers() );
        for(Int i = 0; i < m_acTEncTop[layer].getNumActiveRefLayers(); i++)
        {
          m_acTEncTop[layer].setPredLayerId             ( i, m_acLayerCfg[layer].getPredLayerId(i));
        }
      }
#if REF_REGION_OFFSET
      for(Int i = 0; i < m_acLayerCfg[layer].m_numScaledRefLayerOffsets; i++)
      {
#if AUXILIARY_PICTURES
        Int cf = m_acLayerCfg[i].m_chromaFormatIDC;
        Int rlSubWidthC  = ( cf == CHROMA_420 || cf == CHROMA_422 ) ? 2 : 1;
        Int rlSubHeightC = ( cf == CHROMA_420 ) ? 2 : 1;
#else
        Int rlSubWidthC  = 2;
        Int rlSubHeightC = 2;
#endif
        m_acTEncTop[layer].setRefRegionOffsetPresentFlag( i, m_acLayerCfg[layer].m_refRegionOffsetPresentFlag );
        m_acTEncTop[layer].getRefLayerWindow(i).setWindow( rlSubWidthC  * m_acLayerCfg[layer].m_refRegionLeftOffset[i], rlSubWidthC  * m_acLayerCfg[layer].m_refRegionRightOffset[i],
                                                           rlSubHeightC * m_acLayerCfg[layer].m_refRegionTopOffset[i],  rlSubHeightC * m_acLayerCfg[layer].m_refRegionBottomOffset[i]);
      }
#endif
    }
    else
    {
      assert( layer == 0 );
      m_acTEncTop[layer].setNumDirectRefLayers(0);
    }
#endif //VPS_EXTN_DIRECT_REF_LAYERS
    //===== Slice ========

    //====== Loop/Deblock Filter ========
    m_acTEncTop[layer].setLoopFilterDisable            ( m_bLoopFilterDisable       );
    m_acTEncTop[layer].setLoopFilterOffsetInPPS        ( m_loopFilterOffsetInPPS );
    m_acTEncTop[layer].setLoopFilterBetaOffset         ( m_loopFilterBetaOffsetDiv2  );
    m_acTEncTop[layer].setLoopFilterTcOffset           ( m_loopFilterTcOffsetDiv2    );
    m_acTEncTop[layer].setDeblockingFilterControlPresent( m_DeblockingFilterControlPresent);
    m_acTEncTop[layer].setDeblockingFilterMetric       ( m_DeblockingFilterMetric );

    //====== Motion search ========
    m_acTEncTop[layer].setFastSearch                   ( m_iFastSearch  );
    m_acTEncTop[layer].setSearchRange                  ( m_iSearchRange );
    m_acTEncTop[layer].setBipredSearchRange            ( m_bipredSearchRange );

    //====== Quality control ========
    m_acTEncTop[layer].setMaxDeltaQP                   ( m_iMaxDeltaQP  );
    m_acTEncTop[layer].setMaxCuDQPDepth                ( m_iMaxCuDQPDepth  );

    m_acTEncTop[layer].setChromaCbQpOffset             ( m_cbQpOffset     );
    m_acTEncTop[layer].setChromaCrQpOffset             ( m_crQpOffset  );

#if ADAPTIVE_QP_SELECTION
    m_acTEncTop[layer].setUseAdaptQpSelect             ( m_bUseAdaptQpSelect   );
#endif
    
    m_acTEncTop[layer].setUseAdaptiveQP                ( m_bUseAdaptiveQP  );
    m_acTEncTop[layer].setQPAdaptationRange            ( m_iQPAdaptationRange );

    //====== Tool list ========    
    m_acTEncTop[layer].setDeltaQpRD                    ( m_uiDeltaQpRD  );
    m_acTEncTop[layer].setUseASR                       ( m_bUseASR      );
    m_acTEncTop[layer].setUseHADME                     ( m_bUseHADME    );    
    m_acTEncTop[layer].setdQPs                         ( m_acLayerCfg[layer].getdQPs() );
    m_acTEncTop[layer].setUseRDOQ                      ( m_useRDOQ     );
    m_acTEncTop[layer].setUseRDOQTS                    ( m_useRDOQTS   );
    m_acTEncTop[layer].setRDpenalty                    ( m_rdPenalty );
#if LAYER_CTB
    m_acTEncTop[layer].setQuadtreeTULog2MaxSize        ( m_acLayerCfg[layer].m_uiQuadtreeTULog2MaxSize );
    m_acTEncTop[layer].setQuadtreeTULog2MinSize        ( m_acLayerCfg[layer].m_uiQuadtreeTULog2MinSize );
    m_acTEncTop[layer].setQuadtreeTUMaxDepthInter      ( m_acLayerCfg[layer].m_uiQuadtreeTUMaxDepthInter );
    m_acTEncTop[layer].setQuadtreeTUMaxDepthIntra      ( m_acLayerCfg[layer].m_uiQuadtreeTUMaxDepthIntra );
#else
    m_acTEncTop[layer].setQuadtreeTULog2MaxSize        ( m_uiQuadtreeTULog2MaxSize );
    m_acTEncTop[layer].setQuadtreeTULog2MinSize        ( m_uiQuadtreeTULog2MinSize );
    m_acTEncTop[layer].setQuadtreeTUMaxDepthInter      ( m_uiQuadtreeTUMaxDepthInter );
    m_acTEncTop[layer].setQuadtreeTUMaxDepthIntra      ( m_uiQuadtreeTUMaxDepthIntra );
#endif
    m_acTEncTop[layer].setUseFastEnc                   ( m_bUseFastEnc  );
    m_acTEncTop[layer].setUseEarlyCU                   ( m_bUseEarlyCU  );
    m_acTEncTop[layer].setUseFastDecisionForMerge      ( m_useFastDecisionForMerge  );
    m_acTEncTop[layer].setUseCbfFastMode               ( m_bUseCbfFastMode  );
    m_acTEncTop[layer].setUseEarlySkipDetection        ( m_useEarlySkipDetection );
#if FAST_INTRA_SHVC
    m_acTEncTop[layer].setUseFastIntraScalable         ( m_useFastIntraScalable );
#endif

    m_acTEncTop[layer].setUseTransformSkip             ( m_useTransformSkip      );
    m_acTEncTop[layer].setUseTransformSkipFast         ( m_useTransformSkipFast  );
    m_acTEncTop[layer].setUseConstrainedIntraPred      ( m_bUseConstrainedIntraPred );
    m_acTEncTop[layer].setPCMLog2MinSize               ( m_uiPCMLog2MinSize);
    m_acTEncTop[layer].setUsePCM                       ( m_usePCM );
    m_acTEncTop[layer].setPCMLog2MaxSize               ( m_pcmLog2MaxSize);
    m_acTEncTop[layer].setMaxNumMergeCand              ( m_maxNumMergeCand );


    //====== Weighted Prediction ========
    m_acTEncTop[layer].setUseWP                   ( m_useWeightedPred      );
    m_acTEncTop[layer].setWPBiPred                ( m_useWeightedBiPred   );
#if O0194_WEIGHTED_PREDICTION_CGS
    if( layer != 0 && m_useInterLayerWeightedPred )
    {
      // Enable weighted prediction for enhancement layer
      m_acTEncTop[layer].setUseWP                 ( true   );
      m_acTEncTop[layer].setWPBiPred              ( true   );
    }
#endif
    //====== Parallel Merge Estimation ========
    m_acTEncTop[layer].setLog2ParallelMergeLevelMinus2 ( m_log2ParallelMergeLevel - 2 );

    //====== Slice ========
    m_acTEncTop[layer].setSliceMode               ( m_sliceMode                );
    m_acTEncTop[layer].setSliceArgument           ( m_sliceArgument            );

    //====== Dependent Slice ========
    m_acTEncTop[layer].setSliceSegmentMode        ( m_sliceSegmentMode         );
    m_acTEncTop[layer].setSliceSegmentArgument    ( m_sliceSegmentArgument     );
#if LAYER_CTB
    Int iNumPartInCU = 1<<(m_acLayerCfg[layer].m_uiMaxCUDepth<<1);
#else
    Int iNumPartInCU = 1<<(m_uiMaxCUDepth<<1);
#endif
    if(m_sliceSegmentMode==FIXED_NUMBER_OF_LCU)
    {
      m_acTEncTop[layer].setSliceSegmentArgument ( m_sliceSegmentArgument * iNumPartInCU );
    }
    if(m_sliceMode==FIXED_NUMBER_OF_LCU)
    {
      m_acTEncTop[layer].setSliceArgument ( m_sliceArgument * iNumPartInCU );
    }
    if(m_sliceMode==FIXED_NUMBER_OF_TILES)
    {
      m_acTEncTop[layer].setSliceArgument ( m_sliceArgument );
    }

    if(m_sliceMode == 0 )
    {
      m_bLFCrossSliceBoundaryFlag = true;
    }
    m_acTEncTop[layer].setLFCrossSliceBoundaryFlag( m_bLFCrossSliceBoundaryFlag );
    m_acTEncTop[layer].setUseSAO ( m_bUseSAO );
    m_acTEncTop[layer].setMaxNumOffsetsPerPic (m_maxNumOffsetsPerPic);

    m_acTEncTop[layer].setSaoLcuBoundary (m_saoLcuBoundary);
    m_acTEncTop[layer].setPCMInputBitDepthFlag  ( m_bPCMInputBitDepthFlag);
    m_acTEncTop[layer].setPCMFilterDisableFlag  ( m_bPCMFilterDisableFlag);

    m_acTEncTop[layer].setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
    m_acTEncTop[layer].setRecoveryPointSEIEnabled( m_recoveryPointSEIEnabled );
    m_acTEncTop[layer].setBufferingPeriodSEIEnabled( m_bufferingPeriodSEIEnabled );
    m_acTEncTop[layer].setPictureTimingSEIEnabled( m_pictureTimingSEIEnabled );
    m_acTEncTop[layer].setToneMappingInfoSEIEnabled                 ( m_toneMappingInfoSEIEnabled );
    m_acTEncTop[layer].setTMISEIToneMapId                           ( m_toneMapId );
    m_acTEncTop[layer].setTMISEIToneMapCancelFlag                   ( m_toneMapCancelFlag );
    m_acTEncTop[layer].setTMISEIToneMapPersistenceFlag              ( m_toneMapPersistenceFlag );
    m_acTEncTop[layer].setTMISEICodedDataBitDepth                   ( m_toneMapCodedDataBitDepth );
    m_acTEncTop[layer].setTMISEITargetBitDepth                      ( m_toneMapTargetBitDepth );
    m_acTEncTop[layer].setTMISEIModelID                             ( m_toneMapModelId );
    m_acTEncTop[layer].setTMISEIMinValue                            ( m_toneMapMinValue );
    m_acTEncTop[layer].setTMISEIMaxValue                            ( m_toneMapMaxValue );
    m_acTEncTop[layer].setTMISEISigmoidMidpoint                     ( m_sigmoidMidpoint );
    m_acTEncTop[layer].setTMISEISigmoidWidth                        ( m_sigmoidWidth );
    m_acTEncTop[layer].setTMISEIStartOfCodedInterva                 ( m_startOfCodedInterval );
    m_acTEncTop[layer].setTMISEINumPivots                           ( m_numPivots );
    m_acTEncTop[layer].setTMISEICodedPivotValue                     ( m_codedPivotValue );
    m_acTEncTop[layer].setTMISEITargetPivotValue                    ( m_targetPivotValue );
    m_acTEncTop[layer].setTMISEICameraIsoSpeedIdc                   ( m_cameraIsoSpeedIdc );
    m_acTEncTop[layer].setTMISEICameraIsoSpeedValue                 ( m_cameraIsoSpeedValue );
    m_acTEncTop[layer].setTMISEIExposureCompensationValueSignFlag   ( m_exposureCompensationValueSignFlag );
    m_acTEncTop[layer].setTMISEIExposureCompensationValueNumerator  ( m_exposureCompensationValueNumerator );
    m_acTEncTop[layer].setTMISEIExposureCompensationValueDenomIdc   ( m_exposureCompensationValueDenomIdc );
    m_acTEncTop[layer].setTMISEIRefScreenLuminanceWhite             ( m_refScreenLuminanceWhite );
    m_acTEncTop[layer].setTMISEIExtendedRangeWhiteLevel             ( m_extendedRangeWhiteLevel );
    m_acTEncTop[layer].setTMISEINominalBlackLevelLumaCodeValue      ( m_nominalBlackLevelLumaCodeValue );
    m_acTEncTop[layer].setTMISEINominalWhiteLevelLumaCodeValue      ( m_nominalWhiteLevelLumaCodeValue );
    m_acTEncTop[layer].setTMISEIExtendedWhiteLevelLumaCodeValue     ( m_extendedWhiteLevelLumaCodeValue );
#if P0050_KNEE_FUNCTION_SEI
    m_acTEncTop[layer].setKneeSEIEnabled                            ( m_kneeSEIEnabled );
    m_acTEncTop[layer].setKneeSEIId                                 ( m_kneeSEIId );
    m_acTEncTop[layer].setKneeSEICancelFlag                         ( m_kneeSEICancelFlag );
    m_acTEncTop[layer].setKneeSEIPersistenceFlag                    ( m_kneeSEIPersistenceFlag );
    m_acTEncTop[layer].setKneeSEIMappingFlag                        ( m_kneeSEIMappingFlag );
    m_acTEncTop[layer].setKneeSEIInputDrange                        ( m_kneeSEIInputDrange );
    m_acTEncTop[layer].setKneeSEIInputDispLuminance                 ( m_kneeSEIInputDispLuminance );
    m_acTEncTop[layer].setKneeSEIOutputDrange                       ( m_kneeSEIOutputDrange );
    m_acTEncTop[layer].setKneeSEIOutputDispLuminance                ( m_kneeSEIOutputDispLuminance );
    m_acTEncTop[layer].setKneeSEINumKneePointsMinus1                ( m_kneeSEINumKneePointsMinus1 );
    m_acTEncTop[layer].setKneeSEIInputKneePoint                     ( m_kneeSEIInputKneePoint );
    m_acTEncTop[layer].setKneeSEIOutputKneePoint                    ( m_kneeSEIOutputKneePoint );
#endif
#if Q0074_COLOUR_REMAPPING_SEI
    m_acTEncTop[layer].setCRISEIFile                                ( const_cast<Char*>(m_acLayerCfg[layer].m_colourRemapSEIFile.c_str()) );
    m_acTEncTop[layer].setCRISEIId                                  ( m_acLayerCfg[layer].m_colourRemapSEIId );
    m_acTEncTop[layer].setCRISEICancelFlag                          ( m_acLayerCfg[layer].m_colourRemapSEICancelFlag );
    m_acTEncTop[layer].setCRISEIPersistenceFlag                     ( m_acLayerCfg[layer].m_colourRemapSEIPersistenceFlag );
    m_acTEncTop[layer].setCRISEIVideoSignalInfoPresentFlag          ( m_acLayerCfg[layer].m_colourRemapSEIVideoSignalInfoPresentFlag );
    m_acTEncTop[layer].setCRISEIFullRangeFlag                       ( m_acLayerCfg[layer].m_colourRemapSEIFullRangeFlag );
    m_acTEncTop[layer].setCRISEIPrimaries                           ( m_acLayerCfg[layer].m_colourRemapSEIPrimaries );
    m_acTEncTop[layer].setCRISEITransferFunction                    ( m_acLayerCfg[layer].m_colourRemapSEITransferFunction );
    m_acTEncTop[layer].setCRISEIMatrixCoefficients                  ( m_acLayerCfg[layer].m_colourRemapSEIMatrixCoefficients );
    m_acTEncTop[layer].setCRISEIInputBitDepth                       ( m_acLayerCfg[layer].m_colourRemapSEIInputBitDepth );
    m_acTEncTop[layer].setCRISEIBitDepth                            ( m_acLayerCfg[layer].m_colourRemapSEIBitDepth );
    m_acTEncTop[layer].setCRISEIPreLutNumValMinus1                  ( m_acLayerCfg[layer].m_colourRemapSEIPreLutNumValMinus1 );
    m_acTEncTop[layer].setCRISEIPreLutCodedValue                    ( m_acLayerCfg[layer].m_colourRemapSEIPreLutCodedValue );
    m_acTEncTop[layer].setCRISEIPreLutTargetValue                   ( m_acLayerCfg[layer].m_colourRemapSEIPreLutTargetValue );
    m_acTEncTop[layer].setCRISEIMatrixPresentFlag                   ( m_acLayerCfg[layer].m_colourRemapSEIMatrixPresentFlag );
    m_acTEncTop[layer].setCRISEILog2MatrixDenom                     ( m_acLayerCfg[layer].m_colourRemapSEILog2MatrixDenom );
    m_acTEncTop[layer].setCRISEICoeffs                              ( m_acLayerCfg[layer].m_colourRemapSEICoeffs );
    m_acTEncTop[layer].setCRISEIPostLutNumValMinus1                 ( m_acLayerCfg[layer].m_colourRemapSEIPostLutNumValMinus1 );
    m_acTEncTop[layer].setCRISEIPostLutCodedValue                   ( m_acLayerCfg[layer]. m_colourRemapSEIPostLutCodedValue );
    m_acTEncTop[layer].setCRISEIPostLutTargetValue                  ( m_acLayerCfg[layer].m_colourRemapSEIPostLutTargetValue );
#endif
    m_acTEncTop[layer].setFramePackingArrangementSEIEnabled( m_framePackingSEIEnabled );
    m_acTEncTop[layer].setFramePackingArrangementSEIType( m_framePackingSEIType );
    m_acTEncTop[layer].setFramePackingArrangementSEIId( m_framePackingSEIId );
    m_acTEncTop[layer].setFramePackingArrangementSEIQuincunx( m_framePackingSEIQuincunx );
    m_acTEncTop[layer].setFramePackingArrangementSEIInterpretation( m_framePackingSEIInterpretation );
    m_acTEncTop[layer].setDisplayOrientationSEIAngle( m_displayOrientationSEIAngle );
    m_acTEncTop[layer].setTemporalLevel0IndexSEIEnabled( m_temporalLevel0IndexSEIEnabled );
    m_acTEncTop[layer].setGradualDecodingRefreshInfoEnabled( m_gradualDecodingRefreshInfoEnabled );
    m_acTEncTop[layer].setDecodingUnitInfoSEIEnabled( m_decodingUnitInfoSEIEnabled );
#if LAYERS_NOT_PRESENT_SEI
    m_acTEncTop[layer].setLayersNotPresentSEIEnabled( m_layersNotPresentSEIEnabled );
#endif
    m_acTEncTop[layer].setSOPDescriptionSEIEnabled( m_SOPDescriptionSEIEnabled );
    m_acTEncTop[layer].setScalableNestingSEIEnabled( m_scalableNestingSEIEnabled );
#if Q0189_TMVP_CONSTRAINTS
    m_acTEncTop[layer].setTMVPConstraintsSEIEnabled( m_TMVPConstraintsSEIEnabled);           
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
    m_acTEncTop[layer].setInterLayerConstrainedTileSetsSEIEnabled( m_interLayerConstrainedTileSetsSEIEnabled );
    m_acTEncTop[layer].setIlNumSetsInMessage( m_ilNumSetsInMessage );
    m_acTEncTop[layer].setSkippedTileSetPresentFlag( m_skippedTileSetPresentFlag );
    m_acTEncTop[layer].setTopLeftTileIndex( m_topLeftTileIndex );
    m_acTEncTop[layer].setBottomRightTileIndex( m_bottomRightTileIndex );
    m_acTEncTop[layer].setIlcIdc( m_ilcIdc );
#endif
    m_acTEncTop[layer].setTileUniformSpacingFlag     ( m_tileUniformSpacingFlag );
    m_acTEncTop[layer].setNumColumnsMinus1           ( m_numTileColumnsMinus1 );
    m_acTEncTop[layer].setNumRowsMinus1              ( m_numTileRowsMinus1 );
    if(!m_tileUniformSpacingFlag)
    {
      m_acTEncTop[layer].setColumnWidth              ( m_tileColumnWidth );
      m_acTEncTop[layer].setRowHeight                ( m_tileRowHeight );
    }
    m_acTEncTop[layer].xCheckGSParameters();
    Int uiTilesCount = (m_numTileRowsMinus1+1) * (m_numTileColumnsMinus1+1);
    if(uiTilesCount == 1)
    {
      m_bLFCrossTileBoundaryFlag = true;
    }
    m_acTEncTop[layer].setLFCrossTileBoundaryFlag( m_bLFCrossTileBoundaryFlag );
    m_acTEncTop[layer].setWaveFrontSynchro           ( m_acLayerCfg[layer].m_waveFrontSynchro );
    m_acTEncTop[layer].setWaveFrontSubstreams        ( m_acLayerCfg[layer].m_iWaveFrontSubstreams );
    m_acTEncTop[layer].setTMVPModeId ( m_TMVPModeId );
    m_acTEncTop[layer].setUseScalingListId           ( m_useScalingListId  );
    m_acTEncTop[layer].setScalingListFile            ( m_scalingListFile   );
    m_acTEncTop[layer].setSignHideFlag(m_signHideFlag);
#if RC_SHVC_HARMONIZATION
    m_acTEncTop[layer].setUseRateCtrl     (m_acLayerCfg[layer].getRCEnableRateControl());
    m_acTEncTop[layer].setTargetBitrate   (m_acLayerCfg[layer].getRCTargetBitrate());
    m_acTEncTop[layer].setKeepHierBit     (m_acLayerCfg[layer].getRCKeepHierarchicalBit());
    m_acTEncTop[layer].setLCULevelRC      (m_acLayerCfg[layer].getRCLCULevelRC());
    m_acTEncTop[layer].setUseLCUSeparateModel (m_acLayerCfg[layer].getRCUseLCUSeparateModel());
    m_acTEncTop[layer].setInitialQP           (m_acLayerCfg[layer].getRCInitialQP());
    m_acTEncTop[layer].setForceIntraQP        (m_acLayerCfg[layer].getRCForceIntraQP());
#else
    m_acTEncTop[layer].setUseRateCtrl         ( m_RCEnableRateControl );
    m_acTEncTop[layer].setTargetBitrate       ( m_RCTargetBitrate );
    m_acTEncTop[layer].setKeepHierBit         ( m_RCKeepHierarchicalBit );
    m_acTEncTop[layer].setLCULevelRC          ( m_RCLCULevelRC );
    m_acTEncTop[layer].setUseLCUSeparateModel ( m_RCUseLCUSeparateModel );
    m_acTEncTop[layer].setInitialQP           ( m_RCInitialQP );
    m_acTEncTop[layer].setForceIntraQP        ( m_RCForceIntraQP );
#endif
    m_acTEncTop[layer].setTransquantBypassEnableFlag(m_TransquantBypassEnableFlag);
    m_acTEncTop[layer].setCUTransquantBypassFlagForceValue(m_CUTransquantBypassFlagForce);
    m_acTEncTop[layer].setUseRecalculateQPAccordingToLambda( m_recalculateQPAccordingToLambda );
    m_acTEncTop[layer].setUseStrongIntraSmoothing( m_useStrongIntraSmoothing );
    m_acTEncTop[layer].setActiveParameterSetsSEIEnabled ( m_activeParameterSetsSEIEnabled );
    m_acTEncTop[layer].setVuiParametersPresentFlag( m_vuiParametersPresentFlag );
    m_acTEncTop[layer].setAspectRatioInfoPresentFlag( m_aspectRatioInfoPresentFlag);
    m_acTEncTop[layer].setAspectRatioIdc( m_aspectRatioIdc );
    m_acTEncTop[layer].setSarWidth( m_sarWidth );
    m_acTEncTop[layer].setSarHeight( m_sarHeight );
    m_acTEncTop[layer].setOverscanInfoPresentFlag( m_overscanInfoPresentFlag );
    m_acTEncTop[layer].setOverscanAppropriateFlag( m_overscanAppropriateFlag );
    m_acTEncTop[layer].setVideoSignalTypePresentFlag( m_videoSignalTypePresentFlag );
    m_acTEncTop[layer].setVideoFormat( m_videoFormat );
    m_acTEncTop[layer].setVideoFullRangeFlag( m_videoFullRangeFlag );
    m_acTEncTop[layer].setColourDescriptionPresentFlag( m_colourDescriptionPresentFlag );
    m_acTEncTop[layer].setColourPrimaries( m_colourPrimaries );
    m_acTEncTop[layer].setTransferCharacteristics( m_transferCharacteristics );
    m_acTEncTop[layer].setMatrixCoefficients( m_matrixCoefficients );
    m_acTEncTop[layer].setChromaLocInfoPresentFlag( m_chromaLocInfoPresentFlag );
    m_acTEncTop[layer].setChromaSampleLocTypeTopField( m_chromaSampleLocTypeTopField );
    m_acTEncTop[layer].setChromaSampleLocTypeBottomField( m_chromaSampleLocTypeBottomField );
    m_acTEncTop[layer].setNeutralChromaIndicationFlag( m_neutralChromaIndicationFlag );
    m_acTEncTop[layer].setDefaultDisplayWindow( m_defDispWinLeftOffset, m_defDispWinRightOffset, m_defDispWinTopOffset, m_defDispWinBottomOffset );
    m_acTEncTop[layer].setFrameFieldInfoPresentFlag( m_frameFieldInfoPresentFlag );
    m_acTEncTop[layer].setPocProportionalToTimingFlag( m_pocProportionalToTimingFlag );
    m_acTEncTop[layer].setNumTicksPocDiffOneMinus1   ( m_numTicksPocDiffOneMinus1    );
    m_acTEncTop[layer].setBitstreamRestrictionFlag( m_bitstreamRestrictionFlag );
    m_acTEncTop[layer].setTilesFixedStructureFlag( m_tilesFixedStructureFlag );
    m_acTEncTop[layer].setMotionVectorsOverPicBoundariesFlag( m_motionVectorsOverPicBoundariesFlag );
    m_acTEncTop[layer].setMinSpatialSegmentationIdc( m_minSpatialSegmentationIdc );
    m_acTEncTop[layer].setMaxBytesPerPicDenom( m_maxBytesPerPicDenom );
    m_acTEncTop[layer].setMaxBitsPerMinCuDenom( m_maxBitsPerMinCuDenom );
    m_acTEncTop[layer].setLog2MaxMvLengthHorizontal( m_log2MaxMvLengthHorizontal );
    m_acTEncTop[layer].setLog2MaxMvLengthVertical( m_log2MaxMvLengthVertical );
    m_acTEncTop[layer].setElRapSliceTypeB(layer == 0? 0 : m_elRapSliceBEnabled);
    if( layer > 0 )
    {
#if REF_REGION_OFFSET
#if AUXILIARY_PICTURES
      Int cf = m_acLayerCfg[layer].m_chromaFormatIDC;
      Int subWidthC  = ( cf == CHROMA_420 || cf == CHROMA_422 ) ? 2 : 1;
      Int subHeightC = ( cf == CHROMA_420 ) ? 2 : 1;
#else
      Int subWidthC  = 2;
      Int subHeightC = 2;
#endif
#endif
      m_acTEncTop[layer].setNumScaledRefLayerOffsets( m_acLayerCfg[layer].m_numScaledRefLayerOffsets );
      for(Int i = 0; i < m_acLayerCfg[layer].m_numScaledRefLayerOffsets; i++)
      {
#if O0098_SCALED_REF_LAYER_ID
        m_acTEncTop[layer].setScaledRefLayerId(i, m_acLayerCfg[layer].m_scaledRefLayerId[i]);
#endif
#if REF_REGION_OFFSET
        m_acTEncTop[layer].setScaledRefLayerOffsetPresentFlag( i, m_acLayerCfg[layer].m_scaledRefLayerOffsetPresentFlag[i] );
        m_acTEncTop[layer].getScaledRefLayerWindow(i).setWindow( subWidthC  * m_acLayerCfg[layer].m_scaledRefLayerLeftOffset[i], subWidthC  * m_acLayerCfg[layer].m_scaledRefLayerRightOffset[i],
                                                                 subHeightC * m_acLayerCfg[layer].m_scaledRefLayerTopOffset[i],  subHeightC * m_acLayerCfg[layer].m_scaledRefLayerBottomOffset[i]);
#else
#if P0312_VERT_PHASE_ADJ
        m_acTEncTop[layer].setVertPhasePositionEnableFlag( i, m_acLayerCfg[layer].m_vertPhasePositionEnableFlag[i] );
        m_acTEncTop[layer].getScaledRefLayerWindow(i).setWindow( 2*m_acLayerCfg[layer].m_scaledRefLayerLeftOffset[i], 2*m_acLayerCfg[layer].m_scaledRefLayerRightOffset[i],
                                                  2*m_acLayerCfg[layer].m_scaledRefLayerTopOffset[i], 2*m_acLayerCfg[layer].m_scaledRefLayerBottomOffset[i], m_acLayerCfg[layer].m_vertPhasePositionEnableFlag[i] );
#else
        m_acTEncTop[layer].getScaledRefLayerWindow(i).setWindow( 2*m_acLayerCfg[layer].m_scaledRefLayerLeftOffset[i], 2*m_acLayerCfg[layer].m_scaledRefLayerRightOffset[i],
                                                  2*m_acLayerCfg[layer].m_scaledRefLayerTopOffset[i], 2*m_acLayerCfg[layer].m_scaledRefLayerBottomOffset[i]);
#endif
#endif
#if R0209_GENERIC_PHASE
        m_acTEncTop[layer].setResamplePhaseSetPresentFlag( i, m_acLayerCfg[layer].m_resamplePhaseSetPresentFlag[i] );
        m_acTEncTop[layer].setPhaseHorLuma( i, m_acLayerCfg[layer].m_phaseHorLuma[i] );
        m_acTEncTop[layer].setPhaseVerLuma( i, m_acLayerCfg[layer].m_phaseVerLuma[i] );
        m_acTEncTop[layer].setPhaseHorChroma( i, m_acLayerCfg[layer].m_phaseHorChroma[i] );
        m_acTEncTop[layer].setPhaseVerChroma( i, m_acLayerCfg[layer].m_phaseVerChroma[i] );
#endif
      }
    }
#if M0040_ADAPTIVE_RESOLUTION_CHANGE
    m_acTEncTop[layer].setAdaptiveResolutionChange( m_adaptiveResolutionChange );
#endif
#if AUXILIARY_PICTURES
    m_acTEncTop[layer].setChromaFormatIDC( m_acLayerCfg[layer].m_chromaFormatIDC );
#endif
#if O0153_ALT_OUTPUT_LAYER_FLAG
    m_acTEncTop[layer].setAltOuputLayerFlag( m_altOutputLayerFlag );
#endif
#if O0149_CROSS_LAYER_BLA_FLAG
    m_acTEncTop[layer].setCrossLayerBLAFlag( m_crossLayerBLAFlag );
#endif
#if Q0048_CGS_3D_ASYMLUT
    m_acTEncTop[layer].setCGSFlag( layer == 0 ? 0 : m_nCGSFlag );
    m_acTEncTop[layer].setCGSMaxOctantDepth( m_nCGSMaxOctantDepth );
    m_acTEncTop[layer].setCGSMaxYPartNumLog2( m_nCGSMaxYPartNumLog2 );
    m_acTEncTop[layer].setCGSLUTBit( m_nCGSLUTBit );
#if R0151_CGS_3D_ASYMLUT_IMPROVE
    m_acTEncTop[layer].setCGSAdaptChroma( m_nCGSAdaptiveChroma );
#endif
#if R0179_ENC_OPT_3DLUT_SIZE
    m_acTEncTop[layer].setCGSLutSizeRDO( m_nCGSLutSizeRDO );
#endif
#endif
#if Q0078_ADD_LAYER_SETS
    m_acTEncTop[layer].setNumAddLayerSets( m_numAddLayerSets );
#endif
  }
}
#else //SVC_EXTENSION
Void TAppEncTop::xInitLibCfg()
{
  TComVPS vps;

  vps.setMaxTLayers                       ( m_maxTempLayer );
  if (m_maxTempLayer == 1)
  {
    vps.setTemporalNestingFlag(true);
  }
  vps.setMaxLayers                        ( 1 );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    vps.setNumReorderPics                 ( m_numReorderPics[i], i );
    vps.setMaxDecPicBuffering             ( m_maxDecPicBuffering[i], i );
  }
  m_cTEncTop.setVPS(&vps);

  m_cTEncTop.setProfile(m_profile);
  m_cTEncTop.setLevel(m_levelTier, m_level);
  m_cTEncTop.setProgressiveSourceFlag(m_progressiveSourceFlag);
  m_cTEncTop.setInterlacedSourceFlag(m_interlacedSourceFlag);
  m_cTEncTop.setNonPackedConstraintFlag(m_nonPackedConstraintFlag);
  m_cTEncTop.setFrameOnlyConstraintFlag(m_frameOnlyConstraintFlag);

  m_cTEncTop.setFrameRate                    ( m_iFrameRate );
  m_cTEncTop.setFrameSkip                    ( m_FrameSkip );
  m_cTEncTop.setSourceWidth                  ( m_iSourceWidth );
  m_cTEncTop.setSourceHeight                 ( m_iSourceHeight );
  m_cTEncTop.setConformanceWindow            ( m_confWinLeft, m_confWinRight, m_confWinTop, m_confWinBottom );
  m_cTEncTop.setFramesToBeEncoded            ( m_framesToBeEncoded );

  //====== Coding Structure ========
  m_cTEncTop.setIntraPeriod                  ( m_iIntraPeriod );
  m_cTEncTop.setDecodingRefreshType          ( m_iDecodingRefreshType );
  m_cTEncTop.setGOPSize                      ( m_iGOPSize );
  m_cTEncTop.setGopList                      ( m_GOPList );
  m_cTEncTop.setExtraRPSs                    ( m_extraRPSs );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    m_cTEncTop.setNumReorderPics             ( m_numReorderPics[i], i );
    m_cTEncTop.setMaxDecPicBuffering         ( m_maxDecPicBuffering[i], i );
  }
  for( UInt uiLoop = 0; uiLoop < MAX_TLAYER; ++uiLoop )
  {
    m_cTEncTop.setLambdaModifier( uiLoop, m_adLambdaModifier[ uiLoop ] );
  }
  m_cTEncTop.setQP                           ( m_iQP );

  m_cTEncTop.setPad                          ( m_aiPad );

  m_cTEncTop.setMaxTempLayer                 ( m_maxTempLayer );
  m_cTEncTop.setUseAMP( m_enableAMP );

  //===== Slice ========

  //====== Loop/Deblock Filter ========
  m_cTEncTop.setLoopFilterDisable            ( m_bLoopFilterDisable       );
  m_cTEncTop.setLoopFilterOffsetInPPS        ( m_loopFilterOffsetInPPS );
  m_cTEncTop.setLoopFilterBetaOffset         ( m_loopFilterBetaOffsetDiv2  );
  m_cTEncTop.setLoopFilterTcOffset           ( m_loopFilterTcOffsetDiv2    );
  m_cTEncTop.setDeblockingFilterControlPresent( m_DeblockingFilterControlPresent);
  m_cTEncTop.setDeblockingFilterMetric       ( m_DeblockingFilterMetric );

  //====== Motion search ========
  m_cTEncTop.setFastSearch                   ( m_iFastSearch  );
  m_cTEncTop.setSearchRange                  ( m_iSearchRange );
  m_cTEncTop.setBipredSearchRange            ( m_bipredSearchRange );

  //====== Quality control ========
  m_cTEncTop.setMaxDeltaQP                   ( m_iMaxDeltaQP  );
  m_cTEncTop.setMaxCuDQPDepth                ( m_iMaxCuDQPDepth  );

  m_cTEncTop.setChromaCbQpOffset               ( m_cbQpOffset     );
  m_cTEncTop.setChromaCrQpOffset            ( m_crQpOffset  );

#if ADAPTIVE_QP_SELECTION
  m_cTEncTop.setUseAdaptQpSelect             ( m_bUseAdaptQpSelect   );
#endif

  m_cTEncTop.setUseAdaptiveQP                ( m_bUseAdaptiveQP  );
  m_cTEncTop.setQPAdaptationRange            ( m_iQPAdaptationRange );

  //====== Tool list ========
  m_cTEncTop.setDeltaQpRD                    ( m_uiDeltaQpRD  );
  m_cTEncTop.setUseASR                       ( m_bUseASR      );
  m_cTEncTop.setUseHADME                     ( m_bUseHADME    );
  m_cTEncTop.setdQPs                         ( m_aidQP        );
  m_cTEncTop.setUseRDOQ                      ( m_useRDOQ     );
  m_cTEncTop.setUseRDOQTS                    ( m_useRDOQTS   );
  m_cTEncTop.setRDpenalty                 ( m_rdPenalty );
  m_cTEncTop.setQuadtreeTULog2MaxSize        ( m_uiQuadtreeTULog2MaxSize );
  m_cTEncTop.setQuadtreeTULog2MinSize        ( m_uiQuadtreeTULog2MinSize );
  m_cTEncTop.setQuadtreeTUMaxDepthInter      ( m_uiQuadtreeTUMaxDepthInter );
  m_cTEncTop.setQuadtreeTUMaxDepthIntra      ( m_uiQuadtreeTUMaxDepthIntra );
  m_cTEncTop.setUseFastEnc                   ( m_bUseFastEnc  );
  m_cTEncTop.setUseEarlyCU                   ( m_bUseEarlyCU  );
  m_cTEncTop.setUseFastDecisionForMerge      ( m_useFastDecisionForMerge  );
  m_cTEncTop.setUseCbfFastMode            ( m_bUseCbfFastMode  );
  m_cTEncTop.setUseEarlySkipDetection            ( m_useEarlySkipDetection );
#if FAST_INTRA_SHVC
  m_cTEncTop.setUseFastIntraScalable            ( m_useFastIntraScalable );
#endif

  m_cTEncTop.setUseTransformSkip             ( m_useTransformSkip      );
  m_cTEncTop.setUseTransformSkipFast         ( m_useTransformSkipFast  );
  m_cTEncTop.setUseConstrainedIntraPred      ( m_bUseConstrainedIntraPred );
  m_cTEncTop.setPCMLog2MinSize          ( m_uiPCMLog2MinSize);
  m_cTEncTop.setUsePCM                       ( m_usePCM );
  m_cTEncTop.setPCMLog2MaxSize               ( m_pcmLog2MaxSize);
  m_cTEncTop.setMaxNumMergeCand              ( m_maxNumMergeCand );


  //====== Weighted Prediction ========
  m_cTEncTop.setUseWP                   ( m_useWeightedPred      );
  m_cTEncTop.setWPBiPred                ( m_useWeightedBiPred   );
  //====== Parallel Merge Estimation ========
  m_cTEncTop.setLog2ParallelMergeLevelMinus2 ( m_log2ParallelMergeLevel - 2 );

  //====== Slice ========
  m_cTEncTop.setSliceMode               ( m_sliceMode                );
  m_cTEncTop.setSliceArgument           ( m_sliceArgument            );

  //====== Dependent Slice ========
  m_cTEncTop.setSliceSegmentMode        ( m_sliceSegmentMode         );
  m_cTEncTop.setSliceSegmentArgument    ( m_sliceSegmentArgument     );
  Int iNumPartInCU = 1<<(m_uiMaxCUDepth<<1);
  if(m_sliceSegmentMode==FIXED_NUMBER_OF_LCU)
  {
    m_cTEncTop.setSliceSegmentArgument ( m_sliceSegmentArgument * iNumPartInCU );
  }
  if(m_sliceMode==FIXED_NUMBER_OF_LCU)
  {
    m_cTEncTop.setSliceArgument ( m_sliceArgument * iNumPartInCU );
  }
  if(m_sliceMode==FIXED_NUMBER_OF_TILES)
  {
    m_cTEncTop.setSliceArgument ( m_sliceArgument );
  }

  if(m_sliceMode == 0 )
  {
    m_bLFCrossSliceBoundaryFlag = true;
  }
  m_cTEncTop.setLFCrossSliceBoundaryFlag( m_bLFCrossSliceBoundaryFlag );
  m_cTEncTop.setUseSAO ( m_bUseSAO );
  m_cTEncTop.setMaxNumOffsetsPerPic (m_maxNumOffsetsPerPic);

  m_cTEncTop.setSaoLcuBoundary (m_saoLcuBoundary);
  m_cTEncTop.setPCMInputBitDepthFlag  ( m_bPCMInputBitDepthFlag);
  m_cTEncTop.setPCMFilterDisableFlag  ( m_bPCMFilterDisableFlag);

  m_cTEncTop.setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
  m_cTEncTop.setRecoveryPointSEIEnabled( m_recoveryPointSEIEnabled );
  m_cTEncTop.setBufferingPeriodSEIEnabled( m_bufferingPeriodSEIEnabled );
  m_cTEncTop.setPictureTimingSEIEnabled( m_pictureTimingSEIEnabled );
  m_cTEncTop.setToneMappingInfoSEIEnabled                 ( m_toneMappingInfoSEIEnabled );
  m_cTEncTop.setTMISEIToneMapId                           ( m_toneMapId );
  m_cTEncTop.setTMISEIToneMapCancelFlag                   ( m_toneMapCancelFlag );
  m_cTEncTop.setTMISEIToneMapPersistenceFlag              ( m_toneMapPersistenceFlag );
  m_cTEncTop.setTMISEICodedDataBitDepth                   ( m_toneMapCodedDataBitDepth );
  m_cTEncTop.setTMISEITargetBitDepth                      ( m_toneMapTargetBitDepth );
  m_cTEncTop.setTMISEIModelID                             ( m_toneMapModelId );
  m_cTEncTop.setTMISEIMinValue                            ( m_toneMapMinValue );
  m_cTEncTop.setTMISEIMaxValue                            ( m_toneMapMaxValue );
  m_cTEncTop.setTMISEISigmoidMidpoint                     ( m_sigmoidMidpoint );
  m_cTEncTop.setTMISEISigmoidWidth                        ( m_sigmoidWidth );
  m_cTEncTop.setTMISEIStartOfCodedInterva                 ( m_startOfCodedInterval );
  m_cTEncTop.setTMISEINumPivots                           ( m_numPivots );
  m_cTEncTop.setTMISEICodedPivotValue                     ( m_codedPivotValue );
  m_cTEncTop.setTMISEITargetPivotValue                    ( m_targetPivotValue );
  m_cTEncTop.setTMISEICameraIsoSpeedIdc                   ( m_cameraIsoSpeedIdc );
  m_cTEncTop.setTMISEICameraIsoSpeedValue                 ( m_cameraIsoSpeedValue );
  m_cTEncTop.setTMISEIExposureIndexIdc                    ( m_exposureIndexIdc );
  m_cTEncTop.setTMISEIExposureIndexValue                  ( m_exposureIndexValue );
  m_cTEncTop.setTMISEIExposureCompensationValueSignFlag   ( m_exposureCompensationValueSignFlag );
  m_cTEncTop.setTMISEIExposureCompensationValueNumerator  ( m_exposureCompensationValueNumerator );
  m_cTEncTop.setTMISEIExposureCompensationValueDenomIdc   ( m_exposureCompensationValueDenomIdc );
  m_cTEncTop.setTMISEIRefScreenLuminanceWhite             ( m_refScreenLuminanceWhite );
  m_cTEncTop.setTMISEIExtendedRangeWhiteLevel             ( m_extendedRangeWhiteLevel );
  m_cTEncTop.setTMISEINominalBlackLevelLumaCodeValue      ( m_nominalBlackLevelLumaCodeValue );
  m_cTEncTop.setTMISEINominalWhiteLevelLumaCodeValue      ( m_nominalWhiteLevelLumaCodeValue );
  m_cTEncTop.setTMISEIExtendedWhiteLevelLumaCodeValue     ( m_extendedWhiteLevelLumaCodeValue );
#if P0050_KNEE_FUNCTION_SEI
  m_cTEncTop.setKneeSEIEnabled              ( m_kneeSEIEnabled );
  m_cTEncTop.setKneeSEIId                   ( m_kneeSEIId );
  m_cTEncTop.setKneeSEICancelFlag           ( m_kneeSEICancelFlag );
  m_cTEncTop.setKneeSEIPersistenceFlag      ( m_kneeSEIPersistenceFlag );
  m_cTEncTop.setKneeSEIMappingFlag          ( m_kneeSEIMappingFlag );
  m_cTEncTop.setKneeSEIInputDrange          ( m_kneeSEIInputDrange );
  m_cTEncTop.setKneeSEIInputDispLuminance   ( m_kneeSEIInputDispLuminance );
  m_cTEncTop.setKneeSEIOutputDrange         ( m_kneeSEIOutputDrange );
  m_cTEncTop.setKneeSEIOutputDispLuminance  ( m_kneeSEIOutputDispLuminance );
  m_cTEncTop.setKneeSEINumKneePointsMinus1  ( m_kneeSEINumKneePointsMinus1 );
  m_cTEncTop.setKneeSEIInputKneePoint       ( m_kneeSEIInputKneePoint );
  m_cTEncTop.setKneeSEIOutputKneePoint      ( m_kneeSEIOutputKneePoint );
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  m_cTEncTop.setCRISEIFile                       ( const_cast<Char*>(m_colourRemapSEIFile.c_str()) );
  m_cTEncTop.setCRISEIId                         ( m_colourRemapSEIId );
  m_cTEncTop.setCRISEICancelFlag                 ( m_colourRemapSEICancelFlag );
  m_cTEncTop.setCRISEIPersistenceFlag            ( m_colourRemapSEIPersistenceFlag );
  m_cTEncTop.setCRISEIVideoSignalInfoPresentFlag ( m_colourRemapSEIVideoSignalInfoPresentFlag );
  m_cTEncTop.setCRISEIFullRangeFlag              ( m_colourRemapSEIFullRangeFlag );
  m_cTEncTop.setCRISEIPrimaries                  ( m_colourRemapSEIPrimaries );
  m_cTEncTop.setCRISEITransferFunction           ( m_colourRemapSEITransferFunction );
  m_cTEncTop.setCRISEIMatrixCoefficients         ( m_colourRemapSEIMatrixCoefficients );
  m_cTEncTop.setCRISEIInputBitDepth              ( m_colourRemapSEIInputBitDepth );
  m_cTEncTop.setCRISEIBitDepth                   ( m_colourRemapSEIBitDepth );
  m_cTEncTop.setCRISEIPreLutNumValMinus1         ( m_colourRemapSEIPreLutNumValMinus1 );
  m_cTEncTop.setCRISEIPreLutCodedValue           ( m_colourRemapSEIPreLutCodedValue );
  m_cTEncTop.setCRISEIPreLutTargetValue          ( m_colourRemapSEIPreLutTargetValue );
  m_cTEncTop.setCRISEIMatrixPresentFlag          ( m_colourRemapSEIMatrixPresentFlag );
  m_cTEncTop.setCRISEILog2MatrixDenom            ( m_colourRemapSEILog2MatrixDenom );
  m_cTEncTop.setCRISEICoeffs                     ( m_colourRemapSEICoeffs );
  m_cTEncTop.setCRISEIPostLutNumValMinus1        ( m_colourRemapSEIPostLutNumValMinus1 );
  m_cTEncTop.setCRISEIPostLutCodedValue          ( m_colourRemapSEIPostLutCodedValue );
  m_cTEncTop.setCRISEIPostLutTargetValue         ( m_colourRemapSEIPostLutTargetValue );
#endif
  m_cTEncTop.setFramePackingArrangementSEIEnabled( m_framePackingSEIEnabled );
  m_cTEncTop.setFramePackingArrangementSEIType( m_framePackingSEIType );
  m_cTEncTop.setFramePackingArrangementSEIId( m_framePackingSEIId );
  m_cTEncTop.setFramePackingArrangementSEIQuincunx( m_framePackingSEIQuincunx );
  m_cTEncTop.setFramePackingArrangementSEIInterpretation( m_framePackingSEIInterpretation );
  m_cTEncTop.setDisplayOrientationSEIAngle( m_displayOrientationSEIAngle );
  m_cTEncTop.setTemporalLevel0IndexSEIEnabled( m_temporalLevel0IndexSEIEnabled );
  m_cTEncTop.setGradualDecodingRefreshInfoEnabled( m_gradualDecodingRefreshInfoEnabled );
  m_cTEncTop.setDecodingUnitInfoSEIEnabled( m_decodingUnitInfoSEIEnabled );
#if LAYERS_NOT_PRESENT_SEI
  m_cTEncTop.setLayersNotPresentSEIEnabled( m_layersNotPresentSEIEnabled );
#endif
  m_cTEncTop.setSOPDescriptionSEIEnabled( m_SOPDescriptionSEIEnabled );
  m_cTEncTop.setScalableNestingSEIEnabled( m_scalableNestingSEIEnabled );
  m_cTEncTop.setTileUniformSpacingFlag     ( m_tileUniformSpacingFlag );
  m_cTEncTop.setNumColumnsMinus1           ( m_numTileColumnsMinus1 );
  m_cTEncTop.setNumRowsMinus1              ( m_numTileRowsMinus1 );
  if(!m_tileUniformSpacingFlag)
  {
    m_cTEncTop.setColumnWidth              ( m_tileColumnWidth );
    m_cTEncTop.setRowHeight                ( m_tileRowHeight );
  }
  m_cTEncTop.xCheckGSParameters();
  Int uiTilesCount          = (m_numTileRowsMinus1+1) * (m_numTileColumnsMinus1+1);
  if(uiTilesCount == 1)
  {
    m_bLFCrossTileBoundaryFlag = true;
  }
  m_cTEncTop.setLFCrossTileBoundaryFlag( m_bLFCrossTileBoundaryFlag );
  m_cTEncTop.setWaveFrontSynchro           ( m_iWaveFrontSynchro );
  m_cTEncTop.setWaveFrontSubstreams        ( m_iWaveFrontSubstreams );
  m_cTEncTop.setTMVPModeId ( m_TMVPModeId );
  m_cTEncTop.setUseScalingListId           ( m_useScalingListId  );
  m_cTEncTop.setScalingListFile            ( m_scalingListFile   );
  m_cTEncTop.setSignHideFlag(m_signHideFlag);
  m_cTEncTop.setUseRateCtrl         ( m_RCEnableRateControl );
  m_cTEncTop.setTargetBitrate       ( m_RCTargetBitrate );
  m_cTEncTop.setKeepHierBit         ( m_RCKeepHierarchicalBit );
  m_cTEncTop.setLCULevelRC          ( m_RCLCULevelRC );
  m_cTEncTop.setUseLCUSeparateModel ( m_RCUseLCUSeparateModel );
  m_cTEncTop.setInitialQP           ( m_RCInitialQP );
  m_cTEncTop.setForceIntraQP        ( m_RCForceIntraQP );
  m_cTEncTop.setTransquantBypassEnableFlag(m_TransquantBypassEnableFlag);
  m_cTEncTop.setCUTransquantBypassFlagForceValue(m_CUTransquantBypassFlagForce);
  m_cTEncTop.setUseRecalculateQPAccordingToLambda( m_recalculateQPAccordingToLambda );
  m_cTEncTop.setUseStrongIntraSmoothing( m_useStrongIntraSmoothing );
  m_cTEncTop.setActiveParameterSetsSEIEnabled ( m_activeParameterSetsSEIEnabled );
  m_cTEncTop.setVuiParametersPresentFlag( m_vuiParametersPresentFlag );
  m_cTEncTop.setAspectRatioInfoPresentFlag( m_aspectRatioInfoPresentFlag);
  m_cTEncTop.setAspectRatioIdc( m_aspectRatioIdc );
  m_cTEncTop.setSarWidth( m_sarWidth );
  m_cTEncTop.setSarHeight( m_sarHeight );
  m_cTEncTop.setOverscanInfoPresentFlag( m_overscanInfoPresentFlag );
  m_cTEncTop.setOverscanAppropriateFlag( m_overscanAppropriateFlag );
  m_cTEncTop.setVideoSignalTypePresentFlag( m_videoSignalTypePresentFlag );
  m_cTEncTop.setVideoFormat( m_videoFormat );
  m_cTEncTop.setVideoFullRangeFlag( m_videoFullRangeFlag );
  m_cTEncTop.setColourDescriptionPresentFlag( m_colourDescriptionPresentFlag );
  m_cTEncTop.setColourPrimaries( m_colourPrimaries );
  m_cTEncTop.setTransferCharacteristics( m_transferCharacteristics );
  m_cTEncTop.setMatrixCoefficients( m_matrixCoefficients );
  m_cTEncTop.setChromaLocInfoPresentFlag( m_chromaLocInfoPresentFlag );
  m_cTEncTop.setChromaSampleLocTypeTopField( m_chromaSampleLocTypeTopField );
  m_cTEncTop.setChromaSampleLocTypeBottomField( m_chromaSampleLocTypeBottomField );
  m_cTEncTop.setNeutralChromaIndicationFlag( m_neutralChromaIndicationFlag );
  m_cTEncTop.setDefaultDisplayWindow( m_defDispWinLeftOffset, m_defDispWinRightOffset, m_defDispWinTopOffset, m_defDispWinBottomOffset );
  m_cTEncTop.setFrameFieldInfoPresentFlag( m_frameFieldInfoPresentFlag );
  m_cTEncTop.setPocProportionalToTimingFlag( m_pocProportionalToTimingFlag );
  m_cTEncTop.setNumTicksPocDiffOneMinus1   ( m_numTicksPocDiffOneMinus1    );
  m_cTEncTop.setBitstreamRestrictionFlag( m_bitstreamRestrictionFlag );
  m_cTEncTop.setTilesFixedStructureFlag( m_tilesFixedStructureFlag );
  m_cTEncTop.setMotionVectorsOverPicBoundariesFlag( m_motionVectorsOverPicBoundariesFlag );
  m_cTEncTop.setMinSpatialSegmentationIdc( m_minSpatialSegmentationIdc );
  m_cTEncTop.setMaxBytesPerPicDenom( m_maxBytesPerPicDenom );
  m_cTEncTop.setMaxBitsPerMinCuDenom( m_maxBitsPerMinCuDenom );
  m_cTEncTop.setLog2MaxMvLengthHorizontal( m_log2MaxMvLengthHorizontal );
  m_cTEncTop.setLog2MaxMvLengthVertical( m_log2MaxMvLengthVertical );
}
#endif //SVC_EXTENSION

Void TAppEncTop::xCreateLib()
{
  // Video I/O
#if SVC_EXTENSION
  // initialize global variables
  initROM();

  for(UInt layer=0; layer<m_numLayers; layer++)
  {
#if O0194_DIFFERENT_BITDEPTH_EL_BL
    //2
    g_bitDepthY = m_acLayerCfg[layer].m_internalBitDepthY;
    g_bitDepthC = m_acLayerCfg[layer].m_internalBitDepthC;

    g_uiPCMBitDepthLuma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthY : m_acLayerCfg[layer].m_internalBitDepthY;
    g_uiPCMBitDepthChroma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthC : m_acLayerCfg[layer].m_internalBitDepthC;
#endif
#if LAYER_CTB
    g_uiMaxCUWidth  = g_auiLayerMaxCUWidth[layer];
    g_uiMaxCUHeight = g_auiLayerMaxCUHeight[layer];
    g_uiMaxCUDepth  = g_auiLayerMaxCUDepth[layer];
    g_uiAddCUDepth  = g_auiLayerAddCUDepth[layer];
#endif
#if O0194_DIFFERENT_BITDEPTH_EL_BL
    m_acTVideoIOYuvInputFile[layer].open( (Char *)m_acLayerCfg[layer].getInputFile().c_str(),  false, m_acLayerCfg[layer].m_inputBitDepthY, m_acLayerCfg[layer].m_inputBitDepthC, m_acLayerCfg[layer].m_internalBitDepthY, m_acLayerCfg[layer].m_internalBitDepthC );  // read  mode
#else
    m_acTVideoIOYuvInputFile[layer].open( (Char *)m_acLayerCfg[layer].getInputFile().c_str(),  false, m_inputBitDepthY, m_inputBitDepthC, m_internalBitDepthY, m_internalBitDepthC );  // read  mode
#endif
    m_acTVideoIOYuvInputFile[layer].skipFrames(m_FrameSkip, m_acLayerCfg[layer].getSourceWidth() - m_acLayerCfg[layer].getPad()[0], m_acLayerCfg[layer].getSourceHeight() - m_acLayerCfg[layer].getPad()[1]);

    if (!m_acLayerCfg[layer].getReconFile().empty())
    {
#if O0194_DIFFERENT_BITDEPTH_EL_BL
      m_acTVideoIOYuvReconFile[layer].open((Char *)m_acLayerCfg[layer].getReconFile().c_str(), true, m_acLayerCfg[layer].m_outputBitDepthY, m_acLayerCfg[layer].m_outputBitDepthC, m_acLayerCfg[layer].m_internalBitDepthY, m_acLayerCfg[layer].m_internalBitDepthC );  // write mode
#else
      m_acTVideoIOYuvReconFile[layer].open((Char *)m_acLayerCfg[layer].getReconFile().c_str(), true, m_outputBitDepthY, m_outputBitDepthC, m_internalBitDepthY, m_internalBitDepthC );  // write mode
#endif
    }

    m_acTEncTop[layer].create();
  }
#else //SVC_EXTENSION
  m_cTVideoIOYuvInputFile.open( m_pchInputFile,     false, m_inputBitDepthY, m_inputBitDepthC, m_internalBitDepthY, m_internalBitDepthC );  // read  mode
  m_cTVideoIOYuvInputFile.skipFrames(m_FrameSkip, m_iSourceWidth - m_aiPad[0], m_iSourceHeight - m_aiPad[1]);

  if (m_pchReconFile)
    m_cTVideoIOYuvReconFile.open(m_pchReconFile, true, m_outputBitDepthY, m_outputBitDepthC, m_internalBitDepthY, m_internalBitDepthC);  // write mode

  // Neo Decoder
  m_cTEncTop.create();
#endif //SVC_EXTENSION
}

Void TAppEncTop::xDestroyLib()
{
  // Video I/O
#if SVC_EXTENSION
  // destroy ROM
  destroyROM();

  for(UInt layer=0; layer<m_numLayers; layer++)
  {
#if LAYER_CTB
    g_uiMaxCUWidth  = g_auiLayerMaxCUWidth[layer];
    g_uiMaxCUHeight = g_auiLayerMaxCUHeight[layer];
    g_uiMaxCUDepth  = g_auiLayerMaxCUDepth[layer];
    g_uiAddCUDepth  = g_auiLayerAddCUDepth[layer];
#endif

    m_acTVideoIOYuvInputFile[layer].close();
    m_acTVideoIOYuvReconFile[layer].close();

    m_acTEncTop[layer].destroy();
  }
#else //SVC_EXTENSION
  m_cTVideoIOYuvInputFile.close();
  m_cTVideoIOYuvReconFile.close();

  // Neo Decoder
  m_cTEncTop.destroy();
#endif //SVC_EXTENSION
}

Void TAppEncTop::xInitLib(Bool isFieldCoding)
{
#if SVC_EXTENSION
  for(UInt layer=0; layer<m_numLayers; layer++)
  {
#if O0194_DIFFERENT_BITDEPTH_EL_BL
    //3
    g_bitDepthY = m_acLayerCfg[layer].m_internalBitDepthY;
    g_bitDepthC = m_acLayerCfg[layer].m_internalBitDepthC;

    g_uiPCMBitDepthLuma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthY : m_acLayerCfg[layer].m_internalBitDepthY;
    g_uiPCMBitDepthChroma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthC : m_acLayerCfg[layer].m_internalBitDepthC;
#endif
#if LAYER_CTB
    g_uiMaxCUWidth  = g_auiLayerMaxCUWidth[layer];
    g_uiMaxCUHeight = g_auiLayerMaxCUHeight[layer];
    g_uiMaxCUDepth  = g_auiLayerMaxCUDepth[layer];
    g_uiAddCUDepth  = g_auiLayerAddCUDepth[layer];

    memcpy( g_auiZscanToRaster, g_auiLayerZscanToRaster[layer], sizeof( g_auiZscanToRaster ) );
    memcpy( g_auiRasterToZscan, g_auiLayerRasterToZscan[layer], sizeof( g_auiRasterToZscan ) );
    memcpy( g_auiRasterToPelX,  g_auiLayerRasterToPelX[layer],  sizeof( g_auiRasterToPelX ) );
    memcpy( g_auiRasterToPelY,  g_auiLayerRasterToPelY[layer],  sizeof( g_auiRasterToPelY ) );
#endif
    m_acTEncTop[layer].init(isFieldCoding);
#if P0182_VPS_VUI_PS_FLAG
    m_acTEncTop[layer].getVPS()->setSPSId(layer, m_acTEncTop[layer].getSPS()->getSPSId());
    m_acTEncTop[layer].getVPS()->setPPSId(layer, m_acTEncTop[layer].getPPS()->getPPSId());
#endif
  }
  m_acTEncTop[0].getVPS()->setMaxLayers( m_numLayers );
#if VPS_EXTN_OP_LAYER_SETS
  TComVPS* vps = m_acTEncTop[0].getVPS();
  vps->setMaxLayerId(m_numLayers - 1);    // Set max-layer ID

  vps->setVpsExtensionFlag( m_numLayers > 1 ? true : false );

#if Q0078_ADD_LAYER_SETS
#if OUTPUT_LAYER_SETS_CONFIG
  if (m_numLayerSets > 1)
  {
    vps->setNumLayerSets(m_numLayerSets);
#else
  if (m_numLayerSets > 0)
  {
    vps->setNumLayerSets(m_numLayerSets+1);
#endif
    for (Int setId = 1; setId < vps->getNumLayerSets(); setId++)
    {
      for (Int layerId = 0; layerId <= vps->getMaxLayerId(); layerId++)
      {
        vps->setLayerIdIncludedFlag(false, setId, layerId);
      }
    }
    for (Int setId = 1; setId < vps->getNumLayerSets(); setId++)
    {
#if OUTPUT_LAYER_SETS_CONFIG
      for (Int i = 0; i < m_numLayerInIdList[setId]; i++)
      {
        Int layerId = m_layerSetLayerIdList[setId][i];
#else
      for (Int i = 0; i < m_numLayerInIdList[setId-1]; i++)
      {
        Int layerId = m_layerSetLayerIdList[setId-1][i];
#endif
#if O0194_DIFFERENT_BITDEPTH_EL_BL
        //4
        g_bitDepthY = m_acLayerCfg[layerId].m_internalBitDepthY;
        g_bitDepthC = m_acLayerCfg[layerId].m_internalBitDepthC;

        g_uiPCMBitDepthLuma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layerId].m_inputBitDepthY : m_acLayerCfg[layerId].m_internalBitDepthY;
        g_uiPCMBitDepthChroma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layerId].m_inputBitDepthC : m_acLayerCfg[layerId].m_internalBitDepthC;
#endif

        vps->setLayerIdIncludedFlag(true, setId, layerId);
      }
    }
  }
  else
  {
    // Default layer sets
#endif
    vps->setNumLayerSets(m_numLayers);
    for (Int setId = 1; setId < vps->getNumLayerSets(); setId++)
    {
      for (Int layerId = 0; layerId <= vps->getMaxLayerId(); layerId++)
      {
#if O0194_DIFFERENT_BITDEPTH_EL_BL
        //4
        g_bitDepthY = m_acLayerCfg[layerId].m_internalBitDepthY;
        g_bitDepthC = m_acLayerCfg[layerId].m_internalBitDepthC;

        g_uiPCMBitDepthLuma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layerId].m_inputBitDepthY : m_acLayerCfg[layerId].m_internalBitDepthY;
        g_uiPCMBitDepthChroma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layerId].m_inputBitDepthC : m_acLayerCfg[layerId].m_internalBitDepthC;
#endif
        if (layerId <= setId)
        {
          vps->setLayerIdIncludedFlag(true, setId, layerId);
        }
        else
        {
          vps->setLayerIdIncludedFlag(false, setId, layerId);
        }
      }
    }
#if Q0078_ADD_LAYER_SETS
  }
#endif
#if Q0078_ADD_LAYER_SETS
  vps->setVpsNumLayerSetsMinus1(vps->getNumLayerSets() - 1);
  vps->setNumAddLayerSets(m_numAddLayerSets);
  vps->setNumLayerSets(vps->getNumLayerSets() + vps->getNumAddLayerSets());
  if (m_numAddLayerSets > 0)
  {
    for (Int setId = 0; setId < m_numAddLayerSets; setId++)
    {
      for (Int j = 0; j < m_numHighestLayerIdx[setId]; j++)
      {
        vps->setHighestLayerIdxPlus1(setId, j + 1, m_highestLayerIdx[setId][j] + 1);
      }
    }
  }
#endif
#if VPS_EXTN_MASK_AND_DIM_INFO
  UInt i = 0, dimIdLen = 0;
#if AVC_BASE
#if VPS_AVC_BL_FLAG_REMOVAL
  vps->setNonHEVCBaseLayerFlag( m_nonHEVCBaseLayerFlag );
  if ( m_nonHEVCBaseLayerFlag )
  {
    vps->setBaseLayerInternalFlag (false);
  }
#else
  vps->setAvcBaseLayerFlag(m_avcBaseLayerFlag);
#endif
#else
  vps->setAvcBaseLayerFlag(false);
#endif
  vps->setSplittingFlag(false);
  for(i = 0; i < MAX_VPS_NUM_SCALABILITY_TYPES; i++)
  {
    vps->setScalabilityMask(i, false);
  }
  if(m_numLayers > 1)
  {
    Int scalabilityTypes = 0;
    for(i = 0; i < MAX_VPS_NUM_SCALABILITY_TYPES; i++)
    {
      vps->setScalabilityMask(i, m_scalabilityMask[i]);
      scalabilityTypes += m_scalabilityMask[i];
    }
#if AUXILIARY_PICTURES
    assert( scalabilityTypes <= 2 );
#else
    assert( scalabilityTypes == 1 );
#endif
    vps->setNumScalabilityTypes(scalabilityTypes);
  }
  else
  {
    vps->setNumScalabilityTypes(0);
  }
  while((1 << dimIdLen) < m_numLayers)
  {
    dimIdLen++;
  }
  vps->setDimensionIdLen(0, dimIdLen);
  vps->setNuhLayerIdPresentFlag(false);
  vps->setLayerIdInNuh(0, 0);
  vps->setLayerIdInVps(0, 0);
  for(i = 1; i < vps->getMaxLayers(); i++)
  {
    vps->setLayerIdInNuh(i, i);
    vps->setLayerIdInVps(vps->getLayerIdInNuh(i), i);
    vps->setDimensionId(i, 0, i);
  }
#if AUXILIARY_PICTURES
  if (m_scalabilityMask[3])
  {
    UInt maxAuxId = 0;
    UInt auxDimIdLen = 1;
    for(i = 1; i < vps->getMaxLayers(); i++)
    {
      if (m_acLayerCfg[i].getAuxId() > maxAuxId)
      {
        maxAuxId = m_acLayerCfg[i].getAuxId();
      }
    }
    while((1 << auxDimIdLen) < (maxAuxId + 1))
    {
      auxDimIdLen++;
    }
    vps->setDimensionIdLen(1, auxDimIdLen);
    for(i = 1; i < vps->getMaxLayers(); i++)
    {
      vps->setDimensionId(i, 1, m_acLayerCfg[i].getAuxId());
    }
  }
#endif
#endif
#if VPS_TSLAYERS
  vps->setMaxTSLayersPresentFlag(true);

  for( i = 0; i < vps->getMaxLayers(); i++ )
  {
    vps->setMaxTSLayersMinus1(i, vps->getMaxTLayers()-1);
  }
#endif
  vps->setMaxTidRefPresentFlag(m_maxTidRefPresentFlag);
  if (vps->getMaxTidRefPresentFlag())
  {
    for( i = 0; i < vps->getMaxLayers() - 1; i++ )
    {
#if O0225_MAX_TID_FOR_REF_LAYERS
      for( Int j = i+1; j < vps->getMaxLayers(); j++)
      {
        vps->setMaxTidIlRefPicsPlus1(i, j, m_acTEncTop[i].getMaxTidIlRefPicsPlus1());
      }
#else
      vps->setMaxTidIlRefPicsPlus1(i, m_acTEncTop[i].getMaxTidIlRefPicsPlus1());
#endif 
    }
  }
  else
  {
    for( i = 0; i < vps->getMaxLayers() - 1; i++ )
    {
#if O0225_MAX_TID_FOR_REF_LAYERS
      for( Int j = i+1; j < vps->getMaxLayers(); j++)
      {
        vps->setMaxTidIlRefPicsPlus1(i, j, 7);
      }
#else
      vps->setMaxTidIlRefPicsPlus1(i, 7);
#endif 
    }
  }
    vps->setIlpSshSignalingEnabledFlag(false);
#if VPS_EXTN_PROFILE_INFO

#if LIST_OF_PTL
  vps->getPTLForExtnPtr()->resize(1);   // Dummy object - unused.
  for(i = 0; i < vps->getMaxLayers(); i++)
  {
    // TODO: The profile tier level have to be given support to be included in the configuration files
    if(i == 0)
    {
      if( vps->getBaseLayerInternalFlag() && vps->getMaxLayers() > 1 )
      {
        vps->setProfilePresentFlag(1, false);
        vps->getPTLForExtnPtr()->push_back( *(m_acTEncTop[0].getSPS()->getPTL()) );
      }
    }
    else  // i > 0
    {
      vps->setProfilePresentFlag(i, true);
      // Note - may need to be changed for other layer structures.
      vps->getPTLForExtnPtr()->push_back( *(m_acTEncTop[0].getSPS()->getPTL()) );
    }
  }
#else
  vps->getPTLForExtnPtr()->resize(vps->getNumLayerSets());
  for(Int setId = 1; setId < vps->getNumLayerSets(); setId++)
  {
    vps->setProfilePresentFlag(setId, true);
    // Note - may need to be changed for other layer structures.
    *(vps->getPTLForExtn(setId)) = *(m_acTEncTop[setId].getSPS()->getPTL());
  }
#endif
#endif
#if VPS_EXTN_DIRECT_REF_LAYERS
  // Direct reference layers
  UInt maxDirectRefLayers = 0;
#if O0096_DEFAULT_DEPENDENCY_TYPE
  Bool isDefaultDirectDependencyTypeSet = false;
#endif
  for (UInt layerCtr = 1; layerCtr <= vps->getMaxLayers() - 1; layerCtr++)
  {
    vps->setNumDirectRefLayers(layerCtr, m_acTEncTop[layerCtr].getNumDirectRefLayers());
    maxDirectRefLayers = max<UInt>(maxDirectRefLayers, vps->getNumDirectRefLayers(layerCtr));

    for (i = 0; i < vps->getNumDirectRefLayers(layerCtr); i++)
    {
      vps->setRefLayerId(layerCtr, i, m_acTEncTop[layerCtr].getRefLayerId(i));
    }
    // Set direct dependency flag
    // Initialize flag to 0
    for (Int refLayerCtr = 0; refLayerCtr < layerCtr; refLayerCtr++)
    {
      vps->setDirectDependencyFlag(layerCtr, refLayerCtr, false);
    }
    for (i = 0; i < vps->getNumDirectRefLayers(layerCtr); i++)
    {
      vps->setDirectDependencyFlag(layerCtr, vps->getLayerIdInVps(m_acTEncTop[layerCtr].getRefLayerId(i)), true);
    }
    // prediction indications
    vps->setDirectDepTypeLen(2); // sample and motion types are encoded
    for (Int refLayerCtr = 0; refLayerCtr < layerCtr; refLayerCtr++)
    {
      if (vps->getDirectDependencyFlag(layerCtr, refLayerCtr))
      {
        assert(m_acTEncTop[layerCtr].getSamplePredEnabledFlag(refLayerCtr) || m_acTEncTop[layerCtr].getMotionPredEnabledFlag(refLayerCtr));
        vps->setDirectDependencyType(layerCtr, refLayerCtr, ((m_acTEncTop[layerCtr].getSamplePredEnabledFlag(refLayerCtr) ? 1 : 0) |
          (m_acTEncTop[layerCtr].getMotionPredEnabledFlag(refLayerCtr) ? 2 : 0)) - 1);
#if O0096_DEFAULT_DEPENDENCY_TYPE
        if (!isDefaultDirectDependencyTypeSet)
        {
          vps->setDefaultDirectDependecyTypeFlag(1);
          vps->setDefaultDirectDependecyType(vps->getDirectDependencyType(layerCtr, refLayerCtr));
          isDefaultDirectDependencyTypeSet = true;
        }
        else if (vps->getDirectDependencyType(layerCtr, refLayerCtr) != vps->getDefaultDirectDependencyType())
        {
          vps->setDefaultDirectDependecyTypeFlag(0);
        }
#endif
      }
      else
      {
        vps->setDirectDependencyType(layerCtr, refLayerCtr, 0);
      }
    }
  }

#if O0092_0094_DEPENDENCY_CONSTRAINT
  vps->setNumRefLayers();

  if (vps->getMaxLayers() > MAX_REF_LAYERS)
  {
    for (UInt layerCtr = 1; layerCtr <= vps->getMaxLayers() - 1; layerCtr++)
    {
      assert(vps->getNumRefLayers(vps->getLayerIdInNuh(layerCtr)) <= MAX_REF_LAYERS);
    }
  }
#endif
#if Q0078_ADD_LAYER_SETS
  vps->setPredictedLayerIds();
  vps->setTreePartitionLayerIdList();
  vps->setLayerIdIncludedFlagsForAddLayerSets();
#endif
#endif
#if OUTPUT_LAYER_SETS_CONFIG

  vps->setDefaultTargetOutputLayerIdc( m_defaultTargetOutputLayerIdc ); // As per configuration file

  if( m_numOutputLayerSets == -1 )  // # of output layer sets not specified in the configuration file
  {
    vps->setNumOutputLayerSets(vps->getNumLayerSets());

    for(i = 1; i < vps->getNumLayerSets(); i++)
    {
        vps->setOutputLayerSetIdx(i, i);
    }
  }
  else
  {
    vps->setNumOutputLayerSets( m_numOutputLayerSets );
    for( Int olsCtr = 0; olsCtr < vps->getNumLayerSets(); olsCtr ++ ) // Default output layer sets
    {
      vps->setOutputLayerSetIdx(olsCtr, olsCtr);
    }
    for( Int olsCtr = vps->getNumLayerSets(); olsCtr < vps->getNumOutputLayerSets(); olsCtr ++ )  // Non-default output layer sets
    {
      vps->setOutputLayerSetIdx(olsCtr, m_outputLayerSetIdx[olsCtr - vps->getNumLayerSets()]);
    }
  }
#endif
  // Target output layer
#if LIST_OF_PTL
  vps->setNumProfileTierLevel( vps->getPTLForExtnPtr()->size() ); // +1 for the base VPS PTL()
#else
  vps->setNumOutputLayerSets(vps->getNumLayerSets());
  vps->setNumProfileTierLevel(vps->getNumLayerSets());
#endif
#if !OUTPUT_LAYER_SETS_CONFIG // Taken care by configuration file parameter
#if P0295_DEFAULT_OUT_LAYER_IDC
  vps->setDefaultTargetOutputLayerIdc(1);
#else
#if O0109_DEFAULT_ONE_OUT_LAYER_IDC
  vps->setDefaultOneTargetOutputLayerIdc(1);
#else
  vps->setDefaultOneTargetOutputLayerFlag(true);
#endif
#endif
#endif
#if !PER_LAYER_PTL
  for(i = 1; i < vps->getNumLayerSets(); i++)
  {
    vps->setProfileLevelTierIdx(i, i);
#if !OUTPUT_LAYER_SETS_CONFIG
    vps->setOutputLayerSetIdx(i, i);
#endif
  }  
#endif
#endif
 #if VPS_DPB_SIZE_TABLE
  // The Layer ID List variables can be derived here.  
#if DERIVE_LAYER_ID_LIST_VARIABLES
  vps->deriveLayerIdListVariables();
#endif
#if RESOLUTION_BASED_DPB
  vps->assignSubDpbIndices();
#else
  vps->deriveNumberOfSubDpbs();
#endif

  // derive OutputLayerFlag[i][j] 
#if !OUTPUT_LAYER_SETS_CONFIG
  if( vps->getDefaultTargetOutputLayerIdc() == 1 )
#endif
  {
    // default_output_layer_idc equal to 1 specifies that only the layer with the highest value of nuh_layer_id such that nuh_layer_id equal to nuhLayerIdA and 
    // AuxId[ nuhLayerIdA ] equal to 0 in each of the output layer sets with index in the range of 1 to vps_num_layer_sets_minus1, inclusive, is an output layer of its output layer set.

    // Include the highest layer as output layer for each layer set
    for(Int lsIdx = 1; lsIdx < vps->getNumLayerSets(); lsIdx++)
    {
      for( UInt layer = 0; layer < vps->getNumLayersInIdList(lsIdx); layer++ )
      {
#if !Q0078_ADD_LAYER_SETS  // the following condition is incorrect and is not needed anyway
        if( vps->getLayerIdIncludedFlag(lsIdx, layer) )      
#endif
        {
#if OUTPUT_LAYER_SETS_CONFIG
          switch(vps->getDefaultTargetOutputLayerIdc())
          {
            case 0: vps->setOutputLayerFlag( lsIdx, layer, 1 );
              break;
            case 1: vps->setOutputLayerFlag( lsIdx, layer, layer == vps->getNumLayersInIdList(lsIdx) - 1 );
              break;
            case 2:
            case 3: vps->setOutputLayerFlag( lsIdx, layer, std::find( m_listOfOutputLayers[lsIdx].begin(), m_listOfOutputLayers[lsIdx].end(), layer) != m_listOfOutputLayers[lsIdx].end() );
              break;
          }
#else
          vps->setOutputLayerFlag( lsIdx, layer, layer == vps->getNumLayersInIdList(lsIdx) - 1 );
#endif
        }
      }
    }
#if OUTPUT_LAYER_SETS_CONFIG
    for( Int olsIdx = vps->getNumLayerSets(); olsIdx < vps->getNumOutputLayerSets(); olsIdx++ )
    {
      for( UInt layer = 0; layer < vps->getNumLayersInIdList(vps->getOutputLayerSetIdx(olsIdx)); layer++ )
      {
        vps->setOutputLayerFlag( olsIdx, layer, std::find( m_listOfOutputLayers[olsIdx].begin(), m_listOfOutputLayers[olsIdx].end(), layer) != m_listOfOutputLayers[olsIdx].end());
      }
    }
#endif
  }
#if !OUTPUT_LAYER_SETS_CONFIG
  else
  {
    // cases when default_output_layer_idc is not equal to 1
    assert(!"default_output_layer_idc not equal to 1 is not yet supported");
  }
#endif
#if NECESSARY_LAYER_FLAG
  vps->deriveNecessaryLayerFlag();
  vps->checkNecessaryLayerFlagCondition();
#endif
#if PER_LAYER_PTL
  vps->getProfileLevelTierIdx()->resize(vps->getNumOutputLayerSets());
  vps->getProfileLevelTierIdx(0)->push_back( vps->getBaseLayerInternalFlag() && vps->getMaxLayers() > 1 ? 1 : 0 ); // Default 0-th output layer set
  for(i = 1; i < vps->getNumOutputLayerSets(); i++)
  {
    Int layerSetIdxForOutputLayerSet = vps->getOutputLayerSetIdx( i );
    Int numLayerInLayerSet = vps->getNumLayersInIdList( layerSetIdxForOutputLayerSet );
    for(Int j = 0; j < numLayerInLayerSet; j++)
    {
      Int layerIdxInVps = vps->getLayerIdInVps( vps->getLayerSetLayerIdList(layerSetIdxForOutputLayerSet, j) );
      if( vps->getNecessaryLayerFlag(i, j) )
      {
        vps->getProfileLevelTierIdx(i)->push_back( vps->getBaseLayerInternalFlag() && vps->getMaxLayers() > 1 ? layerIdxInVps + 1 : layerIdxInVps);
      }
      else
      {
        vps->getProfileLevelTierIdx(i)->push_back( -1 );
      }
    }
  }
#endif
#if SUB_LAYERS_IN_LAYER_SET
  vps->calculateMaxSLInLayerSets();
#endif
  // Initialize dpb_size_table() for all ouput layer sets in the VPS extension
  for(i = 1; i < vps->getNumOutputLayerSets(); i++)
  {
#if CHANGE_NUMSUBDPB_IDX
    Int layerSetIdxForOutputLayerSet = vps->getOutputLayerSetIdx( i );
#endif
    Int layerSetId = vps->getOutputLayerSetIdx(i);

    for(Int j = 0; j < vps->getMaxTLayers(); j++)
    {

      Int maxNumReorderPics = -1;
#if CHANGE_NUMSUBDPB_IDX
#if RESOLUTION_BASED_DPB
      for(Int k = 0; k < vps->getNumLayersInIdList(layerSetIdxForOutputLayerSet); k++)
#else
      for(Int k = 0; k < vps->getNumSubDpbs(layerSetIdxForOutputLayerSet); k++)
#endif
#else
      for(Int k = 0; k < vps->getNumSubDpbs(i); k++)
#endif
      {
        Int layerId = vps->getLayerSetLayerIdList(layerSetId, k); // k-th layer in the output layer set
#if RESOLUTION_BASED_DPB
        vps->setMaxVpsLayerDecPicBuffMinus1( i, k, j, m_acTEncTop[layerId].getMaxDecPicBuffering(j) - 1 );
        // Add sub-DPB sizes of layers belonging to a sub-DPB. If a different sub-DPB size is calculated
        // at the encoder, modify below
        Int oldValue = vps->getMaxVpsDecPicBufferingMinus1( i, vps->getSubDpbAssigned( layerSetIdxForOutputLayerSet, k ), j );
        oldValue += vps->getMaxVpsLayerDecPicBuffMinus1( i, k, j ) + 1;
        vps->setMaxVpsDecPicBufferingMinus1( i, vps->getSubDpbAssigned( layerSetIdxForOutputLayerSet, k ), j, oldValue );
#else
        vps->setMaxVpsDecPicBufferingMinus1( i, k, j,  m_acTEncTop[layerId].getMaxDecPicBuffering(j) - 1 );
#endif
        maxNumReorderPics       = std::max( maxNumReorderPics, m_acTEncTop[layerId].getNumReorderPics(j));
      }
#if RESOLUTION_BASED_DPB
      for(Int k = 0; k < vps->getNumSubDpbs(i); k++)
      {
        // Decrement m_maxVpsDecPicBufferingMinus1
        Int oldValue = vps->getMaxVpsDecPicBufferingMinus1( i, vps->getSubDpbAssigned( layerSetIdxForOutputLayerSet, k ), j );
        vps->setMaxVpsDecPicBufferingMinus1( i, vps->getSubDpbAssigned( layerSetIdxForOutputLayerSet, k ), j, oldValue - 1 );
      }
#endif
      vps->setMaxVpsNumReorderPics(i, j, maxNumReorderPics);
      vps->determineSubDpbInfoFlags();
    }
  }
#endif
    vps->setMaxOneActiveRefLayerFlag(maxDirectRefLayers > 1 ? false : true);
#if O0062_POC_LSB_NOT_PRESENT_FLAG
    for(i = 1; i< vps->getMaxLayers(); i++)
    {
      if( vps->getNumDirectRefLayers( vps->getLayerIdInNuh(i) ) == 0  )
      {
#if Q0078_ADD_LAYER_SETS
        vps->setPocLsbNotPresentFlag(i, true); // make independedent layers base-layer compliant
#else
        vps->setPocLsbNotPresentFlag(i, false);
#endif
      }
    }
#endif
#if O0223_PICTURE_TYPES_ALIGN_FLAG
    vps->setCrossLayerPictureTypeAlignFlag( m_crossLayerPictureTypeAlignFlag );
#endif
#if P0068_CROSS_LAYER_ALIGNED_IDR_ONLY_FOR_IRAP_FLAG
    vps->setCrossLayerAlignedIdrOnlyFlag( m_crossLayerAlignedIdrOnlyFlag );
#endif
    vps->setCrossLayerIrapAlignFlag( m_crossLayerIrapAlignFlag );
    for(UInt layerCtr = 1;layerCtr <= vps->getMaxLayers() - 1; layerCtr++)
    {
      for(Int refLayerCtr = 0; refLayerCtr < layerCtr; refLayerCtr++)
      {
        if (vps->getDirectDependencyFlag( layerCtr, refLayerCtr))
        {
          if(m_acTEncTop[layerCtr].getIntraPeriod() !=  m_acTEncTop[refLayerCtr].getIntraPeriod())
          {
            vps->setCrossLayerIrapAlignFlag(false);
            break;
          }
        }
      }
    }
#if M0040_ADAPTIVE_RESOLUTION_CHANGE
  vps->setSingleLayerForNonIrapFlag(m_adaptiveResolutionChange > 0 ? true : false);
#endif 
#if HIGHER_LAYER_IRAP_SKIP_FLAG
  vps->setHigherLayerIrapSkipFlag(m_skipPictureAtArcSwitch);
#endif
#if !P0125_REVERT_VPS_EXTN_OFFSET_TO_RESERVED
#if !VPS_EXTN_OFFSET_CALC
#if VPS_EXTN_OFFSET
  // to be updated according to the current semantics
  vps->setExtensionOffset( 0xffff );
#endif
#endif
#endif

#if O0215_PHASE_ALIGNMENT
  vps->setPhaseAlignFlag( m_phaseAlignFlag );
#endif

#if P0300_ALT_OUTPUT_LAYER_FLAG
  for (Int k = 0; k < MAX_VPS_LAYER_SETS_PLUS1; k++)
  {
    vps->setAltOuputLayerFlag( k, m_altOutputLayerFlag );
  }
#else
#if O0153_ALT_OUTPUT_LAYER_FLAG
  vps->setAltOuputLayerFlag( m_altOutputLayerFlag );
#endif
#endif

#if P0312_VERT_PHASE_ADJ
  Bool vpsVuiVertPhaseInUseFlag = false;
  for( UInt layerId = 1; layerId < m_numLayers; layerId++ )
  {
    for( i = 0; i < m_acLayerCfg[layerId].m_numScaledRefLayerOffsets; i++ )
    {
      if( m_acTEncTop[layerId].getVertPhasePositionEnableFlag(i) )
      {
        vpsVuiVertPhaseInUseFlag = true;
        break;
      }
    }
  }
  vps->setVpsVuiVertPhaseInUseFlag( vpsVuiVertPhaseInUseFlag );
#endif

#if VPS_VUI_BSP_HRD_PARAMS
  vps->setVpsVuiBspHrdPresentFlag(false);
  TEncTop *pcCfg = &m_acTEncTop[0];
  if( pcCfg->getBufferingPeriodSEIEnabled() )
  {
    Int j;
    vps->setVpsVuiBspHrdPresentFlag(true);
    vps->setVpsNumAddHrdParams( vps->getMaxLayers() );
    vps->createBspHrdParamBuffer(vps->getVpsNumAddHrdParams() + 1);
    for( i = vps->getNumHrdParameters(), j = 0; i < vps->getNumHrdParameters() + vps->getVpsNumAddHrdParams(); i++, j++ )
    {
      vps->setCprmsAddPresentFlag( j, true );
      vps->setNumSubLayerHrdMinus1( j, vps->getMaxTLayers() - 1 );

      UInt layerId = j;
      TEncTop *pcCfgLayer = &m_acTEncTop[layerId];

      Int iPicWidth         = pcCfgLayer->getSourceWidth();
      Int iPicHeight        = pcCfgLayer->getSourceHeight();
#if LAYER_CTB
      UInt uiWidthInCU       = ( iPicWidth  % m_acLayerCfg[layerId].m_uiMaxCUWidth  ) ? iPicWidth  / m_acLayerCfg[layerId].m_uiMaxCUWidth  + 1 : iPicWidth  / m_acLayerCfg[layerId].m_uiMaxCUWidth;
      UInt uiHeightInCU      = ( iPicHeight % m_acLayerCfg[layerId].m_uiMaxCUHeight ) ? iPicHeight / m_acLayerCfg[layerId].m_uiMaxCUHeight + 1 : iPicHeight / m_acLayerCfg[layerId].m_uiMaxCUHeight;
      UInt maxCU = pcCfgLayer->getSliceArgument() >> ( m_acLayerCfg[layerId].m_uiMaxCUDepth << 1);
#else
      UInt uiWidthInCU       = ( iPicWidth %m_uiMaxCUWidth  ) ? iPicWidth /m_uiMaxCUWidth  + 1 : iPicWidth /m_uiMaxCUWidth;
      UInt uiHeightInCU      = ( iPicHeight%m_uiMaxCUHeight ) ? iPicHeight/m_uiMaxCUHeight + 1 : iPicHeight/m_uiMaxCUHeight;
      UInt maxCU = pcCfgLayer->getSliceArgument() >> ( m_uiMaxCUDepth << 1);
#endif
      UInt uiNumCUsInFrame   = uiWidthInCU * uiHeightInCU;

      UInt numDU = ( pcCfgLayer->getSliceMode() == 1 ) ? ( uiNumCUsInFrame / maxCU ) : ( 0 );
      if( uiNumCUsInFrame % maxCU != 0 || numDU == 0 )
      {
        numDU ++;
      }
      vps->getBspHrd(i)->setNumDU( numDU );
      vps->setBspHrdParameters( i, pcCfgLayer->getFrameRate(), numDU, pcCfgLayer->getTargetBitrate(), ( pcCfgLayer->getIntraPeriod() > 0 ) );
    }

    // Signalling of additional partitioning schemes
    for(Int h = 1; h < vps->getNumOutputLayerSets(); h++)
    {
      Int lsIdx = vps->getOutputLayerSetIdx( h );
      vps->setNumSignalledPartitioningSchemes(h, 1);  // Only the default per-layer partitioning scheme
      for( j = 1; j < vps->getNumSignalledPartitioningSchemes(h); j++ )
      {
        // ToDo: Add code for additional partitioning schemes here
        // ToDo: Initialize num_partitions_in_scheme_minus1 and layer_included_in_partition_flag
      }

      for( i = 0; i < vps->getNumSignalledPartitioningSchemes(h); i++ )
      {
        if( i == 0 )
        {
          for(Int t = 0; t <= vps->getMaxSLayersInLayerSetMinus1( lsIdx ); t++)
          {
            vps->setNumBspSchedulesMinus1( h, i, t, 0 );
            for( j = 0; j <= vps->getNumBspSchedulesMinus1(h, i, t); j++ )
            {
              for( Int k = 0; k <= vps->getNumPartitionsInSchemeMinus1(h, i); k++ )
              {
                // Only for the default partition
                Int nuhlayerId = vps->getLayerSetLayerIdList( lsIdx, k);
                Int layerIdxInVps = vps->getLayerIdInVps( nuhlayerId );
                vps->setBspHrdIdx(h, i, t, j, k, layerIdxInVps + vps->getNumHrdParameters());

                vps->setBspSchedIdx(h, i, t, j, k, 0);
              }
            }
          }
        }
        else
        {
          assert(0);    // Need to add support for additional partitioning schemes.
        }
      }
    }
  }
#else

#if O0164_MULTI_LAYER_HRD
  vps->setVpsVuiBspHrdPresentFlag(false);
  TEncTop *pcCfg = &m_acTEncTop[0];
  if( pcCfg->getBufferingPeriodSEIEnabled() )
  {
    vps->setVpsVuiBspHrdPresentFlag(true);
#if Q0078_ADD_LAYER_SETS
    vps->setVpsNumBspHrdParametersMinus1(vps->getVpsNumLayerSetsMinus1() - 1); 
#else
    vps->setVpsNumBspHrdParametersMinus1(vps->getNumLayerSets() - 2); 
#endif
    vps->createBspHrdParamBuffer(vps->getVpsNumBspHrdParametersMinus1() + 1);
    for ( i = 0; i <= vps->getVpsNumBspHrdParametersMinus1(); i++ )
    {
      vps->setBspCprmsPresentFlag(i, true);

      UInt layerId = i + 1;
      TEncTop *pcCfgLayer = &m_acTEncTop[layerId];

      Int iPicWidth         = pcCfgLayer->getSourceWidth();
      Int iPicHeight        = pcCfgLayer->getSourceHeight();
#if LAYER_CTB
      UInt uiWidthInCU       = ( iPicWidth  % m_acLayerCfg[layerId].m_uiMaxCUWidth  ) ? iPicWidth  / m_acLayerCfg[layerId].m_uiMaxCUWidth  + 1 : iPicWidth  / m_acLayerCfg[layerId].m_uiMaxCUWidth;
      UInt uiHeightInCU      = ( iPicHeight % m_acLayerCfg[layerId].m_uiMaxCUHeight ) ? iPicHeight / m_acLayerCfg[layerId].m_uiMaxCUHeight + 1 : iPicHeight / m_acLayerCfg[layerId].m_uiMaxCUHeight;
#else
      UInt uiWidthInCU       = ( iPicWidth %m_uiMaxCUWidth  ) ? iPicWidth /m_uiMaxCUWidth  + 1 : iPicWidth /m_uiMaxCUWidth;
      UInt uiHeightInCU      = ( iPicHeight%m_uiMaxCUHeight ) ? iPicHeight/m_uiMaxCUHeight + 1 : iPicHeight/m_uiMaxCUHeight;
#endif
      UInt uiNumCUsInFrame   = uiWidthInCU * uiHeightInCU;

#if LAYER_CTB
      UInt maxCU = pcCfgLayer->getSliceArgument() >> ( m_acLayerCfg[layerId].m_uiMaxCUDepth << 1);
#else
      UInt maxCU = pcCfgLayer->getSliceArgument() >> ( m_uiMaxCUDepth << 1);
#endif
      UInt numDU = ( pcCfgLayer->getSliceMode() == 1 ) ? ( uiNumCUsInFrame / maxCU ) : ( 0 );
      if( uiNumCUsInFrame % maxCU != 0 || numDU == 0 )
      {
        numDU ++;
      }
      vps->getBspHrd(i)->setNumDU( numDU );
      vps->setBspHrdParameters( i, pcCfgLayer->getFrameRate(), numDU, pcCfgLayer->getTargetBitrate(), ( pcCfgLayer->getIntraPeriod() > 0 ) );
    }
#if Q0078_ADD_LAYER_SETS
    for (UInt h = 1; h <= vps->getVpsNumLayerSetsMinus1(); h++)
#else
    for(UInt h = 1; h <= (vps->getNumLayerSets()-1); h++)
#endif
    {
      vps->setNumBitstreamPartitions(h, 1);
      for( i = 0; i < vps->getNumBitstreamPartitions(h); i++ )
      {
        for( UInt j = 0; j <= (vps->getMaxLayers()-1); j++ )
        {
          if (vps->getLayerIdIncludedFlag(h, j) && h == j)
          {
            vps->setLayerInBspFlag(h, i, j, true);
          }
        }
      }
      vps->setNumBspSchedCombinations(h, 1);
      for( i = 0; i < vps->getNumBspSchedCombinations(h); i++ )
      {
        for( UInt j = 0; j < vps->getNumBitstreamPartitions(h); j++ )
        {
          vps->setBspCombHrdIdx(h, i, j, 0);
          vps->setBspCombSchedIdx(h, i, j, 0);
        }
      }
    }
  }
#endif
#endif

#else //SVC_EXTENSION
  m_cTEncTop.init(isFieldCoding);
#endif //SVC_EXTENSION
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 - create internal class
 - initialize internal variable
 - until the end of input YUV file, call encoding function in TEncTop class
 - delete allocated buffers
 - destroy internal class
 .
 */
#if SVC_EXTENSION
Void TAppEncTop::encode()
{
  fstream bitstreamFile(m_pBitstreamFile, fstream::binary | fstream::out);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for writing\n", m_pBitstreamFile);
    exit(EXIT_FAILURE);
  }

  TComPicYuv*       pcPicYuvOrg [MAX_LAYERS];
  TComPicYuv*       pcPicYuvRec = NULL;

  // initialize internal class & member variables
  xInitLibCfg();
  xCreateLib();
  xInitLib(m_isField);

  // main encoder loop
  Int   iNumEncoded = 0, iTotalNumEncoded = 0;
  Bool  bEos = false;

  list<AccessUnit> outputAccessUnits; ///< list of access units to write out.  is populated by the encoding process

  for(UInt layer=0; layer<m_numLayers; layer++)
  {
#if O0194_DIFFERENT_BITDEPTH_EL_BL
    //5
    g_bitDepthY = m_acLayerCfg[layer].m_internalBitDepthY;
    g_bitDepthC = m_acLayerCfg[layer].m_internalBitDepthC;

    g_uiPCMBitDepthLuma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthY : m_acLayerCfg[layer].m_internalBitDepthY;
    g_uiPCMBitDepthChroma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthC : m_acLayerCfg[layer].m_internalBitDepthC;
#endif
    // allocate original YUV buffer
    pcPicYuvOrg[layer] = new TComPicYuv;
    if( m_isField )
    {
#if SVC_EXTENSION
#if LAYER_CTB
#if AUXILIARY_PICTURES
      pcPicYuvOrg[layer]->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeightOrg(), m_acLayerCfg[layer].getChromaFormatIDC(), m_acLayerCfg[layer].m_uiMaxCUWidth, m_acLayerCfg[layer].m_uiMaxCUHeight, m_acLayerCfg[layer].m_uiMaxCUDepth, NULL );
#else
      pcPicYuvOrg[layer]->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeightOrg(), m_acLayerCfg[layer].m_uiMaxCUWidth, m_acLayerCfg[layer].m_uiMaxCUHeight, m_acLayerCfg[layer].m_uiMaxCUDepth, NULL );
#endif
#else
#if AUXILIARY_PICTURES
      pcPicYuvOrg[layer]->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeightOrg(), m_acLayerCfg[layer].getChromaFormatIDC(), m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth, NULL );
#else
      pcPicYuvOrg[layer]->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeightOrg(), m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth, NULL );
#endif
#endif
#else
      pcPicYuvOrg->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeightOrg(), m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
#endif
    }
    else
    {
#if SVC_EXTENSION
#if LAYER_CTB
#if AUXILIARY_PICTURES
      pcPicYuvOrg[layer]->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeight(), m_acLayerCfg[layer].getChromaFormatIDC(), m_acLayerCfg[layer].m_uiMaxCUWidth, m_acLayerCfg[layer].m_uiMaxCUHeight, m_acLayerCfg[layer].m_uiMaxCUDepth, NULL );
#else
      pcPicYuvOrg[layer]->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeight(), m_acLayerCfg[layer].m_uiMaxCUWidth, m_acLayerCfg[layer].m_uiMaxCUHeight, m_acLayerCfg[layer].m_uiMaxCUDepth, NULL );
#endif
#else
#if AUXILIARY_PICTURES
      pcPicYuvOrg[layer]->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeight(), m_acLayerCfg[layer].getChromaFormatIDC(), m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth, NULL );
#else
      pcPicYuvOrg[layer]->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeight(), m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth, NULL );
#endif
#endif
#else
      pcPicYuvOrg->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeight(), m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
#endif
    }
  }

  Bool bFirstFrame = true;
  while ( !bEos )
  {
    // Read enough frames
    Bool bFramesReadyToCode = false;
    while(!bFramesReadyToCode)
    {
      for(UInt layer=0; layer<m_numLayers; layer++)
      {
#if O0194_DIFFERENT_BITDEPTH_EL_BL
        //6
        g_bitDepthY = m_acLayerCfg[layer].m_internalBitDepthY;
        g_bitDepthC = m_acLayerCfg[layer].m_internalBitDepthC;

        g_uiPCMBitDepthLuma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthY : m_acLayerCfg[layer].m_internalBitDepthY;
        g_uiPCMBitDepthChroma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthC : m_acLayerCfg[layer].m_internalBitDepthC;
#endif
#if LAYER_CTB
        g_uiMaxCUWidth  = g_auiLayerMaxCUWidth[layer];
        g_uiMaxCUHeight = g_auiLayerMaxCUHeight[layer];
        g_uiMaxCUDepth  = g_auiLayerMaxCUDepth[layer];
        g_uiAddCUDepth  = g_auiLayerAddCUDepth[layer];
#endif

        // get buffers
        xGetBuffer(pcPicYuvRec, layer);

        // read input YUV file
        m_acTVideoIOYuvInputFile[layer].read( pcPicYuvOrg[layer], m_acLayerCfg[layer].getPad() );

#if AUXILIARY_PICTURES
        if (m_acLayerCfg[layer].getChromaFormatIDC() == CHROMA_400)
        {
          pcPicYuvOrg[layer]->convertToMonochrome();
        }
#endif

        if(layer == m_numLayers-1)
        {
          // increase number of received frames
          m_iFrameRcvd++;
          // check end of file
          bEos = (m_isField && (m_iFrameRcvd == (m_framesToBeEncoded >> 1) )) || ( !m_isField && (m_iFrameRcvd == m_framesToBeEncoded) );
        }

        if ( m_isField )
        {
          m_acTEncTop[layer].encodePrep( pcPicYuvOrg[layer], m_isTopFieldFirst );
        }
        else
        {
          m_acTEncTop[layer].encodePrep( pcPicYuvOrg[layer] );
        }
      }

      bFramesReadyToCode = !(!bFirstFrame && ( m_acTEncTop[m_numLayers-1].getNumPicRcvd() != m_iGOPSize && m_iGOPSize ) && !bEos );
    }
    Bool flush = 0;
    // if end of file (which is only detected on a read failure) flush the encoder of any queued pictures
    if (m_acTVideoIOYuvInputFile[m_numLayers-1].isEof())
    {
      flush = true;
      bEos = true;
      m_iFrameRcvd--;
      m_acTEncTop[m_numLayers-1].setFramesToBeEncoded(m_iFrameRcvd);
    }

#if RC_SHVC_HARMONIZATION
    for(UInt layer=0; layer<m_numLayers; layer++)
    {
      if ( m_acTEncTop[layer].getUseRateCtrl() )
      {
        (m_acTEncTop[layer].getRateCtrl())->initRCGOP(m_acTEncTop[layer].getNumPicRcvd());
      }
    }
#endif

#if M0040_ADAPTIVE_RESOLUTION_CHANGE
    if (m_adaptiveResolutionChange)
    {
      for(UInt layer = 0; layer < m_numLayers; layer++)
      {
        TComList<TComPicYuv*>::iterator iterPicYuvRec;
        for (iterPicYuvRec = m_acListPicYuvRec[layer].begin(); iterPicYuvRec != m_acListPicYuvRec[layer].end(); iterPicYuvRec++)
        {
          TComPicYuv* recPic = *(iterPicYuvRec);
          recPic->setReconstructed(false);
        }
      }
    }
#endif

    // loop through frames in one GOP
    for ( UInt iPicIdInGOP=0; iPicIdInGOP < (bFirstFrame? 1:m_iGOPSize); iPicIdInGOP++ )
    {
      // layer by layer for each frame
      for(UInt layer=0; layer<m_numLayers; layer++)
      {
#if O0194_DIFFERENT_BITDEPTH_EL_BL
        //7
        g_bitDepthY = m_acLayerCfg[layer].m_internalBitDepthY;
        g_bitDepthC = m_acLayerCfg[layer].m_internalBitDepthC;

        g_uiPCMBitDepthLuma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthY : m_acLayerCfg[layer].m_internalBitDepthY;
        g_uiPCMBitDepthChroma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthC : m_acLayerCfg[layer].m_internalBitDepthC;
#endif
#if LAYER_CTB
        g_uiMaxCUWidth  = g_auiLayerMaxCUWidth[layer];
        g_uiMaxCUHeight = g_auiLayerMaxCUHeight[layer];
        g_uiMaxCUDepth  = g_auiLayerMaxCUDepth[layer];
        g_uiAddCUDepth  = g_auiLayerAddCUDepth[layer];

        memcpy( g_auiZscanToRaster, g_auiLayerZscanToRaster[layer], sizeof( g_auiZscanToRaster ) );
        memcpy( g_auiRasterToZscan, g_auiLayerRasterToZscan[layer], sizeof( g_auiRasterToZscan ) );
        memcpy( g_auiRasterToPelX,  g_auiLayerRasterToPelX[layer],  sizeof( g_auiRasterToPelX ) );
        memcpy( g_auiRasterToPelY,  g_auiLayerRasterToPelY[layer],  sizeof( g_auiRasterToPelY ) );
#endif
        // call encoding function for one frame
        if ( m_isField )
        {
          m_acTEncTop[layer].encode( flush ? 0 : pcPicYuvOrg[layer], m_acListPicYuvRec[layer], outputAccessUnits, iPicIdInGOP, m_isTopFieldFirst );
        }
        else
        {
          m_acTEncTop[layer].encode( flush ? 0 : pcPicYuvOrg[layer], m_acListPicYuvRec[layer], outputAccessUnits, iPicIdInGOP );
        }
      }
    }
#if R0247_SEI_ACTIVE
    if(bFirstFrame)
    {
      list<AccessUnit>::iterator first_au = outputAccessUnits.begin();
      AccessUnit::iterator it_sps;
      for (it_sps = first_au->begin(); it_sps != first_au->end(); it_sps++)
      {
        if( (*it_sps)->m_nalUnitType == NAL_UNIT_SPS )
        {
          break;
        }
      }

      for (list<AccessUnit>::iterator it_au = ++outputAccessUnits.begin(); it_au != outputAccessUnits.end(); it_au++)
      {
        for (AccessUnit::iterator it_nalu = it_au->begin(); it_nalu != it_au->end(); it_nalu++)
        {
          if( (*it_nalu)->m_nalUnitType == NAL_UNIT_SPS )
          {
            first_au->insert(++it_sps, *it_nalu);
            it_nalu = it_au->erase(it_nalu);
          }
        }
      }
    }

#endif

#if RC_SHVC_HARMONIZATION
    for(UInt layer=0; layer<m_numLayers; layer++)
    {
      if ( m_acTEncTop[layer].getUseRateCtrl() )
      {
        (m_acTEncTop[layer].getRateCtrl())->destroyRCGOP();
      }
    }
#endif

    iTotalNumEncoded = 0;
    for(UInt layer=0; layer<m_numLayers; layer++)
    {
#if O0194_DIFFERENT_BITDEPTH_EL_BL
      //8
      g_bitDepthY = m_acLayerCfg[layer].m_internalBitDepthY;
      g_bitDepthC = m_acLayerCfg[layer].m_internalBitDepthC;

      g_uiPCMBitDepthLuma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthY : m_acLayerCfg[layer].m_internalBitDepthY;
      g_uiPCMBitDepthChroma = m_bPCMInputBitDepthFlag ? m_acLayerCfg[layer].m_inputBitDepthC : m_acLayerCfg[layer].m_internalBitDepthC;
#endif
      // write bistream to file if necessary
      iNumEncoded = m_acTEncTop[layer].getNumPicRcvd();
      if ( iNumEncoded > 0 )
      {
        xWriteRecon(layer, iNumEncoded);
        iTotalNumEncoded += iNumEncoded;
      }
      m_acTEncTop[layer].setNumPicRcvd( 0 );
    }

    // write bitstream out
    if(iTotalNumEncoded)
    {
#if P0130_EOB
      if( bEos )
      {
        OutputNALUnit nalu(NAL_UNIT_EOB);
        nalu.m_layerId = 0;

        AccessUnit& accessUnit = outputAccessUnits.back();
#if T_ID_EOB_BUG_FIX
        nalu.m_temporalId = 0;
#else
        nalu.m_temporalId = accessUnit.front()->m_temporalId;
#endif
        accessUnit.push_back(new NALUnitEBSP(nalu));
      }
#endif
      xWriteStream(bitstreamFile, iTotalNumEncoded, outputAccessUnits);
      outputAccessUnits.clear();
    }

    // print out summary
    if (bEos)
    {
      printOutSummary(m_isTopFieldFirst);
    }

    bFirstFrame = false;
  }
  // delete original YUV buffer
  for(UInt layer=0; layer<m_numLayers; layer++)
  {
    pcPicYuvOrg[layer]->destroy();
    delete pcPicYuvOrg[layer];
    pcPicYuvOrg[layer] = NULL;

    // delete used buffers in encoder class
    m_acTEncTop[layer].deletePicBuffer();
  }

  // delete buffers & classes
  xDeleteBuffer();
  xDestroyLib();

  printRateSummary();

  return;
}

Void TAppEncTop::printOutSummary(Bool isField)
{
  UInt layer;

  // set frame rate
  for(layer = 0; layer < m_numLayers; layer++)
  {
    if(isField)
    {
      m_gcAnalyzeAll[layer].setFrmRate( m_acLayerCfg[layer].getFrameRate() * 2);
      m_gcAnalyzeI[layer].setFrmRate( m_acLayerCfg[layer].getFrameRate() * 2 );
      m_gcAnalyzeP[layer].setFrmRate( m_acLayerCfg[layer].getFrameRate() * 2 );
      m_gcAnalyzeB[layer].setFrmRate( m_acLayerCfg[layer].getFrameRate() * 2 );
    }
    else
    {
      m_gcAnalyzeAll[layer].setFrmRate( m_acLayerCfg[layer].getFrameRate());
      m_gcAnalyzeI[layer].setFrmRate( m_acLayerCfg[layer].getFrameRate() );
      m_gcAnalyzeP[layer].setFrmRate( m_acLayerCfg[layer].getFrameRate() );
      m_gcAnalyzeB[layer].setFrmRate( m_acLayerCfg[layer].getFrameRate() );
    }
  }

  //-- all
  printf( "\n\nSUMMARY --------------------------------------------------------\n" );
  printf( "\tTotal Frames |  "   "Bitrate    "  "Y-PSNR    "  "U-PSNR    "  "V-PSNR \n" );
  for(layer = 0; layer < m_numLayers; layer++)
  {
    m_gcAnalyzeAll[layer].printOut('a', layer);
  }

  printf( "\n\nI Slices--------------------------------------------------------\n" );
  printf( "\tTotal Frames |  "   "Bitrate    "  "Y-PSNR    "  "U-PSNR    "  "V-PSNR \n" );
  for(layer = 0; layer < m_numLayers; layer++)
  {
    m_gcAnalyzeI[layer].printOut('i', layer);
  }

  printf( "\n\nP Slices--------------------------------------------------------\n" );
  printf( "\tTotal Frames |  "   "Bitrate    "  "Y-PSNR    "  "U-PSNR    "  "V-PSNR \n" );
  for(layer = 0; layer < m_numLayers; layer++)
  {
    m_gcAnalyzeP[layer].printOut('p', layer);
  }

  printf( "\n\nB Slices--------------------------------------------------------\n" );
  printf( "\tTotal Frames |  "   "Bitrate    "  "Y-PSNR    "  "U-PSNR    "  "V-PSNR \n" );
  for(layer = 0; layer < m_numLayers; layer++)
  {
    m_gcAnalyzeB[layer].printOut('b', layer);
  }

  if(isField)
  {
    for(layer = 0; layer < m_numLayers; layer++)
    {
      //-- interlaced summary
      m_gcAnalyzeAll_in.setFrmRate( m_acLayerCfg[layer].getFrameRate());
      printf( "\n\nSUMMARY INTERLACED ---------------------------------------------\n" );
      m_gcAnalyzeAll_in.printOutInterlaced('a',  m_gcAnalyzeAll[layer].getBits());

#if _SUMMARY_OUT_
      m_gcAnalyzeAll_in.printSummaryOutInterlaced();
#endif
    }
  }
}

#else
Void TAppEncTop::encode()
{
  fstream bitstreamFile(m_pchBitstreamFile, fstream::binary | fstream::out);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for writing\n", m_pchBitstreamFile);
    exit(EXIT_FAILURE);
  }

  TComPicYuv*       pcPicYuvOrg = new TComPicYuv;
  TComPicYuv*       pcPicYuvRec = NULL;

  // initialize internal class & member variables
  xInitLibCfg();
  xCreateLib();
  xInitLib(m_isField);

  // main encoder loop
  Int   iNumEncoded = 0;
  Bool  bEos = false;

  list<AccessUnit> outputAccessUnits; ///< list of access units to write out.  is populated by the encoding process

  // allocate original YUV buffer
  if( m_isField )
  {
    pcPicYuvOrg->create( m_iSourceWidth, m_iSourceHeightOrg, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  }
  else
  {
    pcPicYuvOrg->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  }

  while ( !bEos )
  {
    // get buffers
    xGetBuffer(pcPicYuvRec);

    // read input YUV file
    m_cTVideoIOYuvInputFile.read( pcPicYuvOrg, m_aiPad );

    // increase number of received frames
    m_iFrameRcvd++;

    bEos = (m_isField && (m_iFrameRcvd == (m_framesToBeEncoded >> 1) )) || ( !m_isField && (m_iFrameRcvd == m_framesToBeEncoded) );
    Bool flush = 0;
    // if end of file (which is only detected on a read failure) flush the encoder of any queued pictures
    if (m_cTVideoIOYuvInputFile.isEof())
    {
      flush = true;
      bEos = true;
      m_iFrameRcvd--;
      m_cTEncTop.setFramesToBeEncoded(m_iFrameRcvd);
    }

    // call encoding function for one frame
    if ( m_isField )
    {
      m_cTEncTop.encode( bEos, flush ? 0 : pcPicYuvOrg, m_cListPicYuvRec, outputAccessUnits, iNumEncoded, m_isTopFieldFirst);
    }
    else
    {
    m_cTEncTop.encode( bEos, flush ? 0 : pcPicYuvOrg, m_cListPicYuvRec, outputAccessUnits, iNumEncoded );
    }

    // write bistream to file if necessary
    if ( iNumEncoded > 0 )
    {
      xWriteOutput(bitstreamFile, iNumEncoded, outputAccessUnits);
      outputAccessUnits.clear();
    }
  }

  m_cTEncTop.printSummary(m_isField);

  // delete original YUV buffer
  pcPicYuvOrg->destroy();
  delete pcPicYuvOrg;
  pcPicYuvOrg = NULL;

  // delete used buffers in encoder class
  m_cTEncTop.deletePicBuffer();

  // delete buffers & classes
  xDeleteBuffer();
  xDestroyLib();

  printRateSummary();

  return;
}
#endif

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - application has picture buffer list with size of GOP
 - picture buffer list acts as ring buffer
 - end of the list has the latest picture
 .
 */
#if SVC_EXTENSION
Void TAppEncTop::xGetBuffer( TComPicYuv*& rpcPicYuvRec, UInt layer)
{
  assert( m_iGOPSize > 0 );

  // org. buffer
  if ( m_acListPicYuvRec[layer].size() == (UInt)m_iGOPSize )
  {
    rpcPicYuvRec = m_acListPicYuvRec[layer].popFront();

  }
  else
  {
    rpcPicYuvRec = new TComPicYuv;

#if LAYER_CTB
#if AUXILIARY_PICTURES
    rpcPicYuvRec->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeight(), m_acLayerCfg[layer].getChromaFormatIDC(), m_acLayerCfg[layer].m_uiMaxCUWidth, m_acLayerCfg[layer].m_uiMaxCUHeight, m_acLayerCfg[layer].m_uiMaxCUDepth, NULL );
#else
    rpcPicYuvRec->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeight(), m_acLayerCfg[layer].m_uiMaxCUWidth, m_acLayerCfg[layer].m_uiMaxCUHeight, m_acLayerCfg[layer].m_uiMaxCUDepth, NULL );
#endif
#else
#if AUXILIARY_PICTURES
    rpcPicYuvRec->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeight(), m_acLayerCfg[layer].getChromaFormatIDC(), m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth, NULL );
#else
    rpcPicYuvRec->create( m_acLayerCfg[layer].getSourceWidth(), m_acLayerCfg[layer].getSourceHeight(), m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth, NULL );
#endif
#endif

  }
  m_acListPicYuvRec[layer].pushBack( rpcPicYuvRec );
}

Void TAppEncTop::xDeleteBuffer( )
{
  for(UInt layer=0; layer<m_numLayers; layer++)
  {
    TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_acListPicYuvRec[layer].begin();

    Int iSize = Int( m_acListPicYuvRec[layer].size() );

    for ( Int i = 0; i < iSize; i++ )
    {
      TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
      pcPicYuvRec->destroy();
      delete pcPicYuvRec; pcPicYuvRec = NULL;
    }
  }
}

Void TAppEncTop::xWriteRecon(UInt layer, Int iNumEncoded)
{
#if REPN_FORMAT_IN_VPS
  ChromaFormat chromaFormatIdc = m_acLayerCfg[layer].getChromaFormatIDC();
  Int xScal = TComSPS::getWinUnitX( chromaFormatIdc );
  Int yScal = TComSPS::getWinUnitY( chromaFormatIdc );
#endif

  if (m_isField)
  {
    //Reinterlace fields
    Int i;
    TComList<TComPicYuv*>::iterator iterPicYuvRec = m_acListPicYuvRec[layer].end();

    for ( i = 0; i < iNumEncoded; i++ )
    {
      --iterPicYuvRec;
    }

    for ( i = 0; i < iNumEncoded/2; i++ )
    {
      TComPicYuv*  pcPicYuvRecTop  = *(iterPicYuvRec++);
      TComPicYuv*  pcPicYuvRecBottom  = *(iterPicYuvRec++);

#if M0040_ADAPTIVE_RESOLUTION_CHANGE
      if (!m_acLayerCfg[layer].getReconFile().empty() && pcPicYuvRecTop->isReconstructed() && pcPicYuvRecBottom->isReconstructed())
#else
      if (!m_acLayerCfg[layer].getReconFile().empty())
#endif
      {
#if REPN_FORMAT_IN_VPS
        m_acTVideoIOYuvReconFile[layer].write( pcPicYuvRecTop, pcPicYuvRecBottom, m_acLayerCfg[layer].getConfWinLeft() * xScal, m_acLayerCfg[layer].getConfWinRight() * xScal, 
          m_acLayerCfg[layer].getConfWinTop() * yScal, m_acLayerCfg[layer].getConfWinBottom() * yScal, m_isTopFieldFirst );
#else
        m_acTVideoIOYuvReconFile[layer].write( pcPicYuvRecTop, pcPicYuvRecBottom, m_acLayerCfg[layer].getConfWinLeft(), m_acLayerCfg[layer].getConfWinRight(), m_acLayerCfg[layer].getConfWinTop(), m_acLayerCfg[layer].getConfWinBottom(), m_isTopFieldFirst );
#endif
      }
    }
  }
  else
  {
    Int i;

    TComList<TComPicYuv*>::iterator iterPicYuvRec = m_acListPicYuvRec[layer].end();

    for ( i = 0; i < iNumEncoded; i++ )
    {
      --iterPicYuvRec;
    }

    for ( i = 0; i < iNumEncoded; i++ )
    {
      TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
#if M0040_ADAPTIVE_RESOLUTION_CHANGE
      if (!m_acLayerCfg[layer].getReconFile().empty() && pcPicYuvRec->isReconstructed())
#else
      if (!m_acLayerCfg[layer].getReconFile().empty())
#endif
      {
#if REPN_FORMAT_IN_VPS
        m_acTVideoIOYuvReconFile[layer].write( pcPicYuvRec, m_acLayerCfg[layer].getConfWinLeft() * xScal, m_acLayerCfg[layer].getConfWinRight() * xScal,
          m_acLayerCfg[layer].getConfWinTop() * yScal, m_acLayerCfg[layer].getConfWinBottom() * yScal );
#else
        m_acTVideoIOYuvReconFile[layer].write( pcPicYuvRec, m_acLayerCfg[layer].getConfWinLeft(), m_acLayerCfg[layer].getConfWinRight(),
          m_acLayerCfg[layer].getConfWinTop(), m_acLayerCfg[layer].getConfWinBottom() );
#endif
      }
    }
  }
}

Void TAppEncTop::xWriteStream(std::ostream& bitstreamFile, Int iNumEncoded, const std::list<AccessUnit>& accessUnits)
{
  if (m_isField)
  {
    //Reinterlace fields
    Int i;
    list<AccessUnit>::const_iterator iterBitstream = accessUnits.begin();

#if M0040_ADAPTIVE_RESOLUTION_CHANGE
    for ( i = 0; i < iNumEncoded/2 && iterBitstream != accessUnits.end(); i++ )
#else
    for ( i = 0; i < iNumEncoded/2; i++ )
#endif
    {
      const AccessUnit& auTop = *(iterBitstream++);
      const vector<UInt>& statsTop = writeAnnexB(bitstreamFile, auTop);
      rateStatsAccum(auTop, statsTop);

      const AccessUnit& auBottom = *(iterBitstream++);
      const vector<UInt>& statsBottom = writeAnnexB(bitstreamFile, auBottom);
      rateStatsAccum(auBottom, statsBottom);
    }
  }
  else
  {
    Int i;

    list<AccessUnit>::const_iterator iterBitstream = accessUnits.begin();

#if M0040_ADAPTIVE_RESOLUTION_CHANGE
    for ( i = 0; i < iNumEncoded && iterBitstream != accessUnits.end(); i++ )
#else
    for ( i = 0; i < iNumEncoded; i++ )
#endif
    {
      const AccessUnit& au = *(iterBitstream++);
      const vector<UInt>& stats = writeAnnexB(bitstreamFile, au);
      rateStatsAccum(au, stats);
    }
  }
}

#else // SVC_EXTENSION
Void TAppEncTop::xGetBuffer( TComPicYuv*& rpcPicYuvRec)
{
  assert( m_iGOPSize > 0 );

  // org. buffer
  if ( m_cListPicYuvRec.size() == (UInt)m_iGOPSize )
  {
    rpcPicYuvRec = m_cListPicYuvRec.popFront();

  }
  else
  {
    rpcPicYuvRec = new TComPicYuv;

    rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );

  }
  m_cListPicYuvRec.pushBack( rpcPicYuvRec );
}

Void TAppEncTop::xDeleteBuffer( )
{
  TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_cListPicYuvRec.begin();

  Int iSize = Int( m_cListPicYuvRec.size() );

  for ( Int i = 0; i < iSize; i++ )
  {
    TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
    pcPicYuvRec->destroy();
    delete pcPicYuvRec; pcPicYuvRec = NULL;
  }

}

/** \param iNumEncoded  number of encoded frames
 */
Void TAppEncTop::xWriteOutput(std::ostream& bitstreamFile, Int iNumEncoded, const std::list<AccessUnit>& accessUnits)
{
  if (m_isField)
  {
    //Reinterlace fields
    Int i;
    TComList<TComPicYuv*>::iterator iterPicYuvRec = m_cListPicYuvRec.end();
    list<AccessUnit>::const_iterator iterBitstream = accessUnits.begin();

    for ( i = 0; i < iNumEncoded; i++ )
    {
      --iterPicYuvRec;
    }

    for ( i = 0; i < iNumEncoded/2; i++ )
    {
      TComPicYuv*  pcPicYuvRecTop  = *(iterPicYuvRec++);
      TComPicYuv*  pcPicYuvRecBottom  = *(iterPicYuvRec++);

      if (m_pchReconFile)
      {
        m_cTVideoIOYuvReconFile.write( pcPicYuvRecTop, pcPicYuvRecBottom, m_confWinLeft, m_confWinRight, m_confWinTop, m_confWinBottom, m_isTopFieldFirst );
      }

      const AccessUnit& auTop = *(iterBitstream++);
      const vector<UInt>& statsTop = writeAnnexB(bitstreamFile, auTop);
      rateStatsAccum(auTop, statsTop);

      const AccessUnit& auBottom = *(iterBitstream++);
      const vector<UInt>& statsBottom = writeAnnexB(bitstreamFile, auBottom);
      rateStatsAccum(auBottom, statsBottom);
    }
  }
  else
  {
    Int i;
    TComList<TComPicYuv*>::iterator iterPicYuvRec = m_cListPicYuvRec.end();
    list<AccessUnit>::const_iterator iterBitstream = accessUnits.begin();

    for ( i = 0; i < iNumEncoded; i++ )
    {
      --iterPicYuvRec;
    }

    for ( i = 0; i < iNumEncoded; i++ )
    {
      TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
      if (m_pchReconFile)
      {
        m_cTVideoIOYuvReconFile.write( pcPicYuvRec, m_confWinLeft, m_confWinRight, m_confWinTop, m_confWinBottom );
      }

      const AccessUnit& au = *(iterBitstream++);
      const vector<UInt>& stats = writeAnnexB(bitstreamFile, au);
      rateStatsAccum(au, stats);
    }
  }
}
#endif

/**
 *
 */
void TAppEncTop::rateStatsAccum(const AccessUnit& au, const std::vector<UInt>& annexBsizes)
{
  AccessUnit::const_iterator it_au = au.begin();
  vector<UInt>::const_iterator it_stats = annexBsizes.begin();

  for (; it_au != au.end(); it_au++, it_stats++)
  {
    switch ((*it_au)->m_nalUnitType)
    {
    case NAL_UNIT_CODED_SLICE_TRAIL_R:
    case NAL_UNIT_CODED_SLICE_TRAIL_N:
    case NAL_UNIT_CODED_SLICE_TSA_R:
    case NAL_UNIT_CODED_SLICE_TSA_N:
    case NAL_UNIT_CODED_SLICE_STSA_R:
    case NAL_UNIT_CODED_SLICE_STSA_N:
    case NAL_UNIT_CODED_SLICE_BLA_W_LP:
    case NAL_UNIT_CODED_SLICE_BLA_W_RADL:
    case NAL_UNIT_CODED_SLICE_BLA_N_LP:
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:
    case NAL_UNIT_CODED_SLICE_CRA:
    case NAL_UNIT_CODED_SLICE_RADL_N:
    case NAL_UNIT_CODED_SLICE_RADL_R:
    case NAL_UNIT_CODED_SLICE_RASL_N:
    case NAL_UNIT_CODED_SLICE_RASL_R:
    case NAL_UNIT_VPS:
    case NAL_UNIT_SPS:
    case NAL_UNIT_PPS:
      m_essentialBytes += *it_stats;
      break;
    default:
      break;
    }

    m_totalBytes += *it_stats;
  }
}

void TAppEncTop::printRateSummary()
{
#if SVC_EXTENSION
  Double time = (Double) m_iFrameRcvd / m_acLayerCfg[m_numLayers-1].getFrameRate();
#else
  Double time = (Double) m_iFrameRcvd / m_iFrameRate;
#endif
  printf("Bytes written to file: %u (%.3f kbps)\n", m_totalBytes, 0.008 * m_totalBytes / time);
#if VERBOSE_RATE
  printf("Bytes for SPS/PPS/Slice (Incl. Annex B): %u (%.3f kbps)\n", m_essentialBytes, 0.008 * m_essentialBytes / time);
#endif
}

//! \}
