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

#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/SEI.h"
#include "TLibCommon/TComSlice.h"
#include "SEIwrite.h"

//! \ingroup TLibEncoder
//! \{

#if ENC_DEC_TRACE
Void  xTraceSEIHeader()
{
  fprintf( g_hTrace, "=========== SEI message ===========\n");
}

Void  xTraceSEIMessageType(SEI::PayloadType payloadType)
{
  switch (payloadType)
  {
  case SEI::DECODED_PICTURE_HASH:
    fprintf( g_hTrace, "=========== Decoded picture hash SEI message ===========\n");
    break;
  case SEI::USER_DATA_UNREGISTERED:
    fprintf( g_hTrace, "=========== User Data Unregistered SEI message ===========\n");
    break;
  case SEI::ACTIVE_PARAMETER_SETS:
    fprintf( g_hTrace, "=========== Active Parameter sets SEI message ===========\n");
    break;
  case SEI::BUFFERING_PERIOD:
    fprintf( g_hTrace, "=========== Buffering period SEI message ===========\n");
    break;
  case SEI::PICTURE_TIMING:
    fprintf( g_hTrace, "=========== Picture timing SEI message ===========\n");
    break;
  case SEI::RECOVERY_POINT:
    fprintf( g_hTrace, "=========== Recovery point SEI message ===========\n");
    break;
  case SEI::FRAME_PACKING:
    fprintf( g_hTrace, "=========== Frame Packing Arrangement SEI message ===========\n");
    break;
  case SEI::DISPLAY_ORIENTATION:
    fprintf( g_hTrace, "=========== Display Orientation SEI message ===========\n");
    break;
  case SEI::TEMPORAL_LEVEL0_INDEX:
    fprintf( g_hTrace, "=========== Temporal Level Zero Index SEI message ===========\n");
    break;
  case SEI::REGION_REFRESH_INFO:
    fprintf( g_hTrace, "=========== Gradual Decoding Refresh Information SEI message ===========\n");
    break;
  case SEI::DECODING_UNIT_INFO:
    fprintf( g_hTrace, "=========== Decoding Unit Information SEI message ===========\n");
    break;
  case SEI::TONE_MAPPING_INFO:
    fprintf( g_hTrace, "=========== Tone Mapping Info SEI message ===========\n");
    break;
#if P0050_KNEE_FUNCTION_SEI
  case SEI::KNEE_FUNCTION_INFO:
    fprintf( g_hTrace, "=========== Knee Function Information SEI message ===========\n");
    break;
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  case SEI::COLOUR_REMAPPING_INFO:
    fprintf( g_hTrace, "=========== Colour Remapping Information SEI message ===========\n");
    break;
#endif
  case SEI::SOP_DESCRIPTION:
    fprintf( g_hTrace, "=========== SOP Description SEI message ===========\n");
    break;
  case SEI::SCALABLE_NESTING:
    fprintf( g_hTrace, "=========== Scalable Nesting SEI message ===========\n");
    break;
#if SVC_EXTENSION
#if LAYERS_NOT_PRESENT_SEI
  case SEI::LAYERS_NOT_PRESENT:
    fprintf( g_hTrace, "=========== Layers Present SEI message ===========\n");
    break;
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
  case SEI::INTER_LAYER_CONSTRAINED_TILE_SETS:
    fprintf( g_hTrace, "=========== Inter Layer Constrained Tile Sets SEI message ===========\n");
    break;
#endif
#if SUB_BITSTREAM_PROPERTY_SEI
  case SEI::SUB_BITSTREAM_PROPERTY:
    fprintf( g_hTrace, "=========== Sub-bitstream property SEI message ===========\n");
    break;
#endif
#if O0164_MULTI_LAYER_HRD
  case SEI::BSP_NESTING:
    fprintf( g_hTrace, "=========== Bitstream partition nesting SEI message ===========\n");
    break;
  case SEI::BSP_INITIAL_ARRIVAL_TIME:
    fprintf( g_hTrace, "=========== Bitstream parition initial arrival time SEI message ===========\n");
    break;
#if !REMOVE_BSP_HRD_SEI
  case SEI::BSP_HRD:
    fprintf( g_hTrace, "=========== Bitstream parition HRD parameters SEI message ===========\n");
    break;
#endif
#endif
#if Q0078_ADD_LAYER_SETS
  case SEI::OUTPUT_LAYER_SET_NESTING:
    fprintf(g_hTrace, "=========== Output layer set nesting SEI message ===========\n");
    break;
  case SEI::VPS_REWRITING:
    fprintf(g_hTrace, "=========== VPS rewriting SEI message ===========\n");
    break;
#endif
#endif //SVC_EXTENSION
  default:
    fprintf( g_hTrace, "=========== Unknown SEI message ===========\n");
    break;
  }
}
#endif

