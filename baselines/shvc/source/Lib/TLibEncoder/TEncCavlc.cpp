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

/** \file     TEncCavlc.cpp
    \brief    CAVLC encoder class
*/

#include "../TLibCommon/CommonDef.h"
#include "TEncCavlc.h"
#include "SEIwrite.h"

//! \ingroup TLibEncoder
//! \{

#if ENC_DEC_TRACE

Void  xTraceSPSHeader (TComSPS *pSPS)
{
  fprintf( g_hTrace, "=========== Sequence Parameter Set ID: %d ===========\n", pSPS->getSPSId() );
}

Void  xTracePPSHeader (TComPPS *pPPS)
{
  fprintf( g_hTrace, "=========== Picture Parameter Set ID: %d ===========\n", pPPS->getPPSId() );
}

Void  xTraceSliceHeader (TComSlice *pSlice)
{
  fprintf( g_hTrace, "=========== Slice ===========\n");
}

#endif



// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncCavlc::TEncCavlc()
{
  m_pcBitIf           = NULL;
  m_uiCoeffCost       = 0;
}

TEncCavlc::~TEncCavlc()
{
}


// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncCavlc::resetEntropy()
{
}


Void TEncCavlc::codeDFFlag(UInt uiCode, const Char *pSymbolName)
{
  WRITE_FLAG(uiCode, pSymbolName);
}
Void TEncCavlc::codeDFSvlc(Int iCode, const Char *pSymbolName)
{
  WRITE_SVLC(iCode, pSymbolName);
}

Void TEncCavlc::codeShortTermRefPicSet( TComSPS* pcSPS, TComReferencePictureSet* rps, Bool calledFromSliceHeader, Int idx)
{
#if PRINT_RPS_INFO
  Int lastBits = getNumberOfWrittenBits();
#endif
  if (idx > 0)
  {
  WRITE_FLAG( rps->getInterRPSPrediction(), "inter_ref_pic_set_prediction_flag" ); // inter_RPS_prediction_flag
  }
  if (rps->getInterRPSPrediction())
  {
    Int deltaRPS = rps->getDeltaRPS();
    if(calledFromSliceHeader)
    {
      WRITE_UVLC( rps->getDeltaRIdxMinus1(), "delta_idx_minus1" ); // delta index of the Reference Picture Set used for prediction minus 1
    }

    WRITE_CODE( (deltaRPS >=0 ? 0: 1), 1, "delta_rps_sign" ); //delta_rps_sign
    WRITE_UVLC( abs(deltaRPS) - 1, "abs_delta_rps_minus1"); // absolute delta RPS minus 1

    for(Int j=0; j < rps->getNumRefIdc(); j++)
    {
      Int refIdc = rps->getRefIdc(j);
      WRITE_CODE( (refIdc==1? 1: 0), 1, "used_by_curr_pic_flag" ); //first bit is "1" if Idc is 1 
      if (refIdc != 1) 
      {
        WRITE_CODE( refIdc>>1, 1, "use_delta_flag" ); //second bit is "1" if Idc is 2, "0" otherwise.
      }
    }
  }
  else
  {
    WRITE_UVLC( rps->getNumberOfNegativePictures(), "num_negative_pics" );
    WRITE_UVLC( rps->getNumberOfPositivePictures(), "num_positive_pics" );
    Int prev = 0;
    for(Int j=0 ; j < rps->getNumberOfNegativePictures(); j++)
    {
      WRITE_UVLC( prev-rps->getDeltaPOC(j)-1, "delta_poc_s0_minus1" );
      prev = rps->getDeltaPOC(j);
      WRITE_FLAG( rps->getUsed(j), "used_by_curr_pic_s0_flag"); 
    }
    prev = 0;
    for(Int j=rps->getNumberOfNegativePictures(); j < rps->getNumberOfNegativePictures()+rps->getNumberOfPositivePictures(); j++)
    {
      WRITE_UVLC( rps->getDeltaPOC(j)-prev-1, "delta_poc_s1_minus1" );
      prev = rps->getDeltaPOC(j);
      WRITE_FLAG( rps->getUsed(j), "used_by_curr_pic_s1_flag" ); 
    }
  }

#if PRINT_RPS_INFO
  printf("irps=%d (%2d bits) ", rps->getInterRPSPrediction(), getNumberOfWrittenBits() - lastBits);
  rps->printDeltaPOC();
#endif
}


Void TEncCavlc::codePPS( TComPPS* pcPPS 
#if Q0048_CGS_3D_ASYMLUT
  , TEnc3DAsymLUT * pc3DAsymLUT
#endif
  )
{
#if ENC_DEC_TRACE  
  xTracePPSHeader (pcPPS);
#endif
  
  WRITE_UVLC( pcPPS->getPPSId(),                             "pps_pic_parameter_set_id" );
  WRITE_UVLC( pcPPS->getSPSId(),                             "pps_seq_parameter_set_id" );
  WRITE_FLAG( pcPPS->getDependentSliceSegmentsEnabledFlag()    ? 1 : 0, "dependent_slice_segments_enabled_flag" );
  WRITE_FLAG( pcPPS->getOutputFlagPresentFlag() ? 1 : 0,     "output_flag_present_flag" );
  WRITE_CODE( pcPPS->getNumExtraSliceHeaderBits(), 3,        "num_extra_slice_header_bits");
  WRITE_FLAG( pcPPS->getSignHideFlag(), "sign_data_hiding_flag" );
  WRITE_FLAG( pcPPS->getCabacInitPresentFlag() ? 1 : 0,   "cabac_init_present_flag" );
  WRITE_UVLC( pcPPS->getNumRefIdxL0DefaultActive()-1,     "num_ref_idx_l0_default_active_minus1");
  WRITE_UVLC( pcPPS->getNumRefIdxL1DefaultActive()-1,     "num_ref_idx_l1_default_active_minus1");

  WRITE_SVLC( pcPPS->getPicInitQPMinus26(),                  "init_qp_minus26");
  WRITE_FLAG( pcPPS->getConstrainedIntraPred() ? 1 : 0,      "constrained_intra_pred_flag" );
  WRITE_FLAG( pcPPS->getUseTransformSkip() ? 1 : 0,  "transform_skip_enabled_flag" ); 
  WRITE_FLAG( pcPPS->getUseDQP() ? 1 : 0, "cu_qp_delta_enabled_flag" );
  if ( pcPPS->getUseDQP() )
  {
    WRITE_UVLC( pcPPS->getMaxCuDQPDepth(), "diff_cu_qp_delta_depth" );
  }
  WRITE_SVLC( pcPPS->getChromaCbQpOffset(),                   "pps_cb_qp_offset" );
  WRITE_SVLC( pcPPS->getChromaCrQpOffset(),                   "pps_cr_qp_offset" );
  WRITE_FLAG( pcPPS->getSliceChromaQpFlag() ? 1 : 0,          "pps_slice_chroma_qp_offsets_present_flag" );

  WRITE_FLAG( pcPPS->getUseWP() ? 1 : 0,  "weighted_pred_flag" );   // Use of Weighting Prediction (P_SLICE)
  WRITE_FLAG( pcPPS->getWPBiPred() ? 1 : 0, "weighted_bipred_flag" );  // Use of Weighting Bi-Prediction (B_SLICE)
  WRITE_FLAG( pcPPS->getTransquantBypassEnableFlag() ? 1 : 0, "transquant_bypass_enable_flag" );
  WRITE_FLAG( pcPPS->getTilesEnabledFlag()             ? 1 : 0, "tiles_enabled_flag" );
  WRITE_FLAG( pcPPS->getEntropyCodingSyncEnabledFlag() ? 1 : 0, "entropy_coding_sync_enabled_flag" );
  if( pcPPS->getTilesEnabledFlag() )
  {
    WRITE_UVLC( pcPPS->getNumTileColumnsMinus1(),                                    "num_tile_columns_minus1" );
    WRITE_UVLC( pcPPS->getTileNumRowsMinus1(),                                       "num_tile_rows_minus1" );
    WRITE_FLAG( pcPPS->getTileUniformSpacingFlag(),                                  "uniform_spacing_flag" );
    if( !pcPPS->getTileUniformSpacingFlag() )
    {
      for(UInt i=0; i<pcPPS->getNumTileColumnsMinus1(); i++)
      {
        WRITE_UVLC( pcPPS->getTileColumnWidth(i)-1,                                  "column_width_minus1" );
      }
      for(UInt i=0; i<pcPPS->getTileNumRowsMinus1(); i++)
      {
        WRITE_UVLC( pcPPS->getTileRowHeight(i)-1,                                    "row_height_minus1" );
      }
    }
    if(pcPPS->getNumTileColumnsMinus1() !=0 || pcPPS->getTileNumRowsMinus1() !=0)
    {
      WRITE_FLAG( pcPPS->getLoopFilterAcrossTilesEnabledFlag()?1 : 0,          "loop_filter_across_tiles_enabled_flag");
    }
  }
  WRITE_FLAG( pcPPS->getLoopFilterAcrossSlicesEnabledFlag()?1 : 0,        "loop_filter_across_slices_enabled_flag");
  WRITE_FLAG( pcPPS->getDeblockingFilterControlPresentFlag()?1 : 0,       "deblocking_filter_control_present_flag");
  if(pcPPS->getDeblockingFilterControlPresentFlag())
  {
    WRITE_FLAG( pcPPS->getDeblockingFilterOverrideEnabledFlag() ? 1 : 0,  "deblocking_filter_override_enabled_flag" ); 
    WRITE_FLAG( pcPPS->getPicDisableDeblockingFilterFlag() ? 1 : 0,       "pps_disable_deblocking_filter_flag" );
    if(!pcPPS->getPicDisableDeblockingFilterFlag())
    {
      WRITE_SVLC( pcPPS->getDeblockingFilterBetaOffsetDiv2(),             "pps_beta_offset_div2" );
      WRITE_SVLC( pcPPS->getDeblockingFilterTcOffsetDiv2(),               "pps_tc_offset_div2" );
    }
  }

#if !R0042_PROFILE_INDICATION
#if SCALINGLIST_INFERRING
  if( pcPPS->getLayerId() > 0 )
  {
    WRITE_FLAG( pcPPS->getInferScalingListFlag() ? 1 : 0, "pps_infer_scaling_list_flag" );
  }

  if( pcPPS->getInferScalingListFlag() )
  {
    // The value of pps_scaling_list_ref_layer_id shall be in the range of 0 to 62, inclusive
    assert( pcPPS->getScalingListRefLayerId() <= 62 );

    WRITE_UVLC( pcPPS->getScalingListRefLayerId(), "pps_scaling_list_ref_layer_id" );
  }
  else
  {
#endif
#endif

  WRITE_FLAG( pcPPS->getScalingListPresentFlag() ? 1 : 0,                          "pps_scaling_list_data_present_flag" ); 
  if( pcPPS->getScalingListPresentFlag() )
  {
    codeScalingList( m_pcSlice->getScalingList() );
  }
#if !R0042_PROFILE_INDICATION
#if SCALINGLIST_INFERRING
  }
#endif
#endif

  WRITE_FLAG( pcPPS->getListsModificationPresentFlag(), "lists_modification_present_flag");
  WRITE_UVLC( pcPPS->getLog2ParallelMergeLevelMinus2(), "log2_parallel_merge_level_minus2");
  WRITE_FLAG( pcPPS->getSliceHeaderExtensionPresentFlag() ? 1 : 0, "slice_segment_header_extension_present_flag");
#if P0166_MODIFIED_PPS_EXTENSION
#if !R0042_PROFILE_INDICATION
  WRITE_FLAG( pcPPS->getExtensionFlag() ? 1 : 0, "pps_extension_flag" );
#else
  WRITE_FLAG( pcPPS->getExtensionFlag() ? 1 : 0, "pps_extension_present_flag" );
#endif

  if( pcPPS->getExtensionFlag() )
  {
#if !R0042_PROFILE_INDICATION
#if !POC_RESET_IDC
    UInt ppsExtensionTypeFlag[8] = { 0, 1, 0, 0, 0, 0, 0, 0 };
#else
    UInt ppsExtensionTypeFlag[8] = { 1, 0, 0, 0, 0, 0, 0, 0 };
#endif
    for (UInt i = 0; i < 8; i++)
    {
      WRITE_FLAG( ppsExtensionTypeFlag[i], "pps_extension_type_flag" );
    }
#if POC_RESET_IDC
    if( ppsExtensionTypeFlag[0] )
    {
      WRITE_FLAG( pcPPS->getPocResetInfoPresentFlag() ? 1 : 0, "poc_reset_info_present_flag" );
#if REF_REGION_OFFSET
      WRITE_UVLC( pcPPS->getNumScaledRefLayerOffsets(),      "num_scaled_ref_layer_offsets" );
      for(Int i = 0; i < pcPPS->getNumScaledRefLayerOffsets(); i++)
      {
        WRITE_CODE( pcPPS->getScaledRefLayerId(i), 6, "scaled_ref_layer_id" );
        WRITE_FLAG( pcPPS->getScaledRefLayerOffsetPresentFlag(i) ? 1 : 0, "scaled_ref_layer_offset_prsent_flag" );
        if (pcPPS->getScaledRefLayerOffsetPresentFlag(i))
        {
          Window scaledWindow = pcPPS->getScaledRefLayerWindow(i);
          WRITE_SVLC( scaledWindow.getWindowLeftOffset()   >> 1, "scaled_ref_layer_left_offset" );
          WRITE_SVLC( scaledWindow.getWindowTopOffset()    >> 1, "scaled_ref_layer_top_offset" );
          WRITE_SVLC( scaledWindow.getWindowRightOffset()  >> 1, "scaled_ref_layer_right_offset" );
          WRITE_SVLC( scaledWindow.getWindowBottomOffset() >> 1, "scaled_ref_layer_bottom_offset" );
        }
        WRITE_FLAG( pcPPS->getRefRegionOffsetPresentFlag(i) ? 1 : 0, "ref_region_offset_prsent_flag" );
        if (pcPPS->getRefRegionOffsetPresentFlag(i))
        {
          Window refWindow = pcPPS->getRefLayerWindow(i);
          WRITE_SVLC( refWindow.getWindowLeftOffset()   >> 1, "ref_layer_left_offset" );
          WRITE_SVLC( refWindow.getWindowTopOffset()    >> 1, "ref_layer_top_offset" );
          WRITE_SVLC( refWindow.getWindowRightOffset()  >> 1, "ref_layer_right_offset" );
          WRITE_SVLC( refWindow.getWindowBottomOffset() >> 1, "ref_layer_bottom_offset" );
        }
#if R0209_GENERIC_PHASE
        WRITE_FLAG( pcPPS->getResamplePhaseSetPresentFlag(i) ? 1 : 0, "resample_phase_set_present_flag" );
        if (pcPPS->getResamplePhaseSetPresentFlag(i))
        {
          WRITE_UVLC( pcPPS->getPhaseHorLuma(i), "phase_hor_luma" );
          WRITE_UVLC( pcPPS->getPhaseVerLuma(i), "phase_ver_luma" );
          WRITE_UVLC( pcPPS->getPhaseHorChroma(i) + 8, "phase_hor_chroma_plus8" );
          WRITE_UVLC( pcPPS->getPhaseVerChroma(i) + 8, "phase_ver_chroma_plus8" );
        }
#endif
      }
#else
#if MOVE_SCALED_OFFSET_TO_PPS
      WRITE_UVLC( pcPPS->getNumScaledRefLayerOffsets(),      "num_scaled_ref_layer_offsets" );
      for(Int i = 0; i < pcPPS->getNumScaledRefLayerOffsets(); i++)
      {
        Window scaledWindow = pcPPS->getScaledRefLayerWindow(i);
#if O0098_SCALED_REF_LAYER_ID
        WRITE_CODE( pcPPS->getScaledRefLayerId(i), 6,          "scaled_ref_layer_id" );
#endif
        WRITE_SVLC( scaledWindow.getWindowLeftOffset()   >> 1, "scaled_ref_layer_left_offset" );
        WRITE_SVLC( scaledWindow.getWindowTopOffset()    >> 1, "scaled_ref_layer_top_offset" );
        WRITE_SVLC( scaledWindow.getWindowRightOffset()  >> 1, "scaled_ref_layer_right_offset" );
        WRITE_SVLC( scaledWindow.getWindowBottomOffset() >> 1, "scaled_ref_layer_bottom_offset" );
      }
#endif
#endif
#if Q0048_CGS_3D_ASYMLUT
      UInt uiPos = getNumberOfWrittenBits();
      WRITE_FLAG( pcPPS->getCGSFlag() , "colour_mapping_enabled_flag" );
      if( pcPPS->getCGSFlag() )
      {
        assert( pc3DAsymLUT != NULL );
        xCode3DAsymLUT( pc3DAsymLUT );
      }
      pc3DAsymLUT->setPPSBit( getNumberOfWrittenBits() - uiPos );
#endif
#endif
    }
#else
  WRITE_FLAG( 0, "pps_range_extension_flag" );
  WRITE_FLAG( 1, "pps_multilayer_extension_flag" );
  WRITE_CODE( 0, 6,  "pps_extension_6bits" );     

  WRITE_FLAG( pcPPS->getPocResetInfoPresentFlag() ? 1 : 0, "poc_reset_info_present_flag" );
#if SCALINGLIST_INFERRING
  WRITE_FLAG( pcPPS->getInferScalingListFlag() ? 1 : 0, "pps_infer_scaling_list_flag" );
  if( pcPPS->getInferScalingListFlag() )
  {
    // The value of pps_scaling_list_ref_layer_id shall be in the range of 0 to 62, inclusive
    assert( pcPPS->getScalingListRefLayerId() <= 62 );
    WRITE_UVLC( pcPPS->getScalingListRefLayerId(), "pps_scaling_list_ref_layer_id" );
  }
#endif

#if REF_REGION_OFFSET
  WRITE_UVLC( pcPPS->getNumScaledRefLayerOffsets(),      "num_ref_loc_offsets" );
  for(Int i = 0; i < pcPPS->getNumScaledRefLayerOffsets(); i++)
  {
    WRITE_CODE( pcPPS->getScaledRefLayerId(i), 6, "ref_loc_offset_layer_id" );
    WRITE_FLAG( pcPPS->getScaledRefLayerOffsetPresentFlag(i) ? 1 : 0, "scaled_ref_layer_offset_prsent_flag" );
    if (pcPPS->getScaledRefLayerOffsetPresentFlag(i))
    {
      Window scaledWindow = pcPPS->getScaledRefLayerWindow(i);
      WRITE_SVLC( scaledWindow.getWindowLeftOffset()   >> 1, "scaled_ref_layer_left_offset" );
      WRITE_SVLC( scaledWindow.getWindowTopOffset()    >> 1, "scaled_ref_layer_top_offset" );
      WRITE_SVLC( scaledWindow.getWindowRightOffset()  >> 1, "scaled_ref_layer_right_offset" );
      WRITE_SVLC( scaledWindow.getWindowBottomOffset() >> 1, "scaled_ref_layer_bottom_offset" );
    }
    WRITE_FLAG( pcPPS->getRefRegionOffsetPresentFlag(i) ? 1 : 0, "ref_region_offset_prsent_flag" );
    if (pcPPS->getRefRegionOffsetPresentFlag(i))
    {
      Window refWindow = pcPPS->getRefLayerWindow(i);
      WRITE_SVLC( refWindow.getWindowLeftOffset()   >> 1, "ref_region_left_offset" );
      WRITE_SVLC( refWindow.getWindowTopOffset()    >> 1, "ref_region_top_offset" );
      WRITE_SVLC( refWindow.getWindowRightOffset()  >> 1, "ref_region_right_offset" );
      WRITE_SVLC( refWindow.getWindowBottomOffset() >> 1, "ref_region_bottom_offset" );
    }
#if R0209_GENERIC_PHASE
    WRITE_FLAG( pcPPS->getResamplePhaseSetPresentFlag(i) ? 1 : 0, "resample_phase_set_present_flag" );
    if (pcPPS->getResamplePhaseSetPresentFlag(i))
    {
      WRITE_UVLC( pcPPS->getPhaseHorLuma(i), "phase_hor_luma" );
      WRITE_UVLC( pcPPS->getPhaseVerLuma(i), "phase_ver_luma" );
      WRITE_UVLC( pcPPS->getPhaseHorChroma(i) + 8, "phase_hor_chroma_plus8" );
      WRITE_UVLC( pcPPS->getPhaseVerChroma(i) + 8, "phase_ver_chroma_plus8" );
    }
#endif
  }
#else
#if MOVE_SCALED_OFFSET_TO_PPS
  WRITE_UVLC( pcPPS->getNumScaledRefLayerOffsets(),      "num_scaled_ref_layer_offsets" );
  for(Int i = 0; i < pcPPS->getNumScaledRefLayerOffsets(); i++)
  {
    Window scaledWindow = pcPPS->getScaledRefLayerWindow(i);
#if O0098_SCALED_REF_LAYER_ID
    WRITE_CODE( pcPPS->getScaledRefLayerId(i), 6,          "scaled_ref_layer_id" );
#endif
    WRITE_SVLC( scaledWindow.getWindowLeftOffset()   >> 1, "scaled_ref_layer_left_offset" );
    WRITE_SVLC( scaledWindow.getWindowTopOffset()    >> 1, "scaled_ref_layer_top_offset" );
    WRITE_SVLC( scaledWindow.getWindowRightOffset()  >> 1, "scaled_ref_layer_right_offset" );
    WRITE_SVLC( scaledWindow.getWindowBottomOffset() >> 1, "scaled_ref_layer_bottom_offset" );
  }
#endif
#endif
#if Q0048_CGS_3D_ASYMLUT
  UInt uiPos = getNumberOfWrittenBits();
  WRITE_FLAG( pcPPS->getCGSFlag() , "colour_mapping_enabled_flag" );
  if( pcPPS->getCGSFlag() )
  {
    assert( pc3DAsymLUT != NULL );
    xCode3DAsymLUT( pc3DAsymLUT );
  }
  pc3DAsymLUT->setPPSBit( getNumberOfWrittenBits() - uiPos );
#endif
#endif
  }
#else
  WRITE_FLAG( 0, "pps_extension_flag" );
#endif
}

