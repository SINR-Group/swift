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

/** 
 \file     SEIread.cpp
 \brief    reading functionality for SEI messages
 */

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/SEI.h"
#include "TLibCommon/TComSlice.h"
#include "SyntaxElementParser.h"
#include "SEIread.h"

//! \ingroup TLibDecoder
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
    fprintf( g_hTrace, "===========Tone Mapping Info SEI message ===========\n");
    break;
#if P0050_KNEE_FUNCTION_SEI
  case SEI::KNEE_FUNCTION_INFO:
    fprintf( g_hTrace, "=========== Knee Function Information SEI message ===========\n");
    break;
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  case SEI::COLOUR_REMAPPING_INFO:
    fprintf( g_hTrace, "===========Colour Remapping Information SEI message ===========\n");
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
    fprintf( g_hTrace, "=========== Bitstream parition nesting SEI message ===========\n");
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

/**
 * unmarshal a single SEI message from bitstream bs
 */
#if LAYERS_NOT_PRESENT_SEI
void SEIReader::parseSEImessage(TComInputBitstream* bs, SEIMessages& seis, const NalUnitType nalUnitType, TComVPS *vps, TComSPS *sps)
#else
void SEIReader::parseSEImessage(TComInputBitstream* bs, SEIMessages& seis, const NalUnitType nalUnitType, TComSPS *sps)
#endif
{
  setBitstream(bs);

  assert(!m_pcBitstream->getNumBitsUntilByteAligned());
  do
  {
#if LAYERS_NOT_PRESENT_SEI
    xReadSEImessage(seis, nalUnitType, vps, sps);
#else
    xReadSEImessage(seis, nalUnitType, sps);
#endif
    /* SEI messages are an integer number of bytes, something has failed
    * in the parsing if bitstream not byte-aligned */
    assert(!m_pcBitstream->getNumBitsUntilByteAligned());
  } while (m_pcBitstream->getNumBitsLeft() > 8);

  UInt rbspTrailingBits;
  READ_CODE(8, rbspTrailingBits, "rbsp_trailing_bits");
  assert(rbspTrailingBits == 0x80);
}

#if O0164_MULTI_LAYER_HRD
#if LAYERS_NOT_PRESENT_SEI
Void SEIReader::xReadSEImessage(SEIMessages& seis, const NalUnitType nalUnitType, TComVPS *vps, TComSPS *sps, const SEIScalableNesting *nestingSei, const SEIBspNesting *bspNestingSei)
#else
Void SEIReader::xReadSEImessage(SEIMessages& seis, const NalUnitType nalUnitType, TComSPS *sps, const SEIScalableNesting *nestingSei)
#endif
#else
#if LAYERS_NOT_PRESENT_SEI
Void SEIReader::xReadSEImessage(SEIMessages& seis, const NalUnitType nalUnitType, TComVPS *vps, TComSPS *sps)
#else
Void SEIReader::xReadSEImessage(SEIMessages& seis, const NalUnitType nalUnitType, TComSPS *sps)
#endif
#endif
{
#if ENC_DEC_TRACE
  xTraceSEIHeader();
#endif
  Int payloadType = 0;
  UInt val = 0;

  do
  {
    READ_CODE (8, val, "payload_type");
    payloadType += val;
  } while (val==0xFF);

  UInt payloadSize = 0;
  do
  {
    READ_CODE (8, val, "payload_size");
    payloadSize += val;
  } while (val==0xFF);

#if ENC_DEC_TRACE
  xTraceSEIMessageType((SEI::PayloadType)payloadType);
#endif

  /* extract the payload for this single SEI message.
   * This allows greater safety in erroneous parsing of an SEI message
   * from affecting subsequent messages.
   * After parsing the payload, bs needs to be restored as the primary
   * bitstream.
   */
  TComInputBitstream *bs = getBitstream();
  setBitstream(bs->extractSubstream(payloadSize * 8));

  SEI *sei = NULL;

  if(nalUnitType == NAL_UNIT_PREFIX_SEI)
  {
    switch (payloadType)
    {
    case SEI::USER_DATA_UNREGISTERED:
      sei = new SEIuserDataUnregistered;
      xParseSEIuserDataUnregistered((SEIuserDataUnregistered&) *sei, payloadSize);
      break;
    case SEI::ACTIVE_PARAMETER_SETS:
      sei = new SEIActiveParameterSets; 
      xParseSEIActiveParameterSets((SEIActiveParameterSets&) *sei, payloadSize); 
      break; 
    case SEI::DECODING_UNIT_INFO:
      if (!sps)
      {
        printf ("Warning: Found Decoding unit SEI message, but no active SPS is available. Ignoring.");
      }
      else
      {
        sei = new SEIDecodingUnitInfo; 
#if VPS_VUI_BSP_HRD_PARAMS
        xParseSEIDecodingUnitInfo((SEIDecodingUnitInfo&) *sei, payloadSize, sps, nestingSei, bspNestingSei, vps);
#else
        xParseSEIDecodingUnitInfo((SEIDecodingUnitInfo&) *sei, payloadSize, sps);
#endif
      }
      break; 
    case SEI::BUFFERING_PERIOD:
      if (!sps)
      {
        printf ("Warning: Found Buffering period SEI message, but no active SPS is available. Ignoring.");
      }
      else
      {
        sei = new SEIBufferingPeriod;
#if VPS_VUI_BSP_HRD_PARAMS
        xParseSEIBufferingPeriod((SEIBufferingPeriod&) *sei, payloadSize, sps, nestingSei, bspNestingSei, vps);
#else
        xParseSEIBufferingPeriod((SEIBufferingPeriod&) *sei, payloadSize, sps);
#endif
      }
      break;
    case SEI::PICTURE_TIMING:
      if (!sps)
      {
        printf ("Warning: Found Picture timing SEI message, but no active SPS is available. Ignoring.");
      }
      else
      {
        sei = new SEIPictureTiming;
#if VPS_VUI_BSP_HRD_PARAMS
        xParseSEIPictureTiming((SEIPictureTiming&)*sei, payloadSize, sps, nestingSei, bspNestingSei, vps);
#else
        xParseSEIPictureTiming((SEIPictureTiming&)*sei, payloadSize, sps);
#endif
      }
      break;
    case SEI::RECOVERY_POINT:
      sei = new SEIRecoveryPoint;
      xParseSEIRecoveryPoint((SEIRecoveryPoint&) *sei, payloadSize);
      break;
    case SEI::FRAME_PACKING:
      sei = new SEIFramePacking;
      xParseSEIFramePacking((SEIFramePacking&) *sei, payloadSize);
      break;
    case SEI::DISPLAY_ORIENTATION:
      sei = new SEIDisplayOrientation;
      xParseSEIDisplayOrientation((SEIDisplayOrientation&) *sei, payloadSize);
      break;
    case SEI::TEMPORAL_LEVEL0_INDEX:
      sei = new SEITemporalLevel0Index;
      xParseSEITemporalLevel0Index((SEITemporalLevel0Index&) *sei, payloadSize);
      break;
    case SEI::REGION_REFRESH_INFO:
      sei = new SEIGradualDecodingRefreshInfo;
      xParseSEIGradualDecodingRefreshInfo((SEIGradualDecodingRefreshInfo&) *sei, payloadSize);
      break;
    case SEI::TONE_MAPPING_INFO:
      sei = new SEIToneMappingInfo;
      xParseSEIToneMappingInfo((SEIToneMappingInfo&) *sei, payloadSize);
      break;
#if P0050_KNEE_FUNCTION_SEI
    case SEI::KNEE_FUNCTION_INFO:
      sei = new SEIKneeFunctionInfo;
      xParseSEIKneeFunctionInfo((SEIKneeFunctionInfo&) *sei, payloadSize);
      break;
#endif
#if Q0074_COLOUR_REMAPPING_SEI
    case SEI::COLOUR_REMAPPING_INFO:
      sei = new SEIColourRemappingInfo;
      xParseSEIColourRemappingInfo((SEIColourRemappingInfo&) *sei, payloadSize);
      break;
#endif
    case SEI::SOP_DESCRIPTION:
      sei = new SEISOPDescription;
      xParseSEISOPDescription((SEISOPDescription&) *sei, payloadSize);
      break;
    case SEI::SCALABLE_NESTING:
      sei = new SEIScalableNesting;
#if LAYERS_NOT_PRESENT_SEI
      xParseSEIScalableNesting((SEIScalableNesting&) *sei, nalUnitType, payloadSize, vps, sps);
#else
      xParseSEIScalableNesting((SEIScalableNesting&) *sei, nalUnitType, payloadSize, sps);
#endif
      break;
#if SVC_EXTENSION
#if LAYERS_NOT_PRESENT_SEI
    case SEI::LAYERS_NOT_PRESENT:
      if (!vps)
      {
        printf ("Warning: Found Layers not present SEI message, but no active VPS is available. Ignoring.");
      }
      else
      {
        sei = new SEILayersNotPresent;
        xParseSEILayersNotPresent((SEILayersNotPresent&) *sei, payloadSize, vps);
      }
      break;
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
    case SEI::INTER_LAYER_CONSTRAINED_TILE_SETS:
      sei = new SEIInterLayerConstrainedTileSets;
      xParseSEIInterLayerConstrainedTileSets((SEIInterLayerConstrainedTileSets&) *sei, payloadSize);
      break;
#endif
#if SUB_BITSTREAM_PROPERTY_SEI
   case SEI::SUB_BITSTREAM_PROPERTY:
     sei = new SEISubBitstreamProperty;
#if OLS_IDX_CHK
     xParseSEISubBitstreamProperty((SEISubBitstreamProperty&) *sei, vps);
#else
     xParseSEISubBitstreamProperty((SEISubBitstreamProperty&) *sei);
#endif
     break;
#endif
#if O0164_MULTI_LAYER_HRD
   case SEI::BSP_NESTING:
     sei = new SEIBspNesting;
#if LAYERS_NOT_PRESENT_SEI
     xParseSEIBspNesting((SEIBspNesting&) *sei, nalUnitType, vps, sps, *nestingSei);
#else
     xParseSEIBspNesting((SEIBspNesting&) *sei, nalUnitType, sps, *nestingSei);
#endif
     break;
   case SEI::BSP_INITIAL_ARRIVAL_TIME:
     sei = new SEIBspInitialArrivalTime;
     xParseSEIBspInitialArrivalTime((SEIBspInitialArrivalTime&) *sei, vps, sps, *nestingSei, *bspNestingSei);
     break;
#if !REMOVE_BSP_HRD_SEI
   case SEI::BSP_HRD:
     sei = new SEIBspHrd;
     xParseSEIBspHrd((SEIBspHrd&) *sei, sps, *nestingSei);
     break;
#endif
#endif
#if Q0078_ADD_LAYER_SETS
   case SEI::OUTPUT_LAYER_SET_NESTING:
     sei = new SEIOutputLayerSetNesting;
#if LAYERS_NOT_PRESENT_SEI
     xParseSEIOutputLayerSetNesting((SEIOutputLayerSetNesting&)*sei, nalUnitType, vps, sps);
#else
     xParseSEIOutputLayerSetNesting((SEIOutputLayerSetNesting&)*sei, nalUnitType, sps);
#endif
     break;
   case SEI::VPS_REWRITING:
     sei = new SEIVPSRewriting;
     xParseSEIVPSRewriting((SEIVPSRewriting&)*sei);
     break;
#endif
#if Q0189_TMVP_CONSTRAINTS
   case SEI::TMVP_CONSTRAINTS:
     sei =  new SEITMVPConstrains;
     xParseSEITMVPConstraints((SEITMVPConstrains&) *sei, payloadSize);
     break;
#endif
#if Q0247_FRAME_FIELD_INFO
   case SEI::FRAME_FIELD_INFO:
     sei =  new SEIFrameFieldInfo;
     xParseSEIFrameFieldInfo    ((SEIFrameFieldInfo&) *sei, payloadSize); 
     break;
#endif
#endif //SVC_EXTENSION
      break;
    default:
      for (UInt i = 0; i < payloadSize; i++)
      {
        UInt seiByte;
        READ_CODE (8, seiByte, "unknown prefix SEI payload byte");
      }
      printf ("Unknown prefix SEI message (payloadType = %d) was found!\n", payloadType);
    }
  }
  else
  {
    switch (payloadType)
    {
      case SEI::USER_DATA_UNREGISTERED:
        sei = new SEIuserDataUnregistered;
        xParseSEIuserDataUnregistered((SEIuserDataUnregistered&) *sei, payloadSize);
        break;
      case SEI::DECODED_PICTURE_HASH:
        sei = new SEIDecodedPictureHash;
        xParseSEIDecodedPictureHash((SEIDecodedPictureHash&) *sei, payloadSize);
        break;
      default:
        for (UInt i = 0; i < payloadSize; i++)
        {
          UInt seiByte;
          READ_CODE (8, seiByte, "unknown suffix SEI payload byte");
        }
        printf ("Unknown suffix SEI message (payloadType = %d) was found!\n", payloadType);
    }
  }
  if (sei != NULL)
  {
    seis.push_back(sei);
  }

  /* By definition the underlying bitstream terminates in a byte-aligned manner.
   * 1. Extract all bar the last MIN(bitsremaining,nine) bits as reserved_payload_extension_data
   * 2. Examine the final 8 bits to determine the payload_bit_equal_to_one marker
   * 3. Extract the remainingreserved_payload_extension_data bits.
   *
   * If there are fewer than 9 bits available, extract them.
   */
  Int payloadBitsRemaining = getBitstream()->getNumBitsLeft();
  if (payloadBitsRemaining) /* more_data_in_payload() */
  {
    for (; payloadBitsRemaining > 9; payloadBitsRemaining--)
    {
      UInt reservedPayloadExtensionData;
      READ_CODE (1, reservedPayloadExtensionData, "reserved_payload_extension_data");
    }

    /* 2 */
    Int finalBits = getBitstream()->peekBits(payloadBitsRemaining);
    Int finalPayloadBits = 0;
    for (Int mask = 0xff; finalBits & (mask >> finalPayloadBits); finalPayloadBits++)
    {
      continue;
    }

    /* 3 */
    for (; payloadBitsRemaining > 9 - finalPayloadBits; payloadBitsRemaining--)
    {
      UInt reservedPayloadExtensionData;
      READ_FLAG (reservedPayloadExtensionData, "reserved_payload_extension_data");
    }

    UInt dummy;
    READ_FLAG (dummy, "payload_bit_equal_to_one"); payloadBitsRemaining--;
    while (payloadBitsRemaining)
    {
      READ_FLAG (dummy, "payload_bit_equal_to_zero"); payloadBitsRemaining--;
    }
  }

  /* restore primary bitstream for sei_message */
  getBitstream()->deleteFifo();
  delete getBitstream();
  setBitstream(bs);
}