#if O0164_MULTI_LAYER_HRD
#if VPS_VUI_BSP_HRD_PARAMS
void SEIWriter::xWriteSEIpayloadData(TComBitIf& bs, const SEI& sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting* nestingSeiPtr, const SEIBspNesting* bspNestingSeiPtr)
#else
void SEIWriter::xWriteSEIpayloadData(TComBitIf& bs, const SEI& sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting& nestingSei, const SEIBspNesting& bspNestingSei)
#endif
#else
void SEIWriter::xWriteSEIpayloadData(TComBitIf& bs, const SEI& sei, TComSPS *sps)
#endif
{
#if VPS_VUI_BSP_HRD_PARAMS
  SEIScalableNesting nestingSei;
  SEIBspNesting      bspNestingSei;
  if( nestingSeiPtr )
  {
    nestingSei = *nestingSeiPtr;
  }
  if( bspNestingSeiPtr )
  {
    bspNestingSei = *bspNestingSeiPtr;
  }
#endif
  switch (sei.payloadType())
  {
  case SEI::USER_DATA_UNREGISTERED:
    xWriteSEIuserDataUnregistered(*static_cast<const SEIuserDataUnregistered*>(&sei));
    break;
  case SEI::ACTIVE_PARAMETER_SETS:
    xWriteSEIActiveParameterSets(*static_cast<const SEIActiveParameterSets*>(& sei)); 
    break; 
  case SEI::DECODED_PICTURE_HASH:
    xWriteSEIDecodedPictureHash(*static_cast<const SEIDecodedPictureHash*>(&sei));
    break;
#if VPS_VUI_BSP_HRD_PARAMS
  case SEI::DECODING_UNIT_INFO:
    xWriteSEIDecodingUnitInfo(*static_cast<const SEIDecodingUnitInfo*>(& sei), sps, nestingSeiPtr, bspNestingSeiPtr, vps);
    break;
  case SEI::BUFFERING_PERIOD:
    xWriteSEIBufferingPeriod(*static_cast<const SEIBufferingPeriod*>(&sei), sps, nestingSeiPtr, bspNestingSeiPtr, vps);
    break;
  case SEI::PICTURE_TIMING:
    xWriteSEIPictureTiming(*static_cast<const SEIPictureTiming*>(&sei), sps, nestingSeiPtr, bspNestingSeiPtr, vps);
    break;
#else
  case SEI::DECODING_UNIT_INFO:
    xWriteSEIDecodingUnitInfo(*static_cast<const SEIDecodingUnitInfo*>(& sei), sps);
    break;
  case SEI::BUFFERING_PERIOD:
    xWriteSEIBufferingPeriod(*static_cast<const SEIBufferingPeriod*>(&sei), sps);
    break;
  case SEI::PICTURE_TIMING:
    xWriteSEIPictureTiming(*static_cast<const SEIPictureTiming*>(&sei), sps);
    break;
#endif
  case SEI::RECOVERY_POINT:
    xWriteSEIRecoveryPoint(*static_cast<const SEIRecoveryPoint*>(&sei));
    break;
  case SEI::FRAME_PACKING:
    xWriteSEIFramePacking(*static_cast<const SEIFramePacking*>(&sei));
    break;
  case SEI::DISPLAY_ORIENTATION:
    xWriteSEIDisplayOrientation(*static_cast<const SEIDisplayOrientation*>(&sei));
    break;
  case SEI::TEMPORAL_LEVEL0_INDEX:
    xWriteSEITemporalLevel0Index(*static_cast<const SEITemporalLevel0Index*>(&sei));
    break;
  case SEI::REGION_REFRESH_INFO:
    xWriteSEIGradualDecodingRefreshInfo(*static_cast<const SEIGradualDecodingRefreshInfo*>(&sei));
    break;
  case SEI::TONE_MAPPING_INFO:
    xWriteSEIToneMappingInfo(*static_cast<const SEIToneMappingInfo*>(&sei));
    break;
#if P0050_KNEE_FUNCTION_SEI
  case SEI::KNEE_FUNCTION_INFO:
    xWriteSEIKneeFunctionInfo(*static_cast<const SEIKneeFunctionInfo*>(&sei));
    break;
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  case SEI::COLOUR_REMAPPING_INFO:
    xWriteSEIColourRemappingInfo(*static_cast<const SEIColourRemappingInfo*>(&sei));
    break;
#endif
  case SEI::SOP_DESCRIPTION:
    xWriteSEISOPDescription(*static_cast<const SEISOPDescription*>(&sei));
    break;
  case SEI::SCALABLE_NESTING:
#if O0164_MULTI_LAYER_HRD
    xWriteSEIScalableNesting(bs, *static_cast<const SEIScalableNesting*>(&sei), vps, sps);
#else
    xWriteSEIScalableNesting(bs, *static_cast<const SEIScalableNesting*>(&sei), sps);
#endif
    break;
#if SVC_EXTENSION
#if LAYERS_NOT_PRESENT_SEI
  case SEI::LAYERS_NOT_PRESENT:
    xWriteSEILayersNotPresent(*static_cast<const SEILayersNotPresent*>(&sei));
    break;
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
  case SEI::INTER_LAYER_CONSTRAINED_TILE_SETS:
    xWriteSEIInterLayerConstrainedTileSets(*static_cast<const SEIInterLayerConstrainedTileSets*>(&sei));
    break;
#endif
#if SUB_BITSTREAM_PROPERTY_SEI
   case SEI::SUB_BITSTREAM_PROPERTY:
     xWriteSEISubBitstreamProperty(*static_cast<const SEISubBitstreamProperty*>(&sei));
     break;
#endif
#if O0164_MULTI_LAYER_HRD
   case SEI::BSP_NESTING:
     xWriteSEIBspNesting(bs, *static_cast<const SEIBspNesting*>(&sei), vps, sps, nestingSei);
     break;
   case SEI::BSP_INITIAL_ARRIVAL_TIME:
     xWriteSEIBspInitialArrivalTime(*static_cast<const SEIBspInitialArrivalTime*>(&sei), vps, sps, nestingSei, bspNestingSei);
     break;
#if !REMOVE_BSP_HRD_SEI
   case SEI::BSP_HRD:
     xWriteSEIBspHrd(*static_cast<const SEIBspHrd*>(&sei), sps, nestingSei);
     break;
#endif
#endif
#if Q0078_ADD_LAYER_SETS
   case SEI::OUTPUT_LAYER_SET_NESTING:
     xWriteSEIOutputLayerSetNesting(bs, *static_cast<const SEIOutputLayerSetNesting*>(&sei), vps, sps);
     break;
   case SEI::VPS_REWRITING:
     xWriteSEIVPSRewriting(*static_cast<const SEIVPSRewriting*>(&sei));
     break;
#endif
#if Q0189_TMVP_CONSTRAINTS
   case SEI::TMVP_CONSTRAINTS:
     xWriteSEITMVPConstraints(*static_cast<const SEITMVPConstrains*>(&sei));
     break;
#endif
#if Q0247_FRAME_FIELD_INFO
   case SEI::FRAME_FIELD_INFO:
     xWriteSEIFrameFieldInfo(*static_cast<const SEIFrameFieldInfo*>(&sei));
     break;
#endif
#endif //SVC_EXTENSION
  default:
    assert(!"Unhandled SEI message");
  }
}

/**
 * marshal a single SEI message sei, storing the marshalled representation
 * in bitstream bs.
 */
#if O0164_MULTI_LAYER_HRD
Void SEIWriter::writeSEImessage(TComBitIf& bs, const SEI& sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei)
#else
Void SEIWriter::writeSEImessage(TComBitIf& bs, const SEI& sei, TComSPS *sps)
#endif
{
  /* calculate how large the payload data is */
  /* TODO: this would be far nicer if it used vectored buffers */
  TComBitCounter bs_count;
  bs_count.resetBits();
  setBitstream(&bs_count);


#if ENC_DEC_TRACE
  Bool traceEnable = g_HLSTraceEnable;
  g_HLSTraceEnable = false;
#endif
#if O0164_MULTI_LAYER_HRD
#if VPS_VUI_BSP_HRD_PARAMS
  xWriteSEIpayloadData(bs_count, sei, vps, sps, nestingSei, bspNestingSei);
#else
  xWriteSEIpayloadData(bs_count, sei, vps, sps, *nestingSei, *bspNestingSei);
#endif
#else
  xWriteSEIpayloadData(bs_count, sei, sps);
#endif
#if ENC_DEC_TRACE
  g_HLSTraceEnable = traceEnable;
#endif

  UInt payload_data_num_bits = bs_count.getNumberOfWrittenBits();
  assert(0 == payload_data_num_bits % 8);

  setBitstream(&bs);

#if ENC_DEC_TRACE
  if (g_HLSTraceEnable)
  xTraceSEIHeader();
#endif

  UInt payloadType = sei.payloadType();
  for (; payloadType >= 0xff; payloadType -= 0xff)
  {
    WRITE_CODE(0xff, 8, "payload_type");
  }
  WRITE_CODE(payloadType, 8, "payload_type");

  UInt payloadSize = payload_data_num_bits/8;
  for (; payloadSize >= 0xff; payloadSize -= 0xff)
  {
    WRITE_CODE(0xff, 8, "payload_size");
  }
  WRITE_CODE(payloadSize, 8, "payload_size");

  /* payloadData */
#if ENC_DEC_TRACE
  if (g_HLSTraceEnable)
  xTraceSEIMessageType(sei.payloadType());
#endif

#if O0164_MULTI_LAYER_HRD
#if VPS_VUI_BSP_HRD_PARAMS
  xWriteSEIpayloadData(bs, sei, vps, sps, nestingSei, bspNestingSei);
#else
  xWriteSEIpayloadData(bs, sei, vps, sps, *nestingSei, *bspNestingSei);
#endif
#else
  xWriteSEIpayloadData(bs, sei, sps);
#endif
}

/**
 * marshal a user_data_unregistered SEI message sei, storing the marshalled
 * representation in bitstream bs.
 */
Void SEIWriter::xWriteSEIuserDataUnregistered(const SEIuserDataUnregistered &sei)
{
  for (UInt i = 0; i < 16; i++)
  {
    WRITE_CODE(sei.uuid_iso_iec_11578[i], 8 , "sei.uuid_iso_iec_11578[i]");
  }

  for (UInt i = 0; i < sei.userDataLength; i++)
  {
    WRITE_CODE(sei.userData[i], 8 , "user_data");
  }
}

/**
 * marshal a decoded picture hash SEI message, storing the marshalled
 * representation in bitstream bs.
 */
Void SEIWriter::xWriteSEIDecodedPictureHash(const SEIDecodedPictureHash& sei)
{
  UInt val;

  WRITE_CODE(sei.method, 8, "hash_type");

  for(Int yuvIdx = 0; yuvIdx < 3; yuvIdx++)
  {
    if(sei.method == SEIDecodedPictureHash::MD5)
    {
      for (UInt i = 0; i < 16; i++)
      {
        WRITE_CODE(sei.digest[yuvIdx][i], 8, "picture_md5");
      }
    }
    else if(sei.method == SEIDecodedPictureHash::CRC)
    {
      val = (sei.digest[yuvIdx][0] << 8)  + sei.digest[yuvIdx][1];
      WRITE_CODE(val, 16, "picture_crc");
    }
    else if(sei.method == SEIDecodedPictureHash::CHECKSUM)
    {
      val = (sei.digest[yuvIdx][0] << 24)  + (sei.digest[yuvIdx][1] << 16) + (sei.digest[yuvIdx][2] << 8) + sei.digest[yuvIdx][3];
      WRITE_CODE(val, 32, "picture_checksum");
    }
  }
}