Void TEncCavlc::codeVUI( TComVUI *pcVUI, TComSPS* pcSPS )
{
#if ENC_DEC_TRACE
  fprintf( g_hTrace, "----------- vui_parameters -----------\n");
#endif
  WRITE_FLAG(pcVUI->getAspectRatioInfoPresentFlag(),            "aspect_ratio_info_present_flag");
  if (pcVUI->getAspectRatioInfoPresentFlag())
  {
    WRITE_CODE(pcVUI->getAspectRatioIdc(), 8,                   "aspect_ratio_idc" );
    if (pcVUI->getAspectRatioIdc() == 255)
    {
      WRITE_CODE(pcVUI->getSarWidth(), 16,                      "sar_width");
      WRITE_CODE(pcVUI->getSarHeight(), 16,                     "sar_height");
    }
  }
  WRITE_FLAG(pcVUI->getOverscanInfoPresentFlag(),               "overscan_info_present_flag");
  if (pcVUI->getOverscanInfoPresentFlag())
  {
    WRITE_FLAG(pcVUI->getOverscanAppropriateFlag(),             "overscan_appropriate_flag");
  }
  WRITE_FLAG(pcVUI->getVideoSignalTypePresentFlag(),            "video_signal_type_present_flag");
  if (pcVUI->getVideoSignalTypePresentFlag())
  {
    WRITE_CODE(pcVUI->getVideoFormat(), 3,                      "video_format");
    WRITE_FLAG(pcVUI->getVideoFullRangeFlag(),                  "video_full_range_flag");
    WRITE_FLAG(pcVUI->getColourDescriptionPresentFlag(),        "colour_description_present_flag");
    if (pcVUI->getColourDescriptionPresentFlag())
    {
      WRITE_CODE(pcVUI->getColourPrimaries(), 8,                "colour_primaries");
      WRITE_CODE(pcVUI->getTransferCharacteristics(), 8,        "transfer_characteristics");
      WRITE_CODE(pcVUI->getMatrixCoefficients(), 8,             "matrix_coefficients");
    }
  }

  WRITE_FLAG(pcVUI->getChromaLocInfoPresentFlag(),              "chroma_loc_info_present_flag");
  if (pcVUI->getChromaLocInfoPresentFlag())
  {
    WRITE_UVLC(pcVUI->getChromaSampleLocTypeTopField(),         "chroma_sample_loc_type_top_field");
    WRITE_UVLC(pcVUI->getChromaSampleLocTypeBottomField(),      "chroma_sample_loc_type_bottom_field");
  }

  WRITE_FLAG(pcVUI->getNeutralChromaIndicationFlag(),           "neutral_chroma_indication_flag");
  WRITE_FLAG(pcVUI->getFieldSeqFlag(),                          "field_seq_flag");
  WRITE_FLAG(pcVUI->getFrameFieldInfoPresentFlag(),             "frame_field_info_present_flag");

  Window defaultDisplayWindow = pcVUI->getDefaultDisplayWindow();
  WRITE_FLAG(defaultDisplayWindow.getWindowEnabledFlag(),       "default_display_window_flag");
  if( defaultDisplayWindow.getWindowEnabledFlag() )
  {
    WRITE_UVLC(defaultDisplayWindow.getWindowLeftOffset()  / TComSPS::getWinUnitX(pcSPS->getChromaFormatIdc()), "def_disp_win_left_offset");
    WRITE_UVLC(defaultDisplayWindow.getWindowRightOffset() / TComSPS::getWinUnitX(pcSPS->getChromaFormatIdc()), "def_disp_win_right_offset");
    WRITE_UVLC(defaultDisplayWindow.getWindowTopOffset()   / TComSPS::getWinUnitY(pcSPS->getChromaFormatIdc()), "def_disp_win_top_offset");
    WRITE_UVLC(defaultDisplayWindow.getWindowBottomOffset()/ TComSPS::getWinUnitY(pcSPS->getChromaFormatIdc()), "def_disp_win_bottom_offset");
  }
  TimingInfo *timingInfo = pcVUI->getTimingInfo();
  WRITE_FLAG(timingInfo->getTimingInfoPresentFlag(),          "vui_timing_info_present_flag");
  if(timingInfo->getTimingInfoPresentFlag())
  {
    WRITE_CODE(timingInfo->getNumUnitsInTick(), 32,           "vui_num_units_in_tick");
    WRITE_CODE(timingInfo->getTimeScale(),      32,           "vui_time_scale");
    WRITE_FLAG(timingInfo->getPocProportionalToTimingFlag(),  "vui_poc_proportional_to_timing_flag");
    if(timingInfo->getPocProportionalToTimingFlag())
    {
      WRITE_UVLC(timingInfo->getNumTicksPocDiffOneMinus1(),   "vui_num_ticks_poc_diff_one_minus1");
    }
  WRITE_FLAG(pcVUI->getHrdParametersPresentFlag(),              "hrd_parameters_present_flag");
  if( pcVUI->getHrdParametersPresentFlag() )
  {
    codeHrdParameters(pcVUI->getHrdParameters(), 1, pcSPS->getMaxTLayers() - 1 );
  }
  }

  WRITE_FLAG(pcVUI->getBitstreamRestrictionFlag(),              "bitstream_restriction_flag");
  if (pcVUI->getBitstreamRestrictionFlag())
  {
    WRITE_FLAG(pcVUI->getTilesFixedStructureFlag(),             "tiles_fixed_structure_flag");
    WRITE_FLAG(pcVUI->getMotionVectorsOverPicBoundariesFlag(),  "motion_vectors_over_pic_boundaries_flag");
    WRITE_FLAG(pcVUI->getRestrictedRefPicListsFlag(),           "restricted_ref_pic_lists_flag");
    WRITE_UVLC(pcVUI->getMinSpatialSegmentationIdc(),           "min_spatial_segmentation_idc");
    WRITE_UVLC(pcVUI->getMaxBytesPerPicDenom(),                 "max_bytes_per_pic_denom");
    WRITE_UVLC(pcVUI->getMaxBitsPerMinCuDenom(),                "max_bits_per_mincu_denom");
    WRITE_UVLC(pcVUI->getLog2MaxMvLengthHorizontal(),           "log2_max_mv_length_horizontal");
    WRITE_UVLC(pcVUI->getLog2MaxMvLengthVertical(),             "log2_max_mv_length_vertical");
  }
}

Void TEncCavlc::codeHrdParameters( TComHRD *hrd, Bool commonInfPresentFlag, UInt maxNumSubLayersMinus1 )
{
  if( commonInfPresentFlag )
  {
    WRITE_FLAG( hrd->getNalHrdParametersPresentFlag() ? 1 : 0 ,  "nal_hrd_parameters_present_flag" );
    WRITE_FLAG( hrd->getVclHrdParametersPresentFlag() ? 1 : 0 ,  "vcl_hrd_parameters_present_flag" );
    if( hrd->getNalHrdParametersPresentFlag() || hrd->getVclHrdParametersPresentFlag() )
    {
      WRITE_FLAG( hrd->getSubPicCpbParamsPresentFlag() ? 1 : 0,  "sub_pic_cpb_params_present_flag" );
      if( hrd->getSubPicCpbParamsPresentFlag() )
      {
        WRITE_CODE( hrd->getTickDivisorMinus2(), 8,              "tick_divisor_minus2" );
        WRITE_CODE( hrd->getDuCpbRemovalDelayLengthMinus1(), 5,  "du_cpb_removal_delay_length_minus1" );
        WRITE_FLAG( hrd->getSubPicCpbParamsInPicTimingSEIFlag() ? 1 : 0, "sub_pic_cpb_params_in_pic_timing_sei_flag" );
        WRITE_CODE( hrd->getDpbOutputDelayDuLengthMinus1(), 5,   "dpb_output_delay_du_length_minus1"  );
      }
      WRITE_CODE( hrd->getBitRateScale(), 4,                     "bit_rate_scale" );
      WRITE_CODE( hrd->getCpbSizeScale(), 4,                     "cpb_size_scale" );
      if( hrd->getSubPicCpbParamsPresentFlag() )
      {
        WRITE_CODE( hrd->getDuCpbSizeScale(), 4,                "du_cpb_size_scale" ); 
      }
      WRITE_CODE( hrd->getInitialCpbRemovalDelayLengthMinus1(), 5, "initial_cpb_removal_delay_length_minus1" );
      WRITE_CODE( hrd->getCpbRemovalDelayLengthMinus1(),        5, "au_cpb_removal_delay_length_minus1" );
      WRITE_CODE( hrd->getDpbOutputDelayLengthMinus1(),         5, "dpb_output_delay_length_minus1" );
    }
  }
  Int i, j, nalOrVcl;
  for( i = 0; i <= maxNumSubLayersMinus1; i ++ )
  {
    WRITE_FLAG( hrd->getFixedPicRateFlag( i ) ? 1 : 0,          "fixed_pic_rate_general_flag");
    if( !hrd->getFixedPicRateFlag( i ) )
    {
      WRITE_FLAG( hrd->getFixedPicRateWithinCvsFlag( i ) ? 1 : 0, "fixed_pic_rate_within_cvs_flag");
    }
    else
    {
      hrd->setFixedPicRateWithinCvsFlag( i, true );
    }
    if( hrd->getFixedPicRateWithinCvsFlag( i ) )
    {
      WRITE_UVLC( hrd->getPicDurationInTcMinus1( i ),           "elemental_duration_in_tc_minus1");
    }
    else
    {
      WRITE_FLAG( hrd->getLowDelayHrdFlag( i ) ? 1 : 0,           "low_delay_hrd_flag");
    }
    if (!hrd->getLowDelayHrdFlag( i ))
    {
      WRITE_UVLC( hrd->getCpbCntMinus1( i ),                      "cpb_cnt_minus1");
    }
    
    for( nalOrVcl = 0; nalOrVcl < 2; nalOrVcl ++ )
    {
      if( ( ( nalOrVcl == 0 ) && ( hrd->getNalHrdParametersPresentFlag() ) ) ||
          ( ( nalOrVcl == 1 ) && ( hrd->getVclHrdParametersPresentFlag() ) ) )
      {
        for( j = 0; j <= ( hrd->getCpbCntMinus1( i ) ); j ++ )
        {
          WRITE_UVLC( hrd->getBitRateValueMinus1( i, j, nalOrVcl ), "bit_rate_value_minus1");
          WRITE_UVLC( hrd->getCpbSizeValueMinus1( i, j, nalOrVcl ), "cpb_size_value_minus1");
          if( hrd->getSubPicCpbParamsPresentFlag() )
          {
            WRITE_UVLC( hrd->getDuCpbSizeValueMinus1( i, j, nalOrVcl ), "cpb_size_du_value_minus1");  
            WRITE_UVLC( hrd->getDuBitRateValueMinus1( i, j, nalOrVcl ), "bit_rate_du_value_minus1");
          }
          WRITE_FLAG( hrd->getCbrFlag( i, j, nalOrVcl ) ? 1 : 0, "cbr_flag");
        }
      }
    }
  }
}

Void TEncCavlc::codeSPS( TComSPS* pcSPS )
{
#if R0042_PROFILE_INDICATION
  Bool bMultiLayerExtSpsFlag = (pcSPS->getNumDirectRefLayers() != 0 ) ; 
#endif
#if ENC_DEC_TRACE  
  xTraceSPSHeader (pcSPS);
#endif
  WRITE_CODE( pcSPS->getVPSId (),          4,       "sps_video_parameter_set_id" );
#if SVC_EXTENSION
  if(pcSPS->getLayerId() == 0)
  {
#endif
    WRITE_CODE( pcSPS->getMaxTLayers() - 1,  3,       "sps_max_sub_layers_minus1" );
#if SVC_EXTENSION
  }
#if R0042_PROFILE_INDICATION
  else
  {
    WRITE_CODE(bMultiLayerExtSpsFlag? 7:(pcSPS->getMaxTLayers() - 1) ,  3,       "sps_ext_or_max_sub_layers_minus1" );
  }
#endif
#endif

#if SVC_EXTENSION
#if !R0042_PROFILE_INDICATION
  if(pcSPS->getLayerId() == 0)
#else
  if(!bMultiLayerExtSpsFlag)
#endif
  {
#endif
    WRITE_FLAG( pcSPS->getTemporalIdNestingFlag() ? 1 : 0,                             "sps_temporal_id_nesting_flag" );
#if SVC_EXTENSION
  }
#endif
#ifdef SPS_PTL_FIX
#if !R0042_PROFILE_INDICATION
  if (pcSPS->getLayerId() == 0)
#else
  if(!bMultiLayerExtSpsFlag)
#endif
  {
    codePTL(pcSPS->getPTL(), 1, pcSPS->getMaxTLayers() - 1);
  }
#else
  codePTL(pcSPS->getPTL(), 1, pcSPS->getMaxTLayers() - 1);
#endif
  WRITE_UVLC( pcSPS->getSPSId (),                   "sps_seq_parameter_set_id" );
#if REPN_FORMAT_IN_VPS
#if !R0042_PROFILE_INDICATION
  if( pcSPS->getLayerId() > 0 )
#else
  if(bMultiLayerExtSpsFlag)
#endif
  {
    WRITE_FLAG( pcSPS->getUpdateRepFormatFlag(), "update_rep_format_flag" );
  }
#if R0042_PROFILE_INDICATION
  if( bMultiLayerExtSpsFlag && pcSPS->getUpdateRepFormatFlag())
  {
    WRITE_CODE( pcSPS->getUpdateRepFormatIndex(), 8,   "sps_rep_format_idx");
  }
#endif

#if R0042_PROFILE_INDICATION
  if(!bMultiLayerExtSpsFlag)
  {
#endif
#if !R0042_PROFILE_INDICATION
#if O0096_REP_FORMAT_INDEX
  if( pcSPS->getLayerId() == 0 ) 
#else
  if( pcSPS->getLayerId() == 0 || pcSPS->getUpdateRepFormatFlag() ) 
#endif
#endif
  {
#endif
    WRITE_UVLC( pcSPS->getChromaFormatIdc (),         "chroma_format_idc" );
    assert(pcSPS->getChromaFormatIdc () == 1);
    // in the first version chroma_format_idc can only be equal to 1 (4:2:0)
    if( pcSPS->getChromaFormatIdc () == 3 )
    {
      WRITE_FLAG( 0,                                  "separate_colour_plane_flag");
    }

    WRITE_UVLC( pcSPS->getPicWidthInLumaSamples (),   "pic_width_in_luma_samples" );
    WRITE_UVLC( pcSPS->getPicHeightInLumaSamples(),   "pic_height_in_luma_samples" );
#if REPN_FORMAT_IN_VPS
  }
#if !R0042_PROFILE_INDICATION
#if O0096_REP_FORMAT_INDEX
  else if (pcSPS->getUpdateRepFormatFlag())
  {
    WRITE_CODE( pcSPS->getUpdateRepFormatIndex(), 8,   "update_rep_format_index");
  }
#endif
#endif
#endif

#if R0156_CONF_WINDOW_IN_REP_FORMAT
#if REPN_FORMAT_IN_VPS
#if !R0042_PROFILE_INDICATION
#if O0096_REP_FORMAT_INDEX
  if( pcSPS->getLayerId() == 0 ) 
#else
  if( pcSPS->getLayerId() == 0 || pcSPS->getUpdateRepFormatFlag() ) 
#endif  
#endif
  {
#endif
#endif
    Window conf = pcSPS->getConformanceWindow();

    WRITE_FLAG( conf.getWindowEnabledFlag(),          "conformance_window_flag" );
    if (conf.getWindowEnabledFlag())
    {
#if REPN_FORMAT_IN_VPS
      WRITE_UVLC( conf.getWindowLeftOffset(),   "conf_win_left_offset"   );
      WRITE_UVLC( conf.getWindowRightOffset(),  "conf_win_right_offset"  );
      WRITE_UVLC( conf.getWindowTopOffset(),    "conf_win_top_offset"    );
      WRITE_UVLC( conf.getWindowBottomOffset(), "conf_win_bottom_offset" );
#else
      WRITE_UVLC( conf.getWindowLeftOffset()   / TComSPS::getWinUnitX(pcSPS->getChromaFormatIdc() ), "conf_win_left_offset" );
      WRITE_UVLC( conf.getWindowRightOffset()  / TComSPS::getWinUnitX(pcSPS->getChromaFormatIdc() ), "conf_win_right_offset" );
      WRITE_UVLC( conf.getWindowTopOffset()    / TComSPS::getWinUnitY(pcSPS->getChromaFormatIdc() ), "conf_win_top_offset" );
      WRITE_UVLC( conf.getWindowBottomOffset() / TComSPS::getWinUnitY(pcSPS->getChromaFormatIdc() ), "conf_win_bottom_offset" );
#endif
    }
#if R0156_CONF_WINDOW_IN_REP_FORMAT
#if REPN_FORMAT_IN_VPS
  }
#endif
#endif

#if REPN_FORMAT_IN_VPS
#if !R0042_PROFILE_INDICATION
#if O0096_REP_FORMAT_INDEX
  if( pcSPS->getLayerId() == 0 ) 
#else
  if( pcSPS->getLayerId() == 0 || pcSPS->getUpdateRepFormatFlag() ) 
#endif
#endif
  {
    assert( pcSPS->getBitDepthY() >= 8 );
    assert( pcSPS->getBitDepthC() >= 8 );
#endif
    WRITE_UVLC( pcSPS->getBitDepthY() - 8,             "bit_depth_luma_minus8" );
    WRITE_UVLC( pcSPS->getBitDepthC() - 8,             "bit_depth_chroma_minus8" );
#if REPN_FORMAT_IN_VPS
  }
#endif
#if R0042_PROFILE_INDICATION
  }
#endif

  WRITE_UVLC( pcSPS->getBitsForPOC()-4,                 "log2_max_pic_order_cnt_lsb_minus4" );

#if SPS_DPB_PARAMS
#if !R0042_PROFILE_INDICATION
  if( pcSPS->getLayerId() == 0 )
#else
  if(!bMultiLayerExtSpsFlag)
#endif
  {
#endif
  const Bool subLayerOrderingInfoPresentFlag = 1;
  WRITE_FLAG(subLayerOrderingInfoPresentFlag,       "sps_sub_layer_ordering_info_present_flag");
  for(UInt i=0; i <= pcSPS->getMaxTLayers()-1; i++)
  {
    WRITE_UVLC( pcSPS->getMaxDecPicBuffering(i) - 1,       "sps_max_dec_pic_buffering_minus1[i]" );
    WRITE_UVLC( pcSPS->getNumReorderPics(i),               "sps_num_reorder_pics[i]" );
    WRITE_UVLC( pcSPS->getMaxLatencyIncrease(i),           "sps_max_latency_increase_plus1[i]" );
    if (!subLayerOrderingInfoPresentFlag)
    {
      break;
    }
  }
#if SPS_DPB_PARAMS
  }
#endif
  assert( pcSPS->getMaxCUWidth() == pcSPS->getMaxCUHeight() );
  
  WRITE_UVLC( pcSPS->getLog2MinCodingBlockSize() - 3,                                "log2_min_coding_block_size_minus3" );
  WRITE_UVLC( pcSPS->getLog2DiffMaxMinCodingBlockSize(),                             "log2_diff_max_min_coding_block_size" );
  WRITE_UVLC( pcSPS->getQuadtreeTULog2MinSize() - 2,                                 "log2_min_transform_block_size_minus2" );
  WRITE_UVLC( pcSPS->getQuadtreeTULog2MaxSize() - pcSPS->getQuadtreeTULog2MinSize(), "log2_diff_max_min_transform_block_size" );
  WRITE_UVLC( pcSPS->getQuadtreeTUMaxDepthInter() - 1,                               "max_transform_hierarchy_depth_inter" );
  WRITE_UVLC( pcSPS->getQuadtreeTUMaxDepthIntra() - 1,                               "max_transform_hierarchy_depth_intra" );
  WRITE_FLAG( pcSPS->getScalingListFlag() ? 1 : 0,                                   "scaling_list_enabled_flag" ); 
  if(pcSPS->getScalingListFlag())
  {
#if SCALINGLIST_INFERRING
#if !R0042_PROFILE_INDICATION
    if( pcSPS->getLayerId() > 0 )
#else
    if( bMultiLayerExtSpsFlag)
#endif
    {
      WRITE_FLAG( pcSPS->getInferScalingListFlag() ? 1 : 0, "sps_infer_scaling_list_flag" );
    }

    if( pcSPS->getInferScalingListFlag() )
    {
      // The value of pps_scaling_list_ref_layer_id shall be in the range of 0 to 62, inclusive
      assert( pcSPS->getScalingListRefLayerId() <= 62 );

      WRITE_UVLC( pcSPS->getScalingListRefLayerId(), "sps_scaling_list_ref_layer_id" );
    }
    else
    {
#endif
    WRITE_FLAG( pcSPS->getScalingListPresentFlag() ? 1 : 0,                          "sps_scaling_list_data_present_flag" ); 
    if(pcSPS->getScalingListPresentFlag())
    {
      codeScalingList( m_pcSlice->getScalingList() );
    }
#if SCALINGLIST_INFERRING
    }
#endif
  }
  WRITE_FLAG( pcSPS->getUseAMP() ? 1 : 0,                                            "amp_enabled_flag" );
  WRITE_FLAG( pcSPS->getUseSAO() ? 1 : 0,                                            "sample_adaptive_offset_enabled_flag");

  WRITE_FLAG( pcSPS->getUsePCM() ? 1 : 0,                                            "pcm_enabled_flag");
  if( pcSPS->getUsePCM() )
  {
    WRITE_CODE( pcSPS->getPCMBitDepthLuma() - 1, 4,                                  "pcm_sample_bit_depth_luma_minus1" );
    WRITE_CODE( pcSPS->getPCMBitDepthChroma() - 1, 4,                                "pcm_sample_bit_depth_chroma_minus1" );
    WRITE_UVLC( pcSPS->getPCMLog2MinSize() - 3,                                      "log2_min_pcm_luma_coding_block_size_minus3" );
    WRITE_UVLC( pcSPS->getPCMLog2MaxSize() - pcSPS->getPCMLog2MinSize(),             "log2_diff_max_min_pcm_luma_coding_block_size" );
    WRITE_FLAG( pcSPS->getPCMFilterDisableFlag()?1 : 0,                              "pcm_loop_filter_disable_flag");
  }

  assert( pcSPS->getMaxTLayers() > 0 );         

  TComRPSList* rpsList = pcSPS->getRPSList();
  TComReferencePictureSet*      rps;
  
  WRITE_UVLC(rpsList->getNumberOfReferencePictureSets(), "num_short_term_ref_pic_sets" );
  for(Int i=0; i < rpsList->getNumberOfReferencePictureSets(); i++)
  {
    rps = rpsList->getReferencePictureSet(i);
    codeShortTermRefPicSet(pcSPS,rps,false, i);
  }
  WRITE_FLAG( pcSPS->getLongTermRefsPresent() ? 1 : 0,         "long_term_ref_pics_present_flag" );
  if (pcSPS->getLongTermRefsPresent()) 
  {
    WRITE_UVLC(pcSPS->getNumLongTermRefPicSPS(), "num_long_term_ref_pic_sps" );
    for (UInt k = 0; k < pcSPS->getNumLongTermRefPicSPS(); k++)
    {
      WRITE_CODE( pcSPS->getLtRefPicPocLsbSps(k), pcSPS->getBitsForPOC(), "lt_ref_pic_poc_lsb_sps");
      WRITE_FLAG( pcSPS->getUsedByCurrPicLtSPSFlag(k), "used_by_curr_pic_lt_sps_flag");
    }
  }
  WRITE_FLAG( pcSPS->getTMVPFlagsPresent()  ? 1 : 0,           "sps_temporal_mvp_enable_flag" );

  WRITE_FLAG( pcSPS->getUseStrongIntraSmoothing(),             "sps_strong_intra_smoothing_enable_flag" );

  WRITE_FLAG( pcSPS->getVuiParametersPresentFlag(),             "vui_parameters_present_flag" );
  if (pcSPS->getVuiParametersPresentFlag())
  {
      codeVUI(pcSPS->getVuiParameters(), pcSPS);
  }

#if SVC_EXTENSION
#if !R0042_PROFILE_INDICATION
  WRITE_FLAG( pcSPS->getExtensionFlag() ? 1 : 0, "sps_extension_flag" );
#else
  WRITE_FLAG( pcSPS->getExtensionFlag() ? 1 : 0, "sps_extension_present_flag" );
#endif

  if( pcSPS->getExtensionFlag() )
  {
#if !R0042_PROFILE_INDICATION
#if O0142_CONDITIONAL_SPS_EXTENSION
    UInt spsExtensionTypeFlag[8] = { 0, 1, 0, 0, 0, 0, 0, 0 };
    for (UInt i = 0; i < 8; i++)
    {
      WRITE_FLAG( spsExtensionTypeFlag[i], "sps_extension_type_flag" );
    }
    if (spsExtensionTypeFlag[1])
    {
      codeSPSExtension( pcSPS );
    }
#else
    codeSPSExtension( pcSPS );
    WRITE_FLAG( 0, "sps_extension2_flag" );
#endif
#else
    WRITE_FLAG( 0, "sps_range_extension_flag" );
    WRITE_FLAG( 1, "sps_multilayer_extension_flag" );
    WRITE_CODE( 0, 6,  "sps_extension_6bits" );   
    codeSPSExtension( pcSPS ); //it is sps_multilayer_extension
#endif
  }
#else
  WRITE_FLAG( 0, "sps_extension_flag" );
#endif
}

