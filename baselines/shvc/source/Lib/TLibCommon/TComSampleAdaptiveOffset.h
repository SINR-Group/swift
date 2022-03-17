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

/** \file     TComSampleAdaptiveOffset.h
    \brief    sample adaptive offset class (header)
*/

#ifndef __TCOMSAMPLEADAPTIVEOFFSET__
#define __TCOMSAMPLEADAPTIVEOFFSET__

#include "CommonDef.h"
#include "TComPic.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define MAX_SAO_TRUNCATED_BITDEPTH     10 
// ====================================================================================================================
// Class definition
// ====================================================================================================================
#if !SVC_EXTENSION
extern UInt g_saoMaxOffsetQVal[NUM_SAO_COMPONENTS]; 
#endif
#if SAO_SGN_FUNC
template <typename T> int sgn(T val) 
{
  return (T(0) < val) - (val < T(0));
}
#endif

class TComSampleAdaptiveOffset
{
public:
  TComSampleAdaptiveOffset();
  virtual ~TComSampleAdaptiveOffset();
  Void SAOProcess(TComPic* pDecPic);
#if AUXILIARY_PICTURES
  Void create( Int picWidth, Int picHeight, ChromaFormat chromaFormatIDC, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth );
#else
  Void create( Int picWidth, Int picHeight, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth );
#endif
  Void destroy();
  Void reconstructBlkSAOParams(TComPic* pic, SAOBlkParam* saoBlkParams);
  Void PCMLFDisableProcess (TComPic* pcPic);
#if SVC_EXTENSION
  UInt* getSaoMaxOffsetQVal() { return m_saoMaxOffsetQVal; } 
#endif
protected:
  Void offsetBlock(Int compIdx, Int typeIdx, Int* offset, Pel* srcBlk, Pel* resBlk, Int srcStride, Int resStride,  Int width, Int height
                  , Bool isLeftAvail, Bool isRightAvail, Bool isAboveAvail, Bool isBelowAvail, Bool isAboveLeftAvail, Bool isAboveRightAvail, Bool isBelowLeftAvail, Bool isBelowRightAvail);
  Pel* getPicBuf(TComPicYuv* pPicYuv, Int compIdx);
  Void invertQuantOffsets(Int compIdx, Int typeIdc, Int typeAuxInfo, Int* dstOffsets, Int* srcOffsets);
  Void reconstructBlkSAOParam(SAOBlkParam& recParam, std::vector<SAOBlkParam*>& mergeList);
  Int  getMergeList(TComPic* pic, Int ctu, SAOBlkParam* blkParams, std::vector<SAOBlkParam*>& mergeList);
  Void offsetCTU(Int ctu, TComPicYuv* srcYuv, TComPicYuv* resYuv, SAOBlkParam& saoblkParam, TComPic* pPic);
  Void xPCMRestoration(TComPic* pcPic);
  Void xPCMCURestoration ( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth );
  Void xPCMSampleRestoration (TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, TextType ttText);
protected:
  UInt m_offsetStepLog2[NUM_SAO_COMPONENTS]; //offset step  
  Int* m_offsetClip[NUM_SAO_COMPONENTS]; //clip table for fast operation
#if !SAO_SGN_FUNC
  Short* m_sign; //sign table for fast operation
#endif
  TComPicYuv*   m_tempPicYuv; //temporary buffer
  Int m_picWidth;
  Int m_picHeight;
  Int m_maxCUWidth;
  Int m_maxCUHeight;
  Int m_numCTUInWidth;
  Int m_numCTUInHeight;
  Int m_numCTUsPic;
  
  
  Int m_lineBufWidth;
  Char* m_signLineBuf1;
  Char* m_signLineBuf2;
private:
  Bool m_picSAOEnabled[NUM_SAO_COMPONENTS];
  Int*   m_offsetClipTable[NUM_SAO_COMPONENTS];
#if !SAO_SGN_FUNC
  Short* m_signTable;
#endif
#if SVC_EXTENSION
  UInt m_saoMaxOffsetQVal[NUM_SAO_COMPONENTS]; 
#endif
};

//! \}
#endif