Void SEIWriter::xWriteSEIActiveParameterSets(const SEIActiveParameterSets& sei)
{
  WRITE_CODE(sei.activeVPSId,     4,         "active_video_parameter_set_id");
  WRITE_FLAG(sei.m_selfContainedCvsFlag,     "self_contained_cvs_flag");
  WRITE_FLAG(sei.m_noParameterSetUpdateFlag, "no_parameter_set_update_flag");
  WRITE_UVLC(sei.numSpsIdsMinus1,    "num_sps_ids_minus1");

  assert (sei.activeSeqParameterSetId.size() == (sei.numSpsIdsMinus1 + 1));

  for (Int i = 0; i < sei.activeSeqParameterSetId.size(); i++)
  {
    WRITE_UVLC(sei.activeSeqParameterSetId[i], "active_seq_parameter_set_id"); 
  }
#if R0247_SEI_ACTIVE
  for (Int i = 1; i < sei.activeSeqParameterSetId.size(); i++)
  {
    WRITE_UVLC(sei.layerSpsIdx[i], "layer_sps_idx"); 
  }
#endif
  xWriteByteAlign();
}

#if VPS_VUI_BSP_HRD_PARAMS
Void SEIWriter::xWriteSEIDecodingUnitInfo(const SEIDecodingUnitInfo& sei, TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei, TComVPS *vps)
#else
Void SEIWriter::xWriteSEIDecodingUnitInfo(const SEIDecodingUnitInfo& sei, TComSPS *sps)
#endif
{
#if VPS_VUI_BSP_HRD_PARAMS
  TComHRD *hrd;
  if( bspNestingSei )   // If DU info SEI contained inside a BSP nesting SEI message
  {
    assert( nestingSei );
    Int psIdx = bspNestingSei->m_seiPartitioningSchemeIdx;
    Int seiOlsIdx = bspNestingSei->m_seiOlsIdx;
    Int maxTemporalId = nestingSei->m_nestingMaxTemporalIdPlus1[0] - 1;
    Int maxValues = vps->getNumBspSchedulesMinus1(seiOlsIdx, psIdx, maxTemporalId) + 1;
    std::vector<Int> hrdIdx(maxValues, 0);
    std::vector<TComHRD *> hrdVec;
    std::vector<Int> syntaxElemLen(maxValues, 0);
    for(Int i = 0; i < maxValues; i++)
    {
      hrdIdx[i] = vps->getBspHrdIdx( seiOlsIdx, psIdx, maxTemporalId, i, bspNestingSei->m_bspIdx);
      hrdVec.push_back(vps->getBspHrd(hrdIdx[i]));
    
      syntaxElemLen[i] = hrdVec[i]->getInitialCpbRemovalDelayLengthMinus1() + 1;
      if ( !(hrdVec[i]->getNalHrdParametersPresentFlag() || hrdVec[i]->getVclHrdParametersPresentFlag()) )
      {
        assert( syntaxElemLen[i] == 24 ); // Default of value init_cpb_removal_delay_length_minus1 is 23
      }
      if( i > 0 )
      {
        assert( hrdVec[i]->getSubPicCpbParamsPresentFlag()    == hrdVec[i-1]->getSubPicCpbParamsPresentFlag() );
        assert( hrdVec[i]->getSubPicCpbParamsInPicTimingSEIFlag()   == hrdVec[i-1]->getSubPicCpbParamsInPicTimingSEIFlag() );
        assert( hrdVec[i]->getDpbOutputDelayDuLengthMinus1()  == hrdVec[i-1]->getDpbOutputDelayDuLengthMinus1() );
        // To be done: Check CpbDpbDelaysPresentFlag
      }
    }
    hrd = hrdVec[0];
  }
  else
  {
    TComVUI *vui = sps->getVuiParameters();
    hrd = vui->getHrdParameters();
  }
#else
  TComVUI *vui = sps->getVuiParameters();
  TComHrd *hrd = vui->getHrdParameters();
#endif
  WRITE_UVLC(sei.m_decodingUnitIdx, "decoding_unit_idx");
  if(hrd->getSubPicCpbParamsInPicTimingSEIFlag())
  {
    WRITE_CODE( sei.m_duSptCpbRemovalDelay, (hrd->getDuCpbRemovalDelayLengthMinus1() + 1), "du_spt_cpb_removal_delay");
  }
  WRITE_FLAG( sei.m_dpbOutputDuDelayPresentFlag, "dpb_output_du_delay_present_flag");
  if(sei.m_dpbOutputDuDelayPresentFlag)
  {
    WRITE_CODE(sei.m_picSptDpbOutputDuDelay,hrd->getDpbOutputDelayDuLengthMinus1() + 1, "pic_spt_dpb_output_du_delay");
  }
  xWriteByteAlign();
}