#if SVC_EXTENSION
Void TEncCavlc::codeSPSExtension( TComSPS* pcSPS )
{
  // more syntax elements to be written here

  // Vertical MV component restriction is not used in SHVC CTC
  WRITE_FLAG( 0, "inter_view_mv_vert_constraint_flag" );

#if !MOVE_SCALED_OFFSET_TO_PPS
  if( pcSPS->getLayerId() > 0 )
  {
    WRITE_UVLC( pcSPS->getNumScaledRefLayerOffsets(),      "num_scaled_ref_layer_offsets" );
    for(Int i = 0; i < pcSPS->getNumScaledRefLayerOffsets(); i++)
    {
      Window scaledWindow = pcSPS->getScaledRefLayerWindow(i);
#if O0098_SCALED_REF_LAYER_ID
      WRITE_CODE( pcSPS->getScaledRefLayerId(i), 6,          "scaled_ref_layer_id" );
#endif
      WRITE_SVLC( scaledWindow.getWindowLeftOffset()   >> 1, "scaled_ref_layer_left_offset" );
      WRITE_SVLC( scaledWindow.getWindowTopOffset()    >> 1, "scaled_ref_layer_top_offset" );
      WRITE_SVLC( scaledWindow.getWindowRightOffset()  >> 1, "scaled_ref_layer_right_offset" );
      WRITE_SVLC( scaledWindow.getWindowBottomOffset() >> 1, "scaled_ref_layer_bottom_offset" );
#if P0312_VERT_PHASE_ADJ
      WRITE_FLAG( scaledWindow.getVertPhasePositionEnableFlag(), "vert_phase_position_enable_flag" ); 
#endif
    }
  }
#endif
}
#endif //SVC_EXTENSION