#if P0138_USE_ALT_CPB_PARAMS_FLAG
/**
 * Check if SEI message contains payload extension
 */
Bool SEIReader::xPayloadExtensionPresent()
{
  Int payloadBitsRemaining = getBitstream()->getNumBitsLeft();
  Bool payloadExtensionPresent = false;

  if (payloadBitsRemaining > 8)
  {
    payloadExtensionPresent = true;
  }
  else
  {
    Int finalBits = getBitstream()->peekBits(payloadBitsRemaining);
    while (payloadBitsRemaining && (finalBits & 1) == 0)
    {
      payloadBitsRemaining--;
      finalBits >>= 1;
    }
    payloadBitsRemaining--;
    if (payloadBitsRemaining > 0)
    {
      payloadExtensionPresent = true;
    }
  }

  return payloadExtensionPresent;
}
#endif

/**
 * parse bitstream bs and unpack a user_data_unregistered SEI message
 * of payloasSize bytes into sei.
 */
Void SEIReader::xParseSEIuserDataUnregistered(SEIuserDataUnregistered &sei, UInt payloadSize)
{
  assert(payloadSize >= 16);
  UInt val;

  for (UInt i = 0; i < 16; i++)
  {
    READ_CODE (8, val, "uuid_iso_iec_11578");
    sei.uuid_iso_iec_11578[i] = val;
  }

  sei.userDataLength = payloadSize - 16;
  if (!sei.userDataLength)
  {
    sei.userData = 0;
    return;
  }

  sei.userData = new UChar[sei.userDataLength];
  for (UInt i = 0; i < sei.userDataLength; i++)
  {
    READ_CODE (8, val, "user_data" );
    sei.userData[i] = val;
  }
}

/**
 * parse bitstream bs and unpack a decoded picture hash SEI message
 * of payloadSize bytes into sei.
 */