#if VPS_VUI_BSP_HRD_PARAMS
Void SEIWriter::xWriteSEIBufferingPeriod(const SEIBufferingPeriod& sei, TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei, TComVPS *vps)
#else
Void SEIWriter::xWriteSEIBufferingPeriod(const SEIBufferingPeriod& sei, TComSPS *sps)
#endif
{
  Int i, nalOrVcl;
#if VPS_VUI_BSP_HRD_PARAMS
  TComHRD *hrd;
  if( bspNestingSei )   // If BP SEI contained inside a BSP nesting SEI message
  {
    assert( nestingSei );
    Int psIdx = bspNestingSei->m_seiPartitioningSchemeIdx;
    Int seiOlsIdx = bspNestingSei->m_seiOlsIdx;
    Int maxTemporalId = nestingSei->m_nestingMaxTemporalIdPlus1[0] - 1;
    Int maxValues = vps->getNumBspSchedulesMinus1(seiOlsIdx, psIdx, maxTemporalId) + 1;
    std::vector<Int> hrdIdx(maxValues, 0);
    std::vector<TComHRD *> hrdVec;
    std::vector<Int> syntaxElemLen(maxValues, 0);
    for(i = 0; i < maxValues; i++)
    {
      hrdIdx[i] = vps->getBspHrdIdx( seiOlsIdx, psIdx, maxTemporalId, i, bspNestingSei->m_bspIdx);
      hrdVec.push_back(vps->getBspHrd(hrdIdx[i]));
    
      syntaxElemLen[i] = hrdVec[i]->getInitialCpbRemovalDelayLengthMinus1() + 1;
      if ( !(hrdVec[i]->getNalHrdParametersPresentFlag() || hrdVec[i]->getVclHrdParametersPresentFlag()) )
      {
        assert( syntaxElemLen[i] == 24 ); // Default of value init_cpb_removal_delay_length_minus1 is 23
      }
      if( i > 0 )
      {
        assert( hrdVec[i]->getCpbRemovalDelayLengthMinus1()   == hrdVec[i-1]->getCpbRemovalDelayLengthMinus1() );
        assert( hrdVec[i]->getDpbOutputDelayDuLengthMinus1()  == hrdVec[i-1]->getDpbOutputDelayDuLengthMinus1() );
        assert( hrdVec[i]->getSubPicCpbParamsPresentFlag()    == hrdVec[i-1]->getSubPicCpbParamsPresentFlag() );
      }
    }
    hrd = hrdVec[i];
  }
  else
  {
    TComVUI *vui = sps->getVuiParameters();
    hrd = vui->getHrdParameters();
  }
  // To be done: When contained in an BSP HRD SEI message, the hrd structure is to be chosen differently.
#else
  TComVUI *vui = sps->getVuiParameters();
  TComHRD *hrd = vui->getHrdParameters();
#endif

  WRITE_UVLC( sei.m_bpSeqParameterSetId, "bp_seq_parameter_set_id" );
  if( !hrd->getSubPicCpbParamsPresentFlag() )
  {
    WRITE_FLAG( sei.m_rapCpbParamsPresentFlag, "irap_cpb_params_present_flag" );
  }
  if( sei.m_rapCpbParamsPresentFlag )
  {
    WRITE_CODE( sei.m_cpbDelayOffset, hrd->getCpbRemovalDelayLengthMinus1() + 1, "cpb_delay_offset" );
    WRITE_CODE( sei.m_dpbDelayOffset, hrd->getDpbOutputDelayLengthMinus1()  + 1, "dpb_delay_offset" );
  }
  WRITE_FLAG( sei.m_concatenationFlag, "concatenation_flag");
  WRITE_CODE( sei.m_auCpbRemovalDelayDelta - 1, ( hrd->getCpbRemovalDelayLengthMinus1() + 1 ), "au_cpb_removal_delay_delta_minus1" );
  for( nalOrVcl = 0; nalOrVcl < 2; nalOrVcl ++ )
  {
    if( ( ( nalOrVcl == 0 ) && ( hrd->getNalHrdParametersPresentFlag() ) ) ||
        ( ( nalOrVcl == 1 ) && ( hrd->getVclHrdParametersPresentFlag() ) ) )
    {
      for( i = 0; i < ( hrd->getCpbCntMinus1( 0 ) + 1 ); i ++ )
      {
        WRITE_CODE( sei.m_initialCpbRemovalDelay[i][nalOrVcl],( hrd->getInitialCpbRemovalDelayLengthMinus1() + 1 ) ,           "initial_cpb_removal_delay" );
        WRITE_CODE( sei.m_initialCpbRemovalDelayOffset[i][nalOrVcl],( hrd->getInitialCpbRemovalDelayLengthMinus1() + 1 ),      "initial_cpb_removal_delay_offset" );
        if( hrd->getSubPicCpbParamsPresentFlag() || sei.m_rapCpbParamsPresentFlag )
        {
          WRITE_CODE( sei.m_initialAltCpbRemovalDelay[i][nalOrVcl], ( hrd->getInitialCpbRemovalDelayLengthMinus1() + 1 ) ,     "initial_alt_cpb_removal_delay" );
          WRITE_CODE( sei.m_initialAltCpbRemovalDelayOffset[i][nalOrVcl], ( hrd->getInitialCpbRemovalDelayLengthMinus1() + 1 ),"initial_alt_cpb_removal_delay_offset" );
        }
      }
    }
  }
#if P0138_USE_ALT_CPB_PARAMS_FLAG
  if (sei.m_useAltCpbParamsFlagPresent)
  {
    WRITE_FLAG( sei.m_useAltCpbParamsFlag, "use_alt_cpb_params_flag");
  }
#endif
  xWriteByteAlign();
}
#if VPS_VUI_BSP_HRD_PARAMS
Void SEIWriter::xWriteSEIPictureTiming(const SEIPictureTiming& sei,  TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei, TComVPS *vps)
#else
Void SEIWriter::xWriteSEIPictureTiming(const SEIPictureTiming& sei,  TComSPS *sps)
#endif
{
  Int i;
#if VPS_VUI_BSP_HRD_PARAMS
  TComHRD *hrd;    
  TComVUI *vui = sps->getVuiParameters(); 
  if( bspNestingSei )   // If BP SEI contained inside a BSP nesting SEI message
  {
    assert( nestingSei );
    Int psIdx = bspNestingSei->m_seiPartitioningSchemeIdx;
    Int seiOlsIdx = bspNestingSei->m_seiOlsIdx;
    Int maxTemporalId = nestingSei->m_nestingMaxTemporalIdPlus1[0] - 1;
    Int maxValues = vps->getNumBspSchedulesMinus1(seiOlsIdx, psIdx, maxTemporalId) + 1;
    std::vector<Int> hrdIdx(maxValues, 0);
    std::vector<TComHRD *> hrdVec;
    std::vector<Int> syntaxElemLen(maxValues, 0);
    for(i = 0; i < maxValues; i++)
    {
      hrdIdx[i] = vps->getBspHrdIdx( seiOlsIdx, psIdx, maxTemporalId, i, bspNestingSei->m_bspIdx);
      hrdVec.push_back(vps->getBspHrd(hrdIdx[i]));
    
      syntaxElemLen[i] = hrdVec[i]->getInitialCpbRemovalDelayLengthMinus1() + 1;
      if ( !(hrdVec[i]->getNalHrdParametersPresentFlag() || hrdVec[i]->getVclHrdParametersPresentFlag()) )
      {
        assert( syntaxElemLen[i] == 24 ); // Default of value init_cpb_removal_delay_length_minus1 is 23
      }
      if( i > 0 )
      {
        assert( hrdVec[i]->getSubPicCpbParamsPresentFlag()    == hrdVec[i-1]->getSubPicCpbParamsPresentFlag() );
        assert( hrdVec[i]->getSubPicCpbParamsInPicTimingSEIFlag()   == hrdVec[i-1]->getSubPicCpbParamsInPicTimingSEIFlag() );
        assert( hrdVec[i]->getCpbRemovalDelayLengthMinus1()  == hrdVec[i-1]->getCpbRemovalDelayLengthMinus1() );
        assert( hrdVec[i]->getDpbOutputDelayLengthMinus1()  == hrdVec[i-1]->getDpbOutputDelayLengthMinus1() );
        assert( hrdVec[i]->getDpbOutputDelayDuLengthMinus1()  == hrdVec[i-1]->getDpbOutputDelayDuLengthMinus1() );
        assert( hrdVec[i]->getDuCpbRemovalDelayLengthMinus1()  == hrdVec[i-1]->getDuCpbRemovalDelayLengthMinus1() );
        // To be done: Check CpbDpbDelaysPresentFlag
      }
    }
    hrd = hrdVec[0];
  }
  else
  {
    hrd = vui->getHrdParameters();
  }
  // To be done: When contained in an BSP HRD SEI message, the hrd structure is to be chosen differently.
#else
  TComVUI *vui = sps->getVuiParameters();
  TComHRD *hrd = vui->getHrdParameters();
#endif

  if( vui->getFrameFieldInfoPresentFlag() ) // To be done: Check whether this is the correct invocation of vui when PT SEI contained in BSP nesting SEI
  {
    WRITE_CODE( sei.m_picStruct, 4,              "pic_struct" );
    WRITE_CODE( sei.m_sourceScanType, 2,         "source_scan_type" );
    WRITE_FLAG( sei.m_duplicateFlag ? 1 : 0,     "duplicate_flag" );
  }

  if( hrd->getCpbDpbDelaysPresentFlag() )
  {
    WRITE_CODE( sei.m_auCpbRemovalDelay - 1, ( hrd->getCpbRemovalDelayLengthMinus1() + 1 ),                                         "au_cpb_removal_delay_minus1" );
    WRITE_CODE( sei.m_picDpbOutputDelay, ( hrd->getDpbOutputDelayLengthMinus1() + 1 ),                                          "pic_dpb_output_delay" );
    if(hrd->getSubPicCpbParamsPresentFlag())
    {
      WRITE_CODE(sei.m_picDpbOutputDuDelay, hrd->getDpbOutputDelayDuLengthMinus1()+1, "pic_dpb_output_du_delay" );
    }
    if( hrd->getSubPicCpbParamsPresentFlag() && hrd->getSubPicCpbParamsInPicTimingSEIFlag() )
    {
      WRITE_UVLC( sei.m_numDecodingUnitsMinus1,     "num_decoding_units_minus1" );
      WRITE_FLAG( sei.m_duCommonCpbRemovalDelayFlag, "du_common_cpb_removal_delay_flag" );
      if( sei.m_duCommonCpbRemovalDelayFlag )
      {
        WRITE_CODE( sei.m_duCommonCpbRemovalDelayMinus1, ( hrd->getDuCpbRemovalDelayLengthMinus1() + 1 ),                       "du_common_cpb_removal_delay_minus1" );
      }
      for( i = 0; i <= sei.m_numDecodingUnitsMinus1; i ++ )
      {
        WRITE_UVLC( sei.m_numNalusInDuMinus1[ i ],  "num_nalus_in_du_minus1");
        if( ( !sei.m_duCommonCpbRemovalDelayFlag ) && ( i < sei.m_numDecodingUnitsMinus1 ) )
        {
          WRITE_CODE( sei.m_duCpbRemovalDelayMinus1[ i ], ( hrd->getDuCpbRemovalDelayLengthMinus1() + 1 ),                        "du_cpb_removal_delay_minus1" );
        }
      }
    }
  }
  xWriteByteAlign();
}
Void SEIWriter::xWriteSEIRecoveryPoint(const SEIRecoveryPoint& sei)
{
  WRITE_SVLC( sei.m_recoveryPocCnt,    "recovery_poc_cnt"    );
  WRITE_FLAG( sei.m_exactMatchingFlag, "exact_matching_flag" );
  WRITE_FLAG( sei.m_brokenLinkFlag,    "broken_link_flag"    );
  xWriteByteAlign();
}
Void SEIWriter::xWriteSEIFramePacking(const SEIFramePacking& sei)
{
  WRITE_UVLC( sei.m_arrangementId,                  "frame_packing_arrangement_id" );
  WRITE_FLAG( sei.m_arrangementCancelFlag,          "frame_packing_arrangement_cancel_flag" );

  if( sei.m_arrangementCancelFlag == 0 ) {
    WRITE_CODE( sei.m_arrangementType, 7,           "frame_packing_arrangement_type" );

    WRITE_FLAG( sei.m_quincunxSamplingFlag,         "quincunx_sampling_flag" );
    WRITE_CODE( sei.m_contentInterpretationType, 6, "content_interpretation_type" );
    WRITE_FLAG( sei.m_spatialFlippingFlag,          "spatial_flipping_flag" );
    WRITE_FLAG( sei.m_frame0FlippedFlag,            "frame0_flipped_flag" );
    WRITE_FLAG( sei.m_fieldViewsFlag,               "field_views_flag" );
    WRITE_FLAG( sei.m_currentFrameIsFrame0Flag,     "current_frame_is_frame0_flag" );

    WRITE_FLAG( sei.m_frame0SelfContainedFlag,      "frame0_self_contained_flag" );
    WRITE_FLAG( sei.m_frame1SelfContainedFlag,      "frame1_self_contained_flag" );

    if(sei.m_quincunxSamplingFlag == 0 && sei.m_arrangementType != 5)
    {
      WRITE_CODE( sei.m_frame0GridPositionX, 4,     "frame0_grid_position_x" );
      WRITE_CODE( sei.m_frame0GridPositionY, 4,     "frame0_grid_position_y" );
      WRITE_CODE( sei.m_frame1GridPositionX, 4,     "frame1_grid_position_x" );
      WRITE_CODE( sei.m_frame1GridPositionY, 4,     "frame1_grid_position_y" );
    }

    WRITE_CODE( sei.m_arrangementReservedByte, 8,   "frame_packing_arrangement_reserved_byte" );
    WRITE_FLAG( sei.m_arrangementPersistenceFlag,   "frame_packing_arrangement_persistence_flag" );
  }

  WRITE_FLAG( sei.m_upsampledAspectRatio,           "upsampled_aspect_ratio" );

  xWriteByteAlign();
}