Void TEncCavlc::codeVPS( TComVPS* pcVPS )
{
#if !P0125_REVERT_VPS_EXTN_OFFSET_TO_RESERVED
#if VPS_EXTN_OFFSET_CALC
  UInt numBytesInVps = this->m_pcBitIf->getNumberOfWrittenBits();
#endif
#endif
#if !P0307_REMOVE_VPS_VUI_OFFSET
#if VPS_VUI_OFFSET
   m_vpsVuiCounter = this->m_pcBitIf->getNumberOfWrittenBits();
#endif
#endif
  WRITE_CODE( pcVPS->getVPSId(),                    4,        "vps_video_parameter_set_id" );
#if VPS_RESERVED_FLAGS
  WRITE_FLAG( pcVPS->getBaseLayerInternalFlag(),              "vps_base_layer_internal_flag");
  WRITE_FLAG( pcVPS->getBaseLayerAvailableFlag(),             "vps_base_layer_available_flag");
#else
  WRITE_CODE( 3,                                    2,        "vps_reserved_three_2bits" );
#endif
#if SVC_EXTENSION
  WRITE_CODE( pcVPS->getMaxLayers() - 1,            6,        "vps_max_layers_minus1" );            
#else
  WRITE_CODE( 0,                                    6,        "vps_reserved_zero_6bits" );
#endif
  WRITE_CODE( pcVPS->getMaxTLayers() - 1,           3,        "vps_max_sub_layers_minus1" );
  WRITE_FLAG( pcVPS->getTemporalNestingFlag(),                "vps_temporal_id_nesting_flag" );
  assert (pcVPS->getMaxTLayers()>1||pcVPS->getTemporalNestingFlag());
#if !P0125_REVERT_VPS_EXTN_OFFSET_TO_RESERVED
#if VPS_EXTN_OFFSET
  WRITE_CODE( pcVPS->getExtensionOffset(),         16,        "vps_extension_offset" );
#else
  WRITE_CODE( 0xffff,                              16,        "vps_reserved_ffff_16bits" );
#endif
#else
  WRITE_CODE( 0xffff,                              16,        "vps_reserved_ffff_16bits" );
#endif
  codePTL( pcVPS->getPTL(), true, pcVPS->getMaxTLayers() - 1 );
  const Bool subLayerOrderingInfoPresentFlag = 1;
  WRITE_FLAG(subLayerOrderingInfoPresentFlag,              "vps_sub_layer_ordering_info_present_flag");
  for(UInt i=0; i <= pcVPS->getMaxTLayers()-1; i++)
  {
    WRITE_UVLC( pcVPS->getMaxDecPicBuffering(i) - 1,       "vps_max_dec_pic_buffering_minus1[i]" );
    WRITE_UVLC( pcVPS->getNumReorderPics(i),               "vps_num_reorder_pics[i]" );
    WRITE_UVLC( pcVPS->getMaxLatencyIncrease(i),           "vps_max_latency_increase_plus1[i]" );
    if (!subLayerOrderingInfoPresentFlag)
    {
      break;
    }
  }

#if SVC_EXTENSION
  assert( pcVPS->getNumHrdParameters() <= MAX_VPS_LAYER_SETS_PLUS1 );
  assert( pcVPS->getMaxLayerId() < MAX_VPS_LAYER_ID_PLUS1 );
#if !VPS_EXTN_OP_LAYER_SETS     // num layer sets set in TAppEncTop.cpp
  pcVPS->setNumLayerSets(1);
#endif
  WRITE_CODE( pcVPS->getMaxLayerId(), 6,                       "vps_max_layer_id" );
#if Q0078_ADD_LAYER_SETS
  WRITE_UVLC(pcVPS->getVpsNumLayerSetsMinus1(),                "vps_num_layer_sets_minus1");
  for (UInt opsIdx = 1; opsIdx <= pcVPS->getVpsNumLayerSetsMinus1(); opsIdx++)
#else
  WRITE_UVLC( pcVPS->getNumLayerSets() - 1,                 "vps_num_layer_sets_minus1" );
  for (UInt opsIdx = 1; opsIdx <= (pcVPS->getNumLayerSets() - 1); opsIdx++)
#endif
  {
    // Operation point set
    for( UInt i = 0; i <= pcVPS->getMaxLayerId(); i ++ )
#else
  assert( pcVPS->getNumHrdParameters() <= MAX_VPS_NUM_HRD_PARAMETERS );
  assert( pcVPS->getMaxNuhReservedZeroLayerId() < MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1 );
  WRITE_CODE( pcVPS->getMaxNuhReservedZeroLayerId(), 6,     "vps_max_nuh_reserved_zero_layer_id" );
  pcVPS->setMaxOpSets(1);
  WRITE_UVLC( pcVPS->getMaxOpSets() - 1,                    "vps_max_op_sets_minus1" );
  for( UInt opsIdx = 1; opsIdx <= ( pcVPS->getMaxOpSets() - 1 ); opsIdx ++ )
  {
    // Operation point set
    for( UInt i = 0; i <= pcVPS->getMaxNuhReservedZeroLayerId(); i ++ )
#endif
    {
#if !VPS_EXTN_OP_LAYER_SETS     // layer Id include flag set in TAppEncTop.cpp
      // Only applicable for version 1
      pcVPS->setLayerIdIncludedFlag( true, opsIdx, i );
#endif
      WRITE_FLAG( pcVPS->getLayerIdIncludedFlag( opsIdx, i ) ? 1 : 0, "layer_id_included_flag[opsIdx][i]" );
    }
  }
#if !NECESSARY_FLAG   // Already called once in TAppEncTop.cpp
#if DERIVE_LAYER_ID_LIST_VARIABLES
  pcVPS->deriveLayerIdListVariables();
#endif
#endif
  TimingInfo *timingInfo = pcVPS->getTimingInfo();
  WRITE_FLAG(timingInfo->getTimingInfoPresentFlag(),          "vps_timing_info_present_flag");
  if(timingInfo->getTimingInfoPresentFlag())
  {
    WRITE_CODE(timingInfo->getNumUnitsInTick(), 32,           "vps_num_units_in_tick");
    WRITE_CODE(timingInfo->getTimeScale(),      32,           "vps_time_scale");
    WRITE_FLAG(timingInfo->getPocProportionalToTimingFlag(),  "vps_poc_proportional_to_timing_flag");
    if(timingInfo->getPocProportionalToTimingFlag())
    {
      WRITE_UVLC(timingInfo->getNumTicksPocDiffOneMinus1(),   "vps_num_ticks_poc_diff_one_minus1");
    }
    pcVPS->setNumHrdParameters( 0 );
    WRITE_UVLC( pcVPS->getNumHrdParameters(),                 "vps_num_hrd_parameters" );

    if( pcVPS->getNumHrdParameters() > 0 )
    {
      pcVPS->createHrdParamBuffer();
    }
    for( UInt i = 0; i < pcVPS->getNumHrdParameters(); i ++ )
    {
      // Only applicable for version 1
      pcVPS->setHrdOpSetIdx( 0, i );
      WRITE_UVLC( pcVPS->getHrdOpSetIdx( i ),                "hrd_op_set_idx" );
      if( i > 0 )
      {
        WRITE_FLAG( pcVPS->getCprmsPresentFlag( i ) ? 1 : 0, "cprms_present_flag[i]" );
      }
      codeHrdParameters(pcVPS->getHrdParameters(i), pcVPS->getCprmsPresentFlag( i ), pcVPS->getMaxTLayers() - 1);
    }
  }
#if SVC_EXTENSION
  // When MaxLayersMinus1 is greater than 0, vps_extension_flag shall be equal to 1.
  if( pcVPS->getMaxLayers() > 1 )
  {
    assert( pcVPS->getVpsExtensionFlag() == true );
  }

  WRITE_FLAG( pcVPS->getVpsExtensionFlag() ? 1 : 0,                     "vps_extension_flag" );

  if( pcVPS->getVpsExtensionFlag() )
  {
    while ( m_pcBitIf->getNumberOfWrittenBits() % 8 != 0 )
    {
      WRITE_FLAG(1,                  "vps_extension_alignment_bit_equal_to_one");
    }
#if !P0125_REVERT_VPS_EXTN_OFFSET_TO_RESERVED
#if VPS_EXTN_OFFSET_CALC
    Int vpsExntOffsetValueInBits = this->m_pcBitIf->getNumberOfWrittenBits() - numBytesInVps + 16; // 2 bytes for NUH
    assert( vpsExntOffsetValueInBits % 8 == 0 );
    pcVPS->setExtensionOffset( vpsExntOffsetValueInBits >> 3 );
#endif
#endif
    codeVPSExtension(pcVPS);
    WRITE_FLAG( 0,                     "vps_extension2_flag" );   // Flag value of 1 reserved
  }
#else
  WRITE_FLAG( 0,                     "vps_extension_flag" );
#endif  
  //future extensions here..
  
  return;
}

#if SVC_EXTENSION
Void TEncCavlc::codeVPSExtension (TComVPS *vps)
{
  // ... More syntax elements to be written here
#if P0300_ALT_OUTPUT_LAYER_FLAG
  Int NumOutputLayersInOutputLayerSet[MAX_VPS_LAYER_SETS_PLUS1];
  Int OlsHighestOutputLayerId[MAX_VPS_LAYER_SETS_PLUS1];
#endif
#if LIST_OF_PTL
  if( vps->getMaxLayers() > 1 && vps->getBaseLayerInternalFlag() )
  {
    codePTL( vps->getPTLForExtn(1), false, vps->getMaxTLayers() - 1 );
  }
#endif
#if VPS_EXTN_MASK_AND_DIM_INFO
  UInt i = 0, j = 0;
#if !VPS_AVC_BL_FLAG_REMOVAL
  WRITE_FLAG( vps->getAvcBaseLayerFlag(),              "avc_base_layer_flag" );
#endif
#if !P0307_REMOVE_VPS_VUI_OFFSET
#if O0109_MOVE_VPS_VUI_FLAG
  WRITE_FLAG( 1,                     "vps_vui_present_flag" );
  vps->setVpsVuiPresentFlag(true);
  if ( vps->getVpsVuiPresentFlag() ) 
  {
#if VPS_VUI_OFFSET
    WRITE_CODE( vps->getVpsVuiOffset(  ), 16,             "vps_vui_offset" );
#endif
    WRITE_FLAG( vps->getSplittingFlag(),                 "splitting_flag" );
  }
#else
#if VPS_VUI_OFFSET
  WRITE_CODE( vps->getVpsVuiOffset(  ), 16,             "vps_vui_offset" );  
#endif
  WRITE_FLAG( vps->getSplittingFlag(),                 "splitting_flag" );
#endif // O0109_MOVE_VPS_VUI_FLAG
#endif
  WRITE_FLAG( vps->getSplittingFlag(),                 "splitting_flag" );

  for(i = 0; i < MAX_VPS_NUM_SCALABILITY_TYPES; i++)
  {
    WRITE_FLAG( vps->getScalabilityMask(i),            "scalability_mask[i]" );
  }

  for(j = 0; j < vps->getNumScalabilityTypes() - vps->getSplittingFlag(); j++)
  {
    WRITE_CODE( vps->getDimensionIdLen(j) - 1, 3,      "dimension_id_len_minus1[j]" );
  }

  // The value of dimBitOffset[ NumScalabilityTypes ] is set equal to 6.
  if(vps->getSplittingFlag())
  {
    UInt splDimSum=0;
    for(j = 0; j < vps->getNumScalabilityTypes(); j++)
    {
      splDimSum+=(vps->getDimensionIdLen(j));
    }
    assert(splDimSum<=6);
  }

  WRITE_FLAG( vps->getNuhLayerIdPresentFlag(),         "vps_nuh_layer_id_present_flag" );
  for(i = 1; i < vps->getMaxLayers(); i++)
  {
    if( vps->getNuhLayerIdPresentFlag() )
    {
      WRITE_CODE( vps->getLayerIdInNuh(i),     6,      "layer_id_in_nuh[i]" );
    }

    if( !vps->getSplittingFlag() )
    {
    for(j = 0; j < vps->getNumScalabilityTypes(); j++)
    {
      UInt bits = vps->getDimensionIdLen(j);
      WRITE_CODE( vps->getDimensionId(i, j),   bits,   "dimension_id[i][j]" );
    }
  }
  }
#endif
#if VIEW_ID_RELATED_SIGNALING
  // if ( pcVPS->getNumViews() > 1 )  
  //   However, this is a bug in the text since, view_id_len_minus1 is needed to parse view_id_val. 
  {
#if O0109_VIEW_ID_LEN
    WRITE_CODE( vps->getViewIdLen( ), 4, "view_id_len" );
    assert ( vps->getNumViews() >= (1<<vps->getViewIdLen()) );
#else
    WRITE_CODE( vps->getViewIdLenMinus1( ), 4, "view_id_len_minus1" );
#endif
  }

#if O0109_VIEW_ID_LEN
  if ( vps->getViewIdLen() > 0 )
  {
#endif
  for(  i = 0; i < vps->getNumViews(); i++ )
  {
#if O0109_VIEW_ID_LEN
    WRITE_CODE( vps->getViewIdVal( i ), vps->getViewIdLen( ), "view_id_val[i]" );
#else
    WRITE_CODE( vps->getViewIdVal( i ), vps->getViewIdLenMinus1( ) + 1, "view_id_val[i]" );
#endif
  }
#if O0109_VIEW_ID_LEN
  }
#endif
#endif // VIEW_ID_RELATED_SIGNALING

#if VPS_EXTN_DIRECT_REF_LAYERS
  for( Int layerCtr = 1; layerCtr <= vps->getMaxLayers() - 1; layerCtr++)
  {
    for( Int refLayerCtr = 0; refLayerCtr < layerCtr; refLayerCtr++)
    {
      WRITE_FLAG(vps->getDirectDependencyFlag(layerCtr, refLayerCtr), "direct_dependency_flag[i][j]" );
    }
  }
#endif
#if MOVE_ADDN_LS_SIGNALLING
#if Q0078_ADD_LAYER_SETS
  if (vps->getNumIndependentLayers() > 1)
  {
    WRITE_UVLC( vps->getNumAddLayerSets(), "num_add_layer_sets" );
    for (i = 0; i < vps->getNumAddLayerSets(); i++)
    {
      for (j = 1; j < vps->getNumIndependentLayers(); j++)
      {
        int len = 1;
        while ((1 << len) < (vps->getNumLayersInTreePartition(j) + 1))
        {
          len++;
        }
        WRITE_CODE(vps->getHighestLayerIdxPlus1(i, j), len, "highest_layer_idx_plus1[i][j]");
      }
    }
  }
#endif
#endif
#if VPS_TSLAYERS
    WRITE_FLAG( vps->getMaxTSLayersPresentFlag(), "vps_sub_layers_max_minus1_present_flag");
    if (vps->getMaxTSLayersPresentFlag())
    {
        for( i = 0; i < vps->getMaxLayers(); i++)
        {
            WRITE_CODE(vps->getMaxTSLayersMinus1(i), 3, "sub_layers_vps_max_minus1[i]" );
        }
    }
#endif
   WRITE_FLAG( vps->getMaxTidRefPresentFlag(), "max_tid_ref_present_flag");
   if (vps->getMaxTidRefPresentFlag())
   {
     for( i = 0; i < vps->getMaxLayers() - 1; i++)
     {
#if O0225_MAX_TID_FOR_REF_LAYERS
       for( j = i+1; j <= vps->getMaxLayers() - 1; j++)
       {
         if(vps->getDirectDependencyFlag(j, i))
         {
           WRITE_CODE(vps->getMaxTidIlRefPicsPlus1(i,j), 3, "max_tid_il_ref_pics_plus1[i][j]" );
         }
       }
#else
       WRITE_CODE(vps->getMaxTidIlRefPicsPlus1(i), 3, "max_tid_il_ref_pics_plus1[i]" );
#endif 
     }
   }
   WRITE_FLAG( vps->getIlpSshSignalingEnabledFlag(), "all_ref_layers_active_flag" );
#if VPS_EXTN_PROFILE_INFO
  // Profile-tier-level signalling
#if !VPS_EXTN_UEV_CODING
  WRITE_CODE( vps->getNumLayerSets() - 1   , 10, "vps_number_layer_sets_minus1" );     
  WRITE_CODE( vps->getNumProfileTierLevel() - 1,  6, "vps_num_profile_tier_level_minus1"); 
#else
  WRITE_UVLC( vps->getNumProfileTierLevel() - 1, "vps_num_profile_tier_level_minus1"); 
#if PER_LAYER_PTL
  Int const numBitsForPtlIdx = vps->calculateLenOfSyntaxElement( vps->getNumProfileTierLevel() );
#endif
#endif
#if LIST_OF_PTL
  assert( vps->getNumProfileTierLevel() == vps->getPTLForExtnPtr()->size());
  for(Int idx = vps->getBaseLayerInternalFlag() ? 2 : 1; idx <= vps->getNumProfileTierLevel() - 1; idx++)
#else
  for(Int idx = 1; idx <= vps->getNumProfileTierLevel() - 1; idx++)
#endif
  {
    WRITE_FLAG( vps->getProfilePresentFlag(idx),       "vps_profile_present_flag[i]" );
#if !P0048_REMOVE_PROFILE_REF
    if( !vps->getProfilePresentFlag(idx) )
    {
      WRITE_CODE( vps->getProfileLayerSetRef(idx) - 1, 6, "profile_ref_minus1[i]" );
    }
#endif
    codePTL( vps->getPTLForExtn(idx), vps->getProfilePresentFlag(idx), vps->getMaxTLayers() - 1 );
  }
#endif


#if !MOVE_ADDN_LS_SIGNALLING
#if Q0078_ADD_LAYER_SETS
  if (vps->getNumIndependentLayers() > 1)
  {
    WRITE_UVLC( vps->getNumAddLayerSets(), "num_add_layer_sets" );
    for (i = 0; i < vps->getNumAddLayerSets(); i++)
    {
      for (j = 1; j < vps->getNumIndependentLayers(); j++)
      {
        int len = 1;
        while ((1 << len) < (vps->getNumLayersInTreePartition(j) + 1))
        {
          len++;
        }
        WRITE_CODE(vps->getHighestLayerIdxPlus1(i, j), len, "highest_layer_idx_plus1[i][j]");
      }
    }
  }
#endif
#endif

#if !VPS_EXTN_UEV_CODING
  Int numOutputLayerSets = vps->getNumOutputLayerSets() ;
  WRITE_FLAG(  (numOutputLayerSets > vps->getNumLayerSets()), "more_output_layer_sets_than_default_flag" ); 
  if(numOutputLayerSets > vps->getNumLayerSets())
  {
    WRITE_CODE( numOutputLayerSets - vps->getNumLayerSets(), 10, "num_add_output_layer_sets" );
  }
#else
  Int numOutputLayerSets = vps->getNumOutputLayerSets();
  Int numAddOutputLayerSets = numOutputLayerSets - (Int)vps->getNumLayerSets();

  // The value of num_add_olss shall be in the range of 0 to 1023, inclusive.
  assert( numAddOutputLayerSets >= 0 && numAddOutputLayerSets < 1024 );

#if Q0165_NUM_ADD_OUTPUT_LAYER_SETS
  if( vps->getNumLayerSets() > 1 )
  {
    WRITE_UVLC( numAddOutputLayerSets, "num_add_olss" );
    WRITE_CODE( vps->getDefaultTargetOutputLayerIdc(), 2, "default_output_layer_idc" );
  }
#else
  WRITE_UVLC( numOutputLayerSets - vps->getNumLayerSets(), "num_add_output_layer_sets" );
#endif
#endif

#if !Q0165_NUM_ADD_OUTPUT_LAYER_SETS
  if( numOutputLayerSets > 1 )
  {
#if P0295_DEFAULT_OUT_LAYER_IDC
    WRITE_CODE( vps->getDefaultTargetOutputLayerIdc(), 2, "default_target_output_layer_idc" );   
#else
#if O0109_DEFAULT_ONE_OUT_LAYER_IDC
    WRITE_CODE( vps->getDefaultOneTargetOutputLayerIdc(), 2, "default_one_target_output_layer_idc" );   
#else
    WRITE_FLAG( vps->getDefaultOneTargetOutputLayerFlag(), "default_one_target_output_layer_flag" );   
#endif
#endif
  }
#endif

  for(i = 1; i < numOutputLayerSets; i++)
  {
    Int layerSetIdxForOutputLayerSet = vps->getOutputLayerSetIdx(i);
    if( i > (vps->getNumLayerSets() - 1) )
    {
      Int numBits = 1;
      while ((1 << numBits) < (vps->getNumLayerSets() - 1))
      {
        numBits++;
      }
      WRITE_CODE( vps->getOutputLayerSetIdx(i) - 1, numBits, "layer_set_idx_for_ols_minus1"); 
#if P0295_DEFAULT_OUT_LAYER_IDC
    }
#if Q0078_ADD_LAYER_SETS
    if ( i > vps->getVpsNumLayerSetsMinus1() || vps->getDefaultTargetOutputLayerIdc() >= 2 ) //Instead of == 2, >= 2 is used to follow the agreement that value 3 should be interpreted as 2
#else
    if ( i > (vps->getNumLayerSets() - 1) || vps->getDefaultTargetOutputLayerIdc() >= 2 ) //Instead of == 2, >= 2 is used to follow the agreement that value 3 should be interpreted as 2
#endif
    {
#endif
#if NUM_OL_FLAGS
      for(j = 0; j < vps->getNumLayersInIdList(layerSetIdxForOutputLayerSet) ; j++)
#else
      for(j = 0; j < vps->getNumLayersInIdList(lsIdx) - 1; j++)
#endif
      {
        WRITE_FLAG( vps->getOutputLayerFlag(i,j), "output_layer_flag[i][j]");
      }
    }
#if PER_LAYER_PTL
    for(j = 0; j < vps->getNumLayersInIdList(layerSetIdxForOutputLayerSet) ; j++)
    {
      if( vps->getNecessaryLayerFlag(i, j) )
      {
        WRITE_CODE( vps->getProfileLevelTierIdx(i, j), numBitsForPtlIdx, "profile_level_tier_idx[i]" );
      }
    }
#else
    Int numBits = 1;
    while ((1 << numBits) < (vps->getNumProfileTierLevel()))
    {
      numBits++;
    }
    WRITE_CODE( vps->getProfileLevelTierIdx(i), numBits, "profile_level_tier_idx[i]" );     
#endif
#if P0300_ALT_OUTPUT_LAYER_FLAG
    NumOutputLayersInOutputLayerSet[i] = 0;
    for (j = 0; j < vps->getNumLayersInIdList(layerSetIdxForOutputLayerSet); j++)
    {
      NumOutputLayersInOutputLayerSet[i] += vps->getOutputLayerFlag(i, j);
      if (vps->getOutputLayerFlag(i, j))
      {
        OlsHighestOutputLayerId[i] = vps->getLayerSetLayerIdList(layerSetIdxForOutputLayerSet, j);
      }
    }
    if (NumOutputLayersInOutputLayerSet[i] == 1 && vps->getNumDirectRefLayers(OlsHighestOutputLayerId[i]) > 0)
    {
      WRITE_FLAG(vps->getAltOuputLayerFlag(i), "alt_output_layer_flag[i]");
    }

#if Q0165_OUTPUT_LAYER_SET
    assert( NumOutputLayersInOutputLayerSet[i]>0 );
#endif

#endif
  }

#if !P0300_ALT_OUTPUT_LAYER_FLAG
#if O0153_ALT_OUTPUT_LAYER_FLAG
  if( vps->getMaxLayers() > 1 )
  {
    WRITE_FLAG( vps->getAltOuputLayerFlag(), "alt_output_layer_flag" );   
  }
#endif
#endif

#if REPN_FORMAT_IN_VPS
#if Q0195_REP_FORMAT_CLEANUP  
  // The value of vps_num_rep_formats_minus1 shall be in the range of 0 to 255, inclusive.
  assert( vps->getVpsNumRepFormats() > 0 && vps->getVpsNumRepFormats() <= 256 );
  
  WRITE_UVLC( vps->getVpsNumRepFormats() - 1, "vps_num_rep_formats_minus1" );

  for(i = 0; i < vps->getVpsNumRepFormats(); i++)
  {
    // Write rep_format_structures
    codeRepFormat( vps->getVpsRepFormat(i) );
  }

  if( vps->getVpsNumRepFormats() > 1 )
  {
    WRITE_FLAG( vps->getRepFormatIdxPresentFlag(), "rep_format_idx_present_flag"); 
  }
  else
  {
    // When not present, the value of rep_format_idx_present_flag is inferred to be equal to 0
    assert( !vps->getRepFormatIdxPresentFlag() );
  }

  if( vps->getRepFormatIdxPresentFlag() )
  {
    for(i = 1; i < vps->getMaxLayers(); i++)
    {
      Int numBits = 1;
      while ((1 << numBits) < (vps->getVpsNumRepFormats()))
      {
        numBits++;
      }
      WRITE_CODE( vps->getVpsRepFormatIdx(i), numBits, "vps_rep_format_idx[i]" );
    }
  }
#else
  WRITE_FLAG( vps->getRepFormatIdxPresentFlag(), "rep_format_idx_present_flag"); 

  if( vps->getRepFormatIdxPresentFlag() )
  {
    // The value of vps_num_rep_formats_minus1 shall be in the range of 0 to 255, inclusive.
    assert( vps->getVpsNumRepFormats() > 0 && vps->getVpsNumRepFormats() <= 256 );

#if O0096_REP_FORMAT_INDEX
#if !VPS_EXTN_UEV_CODING
    WRITE_CODE( vps->getVpsNumRepFormats() - 1, 8, "vps_num_rep_formats_minus1" );
#else
    WRITE_UVLC( vps->getVpsNumRepFormats() - 1, "vps_num_rep_formats_minus1" );
#endif
#else
    WRITE_CODE( vps->getVpsNumRepFormats() - 1, 4, "vps_num_rep_formats_minus1" );
#endif
  }
  for(i = 0; i < vps->getVpsNumRepFormats(); i++)
  {
    // Read rep_format_structures
    codeRepFormat( vps->getVpsRepFormat(i) );
  }
  
  if( vps->getRepFormatIdxPresentFlag() )
  {
    for(i = 1; i < vps->getMaxLayers(); i++)
    {
      if( vps->getVpsNumRepFormats() > 1 )
      {
#if O0096_REP_FORMAT_INDEX
#if !VPS_EXTN_UEV_CODING
        WRITE_CODE( vps->getVpsRepFormatIdx(i), 8, "vps_rep_format_idx[i]" );
#else
        Int numBits = 1;
        while ((1 << numBits) < (vps->getVpsNumRepFormats()))
        {
          numBits++;
        }
        WRITE_CODE( vps->getVpsRepFormatIdx(i), numBits, "vps_rep_format_idx[i]" );
#endif
#else
        WRITE_CODE( vps->getVpsRepFormatIdx(i), 4, "vps_rep_format_idx[i]" );
#endif
      }
    }
  }
#endif
#endif

  WRITE_FLAG(vps->getMaxOneActiveRefLayerFlag(), "max_one_active_ref_layer_flag");
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  WRITE_FLAG(vps->getVpsPocLsbAlignedFlag(), "vps_poc_lsb_aligned_flag");
#endif
#if O0062_POC_LSB_NOT_PRESENT_FLAG
  for(i = 1; i< vps->getMaxLayers(); i++)
  {
    if( vps->getNumDirectRefLayers( vps->getLayerIdInNuh(i) ) == 0  )
    {
      WRITE_FLAG(vps->getPocLsbNotPresentFlag(i), "poc_lsb_not_present_flag[i]");
    }
  }
#endif
#if O0215_PHASE_ALIGNMENT
  WRITE_FLAG(vps->getPhaseAlignFlag(), "cross_layer_phase_alignment_flag" );
#endif
#if !IRAP_ALIGN_FLAG_IN_VPS_VUI
  WRITE_FLAG(vps->getCrossLayerIrapAlignFlag(), "cross_layer_irap_aligned_flag");
#endif 
#if VPS_DPB_SIZE_TABLE
  codeVpsDpbSizeTable(vps);
#endif
#if VPS_EXTN_DIRECT_REF_LAYERS
  WRITE_UVLC( vps->getDirectDepTypeLen()-2,                           "direct_dep_type_len_minus2");
#if O0096_DEFAULT_DEPENDENCY_TYPE
  WRITE_FLAG(vps->getDefaultDirectDependencyTypeFlag(), "default_direct_dependency_flag");
  if (vps->getDefaultDirectDependencyTypeFlag())
  {
    WRITE_CODE( vps->getDefaultDirectDependencyType(), vps->getDirectDepTypeLen(), "default_direct_dependency_type" );
  }
  else
  {
    for(i = 1; i < vps->getMaxLayers(); i++)
    {
      for(j = 0; j < i; j++)
      {
        if (vps->getDirectDependencyFlag(i, j))
        {
          WRITE_CODE( vps->getDirectDependencyType(i, j), vps->getDirectDepTypeLen(), "direct_dependency_type[i][j]" );
        }
      }
    }
  }
#else
  for(i = 1; i < vps->getMaxLayers(); i++)
  {
    for(j = 0; j < i; j++)
    {
      if (vps->getDirectDependencyFlag(i, j))
      {
        WRITE_CODE( vps->getDirectDependencyType(i, j), vps->getDirectDepTypeLen(), "direct_dependency_type[i][j]" );
      }
    }
  }
#endif
#endif

#if !O0109_O0199_FLAGS_TO_VUI
#if M0040_ADAPTIVE_RESOLUTION_CHANGE
  WRITE_FLAG(vps->getSingleLayerForNonIrapFlag(), "single_layer_for_non_irap_flag" );
#endif
#if HIGHER_LAYER_IRAP_SKIP_FLAG
  WRITE_FLAG(vps->getHigherLayerIrapSkipFlag(), "higher_layer_irap_skip_flag" );
#endif
#endif

#if P0307_VPS_NON_VUI_EXTENSION
  // The value of vps_non_vui_extension_length shall be in the range of 0 to 4096, inclusive.
  assert( vps->getVpsNonVuiExtLength() >= 0 && vps->getVpsNonVuiExtLength() <= 4096 );

  WRITE_UVLC( vps->getVpsNonVuiExtLength(), "vps_non_vui_extension_length" );
#if P0307_VPS_NON_VUI_EXT_UPDATE
  for (i = 1; i <= vps->getVpsNonVuiExtLength(); i++)
  {
    WRITE_CODE(1, 8, "vps_non_vui_extension_data_byte");
  }
#else
  if ( vps->getVpsNonVuiExtLength() > 0 )
  {
    printf("\n\nUp to the current spec, the value of vps_non_vui_extension_length is supposed to be 0\n");
  }
#endif
#endif

#if !O0109_MOVE_VPS_VUI_FLAG
  WRITE_FLAG( 1,                     "vps_vui_present_flag" );
  if(1)   // Should be conditioned on the value of vps_vui_present_flag
  {
    while ( m_pcBitIf->getNumberOfWrittenBits() % 8 != 0 )
    {
      WRITE_FLAG(1,                  "vps_vui_alignment_bit_equal_to_one");
    }
#if VPS_VUI_OFFSET
    Int vpsVuiOffsetValeInBits = this->m_pcBitIf->getNumberOfWrittenBits() - m_vpsVuiCounter + 16; // 2 bytes for NUH
    assert( vpsVuiOffsetValeInBits % 8 == 0 );
    vps->setVpsVuiOffset( vpsVuiOffsetValeInBits >> 3 );
#endif
    codeVPSVUI(vps);  
  }
#else
#if P0307_REMOVE_VPS_VUI_OFFSET
  vps->setVpsVuiPresentFlag(true);
  WRITE_FLAG( vps->getVpsVuiPresentFlag() ? 1 : 0,                     "vps_vui_present_flag" );
#endif
  if(vps->getVpsVuiPresentFlag())   // Should be conditioned on the value of vps_vui_present_flag
  {
    while ( m_pcBitIf->getNumberOfWrittenBits() % 8 != 0 )
    {
      WRITE_FLAG(1,                  "vps_vui_alignment_bit_equal_to_one");
    }
#if !P0307_REMOVE_VPS_VUI_OFFSET
#if VPS_VUI_OFFSET
    Int vpsVuiOffsetValeInBits = this->m_pcBitIf->getNumberOfWrittenBits() - m_vpsVuiCounter + 16; // 2 bytes for NUH
    assert( vpsVuiOffsetValeInBits % 8 == 0 );
    vps->setVpsVuiOffset( vpsVuiOffsetValeInBits >> 3 );
#endif
#endif
    codeVPSVUI(vps);  
  }
#endif // 0109_MOVE_VPS_FLAG
}

#if REPN_FORMAT_IN_VPS
Void  TEncCavlc::codeRepFormat( RepFormat *repFormat )
{
#if REPN_FORMAT_CONTROL_FLAG
  WRITE_CODE( repFormat->getPicWidthVpsInLumaSamples (), 16, "pic_width_vps_in_luma_samples" );    
  WRITE_CODE( repFormat->getPicHeightVpsInLumaSamples(), 16, "pic_height_vps_in_luma_samples" );  
  WRITE_FLAG( repFormat->getChromaAndBitDepthVpsPresentFlag(), "chroma_and_bit_depth_vps_present_flag" );

  if( repFormat->getChromaAndBitDepthVpsPresentFlag() )
  {
    WRITE_CODE( repFormat->getChromaFormatVpsIdc(), 2, "chroma_format_vps_idc" );   

    if( repFormat->getChromaFormatVpsIdc() == 3 )
    {
      WRITE_FLAG( repFormat->getSeparateColourPlaneVpsFlag(), "separate_colour_plane_vps_flag" );      
    }

    assert( repFormat->getBitDepthVpsLuma() >= 8 );
    assert( repFormat->getBitDepthVpsChroma() >= 8 );
    WRITE_CODE( repFormat->getBitDepthVpsLuma() - 8,   4, "bit_depth_vps_luma_minus8" );           
    WRITE_CODE( repFormat->getBitDepthVpsChroma() - 8, 4, "bit_depth_vps_chroma_minus8" );
  }
#else 
  WRITE_CODE( repFormat->getChromaFormatVpsIdc(), 2, "chroma_format_idc" );    
  
  if( repFormat->getChromaFormatVpsIdc() == 3 )
  {
    WRITE_FLAG( repFormat->getSeparateColourPlaneVpsFlag(), "separate_colour_plane_flag");      
  }

  WRITE_CODE ( repFormat->getPicWidthVpsInLumaSamples (), 16, "pic_width_in_luma_samples" );    
  WRITE_CODE ( repFormat->getPicHeightVpsInLumaSamples(), 16, "pic_height_in_luma_samples" );    
  
  assert( repFormat->getBitDepthVpsLuma() >= 8 );
  assert( repFormat->getBitDepthVpsChroma() >= 8 );
  WRITE_CODE( repFormat->getBitDepthVpsLuma() - 8,   4, "bit_depth_luma_minus8" );           
  WRITE_CODE( repFormat->getBitDepthVpsChroma() - 8, 4, "bit_depth_chroma_minus8" );
#endif

#if R0156_CONF_WINDOW_IN_REP_FORMAT
  Window conf = repFormat->getConformanceWindowVps();

  WRITE_FLAG( conf.getWindowEnabledFlag(),    "conformance_window_vps_flag" );
  if (conf.getWindowEnabledFlag())
  {
    WRITE_UVLC( conf.getWindowLeftOffset(),   "conf_win_vps_left_offset"   );
    WRITE_UVLC( conf.getWindowRightOffset(),  "conf_win_vps_right_offset"  );
    WRITE_UVLC( conf.getWindowTopOffset(),    "conf_win_vps_top_offset"    );
    WRITE_UVLC( conf.getWindowBottomOffset(), "conf_win_vps_bottom_offset" );
  }
#endif
}
#endif
#if VPS_DPB_SIZE_TABLE
Void TEncCavlc::codeVpsDpbSizeTable(TComVPS *vps)
{
#if !SUB_LAYERS_IN_LAYER_SET  // MaxSLInLayerSets calculated earlier in the encoder
#if DPB_PARAMS_MAXTLAYERS
#if BITRATE_PICRATE_SIGNALLING
    Int * MaxSubLayersInLayerSetMinus1 = new Int[vps->getNumLayerSets()];
    for(Int i = 0; i < vps->getNumLayerSets(); i++)
#else
    Int * MaxSubLayersInLayerSetMinus1 = new Int[vps->getNumOutputLayerSets()];
    for(Int i = 1; i < vps->getNumOutputLayerSets(); i++)
#endif
    {
        UInt maxSLMinus1 = 0;
#if CHANGE_NUMSUBDPB_IDX
        Int optLsIdx = vps->getOutputLayerSetIdx( i );
#else
        Int optLsIdx = i;
#endif
#if BITRATE_PICRATE_SIGNALLING
        optLsIdx = i;
#endif
        for(Int k = 0; k < vps->getNumLayersInIdList(optLsIdx); k++ ) {
            Int  lId = vps->getLayerSetLayerIdList(optLsIdx, k);
            maxSLMinus1 = max(maxSLMinus1, vps->getMaxTSLayersMinus1(vps->getLayerIdInVps(lId)));
        }
        MaxSubLayersInLayerSetMinus1[ i ] = maxSLMinus1;
#if BITRATE_PICRATE_SIGNALLING
        vps->setMaxSLayersInLayerSetMinus1(i,MaxSubLayersInLayerSetMinus1[ i ]);
#endif
    }
#endif
#endif
    
    
  for(Int i = 1; i < vps->getNumOutputLayerSets(); i++)
  {
#if CHANGE_NUMSUBDPB_IDX
    Int layerSetIdxForOutputLayerSet = vps->getOutputLayerSetIdx( i );
#endif
    WRITE_FLAG( vps->getSubLayerFlagInfoPresentFlag( i ), "sub_layer_flag_info_present_flag[i]");
#if SUB_LAYERS_IN_LAYER_SET
    for(Int j = 0; j <= vps->getMaxSLayersInLayerSetMinus1( layerSetIdxForOutputLayerSet ); j++)
#else
#if DPB_PARAMS_MAXTLAYERS
#if BITRATE_PICRATE_SIGNALLING
    for(Int j = 0; j <= MaxSubLayersInLayerSetMinus1[ vps->getOutputLayerSetIdx( i ) ]; j++)
#else
    for(Int j = 0; j <= MaxSubLayersInLayerSetMinus1[ i ]; j++)
#endif
#else
    for(Int j = 0; j < vps->getMaxTLayers(); j++)
#endif
#endif
    {
      if( j > 0 && vps->getSubLayerFlagInfoPresentFlag(i) )
      {
        WRITE_FLAG( vps->getSubLayerDpbInfoPresentFlag( i, j), "sub_layer_dpb_info_present_flag[i]");  
      }
      if( vps->getSubLayerDpbInfoPresentFlag(i, j) )
      {
#if CHANGE_NUMSUBDPB_IDX
#if RESOLUTION_BASED_DPB
        for(Int k = 0; k < vps->getNumSubDpbs(layerSetIdxForOutputLayerSet); k++)
#else
        for(Int k = 0; k < vps->getNumLayersInIdList( layerSetIdxForOutputLayerSet ); k++)
#endif
#else
        for(Int k = 0; k < vps->getNumSubDpbs(i); k++)
#endif
        {
#if DPB_INTERNAL_BL_SIG
        if(vps->getBaseLayerInternalFlag()  || ( vps->getLayerSetLayerIdList(layerSetIdxForOutputLayerSet, k)   !=  0 ) )
#endif
          WRITE_UVLC( vps->getMaxVpsDecPicBufferingMinus1( i, k, j), "max_vps_dec_pic_buffering_minus1[i][k][j]" );
        }
        WRITE_UVLC( vps->getMaxVpsNumReorderPics( i, j), "max_vps_num_reorder_pics[i][j]" );              
#if RESOLUTION_BASED_DPB
        if( vps->getNumSubDpbs(layerSetIdxForOutputLayerSet) != vps->getNumLayersInIdList( layerSetIdxForOutputLayerSet ) )  // NumSubDpbs
        {
          for(Int k = 0; k < vps->getNumLayersInIdList( layerSetIdxForOutputLayerSet ); k++)
          {
            WRITE_UVLC( vps->getMaxVpsLayerDecPicBuffMinus1( i, k, j), "max_vps_layer_dec_pic_buff_minus1[i][k][j]" );
          }
        }
#endif
        WRITE_UVLC( vps->getMaxVpsLatencyIncreasePlus1( i, j), "max_vps_latency_increase_plus1[i][j]" );        
      }
    }
  }

#if !SUB_LAYERS_IN_LAYER_SET
#if BITRATE_PICRATE_SIGNALLING
  if( MaxSubLayersInLayerSetMinus1 )
  {
    delete [] MaxSubLayersInLayerSetMinus1;
  }
#endif
#endif
}
#endif

Void TEncCavlc::codeVPSVUI (TComVPS *vps)
{
  Int i,j;
#if O0223_PICTURE_TYPES_ALIGN_FLAG
  WRITE_FLAG(vps->getCrossLayerPictureTypeAlignFlag(), "cross_layer_pic_type_aligned_flag");
  if (!vps->getCrossLayerPictureTypeAlignFlag())
  {
#endif 
#if IRAP_ALIGN_FLAG_IN_VPS_VUI
    WRITE_FLAG(vps->getCrossLayerIrapAlignFlag(), "cross_layer_irap_aligned_flag");
#if P0068_CROSS_LAYER_ALIGNED_IDR_ONLY_FOR_IRAP_FLAG
    if(vps->getCrossLayerIrapAlignFlag())
    {
       WRITE_FLAG(vps->getCrossLayerAlignedIdrOnlyFlag(), "all_layers_idr_aligned_flag");
    }
#endif
#endif 
#if O0223_PICTURE_TYPES_ALIGN_FLAG
  }
#endif
  WRITE_FLAG( vps->getBitRatePresentVpsFlag(),        "bit_rate_present_vps_flag" );
  WRITE_FLAG( vps->getPicRatePresentVpsFlag(),        "pic_rate_present_vps_flag" );

  if( vps->getBitRatePresentVpsFlag() || vps->getPicRatePresentVpsFlag() )
  {
#if Q0078_ADD_LAYER_SETS
#if R0227_BR_PR_ADD_LAYER_SET
#if SIGNALLING_BITRATE_PICRATE_FIX
    for( i = vps->getBaseLayerInternalFlag() ? 0 : 1; i < vps->getNumLayerSets(); i++ )
#else
    for( i = 0; i < vps->getNumLayerSets(); i++ )
#endif
#else
    for( i = 0; i <= vps->getVpsNumLayerSetsMinus1(); i++ )
#endif
#else
    for( i = 0; i < vps->getNumLayerSets(); i++ )
#endif
    {
#if BITRATE_PICRATE_SIGNALLING
      for( j = 0; j <= vps->getMaxSLayersInLayerSetMinus1(i); j++ )
#else
      for( j = 0; j < vps->getMaxTLayers(); j++ )
#endif
      {
        if( vps->getBitRatePresentVpsFlag() )
        {
          WRITE_FLAG( vps->getBitRatePresentFlag( i, j),        "bit_rate_present_flag[i][j]" );
        }
        if( vps->getPicRatePresentVpsFlag() )
        {
          WRITE_FLAG( vps->getPicRatePresentFlag( i, j),        "pic_rate_present_flag[i][j]" );
        }
        if( vps->getBitRatePresentFlag(i, j) )
        {
          WRITE_CODE( vps->getAvgBitRate( i, j ), 16, "avg_bit_rate[i][j]" );
          WRITE_CODE( vps->getAvgBitRate( i, j ), 16, "max_bit_rate[i][j]" );
        }
        if( vps->getPicRatePresentFlag(i, j) )
        {
          WRITE_CODE( vps->getConstPicRateIdc( i, j), 2 , "constant_pic_rate_idc[i][j]" ); 
          WRITE_CODE( vps->getConstPicRateIdc( i, j), 16, "avg_pic_rate[i][j]"          ); 
        }
      }
    }
  }
#if VPS_VUI_VIDEO_SIGNAL_MOVE
  WRITE_FLAG( vps->getVideoSigPresentVpsFlag(), "video_signal_info_idx_present_flag" );
  if (vps->getVideoSigPresentVpsFlag())
  {
    WRITE_CODE(vps->getNumVideoSignalInfo()-1, 4, "vps_num_video_signal_info_minus1" );
  }

  for(i = 0; i < vps->getNumVideoSignalInfo(); i++)
  {
    WRITE_CODE(vps->getVideoVPSFormat(i), 3, "video_vps_format" );
    WRITE_FLAG(vps->getVideoFullRangeVpsFlag(i), "video_full_range_vps_flag" );
    WRITE_CODE(vps->getColorPrimaries(i), 8, "color_primaries_vps" );
    WRITE_CODE(vps->getTransCharacter(i), 8, "transfer_characteristics_vps" );
    WRITE_CODE(vps->getMaxtrixCoeff(i), 8, "matrix_coeffs_vps" );
  }

  if (vps->getVideoSigPresentVpsFlag() && vps->getNumVideoSignalInfo() > 1 )
  {
#if VPS_VUI_VST_PARAMS
    for(i = vps->getBaseLayerInternalFlag() ? 0 : 1; i < vps->getMaxLayers(); i++)
    {
      WRITE_CODE( vps->getVideoSignalInfoIdx(i), 4, "vps_video_signal_info_idx" );
    }
#else
    for (i=1; i < vps->getMaxLayers(); i++)
      WRITE_CODE(vps->getVideoSignalInfoIdx(i), 4, "vps_video_signal_info_idx" );
#endif
  }
#endif 
#if VPS_VUI_TILES_NOT_IN_USE__FLAG
  UInt layerIdx;
  WRITE_FLAG( vps->getTilesNotInUseFlag() ? 1 : 0 , "tiles_not_in_use_flag" );
  if (!vps->getTilesNotInUseFlag())
  {
    for(i = 0; i < vps->getMaxLayers(); i++)
    {
      WRITE_FLAG( vps->getTilesInUseFlag(i) ? 1 : 0 , "tiles_in_use_flag[ i ]" );
      if (vps->getTilesInUseFlag(i))
      {
        WRITE_FLAG( vps->getLoopFilterNotAcrossTilesFlag(i) ? 1 : 0 , "loop_filter_not_across_tiles_flag[ i ]" );
      }
    }
#endif

    for(i = 1; i < vps->getMaxLayers(); i++)
    {
      for(j = 0; j < vps->getNumDirectRefLayers(vps->getLayerIdInNuh(i)); j++)
      {
#if VPS_VUI_TILES_NOT_IN_USE__FLAG
        layerIdx = vps->getLayerIdInVps(vps->getRefLayerId(vps->getLayerIdInNuh(i), j));
        if (vps->getTilesInUseFlag(i) && vps->getTilesInUseFlag(layerIdx)) {
          WRITE_FLAG( vps->getTileBoundariesAlignedFlag(i,j) ? 1 : 0 , "tile_boundaries_aligned_flag[i][j]" );
        }
#else
        WRITE_FLAG( vps->getTileBoundariesAlignedFlag(i,j) ? 1 : 0 , "tile_boundaries_aligned_flag[i][j]" );
#endif
      }
    }  
#if VPS_VUI_TILES_NOT_IN_USE__FLAG
  }
#endif
#if VPS_VUI_WPP_NOT_IN_USE__FLAG
  WRITE_FLAG( vps->getWppNotInUseFlag() ? 1 : 0 , "wpp_not_in_use_flag" );
  if (!vps->getWppNotInUseFlag())
  {
    for(i = 0; i < vps->getMaxLayers(); i++)
    {
      WRITE_FLAG( vps->getWppInUseFlag(i) ? 1 : 0 , "wpp_in_use_flag[ i ]" );
    }
  }
#endif

#if O0109_O0199_FLAGS_TO_VUI
#if M0040_ADAPTIVE_RESOLUTION_CHANGE
  WRITE_FLAG(vps->getSingleLayerForNonIrapFlag(), "single_layer_for_non_irap_flag" );
#endif
#if HIGHER_LAYER_IRAP_SKIP_FLAG
  // When single_layer_for_non_irap_flag is equal to 0, higher_layer_irap_skip_flag shall be equal to 0
  if( !vps->getSingleLayerForNonIrapFlag() )
  {
    assert( !vps->getHigherLayerIrapSkipFlag() );
  }

  WRITE_FLAG(vps->getHigherLayerIrapSkipFlag(), "higher_layer_irap_skip_flag" );
#endif
#endif
#if P0312_VERT_PHASE_ADJ
  WRITE_FLAG( vps->getVpsVuiVertPhaseInUseFlag(), "vps_vui_vert_phase_in_use_flag" );
#endif
#if N0160_VUI_EXT_ILP_REF
  WRITE_FLAG( vps->getIlpRestrictedRefLayersFlag() ? 1 : 0 , "ilp_restricted_ref_layers_flag" );    
  if( vps->getIlpRestrictedRefLayersFlag())
  {
    for(i = 1; i < vps->getMaxLayers(); i++)
    {
      for(j = 0; j < vps->getNumDirectRefLayers(vps->getLayerIdInNuh(i)); j++)
      {        
        WRITE_UVLC(vps->getMinSpatialSegmentOffsetPlus1( i, j),    "min_spatial_segment_offset_plus1[i][j]");
       
        if( vps->getMinSpatialSegmentOffsetPlus1(i,j ) > 0 ) 
        {  
          WRITE_FLAG( vps->getCtuBasedOffsetEnabledFlag( i, j) ? 1 : 0 , "ctu_based_offset_enabled_flag[i][j]" );    
          
          if(vps->getCtuBasedOffsetEnabledFlag(i,j))  
          {
            WRITE_UVLC(vps->getMinHorizontalCtuOffsetPlus1( i, j),    "min_horizontal_ctu_offset_plus1[i][j]");            
          }
        }  
      }  
    }
  }
#endif 
#if VPS_VUI_VIDEO_SIGNAL
#if VPS_VUI_VIDEO_SIGNAL_MOVE
#else 
    WRITE_FLAG( vps->getVideoSigPresentVpsFlag(), "video_signal_info_idx_present_flag" );
    if (vps->getVideoSigPresentVpsFlag())
    {
        WRITE_CODE(vps->getNumVideoSignalInfo()-1, 4, "vps_num_video_signal_info_minus1" );
    }
    
    for(i = 0; i < vps->getNumVideoSignalInfo(); i++)
    {
        WRITE_CODE(vps->getVideoVPSFormat(i), 3, "video_vps_format" );
        WRITE_FLAG(vps->getVideoFullRangeVpsFlag(i), "video_full_range_vps_flag" );
        WRITE_CODE(vps->getColorPrimaries(i), 8, "color_primaries_vps" );
        WRITE_CODE(vps->getTransCharacter(i), 8, "transfer_characteristics_vps" );
        WRITE_CODE(vps->getMaxtrixCoeff(i), 8, "matrix_coeffs_vps" );
    }
    
    if (vps->getVideoSigPresentVpsFlag() && vps->getNumVideoSignalInfo() > 1 )
    {
        for (i=1; i < vps->getMaxLayers(); i++)
            WRITE_CODE(vps->getVideoSignalInfoIdx(i), 4, "vps_video_signal_info_idx" );
    }
#endif 
#endif
#if O0164_MULTI_LAYER_HRD
    WRITE_FLAG(vps->getVpsVuiBspHrdPresentFlag(), "vps_vui_bsp_hrd_present_flag" );
    if (vps->getVpsVuiBspHrdPresentFlag())
    {
#if VPS_VUI_BSP_HRD_PARAMS
      codeVpsVuiBspHrdParams(vps);
#else
      WRITE_UVLC( vps->getVpsNumBspHrdParametersMinus1(), "vps_num_bsp_hrd_parameters_minus1" );
      for( i = 0; i <= vps->getVpsNumBspHrdParametersMinus1(); i++ )
      {
        if( i > 0 )
        {
          WRITE_FLAG( vps->getBspCprmsPresentFlag(i), "bsp_cprms_present_flag[i]" );
        }
        codeHrdParameters(vps->getBspHrd(i), i==0 ? 1 : vps->getBspCprmsPresentFlag(i), vps->getMaxTLayers()-1);
      }
#if Q0078_ADD_LAYER_SETS
      for( UInt h = 1; h <= vps->getVpsNumLayerSetsMinus1(); h++ )
#else
      for( UInt h = 1; h <= (vps->getNumLayerSets()-1); h++ )
#endif
      {
        WRITE_UVLC( vps->getNumBitstreamPartitions(h), "num_bitstream_partitions[i]");
        for( i = 0; i < vps->getNumBitstreamPartitions(h); i++ )
        {
          for( j = 0; j <= (vps->getMaxLayers()-1); j++ )
          {
            if (vps->getLayerIdIncludedFlag(h, j))
            {
              WRITE_FLAG( vps->getLayerInBspFlag(h, i, j), "layer_in_bsp_flag[h][i][j]" );
            }
          }
        }
        if (vps->getNumBitstreamPartitions(h))
        {
#if Q0182_MULTI_LAYER_HRD_UPDATE
          WRITE_UVLC(vps->getNumBspSchedCombinations(h) - 1, "num_bsp_sched_combinations_minus1[h]");
#else
          WRITE_UVLC( vps->getNumBspSchedCombinations(h), "num_bsp_sched_combinations[h]");
#endif
          for( i = 0; i < vps->getNumBspSchedCombinations(h); i++ )
          {
            for( j = 0; j < vps->getNumBitstreamPartitions(h); j++ )
            {
              WRITE_UVLC( vps->getBspCombHrdIdx(h, i, j), "bsp_comb_hrd_idx[h][i][j]");
              WRITE_UVLC( vps->getBspCombSchedIdx(h, i, j), "bsp_comb_sched_idx[h][i][j]");
            }
          }
        }
      }
#endif
    }
#endif
#if P0182_VPS_VUI_PS_FLAG
    for(i = 1; i < vps->getMaxLayers(); i++)
    {
      if( vps->getNumRefLayers(vps->getLayerIdInNuh(i)) == 0 ) 
      {
        if( (vps->getSPSId(i) == 0) && (vps->getPPSId(i) == 0) )
        {
          vps->setBaseLayerPSCompatibilityFlag(i, 1);
        }
        else
        {
          vps->setBaseLayerPSCompatibilityFlag(i, 0);
        }
        WRITE_FLAG(vps->getBaseLayerPSCompatibilityFlag(i), "base_layer_parameter_set_compatibility_flag" );
      }
    }
#endif
}
#endif //SVC_EXTENSION

Void TEncCavlc::codeSliceHeader         ( TComSlice* pcSlice )
{
#if ENC_DEC_TRACE  
  xTraceSliceHeader (pcSlice);
#endif

  //calculate number of bits required for slice address
  Int maxSliceSegmentAddress = pcSlice->getPic()->getNumCUsInFrame();
  Int bitsSliceSegmentAddress = 0;
  while(maxSliceSegmentAddress>(1<<bitsSliceSegmentAddress)) 
  {
    bitsSliceSegmentAddress++;
  }
  Int ctuAddress;
  if (pcSlice->isNextSlice())
  {
    // Calculate slice address
    ctuAddress = (pcSlice->getSliceCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU());
  }
  else
  {
    // Calculate slice address
    ctuAddress = (pcSlice->getSliceSegmentCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU());
  }
  //write slice address
  Int sliceSegmentAddress = pcSlice->getPic()->getPicSym()->getCUOrderMap(ctuAddress);

  WRITE_FLAG( sliceSegmentAddress==0, "first_slice_segment_in_pic_flag" );
  if ( pcSlice->getRapPicFlag() )
  {
#if SETTING_NO_OUT_PIC_PRIOR
    WRITE_FLAG( pcSlice->getNoOutputPriorPicsFlag() ? 1 : 0, "no_output_of_prior_pics_flag" );
#else
    WRITE_FLAG( 0, "no_output_of_prior_pics_flag" );
#endif
  }
  WRITE_UVLC( pcSlice->getPPS()->getPPSId(), "slice_pic_parameter_set_id" );
  pcSlice->setDependentSliceSegmentFlag(!pcSlice->isNextSlice());
  if ( pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag() && (sliceSegmentAddress!=0) )
  {
    WRITE_FLAG( pcSlice->getDependentSliceSegmentFlag() ? 1 : 0, "dependent_slice_segment_flag" );
  }
  if(sliceSegmentAddress>0)
  {
    WRITE_CODE( sliceSegmentAddress, bitsSliceSegmentAddress, "slice_segment_address" );
  }
  if ( !pcSlice->getDependentSliceSegmentFlag() )
  {
#if SVC_EXTENSION
#if POC_RESET_FLAG
    Int iBits = 0;
    if( pcSlice->getPPS()->getNumExtraSliceHeaderBits() > iBits )
    {
      WRITE_FLAG( pcSlice->getPocResetFlag(), "poc_reset_flag" );
      iBits++;
    }
    if( pcSlice->getPPS()->getNumExtraSliceHeaderBits() > iBits )
    {
      assert(!!"discardable_flag");
      WRITE_FLAG(pcSlice->getDiscardableFlag(), "discardable_flag");
      iBits++;
    }
#if O0149_CROSS_LAYER_BLA_FLAG
    if( pcSlice->getPPS()->getNumExtraSliceHeaderBits() > iBits )
    {
      assert(!!"cross_layer_bla_flag");
      WRITE_FLAG(pcSlice->getCrossLayerBLAFlag(), "cross_layer_bla_flag");
      iBits++;
    }
#endif
    for ( ; iBits < pcSlice->getPPS()->getNumExtraSliceHeaderBits(); iBits++)
    {
      assert(!!"slice_reserved_undetermined_flag[]");
      WRITE_FLAG(0, "slice_reserved_undetermined_flag[]");
    }
#else
#if CROSS_LAYER_BLA_FLAG_FIX
    Int iBits = 0;
    if(pcSlice->getPPS()->getNumExtraSliceHeaderBits() > iBits)
#else
    if (pcSlice->getPPS()->getNumExtraSliceHeaderBits()>0)
#endif
    {
      assert(!!"discardable_flag");
#if NON_REF_NAL_TYPE_DISCARDABLE
      if (pcSlice->getDiscardableFlag())
      {
        assert(pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_TRAIL_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_TSA_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_STSA_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_RADL_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_RASL_R);
      }
#endif
      WRITE_FLAG(pcSlice->getDiscardableFlag(), "discardable_flag");
#if CROSS_LAYER_BLA_FLAG_FIX
      iBits++;
#endif
    }
#if CROSS_LAYER_BLA_FLAG_FIX
    if( pcSlice->getPPS()->getNumExtraSliceHeaderBits() > iBits )
    {
      assert(!!"cross_layer_bla_flag");
      WRITE_FLAG(pcSlice->getCrossLayerBLAFlag(), "cross_layer_bla_flag");
      iBits++;
    }
    for (; iBits < pcSlice->getPPS()->getNumExtraSliceHeaderBits(); iBits++)
#else
    for (Int i = 1; i < pcSlice->getPPS()->getNumExtraSliceHeaderBits(); i++)
#endif
    {
      assert(!!"slice_reserved_undetermined_flag[]");
      WRITE_FLAG(0, "slice_reserved_undetermined_flag[]");
    }
#endif
#else //SVC_EXTENSION
    for (Int i = 0; i < pcSlice->getPPS()->getNumExtraSliceHeaderBits(); i++)
    {
      assert(!!"slice_reserved_undetermined_flag[]");
      WRITE_FLAG(0, "slice_reserved_undetermined_flag[]");
    }
#endif //SVC_EXTENSION

    WRITE_UVLC( pcSlice->getSliceType(),       "slice_type" );

    if( pcSlice->getPPS()->getOutputFlagPresentFlag() )
    {
      WRITE_FLAG( pcSlice->getPicOutputFlag() ? 1 : 0, "pic_output_flag" );
    }

#if !AUXILIARY_PICTURES
#if REPN_FORMAT_IN_VPS
    // in the first version chroma_format_idc is equal to one, thus colour_plane_id will not be present
    assert( pcSlice->getChromaFormatIdc() == 1 );
#else
    // in the first version chroma_format_idc is equal to one, thus colour_plane_id will not be present
    assert (pcSlice->getSPS()->getChromaFormatIdc() == 1 );
#endif
#endif
    // if( separate_colour_plane_flag  ==  1 )
    //   colour_plane_id                                      u(2)

#if N0065_LAYER_POC_ALIGNMENT
#if O0062_POC_LSB_NOT_PRESENT_FLAG
    if( (pcSlice->getLayerId() > 0 && !pcSlice->getVPS()->getPocLsbNotPresentFlag( pcSlice->getVPS()->getLayerIdInVps(pcSlice->getLayerId())) ) || !pcSlice->getIdrPicFlag())
#else
    if( pcSlice->getLayerId() > 0 || !pcSlice->getIdrPicFlag() )
#endif
#else
    if( !pcSlice->getIdrPicFlag() )
#endif
    {
#if POC_RESET_FLAG
      Int picOrderCntLSB;
      if( !pcSlice->getPocResetFlag() )
      {
        picOrderCntLSB = (pcSlice->getPOC()-pcSlice->getLastIDR()+(1<<pcSlice->getSPS()->getBitsForPOC())) & ((1<<pcSlice->getSPS()->getBitsForPOC())-1);
      }
      else
      {
        picOrderCntLSB = (pcSlice->getPocValueBeforeReset()-pcSlice->getLastIDR()+(1<<pcSlice->getSPS()->getBitsForPOC())) & ((1<<pcSlice->getSPS()->getBitsForPOC())-1);
      }
#else
#if POC_RESET_IDC_ENCODER
      Int picOrderCntLSB;
      if( pcSlice->getPocResetIdc() == 2 )  // i.e. the LSB is reset
      {
        picOrderCntLSB = pcSlice->getPicOrderCntLsb();  // This will be the LSB value w.r.t to the previous POC reset period.
      }
      else
      {
        picOrderCntLSB = (pcSlice->getPOC() + (1<<pcSlice->getSPS()->getBitsForPOC())) & ((1<<pcSlice->getSPS()->getBitsForPOC())-1);
      }
#else
      Int picOrderCntLSB = (pcSlice->getPOC()-pcSlice->getLastIDR()+(1<<pcSlice->getSPS()->getBitsForPOC())) & ((1<<pcSlice->getSPS()->getBitsForPOC())-1);
#endif
#endif
      WRITE_CODE( picOrderCntLSB, pcSlice->getSPS()->getBitsForPOC(), "pic_order_cnt_lsb");

#if N0065_LAYER_POC_ALIGNMENT
#if SHM_FIX7
    }
#endif
      if( !pcSlice->getIdrPicFlag() )
      {
#endif
      TComReferencePictureSet* rps = pcSlice->getRPS();
      
      // check for bitstream restriction stating that:
      // If the current picture is a BLA or CRA picture, the value of NumPocTotalCurr shall be equal to 0.
      // Ideally this process should not be repeated for each slice in a picture
#if SVC_EXTENSION
      if( pcSlice->getLayerId() == 0 )
#endif
      if (pcSlice->isIRAP())
      {
        for (Int picIdx = 0; picIdx < rps->getNumberOfPictures(); picIdx++)
        {
          assert (!rps->getUsed(picIdx));
        }
      }

      if(pcSlice->getRPSidx() < 0)
      {
        WRITE_FLAG( 0, "short_term_ref_pic_set_sps_flag");
        codeShortTermRefPicSet(pcSlice->getSPS(), rps, true, pcSlice->getSPS()->getRPSList()->getNumberOfReferencePictureSets());
      }
      else
      {
        WRITE_FLAG( 1, "short_term_ref_pic_set_sps_flag");
        Int numBits = 0;
        while ((1 << numBits) < pcSlice->getSPS()->getRPSList()->getNumberOfReferencePictureSets())
        {
          numBits++;
        }
        if (numBits > 0)
        {
          WRITE_CODE( pcSlice->getRPSidx(), numBits, "short_term_ref_pic_set_idx" );          
        }
      }
      if(pcSlice->getSPS()->getLongTermRefsPresent())
      {
        Int numLtrpInSH = rps->getNumberOfLongtermPictures();
        Int ltrpInSPS[MAX_NUM_REF_PICS];
        Int numLtrpInSPS = 0;
        UInt ltrpIndex;
        Int counter = 0;
        for(Int k = rps->getNumberOfPictures()-1; k > rps->getNumberOfPictures()-rps->getNumberOfLongtermPictures()-1; k--) 
        {
          if (findMatchingLTRP(pcSlice, &ltrpIndex, rps->getPOC(k), rps->getUsed(k))) 
          {
            ltrpInSPS[numLtrpInSPS] = ltrpIndex;
            numLtrpInSPS++;
          }
          else
          {
            counter++;
          }
        }
        numLtrpInSH -= numLtrpInSPS;

        Int bitsForLtrpInSPS = 0;
        while (pcSlice->getSPS()->getNumLongTermRefPicSPS() > (1 << bitsForLtrpInSPS))
        {
          bitsForLtrpInSPS++;
        }
        if (pcSlice->getSPS()->getNumLongTermRefPicSPS() > 0) 
        {
          WRITE_UVLC( numLtrpInSPS, "num_long_term_sps");
        }
        WRITE_UVLC( numLtrpInSH, "num_long_term_pics");
        // Note that the LSBs of the LT ref. pic. POCs must be sorted before.
        // Not sorted here because LT ref indices will be used in setRefPicList()
        Int prevDeltaMSB = 0, prevLSB = 0;
        Int offset = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures();
        for(Int i=rps->getNumberOfPictures()-1 ; i > offset-1; i--)
        {
          if (counter < numLtrpInSPS)
          {
            if (bitsForLtrpInSPS > 0)
            {
              WRITE_CODE( ltrpInSPS[counter], bitsForLtrpInSPS, "lt_idx_sps[i]");              
            }
          }
          else 
          {
            WRITE_CODE( rps->getPocLSBLT(i), pcSlice->getSPS()->getBitsForPOC(), "poc_lsb_lt");
            WRITE_FLAG( rps->getUsed(i), "used_by_curr_pic_lt_flag"); 
          }
          WRITE_FLAG( rps->getDeltaPocMSBPresentFlag(i), "delta_poc_msb_present_flag");

          if(rps->getDeltaPocMSBPresentFlag(i))
          {
            Bool deltaFlag = false;
            //  First LTRP from SPS                 ||  First LTRP from SH                              || curr LSB            != prev LSB
            if( (i == rps->getNumberOfPictures()-1) || (i == rps->getNumberOfPictures()-1-numLtrpInSPS) || (rps->getPocLSBLT(i) != prevLSB) )
            {
              deltaFlag = true;
            }
            if(deltaFlag)
            {
              WRITE_UVLC( rps->getDeltaPocMSBCycleLT(i), "delta_poc_msb_cycle_lt[i]" );
            }
            else
            {              
              Int differenceInDeltaMSB = rps->getDeltaPocMSBCycleLT(i) - prevDeltaMSB;
              assert(differenceInDeltaMSB >= 0);
              WRITE_UVLC( differenceInDeltaMSB, "delta_poc_msb_cycle_lt[i]" );
            }
            prevLSB = rps->getPocLSBLT(i);
            prevDeltaMSB = rps->getDeltaPocMSBCycleLT(i);
          }
        }
      }
      if (pcSlice->getSPS()->getTMVPFlagsPresent())
      {
#if R0226_SLICE_TMVP
        WRITE_FLAG( pcSlice->getEnableTMVPFlag() ? 1 : 0, "slice_temporal_mvp_enabled_flag" );
#else
        WRITE_FLAG( pcSlice->getEnableTMVPFlag() ? 1 : 0, "slice_temporal_mvp_enable_flag" );
#endif
      }
#if N0065_LAYER_POC_ALIGNMENT && !SHM_FIX7
      }
#endif
    }

#if SVC_EXTENSION
    if((pcSlice->getLayerId() > 0) && !(pcSlice->getVPS()->getIlpSshSignalingEnabledFlag()) && (pcSlice->getNumILRRefIdx() > 0) )
    {
      WRITE_FLAG(pcSlice->getInterLayerPredEnabledFlag(),"inter_layer_pred_enabled_flag");
      if( pcSlice->getInterLayerPredEnabledFlag())
      {
        if(pcSlice->getNumILRRefIdx() > 1)
        {
          Int numBits = 1;
          while ((1 << numBits) < pcSlice->getNumILRRefIdx())
          {
            numBits++;
          }
          if( !pcSlice->getVPS()->getMaxOneActiveRefLayerFlag()) 
          {
            WRITE_CODE(pcSlice->getActiveNumILRRefIdx() - 1, numBits,"num_inter_layer_ref_pics_minus1");
          }       

          if( pcSlice->getNumILRRefIdx() != pcSlice->getActiveNumILRRefIdx() )
          {
            for(Int i = 0; i < pcSlice->getActiveNumILRRefIdx(); i++ )
            {
              WRITE_CODE(pcSlice->getInterLayerPredLayerIdc(i),numBits,"inter_layer_pred_layer_idc[i]");   
            }
          }
        }
      }
    }     
#if P0312_VERT_PHASE_ADJ
    for(Int i = 0; i < pcSlice->getActiveNumILRRefIdx(); i++ )
    {
      UInt refLayerIdc = pcSlice->getInterLayerPredLayerIdc(i);
      if( pcSlice->getSPS()->getVertPhasePositionEnableFlag(refLayerIdc) )
      {
        WRITE_FLAG( pcSlice->getVertPhasePositionFlag(refLayerIdc), "vert_phase_position_flag" );
      }
    }
#endif
#endif //SVC_EXTENSION

    if(pcSlice->getSPS()->getUseSAO())
    {
      if (pcSlice->getSPS()->getUseSAO())
      {
         WRITE_FLAG( pcSlice->getSaoEnabledFlag(), "slice_sao_luma_flag" );         
#if AUXILIARY_PICTURES
         if (pcSlice->getChromaFormatIdc() != CHROMA_400)         
#endif
         WRITE_FLAG( pcSlice->getSaoEnabledFlagChroma(), "slice_sao_chroma_flag" );
      }
    }   

    //check if numrefidxes match the defaults. If not, override

    if (!pcSlice->isIntra())
    {
      Bool overrideFlag = (pcSlice->getNumRefIdx( REF_PIC_LIST_0 )!=pcSlice->getPPS()->getNumRefIdxL0DefaultActive()||(pcSlice->isInterB()&&pcSlice->getNumRefIdx( REF_PIC_LIST_1 )!=pcSlice->getPPS()->getNumRefIdxL1DefaultActive()));
      WRITE_FLAG( overrideFlag ? 1 : 0,                               "num_ref_idx_active_override_flag");
      if (overrideFlag) 
      {
        WRITE_UVLC( pcSlice->getNumRefIdx( REF_PIC_LIST_0 ) - 1,      "num_ref_idx_l0_active_minus1" );
        if (pcSlice->isInterB())
        {
          WRITE_UVLC( pcSlice->getNumRefIdx( REF_PIC_LIST_1 ) - 1,    "num_ref_idx_l1_active_minus1" );
        }
        else
        {
          pcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
        }
      }
    }
    else
    {
      pcSlice->setNumRefIdx(REF_PIC_LIST_0, 0);
      pcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
    }

    if( pcSlice->getPPS()->getListsModificationPresentFlag() && pcSlice->getNumRpsCurrTempList() > 1)
    {
      TComRefPicListModification* refPicListModification = pcSlice->getRefPicListModification();
      if(!pcSlice->isIntra())
      {
        WRITE_FLAG(pcSlice->getRefPicListModification()->getRefPicListModificationFlagL0() ? 1 : 0,       "ref_pic_list_modification_flag_l0" );
        if (pcSlice->getRefPicListModification()->getRefPicListModificationFlagL0())
        {
          Int numRpsCurrTempList0 = pcSlice->getNumRpsCurrTempList();
          if (numRpsCurrTempList0 > 1)
          {
            Int length = 1;
            numRpsCurrTempList0 --;
            while ( numRpsCurrTempList0 >>= 1) 
            {
              length ++;
            }
            for(Int i = 0; i < pcSlice->getNumRefIdx( REF_PIC_LIST_0 ); i++)
            {
              WRITE_CODE( refPicListModification->getRefPicSetIdxL0(i), length, "list_entry_l0");
            }
          }
        }
      }
      if(pcSlice->isInterB())
      {    
        WRITE_FLAG(pcSlice->getRefPicListModification()->getRefPicListModificationFlagL1() ? 1 : 0,       "ref_pic_list_modification_flag_l1" );
        if (pcSlice->getRefPicListModification()->getRefPicListModificationFlagL1())
        {
          Int numRpsCurrTempList1 = pcSlice->getNumRpsCurrTempList();
          if ( numRpsCurrTempList1 > 1 )
          {
            Int length = 1;
            numRpsCurrTempList1 --;
            while ( numRpsCurrTempList1 >>= 1)
            {
              length ++;
            }
            for(Int i = 0; i < pcSlice->getNumRefIdx( REF_PIC_LIST_1 ); i++)
            {
              WRITE_CODE( refPicListModification->getRefPicSetIdxL1(i), length, "list_entry_l1");
            }
          }
        }
      }
    }
    
    if (pcSlice->isInterB())
    {
      WRITE_FLAG( pcSlice->getMvdL1ZeroFlag() ? 1 : 0,   "mvd_l1_zero_flag");
    }

    if(!pcSlice->isIntra())
    {
      if (!pcSlice->isIntra() && pcSlice->getPPS()->getCabacInitPresentFlag())
      {
        SliceType sliceType   = pcSlice->getSliceType();
        Int  encCABACTableIdx = pcSlice->getPPS()->getEncCABACTableIdx();
        Bool encCabacInitFlag = (sliceType!=encCABACTableIdx && encCABACTableIdx!=I_SLICE) ? true : false;
        pcSlice->setCabacInitFlag( encCabacInitFlag );
        WRITE_FLAG( encCabacInitFlag?1:0, "cabac_init_flag" );
      }
    }

    if ( pcSlice->getEnableTMVPFlag() )
    {
      if ( pcSlice->getSliceType() == B_SLICE )
      {
        WRITE_FLAG( pcSlice->getColFromL0Flag(), "collocated_from_l0_flag" );
      }

      if ( pcSlice->getSliceType() != I_SLICE &&
        ((pcSlice->getColFromL0Flag()==1 && pcSlice->getNumRefIdx(REF_PIC_LIST_0)>1)||
        (pcSlice->getColFromL0Flag()==0  && pcSlice->getNumRefIdx(REF_PIC_LIST_1)>1)))
      {
        WRITE_UVLC( pcSlice->getColRefIdx(), "collocated_ref_idx" );
      }
    }
    if ( (pcSlice->getPPS()->getUseWP() && pcSlice->getSliceType()==P_SLICE) || (pcSlice->getPPS()->getWPBiPred() && pcSlice->getSliceType()==B_SLICE) )
    {
      xCodePredWeightTable( pcSlice );
    }
    assert(pcSlice->getMaxNumMergeCand()<=MRG_MAX_NUM_CANDS);
    if (!pcSlice->isIntra())
    {
      WRITE_UVLC(MRG_MAX_NUM_CANDS - pcSlice->getMaxNumMergeCand(), "five_minus_max_num_merge_cand");
    }
    Int iCode = pcSlice->getSliceQp() - ( pcSlice->getPPS()->getPicInitQPMinus26() + 26 );
    WRITE_SVLC( iCode, "slice_qp_delta" ); 
    if (pcSlice->getPPS()->getSliceChromaQpFlag())
    {
      iCode = pcSlice->getSliceQpDeltaCb();
      WRITE_SVLC( iCode, "slice_qp_delta_cb" );
      iCode = pcSlice->getSliceQpDeltaCr();
      WRITE_SVLC( iCode, "slice_qp_delta_cr" );
    }
    if (pcSlice->getPPS()->getDeblockingFilterControlPresentFlag())
    {
      if (pcSlice->getPPS()->getDeblockingFilterOverrideEnabledFlag() )
      {
        WRITE_FLAG(pcSlice->getDeblockingFilterOverrideFlag(), "deblocking_filter_override_flag");
      }
      if (pcSlice->getDeblockingFilterOverrideFlag())
      {
        WRITE_FLAG(pcSlice->getDeblockingFilterDisable(), "slice_disable_deblocking_filter_flag");
        if(!pcSlice->getDeblockingFilterDisable())
        {
          WRITE_SVLC (pcSlice->getDeblockingFilterBetaOffsetDiv2(), "slice_beta_offset_div2");
          WRITE_SVLC (pcSlice->getDeblockingFilterTcOffsetDiv2(),   "slice_tc_offset_div2");
        }
      }
    }

    Bool isSAOEnabled = (!pcSlice->getSPS()->getUseSAO())?(false):(pcSlice->getSaoEnabledFlag()||pcSlice->getSaoEnabledFlagChroma());
    Bool isDBFEnabled = (!pcSlice->getDeblockingFilterDisable());

    if(pcSlice->getPPS()->getLoopFilterAcrossSlicesEnabledFlag() && ( isSAOEnabled || isDBFEnabled ))
    {
      WRITE_FLAG(pcSlice->getLFCrossSliceBoundaryFlag()?1:0, "slice_loop_filter_across_slices_enabled_flag");
    }
  }

#if !POC_RESET_IDC_SIGNALLING   // Wrong place to put slice header extension
  if(pcSlice->getPPS()->getSliceHeaderExtensionPresentFlag())
  {
    WRITE_UVLC(0,"slice_header_extension_length");
  }
#endif
}

