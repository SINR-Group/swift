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

#pragma once

#include "SyntaxElementWriter.h"
#include "TLibCommon/SEI.h"

class TComBitIf;

//! \ingroup TLibEncoder
//! \{
class SEIWriter:public SyntaxElementWriter
{
public:
  SEIWriter() {};
  virtual ~SEIWriter() {};

#if O0164_MULTI_LAYER_HRD
  void writeSEImessage(TComBitIf& bs, const SEI& sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting* nestingSei=NULL, const SEIBspNesting* bspNestingSei=NULL);
#else
  void writeSEImessage(TComBitIf& bs, const SEI& sei, TComSPS *sps);
#endif

protected:
#if O0164_MULTI_LAYER_HRD
#if VPS_VUI_BSP_HRD_PARAMS
  Void xWriteSEIpayloadData(TComBitIf& bs, const SEI& sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei);
#else
  Void xWriteSEIpayloadData(TComBitIf& bs, const SEI& sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting& nestingSei, const SEIBspNesting& bspNestingSei);
#endif
#else
  Void xWriteSEIpayloadData(TComBitIf& bs, const SEI& sei, TComSPS *sps);
#endif
  Void xWriteSEIuserDataUnregistered(const SEIuserDataUnregistered &sei);
  Void xWriteSEIActiveParameterSets(const SEIActiveParameterSets& sei);
  Void xWriteSEIDecodedPictureHash(const SEIDecodedPictureHash& sei);
#if VPS_VUI_BSP_HRD_PARAMS
  Void xWriteSEIDecodingUnitInfo(const SEIDecodingUnitInfo& sei, TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei, TComVPS *vps);
  Void xWriteSEIBufferingPeriod(const SEIBufferingPeriod& sei, TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei, TComVPS *vps);
  Void xWriteSEIPictureTiming(const SEIPictureTiming& sei, TComSPS *sps, const SEIScalableNesting* nestingSei, const SEIBspNesting* bspNestingSei, TComVPS *vps);
#else
  Void xWriteSEIDecodingUnitInfo(const SEIDecodingUnitInfo& sei, TComSPS *sps);
  Void xWriteSEIBufferingPeriod(const SEIBufferingPeriod& sei, TComSPS *sps);
  Void xWriteSEIPictureTiming(const SEIPictureTiming& sei, TComSPS *sps);
#endif
  TComSPS *m_pSPS;
  Void xWriteSEIRecoveryPoint(const SEIRecoveryPoint& sei);
  Void xWriteSEIFramePacking(const SEIFramePacking& sei);
  Void xWriteSEIDisplayOrientation(const SEIDisplayOrientation &sei);
  Void xWriteSEITemporalLevel0Index(const SEITemporalLevel0Index &sei);
  Void xWriteSEIGradualDecodingRefreshInfo(const SEIGradualDecodingRefreshInfo &sei);
  Void xWriteSEIToneMappingInfo(const SEIToneMappingInfo& sei);
#if P0050_KNEE_FUNCTION_SEI
  Void xWriteSEIKneeFunctionInfo(const SEIKneeFunctionInfo &sei);
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  Void xWriteSEIColourRemappingInfo(const SEIColourRemappingInfo& sei);
#endif
  Void xWriteSEISOPDescription(const SEISOPDescription& sei);
#if O0164_MULTI_LAYER_HRD
  Void xWriteSEIScalableNesting(TComBitIf& bs, const SEIScalableNesting& sei, TComVPS *vps, TComSPS *sps);
#else
  Void xWriteSEIScalableNesting(TComBitIf& bs, const SEIScalableNesting& sei, TComSPS *sps);
#endif
  Void xWriteByteAlign();
#if SVC_EXTENSION
#if LAYERS_NOT_PRESENT_SEI
  Void xWriteSEILayersNotPresent(const SEILayersNotPresent& sei);
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
  Void xWriteSEIInterLayerConstrainedTileSets(const SEIInterLayerConstrainedTileSets& sei);
#endif
#if SUB_BITSTREAM_PROPERTY_SEI
  Void xWriteSEISubBitstreamProperty(const SEISubBitstreamProperty &sei);
#endif
#if Q0189_TMVP_CONSTRAINTS 
Void xWriteSEITMVPConstraints (const SEITMVPConstrains &sei);
#endif
#if Q0247_FRAME_FIELD_INFO
  Void xWriteSEIFrameFieldInfo  (const SEIFrameFieldInfo &sei);
#endif
#if O0164_MULTI_LAYER_HRD
  Void xWriteSEIBspNesting(TComBitIf& bs, const SEIBspNesting &sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting &nestingSei);
  Void xWriteSEIBspInitialArrivalTime(const SEIBspInitialArrivalTime &sei, TComVPS *vps, TComSPS *sps, const SEIScalableNesting &nestingSei, const SEIBspNesting &bspNestingSei);
#if !REMOVE_BSP_HRD_SEI
  Void xWriteSEIBspHrd(const SEIBspHrd &sei, TComSPS *sps, const SEIScalableNesting &nestingSei);
#endif
  Void xCodeHrdParameters( TComHRD *hrd, Bool commonInfPresentFlag, UInt maxNumSubLayersMinus1 );
#endif
#if Q0078_ADD_LAYER_SETS
  Void xWriteSEIOutputLayerSetNesting(TComBitIf& bs, const SEIOutputLayerSetNesting &sei, TComVPS *vps, TComSPS *sps);
  Void xWriteSEIVPSRewriting(const SEIVPSRewriting &sei);
#endif
#endif //SVC_EXTENSION
};

//! \}