Void SEIWriter::xWriteSEIToneMappingInfo(const SEIToneMappingInfo& sei)
{
  Int i;
  WRITE_UVLC( sei.m_toneMapId,                    "tone_map_id" );
  WRITE_FLAG( sei.m_toneMapCancelFlag,            "tone_map_cancel_flag" );
  if( !sei.m_toneMapCancelFlag ) 
  {
    WRITE_FLAG( sei.m_toneMapPersistenceFlag,     "tone_map_persistence_flag" );
    WRITE_CODE( sei.m_codedDataBitDepth,    8,    "coded_data_bit_depth" );
    WRITE_CODE( sei.m_targetBitDepth,       8,    "target_bit_depth" );
    WRITE_UVLC( sei.m_modelId,                    "model_id" );
    switch(sei.m_modelId)
    {
    case 0:
      {
        WRITE_CODE( sei.m_minValue,  32,        "min_value" );
        WRITE_CODE( sei.m_maxValue, 32,         "max_value" );
        break;
      }
    case 1:
      {
        WRITE_CODE( sei.m_sigmoidMidpoint, 32,  "sigmoid_midpoint" );
        WRITE_CODE( sei.m_sigmoidWidth,    32,  "sigmoid_width"    );
        break;
      }
    case 2:
      {
        UInt num = 1u << sei.m_targetBitDepth;
        for(i = 0; i < num; i++)
        {
          WRITE_CODE( sei.m_startOfCodedInterval[i], (( sei.m_codedDataBitDepth + 7 ) >> 3 ) << 3,  "start_of_coded_interval" );
        }
        break;
      }
    case 3:
      {
        WRITE_CODE( sei.m_numPivots, 16,          "num_pivots" );
        for(i = 0; i < sei.m_numPivots; i++ )
        {
          WRITE_CODE( sei.m_codedPivotValue[i], (( sei.m_codedDataBitDepth + 7 ) >> 3 ) << 3,       "coded_pivot_value" );
          WRITE_CODE( sei.m_targetPivotValue[i], (( sei.m_targetBitDepth + 7 ) >> 3 ) << 3,         "target_pivot_value");
        }
        break;
      }
    case 4:
      {
        WRITE_CODE( sei.m_cameraIsoSpeedIdc,    8,    "camera_iso_speed_idc" );
        if( sei.m_cameraIsoSpeedIdc == 255) //Extended_ISO
        {
          WRITE_CODE( sei.m_cameraIsoSpeedValue,    32,    "camera_iso_speed_value" );
        }
        WRITE_CODE( sei.m_exposureIndexIdc,     8,    "exposure_index_idc" );
        if( sei.m_exposureIndexIdc == 255) //Extended_ISO
        {
          WRITE_CODE( sei.m_exposureIndexValue,     32,    "exposure_index_value" );
        }
        WRITE_FLAG( sei.m_exposureCompensationValueSignFlag,           "exposure_compensation_value_sign_flag" );
        WRITE_CODE( sei.m_exposureCompensationValueNumerator,     16,  "exposure_compensation_value_numerator" );
        WRITE_CODE( sei.m_exposureCompensationValueDenomIdc,      16,  "exposure_compensation_value_denom_idc" );
        WRITE_CODE( sei.m_refScreenLuminanceWhite,                32,  "ref_screen_luminance_white" );
        WRITE_CODE( sei.m_extendedRangeWhiteLevel,                32,  "extended_range_white_level" );
        WRITE_CODE( sei.m_nominalBlackLevelLumaCodeValue,         16,  "nominal_black_level_luma_code_value" );
        WRITE_CODE( sei.m_nominalWhiteLevelLumaCodeValue,         16,  "nominal_white_level_luma_code_value" );
        WRITE_CODE( sei.m_extendedWhiteLevelLumaCodeValue,        16,  "extended_white_level_luma_code_value" );
        break;
      }
    default:
      {
        assert(!"Undefined SEIToneMapModelId");
        break;
      }
    }//switch m_modelId
  }//if(!sei.m_toneMapCancelFlag)

  xWriteByteAlign();
}
#if P0050_KNEE_FUNCTION_SEI
Void SEIWriter::xWriteSEIKneeFunctionInfo(const SEIKneeFunctionInfo &sei)
{
  WRITE_UVLC( sei.m_kneeId, "knee_function_id" );
  WRITE_FLAG( sei.m_kneeCancelFlag, "knee_function_cancel_flag" ); 
  if ( !sei.m_kneeCancelFlag )
  {
    WRITE_FLAG( sei.m_kneePersistenceFlag, "knee_function_persistence_flag" );
    WRITE_FLAG( sei.m_kneeMappingFlag, "mapping_flag" );
    WRITE_CODE( (UInt)sei.m_kneeInputDrange , 32,  "input_d_range" );
    WRITE_CODE( (UInt)sei.m_kneeInputDispLuminance, 32,  "input_disp_luminance" );
    WRITE_CODE( (UInt)sei.m_kneeOutputDrange, 32,  "output_d_range" );
    WRITE_CODE( (UInt)sei.m_kneeOutputDispLuminance, 32,  "output_disp_luminance" );
    WRITE_UVLC( sei.m_kneeNumKneePointsMinus1, "num_knee_points_minus1" );
    for(Int i = 0; i <= sei.m_kneeNumKneePointsMinus1; i++ )
    {
      WRITE_CODE( (UInt)sei.m_kneeInputKneePoint[i], 10,"input_knee_point" );
      WRITE_CODE( (UInt)sei.m_kneeOutputKneePoint[i], 10, "output_knee_point" );
    }
  }
  xWriteByteAlign();
}
#endif
#if Q0074_COLOUR_REMAPPING_SEI
Void SEIWriter::xWriteSEIColourRemappingInfo(const SEIColourRemappingInfo& sei)
{
  WRITE_UVLC( sei.m_colourRemapId,                             "colour_remap_id" );
  WRITE_FLAG( sei.m_colourRemapCancelFlag,                     "colour_remap_cancel_flag" );
  if( !sei.m_colourRemapCancelFlag ) 
  {
    WRITE_FLAG( sei.m_colourRemapPersistenceFlag,              "colour_remap_persistence_flag" );
    WRITE_FLAG( sei.m_colourRemapVideoSignalInfoPresentFlag,   "colour_remap_video_signal_info_present_flag" );
    if ( sei.m_colourRemapVideoSignalInfoPresentFlag )
    {
      WRITE_FLAG( sei.m_colourRemapFullRangeFlag,              "colour_remap_full_range_flag" );
      WRITE_CODE( sei.m_colourRemapPrimaries,               8, "colour_remap_primaries" );
      WRITE_CODE( sei.m_colourRemapTransferFunction,        8, "colour_remap_transfer_function" );
      WRITE_CODE( sei.m_colourRemapMatrixCoefficients,      8, "colour_remap_matrix_coefficients" );
    }
    WRITE_CODE( sei.m_colourRemapInputBitDepth,             8, "colour_remap_input_bit_depth" );
    WRITE_CODE( sei.m_colourRemapBitDepth,                  8, "colour_remap_bit_depth" );
    for( Int c=0 ; c<3 ; c++ )
    {
      WRITE_CODE( sei.m_preLutNumValMinus1[c],              8, "pre_lut_num_val_minus1[c]" );
      if( sei.m_preLutNumValMinus1[c]>0 )
        for( Int i=0 ; i<=sei.m_preLutNumValMinus1[c] ; i++ )
        {
          WRITE_CODE( sei.m_preLutCodedValue[c][i], (( sei.m_colourRemapInputBitDepth + 7 ) >> 3 ) << 3, "pre_lut_coded_value[c][i]" );
          WRITE_CODE( sei.m_preLutTargetValue[c][i], (( sei.m_colourRemapBitDepth + 7 ) >> 3 ) << 3, "pre_lut_target_value[c][i]" );
        }
    }
    WRITE_FLAG( sei.m_colourRemapMatrixPresentFlag,            "colour_remap_matrix_present_flag" );
    if( sei.m_colourRemapMatrixPresentFlag )
    {
      WRITE_CODE( sei.m_log2MatrixDenom,                    4, "log2_matrix_denom" );
      for( Int c=0 ; c<3 ; c++ )
        for( Int i=0 ; i<3 ; i++ )
          WRITE_SVLC( sei.m_colourRemapCoeffs[c][i],           "colour_remap_coeffs[c][i]" );
    }

    for( Int c=0 ; c<3 ; c++ )
    {
      WRITE_CODE( sei.m_postLutNumValMinus1[c],             8, "m_postLutNumValMinus1[c]" );
      if( sei.m_postLutNumValMinus1[c]>0 )
        for( Int i=0 ; i<=sei.m_postLutNumValMinus1[c] ; i++ )
        {
          WRITE_CODE( sei.m_postLutCodedValue[c][i], (( sei.m_colourRemapBitDepth + 7 ) >> 3 ) << 3, "post_lut_coded_value[c][i]" );       
          WRITE_CODE( sei.m_postLutTargetValue[c][i], (( sei.m_colourRemapBitDepth + 7 ) >> 3 ) << 3, "post_lut_target_value[c][i]" );
        }
    }
  }
  xWriteByteAlign();
}
#endif