Void TEncCavlc::codePTL( TComPTL* pcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1)
{
  if(profilePresentFlag)
  {
    codeProfileTier(pcPTL->getGeneralPTL());    // general_...
  }
  WRITE_CODE( pcPTL->getGeneralPTL()->getLevelIdc(), 8, "general_level_idc" );

  for (Int i = 0; i < maxNumSubLayersMinus1; i++)
  {
    if(profilePresentFlag)
    {
      WRITE_FLAG( pcPTL->getSubLayerProfilePresentFlag(i), "sub_layer_profile_present_flag[i]" );
    }
    
    WRITE_FLAG( pcPTL->getSubLayerLevelPresentFlag(i),   "sub_layer_level_present_flag[i]" );
  }
  
  if (maxNumSubLayersMinus1 > 0)
  {
    for (Int i = maxNumSubLayersMinus1; i < 8; i++)
    {
      WRITE_CODE(0, 2, "reserved_zero_2bits");
    }
  }
  
  for(Int i = 0; i < maxNumSubLayersMinus1; i++)
  {
    if( profilePresentFlag && pcPTL->getSubLayerProfilePresentFlag(i) )
    {
      codeProfileTier(pcPTL->getSubLayerPTL(i));  // sub_layer_...
    }
    if( pcPTL->getSubLayerLevelPresentFlag(i) )
    {
      WRITE_CODE( pcPTL->getSubLayerPTL(i)->getLevelIdc(), 8, "sub_layer_level_idc[i]" );
    }
  }
}
Void TEncCavlc::codeProfileTier( ProfileTierLevel* ptl )
{
  WRITE_CODE( ptl->getProfileSpace(), 2 ,     "XXX_profile_space[]");
  WRITE_FLAG( ptl->getTierFlag    (),         "XXX_tier_flag[]"    );
  WRITE_CODE( ptl->getProfileIdc  (), 5 ,     "XXX_profile_idc[]"  );   
  for(Int j = 0; j < 32; j++)
  {
    WRITE_FLAG( ptl->getProfileCompatibilityFlag(j), "XXX_profile_compatibility_flag[][j]");   
  }

  WRITE_FLAG(ptl->getProgressiveSourceFlag(),   "general_progressive_source_flag");
  WRITE_FLAG(ptl->getInterlacedSourceFlag(),    "general_interlaced_source_flag");
  WRITE_FLAG(ptl->getNonPackedConstraintFlag(), "general_non_packed_constraint_flag");
  WRITE_FLAG(ptl->getFrameOnlyConstraintFlag(), "general_frame_only_constraint_flag");
  
  WRITE_CODE(0 , 16, "XXX_reserved_zero_44bits[0..15]");
  WRITE_CODE(0 , 16, "XXX_reserved_zero_44bits[16..31]");
  WRITE_CODE(0 , 12, "XXX_reserved_zero_44bits[32..43]");
}