Void SEIReader::xParseSEIDecodedPictureHash(SEIDecodedPictureHash& sei, UInt /*payloadSize*/)
{
  UInt val;
  READ_CODE (8, val, "hash_type");
  sei.method = static_cast<SEIDecodedPictureHash::Method>(val);
  for(Int yuvIdx = 0; yuvIdx < 3; yuvIdx++)
  {
    if(SEIDecodedPictureHash::MD5 == sei.method)
    {
      for (UInt i = 0; i < 16; i++)
      {
        READ_CODE(8, val, "picture_md5");
        sei.digest[yuvIdx][i] = val;
      }
    }
    else if(SEIDecodedPictureHash::CRC == sei.method)
    {
      READ_CODE(16, val, "picture_crc");
      sei.digest[yuvIdx][0] = val >> 8 & 0xFF;
      sei.digest[yuvIdx][1] = val & 0xFF;
    }
    else if(SEIDecodedPictureHash::CHECKSUM == sei.method)
    {
      READ_CODE(32, val, "picture_checksum");
      sei.digest[yuvIdx][0] = (val>>24) & 0xff;
      sei.digest[yuvIdx][1] = (val>>16) & 0xff;
      sei.digest[yuvIdx][2] = (val>>8)  & 0xff;
      sei.digest[yuvIdx][3] =  val      & 0xff;
    }
  }
}
Void SEIReader::xParseSEIActiveParameterSets(SEIActiveParameterSets& sei, UInt /*payloadSize*/)
{
  UInt val; 
  READ_CODE(4, val, "active_video_parameter_set_id");   sei.activeVPSId = val; 
  READ_FLAG(   val, "self_contained_cvs_flag");         sei.m_selfContainedCvsFlag = val ? true : false;
  READ_FLAG(   val, "no_parameter_set_update_flag");    sei.m_noParameterSetUpdateFlag = val ? true : false;
  READ_UVLC(   val, "num_sps_ids_minus1"); sei.numSpsIdsMinus1 = val;

  sei.activeSeqParameterSetId.resize(sei.numSpsIdsMinus1 + 1);
#if R0247_SEI_ACTIVE
  sei.layerSpsIdx.resize(sei.numSpsIdsMinus1 + 1);
#endif
  for (Int i=0; i < (sei.numSpsIdsMinus1 + 1); i++)
  {
    READ_UVLC(val, "active_seq_parameter_set_id");      sei.activeSeqParameterSetId[i] = val; 
  }
#if R0247_SEI_ACTIVE
  for (Int i=1; i < (sei.numSpsIdsMinus1 + 1); i++)
  {
    READ_UVLC(val, "layer_sps_idx"); sei.layerSpsIdx[i] = val; 
  }
#endif
  xParseByteAlign();
}

#if VPS_VUI_BSP_HRD_PARAMS
Void SEIReader::xParseSEIDecodingUnitInfo(SEIDecodingUnitInfo& sei, UInt /*payloadSize*/, TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei, TComVPS *vps)
#else
Void SEIReader::xParseSEIDecodingUnitInfo(SEIDecodingUnitInfo& sei, UInt /*payloadSize*/, TComSPS *sps)
#endif
{
  UInt val;
  READ_UVLC(val, "decoding_unit_idx");
  sei.m_decodingUnitIdx = val;

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
  if(hrd->getSubPicCpbParamsInPicTimingSEIFlag())
  {
    READ_CODE( ( hrd->getDuCpbRemovalDelayLengthMinus1() + 1 ), val, "du_spt_cpb_removal_delay");
    sei.m_duSptCpbRemovalDelay = val;
  }
  else
  {
    sei.m_duSptCpbRemovalDelay = 0;
  }
  READ_FLAG( val, "dpb_output_du_delay_present_flag"); sei.m_dpbOutputDuDelayPresentFlag = val ? true : false;
  if(sei.m_dpbOutputDuDelayPresentFlag)
  {
    READ_CODE(hrd->getDpbOutputDelayDuLengthMinus1() + 1, val, "pic_spt_dpb_output_du_delay"); 
    sei.m_picSptDpbOutputDuDelay = val;
  }
  xParseByteAlign();
}

#if VPS_VUI_BSP_HRD_PARAMS
Void SEIReader::xParseSEIBufferingPeriod(SEIBufferingPeriod& sei, UInt /*payloadSize*/, TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei, TComVPS *vps)
#else
Void SEIReader::xParseSEIBufferingPeriod(SEIBufferingPeriod& sei, UInt /*payloadSize*/, TComSPS *sps)
#endif
{
  Int i, nalOrVcl;
  UInt code;

#if VPS_VUI_BSP_HRD_PARAMS
  TComHRD *pHRD;
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
    pHRD = hrdVec[i];
  }
  else
  {
    TComVUI *vui = sps->getVuiParameters();
    pHRD = vui->getHrdParameters();
  }
  // To be done: When contained in an BSP HRD SEI message, the hrd structure is to be chosen differently.
#else
  TComVUI *pVUI = sps->getVuiParameters();
  TComHRD *pHRD = pVUI->getHrdParameters();
#endif

  READ_UVLC( code, "bp_seq_parameter_set_id" );                         sei.m_bpSeqParameterSetId     = code;
  if( !pHRD->getSubPicCpbParamsPresentFlag() )
  {
    READ_FLAG( code, "irap_cpb_params_present_flag" );                   sei.m_rapCpbParamsPresentFlag = code;
  }
  if( sei.m_rapCpbParamsPresentFlag )
  {
    READ_CODE( pHRD->getCpbRemovalDelayLengthMinus1() + 1, code, "cpb_delay_offset" );      sei.m_cpbDelayOffset = code;
    READ_CODE( pHRD->getDpbOutputDelayLengthMinus1()  + 1, code, "dpb_delay_offset" );      sei.m_dpbDelayOffset = code;
  }
  //read splicing flag and cpb_removal_delay_delta
  READ_FLAG( code, "concatenation_flag"); 
  sei.m_concatenationFlag = code;
  READ_CODE( ( pHRD->getCpbRemovalDelayLengthMinus1() + 1 ), code, "au_cpb_removal_delay_delta_minus1" );
  sei.m_auCpbRemovalDelayDelta = code + 1;
  for( nalOrVcl = 0; nalOrVcl < 2; nalOrVcl ++ )
  {
    if( ( ( nalOrVcl == 0 ) && ( pHRD->getNalHrdParametersPresentFlag() ) ) ||
        ( ( nalOrVcl == 1 ) && ( pHRD->getVclHrdParametersPresentFlag() ) ) )
    {
      for( i = 0; i < ( pHRD->getCpbCntMinus1( 0 ) + 1 ); i ++ )
      {
        READ_CODE( ( pHRD->getInitialCpbRemovalDelayLengthMinus1() + 1 ) , code, "initial_cpb_removal_delay" );
        sei.m_initialCpbRemovalDelay[i][nalOrVcl] = code;
        READ_CODE( ( pHRD->getInitialCpbRemovalDelayLengthMinus1() + 1 ) , code, "initial_cpb_removal_delay_offset" );
        sei.m_initialCpbRemovalDelayOffset[i][nalOrVcl] = code;
        if( pHRD->getSubPicCpbParamsPresentFlag() || sei.m_rapCpbParamsPresentFlag )
        {
          READ_CODE( ( pHRD->getInitialCpbRemovalDelayLengthMinus1() + 1 ) , code, "initial_alt_cpb_removal_delay" );
          sei.m_initialAltCpbRemovalDelay[i][nalOrVcl] = code;
          READ_CODE( ( pHRD->getInitialCpbRemovalDelayLengthMinus1() + 1 ) , code, "initial_alt_cpb_removal_delay_offset" );
          sei.m_initialAltCpbRemovalDelayOffset[i][nalOrVcl] = code;
        }
      }
    }
  }

#if P0138_USE_ALT_CPB_PARAMS_FLAG
  sei.m_useAltCpbParamsFlag = false;
  sei.m_useAltCpbParamsFlagPresent = false;
  if (xPayloadExtensionPresent())
  {
    READ_FLAG (code, "use_alt_cpb_params_flag");
    sei.m_useAltCpbParamsFlag = code;
    sei.m_useAltCpbParamsFlagPresent = true;
  }
