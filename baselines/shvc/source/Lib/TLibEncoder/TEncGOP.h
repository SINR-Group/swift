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

/** \file     TEncGOP.h
    \brief    GOP encoder class (header)
*/

#ifndef __TENCGOP__
#define __TENCGOP__

#include <list>

#include <stdlib.h>

#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComLoopFilter.h"
#include "TLibCommon/AccessUnit.h"
#include "TEncSampleAdaptiveOffset.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"
#include "SEIwrite.h"
#if Q0048_CGS_3D_ASYMLUT
#include "TEnc3DAsymLUT.h"
#endif

#include "TEncAnalyze.h"
#include "TEncRateCtrl.h"
#include <vector>

//! \ingroup TLibEncoder
//! \{

class TEncTop;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// GOP encoder class
class TEncGOP
{
private:
  //  Data
  Bool                    m_bLongtermTestPictureHasBeenCoded;
  Bool                    m_bLongtermTestPictureHasBeenCoded2;
  UInt            m_numLongTermRefPicSPS;
  UInt            m_ltRefPicPocLsbSps[33];
  Bool            m_ltRefPicUsedByCurrPicFlag[33];
  Int                     m_iLastIDR;
  Int                     m_iGopSize;
  Int                     m_iNumPicCoded;
  Bool                    m_bFirst;
#if ALLOW_RECOVERY_POINT_AS_RAP
  Int                     m_iLastRecoveryPicPOC;
#endif
  
  //  Access channel
  TEncTop*                m_pcEncTop;
  TEncCfg*                m_pcCfg;
  TEncSlice*              m_pcSliceEncoder;
  TComList<TComPic*>*     m_pcListPic;
  
  TEncEntropy*            m_pcEntropyCoder;
  TEncCavlc*              m_pcCavlcCoder;
  TEncSbac*               m_pcSbacCoder;
  TEncBinCABAC*           m_pcBinCABAC;
  TComLoopFilter*         m_pcLoopFilter;

  SEIWriter               m_seiWriter;
  
  //--Adaptive Loop filter
  TEncSampleAdaptiveOffset*  m_pcSAO;
  TComBitCounter*         m_pcBitCounter;
  TEncRateCtrl*           m_pcRateCtrl;
  // indicate sequence first
  Bool                    m_bSeqFirst;
  
  // clean decoding refresh
  Bool                    m_bRefreshPending;
  Int                     m_pocCRA;
#if POC_RESET_IDC_ENCODER
  Int                     m_pocCraWithoutReset;
  Int                     m_associatedIrapPocBeforeReset;
#endif
  std::vector<Int>        m_storedStartCUAddrForEncodingSlice;
  std::vector<Int>        m_storedStartCUAddrForEncodingSliceSegment;
#if FIX1172
  NalUnitType             m_associatedIRAPType;
  Int                     m_associatedIRAPPOC;
#endif

  std::vector<Int> m_vRVM_RP;
  UInt                    m_lastBPSEI;
  UInt                    m_totalCoded;
  UInt                    m_cpbRemovalDelay;
  UInt                    m_tl0Idx;
  UInt                    m_rapIdx;
  Bool                    m_activeParameterSetSEIPresentInAU;
  Bool                    m_bufferingPeriodSEIPresentInAU;
  Bool                    m_pictureTimingSEIPresentInAU;
  Bool                    m_nestedBufferingPeriodSEIPresentInAU;
  Bool                    m_nestedPictureTimingSEIPresentInAU;

#if SVC_EXTENSION
  UInt                    m_layerId;      
  TEncTop**               m_ppcTEncTop;
  TEncSearch*             m_pcPredSearch;                       ///< encoder search class
#if Q0048_CGS_3D_ASYMLUT
  TEnc3DAsymLUT           m_Enc3DAsymLUTPicUpdate;
  TEnc3DAsymLUT           m_Enc3DAsymLUTPPS;
  TComPicYuv*             m_pColorMappedPic;

  Int m_iTap;
  const Int (*m_phase_filter)[13];
  const Int (*m_phase_filter_luma)[13];
  const Int (*m_phase_filter_chroma)[13];
  Int m_iM, m_iN;
  static const Int m_phase_filter_0_t0[4][13];
  static const Int m_phase_filter_0_t1[4][13];
  static const Int m_phase_filter_0_t1_chroma[4][13];
  static const Int m_phase_filter_1[8][13];
  Int   **m_temp;
#endif
#if POC_RESET_IDC_ENCODER
  Int   m_lastPocPeriodId;
#endif
#endif
  
public:
  TEncGOP();
  virtual ~TEncGOP();
  
#if SVC_EXTENSION
  Void  create      ( UInt layerId );
#else
  Void  create      ();
#endif
  Void  destroy     ();
  
  Void  init        ( TEncTop* pcTEncTop );
#if SVC_EXTENSION
  Void  compressGOP ( Int iPicIdInGOP, Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRec, std::list<AccessUnit>& accessUnitsInGOP, Bool isField, Bool isTff );
#else
  Void  compressGOP ( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRec, std::list<AccessUnit>& accessUnitsInGOP, Bool isField, Bool isTff );
#endif
#if POC_RESET_IDC_ENCODER
  Void  determinePocResetIdc( Int const pocCurr, TComSlice *const slice);
  Int   getIntraRefreshInterval()  { return m_pcCfg->getIntraPeriod(); }
  Int   getIntraRefreshType()      { return m_pcCfg->getDecodingRefreshType(); }
  // Int   getIntraRefreshInterval ()  { return (m_pcCfg) ? m_pcCfg->getIntraPeriod() : 0;  }
  Int   getLastPocPeriodId()      { return m_lastPocPeriodId; }
  Void  setLastPocPeriodId(Int x) { m_lastPocPeriodId = x;    }
  Void  updatePocValuesOfPics( Int const pocCurr, TComSlice *const slice);
#endif
  Void  xAttachSliceDataToNalUnit (OutputNALUnit& rNalu, TComOutputBitstream*& rpcBitstreamRedirect);

  
  Int   getGOPSize()          { return  m_iGopSize;  }
  