/**
 - write wavefront substreams sizes for the slice header.
 .
 \param pcSlice Where we find the substream size information.
 */
Void  TEncCavlc::codeTilesWPPEntryPoint( TComSlice* pSlice )
{
  if (!pSlice->getPPS()->getTilesEnabledFlag() && !pSlice->getPPS()->getEntropyCodingSyncEnabledFlag())
  {
    return;
  }
  UInt numEntryPointOffsets = 0, offsetLenMinus1 = 0, maxOffset = 0;
#if !WPP_FIX
  Int  numZeroSubstreamsAtStartOfSlice  = 0;
#endif
  UInt *entryPointOffset = NULL;
  if ( pSlice->getPPS()->getTilesEnabledFlag() )
  {
    numEntryPointOffsets = pSlice->getTileLocationCount();
    entryPointOffset     = new UInt[numEntryPointOffsets];
    for (Int idx=0; idx<pSlice->getTileLocationCount(); idx++)
    {
      if ( idx == 0 )
      {
        entryPointOffset [ idx ] = pSlice->getTileLocation( 0 );
      }
      else
      {
        entryPointOffset [ idx ] = pSlice->getTileLocation( idx ) - pSlice->getTileLocation( idx-1 );
      }

      if ( entryPointOffset[ idx ] > maxOffset )
      {
        maxOffset = entryPointOffset[ idx ];
      }
    }
  }
  else if ( pSlice->getPPS()->getEntropyCodingSyncEnabledFlag() )
  {
    UInt* pSubstreamSizes               = pSlice->getSubstreamSizes();
    Int maxNumParts                       = pSlice->getPic()->getNumPartInCU();
#if WPP_FIX
    Int  numZeroSubstreamsAtStartOfSlice  = pSlice->getPic()->getSubstreamForLCUAddr(pSlice->getSliceSegmentCurStartCUAddr()/maxNumParts, false, pSlice); 
    Int  subStreamOfLastSegmentOfSlice    = pSlice->getPic()->getSubstreamForLCUAddr((pSlice->getSliceSegmentCurEndCUAddr()/maxNumParts)-1, false, pSlice); 
    numEntryPointOffsets                  = subStreamOfLastSegmentOfSlice-numZeroSubstreamsAtStartOfSlice;
#else
    numZeroSubstreamsAtStartOfSlice       = pSlice->getSliceSegmentCurStartCUAddr()/maxNumParts/pSlice->getPic()->getFrameWidthInCU();
    Int  numZeroSubstreamsAtEndOfSlice    = pSlice->getPic()->getFrameHeightInCU()-1 - ((pSlice->getSliceSegmentCurEndCUAddr()-1)/maxNumParts/pSlice->getPic()->getFrameWidthInCU());
    numEntryPointOffsets                  = pSlice->getPPS()->getNumSubstreams() - numZeroSubstreamsAtStartOfSlice - numZeroSubstreamsAtEndOfSlice - 1;
#endif
    pSlice->setNumEntryPointOffsets(numEntryPointOffsets);
    entryPointOffset           = new UInt[numEntryPointOffsets];
    for (Int idx=0; idx<numEntryPointOffsets; idx++)
    {
      entryPointOffset[ idx ] = ( pSubstreamSizes[ idx+numZeroSubstreamsAtStartOfSlice ] >> 3 ) ;
      if ( entryPointOffset[ idx ] > maxOffset )
      {
        maxOffset = entryPointOffset[ idx ];
      }
    }
  }
  // Determine number of bits "offsetLenMinus1+1" required for entry point information
  offsetLenMinus1 = 0;
  while (maxOffset >= (1u << (offsetLenMinus1 + 1)))
  {
    offsetLenMinus1++;
    assert(offsetLenMinus1 + 1 < 32);    
  }

  WRITE_UVLC(numEntryPointOffsets, "num_entry_point_offsets");
  if (numEntryPointOffsets>0)
  {
    WRITE_UVLC(offsetLenMinus1, "offset_len_minus1");
  }

  for (UInt idx=0; idx<numEntryPointOffsets; idx++)
  {
    WRITE_CODE(entryPointOffset[ idx ]-1, offsetLenMinus1+1, "entry_point_offset_minus1");
  }

  delete [] entryPointOffset;
}