#endif

  xParseByteAlign();
}
#if VPS_VUI_BSP_HRD_PARAMS
Void SEIReader::xParseSEIPictureTiming(SEIPictureTiming& sei, UInt /*payloadSize*/, TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei, TComVPS *vps)
#else
Void SEIReader::xParseSEIPictureTiming(SEIPictureTiming& sei, UInt /*payloadSize*/, TComSPS *sps)
#endif
{
  Int i;
  UInt code;

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

  if( vui->getFrameFieldInfoPresentFlag() )
  {
    READ_CODE( 4, code, "pic_struct" );             sei.m_picStruct            = code;
    READ_CODE( 2, code, "source_scan_type" );       sei.m_sourceScanType = code;
    READ_FLAG(    code, "duplicate_flag" );         sei.m_duplicateFlag        = ( code == 1 ? true : false );
  }

  if( hrd->getCpbDpbDelaysPresentFlag())
  {
    READ_CODE( ( hrd->getCpbRemovalDelayLengthMinus1() + 1 ), code, "au_cpb_removal_delay_minus1" );
    sei.m_auCpbRemovalDelay = code + 1;
    READ_CODE( ( hrd->getDpbOutputDelayLengthMinus1() + 1 ), code, "pic_dpb_output_delay" );
    sei.m_picDpbOutputDelay = code;

    if(hrd->getSubPicCpbParamsPresentFlag())
    {
      READ_CODE(hrd->getDpbOutputDelayDuLengthMinus1()+1, code, "pic_dpb_output_du_delay" );
      sei.m_picDpbOutputDuDelay = code;
    }
    if( hrd->getSubPicCpbParamsPresentFlag() && hrd->getSubPicCpbParamsInPicTimingSEIFlag() )
    {
      READ_UVLC( code, "num_decoding_units_minus1");
      sei.m_numDecodingUnitsMinus1 = code;
      READ_FLAG( code, "du_common_cpb_removal_delay_flag" );
      sei.m_duCommonCpbRemovalDelayFlag = code;
      if( sei.m_duCommonCpbRemovalDelayFlag )
      {
        READ_CODE( ( hrd->getDuCpbRemovalDelayLengthMinus1() + 1 ), code, "du_common_cpb_removal_delay_minus1" );
        sei.m_duCommonCpbRemovalDelayMinus1 = code;
      }
      if( sei.m_numNalusInDuMinus1 != NULL )
      {
        delete sei.m_numNalusInDuMinus1;
      }
      sei.m_numNalusInDuMinus1 = new UInt[ ( sei.m_numDecodingUnitsMinus1 + 1 ) ];
      if( sei.m_duCpbRemovalDelayMinus1  != NULL )
      {
        delete sei.m_duCpbRemovalDelayMinus1;
      }
      sei.m_duCpbRemovalDelayMinus1  = new UInt[ ( sei.m_numDecodingUnitsMinus1 + 1 ) ];

      for( i = 0; i <= sei.m_numDecodingUnitsMinus1; i ++ )
      {
        READ_UVLC( code, "num_nalus_in_du_minus1");
        sei.m_numNalusInDuMinus1[ i ] = code;
        if( ( !sei.m_duCommonCpbRemovalDelayFlag ) && ( i < sei.m_numDecodingUnitsMinus1 ) )
        {
          READ_CODE( ( hrd->getDuCpbRemovalDelayLengthMinus1() + 1 ), code, "du_cpb_removal_delay_minus1" );
          sei.m_duCpbRemovalDelayMinus1[ i ] = code;
        }
      }
    }
  }
  xParseByteAlign();
}
Void SEIReader::xParseSEIRecoveryPoint(SEIRecoveryPoint& sei, UInt /*payloadSize*/)
{
  Int  iCode;
  UInt uiCode;
  READ_SVLC( iCode,  "recovery_poc_cnt" );      sei.m_recoveryPocCnt     = iCode;
  READ_FLAG( uiCode, "exact_matching_flag" );   sei.m_exactMatchingFlag  = uiCode;
  READ_FLAG( uiCode, "broken_link_flag" );      sei.m_brokenLinkFlag     = uiCode;
  xParseByteAlign();
}
Void SEIReader::xParseSEIFramePacking(SEIFramePacking& sei, UInt /*payloadSize*/)
{
  UInt val;
  READ_UVLC( val, "frame_packing_arrangement_id" );                 sei.m_arrangementId = val;
  READ_FLAG( val, "frame_packing_arrangement_cancel_flag" );        sei.m_arrangementCancelFlag = val;

  if ( !sei.m_arrangementCancelFlag )
  {
    READ_CODE( 7, val, "frame_packing_arrangement_type" );          sei.m_arrangementType = val;
    assert((sei.m_arrangementType > 2) && (sei.m_arrangementType < 6) );
    READ_FLAG( val, "quincunx_sampling_flag" );                     sei.m_quincunxSamplingFlag = val;

    READ_CODE( 6, val, "content_interpretation_type" );             sei.m_contentInterpretationType = val;
    READ_FLAG( val, "spatial_flipping_flag" );                      sei.m_spatialFlippingFlag = val;
    READ_FLAG( val, "frame0_flipped_flag" );                        sei.m_frame0FlippedFlag = val;
    READ_FLAG( val, "field_views_flag" );                           sei.m_fieldViewsFlag = val;
    READ_FLAG( val, "current_frame_is_frame0_flag" );               sei.m_currentFrameIsFrame0Flag = val;
    READ_FLAG( val, "frame0_self_contained_flag" );                 sei.m_frame0SelfContainedFlag = val;
    READ_FLAG( val, "frame1_self_contained_flag" );                 sei.m_frame1SelfContainedFlag = val;

    if ( sei.m_quincunxSamplingFlag == 0 && sei.m_arrangementType != 5)
    {
      READ_CODE( 4, val, "frame0_grid_position_x" );                sei.m_frame0GridPositionX = val;
      READ_CODE( 4, val, "frame0_grid_position_y" );                sei.m_frame0GridPositionY = val;
      READ_CODE( 4, val, "frame1_grid_position_x" );                sei.m_frame1GridPositionX = val;
      READ_CODE( 4, val, "frame1_grid_position_y" );                sei.m_frame1GridPositionY = val;
    }

    READ_CODE( 8, val, "frame_packing_arrangement_reserved_byte" );   sei.m_arrangementReservedByte = val;
    READ_FLAG( val,  "frame_packing_arrangement_persistence_flag" );  sei.m_arrangementPersistenceFlag = val ? true : false;
  }
  READ_FLAG( val, "upsampled_aspect_ratio" );                       sei.m_upsampledAspectRatio = val;

  xParseByteAlign();
}

Void SEIReader::xParseSEIDisplayOrientation(SEIDisplayOrientation& sei, UInt /*payloadSize*/)
{
  UInt val;
  READ_FLAG( val,       "display_orientation_cancel_flag" );       sei.cancelFlag            = val;
  if( !sei.cancelFlag ) 
  {
    READ_FLAG( val,     "hor_flip" );                              sei.horFlip               = val;
    READ_FLAG( val,     "ver_flip" );                              sei.verFlip               = val;
    READ_CODE( 16, val, "anticlockwise_rotation" );                sei.anticlockwiseRotation = val;
    READ_FLAG( val,     "display_orientation_persistence_flag" );  sei.persistenceFlag       = val;
  }
  xParseByteAlign();
}

Void SEIReader::xParseSEITemporalLevel0Index(SEITemporalLevel0Index& sei, UInt /*payloadSize*/)
{
  UInt val;
  READ_CODE ( 8, val, "tl0_idx" );  sei.tl0Idx = val;
  READ_CODE ( 8, val, "rap_idx" );  sei.rapIdx = val;
  xParseByteAlign();
}

Void SEIReader::xParseSEIGradualDecodingRefreshInfo(SEIGradualDecodingRefreshInfo& sei, UInt /*payloadSize*/)
{
  UInt val;
  READ_FLAG( val, "gdr_foreground_flag" ); sei.m_gdrForegroundFlag = val ? 1 : 0;
  xParseByteAlign();
}

Void SEIReader::xParseSEIToneMappingInfo(SEIToneMappingInfo& sei, UInt /*payloadSize*/)
{
  Int i;
  UInt val;
  READ_UVLC( val, "tone_map_id" );                         sei.m_toneMapId = val;
  READ_FLAG( val, "tone_map_cancel_flag" );                sei.m_toneMapCancelFlag = val;

  if ( !sei.m_toneMapCancelFlag )
  {
    READ_FLAG( val, "tone_map_persistence_flag" );         sei.m_toneMapPersistenceFlag = val; 
    READ_CODE( 8, val, "coded_data_bit_depth" );           sei.m_codedDataBitDepth = val;
    READ_CODE( 8, val, "target_bit_depth" );               sei.m_targetBitDepth = val;
    READ_UVLC( val, "model_id" );                          sei.m_modelId = val; 
    switch(sei.m_modelId)
    {
    case 0:
      {
        READ_CODE( 32, val, "min_value" );                 sei.m_minValue = val;
        READ_CODE( 32, val, "max_value" );                 sei.m_maxValue = val;
        break;
      }
    case 1:
      {
        READ_CODE( 32, val, "sigmoid_midpoint" );          sei.m_sigmoidMidpoint = val;
        READ_CODE( 32, val, "sigmoid_width" );             sei.m_sigmoidWidth = val;
        break;
      }
    case 2:
      {
        UInt num = 1u << sei.m_targetBitDepth;
        sei.m_startOfCodedInterval.resize(num+1);
        for(i = 0; i < num; i++)
        {
          READ_CODE( ((( sei.m_codedDataBitDepth + 7 ) >> 3 ) << 3), val, "start_of_coded_interval" );
          sei.m_startOfCodedInterval[i] = val;
        }
        sei.m_startOfCodedInterval[num] = 1u << sei.m_codedDataBitDepth;
        break;
      }
    case 3:
      {
        READ_CODE( 16, val,  "num_pivots" );                       sei.m_numPivots = val;
        sei.m_codedPivotValue.resize(sei.m_numPivots);
        sei.m_targetPivotValue.resize(sei.m_numPivots);
        for(i = 0; i < sei.m_numPivots; i++ )
        {
          READ_CODE( ((( sei.m_codedDataBitDepth + 7 ) >> 3 ) << 3), val, "coded_pivot_value" );
          sei.m_codedPivotValue[i] = val;
          READ_CODE( ((( sei.m_targetBitDepth + 7 ) >> 3 ) << 3),    val, "target_pivot_value" );
          sei.m_targetPivotValue[i] = val;
        }
        break;
      }
    case 4:
      {
        READ_CODE( 8, val, "camera_iso_speed_idc" );                     sei.m_cameraIsoSpeedIdc = val;
        if( sei.m_cameraIsoSpeedIdc == 255) //Extended_ISO
        {
          READ_CODE( 32,   val,   "camera_iso_speed_value" );            sei.m_cameraIsoSpeedValue = val;
        }
        READ_CODE( 8, val, "exposure_index_idc" );                       sei.m_exposureIndexIdc = val;
        if( sei.m_exposureIndexIdc == 255) //Extended_ISO
        {
          READ_CODE( 32,   val,   "exposure_index_value" );              sei.m_exposureIndexValue = val;
        }
        READ_FLAG( val, "exposure_compensation_value_sign_flag" );       sei.m_exposureCompensationValueSignFlag = val;
        READ_CODE( 16, val, "exposure_compensation_value_numerator" );   sei.m_exposureCompensationValueNumerator = val;
        READ_CODE( 16, val, "exposure_compensation_value_denom_idc" );   sei.m_exposureCompensationValueDenomIdc = val;
        READ_CODE( 32, val, "ref_screen_luminance_white" );              sei.m_refScreenLuminanceWhite = val;
        READ_CODE( 32, val, "extended_range_white_level" );              sei.m_extendedRangeWhiteLevel = val;
        READ_CODE( 16, val, "nominal_black_level_luma_code_value" );     sei.m_nominalBlackLevelLumaCodeValue = val;
        READ_CODE( 16, val, "nominal_white_level_luma_code_value" );     sei.m_nominalWhiteLevelLumaCodeValue= val;
        READ_CODE( 16, val, "extended_white_level_luma_code_value" );    sei.m_extendedWhiteLevelLumaCodeValue = val;
        break;
      }
    default:
      {
        assert(!"Undefined SEIToneMapModelId");
        break;
      }
    }//switch model id
  }// if(!sei.m_toneMapCancelFlag) 

  xParseByteAlign();
}

