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

/** \file     TDecCAVLC.h
    \brief    CAVLC decoder class (header)
*/

#ifndef __TDECCAVLC__
#define __TDECCAVLC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TDecEntropy.h"
#include "SyntaxElementParser.h"

#if Q0048_CGS_3D_ASYMLUT
class TCom3DAsymLUT;
#endif
//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CAVLC decoder class
class TDecCavlc : public SyntaxElementParser, public TDecEntropyIf
{
public:
  TDecCavlc();
  virtual ~TDecCavlc();
  
protected:
  void  parseShortTermRefPicSet            (TComSPS* pcSPS, TComReferencePictureSet* pcRPS, Int idx);
  
public:

  /// rest entropy coder by intial QP and IDC in CABAC
  Void  resetEntropy        ( TComSlice* /*pcSlice*/  )     { assert(0); };
  Void  setBitstream        ( TComInputBitstream* p )   { m_pcBitstream = p; }
  Void  parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize );
  Void  parseQtCbf          ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth );
  Void  parseQtRootCbf      ( UInt uiAbsPartIdx, UInt& uiQtRootCbf );
  Void  parseVPS            ( TComVPS* pcVPS );
#if SVC_EXTENSION
  Void  parseVPSExtension   ( TComVPS* pcVPS );
  Void  defaultVPSExtension ( TComVPS* pcVPS );
  Void  parseVPSVUI         ( TComVPS* pcVPS );
  Void  defaultVPSVUI       ( TComVPS* pcVPS );
#if REPN_FORMAT_IN_VPS
  Void  parseRepFormat      ( RepFormat *repFormat, RepFormat *repFormatPrev );
#endif
#if VPS_DPB_SIZE_TABLE
  Void  parseVpsDpbSizeTable( TComVPS *vps );
#endif
#if VPS_VUI_BSP_HRD_PARAMS
  Void  parseVpsVuiBspHrdParams( TComVPS *vps );
#endif
#if SPS_DPB_PARAMS
  Void  parseSPS            ( TComSPS* pcSPS ); // it should be removed after macro clean up
#else
  Void  parseSPS            ( TComSPS* pcSPS, ParameterSetManagerDecoder *parameterSetManager );
#endif
  Void  parseSPSExtension    ( TComSPS* pcSPS );
#else //SVC_EXTENSION
  Void  parseSPS            ( TComSPS* pcSPS );
#endif //SVC_EXTENSION
  Void  parsePPS            ( TComPPS* pcPPS
#if Q0048_CGS_3D_ASYMLUT
    , TCom3DAsymLUT * pc3DAsymLUT , Int nLayerID
#endif
    );
  Void  parseVUI            ( TComVUI* pcVUI, TComSPS* pcSPS );
  Void  parseSEI            ( SEIMessages& );
  Void  parsePTL            ( TComPTL *rpcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1 );
  Void  parseProfileTier    (ProfileTierLevel *ptl);
  Void  parseHrdParameters  (TComHRD *hrd, Bool cprms_present_flag, UInt tempLevelHigh);
  Void  parseSliceHeader    ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager);
  Void  parseTerminatingBit ( UInt& ruiBit );
  
  Void  parseMVPIdx         ( Int& riMVPIdx );
  
  Void  parseSkipFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void  parseCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseMergeFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx );
  Void parseMergeIndex      ( TComDataCU* pcCU, UInt& ruiMergeIndex );
  Void parseSplitFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePartSize        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePredMode        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIntraDirLumaAng ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIntraDirChroma  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseInterDir        ( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx );
  Void parseRefFrmIdx       ( TComDataCU* pcCU, Int& riRefFrmIdx,  RefPicList eRefList );
  Void parseMvd             ( TComDataCU* pcCU, UInt uiAbsPartAddr,UInt uiPartIdx,    UInt uiDepth, RefPicList eRefList );
  
  Void parseDeltaQP         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseCoeffNxN        ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType );
  Void parseTransformSkipFlags ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt width, UInt height, UInt uiDepth, TextType eTType);

  Void parseIPCMInfo        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth);

  Void updateContextTables  ( SliceType /*eSliceType*/, Int /*iQp*/ ) { return; }

  Void xParsePredWeightTable ( TComSlice* pcSlice );
  Void  parseScalingList               ( TComScalingList* scalingList );
  Void xDecodeScalingList    ( TComScalingList *scalingList, UInt sizeId, UInt listId);
protected:
  Bool  xMoreRbspData();

#if Q0048_CGS_3D_ASYMLUT
  Void xParse3DAsymLUT( TCom3DAsymLUT * pc3DAsymLUT );
  Void xParse3DAsymLUTOctant( TCom3DAsymLUT * pc3DAsymLUT , Int nDepth , Int yIdx , Int uIdx , Int vIdx , Int nLength );
#if R0151_CGS_3D_ASYMLUT_IMPROVE
#if R0300_CGS_RES_COEFF_CODING
  Void xReadParam( Int& param, Int flc_bits );
#else
  Void xReadParam( Int& param );
#endif
#endif
#endif
};

//! \}

#endif // !defined(AFX_TDECCAVLC_H__9732DD64_59B0_4A41_B29E_1A5B18821EAD__INCLUDED_)