#if POC_RESET_IDC_SIGNALLING
Void  TEncCavlc::codeSliceHeaderExtn( TComSlice* slice, Int shBitsWrittenTillNow )
{
  Int tmpBitsBeforeWriting = getNumberOfWrittenBits();
  Int maxPocLsb = 1 << slice->getSPS()->getBitsForPOC();
  if(slice->getPPS()->getSliceHeaderExtensionPresentFlag())
  {
    // Derive the value of PocMsbValRequiredFlag
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
    slice->setPocMsbValRequiredFlag( (slice->getCraPicFlag() || slice->getBlaPicFlag())
                                  && (!slice->getVPS()->getVpsPocLsbAlignedFlag() ||
                                      (slice->getVPS()->getVpsPocLsbAlignedFlag() && slice->getVPS()->getNumDirectRefLayers(slice->getLayerId()) == 0))
                                   );
#else
    slice->setPocMsbValRequiredFlag( slice->getCraPicFlag() || slice->getBlaPicFlag() );
#endif

    // Determine value of SH extension length.
    Int shExtnLengthInBit = 0;
    if (slice->getPPS()->getPocResetInfoPresentFlag())
    {
      shExtnLengthInBit += 2;
    }
    if (slice->getPocResetIdc() > 0)
    {
      shExtnLengthInBit += 6;
    }
    if (slice->getPocResetIdc() == 3)
    {
      shExtnLengthInBit += (slice->getSPS()->getBitsForPOC() + 1);
    }


#if P0297_VPS_POC_LSB_ALIGNED_FLAG
    if (!slice->getPocMsbValRequiredFlag() && slice->getVPS()->getVpsPocLsbAlignedFlag())
#else
    if (!slice->getPocMsbValRequiredFlag() /* &&  vps_poc_lsb_aligned_flag */)
#endif
    {
      shExtnLengthInBit++;
    }
    else
    {
      if( slice->getPocMsbValRequiredFlag() )
      {
        slice->setPocMsbValPresentFlag( true );
      }
      else
      {
        slice->setPocMsbValPresentFlag( false );
      }
    }

#if P0297_VPS_POC_LSB_ALIGNED_FLAG
    if (slice->getPocMsbNeeded())
    {
      slice->setPocMsbValPresentFlag(true);
    }
#endif

    if (slice->getPocMsbValPresentFlag())
    {
      UInt lengthVal = 1;
      UInt tempVal = (slice->getPocMsbVal() / maxPocLsb) + 1;
      assert ( tempVal );
      while( 1 != tempVal )
      {
        tempVal >>= 1;
        lengthVal += 2;
      }
      shExtnLengthInBit += lengthVal;
    }
    Int shExtnAdditionalBits = 0;
    if(shExtnLengthInBit % 8 != 0)
    {
      shExtnAdditionalBits = 8 - (shExtnLengthInBit % 8);
    }
    Int shExtnLength = (shExtnLengthInBit + shExtnAdditionalBits) / 8;
    WRITE_UVLC( shExtnLength, "slice_header_extension_length" );

    if(slice->getPPS()->getPocResetInfoPresentFlag())
    {
      WRITE_CODE( slice->getPocResetIdc(), 2,                                 "poc_reset_idc");
    }
    if(slice->getPocResetIdc() > 0)
    {
      WRITE_CODE( slice->getPocResetPeriodId(), 6,                            "poc_reset_period_id");
    }
    if(slice->getPocResetIdc() == 3) 
    {
      WRITE_FLAG( slice->getFullPocResetFlag() ? 1 : 0,                       "full_poc_reset_flag");
      WRITE_CODE( slice->getPocLsbVal(), slice->getSPS()->getBitsForPOC(),  "poc_lsb_val");
    }

#if P0297_VPS_POC_LSB_ALIGNED_FLAG
    if (!slice->getPocMsbValRequiredFlag() && slice->getVPS()->getVpsPocLsbAlignedFlag())
#else
    if (!slice->getPocMsbValRequiredFlag() /* &&  vps_poc_lsb_aligned_flag */)
#endif
    {
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
      WRITE_FLAG( slice->getPocMsbValPresentFlag(),                           "poc_msb_cycle_val_present_flag" );
#else
      WRITE_FLAG( slice->getPocMsbValPresentFlag(),                           "poc_msb_val_present_flag" );
#endif
    }
    if (slice->getPocMsbValPresentFlag())
    {
      assert(slice->getPocMsbVal() % maxPocLsb == 0);
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
      WRITE_UVLC(slice->getPocMsbVal() / maxPocLsb, "poc_msb_cycle_val");
#else
      WRITE_UVLC(slice->getPocMsbVal() / maxPocLsb, "poc_msb_val");
#endif
    }
    for (Int i = 0; i < shExtnAdditionalBits; i++)
    {
#if Q0146_SSH_EXT_DATA_BIT 
      WRITE_FLAG( 1, "slice_segment_header_extension_data_bit");
#else
      WRITE_FLAG( 1, "slice_segment_header_extension_reserved_bit");
#endif
    }
  }
  shBitsWrittenTillNow += ( getNumberOfWrittenBits() - tmpBitsBeforeWriting );
  
  // Slice header byte_alignment() included in xAttachSliceDataToNalUnit
}
#endif

Void TEncCavlc::codeTerminatingBit      ( UInt uilsLast )
{
}

Void TEncCavlc::codeSliceFinish ()
{
}

Void TEncCavlc::codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert(0);
}

Void TEncCavlc::codePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TEncCavlc::codePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeMergeFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeInterModeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiEncMode )
{
  assert(0);
}

Void TEncCavlc::codeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeSplitFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TEncCavlc::codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  assert(0);
}

Void TEncCavlc::codeQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  assert(0);
}

Void TEncCavlc::codeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeQtCbfZero( TComDataCU* pcCU, TextType eType, UInt uiTrDepth )
{
  assert(0);
}
Void TEncCavlc::codeQtRootCbfZero( TComDataCU* pcCU )
{
  assert(0);
}

Void TEncCavlc::codeTransformSkipFlags (TComDataCU* pcCU, UInt uiAbsPartIdx, UInt width, UInt height, TextType eTType )
{
  assert(0);
}

/** Code I_PCM information.
 * \param pcCU pointer to CU
 * \param uiAbsPartIdx CU index
 * \returns Void
 */
Void TEncCavlc::codeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool isMultiple)
{
  assert(0);
}

Void TEncCavlc::codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeRefFrmIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert(0);
}

Void TEncCavlc::codeMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert(0);
}

Void TEncCavlc::codeDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDQp  = pcCU->getQP( uiAbsPartIdx ) - pcCU->getRefQP( uiAbsPartIdx );

#if REPN_FORMAT_IN_VPS
  Int qpBdOffsetY =  pcCU->getSlice()->getQpBDOffsetY();
#else
  Int qpBdOffsetY =  pcCU->getSlice()->getSPS()->getQpBDOffsetY();
#endif
  iDQp = (iDQp + 78 + qpBdOffsetY + (qpBdOffsetY/2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY/2);

  xWriteSvlc( iDQp );
  
  return;
}

Void TEncCavlc::codeCoeffNxN    ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
{
  assert(0);
}