#if P0050_KNEE_FUNCTION_SEI
Void SEIReader::xParseSEIKneeFunctionInfo(SEIKneeFunctionInfo& sei, UInt /*payloadSize*/){
  Int i;
  UInt val;
  READ_UVLC( val, "knee_function_id" );                   sei.m_kneeId = val;
  READ_FLAG( val, "knee_function_cancel_flag" );          sei.m_kneeCancelFlag = val;
  if ( !sei.m_kneeCancelFlag )
  {
    READ_FLAG( val, "knee_function_persistence_flag" );   sei.m_kneePersistenceFlag = val;
    READ_FLAG( val, "mapping_flag" );                     sei.m_kneeMappingFlag = val;
    READ_CODE( 32, val, "input_d_range" );                sei.m_kneeInputDrange = val;
    READ_CODE( 32, val, "input_disp_luminance" );         sei.m_kneeInputDispLuminance = val;
    READ_CODE( 32, val, "output_d_range" );               sei.m_kneeOutputDrange = val;
    READ_CODE( 32, val, "output_disp_luminance" );        sei.m_kneeOutputDispLuminance = val;
    READ_UVLC( val, "num_knee_points_minus1" );           sei.m_kneeNumKneePointsMinus1 = val;
    assert( sei.m_kneeNumKneePointsMinus1 > 0 );
    sei.m_kneeInputKneePoint.resize(sei.m_kneeNumKneePointsMinus1+1);
    sei.m_kneeOutputKneePoint.resize(sei.m_kneeNumKneePointsMinus1+1);
    for(i = 0; i <= sei.m_kneeNumKneePointsMinus1; i++ )
    {
      READ_CODE( 10, val, "input_knee_point" );           sei.m_kneeInputKneePoint[i] = val;
      READ_CODE( 10, val, "output_knee_point" );          sei.m_kneeOutputKneePoint[i] = val;
    }
  }
}
#endif

#if Q0074_COLOUR_REMAPPING_SEI
Void SEIReader::xParseSEIColourRemappingInfo(SEIColourRemappingInfo& sei, UInt /*payloadSize*/)
{
  UInt  uiVal;
  Int   iVal;

  READ_UVLC( uiVal, "colour_remap_id" );          sei.m_colourRemapId = uiVal;
  READ_FLAG( uiVal, "colour_remap_cancel_flag" ); sei.m_colourRemapCancelFlag = uiVal;
  if( !sei.m_colourRemapCancelFlag ) 
  {
    READ_FLAG( uiVal, "colour_remap_persistence_flag" );                sei.m_colourRemapPersistenceFlag = uiVal;
    READ_FLAG( uiVal, "colour_remap_video_signal_info_present_flag" );  sei.m_colourRemapVideoSignalInfoPresentFlag = uiVal;
    if ( sei.m_colourRemapVideoSignalInfoPresentFlag )
    {
      READ_FLAG( uiVal,    "colour_remap_full_range_flag" );           sei.m_colourRemapFullRangeFlag = uiVal;
      READ_CODE( 8, uiVal, "colour_remap_primaries" );                 sei.m_colourRemapPrimaries = uiVal;
      READ_CODE( 8, uiVal, "colour_remap_transfer_function" );         sei.m_colourRemapTransferFunction = uiVal;
      READ_CODE( 8, uiVal, "colour_remap_matrix_coefficients" );       sei.m_colourRemapMatrixCoefficients = uiVal;
    }
    READ_CODE( 8, uiVal, "colour_remap_input_bit_depth" ); sei.m_colourRemapInputBitDepth = uiVal;
    READ_CODE( 8, uiVal, "colour_remap_bit_depth" ); sei.m_colourRemapBitDepth = uiVal;
  
    for( Int c=0 ; c<3 ; c++ )
    {
      READ_CODE( 8, uiVal, "pre_lut_num_val_minus1[c]" ); sei.m_preLutNumValMinus1[c] = (uiVal==0) ? 1 : uiVal;
      sei.m_preLutCodedValue[c].resize(sei.m_preLutNumValMinus1[c]+1);
      sei.m_preLutTargetValue[c].resize(sei.m_preLutNumValMinus1[c]+1);
      if( uiVal> 0 )
        for ( Int i=0 ; i<=sei.m_preLutNumValMinus1[c] ; i++ )
        {
          READ_CODE( (( sei.m_colourRemapInputBitDepth   + 7 ) >> 3 ) << 3, uiVal, "pre_lut_coded_value[c][i]" );  sei.m_preLutCodedValue[c][i]  = uiVal;
          READ_CODE( (( sei.m_colourRemapBitDepth + 7 ) >> 3 ) << 3, uiVal, "pre_lut_target_value[c][i]" ); sei.m_preLutTargetValue[c][i] = uiVal;
        }
      else // pre_lut_num_val_minus1[c] == 0
      {
        sei.m_preLutCodedValue[c][0]  = 0;
        sei.m_preLutTargetValue[c][0] = 0;
        sei.m_preLutCodedValue[c][1]  = (1 << sei.m_colourRemapInputBitDepth) - 1 ;
        sei.m_preLutTargetValue[c][1] = (1 << sei.m_colourRemapBitDepth) - 1 ;
      }
    }

    READ_FLAG( uiVal,      "colour_remap_matrix_present_flag" ); sei.m_colourRemapMatrixPresentFlag = uiVal;
    if( sei.m_colourRemapMatrixPresentFlag )
    {
      READ_CODE( 4, uiVal, "log2_matrix_denom" ); sei.m_log2MatrixDenom = uiVal;
      for ( Int c=0 ; c<3 ; c++ )
        for ( Int i=0 ; i<3 ; i++ )
        {
          READ_SVLC( iVal, "colour_remap_coeffs[c][i]" ); sei.m_colourRemapCoeffs[c][i] = iVal;
        }
    }
    else // setting default matrix (I3)
    {
      sei.m_log2MatrixDenom = 0;
      for ( Int c=0 ; c<3 ; c++ )
        for ( Int i=0 ; i<3 ; i++ )
          sei.m_colourRemapCoeffs[c][i] = (c==i) ? 1 : 0;
    }
    for( Int c=0 ; c<3 ; c++ )
    {
      READ_CODE( 8, uiVal, "post_lut_num_val_minus1[c]" ); sei.m_postLutNumValMinus1[c] = (uiVal==0) ? 1 : uiVal;
      sei.m_postLutCodedValue[c].resize(sei.m_postLutNumValMinus1[c]+1);
      sei.m_postLutTargetValue[c].resize(sei.m_postLutNumValMinus1[c]+1);
      if( uiVal > 0 )
        for ( Int i=0 ; i<=sei.m_postLutNumValMinus1[c] ; i++ )
        {
          READ_CODE( (( sei.m_colourRemapBitDepth + 7 ) >> 3 ) << 3, uiVal, "post_lut_coded_value[c][i]" );  sei.m_postLutCodedValue[c][i] = uiVal;
          READ_CODE( (( sei.m_colourRemapBitDepth + 7 ) >> 3 ) << 3, uiVal, "post_lut_target_value[c][i]" ); sei.m_postLutTargetValue[c][i] = uiVal;
        }
      else
      {
        sei.m_postLutCodedValue[c][0]  = 0;
        sei.m_postLutTargetValue[c][0] = 0;
        sei.m_postLutTargetValue[c][1] = (1 << sei.m_colourRemapBitDepth) - 1;
        sei.m_postLutCodedValue[c][1]  = (1 << sei.m_colourRemapBitDepth) - 1;
      }
    }
  }

  xParseByteAlign();
}
#endif