Void SEIWriter::xWriteSEIDisplayOrientation(const SEIDisplayOrientation &sei)
{
  WRITE_FLAG( sei.cancelFlag,           "display_orientation_cancel_flag" );
  if( !sei.cancelFlag )
  {
    WRITE_FLAG( sei.horFlip,                   "hor_flip" );
    WRITE_FLAG( sei.verFlip,                   "ver_flip" );
    WRITE_CODE( sei.anticlockwiseRotation, 16, "anticlockwise_rotation" );
    WRITE_FLAG( sei.persistenceFlag,          "display_orientation_persistence_flag" );
  }
  xWriteByteAlign();
}

Void SEIWriter::xWriteSEITemporalLevel0Index(const SEITemporalLevel0Index &sei)
{
  WRITE_CODE( sei.tl0Idx, 8 , "tl0_idx" );
  WRITE_CODE( sei.rapIdx, 8 , "rap_idx" );
  xWriteByteAlign();
}

Void SEIWriter::xWriteSEIGradualDecodingRefreshInfo(const SEIGradualDecodingRefreshInfo &sei)
{
  WRITE_FLAG( sei.m_gdrForegroundFlag, "gdr_foreground_flag");
  xWriteByteAlign();
}

Void SEIWriter::xWriteSEISOPDescription(const SEISOPDescription& sei)
{
  WRITE_UVLC( sei.m_sopSeqParameterSetId,           "sop_seq_parameter_set_id"               );
  WRITE_UVLC( sei.m_numPicsInSopMinus1,             "num_pics_in_sop_minus1"               );
  for (UInt i = 0; i <= sei.m_numPicsInSopMinus1; i++)
  {
    WRITE_CODE( sei.m_sopDescVclNaluType[i], 6, "sop_desc_vcl_nalu_type" );
    WRITE_CODE( sei.m_sopDescTemporalId[i],  3, "sop_desc_temporal_id" );
    if (sei.m_sopDescVclNaluType[i] != NAL_UNIT_CODED_SLICE_IDR_W_RADL && sei.m_sopDescVclNaluType[i] != NAL_UNIT_CODED_SLICE_IDR_N_LP)
    {
      WRITE_UVLC( sei.m_sopDescStRpsIdx[i],           "sop_desc_st_rps_idx"               );
    }
    if (i > 0)
    {
      WRITE_SVLC( sei.m_sopDescPocDelta[i],           "sop_desc_poc_delta"               );
    }
  }

  xWriteByteAlign();
}