  TComList<TComPic*>*   getListPic()      { return m_pcListPic; }
  
#if !SVC_EXTENSION
  Void  printOutSummary      ( UInt uiNumAllPicCoded, Bool isField);
#endif
  Void  preLoopFilterPicAll  ( TComPic* pcPic, UInt64& ruiDist, UInt64& ruiBits );
  
  TEncSlice*  getSliceEncoder()   { return m_pcSliceEncoder; }
  NalUnitType getNalUnitType( Int pocCurr, Int lastIdr, Bool isField );
  Void arrangeLongtermPicturesInRPS(TComSlice *, TComList<TComPic*>& );
protected:
  TEncRateCtrl* getRateCtrl()       { return m_pcRateCtrl;  }

protected:
  
  Void  xInitGOP          ( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, bool isField );
  Void  xGetBuffer        ( TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, Int iNumPicRcvd, Int iTimeOffset, TComPic*& rpcPic, TComPicYuv*& rpcPicYuvRecOut, Int pocCurr, bool isField );
  
  Void  xCalculateAddPSNR ( TComPic* pcPic, TComPicYuv* pcPicD, const AccessUnit&, Double dEncTime );
  Void  xCalculateInterlacedAddPSNR( TComPic* pcPicOrgTop, TComPic* pcPicOrgBottom, TComPicYuv* pcPicRecTop, TComPicYuv* pcPicRecBottom, const AccessUnit& accessUnit, Double dEncTime );
  
  UInt64 xFindDistortionFrame (TComPicYuv* pcPic0, TComPicYuv* pcPic1);

  Double xCalculateRVM();

  SEIActiveParameterSets* xCreateSEIActiveParameterSets (TComSPS *sps);
  SEIFramePacking*        xCreateSEIFramePacking();
  SEIDisplayOrientation*  xCreateSEIDisplayOrientation();

  SEIToneMappingInfo*     xCreateSEIToneMappingInfo();
#if P0050_KNEE_FUNCTION_SEI
  SEIKneeFunctionInfo*    xCreateSEIKneeFunctionInfo();
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  SEIColourRemappingInfo* xCreateSEIColourRemappingInfo();
#endif

  Void xCreateLeadingSEIMessages (/*SEIMessages seiMessages,*/ AccessUnit &accessUnit, TComSPS *sps);
  Int xGetFirstSeiLocation (AccessUnit &accessUnit);
  Void xResetNonNestedSEIPresentFlags()
  {
    m_activeParameterSetSEIPresentInAU = false;
    m_bufferingPeriodSEIPresentInAU    = false;
    m_pictureTimingSEIPresentInAU      = false;
  }
  Void xResetNestedSEIPresentFlags()
  {
    m_nestedBufferingPeriodSEIPresentInAU    = false;
    m_nestedPictureTimingSEIPresentInAU      = false;
  }
  Void dblMetric( TComPic* pcPic, UInt uiNumSlices );

#if SVC_EXTENSION
#if LAYERS_NOT_PRESENT_SEI
  SEILayersNotPresent*    xCreateSEILayersNotPresent ();
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
  Void xBuildTileSetsMap(TComPicSym* picSym);
  SEIInterLayerConstrainedTileSets* xCreateSEIInterLayerConstrainedTileSets();
#endif
#if O0164_MULTI_LAYER_HRD
#if VPS_VUI_BSP_HRD_PARAMS
  SEIScalableNesting* xCreateBspNestingSEI(TComSlice *pcSlice, Int olsIdx, Int partitioningSchemeIdx, Int bspIdx);
#else
  SEIScalableNesting* xCreateBspNestingSEI(TComSlice *pcSlice);
#endif
#endif
#if Q0048_CGS_3D_ASYMLUT
  Void xDetermin3DAsymLUT( TComSlice * pSlice , TComPic * pCurPic , UInt refLayerIdc , TEncCfg * pCfg , Bool bSignalPPS );
  Void downScalePic( TComPicYuv* pcYuvSrc, TComPicYuv* pcYuvDest);
  Void downScaleComponent2x2( const Pel* pSrc, Pel* pDest, const Int iSrcStride, const Int iDestStride, const Int iSrcWidth, const Int iSrcHeight, const Int inputBitDepth, const Int outputBitDepth );
  inline Short  xClip( Short x , Int bitdepth );
  Void initDs(Int iWidth, Int iHeight, Int iType);
  Void filterImg(
    Pel           *src,
    Int           iSrcStride,
    Pel           *dst,
    Int           iDstStride,
    Int           height1,  
    Int           width1,  
    Int           shift,
    Int           plane);

  Int get_mem2DintWithPad(Int ***array2D, Int dim0, Int dim1, Int iPadY, Int iPadX);
  Void free_mem2DintWithPad(Int **array2D, Int iPadY, Int iPadX);
#endif
#endif //SVC_EXTENSION
};// END CLASS DEFINITION TEncGOP

// ====================================================================================================================
// Enumeration
// ====================================================================================================================
enum PROCESSING_STATE
{
  EXECUTE_INLOOPFILTER,
  ENCODE_SLICE
};

enum SCALING_LIST_PARAMETER
{
  SCALING_LIST_OFF,
  SCALING_LIST_DEFAULT,
  SCALING_LIST_FILE_READ
};

//! \}

#endif // __TENCGOP__