Void SEIReader::xParseSEISOPDescription(SEISOPDescription &sei, UInt payloadSize)
{
  Int iCode;
  UInt uiCode;

  READ_UVLC( uiCode,           "sop_seq_parameter_set_id"            ); sei.m_sopSeqParameterSetId = uiCode;
  READ_UVLC( uiCode,           "num_pics_in_sop_minus1"              ); sei.m_numPicsInSopMinus1 = uiCode;
  for (UInt i = 0; i <= sei.m_numPicsInSopMinus1; i++)
  {
    READ_CODE( 6, uiCode,                     "sop_desc_vcl_nalu_type" );  sei.m_sopDescVclNaluType[i] = uiCode;
    READ_CODE( 3, sei.m_sopDescTemporalId[i], "sop_desc_temporal_id"   );  sei.m_sopDescTemporalId[i] = uiCode;
    if (sei.m_sopDescVclNaluType[i] != NAL_UNIT_CODED_SLICE_IDR_W_RADL && sei.m_sopDescVclNaluType[i] != NAL_UNIT_CODED_SLICE_IDR_N_LP)
    {
      READ_UVLC( sei.m_sopDescStRpsIdx[i],    "sop_desc_st_rps_idx"    ); sei.m_sopDescStRpsIdx[i] = uiCode;
    }
    if (i > 0)
    {
      READ_SVLC( iCode,                       "sop_desc_poc_delta"     ); sei.m_sopDescPocDelta[i] = iCode;
    }
  }

  xParseByteAlign();
}

#if Q0189_TMVP_CONSTRAINTS 
Void SEIReader::xParseSEITMVPConstraints   (SEITMVPConstrains& sei, UInt payloadSize)
{
  UInt uiCode;
  READ_UVLC( uiCode,           "prev_pics_not_used_flag"              ); sei.prev_pics_not_used_flag = uiCode;
  READ_UVLC( uiCode,           "no_intra_layer_col_pic_flag"            ); sei.no_intra_layer_col_pic_flag = uiCode;
  xParseByteAlign();
}
#endif

#if Q0247_FRAME_FIELD_INFO
Void SEIReader::xParseSEIFrameFieldInfo    (SEIFrameFieldInfo& sei, UInt payloadSize)
{
  UInt code;
  READ_CODE( 4, code, "ffinfo_pic_struct" );             sei.m_ffinfo_picStruct            = code;
  READ_CODE( 2, code, "ffinfo_source_scan_type" );       sei.m_ffinfo_sourceScanType = code;
  READ_FLAG(    code, "ffinfo_duplicate_flag" );         sei.m_ffinfo_duplicateFlag    = ( code == 1 ? true : false );
  xParseByteAlign();
}
#endif

#if LAYERS_NOT_PRESENT_SEI
Void SEIReader::xParseSEIScalableNesting(SEIScalableNesting& sei, const NalUnitType nalUnitType, UInt payloadSize, TComVPS *vps, TComSPS *sps)
#else
Void SEIReader::xParseSEIScalableNesting(SEIScalableNesting& sei, const NalUnitType nalUnitType, UInt payloadSize, TComSPS *sps)
#endif
{
  UInt uiCode;
  SEIMessages seis;

  READ_FLAG( uiCode,            "bitstream_subset_flag"         ); sei.m_bitStreamSubsetFlag = uiCode;
  READ_FLAG( uiCode,            "nesting_op_flag"               ); sei.m_nestingOpFlag = uiCode;
  if (sei.m_nestingOpFlag)
  {
    READ_FLAG( uiCode,            "default_op_flag"               ); sei.m_defaultOpFlag = uiCode;
    READ_UVLC( uiCode,            "nesting_num_ops_minus1"        ); sei.m_nestingNumOpsMinus1 = uiCode;
    for (UInt i = sei.m_defaultOpFlag; i <= sei.m_nestingNumOpsMinus1; i++)
    {
      READ_CODE( 3,        uiCode,  "nesting_max_temporal_id_plus1"   ); sei.m_nestingMaxTemporalIdPlus1[i] = uiCode;
      READ_UVLC( uiCode,            "nesting_op_idx"                  ); sei.m_nestingOpIdx[i] = uiCode;
    }
  }
  else
  {
    READ_FLAG( uiCode,            "all_layers_flag"               ); sei.m_allLayersFlag       = uiCode;
    if (!sei.m_allLayersFlag)
    {
      READ_CODE( 3,        uiCode,  "nesting_no_op_max_temporal_id_plus1"  ); sei.m_nestingNoOpMaxTemporalIdPlus1 = uiCode;
      READ_UVLC( uiCode,            "nesting_num_layers_minus1"            ); sei.m_nestingNumLayersMinus1        = uiCode;
      for (UInt i = 0; i <= sei.m_nestingNumLayersMinus1; i++)
      {
        READ_CODE( 6,           uiCode,     "nesting_layer_id"      ); sei.m_nestingLayerId[i]   = uiCode;
      }
    }
  }

  // byte alignment
  while ( m_pcBitstream->getNumBitsRead() % 8 != 0 )
  {
    UInt code;
    READ_FLAG( code, "nesting_zero_bit" );
  }

  sei.m_callerOwnsSEIs = false;

  // read nested SEI messages
  do {
#if O0164_MULTI_LAYER_HRD
#if LAYERS_NOT_PRESENT_SEI
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, vps, sps, &sei);
#else
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, sps, &sei);
#endif
#else
#if LAYERS_NOT_PRESENT_SEI
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, vps, sps);
#else
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, sps);
#endif
#endif
  } while (m_pcBitstream->getNumBitsLeft() > 8);

}

Void SEIReader::xParseByteAlign()
{
  UInt code;
  if( m_pcBitstream->getNumBitsRead() % 8 != 0 )
  {
    READ_FLAG( code, "bit_equal_to_one" );          assert( code == 1 );
  }
  while( m_pcBitstream->getNumBitsRead() % 8 != 0 )
  {
    READ_FLAG( code, "bit_equal_to_zero" );         assert( code == 0 );
  }
}

#if SVC_EXTENSION
#if LAYERS_NOT_PRESENT_SEI
Void SEIReader::xParseSEILayersNotPresent(SEILayersNotPresent &sei, UInt payloadSize, TComVPS *vps)
{
  UInt uiCode;
  UInt i = 0;

  READ_UVLC( uiCode,           "lp_sei_active_vps_id" ); sei.m_activeVpsId = uiCode;
  assert(vps->getVPSId() == sei.m_activeVpsId);
  sei.m_vpsMaxLayers = vps->getMaxLayers();
  for (; i < sei.m_vpsMaxLayers; i++)
  {
    READ_FLAG( uiCode,         "layer_not_present_flag"   ); sei.m_layerNotPresentFlag[i] = uiCode ? true : false;
  }
  for (; i < MAX_LAYERS; i++)
  {
    sei.m_layerNotPresentFlag[i] = false;
  }
  xParseByteAlign();
}
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
Void SEIReader::xParseSEIInterLayerConstrainedTileSets (SEIInterLayerConstrainedTileSets &sei, UInt payloadSize)
{
  UInt uiCode;

  READ_FLAG( uiCode, "il_all_tiles_exact_sample_value_match_flag"   ); sei.m_ilAllTilesExactSampleValueMatchFlag = uiCode;
  READ_FLAG( uiCode, "il_one_tile_per_tile_set_flag"                ); sei.m_ilOneTilePerTileSetFlag = uiCode;
  if( !sei.m_ilOneTilePerTileSetFlag )
  {
    READ_UVLC( uiCode, "il_num_sets_in_message_minus1"                ); sei.m_ilNumSetsInMessageMinus1 = uiCode;
    if( sei.m_ilNumSetsInMessageMinus1 )
    {
      READ_FLAG( uiCode, "skipped_tile_set_present_flag"                ); sei.m_skippedTileSetPresentFlag = uiCode;
    }
    else
    {
      sei.m_skippedTileSetPresentFlag = false;
    }
    UInt numSignificantSets = sei.m_ilNumSetsInMessageMinus1 - (sei.m_skippedTileSetPresentFlag ? 1 : 0) + 1;
    for( UInt i = 0; i < numSignificantSets; i++ )
    {
      READ_UVLC( uiCode, "ilcts_id"                                     ); sei.m_ilctsId[i] = uiCode;
      READ_UVLC( uiCode, "il_num_tile_rects_in_set_minus1"              ) ;sei.m_ilNumTileRectsInSetMinus1[i] = uiCode;
      for( UInt j = 0; j <= sei.m_ilNumTileRectsInSetMinus1[i]; j++ )
      {
        READ_UVLC( uiCode, "il_top_left_tile_index"                       ); sei.m_ilTopLeftTileIndex[i][j] = uiCode;
        READ_UVLC( uiCode, "il_bottom_right_tile_index"                   ); sei.m_ilBottomRightTileIndex[i][j] = uiCode;
      }
      READ_CODE( 2, uiCode, "ilc_idc"                                   ); sei.m_ilcIdc[i] = uiCode;
      if( sei.m_ilAllTilesExactSampleValueMatchFlag )
      {
        READ_FLAG( uiCode, "il_exact_sample_value_match_flag"             ); sei.m_ilExactSampleValueMatchFlag[i] = uiCode;
      }
    }
  }
  else
  {
    READ_CODE( 2, uiCode, "all_tiles_ilc_idc"                         ); sei.m_allTilesIlcIdc = uiCode;
  }

  xParseByteAlign();
}
#endif
#if SUB_BITSTREAM_PROPERTY_SEI
#if OLS_IDX_CHK
Void SEIReader::xParseSEISubBitstreamProperty(SEISubBitstreamProperty &sei, TComVPS *vps)
#else
Void SEIReader::xParseSEISubBitstreamProperty(SEISubBitstreamProperty &sei)
#endif
{
  UInt uiCode;
  READ_CODE( 4, uiCode, "active_vps_id" );                      sei.m_activeVpsId = uiCode;
  READ_UVLC(    uiCode, "num_additional_sub_streams_minus1" );  sei.m_numAdditionalSubStreams = uiCode + 1;

  for( Int i = 0; i < sei.m_numAdditionalSubStreams; i++ )
  {
    READ_CODE(  2, uiCode, "sub_bitstream_mode[i]"           ); sei.m_subBitstreamMode[i] = uiCode;
    READ_UVLC(     uiCode, "output_layer_set_idx_to_vps[i]"  );
#if OLS_IDX_CHK
      // The value of output_layer_set_idx_to_vps[ i ]  shall be in the range of 0 to NumOutputLayerSets − 1, inclusive.
      assert(uiCode > 0 && uiCode <= vps->getNumOutputLayerSets()-1);
#endif
      sei.m_outputLayerSetIdxToVps[i] = uiCode;
    READ_CODE(  3, uiCode, "highest_sub_layer_id[i]"         ); sei.m_highestSublayerId[i] = uiCode;
    READ_CODE( 16, uiCode, "avg_bit_rate[i]"                 ); sei.m_avgBitRate[i] = uiCode;
    READ_CODE( 16, uiCode, "max_bit_rate[i]"                 ); sei.m_maxBitRate[i] = uiCode;
  }
  xParseByteAlign();
}
#endif