#if O0164_MULTI_LAYER_HRD
Void SEIWriter::xWriteSEIScalableNesting(TComBitIf& bs, const SEIScalableNesting& sei, TComVPS *vps, TComSPS *sps)
#else
Void SEIWriter::xWriteSEIScalableNesting(TComBitIf& bs, const SEIScalableNesting& sei, TComSPS *sps)
#endif
{
  WRITE_FLAG( sei.m_bitStreamSubsetFlag,             "bitstream_subset_flag"         );
  WRITE_FLAG( sei.m_nestingOpFlag,                   "nesting_op_flag      "         );
  if (sei.m_nestingOpFlag)
  {
    WRITE_FLAG( sei.m_defaultOpFlag,                 "default_op_flag"               );
    WRITE_UVLC( sei.m_nestingNumOpsMinus1,           "nesting_num_ops_minus1"        );
    for (UInt i = (sei.m_defaultOpFlag ? 1 : 0); i <= sei.m_nestingNumOpsMinus1; i++)
    {
      WRITE_CODE( sei.m_nestingMaxTemporalIdPlus1[i], 3,  "nesting_max_temporal_id_plus1" );
      WRITE_UVLC( sei.m_nestingOpIdx[i],                  "nesting_op_idx"                );
    }
  }
  else
  {
    WRITE_FLAG( sei.m_allLayersFlag,                      "all_layers_flag"               );
    if (!sei.m_allLayersFlag)
    {
      WRITE_CODE( sei.m_nestingNoOpMaxTemporalIdPlus1, 3, "nesting_no_op_max_temporal_id_plus1" );
      WRITE_UVLC( sei.m_nestingNumLayersMinus1,           "nesting_num_layers"                  );
      for (UInt i = 0; i <= sei.m_nestingNumLayersMinus1; i++)
      {
        WRITE_CODE( sei.m_nestingLayerId[i], 6,           "nesting_layer_id"              );
      }
    }
  }
 
  // byte alignment
  while ( m_pcBitIf->getNumberOfWrittenBits() % 8 != 0 )
  {
    WRITE_FLAG( 0, "nesting_zero_bit" );
  }

  // write nested SEI messages
  for (SEIMessages::const_iterator it = sei.m_nestedSEIs.begin(); it != sei.m_nestedSEIs.end(); it++)
  {
#if O0164_MULTI_LAYER_HRD
    writeSEImessage(bs, *(*it), vps, sps, &sei);
#else
    writeSEImessage(bs, *(*it), sps);
#endif
  }
}

Void SEIWriter::xWriteByteAlign()
{
  if( m_pcBitIf->getNumberOfWrittenBits() % 8 != 0)
  {
    WRITE_FLAG( 1, "bit_equal_to_one" );
    while( m_pcBitIf->getNumberOfWrittenBits() % 8 != 0 )
    {
      WRITE_FLAG( 0, "bit_equal_to_zero" );
    }
  }
};

#if SVC_EXTENSION
#if LAYERS_NOT_PRESENT_SEI
Void SEIWriter::xWriteSEILayersNotPresent(const SEILayersNotPresent& sei)
{
  WRITE_UVLC( sei.m_activeVpsId,           "lp_sei_active_vps_id" );
  for (UInt i = 0; i < sei.m_vpsMaxLayers; i++)
  {
    WRITE_FLAG( sei.m_layerNotPresentFlag[i], "layer_not_present_flag"   );
  }
  xWriteByteAlign();
}
#endif

#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
Void SEIWriter::xWriteSEIInterLayerConstrainedTileSets(const SEIInterLayerConstrainedTileSets& sei)
{
  WRITE_FLAG( sei.m_ilAllTilesExactSampleValueMatchFlag,  "il_all_tiles_exact_sample_value_match_flag"   );
  WRITE_FLAG( sei.m_ilOneTilePerTileSetFlag,              "il_one_tile_per_tile_set_flag"                );
  if( !sei.m_ilOneTilePerTileSetFlag )
  {
    WRITE_UVLC( sei.m_ilNumSetsInMessageMinus1,             "il_num_sets_in_message_minus1"                );
    if( sei.m_ilNumSetsInMessageMinus1 )
    {
      WRITE_FLAG( sei.m_skippedTileSetPresentFlag,            "skipped_tile_set_present_flag"                );
    }
    UInt numSignificantSets = sei.m_ilNumSetsInMessageMinus1 - (sei.m_skippedTileSetPresentFlag ? 1 : 0) + 1;
    for( UInt i = 0; i < numSignificantSets; i++ )
    {
      WRITE_UVLC( sei.m_ilctsId[i],                           "ilcts_id"                                     );
      WRITE_UVLC( sei.m_ilNumTileRectsInSetMinus1[i],         "il_num_tile_rects_in_set_minus1"              );
      for( UInt j = 0; j <= sei.m_ilNumTileRectsInSetMinus1[i]; j++ )
      {
        WRITE_UVLC( sei.m_ilTopLeftTileIndex[i][j],             "il_top_left_tile_index"                       );
        WRITE_UVLC( sei.m_ilBottomRightTileIndex[i][j],         "il_bottom_right_tile_index"                   );
      }
      WRITE_CODE( sei.m_ilcIdc[i], 2,                         "ilc_idc"                                      );
      if( sei.m_ilAllTilesExactSampleValueMatchFlag )
      {
        WRITE_FLAG( sei.m_ilExactSampleValueMatchFlag[i],        "il_exact_sample_value_match_flag"            );
      }
    }
  }
  else
  {
    WRITE_CODE( sei.m_allTilesIlcIdc, 2,                    "all_tiles_ilc_idc"                          );
  }

  xWriteByteAlign();
}
#endif
#if SUB_BITSTREAM_PROPERTY_SEI
Void SEIWriter::xWriteSEISubBitstreamProperty(const SEISubBitstreamProperty &sei)
{
  WRITE_CODE( sei.m_activeVpsId, 4, "active_vps_id" );
  assert( sei.m_numAdditionalSubStreams >= 1 );
  WRITE_UVLC( sei.m_numAdditionalSubStreams - 1, "num_additional_sub_streams_minus1" );

  for( Int i = 0; i < sei.m_numAdditionalSubStreams; i++ )
  {
    WRITE_CODE( sei.m_subBitstreamMode[i],       2, "sub_bitstream_mode[i]"           );
    WRITE_UVLC( sei.m_outputLayerSetIdxToVps[i],    "output_layer_set_idx_to_vps[i]"  );
    WRITE_CODE( sei.m_highestSublayerId[i],      3, "highest_sub_layer_id[i]"         );
    WRITE_CODE( sei.m_avgBitRate[i],            16, "avg_bit_rate[i]"                 );
    WRITE_CODE( sei.m_maxBitRate[i],            16, "max_bit_rate[i]"                 );
  }
  xWriteByteAlign();
}
#endif

#if Q0189_TMVP_CONSTRAINTS 
Void SEIWriter::xWriteSEITMVPConstraints (const SEITMVPConstrains &sei)
{
  WRITE_UVLC( sei.prev_pics_not_used_flag ,    "prev_pics_not_used_flag"  );
  WRITE_UVLC( sei.no_intra_layer_col_pic_flag ,    "no_intra_layer_col_pic_flag"  ); 
  xWriteByteAlign();
}
#endif

#if Q0247_FRAME_FIELD_INFO
Void SEIWriter::xWriteSEIFrameFieldInfo  (const SEIFrameFieldInfo &sei)
{
  WRITE_CODE( sei.m_ffinfo_picStruct , 4,             "ffinfo_pic_struct" );
  WRITE_CODE( sei.m_ffinfo_sourceScanType, 2,         "ffinfo_source_scan_type" );
  WRITE_FLAG( sei.m_ffinfo_duplicateFlag ? 1 : 0,     "ffinfo_duplicate_flag" );
  xWriteByteAlign();
}
#endif

#if O0164_MULTI_LAYER_HRD
Void SEIWriter::xWriteSEIBspNesting(TComBitIf& bs, const SEIBspNesting &sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting &nestingSei)
{
  WRITE_UVLC( sei.m_bspIdx, "bsp_idx" );

  while ( m_pcBitIf->getNumberOfWrittenBits() % 8 != 0 )
  {
    WRITE_FLAG( 0, "bsp_nesting_zero_bit" );
  }
#if NESTING_SEI_EXTENSIBILITY
  assert( sei.m_nestedSEIs.size() <= MAX_SEIS_IN_BSP_NESTING );
  WRITE_UVLC( sei.m_nestedSEIs.size(), "num_seis_in_bsp_minus1" );
#endif
  // write nested SEI messages
  for (SEIMessages::const_iterator it = sei.m_nestedSEIs.begin(); it != sei.m_nestedSEIs.end(); it++)
  {
    writeSEImessage(bs, *(*it), vps, sps, &nestingSei, &sei);
  }
}