Void TEncCavlc::estBit( estBitsSbacStruct* pcEstBitsCabac, Int width, Int height, TextType eTType )
{
  // printf("error : no VLC mode support in this version\n");
  return;
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** code explicit wp tables
 * \param TComSlice* pcSlice
 * \returns Void
 */
Void TEncCavlc::xCodePredWeightTable( TComSlice* pcSlice )
{
  wpScalingParam  *wp;
  Bool            bChroma     = true; // color always present in HEVC ?
  Int             iNbRef       = (pcSlice->getSliceType() == B_SLICE ) ? (2) : (1);
  Bool            bDenomCoded  = false;
  UInt            uiMode = 0;
  UInt            uiTotalSignalledWeightFlags = 0;
#if AUXILIARY_PICTURES
  if (pcSlice->getChromaFormatIdc() == CHROMA_400)
  {
    bChroma = false;
  }
#endif
  if ( (pcSlice->getSliceType()==P_SLICE && pcSlice->getPPS()->getUseWP()) || (pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPred()) )
  {
    uiMode = 1; // explicit
  }
  if(uiMode == 1)
  {

    for ( Int iNumRef=0 ; iNumRef<iNbRef ; iNumRef++ ) 
    {
      RefPicList  eRefPicList = ( iNumRef ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );

      for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
        if ( !bDenomCoded ) 
        {
          Int iDeltaDenom;
          WRITE_UVLC( wp[0].uiLog2WeightDenom, "luma_log2_weight_denom" );     // ue(v): luma_log2_weight_denom

          if( bChroma )
          {
            iDeltaDenom = (wp[1].uiLog2WeightDenom - wp[0].uiLog2WeightDenom);
            WRITE_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );       // se(v): delta_chroma_log2_weight_denom
          }
          bDenomCoded = true;
        }
        WRITE_FLAG( wp[0].bPresentFlag, "luma_weight_lX_flag" );               // u(1): luma_weight_lX_flag
        uiTotalSignalledWeightFlags += wp[0].bPresentFlag;
      }
      if (bChroma) 
      {
        for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
        {
          pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
          WRITE_FLAG( wp[1].bPresentFlag, "chroma_weight_lX_flag" );           // u(1): chroma_weight_lX_flag
          uiTotalSignalledWeightFlags += 2*wp[1].bPresentFlag;
        }
      }

      for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
        if ( wp[0].bPresentFlag ) 
        {
          Int iDeltaWeight = (wp[0].iWeight - (1<<wp[0].uiLog2WeightDenom));
          WRITE_SVLC( iDeltaWeight, "delta_luma_weight_lX" );                  // se(v): delta_luma_weight_lX
          WRITE_SVLC( wp[0].iOffset, "luma_offset_lX" );                       // se(v): luma_offset_lX
        }

        if ( bChroma ) 
        {
          if ( wp[1].bPresentFlag )
          {
            for ( Int j=1 ; j<3 ; j++ ) 
            {
              Int iDeltaWeight = (wp[j].iWeight - (1<<wp[1].uiLog2WeightDenom));
              WRITE_SVLC( iDeltaWeight, "delta_chroma_weight_lX" );            // se(v): delta_chroma_weight_lX

              Int pred = ( 128 - ( ( 128*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) );
              Int iDeltaChroma = (wp[j].iOffset - pred);
              WRITE_SVLC( iDeltaChroma, "delta_chroma_offset_lX" );            // se(v): delta_chroma_offset_lX
            }
          }
        }
      }
    }
    assert(uiTotalSignalledWeightFlags<=24);
  }
}

/** code quantization matrix
 *  \param scalingList quantization matrix information
 */
Void TEncCavlc::codeScalingList( TComScalingList* scalingList )
{
  UInt listId,sizeId;
  Bool scalingListPredModeFlag;

    //for each size
    for(sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
      for(listId = 0; listId < g_scalingListNum[sizeId]; listId++)
      {
        scalingListPredModeFlag = scalingList->checkPredMode( sizeId, listId );
        WRITE_FLAG( scalingListPredModeFlag, "scaling_list_pred_mode_flag" );
        if(!scalingListPredModeFlag)// Copy Mode
        {
          WRITE_UVLC( (Int)listId - (Int)scalingList->getRefMatrixId (sizeId,listId), "scaling_list_pred_matrix_id_delta");
        }
        else// DPCM Mode
        {
          xCodeScalingList(scalingList, sizeId, listId);
        }
      }
    }
  return;
}
/** code DPCM
 * \param scalingList quantization matrix information
 * \param sizeIdc size index
 * \param listIdc list index
 */
Void TEncCavlc::xCodeScalingList(TComScalingList* scalingList, UInt sizeId, UInt listId)
{
  Int coefNum = min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]);
  UInt* scan  = (sizeId == 0) ? g_auiSigLastScan [ SCAN_DIAG ] [ 1 ] :  g_sigLastScanCG32x32;
  Int nextCoef = SCALING_LIST_START_VALUE;
  Int data;
  Int *src = scalingList->getScalingListAddress(sizeId, listId);
  if( sizeId > SCALING_LIST_8x8 )
  {
    WRITE_SVLC( scalingList->getScalingListDC(sizeId,listId) - 8, "scaling_list_dc_coef_minus8");
    nextCoef = scalingList->getScalingListDC(sizeId,listId);
  }
  for(Int i=0;i<coefNum;i++)
  {
    data = src[scan[i]] - nextCoef;
    nextCoef = src[scan[i]];
    if(data > 127)
    {
      data = data - 256;
    }
    if(data < -128)
    {
      data = data + 256;
    }

    WRITE_SVLC( data,  "scaling_list_delta_coef");
  }
}
Bool TEncCavlc::findMatchingLTRP ( TComSlice* pcSlice, UInt *ltrpsIndex, Int ltrpPOC, Bool usedFlag )
{
  // Bool state = true, state2 = false;
  Int lsb = ltrpPOC & ((1<<pcSlice->getSPS()->getBitsForPOC())-1);
  for (Int k = 0; k < pcSlice->getSPS()->getNumLongTermRefPicSPS(); k++)
  {
    if ( (lsb == pcSlice->getSPS()->getLtRefPicPocLsbSps(k)) && (usedFlag == pcSlice->getSPS()->getUsedByCurrPicLtSPSFlag(k)) )
    {
      *ltrpsIndex = k;
      return true;
    }
  }
  return false;
}
Bool TComScalingList::checkPredMode(UInt sizeId, UInt listId)
{
  for(Int predListIdx = (Int)listId ; predListIdx >= 0; predListIdx--)
  {
    if( !memcmp(getScalingListAddress(sizeId,listId),((listId == predListIdx) ?
      getScalingListDefaultAddress(sizeId, predListIdx): getScalingListAddress(sizeId, predListIdx)),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId])) // check value of matrix
     && ((sizeId < SCALING_LIST_16x16) || (getScalingListDC(sizeId,listId) == getScalingListDC(sizeId,predListIdx)))) // check DC value
    {
      setRefMatrixId(sizeId, listId, predListIdx);
      return false;
    }
  }
  return true;
}

#if Q0048_CGS_3D_ASYMLUT
Void TEncCavlc::xCode3DAsymLUT( TCom3DAsymLUT * pc3DAsymLUT )
{
#if R0150_CGS_SIGNAL_CONSTRAINTS
  UInt uiNumRefLayers = ( UInt )pc3DAsymLUT->getRefLayerNum();
  WRITE_UVLC( uiNumRefLayers - 1 , "num_cm_ref_layers_minus1" );
  for( UInt i = 0 ; i < uiNumRefLayers ; i++ )
  {
    WRITE_CODE( pc3DAsymLUT->getRefLayerId( i ) , 6 , "cm_ref_layer_id" );
  }
#endif
  assert( pc3DAsymLUT->getCurOctantDepth() < 4 );
  WRITE_CODE( pc3DAsymLUT->getCurOctantDepth() , 2 , "cm_octant_depth" );
  assert( pc3DAsymLUT->getCurYPartNumLog2() < 4 );
  WRITE_CODE( pc3DAsymLUT->getCurYPartNumLog2() , 2 , "cm_y_part_num_log2" );
  assert( pc3DAsymLUT->getInputBitDepthY() < 16 );
#if R0150_CGS_SIGNAL_CONSTRAINTS
  WRITE_UVLC( pc3DAsymLUT->getInputBitDepthY() - 8 , "cm_input_luma_bit_depth_minus8" );
  WRITE_UVLC( pc3DAsymLUT->getInputBitDepthC() - 8 , "cm_input_chroma_bit_depth_minus8" );
  WRITE_UVLC( pc3DAsymLUT->getOutputBitDepthY() - 8 , "cm_output_luma_bit_depth_minus8" );
  WRITE_UVLC( pc3DAsymLUT->getOutputBitDepthC() - 8 , "cm_output_chroma_bit_depth_minus8" );
#else
  WRITE_CODE( pc3DAsymLUT->getInputBitDepthY() - 8 , 3 , "cm_input_bit_depth_minus8" );
  WRITE_SVLC(pc3DAsymLUT->getInputBitDepthC()-pc3DAsymLUT->getInputBitDepthY(), "cm_input_bit_depth_chroma delta");
  assert( pc3DAsymLUT->getOutputBitDepthY() < 16 );
  WRITE_CODE( pc3DAsymLUT->getOutputBitDepthY() - 8 , 3 , "cm_output_bit_depth_minus8" );
  WRITE_SVLC(pc3DAsymLUT->getOutputBitDepthC()-pc3DAsymLUT->getOutputBitDepthY(), "cm_output_bit_depth_chroma_delta");
#endif
  assert( pc3DAsymLUT->getResQuantBit() < 4 );
  WRITE_CODE( pc3DAsymLUT->getResQuantBit() , 2 , "cm_res_quant_bit" );
#if R0300_CGS_RES_COEFF_CODING
  xFindDeltaBits( pc3DAsymLUT );
  assert(pc3DAsymLUT->getDeltaBits() >=1 && pc3DAsymLUT->getDeltaBits() <= 4);
  WRITE_CODE( pc3DAsymLUT->getDeltaBits()-1 , 2 , "cm_delta_bit" );
#endif 
#if R0151_CGS_3D_ASYMLUT_IMPROVE
  if( pc3DAsymLUT->getCurOctantDepth() == 1 )
  {
    WRITE_SVLC( pc3DAsymLUT->getAdaptChromaThresholdU() - ( 1 << ( pc3DAsymLUT->getInputBitDepthC() - 1 ) ) , "cm_adapt_threshold_u_delta" );
    WRITE_SVLC( pc3DAsymLUT->getAdaptChromaThresholdV() - ( 1 << ( pc3DAsymLUT->getInputBitDepthC() - 1 ) ) , "cm_adapt_threshold_v_delta" );
  }
#endif

#if R0164_CGS_LUT_BUGFIX_CHECK
  pc3DAsymLUT->xInitCuboids();
#endif
  xCode3DAsymLUTOctant( pc3DAsymLUT , 0 , 0 , 0 , 0 , 1 << pc3DAsymLUT->getCurOctantDepth() );
#if R0164_CGS_LUT_BUGFIX_CHECK
  xCuboidsFilledCheck( false );
  pc3DAsymLUT->display( false );
#endif
}

Void TEncCavlc::xCode3DAsymLUTOctant( TCom3DAsymLUT * pc3DAsymLUT , Int nDepth , Int yIdx , Int uIdx , Int vIdx , Int nLength )
{
  UInt uiOctantSplit = nDepth < pc3DAsymLUT->getCurOctantDepth();
  if( nDepth < pc3DAsymLUT->getCurOctantDepth() )
    WRITE_FLAG( uiOctantSplit , "split_octant_flag" );
  Int nYPartNum = 1 << pc3DAsymLUT->getCurYPartNumLog2();
  if( uiOctantSplit )
  {
    Int nHalfLength = nLength >> 1;
    for( Int l = 0 ; l < 2 ; l++ )
    {
      for( Int m = 0 ; m < 2 ; m++ )
      {
        for( Int n = 0 ; n < 2 ; n++ )
        {
          xCode3DAsymLUTOctant( pc3DAsymLUT , nDepth + 1 , yIdx + l * nHalfLength * nYPartNum , uIdx + m * nHalfLength , vIdx + n * nHalfLength , nHalfLength );
        }
      }
    }
  }
  else
  {
#if R0300_CGS_RES_COEFF_CODING
    Int nFLCbits = pc3DAsymLUT->getMappingShift()-pc3DAsymLUT->getResQuantBit()-pc3DAsymLUT->getDeltaBits() ; 
    nFLCbits = nFLCbits >= 0 ? nFLCbits : 0;
#endif
    for( Int l = 0 ; l < nYPartNum ; l++ )
    {
#if R0164_CGS_LUT_BUGFIX      
      Int shift = pc3DAsymLUT->getCurOctantDepth() - nDepth ;
#endif
      for( Int nVertexIdx = 0 ; nVertexIdx < 4 ; nVertexIdx++ )
      {
#if R0164_CGS_LUT_BUGFIX 
        SYUVP sRes = pc3DAsymLUT->getCuboidVertexResTree( yIdx + (l<<shift) , uIdx , vIdx , nVertexIdx );
#else
        SYUVP sRes = pc3DAsymLUT->getCuboidVertexResTree( yIdx + l , uIdx , vIdx , nVertexIdx );
#endif
        UInt uiCodeVertex = sRes.Y != 0 || sRes.U != 0 || sRes.V != 0;
        WRITE_FLAG( uiCodeVertex , "coded_vertex_flag" );
        if( uiCodeVertex )
        {
#if R0151_CGS_3D_ASYMLUT_IMPROVE
#if R0300_CGS_RES_COEFF_CODING
          xWriteParam( sRes.Y, nFLCbits );
          xWriteParam( sRes.U, nFLCbits );
          xWriteParam( sRes.V, nFLCbits );
#else
          xWriteParam( sRes.Y );
          xWriteParam( sRes.U );
          xWriteParam( sRes.V );
#endif
#else
          WRITE_SVLC( sRes.Y , "resY" );
          WRITE_SVLC( sRes.U , "resU" );
          WRITE_SVLC( sRes.V , "resV" );
#endif
        }
      }
#if R0164_CGS_LUT_BUGFIX_CHECK
      pc3DAsymLUT->xSetExplicit( yIdx + (l<<shift) , uIdx , vIdx );
#endif
    }
  }
}

#if R0151_CGS_3D_ASYMLUT_IMPROVE
#if R0300_CGS_RES_COEFF_CODING
Void TEncCavlc::xWriteParam( Int param, UInt rParam)
#else
Void TEncCavlc::xWriteParam( Int param)
#endif
{
#if !R0300_CGS_RES_COEFF_CODING
  const UInt rParam = 7;
#endif
  Int codeNumber = abs(param);
  WRITE_UVLC(codeNumber / (1 << rParam), "quotient");
  WRITE_CODE((codeNumber % (1 << rParam)), rParam, "remainder");
  if (abs(param))
    WRITE_FLAG( param <0, "sign");
}
#endif

#if R0300_CGS_RES_COEFF_CODING
Void TEncCavlc::xFindDeltaBits( TCom3DAsymLUT * pc3DAsymLUT )
{
  Int nDeltaBits; 
  Int nBestDeltaBits = -1; 
  Int nBestBits = MAX_INT; 
  for( nDeltaBits = 1; nDeltaBits < 5; nDeltaBits++)
  {
    Int nCurBits = 0;
    xTally3DAsymLUTOctantBits( pc3DAsymLUT , 0 , 0 , 0 , 0 , 1 << pc3DAsymLUT->getCurOctantDepth(), nDeltaBits, nCurBits );
    //printf("%d, %d, %d\n", nDeltaBits, nCurBits, nBestBits); 
    if(nCurBits < nBestBits)
    {
      nBestDeltaBits = nDeltaBits; 
      nBestBits = nCurBits;
    }
  }

  assert(nBestDeltaBits >=1 && nBestDeltaBits < 5);
  pc3DAsymLUT->setDeltaBits(nBestDeltaBits); 
}

Void TEncCavlc::xTally3DAsymLUTOctantBits( TCom3DAsymLUT * pc3DAsymLUT , Int nDepth , Int yIdx , Int uIdx , Int vIdx , Int nLength, Int nDeltaBits, Int& nCurBits )
{
  UInt uiOctantSplit = nDepth < pc3DAsymLUT->getCurOctantDepth();
  if( nDepth < pc3DAsymLUT->getCurOctantDepth() )
    nCurBits ++; 
  Int nYPartNum = 1 << pc3DAsymLUT->getCurYPartNumLog2();
  if( uiOctantSplit )
  {
    Int nHalfLength = nLength >> 1;
    for( Int l = 0 ; l < 2 ; l++ )
    {
      for( Int m = 0 ; m < 2 ; m++ )
      {
        for( Int n = 0 ; n < 2 ; n++ )
        {
          xTally3DAsymLUTOctantBits( pc3DAsymLUT , nDepth + 1 , yIdx + l * nHalfLength * nYPartNum , uIdx + m * nHalfLength , vIdx + n * nHalfLength , nHalfLength, nDeltaBits, nCurBits );
        }
      }
    }
  }
  else
  {
    Int nFLCbits = pc3DAsymLUT->getMappingShift()-pc3DAsymLUT->getResQuantBit()-nDeltaBits ; 
    nFLCbits = nFLCbits >= 0 ? nFLCbits:0;
    //printf("nFLCbits = %d\n", nFLCbits);

    for( Int l = 0 ; l < nYPartNum ; l++ )
    {
      for( Int nVertexIdx = 0 ; nVertexIdx < 4 ; nVertexIdx++ )
      {
        SYUVP sRes = pc3DAsymLUT->getCuboidVertexResTree( yIdx + l , uIdx , vIdx , nVertexIdx );

        UInt uiCodeVertex = sRes.Y != 0 || sRes.U != 0 || sRes.V != 0;
        nCurBits++;
        if( uiCodeVertex )
        {
          xCheckParamBits( sRes.Y, nFLCbits, nCurBits );
          xCheckParamBits( sRes.U, nFLCbits, nCurBits );
          xCheckParamBits( sRes.V, nFLCbits, nCurBits );
        }
      }
    }
  }
}

Void TEncCavlc::xCheckParamBits( Int param, Int rParam, Int &nBits)
{
  Int codeNumber = abs(param);
  Int codeQuotient = codeNumber >> rParam;
  Int qLen; 

  UInt uiLength = 1;
  UInt uiTemp = ++codeQuotient;
    
  while( 1 != uiTemp )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }

  qLen  = (uiLength >> 1);
  qLen += ((uiLength+1) >> 1);

  nBits += qLen; 
  nBits += rParam; 
  if (abs(param))
    nBits++; 
}
#endif
#if VPS_VUI_BSP_HRD_PARAMS
Void TEncCavlc::codeVpsVuiBspHrdParams(TComVPS * const vps)
{
  WRITE_UVLC( vps->getVpsNumAddHrdParams(), "vps_num_add_hrd_params" );
  for( Int i = vps->getNumHrdParameters(), j = 0; i < vps->getNumHrdParameters() + vps->getVpsNumAddHrdParams(); i++, j++ ) // j = i - vps->getNumHrdParameters()
  {
    if( i > 0 )
    {
      WRITE_FLAG( vps->getCprmsAddPresentFlag(j), "cprms_add_present_flag[i]" );
    }
    WRITE_UVLC( vps->getNumSubLayerHrdMinus1(j), "num_sub_layer_hrd_minus1[i]" );
    codeHrdParameters(vps->getBspHrd(j), i == 0 ? true : vps->getCprmsAddPresentFlag(j), vps->getNumSubLayerHrdMinus1(j));
  }
  for( Int h = 1; h < vps->getNumOutputLayerSets(); h++ )
  {
    Int lsIdx = vps->getOutputLayerSetIdx( h );
    WRITE_UVLC( vps->getNumSignalledPartitioningSchemes(h), "num_signalled_partitioning_schemes[h]");
    for( Int j = 0; j < vps->getNumSignalledPartitioningSchemes(h); j++ )
    {
      WRITE_UVLC( vps->getNumPartitionsInSchemeMinus1(h, j), "num_partitions_in_scheme_minus1[h][j]" );
      for( Int k = 0; k <= vps->getNumPartitionsInSchemeMinus1(h, j); k++ )
      {
        for( Int r = 0; r < vps->getNumLayersInIdList( lsIdx ); r++ )
        {
          WRITE_FLAG( vps->getLayerIncludedInPartitionFlag(h, j, k, r), "layer_included_in_partition_flag[h][j][k][r]" );
        }
      }
    }
    for( Int i = 0; i < vps->getNumSignalledPartitioningSchemes(h) + 1; i++ )
    {
      for( Int t = 0; t <= vps->getMaxSLayersInLayerSetMinus1(lsIdx); t++ )
      {
        WRITE_UVLC(vps->getNumBspSchedulesMinus1(h, i, t), "num_bsp_schedules_minus1[h][i][t]");
        for( Int j = 0; j <= vps->getNumBspSchedulesMinus1(h, i, t); j++ )
        {
          for( Int k = 0; k < vps->getNumPartitionsInSchemeMinus1(h, i); k++ )
          {
            WRITE_UVLC( vps->getBspHrdIdx(h, i, t, j, k),   "bsp_comb_hrd_idx[h][i][t][j][k]");
            WRITE_UVLC( vps->getBspSchedIdx(h, i, t, j, k), "bsp_comb_sched_idx[h][i][t][j][k]");
          }
        }
      }
    }
  }
}
#endif
#endif
//! \}