#if O0164_MULTI_LAYER_HRD
#if LAYERS_NOT_PRESENT_SEI
Void SEIReader::xParseSEIBspNesting(SEIBspNesting &sei, const NalUnitType nalUnitType, TComVPS *vps, TComSPS *sps, const SEIScalableNesting &nestingSei)
#else
Void SEIReader::xParseSEIBspNesting(SEIBspNesting &sei, const NalUnitType nalUnitType, TComSPS *sps, const SEIScalableNesting &nestingSei)
#endif
{
  UInt uiCode;
  READ_UVLC( uiCode, "bsp_idx" ); sei.m_bspIdx = uiCode;

  // byte alignment
  while ( m_pcBitstream->getNumBitsRead() % 8 != 0 )
  {
    UInt code;
    READ_FLAG( code, "bsp_nesting_zero_bit" );
  }

  sei.m_callerOwnsSEIs = false;

  // read nested SEI messages
#if NESTING_SEI_EXTENSIBILITY
  Int numSeiMessages = 0;
  READ_UVLC( uiCode, "num_seis_in_bsp_minus1" );  assert( uiCode <= MAX_SEIS_IN_BSP_NESTING );
  numSeiMessages = uiCode;
  for(Int i = 0; i < numSeiMessages; i++)
  {
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, vps, sps, &nestingSei, &sei);
  }
#else
  do {
#if LAYERS_NOT_PRESENT_SEI
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, vps, sps, &nestingSei, &sei);
#else
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, sps, &nestingSei);
#endif
  } while (m_pcBitstream->getNumBitsLeft() > 8);
#endif
}

Void SEIReader::xParseSEIBspInitialArrivalTime(SEIBspInitialArrivalTime &sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting &nestingSei, const SEIBspNesting &bspNestingSei)
{
  assert(vps->getVpsVuiPresentFlag());

#if VPS_VUI_BSP_HRD_PARAMS
  UInt uiCode;
  Int psIdx         = bspNestingSei.m_seiPartitioningSchemeIdx;
  Int seiOlsIdx     = bspNestingSei.m_seiOlsIdx;
  Int maxTemporalId = nestingSei.m_nestingMaxTemporalIdPlus1[0];
  Int maxValues     = vps->getNumBspSchedulesMinus1(seiOlsIdx, psIdx, maxTemporalId) + 1;
  std::vector<Int> hrdIdx(0, maxValues);
  std::vector<TComHRD *> hrdVec;
  std::vector<Int> syntaxElemLen;
  for(Int i = 0; i < maxValues; i++)
  {
    hrdIdx[i] = vps->getBspHrdIdx( seiOlsIdx, psIdx, maxTemporalId, i, bspNestingSei.m_bspIdx);
    hrdVec[i] = vps->getBspHrd(hrdIdx[i]);
    
    syntaxElemLen[i] = hrdVec[i]->getInitialCpbRemovalDelayLengthMinus1() + 1;
    if ( !(hrdVec[i]->getNalHrdParametersPresentFlag() || hrdVec[i]->getVclHrdParametersPresentFlag()) )
    {
      assert( syntaxElemLen[i] == 24 ); // Default value of init_cpb_removal_delay_length_minus1 is 23
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
      READ_CODE( syntaxElemLen[i], uiCode, "nal_initial_arrival_delay[i]" ); sei.m_nalInitialArrivalDelay[i] = uiCode;
    }
  }
  if( hrdVec[0]->getVclHrdParametersPresentFlag() )
  {
    for(UInt i = 0; i < maxValues; i++)
    {
      READ_CODE( syntaxElemLen[i], uiCode, "vcl_initial_arrival_delay[i]" ); sei.m_vclInitialArrivalDelay[i] = uiCode;
    }
  }
#else
  UInt schedCombCnt = vps->getNumBspSchedCombinations(nestingSei.m_nestingOpIdx[0]);
  UInt len;
  UInt hrdIdx;
  UInt uiCode;

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
      READ_CODE( len, uiCode, "nal_initial_arrival_delay" ); sei.m_nalInitialArrivalDelay[i] = uiCode;
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
      READ_CODE( len, uiCode, "vcl_initial_arrival_delay" ); sei.m_vclInitialArrivalDelay[i] = uiCode;
    }
  }
#endif
}