Void SEIWriter::xWriteSEIBspInitialArrivalTime(const SEIBspInitialArrivalTime &sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting &nestingSei, const SEIBspNesting &bspNestingSei)
{
  assert(vps->getVpsVuiPresentFlag());

#if VPS_VUI_BSP_HRD_PARAMS
  Int psIdx = bspNestingSei.m_seiPartitioningSchemeIdx;
  Int seiOlsIdx = bspNestingSei.m_seiOlsIdx;
  Int maxTemporalId = nestingSei.m_nestingMaxTemporalIdPlus1[0] - 1;
  Int maxValues = vps->getNumBspSchedulesMinus1(seiOlsIdx, psIdx, maxTemporalId) + 1;
  std::vector<Int> hrdIdx(maxValues, 0);
  std::vector<TComHRD *> hrdVec;
  std::vector<Int> syntaxElemLen(maxValues, 0);
  for(Int i = 0; i < maxValues; i++)
  {
    hrdIdx[i] = vps->getBspHrdIdx( seiOlsIdx, psIdx, maxTemporalId, i, bspNestingSei.m_bspIdx);
    hrdVec.push_back(vps->getBspHrd(hrdIdx[i]));
    
    syntaxElemLen[i] = hrdVec[i]->getInitialCpbRemovalDelayLengthMinus1() + 1;
    if ( !(hrdVec[i]->getNalHrdParametersPresentFlag() || hrdVec[i]->getVclHrdParametersPresentFlag()) )
    {
      assert( syntaxElemLen[i] == 24 ); // Default of value init_cpb_removal_delay_length_minus1 is 23
    }
    if( i > 0 )
    {
      assert( hrdVec[i]->getNalHrdParametersPresentFlag() == hrdVec[i-1]->getNalHrdParametersPresentFlag() );
      assert( hrdVec[i]->getVclHrdParametersPresentFlag() == hrdVec[i-1]->getVclHrdParametersPresentFlag() );
    }
  }
  if (hrdVec[0]->getNalHrdParametersPresentFlag())
  {
    for(UInt i = 0; i < maxValues; i++)
    {
      WRITE_CODE( sei.m_nalInitialArrivalDelay[i], syntaxElemLen[i], "nal_initial_arrival_delay[i]" );
    }
  }
  if( hrdVec[0]->getVclHrdParametersPresentFlag() )
  {
    for(UInt i = 0; i < maxValues; i++)
    {
      WRITE_CODE( sei.m_vclInitialArrivalDelay[i], syntaxElemLen[i], "vcl_initial_arrival_delay[i]" );
    }
  }
#else
  UInt schedCombCnt = vps->getNumBspSchedCombinations(nestingSei.m_nestingOpIdx[0]);
  UInt len;
  UInt hrdIdx;

  if (schedCombCnt > 0)
  {
    hrdIdx = vps->getBspCombHrdIdx(nestingSei.m_nestingOpIdx[0], 0, bspNestingSei.m_bspIdx);
  }
  else
  {
    hrdIdx = 0;
  }

  TComHRD *hrd = vps->getBspHrd(hrdIdx);

  if (hrd->getNalHrdParametersPresentFlag() || hrd->getVclHrdParametersPresentFlag())
  {
    len = hrd->getInitialCpbRemovalDelayLengthMinus1() + 1;
  }
  else
  {
    len = 23 + 1;
  }

  if (hrd->getNalHrdParametersPresentFlag())
  {
    for(UInt i = 0; i < schedCombCnt; i++)
    {
      WRITE_CODE( sei.m_nalInitialArrivalDelay[i], len, "nal_initial_arrival_delay" );
    }
  }
#if BSP_INIT_ARRIVAL_SEI
  if( hrd->getVclHrdParametersPresentFlag() )
#else
  else
#endif
  {
    for(UInt i = 0; i < schedCombCnt; i++)
    {
      WRITE_CODE( sei.m_vclInitialArrivalDelay[i], len, "vcl_initial_arrival_delay" );
    }
  }
#endif
}

#if !REMOVE_BSP_HRD_SEI
Void SEIWriter::xWriteSEIBspHrd(const SEIBspHrd &sei, TComSPS *sps, const SEIScalableNesting &nestingSei)
{
  WRITE_UVLC( sei.m_seiNumBspHrdParametersMinus1, "sei_num_bsp_hrd_parameters_minus1" );
  for (UInt i = 0; i <= sei.m_seiNumBspHrdParametersMinus1; i++)
  {
    if (i > 0)
    {
      WRITE_FLAG( sei.m_seiBspCprmsPresentFlag[i], "sei_bsp_cprms_present_flag" );
    }
    xCodeHrdParameters(sei.hrd, i==0 ? 1 : sei.m_seiBspCprmsPresentFlag[i], nestingSei.m_nestingMaxTemporalIdPlus1[0]-1);
  }
  for (UInt h = 0; h <= nestingSei.m_nestingNumOpsMinus1; h++)
  {
    UInt lsIdx = nestingSei.m_nestingOpIdx[h];
    WRITE_UVLC( sei.m_seiNumBitstreamPartitionsMinus1[lsIdx], "num_sei_bitstream_partitions_minus1[i]");
    for (UInt i = 0; i <= sei.m_seiNumBitstreamPartitionsMinus1[lsIdx]; i++)
    {
#if HRD_BPB
      UInt nl=0;
      for (UInt j = 0; j < sei.m_vpsMaxLayers; j++)
      {
        if (sei.m_layerIdIncludedFlag[lsIdx][j])
        {
          nl++;
        }
      }
      for (UInt j = 0; j < nl; j++)
      {
#else
      for (UInt j = 0; j < sei.m_vpsMaxLayers; j++)
      {
        if (sei.m_layerIdIncludedFlag[lsIdx][j])
        {
#endif
          WRITE_FLAG( sei.m_seiLayerInBspFlag[lsIdx][i][j], "sei_layer_in_bsp_flag[lsIdx][i][j]" );
        }
#if !HRD_BPB
      }
#endif
    }
    WRITE_UVLC( sei.m_seiNumBspSchedCombinationsMinus1[lsIdx], "sei_num_bsp_sched_combinations_minus1[i]");
    for (UInt i = 0; i <= sei.m_seiNumBspSchedCombinationsMinus1[lsIdx]; i++)
    {
      for (UInt j = 0; j <= sei.m_seiNumBitstreamPartitionsMinus1[lsIdx]; j++)
      {
        WRITE_UVLC( sei.m_seiBspCombHrdIdx[lsIdx][i][j], "sei_bsp_comb_hrd_idx[lsIdx][i][j]");
        WRITE_UVLC( sei.m_seiBspCombScheddx[lsIdx][i][j], "sei_bsp_comb_sched_idx[lsIdx][i][j]");
      }
    }
  }
}
#endif

Void SEIWriter::xCodeHrdParameters( TComHRD *hrd, Bool commonInfPresentFlag, UInt maxNumSubLayersMinus1 )
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

#endif

#if Q0078_ADD_LAYER_SETS

Void SEIWriter::xWriteSEIOutputLayerSetNesting(TComBitIf& bs, const SEIOutputLayerSetNesting &sei, TComVPS *vps, TComSPS *sps)
{
  WRITE_FLAG(sei.m_olsFlag, "ols_flag");
  WRITE_UVLC(sei.m_numOlsIndicesMinus1, "num_ols_indices_minus1");

  for (Int i = 0; i <= sei.m_numOlsIndicesMinus1; i++)
  {
    WRITE_UVLC(sei.m_olsIdx[i], "ols_idx[i]");
  }

  while (m_pcBitIf->getNumberOfWrittenBits() % 8 != 0)
  {
    WRITE_FLAG(0, "ols_nesting_zero_bit");
  }

  // write nested SEI messages
  for (SEIMessages::const_iterator it = sei.m_nestedSEIs.begin(); it != sei.m_nestedSEIs.end(); it++)
  {
    writeSEImessage(bs, *(*it), vps, sps);
  }
}

Void SEIWriter::xWriteSEIVPSRewriting(const SEIVPSRewriting &sei)
{
  //sei.nalu->
}

#endif

#endif //SVC_EXTENSION

//! \}