#if !REMOVE_BSP_HRD_SEI
Void SEIReader::xParseSEIBspHrd(SEIBspHrd &sei, TComSPS *sps, const SEIScalableNesting &nestingSei)
{
  UInt uiCode;
  READ_UVLC( uiCode, "sei_num_bsp_hrd_parameters_minus1" ); sei.m_seiNumBspHrdParametersMinus1 = uiCode;
  for (UInt i = 0; i <= sei.m_seiNumBspHrdParametersMinus1; i++)
  {
    if (i > 0)
    {
      READ_FLAG( uiCode, "sei_bsp_cprms_present_flag" ); sei.m_seiBspCprmsPresentFlag[i] = uiCode;
    }
    xParseHrdParameters(sei.hrd, i==0 ? 1 : sei.m_seiBspCprmsPresentFlag[i], nestingSei.m_nestingMaxTemporalIdPlus1[0]-1);
  }
  for (UInt h = 0; h <= nestingSei.m_nestingNumOpsMinus1; h++)
  {
    UInt lsIdx = nestingSei.m_nestingOpIdx[h];
    READ_UVLC( uiCode, "num_sei_bitstream_partitions_minus1[i]"); sei.m_seiNumBitstreamPartitionsMinus1[lsIdx] = uiCode;
#if HRD_BPB
    Int chkPart=0;
#endif
    UInt i;
    for(i = 0; i <= sei.m_seiNumBitstreamPartitionsMinus1[lsIdx]; i++)
    {
#if HRD_BPB
      UInt nl=0; UInt j;
      for(j = 0; j < sei.m_vpsMaxLayers; j++)
      {
        if (sei.m_layerIdIncludedFlag[lsIdx][j])
        {
          nl++;
        }
      }
      for (j = 0; j < nl; j++)
      {
#else
      for (UInt j = 0; j < sei.m_vpsMaxLayers; j++)
      {
        if (sei.m_layerIdIncludedFlag[lsIdx][j])
        {
#endif
          READ_FLAG( uiCode, "sei_layer_in_bsp_flag[lsIdx][i][j]" ); sei.m_seiLayerInBspFlag[lsIdx][i][j] = uiCode;
        }
#if !HRD_BPB
      }
#endif
#if HRD_BPB
      chkPart+=sei.m_seiLayerInBspFlag[lsIdx][i][j];
#endif
    }
#if HRD_BPB
    assert(chkPart<=1);
#endif
#if HRD_BPB
    if(sei.m_seiNumBitstreamPartitionsMinus1[lsIdx]==0)
    {
      Int chkPartition1=0; Int chkPartition2=0;
      for (UInt j = 0; j < sei.m_vpsMaxLayers; j++)
      {
        if( sei.m_layerIdIncludedFlag[lsIdx][j] )
        {
          chkPartition1+=sei.m_seiLayerInBspFlag[lsIdx][0][j];
          chkPartition2++;
        }
      }
      assert(chkPartition1!=chkPartition2);
    }
#endif
      
    READ_UVLC( uiCode, "sei_num_bsp_sched_combinations_minus1[i]"); sei.m_seiNumBspSchedCombinationsMinus1[lsIdx] = uiCode;
    for (i = 0; i <= sei.m_seiNumBspSchedCombinationsMinus1[lsIdx]; i++)
    {
      for (UInt j = 0; j <= sei.m_seiNumBitstreamPartitionsMinus1[lsIdx]; j++)
      {
        READ_UVLC( uiCode, "sei_bsp_comb_hrd_idx[lsIdx][i][j]"); sei.m_seiBspCombHrdIdx[lsIdx][i][j] = uiCode;
#if HRD_BPB
        assert(uiCode <= sei.m_seiNumBspHrdParametersMinus1);
#endif
        READ_UVLC( uiCode, "sei_bsp_comb_sched_idx[lsIdx][i][j]"); sei.m_seiBspCombScheddx[lsIdx][i][j] = uiCode;
#if HRD_BPB
        assert(uiCode <= sei.hrd->getCpbCntMinus1( sps->getMaxTLayers()-1 ));
#endif

      }
    }
  }
}
#endif

Void SEIReader::xParseHrdParameters(TComHRD *hrd, Bool commonInfPresentFlag, UInt maxNumSubLayersMinus1)
{
  UInt  uiCode;
  if( commonInfPresentFlag )
  {
    READ_FLAG( uiCode, "nal_hrd_parameters_present_flag" );           hrd->setNalHrdParametersPresentFlag( uiCode == 1 ? true : false );
    READ_FLAG( uiCode, "vcl_hrd_parameters_present_flag" );           hrd->setVclHrdParametersPresentFlag( uiCode == 1 ? true : false );
    if( hrd->getNalHrdParametersPresentFlag() || hrd->getVclHrdParametersPresentFlag() )
    {
      READ_FLAG( uiCode, "sub_pic_cpb_params_present_flag" );         hrd->setSubPicCpbParamsPresentFlag( uiCode == 1 ? true : false );
      if( hrd->getSubPicCpbParamsPresentFlag() )
      {
        READ_CODE( 8, uiCode, "tick_divisor_minus2" );                hrd->setTickDivisorMinus2( uiCode );
        READ_CODE( 5, uiCode, "du_cpb_removal_delay_length_minus1" ); hrd->setDuCpbRemovalDelayLengthMinus1( uiCode );
        READ_FLAG( uiCode, "sub_pic_cpb_params_in_pic_timing_sei_flag" ); hrd->setSubPicCpbParamsInPicTimingSEIFlag( uiCode == 1 ? true : false );
        READ_CODE( 5, uiCode, "dpb_output_delay_du_length_minus1"  ); hrd->setDpbOutputDelayDuLengthMinus1( uiCode );
      }
      READ_CODE( 4, uiCode, "bit_rate_scale" );                       hrd->setBitRateScale( uiCode );
      READ_CODE( 4, uiCode, "cpb_size_scale" );                       hrd->setCpbSizeScale( uiCode );
      if( hrd->getSubPicCpbParamsPresentFlag() )
      {
        READ_CODE( 4, uiCode, "cpb_size_du_scale" );                  hrd->setDuCpbSizeScale( uiCode );
      }
      READ_CODE( 5, uiCode, "initial_cpb_removal_delay_length_minus1" ); hrd->setInitialCpbRemovalDelayLengthMinus1( uiCode );
      READ_CODE( 5, uiCode, "au_cpb_removal_delay_length_minus1" );      hrd->setCpbRemovalDelayLengthMinus1( uiCode );
      READ_CODE( 5, uiCode, "dpb_output_delay_length_minus1" );       hrd->setDpbOutputDelayLengthMinus1( uiCode );
    }
  }
  Int i, j, nalOrVcl;
  for( i = 0; i <= maxNumSubLayersMinus1; i ++ )
  {
    READ_FLAG( uiCode, "fixed_pic_rate_general_flag" );                     hrd->setFixedPicRateFlag( i, uiCode == 1 ? true : false  );
    if( !hrd->getFixedPicRateFlag( i ) )
    {
      READ_FLAG( uiCode, "fixed_pic_rate_within_cvs_flag" );                hrd->setFixedPicRateWithinCvsFlag( i, uiCode == 1 ? true : false  );
    }
    else
    {
      hrd->setFixedPicRateWithinCvsFlag( i, true );
    }
    hrd->setLowDelayHrdFlag( i, 0 ); // Infered to be 0 when not present
    hrd->setCpbCntMinus1   ( i, 0 ); // Infered to be 0 when not present
    if( hrd->getFixedPicRateWithinCvsFlag( i ) )
    {
      READ_UVLC( uiCode, "elemental_duration_in_tc_minus1" );             hrd->setPicDurationInTcMinus1( i, uiCode );
    }
    else
    {
      READ_FLAG( uiCode, "low_delay_hrd_flag" );                      hrd->setLowDelayHrdFlag( i, uiCode == 1 ? true : false  );
    }
    if (!hrd->getLowDelayHrdFlag( i ))
    {
      READ_UVLC( uiCode, "cpb_cnt_minus1" );                          hrd->setCpbCntMinus1( i, uiCode );
    }
    for( nalOrVcl = 0; nalOrVcl < 2; nalOrVcl ++ )
    {
      if( ( ( nalOrVcl == 0 ) && ( hrd->getNalHrdParametersPresentFlag() ) ) ||
          ( ( nalOrVcl == 1 ) && ( hrd->getVclHrdParametersPresentFlag() ) ) )
      {
        for( j = 0; j <= ( hrd->getCpbCntMinus1( i ) ); j ++ )
        {
          READ_UVLC( uiCode, "bit_rate_value_minus1" );             hrd->setBitRateValueMinus1( i, j, nalOrVcl, uiCode );
          READ_UVLC( uiCode, "cpb_size_value_minus1" );             hrd->setCpbSizeValueMinus1( i, j, nalOrVcl, uiCode );
          if( hrd->getSubPicCpbParamsPresentFlag() )
          {
            READ_UVLC( uiCode, "cpb_size_du_value_minus1" );       hrd->setDuCpbSizeValueMinus1( i, j, nalOrVcl, uiCode );
            READ_UVLC( uiCode, "bit_rate_du_value_minus1" );       hrd->setDuBitRateValueMinus1( i, j, nalOrVcl, uiCode );
          }
          READ_FLAG( uiCode, "cbr_flag" );                          hrd->setCbrFlag( i, j, nalOrVcl, uiCode == 1 ? true : false  );
        }
      }
    }
  }
}
#endif

#if Q0078_ADD_LAYER_SETS

#if LAYERS_NOT_PRESENT_SEI
Void SEIReader::xParseSEIOutputLayerSetNesting(SEIOutputLayerSetNesting& sei, const NalUnitType nalUnitType, TComVPS *vps, TComSPS *sps)
#else
Void SEIReader::xParseSEIOutputLayerSetNesting(SEIOutputLayerSetNesting& sei, const NalUnitType nalUnitType, TComSPS *sps)
#endif
{
  UInt uiCode;
  SEIMessages seis;

  READ_FLAG(uiCode, "ols_flag"); sei.m_olsFlag = uiCode;
  READ_UVLC(uiCode, "num_ols_indices_minus1"); sei.m_numOlsIndicesMinus1 = uiCode;

  for (Int i = 0; i <= sei.m_numOlsIndicesMinus1; i++)
  {
    READ_UVLC(uiCode, "ols_idx[i]"); sei.m_olsIdx[i] = uiCode;
  }

  // byte alignment
  while (m_pcBitstream->getNumBitsRead() % 8 != 0)
  {
    UInt code;
    READ_FLAG(code, "ols_nesting_zero_bit");
  }

  sei.m_callerOwnsSEIs = false;

  // read nested SEI messages
  do {
#if O0164_MULTI_LAYER_HRD
#if LAYERS_NOT_PRESENT_SEI
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, vps, sps);
#else
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, sps);
#endif
#else
#if LAYERS_NOT_PRESENT_SEI
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, vps, sps);
#else
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, sps);
#endif
#endif
  } while (m_pcBitstream->getNumBitsLeft() > 8);

}

Void SEIReader::xParseSEIVPSRewriting(SEIVPSRewriting &sei)
{
}

#endif

#endif //SVC_EXTENSION

//! \}
