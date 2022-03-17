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

/** \file     TEncGOP.cpp
    \brief    GOP encoder class
*/

#include <list>
#include <algorithm>
#include <functional>

#include "TEncTop.h"
#include "TEncGOP.h"
#include "TEncAnalyze.h"
#include "libmd5/MD5.h"
#include "TLibCommon/SEI.h"
#include "TLibCommon/NAL.h"
#include "NALwrite.h"
#include <time.h>
#include <math.h>
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
#include <limits.h>
#endif

using namespace std;
//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================
Int getLSB(Int poc, Int maxLSB)
{
  if (poc >= 0)
  {
    return poc % maxLSB;
  }
  else
  {
    return (maxLSB - ((-poc) % maxLSB)) % maxLSB;
  }
}

TEncGOP::TEncGOP()
{
  m_iLastIDR            = 0;
  m_iGopSize            = 0;
  m_iNumPicCoded        = 0; //Niko
  m_bFirst              = true;
#if ALLOW_RECOVERY_POINT_AS_RAP
  m_iLastRecoveryPicPOC = 0;
#endif
  
  m_pcCfg               = NULL;
  m_pcSliceEncoder      = NULL;
  m_pcListPic           = NULL;
  
  m_pcEntropyCoder      = NULL;
  m_pcCavlcCoder        = NULL;
  m_pcSbacCoder         = NULL;
  m_pcBinCABAC          = NULL;
  
  m_bSeqFirst           = true;
  
  m_bRefreshPending     = 0;
  m_pocCRA            = 0;
#if POC_RESET_IDC_ENCODER
  m_pocCraWithoutReset = 0;
  m_associatedIrapPocBeforeReset = 0;
#endif
  m_numLongTermRefPicSPS = 0;
  ::memset(m_ltRefPicPocLsbSps, 0, sizeof(m_ltRefPicPocLsbSps));
  ::memset(m_ltRefPicUsedByCurrPicFlag, 0, sizeof(m_ltRefPicUsedByCurrPicFlag));
  m_cpbRemovalDelay   = 0;
  m_lastBPSEI         = 0;
  xResetNonNestedSEIPresentFlags();
  xResetNestedSEIPresentFlags();
#if FIX1172
  m_associatedIRAPType = NAL_UNIT_CODED_SLICE_IDR_N_LP;
  m_associatedIRAPPOC  = 0;
#endif
#if SVC_EXTENSION
  m_pcPredSearch        = NULL;
#if Q0048_CGS_3D_ASYMLUT
  m_temp = NULL;
  m_pColorMappedPic = NULL;
#endif
#if POC_RESET_IDC_ENCODER
  m_lastPocPeriodId = -1;
#endif
#endif //SVC_EXTENSION
  return;
}

TEncGOP::~TEncGOP()
{
#if Q0048_CGS_3D_ASYMLUT
  if(m_pColorMappedPic)
  {
    m_pColorMappedPic->destroy();
    delete m_pColorMappedPic;
    m_pColorMappedPic = NULL;                
  }
  if(m_temp)
  {
    free_mem2DintWithPad(m_temp, m_iTap>>1, 0);
    m_temp = NULL;
  }
#endif
}

/** Create list to contain pointers to LCU start addresses of slice.
 */
#if SVC_EXTENSION
Void  TEncGOP::create( UInt layerId )
{
  m_bLongtermTestPictureHasBeenCoded = 0;
  m_bLongtermTestPictureHasBeenCoded2 = 0;
  m_layerId = layerId;
}
#else
Void  TEncGOP::create()
{
  m_bLongtermTestPictureHasBeenCoded = 0;
  m_bLongtermTestPictureHasBeenCoded2 = 0;
}
#endif

Void  TEncGOP::destroy()
{
}

Void TEncGOP::init ( TEncTop* pcTEncTop )
{
  m_pcEncTop     = pcTEncTop;
  m_pcCfg                = pcTEncTop;
  m_pcSliceEncoder       = pcTEncTop->getSliceEncoder();
  m_pcListPic            = pcTEncTop->getListPic();  
  
  m_pcEntropyCoder       = pcTEncTop->getEntropyCoder();
  m_pcCavlcCoder         = pcTEncTop->getCavlcCoder();
  m_pcSbacCoder          = pcTEncTop->getSbacCoder();
  m_pcBinCABAC           = pcTEncTop->getBinCABAC();
  m_pcLoopFilter         = pcTEncTop->getLoopFilter();
  m_pcBitCounter         = pcTEncTop->getBitCounter();
  
  //--Adaptive Loop filter
  m_pcSAO                = pcTEncTop->getSAO();
  m_pcRateCtrl           = pcTEncTop->getRateCtrl();
  m_lastBPSEI          = 0;
  m_totalCoded         = 0;

#if SVC_EXTENSION
  m_ppcTEncTop           = pcTEncTop->getLayerEnc();
  m_pcPredSearch         = pcTEncTop->getPredSearch();                       ///< encoder search class
#if Q0048_CGS_3D_ASYMLUT
  if( pcTEncTop->getLayerId() )
  {
    m_Enc3DAsymLUTPicUpdate.create( m_pcCfg->getCGSMaxOctantDepth() , g_bitDepthYLayer[pcTEncTop->getLayerId()-1] , g_bitDepthCLayer[pcTEncTop->getLayerId()-1] , g_bitDepthYLayer[pcTEncTop->getLayerId()] , g_bitDepthCLayer[pcTEncTop->getLayerId()] , m_pcCfg->getCGSMaxYPartNumLog2() /*, m_pcCfg->getCGSPhaseAlignment()*/ );
    m_Enc3DAsymLUTPPS.create(   m_pcCfg->getCGSMaxOctantDepth() , g_bitDepthYLayer[pcTEncTop->getLayerId()-1] , g_bitDepthCLayer[pcTEncTop->getLayerId()-1] , g_bitDepthYLayer[pcTEncTop->getLayerId()] , g_bitDepthCLayer[pcTEncTop->getLayerId()] , m_pcCfg->getCGSMaxYPartNumLog2() /*, m_pcCfg->getCGSPhaseAlignment()*/ );
    if(!m_pColorMappedPic)
    {
      m_pColorMappedPic = new TComPicYuv;
      m_pColorMappedPic->create( m_ppcTEncTop[0]->getSourceWidth(), m_ppcTEncTop[0]->getSourceHeight(), m_ppcTEncTop[0]->getChromaFormatIDC(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, NULL );
    }
  }
#endif
#endif //SVC_EXTENSION
}

SEIActiveParameterSets* TEncGOP::xCreateSEIActiveParameterSets (TComSPS *sps)
{
  SEIActiveParameterSets *seiActiveParameterSets = new SEIActiveParameterSets(); 
  seiActiveParameterSets->activeVPSId = m_pcCfg->getVPS()->getVPSId(); 
  seiActiveParameterSets->m_selfContainedCvsFlag = false;
  seiActiveParameterSets->m_noParameterSetUpdateFlag = false;
#if !R0247_SEI_ACTIVE
  seiActiveParameterSets->numSpsIdsMinus1 = 0;
  seiActiveParameterSets->activeSeqParameterSetId.resize(seiActiveParameterSets->numSpsIdsMinus1 + 1); 
  seiActiveParameterSets->activeSeqParameterSetId[0] = sps->getSPSId();
#else
  seiActiveParameterSets->numSpsIdsMinus1 = m_pcCfg->getNumLayer()-1;
  seiActiveParameterSets->activeSeqParameterSetId.resize(seiActiveParameterSets->numSpsIdsMinus1 + 1); 
  seiActiveParameterSets->layerSpsIdx.resize(seiActiveParameterSets->numSpsIdsMinus1+ 1);  
  for (Int c=0; c <= seiActiveParameterSets->numSpsIdsMinus1; c++)
  {
     seiActiveParameterSets->activeSeqParameterSetId[c] = c;
  }
  for (Int c=1; c <= seiActiveParameterSets->numSpsIdsMinus1; c++)
  {
     seiActiveParameterSets->layerSpsIdx[c] = c;
  }
#endif
  return seiActiveParameterSets;
}

SEIFramePacking* TEncGOP::xCreateSEIFramePacking()
{
  SEIFramePacking *seiFramePacking = new SEIFramePacking();
  seiFramePacking->m_arrangementId = m_pcCfg->getFramePackingArrangementSEIId();
  seiFramePacking->m_arrangementCancelFlag = 0;
  seiFramePacking->m_arrangementType = m_pcCfg->getFramePackingArrangementSEIType();
  assert((seiFramePacking->m_arrangementType > 2) && (seiFramePacking->m_arrangementType < 6) );
  seiFramePacking->m_quincunxSamplingFlag = m_pcCfg->getFramePackingArrangementSEIQuincunx();
  seiFramePacking->m_contentInterpretationType = m_pcCfg->getFramePackingArrangementSEIInterpretation();
  seiFramePacking->m_spatialFlippingFlag = 0;
  seiFramePacking->m_frame0FlippedFlag = 0;
  seiFramePacking->m_fieldViewsFlag = (seiFramePacking->m_arrangementType == 2);
  seiFramePacking->m_currentFrameIsFrame0Flag = ((seiFramePacking->m_arrangementType == 5) && m_iNumPicCoded&1);
  seiFramePacking->m_frame0SelfContainedFlag = 0;
  seiFramePacking->m_frame1SelfContainedFlag = 0;
  seiFramePacking->m_frame0GridPositionX = 0;
  seiFramePacking->m_frame0GridPositionY = 0;
  seiFramePacking->m_frame1GridPositionX = 0;
  seiFramePacking->m_frame1GridPositionY = 0;
  seiFramePacking->m_arrangementReservedByte = 0;
  seiFramePacking->m_arrangementPersistenceFlag = true;
  seiFramePacking->m_upsampledAspectRatio = 0;
  return seiFramePacking;
}

SEIDisplayOrientation* TEncGOP::xCreateSEIDisplayOrientation()
{
  SEIDisplayOrientation *seiDisplayOrientation = new SEIDisplayOrientation();
  seiDisplayOrientation->cancelFlag = false;
  seiDisplayOrientation->horFlip = false;
  seiDisplayOrientation->verFlip = false;
  seiDisplayOrientation->anticlockwiseRotation = m_pcCfg->getDisplayOrientationSEIAngle();
  return seiDisplayOrientation;
}

SEIToneMappingInfo*  TEncGOP::xCreateSEIToneMappingInfo()
{
  SEIToneMappingInfo *seiToneMappingInfo = new SEIToneMappingInfo();
  seiToneMappingInfo->m_toneMapId = m_pcCfg->getTMISEIToneMapId();
  seiToneMappingInfo->m_toneMapCancelFlag = m_pcCfg->getTMISEIToneMapCancelFlag();
  seiToneMappingInfo->m_toneMapPersistenceFlag = m_pcCfg->getTMISEIToneMapPersistenceFlag();

  seiToneMappingInfo->m_codedDataBitDepth = m_pcCfg->getTMISEICodedDataBitDepth();
  assert(seiToneMappingInfo->m_codedDataBitDepth >= 8 && seiToneMappingInfo->m_codedDataBitDepth <= 14);
  seiToneMappingInfo->m_targetBitDepth = m_pcCfg->getTMISEITargetBitDepth();
  assert( seiToneMappingInfo->m_targetBitDepth >= 1 && seiToneMappingInfo->m_targetBitDepth <= 17 );
  seiToneMappingInfo->m_modelId = m_pcCfg->getTMISEIModelID();
  assert(seiToneMappingInfo->m_modelId >=0 &&seiToneMappingInfo->m_modelId<=4);

  switch( seiToneMappingInfo->m_modelId)
  {
  case 0:
    {
      seiToneMappingInfo->m_minValue = m_pcCfg->getTMISEIMinValue();
      seiToneMappingInfo->m_maxValue = m_pcCfg->getTMISEIMaxValue();
      break;
    }
  case 1:
    {
      seiToneMappingInfo->m_sigmoidMidpoint = m_pcCfg->getTMISEISigmoidMidpoint();
      seiToneMappingInfo->m_sigmoidWidth = m_pcCfg->getTMISEISigmoidWidth();
      break;
    }
  case 2:
    {
      UInt num = 1u<<(seiToneMappingInfo->m_targetBitDepth);
      seiToneMappingInfo->m_startOfCodedInterval.resize(num);
      Int* ptmp = m_pcCfg->getTMISEIStartOfCodedInterva();
      if(ptmp)
      {
        for(int i=0; i<num;i++)
        {
          seiToneMappingInfo->m_startOfCodedInterval[i] = ptmp[i];
        }
      }
      break;
    }
  case 3:
    {
      seiToneMappingInfo->m_numPivots = m_pcCfg->getTMISEINumPivots();
      seiToneMappingInfo->m_codedPivotValue.resize(seiToneMappingInfo->m_numPivots);
      seiToneMappingInfo->m_targetPivotValue.resize(seiToneMappingInfo->m_numPivots);
      Int* ptmpcoded = m_pcCfg->getTMISEICodedPivotValue();
      Int* ptmptarget = m_pcCfg->getTMISEITargetPivotValue();
      if(ptmpcoded&&ptmptarget)
      {
        for(int i=0; i<(seiToneMappingInfo->m_numPivots);i++)
        {
          seiToneMappingInfo->m_codedPivotValue[i]=ptmpcoded[i];
          seiToneMappingInfo->m_targetPivotValue[i]=ptmptarget[i];
         }
       }
       break;
     }
  case 4:
     {
       seiToneMappingInfo->m_cameraIsoSpeedIdc = m_pcCfg->getTMISEICameraIsoSpeedIdc();
       seiToneMappingInfo->m_cameraIsoSpeedValue = m_pcCfg->getTMISEICameraIsoSpeedValue();
       assert( seiToneMappingInfo->m_cameraIsoSpeedValue !=0 );
       seiToneMappingInfo->m_exposureIndexIdc = m_pcCfg->getTMISEIExposurIndexIdc();
       seiToneMappingInfo->m_exposureIndexValue = m_pcCfg->getTMISEIExposurIndexValue();
       assert( seiToneMappingInfo->m_exposureIndexValue !=0 );
       seiToneMappingInfo->m_exposureCompensationValueSignFlag = m_pcCfg->getTMISEIExposureCompensationValueSignFlag();
       seiToneMappingInfo->m_exposureCompensationValueNumerator = m_pcCfg->getTMISEIExposureCompensationValueNumerator();
       seiToneMappingInfo->m_exposureCompensationValueDenomIdc = m_pcCfg->getTMISEIExposureCompensationValueDenomIdc();
       seiToneMappingInfo->m_refScreenLuminanceWhite = m_pcCfg->getTMISEIRefScreenLuminanceWhite();
       seiToneMappingInfo->m_extendedRangeWhiteLevel = m_pcCfg->getTMISEIExtendedRangeWhiteLevel();
       assert( seiToneMappingInfo->m_extendedRangeWhiteLevel >= 100 );
       seiToneMappingInfo->m_nominalBlackLevelLumaCodeValue = m_pcCfg->getTMISEINominalBlackLevelLumaCodeValue();
       seiToneMappingInfo->m_nominalWhiteLevelLumaCodeValue = m_pcCfg->getTMISEINominalWhiteLevelLumaCodeValue();
       assert( seiToneMappingInfo->m_nominalWhiteLevelLumaCodeValue > seiToneMappingInfo->m_nominalBlackLevelLumaCodeValue );
       seiToneMappingInfo->m_extendedWhiteLevelLumaCodeValue = m_pcCfg->getTMISEIExtendedWhiteLevelLumaCodeValue();
       assert( seiToneMappingInfo->m_extendedWhiteLevelLumaCodeValue >= seiToneMappingInfo->m_nominalWhiteLevelLumaCodeValue );
       break;
    }
  default:
    {
      assert(!"Undefined SEIToneMapModelId");
      break;
    }
  }
  return seiToneMappingInfo;
}

#if P0050_KNEE_FUNCTION_SEI
SEIKneeFunctionInfo* TEncGOP::xCreateSEIKneeFunctionInfo()
{
  SEIKneeFunctionInfo *seiKneeFunctionInfo = new SEIKneeFunctionInfo();
  seiKneeFunctionInfo->m_kneeId = m_pcCfg->getKneeSEIId();
  seiKneeFunctionInfo->m_kneeCancelFlag = m_pcCfg->getKneeSEICancelFlag();
  if ( !seiKneeFunctionInfo->m_kneeCancelFlag )
  {
    seiKneeFunctionInfo->m_kneePersistenceFlag = m_pcCfg->getKneeSEIPersistenceFlag();
    seiKneeFunctionInfo->m_kneeMappingFlag = m_pcCfg->getKneeSEIMappingFlag();
    seiKneeFunctionInfo->m_kneeInputDrange = m_pcCfg->getKneeSEIInputDrange();
    seiKneeFunctionInfo->m_kneeInputDispLuminance = m_pcCfg->getKneeSEIInputDispLuminance();
    seiKneeFunctionInfo->m_kneeOutputDrange = m_pcCfg->getKneeSEIOutputDrange();
    seiKneeFunctionInfo->m_kneeOutputDispLuminance = m_pcCfg->getKneeSEIOutputDispLuminance();

    seiKneeFunctionInfo->m_kneeNumKneePointsMinus1 = m_pcCfg->getKneeSEINumKneePointsMinus1();
    Int* piInputKneePoint  = m_pcCfg->getKneeSEIInputKneePoint();
    Int* piOutputKneePoint = m_pcCfg->getKneeSEIOutputKneePoint();
    if(piInputKneePoint&&piOutputKneePoint)
    {
      seiKneeFunctionInfo->m_kneeInputKneePoint.resize(seiKneeFunctionInfo->m_kneeNumKneePointsMinus1+1);
      seiKneeFunctionInfo->m_kneeOutputKneePoint.resize(seiKneeFunctionInfo->m_kneeNumKneePointsMinus1+1);
      for(Int i=0; i<=seiKneeFunctionInfo->m_kneeNumKneePointsMinus1; i++)
      {
        seiKneeFunctionInfo->m_kneeInputKneePoint[i] = piInputKneePoint[i];
        seiKneeFunctionInfo->m_kneeOutputKneePoint[i] = piOutputKneePoint[i];
       }
    }
  }
  return seiKneeFunctionInfo;
}
#endif

#if Q0074_COLOUR_REMAPPING_SEI
SEIColourRemappingInfo*  TEncGOP::xCreateSEIColourRemappingInfo()
{
  SEIColourRemappingInfo *seiColourRemappingInfo = new SEIColourRemappingInfo();
  seiColourRemappingInfo->m_colourRemapId         = m_pcCfg->getCRISEIId();
  seiColourRemappingInfo->m_colourRemapCancelFlag = m_pcCfg->getCRISEICancelFlag();
  if( !seiColourRemappingInfo->m_colourRemapCancelFlag )
  {
    seiColourRemappingInfo->m_colourRemapPersistenceFlag            = m_pcCfg->getCRISEIPersistenceFlag();
    seiColourRemappingInfo->m_colourRemapVideoSignalInfoPresentFlag = m_pcCfg->getCRISEIVideoSignalInfoPresentFlag();
    if( seiColourRemappingInfo->m_colourRemapVideoSignalInfoPresentFlag )
    {
      seiColourRemappingInfo->m_colourRemapFullRangeFlag           = m_pcCfg->getCRISEIFullRangeFlag();
      seiColourRemappingInfo->m_colourRemapPrimaries               = m_pcCfg->getCRISEIPrimaries();
      seiColourRemappingInfo->m_colourRemapTransferFunction        = m_pcCfg->getCRISEITransferFunction();
      seiColourRemappingInfo->m_colourRemapMatrixCoefficients      = m_pcCfg->getCRISEIMatrixCoefficients();
    }
    seiColourRemappingInfo->m_colourRemapInputBitDepth             = m_pcCfg->getCRISEIInputBitDepth();
    seiColourRemappingInfo->m_colourRemapBitDepth                  = m_pcCfg->getCRISEIBitDepth();
    for( Int c=0 ; c<3 ; c++ )
    {
      seiColourRemappingInfo->m_preLutNumValMinus1[c] = m_pcCfg->getCRISEIPreLutNumValMinus1(c);
      if( seiColourRemappingInfo->m_preLutNumValMinus1[c]>0 )
      {
        seiColourRemappingInfo->m_preLutCodedValue[c].resize(seiColourRemappingInfo->m_preLutNumValMinus1[c]+1);
        seiColourRemappingInfo->m_preLutTargetValue[c].resize(seiColourRemappingInfo->m_preLutNumValMinus1[c]+1);
        for( Int i=0 ; i<=seiColourRemappingInfo->m_preLutNumValMinus1[c] ; i++)
        {
          seiColourRemappingInfo->m_preLutCodedValue[c][i]  = (m_pcCfg->getCRISEIPreLutCodedValue(c))[i];
          seiColourRemappingInfo->m_preLutTargetValue[c][i] = (m_pcCfg->getCRISEIPreLutTargetValue(c))[i];
        }
      }
    }
    seiColourRemappingInfo->m_colourRemapMatrixPresentFlag = m_pcCfg->getCRISEIMatrixPresentFlag();
    if( seiColourRemappingInfo->m_colourRemapMatrixPresentFlag )
    {
      seiColourRemappingInfo->m_log2MatrixDenom = m_pcCfg->getCRISEILog2MatrixDenom();
      for( Int c=0 ; c<3 ; c++ )
        for( Int i=0 ; i<3 ; i++ )
          seiColourRemappingInfo->m_colourRemapCoeffs[c][i] = (m_pcCfg->getCRISEICoeffs(c))[i];
    }
    for( Int c=0 ; c<3 ; c++ )
    {
      seiColourRemappingInfo->m_postLutNumValMinus1[c] = m_pcCfg->getCRISEIPostLutNumValMinus1(c);
      if( seiColourRemappingInfo->m_postLutNumValMinus1[c]>0 )
      {
        seiColourRemappingInfo->m_postLutCodedValue[c].resize(seiColourRemappingInfo->m_postLutNumValMinus1[c]+1);
        seiColourRemappingInfo->m_postLutTargetValue[c].resize(seiColourRemappingInfo->m_postLutNumValMinus1[c]+1);
        for( Int i=0 ; i<=seiColourRemappingInfo->m_postLutNumValMinus1[c] ; i++)
        {
          seiColourRemappingInfo->m_postLutCodedValue[c][i]  = (m_pcCfg->getCRISEIPostLutCodedValue(c))[i];
          seiColourRemappingInfo->m_postLutTargetValue[c][i] = (m_pcCfg->getCRISEIPostLutTargetValue(c))[i];
        }
      }
    }
  }
  return seiColourRemappingInfo;
}
#endif

Void TEncGOP::xCreateLeadingSEIMessages (/*SEIMessages seiMessages,*/ AccessUnit &accessUnit, TComSPS *sps)
{
  OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);

  if(m_pcCfg->getActiveParameterSetsSEIEnabled()
#if R0247_SEI_ACTIVE
    && m_layerId == 0
#endif
    )
  {
    SEIActiveParameterSets *sei = xCreateSEIActiveParameterSets (sps);

    //nalu = NALUnit(NAL_UNIT_SEI); 
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if O0164_MULTI_LAYER_HRD
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, m_pcEncTop->getVPS(), sps); 
#else
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps); 
#endif
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
    m_activeParameterSetSEIPresentInAU = true;
  }

  if(m_pcCfg->getFramePackingArrangementSEIEnabled())
  {
    SEIFramePacking *sei = xCreateSEIFramePacking ();

    nalu = NALUnit(NAL_UNIT_PREFIX_SEI);
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if O0164_MULTI_LAYER_HRD
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, m_pcEncTop->getVPS(), sps);
#else
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps);
#endif
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
  if (m_pcCfg->getDisplayOrientationSEIAngle())
  {
    SEIDisplayOrientation *sei = xCreateSEIDisplayOrientation();

    nalu = NALUnit(NAL_UNIT_PREFIX_SEI); 
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if O0164_MULTI_LAYER_HRD
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, m_pcEncTop->getVPS(), sps); 
#else
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps); 
#endif
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
  if(m_pcCfg->getToneMappingInfoSEIEnabled())
  {
    SEIToneMappingInfo *sei = xCreateSEIToneMappingInfo ();
      
    nalu = NALUnit(NAL_UNIT_PREFIX_SEI); 
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if O0164_MULTI_LAYER_HRD
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, m_pcEncTop->getVPS(), sps); 
#else
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps); 
#endif
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
#if P0050_KNEE_FUNCTION_SEI
  if(m_pcCfg->getKneeSEIEnabled())
  {
    SEIKneeFunctionInfo *sei = xCreateSEIKneeFunctionInfo();

    nalu = NALUnit(NAL_UNIT_PREFIX_SEI);
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if O0164_MULTI_LAYER_HRD
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, m_pcEncTop->getVPS(), sps);
#else
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps);
#endif
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  if(strlen(m_pcCfg->getCRISEIFile()))
  {
    SEIColourRemappingInfo *sei = xCreateSEIColourRemappingInfo ();
      
#if SVC_EXTENSION
    nalu = NALUnit(NAL_UNIT_PREFIX_SEI, 0, sps->getLayerId());  // SEI-CRI is applied per layer
#else
    nalu = NALUnit(NAL_UNIT_PREFIX_SEI);
#endif
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if O0164_MULTI_LAYER_HRD
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, m_pcEncTop->getVPS(), sps); 
#else
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps); 
#endif
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
#endif

#if SVC_EXTENSION
#if LAYERS_NOT_PRESENT_SEI
  if(m_pcCfg->getLayersNotPresentSEIEnabled())
  {
    SEILayersNotPresent *sei = xCreateSEILayersNotPresent ();
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if O0164_MULTI_LAYER_HRD
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, m_pcEncTop->getVPS(), sps); 
#else
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps); 
#endif
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
#endif

#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
  if(m_pcCfg->getInterLayerConstrainedTileSetsSEIEnabled())
  {
    SEIInterLayerConstrainedTileSets *sei = xCreateSEIInterLayerConstrainedTileSets ();

    nalu = NALUnit(NAL_UNIT_PREFIX_SEI, 0, m_pcCfg->getNumLayer()-1); // For highest layer
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if O0164_MULTI_LAYER_HRD
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, m_pcEncTop->getVPS(), sps); 
#else
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps); 
#endif
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
#endif
#endif //SVC_EXTENSION
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================
#if SVC_EXTENSION
Void TEncGOP::compressGOP( Int iPicIdInGOP, Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsInGOP, Bool isField, Bool isTff)
#else
Void TEncGOP::compressGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsInGOP, Bool isField, Bool isTff)
#endif
{
  TComPic*        pcPic;
  TComPicYuv*     pcPicYuvRecOut;
  TComSlice*      pcSlice;
  TComOutputBitstream  *pcBitstreamRedirect;
  pcBitstreamRedirect = new TComOutputBitstream;
  AccessUnit::iterator  itLocationToPushSliceHeaderNALU; // used to store location where NALU containing slice header is to be inserted
  UInt                  uiOneBitstreamPerSliceLength = 0;
  TEncSbac* pcSbacCoders = NULL;
  TComOutputBitstream* pcSubstreamsOut = NULL;

  xInitGOP( iPOCLast, iNumPicRcvd, rcListPic, rcListPicYuvRecOut, isField );

  m_iNumPicCoded = 0;
  SEIPictureTiming pictureTimingSEI;
  Bool writeSOP = m_pcCfg->getSOPDescriptionSEIEnabled();
  // Initialize Scalable Nesting SEI with single layer values
  SEIScalableNesting scalableNestingSEI;
  scalableNestingSEI.m_bitStreamSubsetFlag           = 1;      // If the nested SEI messages are picture buffereing SEI mesages, picure timing SEI messages or sub-picture timing SEI messages, bitstream_subset_flag shall be equal to 1
  scalableNestingSEI.m_nestingOpFlag                 = 0;
  scalableNestingSEI.m_nestingNumOpsMinus1           = 0;      //nesting_num_ops_minus1
  scalableNestingSEI.m_allLayersFlag                 = 0;
  scalableNestingSEI.m_nestingNoOpMaxTemporalIdPlus1 = 6 + 1;  //nesting_no_op_max_temporal_id_plus1
  scalableNestingSEI.m_nestingNumLayersMinus1        = 1 - 1;  //nesting_num_layers_minus1
  scalableNestingSEI.m_nestingLayerId[0]             = 0;
  scalableNestingSEI.m_callerOwnsSEIs                = true;
  Int picSptDpbOutputDuDelay = 0;
  UInt *accumBitsDU = NULL;
  UInt *accumNalsDU = NULL;
  SEIDecodingUnitInfo decodingUnitInfoSEI;
#if EFFICIENT_FIELD_IRAP
  Int IRAPGOPid = -1;
  Bool IRAPtoReorder = false;
  Bool swapIRAPForward = false;
  if(isField)
  {
    Int pocCurr;
#if SVC_EXTENSION
    for ( Int iGOPid=iPicIdInGOP; iGOPid < iPicIdInGOP+1; iGOPid++ )
#else
    for ( Int iGOPid=0; iGOPid < m_iGopSize; iGOPid++ )
#endif    
    {
      // determine actual POC
      if(iPOCLast == 0) //case first frame or first top field
      {
        pocCurr=0;
      }
      else if(iPOCLast == 1 && isField) //case first bottom field, just like the first frame, the poc computation is not right anymore, we set the right value
      {
        pocCurr = 1;
      }
      else
      {
        pocCurr = iPOCLast - iNumPicRcvd + m_pcCfg->getGOPEntry(iGOPid).m_POC - isField;
      }

      // check if POC corresponds to IRAP
      NalUnitType tmpUnitType = getNalUnitType(pocCurr, m_iLastIDR, isField);
      if(tmpUnitType >= NAL_UNIT_CODED_SLICE_BLA_W_LP && tmpUnitType <= NAL_UNIT_CODED_SLICE_CRA) // if picture is an IRAP
      {
        if(pocCurr%2 == 0 && iGOPid < m_iGopSize-1 && m_pcCfg->getGOPEntry(iGOPid).m_POC == m_pcCfg->getGOPEntry(iGOPid+1).m_POC-1)
        { // if top field and following picture in enc order is associated bottom field
          IRAPGOPid = iGOPid;
          IRAPtoReorder = true;
          swapIRAPForward = true; 
          break;
        }
        if(pocCurr%2 != 0 && iGOPid > 0 && m_pcCfg->getGOPEntry(iGOPid).m_POC == m_pcCfg->getGOPEntry(iGOPid-1).m_POC+1)
        {
          // if picture is an IRAP remember to process it first
          IRAPGOPid = iGOPid;
          IRAPtoReorder = true;
          swapIRAPForward = false; 
          break;
        }
      }
    }
  }
#endif
#if SVC_EXTENSION
  for ( Int iGOPid=iPicIdInGOP; iGOPid < iPicIdInGOP+1; iGOPid++ )
#else
  for ( Int iGOPid=0; iGOPid < m_iGopSize; iGOPid++ )
#endif
  {
#if EFFICIENT_FIELD_IRAP
    if(IRAPtoReorder)
    {
      if(swapIRAPForward)
      {
        if(iGOPid == IRAPGOPid)
        {
          iGOPid = IRAPGOPid +1;
        }
        else if(iGOPid == IRAPGOPid +1)
        {
          iGOPid = IRAPGOPid;
        }
      }
      else
      {
        if(iGOPid == IRAPGOPid -1)
        {
          iGOPid = IRAPGOPid;
        }
        else if(iGOPid == IRAPGOPid)
        {
          iGOPid = IRAPGOPid -1;
        }
      }
    }
#endif
    UInt uiColDir = 1;
    //-- For time output for each slice
    long iBeforeTime = clock();

    //select uiColDir
    Int iCloseLeft=1, iCloseRight=-1;
    for(Int i = 0; i<m_pcCfg->getGOPEntry(iGOPid).m_numRefPics; i++) 
    {
      Int iRef = m_pcCfg->getGOPEntry(iGOPid).m_referencePics[i];
      if(iRef>0&&(iRef<iCloseRight||iCloseRight==-1))
      {
        iCloseRight=iRef;
      }
      else if(iRef<0&&(iRef>iCloseLeft||iCloseLeft==1))
      {
        iCloseLeft=iRef;
      }
    }
    if(iCloseRight>-1)
    {
      iCloseRight=iCloseRight+m_pcCfg->getGOPEntry(iGOPid).m_POC-1;
    }
    if(iCloseLeft<1) 
    {
      iCloseLeft=iCloseLeft+m_pcCfg->getGOPEntry(iGOPid).m_POC-1;
      while(iCloseLeft<0)
      {
        iCloseLeft+=m_iGopSize;
      }
    }
    Int iLeftQP=0, iRightQP=0;
    for(Int i=0; i<m_iGopSize; i++)
    {
      if(m_pcCfg->getGOPEntry(i).m_POC==(iCloseLeft%m_iGopSize)+1)
      {
        iLeftQP= m_pcCfg->getGOPEntry(i).m_QPOffset;
      }
      if (m_pcCfg->getGOPEntry(i).m_POC==(iCloseRight%m_iGopSize)+1)
      {
        iRightQP=m_pcCfg->getGOPEntry(i).m_QPOffset;
      }
    }
    if(iCloseRight>-1&&iRightQP<iLeftQP)
    {
      uiColDir=0;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////// Initial to start encoding
    Int iTimeOffset;
    Int pocCurr;
    
    if(iPOCLast == 0) //case first frame or first top field
    {
      pocCurr=0;
      iTimeOffset = 1;
    }
    else if(iPOCLast == 1 && isField) //case first bottom field, just like the first frame, the poc computation is not right anymore, we set the right value
    {
      pocCurr = 1;
      iTimeOffset = 1;
    }
    else
    {
      pocCurr = iPOCLast - iNumPicRcvd + m_pcCfg->getGOPEntry(iGOPid).m_POC - isField;
      iTimeOffset = m_pcCfg->getGOPEntry(iGOPid).m_POC;
    }

    if(pocCurr>=m_pcCfg->getFramesToBeEncoded())
    {
#if EFFICIENT_FIELD_IRAP
      if(IRAPtoReorder)
      {
        if(swapIRAPForward)
        {
          if(iGOPid == IRAPGOPid)
          {
            iGOPid = IRAPGOPid +1;
            IRAPtoReorder = false;
          }
          else if(iGOPid == IRAPGOPid +1)
          {
            iGOPid --;
          }
        }
        else
        {
          if(iGOPid == IRAPGOPid)
          {
            iGOPid = IRAPGOPid -1;
          }
          else if(iGOPid == IRAPGOPid -1)
          {
            iGOPid = IRAPGOPid;
            IRAPtoReorder = false;
          }
        }
      }
#endif
      continue;
    }

#if M0040_ADAPTIVE_RESOLUTION_CHANGE
    if (m_pcEncTop->getAdaptiveResolutionChange() > 0 && ((m_layerId == 1 && pocCurr < m_pcEncTop->getAdaptiveResolutionChange()) ||
                                                          (m_layerId == 0 && pocCurr > m_pcEncTop->getAdaptiveResolutionChange())) )
    {
      continue;
    }
#endif

    if( getNalUnitType(pocCurr, m_iLastIDR, isField) == NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType(pocCurr, m_iLastIDR, isField) == NAL_UNIT_CODED_SLICE_IDR_N_LP )
    {
      m_iLastIDR = pocCurr;
    }        
    // start a new access unit: create an entry in the list of output access units
    accessUnitsInGOP.push_back(AccessUnit());
    AccessUnit& accessUnit = accessUnitsInGOP.back();
    xGetBuffer( rcListPic, rcListPicYuvRecOut, iNumPicRcvd, iTimeOffset, pcPic, pcPicYuvRecOut, pocCurr, isField);

    //  Slice data initialization
    pcPic->clearSliceBuffer();
    assert(pcPic->getNumAllocatedSlice() == 1);
    m_pcSliceEncoder->setSliceIdx(0);
    pcPic->setCurrSliceIdx(0);
#if SVC_EXTENSION
    pcPic->setLayerId( m_layerId );
    m_pcSliceEncoder->initEncSlice ( pcPic, iPOCLast, pocCurr, iNumPicRcvd, iGOPid, pcSlice, m_pcEncTop->getSPS(), m_pcEncTop->getPPS(), m_pcEncTop->getVPS(), isField );
#else
    m_pcSliceEncoder->initEncSlice ( pcPic, iPOCLast, pocCurr, iNumPicRcvd, iGOPid, pcSlice, m_pcEncTop->getSPS(), m_pcEncTop->getPPS(), isField );
#endif

    //Set Frame/Field coding
    pcSlice->getPic()->setField(isField);

#if SVC_EXTENSION
#if POC_RESET_FLAG
    if( !pcSlice->getPocResetFlag() ) // For picture that are not reset, we should adjust the value of POC calculated from the configuration files.
    {
      // Subtract POC adjustment value until now.
      pcSlice->setPOC( pcSlice->getPOC() - m_pcEncTop->getPocAdjustmentValue() );
    }
    else
    {
      // Check if this is the first slice in the picture
      // In the encoder, the POC values are copied along with copySliceInfo, so we only need
      // to do this for the first slice.
      Int pocAdjustValue = pcSlice->getPOC() - m_pcEncTop->getPocAdjustmentValue();
      if( pcSlice->getSliceIdx() == 0 )
      {
        TComList<TComPic*>::iterator  iterPic = rcListPic.begin();  

        // Iterate through all picture in DPB
        while( iterPic != rcListPic.end() )
        {              
          TComPic *dpbPic = *iterPic;
          if( dpbPic->getPOC() == pocCurr )
          {
            if( dpbPic->getReconMark() )
            {
              assert( !( dpbPic->getSlice(0)->isReferenced() ) && !( dpbPic->getOutputMark() ) );
            }
          }
          // Check if the picture pointed to by iterPic is either used for reference or
          // needed for output, are in the same layer, and not the current picture.
          if( /* ( ( dpbPic->getSlice(0)->isReferenced() ) || ( dpbPic->getOutputMark() ) )
              && */ ( dpbPic->getLayerId() == pcSlice->getLayerId() )
              && ( dpbPic->getReconMark() ) 
            )
          {
            for(Int i = dpbPic->getNumAllocatedSlice()-1; i >= 0; i--)
            {
              TComSlice *slice = dpbPic->getSlice(i);
              TComReferencePictureSet *rps = slice->getRPS();
              slice->setPOC( dpbPic->getSlice(i)->getPOC() - pocAdjustValue );

              // Also adjust the POC value stored in the RPS of each such slice
              for(Int j = rps->getNumberOfPictures(); j >= 0; j--)
              {
                rps->setPOC( j, rps->getPOC(j) - pocAdjustValue );
              }
              // Also adjust the value of refPOC
              for(Int k = 0; k < 2; k++)  // For List 0 and List 1
              {
                RefPicList list = (k == 1) ? REF_PIC_LIST_1 : REF_PIC_LIST_0;
                for(Int j = 0; j < slice->getNumRefIdx(list); j++)
                {
                  slice->setRefPOC( slice->getRefPOC(list, j) - pocAdjustValue, list, j);
                }
              }
            }
          }
          iterPic++;
        }
        m_pcEncTop->setPocAdjustmentValue( m_pcEncTop->getPocAdjustmentValue() + pocAdjustValue );
      }
      pcSlice->setPocValueBeforeReset( pcSlice->getPOC() - m_pcEncTop->getPocAdjustmentValue() + pocAdjustValue );
      pcSlice->setPOC( 0 );
    }
#endif
#if POC_RESET_IDC_ENCODER
    pcSlice->setPocValueBeforeReset( pocCurr );
    // Check if the current picture is to be assigned as a reset picture
    determinePocResetIdc(pocCurr, pcSlice);

#if P0297_VPS_POC_LSB_ALIGNED_FLAG
    Bool pocResettingFlag = false;

    if (pcSlice->getPocResetIdc() != 0)
    {
      if (pcSlice->getVPS()->getVpsPocLsbAlignedFlag())
      {
        pocResettingFlag = true;
      }
      else if (m_pcEncTop->getPocDecrementedInDPBFlag())
      {
        pocResettingFlag = false;
      }
      else
      {
        pocResettingFlag = true;
      }
    }
#endif

    // If reset, do the following steps:
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
    if( pocResettingFlag )
#else
    if( pcSlice->getPocResetIdc() )
#endif
    {
      updatePocValuesOfPics(pocCurr, pcSlice);
    }
    else
    {
      // Check the base layer picture is IDR. If so, just set current POC equal to 0 (alignment of POC)
      if( ( m_ppcTEncTop[0]->getGOPEncoder()->getIntraRefreshType() == 2) && ( pocCurr % m_ppcTEncTop[0]->getGOPEncoder()->getIntraRefreshInterval() == 0 ) )        
      {
        m_pcEncTop->setPocAdjustmentValue( pocCurr );
      }
      // else
      {
        // Just subtract POC by the current cumulative POC delta
        pcSlice->setPOC( pocCurr - m_pcEncTop->getPocAdjustmentValue() );
      }

      Int maxPocLsb = 1 << pcSlice->getSPS()->getBitsForPOC();
      pcSlice->setPocMsbVal( pcSlice->getPOC() - ( pcSlice->getPOC() & (maxPocLsb-1) ) );
    }
    // Update the POC of current picture, pictures in the DPB, including references inside the reference pictures

#endif

#if O0149_CROSS_LAYER_BLA_FLAG
    if( m_layerId == 0 && (getNalUnitType(pocCurr, m_iLastIDR, isField) == NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType(pocCurr, m_iLastIDR, isField) == NAL_UNIT_CODED_SLICE_IDR_N_LP) )
    {
      pcSlice->setCrossLayerBLAFlag(m_pcEncTop->getCrossLayerBLAFlag());
    }
    else
    {
      pcSlice->setCrossLayerBLAFlag(false);
    }
#endif
#if NO_CLRAS_OUTPUT_FLAG
    if (m_layerId == 0 &&
        (pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA))
    {
      if (m_bFirst)
      {
        m_pcEncTop->setNoClrasOutputFlag(true);
      }
      else if (pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
            || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP)
      {
        m_pcEncTop->setNoClrasOutputFlag(true);
      }
#if O0149_CROSS_LAYER_BLA_FLAG
      else if ((pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP) &&
               pcSlice->getCrossLayerBLAFlag())
      {
        m_pcEncTop->setNoClrasOutputFlag(true);
      }
#endif
      else
      {
        m_pcEncTop->setNoClrasOutputFlag(false);
      }
      if (m_pcEncTop->getNoClrasOutputFlag())
      {
        for (UInt i = 0; i < m_pcCfg->getNumLayer(); i++)
        {
          m_ppcTEncTop[i]->setLayerInitializedFlag(false);
          m_ppcTEncTop[i]->setFirstPicInLayerDecodedFlag(false);
        }
      }
    }
#endif
#if M0040_ADAPTIVE_RESOLUTION_CHANGE
    if (m_pcEncTop->getAdaptiveResolutionChange() > 0 && m_layerId == 1 && pocCurr > m_pcEncTop->getAdaptiveResolutionChange())
    {
      pcSlice->setActiveNumILRRefIdx(0);
      pcSlice->setInterLayerPredEnabledFlag(false);
      pcSlice->setMFMEnabledFlag(false);
    }
#endif
#endif //SVC_EXTENSION

    pcSlice->setLastIDR(m_iLastIDR);
    pcSlice->setSliceIdx(0);
    //set default slice level flag to the same as SPS level flag
    pcSlice->setLFCrossSliceBoundaryFlag(  pcSlice->getPPS()->getLoopFilterAcrossSlicesEnabledFlag()  );
    pcSlice->setScalingList ( m_pcEncTop->getScalingList()  );
    if(m_pcEncTop->getUseScalingListId() == SCALING_LIST_OFF)
    {
      m_pcEncTop->getTrQuant()->setFlatScalingList();
      m_pcEncTop->getTrQuant()->setUseScalingList(false);
      m_pcEncTop->getSPS()->setScalingListPresentFlag(false);
      m_pcEncTop->getPPS()->setScalingListPresentFlag(false);
    }
    else if(m_pcEncTop->getUseScalingListId() == SCALING_LIST_DEFAULT)
    {
#if SCALINGLIST_INFERRING
      // inferring of the scaling list can be moved to the config file
      UInt refLayerId = 0;
#if VPS_AVC_BL_FLAG_REMOVAL
      if( m_layerId > 0 && !m_pcEncTop->getVPS()->getNonHEVCBaseLayerFlag() && m_pcEncTop->getVPS()->getRecursiveRefLayerFlag( m_layerId, refLayerId ) )
#else
      if( m_layerId > 0 && !m_pcEncTop->getVPS()->getAvcBaseLayerFlag() && m_pcEncTop->getVPS()->getRecursiveRefLayerFlag( m_layerId, refLayerId ) )
#endif
      {
        m_pcEncTop->getSPS()->setInferScalingListFlag( true );
        m_pcEncTop->getSPS()->setScalingListRefLayerId( refLayerId );
        m_pcEncTop->getSPS()->setScalingListPresentFlag( false );
        m_pcEncTop->getPPS()->setInferScalingListFlag( false );
        m_pcEncTop->getPPS()->setScalingListPresentFlag( false );

        // infer the scaling list from the reference layer
        pcSlice->setScalingList ( m_ppcTEncTop[refLayerId]->getScalingList() );
      }
      else
      {
#endif
      pcSlice->setDefaultScalingList ();
      m_pcEncTop->getSPS()->setScalingListPresentFlag(false);
      m_pcEncTop->getPPS()->setScalingListPresentFlag(false);

#if SCALINGLIST_INFERRING
      }
#endif

      m_pcEncTop->getTrQuant()->setScalingList(pcSlice->getScalingList());
      m_pcEncTop->getTrQuant()->setUseScalingList(true);
    }
    else if(m_pcEncTop->getUseScalingListId() == SCALING_LIST_FILE_READ)
    {
#if SCALINGLIST_INFERRING
      // inferring of the scaling list can be moved to the config file
      UInt refLayerId = 0;
#if VPS_AVC_BL_FLAG_REMOVAL
      if( m_layerId > 0 && !m_pcEncTop->getVPS()->getNonHEVCBaseLayerFlag() && m_pcEncTop->getVPS()->getRecursiveRefLayerFlag( m_layerId, refLayerId ) )
#else
      if( m_layerId > 0 && !m_pcEncTop->getVPS()->getAvcBaseLayerFlag() && m_pcEncTop->getVPS()->getRecursiveRefLayerFlag( m_layerId, refLayerId ) )
#endif
      {
        m_pcEncTop->getSPS()->setInferScalingListFlag( true );
        m_pcEncTop->getSPS()->setScalingListRefLayerId( refLayerId );
        m_pcEncTop->getSPS()->setScalingListPresentFlag( false );
        m_pcEncTop->getPPS()->setInferScalingListFlag( false );
        m_pcEncTop->getPPS()->setScalingListPresentFlag( false );

        // infer the scaling list from the reference layer
        pcSlice->setScalingList ( m_ppcTEncTop[refLayerId]->getScalingList() );
      }
      else
      {
#endif

      if(pcSlice->getScalingList()->xParseScalingList(m_pcCfg->getScalingListFile()))
      {
        pcSlice->setDefaultScalingList ();
      }
      pcSlice->getScalingList()->checkDcOfMatrix();
      m_pcEncTop->getSPS()->setScalingListPresentFlag(pcSlice->checkDefaultScalingList());
      m_pcEncTop->getPPS()->setScalingListPresentFlag(false);

#if SCALINGLIST_INFERRING
    }
#endif

      m_pcEncTop->getTrQuant()->setScalingList(pcSlice->getScalingList());
      m_pcEncTop->getTrQuant()->setUseScalingList(true);
    }
    else
    {
      printf("error : ScalingList == %d no support\n",m_pcEncTop->getUseScalingListId());
      assert(0);
    }

    if(pcSlice->getSliceType()==B_SLICE&&m_pcCfg->getGOPEntry(iGOPid).m_sliceType=='P')
    {
      pcSlice->setSliceType(P_SLICE);
    }
    if(pcSlice->getSliceType()==B_SLICE&&m_pcCfg->getGOPEntry(iGOPid).m_sliceType=='I')
    {
      pcSlice->setSliceType(I_SLICE);
    }

    // Set the nal unit type
    pcSlice->setNalUnitType(getNalUnitType(pocCurr, m_iLastIDR, isField));
#if SVC_EXTENSION
    if (m_layerId > 0)
    {
      Int interLayerPredLayerIdcTmp[MAX_VPS_LAYER_ID_PLUS1];
      Int activeNumILRRefIdxTmp = 0;

      for( Int i = 0; i < pcSlice->getActiveNumILRRefIdx(); i++ )
      {
        UInt refLayerIdc = pcSlice->getInterLayerPredLayerIdc(i);
        UInt refLayerId = pcSlice->getVPS()->getRefLayerId(m_layerId, refLayerIdc);
#if VPS_EXTN_DIRECT_REF_LAYERS
        TComList<TComPic*> *cListPic = m_ppcTEncTop[m_layerId]->getRefLayerEnc(refLayerIdc)->getListPic();
#else
        TComList<TComPic*> *cListPic = m_ppcTEncTop[m_layerId-1]->getListPic();
#endif
        pcSlice->setBaseColPic( *cListPic, refLayerIdc );

        // Apply temporal layer restriction to inter-layer prediction
#if O0225_MAX_TID_FOR_REF_LAYERS
        Int maxTidIlRefPicsPlus1 = m_pcEncTop->getVPS()->getMaxTidIlRefPicsPlus1(pcSlice->getBaseColPic(refLayerIdc)->getSlice(0)->getLayerId(),m_layerId);
#else
        Int maxTidIlRefPicsPlus1 = m_pcEncTop->getVPS()->getMaxTidIlRefPicsPlus1(pcSlice->getBaseColPic(refLayerIdc)->getSlice(0)->getLayerId());
#endif 
        if( ((Int)(pcSlice->getBaseColPic(refLayerIdc)->getSlice(0)->getTLayer())<=maxTidIlRefPicsPlus1-1) || (maxTidIlRefPicsPlus1==0 && pcSlice->getBaseColPic(refLayerIdc)->getSlice(0)->getRapPicFlag()) )
        {
          interLayerPredLayerIdcTmp[activeNumILRRefIdxTmp++] = refLayerIdc; // add picture to the list of valid inter-layer pictures
        }
        else
        {
          continue; // ILP is not valid due to temporal layer restriction
        }

#if O0098_SCALED_REF_LAYER_ID
        const Window &scalEL = m_pcEncTop->getScaledRefLayerWindowForLayer(refLayerId);
#else
        const Window &scalEL = m_pcEncTop->getScaledRefLayerWindow(refLayerIdc);
#endif

#if REF_REGION_OFFSET
        const Window &windowRL  = m_pcEncTop->getRefLayerWindowForLayer(pcSlice->getVPS()->getRefLayerId(m_layerId, refLayerIdc));
        Int widthBL   = pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec()->getWidth() - windowRL.getWindowLeftOffset() - windowRL.getWindowRightOffset();
        Int heightBL  = pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec()->getHeight() - windowRL.getWindowTopOffset() - windowRL.getWindowBottomOffset();
#else
        Int widthBL   = pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec()->getWidth();
        Int heightBL  = pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec()->getHeight();
#if Q0200_CONFORMANCE_BL_SIZE
        Int chromaFormatIdc = pcSlice->getBaseColPic(refLayerIdc)->getSlice(0)->getChromaFormatIdc();
        const Window &confBL = pcSlice->getBaseColPic(refLayerIdc)->getConformanceWindow();
        widthBL  -= ( confBL.getWindowLeftOffset() + confBL.getWindowRightOffset() ) * TComSPS::getWinUnitX( chromaFormatIdc );
        heightBL -= ( confBL.getWindowTopOffset() + confBL.getWindowBottomOffset() ) * TComSPS::getWinUnitY( chromaFormatIdc );
#endif
#endif
        Int widthEL   = pcPic->getPicYuvRec()->getWidth()  - scalEL.getWindowLeftOffset() - scalEL.getWindowRightOffset();
        Int heightEL  = pcPic->getPicYuvRec()->getHeight() - scalEL.getWindowTopOffset()  - scalEL.getWindowBottomOffset();

#if RESAMPLING_FIX
#if REF_REGION_OFFSET
        // conformance check: the values of RefLayerRegionWidthInSamplesY, RefLayerRegionHeightInSamplesY, ScaledRefRegionWidthInSamplesY and ScaledRefRegionHeightInSamplesY shall be greater than 0
        assert(widthEL > 0 && heightEL > 0 && widthBL > 0 && widthEL > 0);

        // conformance check: ScaledRefRegionWidthInSamplesY shall be greater or equal to RefLayerRegionWidthInSamplesY and ScaledRefRegionHeightInSamplesY shall be greater or equal to RefLayerRegionHeightInSamplesY
        assert(widthEL >= widthBL && heightEL >= heightBL);

#if R0209_GENERIC_PHASE
        // conformance check: when ScaledRefRegionWidthInSamplesY is equal to RefLayerRegionWidthInSamplesY, PhaseHorY shall be equal to 0, when ScaledRefRegionWidthInSamplesC is equal to RefLayerRegionWidthInSamplesC, PhaseHorC shall be equal to 0, when ScaledRefRegionHeightInSamplesY is equal to RefLayerRegionHeightInSamplesY, PhaseVerY shall be equal to 0, and when ScaledRefRegionHeightInSamplesC is equal to RefLayerRegionHeightInSamplesC, PhaseVerC shall be equal to 0.
        Int phaseHorLuma   = pcSlice->getPPS()->getPhaseHorLuma(refLayerIdc);
        Int phaseVerLuma   = pcSlice->getPPS()->getPhaseVerLuma(refLayerIdc);
        Int phaseHorChroma = pcSlice->getPPS()->getPhaseHorChroma(refLayerIdc);
        Int phaseVerChroma = pcSlice->getPPS()->getPhaseVerChroma(refLayerIdc);
        assert( ( (widthEL  != widthBL)  || (phaseHorLuma == 0 && phaseHorChroma == 0) )
             && ( (heightEL != heightBL) || (phaseVerLuma == 0 && phaseVerChroma == 0) ) );
#endif
#endif
#endif

        g_mvScalingFactor[refLayerIdc][0] = widthEL  == widthBL  ? 4096 : Clip3(-4096, 4095, ((widthEL  << 8) + (widthBL  >> 1)) / widthBL);
        g_mvScalingFactor[refLayerIdc][1] = heightEL == heightBL ? 4096 : Clip3(-4096, 4095, ((heightEL << 8) + (heightBL >> 1)) / heightBL);

        g_posScalingFactor[refLayerIdc][0] = ((widthBL  << 16) + (widthEL  >> 1)) / widthEL;
        g_posScalingFactor[refLayerIdc][1] = ((heightBL << 16) + (heightEL >> 1)) / heightEL;

#if Q0048_CGS_3D_ASYMLUT 
        TComPicYuv* pBaseColRec = pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec();
        if( pcSlice->getPPS()->getCGSFlag() )
        {
#if R0150_CGS_SIGNAL_CONSTRAINTS
          // all reference layers are currently taken as CGS reference layers
          m_Enc3DAsymLUTPPS.addRefLayerId( pcSlice->getVPS()->getRefLayerId(m_layerId, refLayerIdc) );
          m_Enc3DAsymLUTPicUpdate.addRefLayerId( pcSlice->getVPS()->getRefLayerId(m_layerId, refLayerIdc) );
#endif
          if(g_posScalingFactor[refLayerIdc][0] < (1<<16) || g_posScalingFactor[refLayerIdc][1] < (1<<16)) //if(pcPic->isSpatialEnhLayer(refLayerIdc))
          {
            //downsampling;
            downScalePic(pcPic->getPicYuvOrg(), pcSlice->getBaseColPic(refLayerIdc)->getPicYuvOrg());
            //pcSlice->getBaseColPic(refLayerIdc)->getPicYuvOrg()->dump("ds.yuv", true, true);
            m_Enc3DAsymLUTPPS.setDsOrigPic(pcSlice->getBaseColPic(refLayerIdc)->getPicYuvOrg());
            m_Enc3DAsymLUTPicUpdate.setDsOrigPic(pcSlice->getBaseColPic(refLayerIdc)->getPicYuvOrg());
          }
          else
          {
            m_Enc3DAsymLUTPPS.setDsOrigPic(pcPic->getPicYuvOrg());
            m_Enc3DAsymLUTPicUpdate.setDsOrigPic(pcPic->getPicYuvOrg());
          }

          Bool bSignalPPS = m_bSeqFirst;
          bSignalPPS |= m_pcCfg->getGOPSize() > 1 ? pocCurr % m_pcCfg->getIntraPeriod() == 0 : pocCurr % m_pcCfg->getFrameRate() == 0;
          xDetermin3DAsymLUT( pcSlice , pcPic , refLayerIdc , m_pcCfg , bSignalPPS );
          m_Enc3DAsymLUTPPS.colorMapping( pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec(),  m_pColorMappedPic );
          pBaseColRec = m_pColorMappedPic;
        }
#endif
#if SVC_EXTENSION
        if( pcPic->isSpatialEnhLayer(refLayerIdc) )
        {
          // check for the sample prediction picture type
          if( m_ppcTEncTop[m_layerId]->getSamplePredEnabledFlag(refLayerId) )
          {
#if P0312_VERT_PHASE_ADJ
            //when PhasePositionEnableFlag is equal to 1, set vertPhasePositionFlag to 0 if BL is top field and 1 if bottom
            if( scalEL.getVertPhasePositionEnableFlag() )
            {
              pcSlice->setVertPhasePositionFlag( pcSlice->getPOC()%2, refLayerIdc );
            }
#endif
#if O0215_PHASE_ALIGNMENT
#if O0194_JOINT_US_BITSHIFT
#if Q0048_CGS_3D_ASYMLUT 
            m_pcPredSearch->upsampleBasePic( pcSlice, refLayerIdc, pcPic->getFullPelBaseRec(refLayerIdc), pBaseColRec, pcPic->getPicYuvRec(), pcSlice->getVPS()->getPhaseAlignFlag() );
#else
            m_pcPredSearch->upsampleBasePic( pcSlice, refLayerIdc, pcPic->getFullPelBaseRec(refLayerIdc), pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec(), pcPic->getPicYuvRec(), pcSlice->getVPS()->getPhaseAlignFlag() );
#endif
#else
#if Q0048_CGS_3D_ASYMLUT
            m_pcPredSearch->upsampleBasePic( refLayerIdc, pcPic->getFullPelBaseRec(refLayerIdc), pBaseColRec, pcPic->getPicYuvRec(), scalEL, pcSlice->getVPS()->getPhaseAlignFlag() );
#else
            m_pcPredSearch->upsampleBasePic( refLayerIdc, pcPic->getFullPelBaseRec(refLayerIdc), pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec(), pcPic->getPicYuvRec(), scalEL, pcSlice->getVPS()->getPhaseAlignFlag() );
#endif
#endif
#else
#if O0194_JOINT_US_BITSHIFT
#if Q0048_CGS_3D_ASYMLUT 
#if REF_REGION_OFFSET
          m_pcPredSearch->upsampleBasePic( pcSlice, refLayerIdc, pcPic->getFullPelBaseRec(refLayerIdc), pBaseColRec, pcPic->getPicYuvRec(), scalEL, altRL );
#else
          m_pcPredSearch->upsampleBasePic( pcSlice, refLayerIdc, pcPic->getFullPelBaseRec(refLayerIdc), pBaseColRec, pcPic->getPicYuvRec(), scalEL );
#endif
#else
          m_pcPredSearch->upsampleBasePic( pcSlice, refLayerIdc, pcPic->getFullPelBaseRec(refLayerIdc), pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec(), pcPic->getPicYuvRec(), scalEL );
#endif
#else
#if Q0048_CGS_3D_ASYMLUT 
            m_pcPredSearch->upsampleBasePic( refLayerIdc, pcPic->getFullPelBaseRec(refLayerIdc), pBaseColRec, pcPic->getPicYuvRec(), scalEL );
#else
            m_pcPredSearch->upsampleBasePic( refLayerIdc, pcPic->getFullPelBaseRec(refLayerIdc), pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec(), pcPic->getPicYuvRec(), scalEL );
#endif
#endif
#endif
          }
        }
        else
        {
#if Q0048_CGS_3D_ASYMLUT 
          pcPic->setFullPelBaseRec( refLayerIdc, pBaseColRec );
#else
          pcPic->setFullPelBaseRec( refLayerIdc, pcSlice->getBaseColPic(refLayerIdc)->getPicYuvRec() );
#endif
        }
        pcSlice->setFullPelBaseRec ( refLayerIdc, pcPic->getFullPelBaseRec(refLayerIdc) );
#endif //SVC_EXTENSION
      }

      // Update the list of active inter-layer pictures
      for ( Int i = 0; i < activeNumILRRefIdxTmp; i++)
      {
        pcSlice->setInterLayerPredLayerIdc( interLayerPredLayerIdcTmp[i], i );
      }

#if !O0225_TID_BASED_IL_RPS_DERIV || Q0060_MAX_TID_REF_EQUAL_TO_ZERO
      pcSlice->setActiveNumILRRefIdx( activeNumILRRefIdxTmp );
#endif 
      if ( pcSlice->getActiveNumILRRefIdx() == 0 )
      {
        // No valid inter-layer pictures -> disable inter-layer prediction
        pcSlice->setInterLayerPredEnabledFlag(false);
      }

      if( pocCurr % m_pcCfg->getIntraPeriod() == 0 )
      {
        if(pcSlice->getVPS()->getCrossLayerIrapAlignFlag())
        {
          TComList<TComPic*> *cListPic = m_ppcTEncTop[m_layerId]->getRefLayerEnc(0)->getListPic();
          TComPic* picLayer0 = pcSlice->getRefPic(*cListPic, pcSlice->getPOC() );
          if(picLayer0)
          {
            pcSlice->setNalUnitType(picLayer0->getSlice(0)->getNalUnitType());
          }
          else
          {
            pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_CRA);
          }
        }
        else
        {
#if !ALIGN_IRAP_BUGFIX
          pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_CRA);
#endif
        }
      }
#if ISLICE_TYPE_NUMDIR
      if( pcSlice->getActiveNumILRRefIdx() == 0 && pcSlice->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && pcSlice->getNalUnitType() <= NAL_UNIT_CODED_SLICE_CRA && (m_pcEncTop->getNumDirectRefLayers() == 0) )
#else
      if( pcSlice->getActiveNumILRRefIdx() == 0 && pcSlice->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && pcSlice->getNalUnitType() <= NAL_UNIT_CODED_SLICE_CRA )
#endif
      {
        pcSlice->setSliceType(I_SLICE);
      }
      else if( !m_pcEncTop->getElRapSliceTypeB() )
      {
        if( (pcSlice->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP) &&
          (pcSlice->getNalUnitType() <= NAL_UNIT_CODED_SLICE_CRA) &&
          pcSlice->getSliceType() == B_SLICE )
        {
          pcSlice->setSliceType(P_SLICE);
        }
      }      
    }
#endif //#if SVC_EXTENSION
    if(pcSlice->getTemporalLayerNonReferenceFlag())
    {
      if (pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_TRAIL_R &&
#if SVC_EXTENSION
        ( m_iGopSize != 1 || m_ppcTEncTop[m_layerId]->getIntraPeriod() > 1 ) )
#else
          !(m_iGopSize == 1 && pcSlice->getSliceType() == I_SLICE))
#endif
        // Add this condition to avoid POC issues with encoder_intra_main.cfg configuration (see #1127 in bug tracker)
      {
        pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TRAIL_N);
    }
      if(pcSlice->getNalUnitType()==NAL_UNIT_CODED_SLICE_RADL_R)
      {
        pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_RADL_N);
      }
      if(pcSlice->getNalUnitType()==NAL_UNIT_CODED_SLICE_RASL_R)
      {
        pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_RASL_N);
      }
    }

#if EFFICIENT_FIELD_IRAP
#if FIX1172
    if ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA )  // IRAP picture
    {
      m_associatedIRAPType = pcSlice->getNalUnitType();
#if POC_RESET_IDC_ENCODER
      m_associatedIRAPPOC = pcSlice->getPOC();
      m_associatedIrapPocBeforeReset = pocCurr;
#else
      m_associatedIRAPPOC = pocCurr;
#endif
    }
    pcSlice->setAssociatedIRAPType(m_associatedIRAPType);
    pcSlice->setAssociatedIRAPPOC(m_associatedIRAPPOC);
#if POC_RESET_IDC_ENCODER
    pcSlice->setAssociatedIrapPocBeforeReset(m_associatedIrapPocBeforeReset);
#endif
#endif
#endif
    // Do decoding refresh marking if any 
#if NO_CLRAS_OUTPUT_FLAG
    pcSlice->decodingRefreshMarking(m_pocCRA, m_bRefreshPending, rcListPic, m_pcEncTop->getNoClrasOutputFlag());
#else
    pcSlice->decodingRefreshMarking(m_pocCRA, m_bRefreshPending, rcListPic);
#endif
#if POC_RESET_IDC_ENCODER
    // m_pocCRA may have been update here; update m_pocCraWithoutReset
    m_pocCraWithoutReset = m_pocCRA + m_pcEncTop->getPocAdjustmentValue();
#endif
    m_pcEncTop->selectReferencePictureSet(pcSlice, pocCurr, iGOPid);
    pcSlice->getRPS()->setNumberOfLongtermPictures(0);
#if EFFICIENT_FIELD_IRAP
#else
#if FIX1172
    if ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA )  // IRAP picture
    {
      m_associatedIRAPType = pcSlice->getNalUnitType();
      m_associatedIRAPPOC = pocCurr;
    }
    pcSlice->setAssociatedIRAPType(m_associatedIRAPType);
    pcSlice->setAssociatedIRAPPOC(m_associatedIRAPPOC);
#endif
#endif

#if ALLOW_RECOVERY_POINT_AS_RAP
    if ((pcSlice->checkThatAllRefPicsAreAvailable(rcListPic, pcSlice->getRPS(), false, m_iLastRecoveryPicPOC, m_pcCfg->getDecodingRefreshType() == 3) != 0) || (pcSlice->isIRAP()) 
#if EFFICIENT_FIELD_IRAP
      || (isField && pcSlice->getAssociatedIRAPType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && pcSlice->getAssociatedIRAPType() <= NAL_UNIT_CODED_SLICE_CRA && pcSlice->getAssociatedIRAPPOC() == pcSlice->getPOC()+1)
#endif
      )
    {
      pcSlice->createExplicitReferencePictureSetFromReference(rcListPic, pcSlice->getRPS(), pcSlice->isIRAP(), m_iLastRecoveryPicPOC, m_pcCfg->getDecodingRefreshType() == 3);
    }
#else
    if ((pcSlice->checkThatAllRefPicsAreAvailable(rcListPic, pcSlice->getRPS(), false) != 0) || (pcSlice->isIRAP()))
    {
      pcSlice->createExplicitReferencePictureSetFromReference(rcListPic, pcSlice->getRPS(), pcSlice->isIRAP());
    }
#endif
#if ALIGNED_BUMPING
#if POC_RESET_IDC_ENCODER
    pcSlice->checkLeadingPictureRestrictions(rcListPic, true);
#else
    pcSlice->checkLeadingPictureRestrictions(rcListPic);
#endif
#endif
    pcSlice->applyReferencePictureSet(rcListPic, pcSlice->getRPS());

    if(pcSlice->getTLayer() > 0 
      &&  !( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_N     // Check if not a leading picture
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_R
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R )
        )
    {
      if(pcSlice->isTemporalLayerSwitchingPoint(rcListPic) || pcSlice->getSPS()->getTemporalIdNestingFlag())
      {
#if SVC_EXTENSION && !Q0108_TSA_STSA
        if( pcSlice->getLayerId() > 0 )
        {
          Bool oneRefLayerTSA = false, oneRefLayerNotTSA = false;
          for( Int i = 0; i < pcSlice->getLayerId(); i++)
          {
            TComList<TComPic *> *cListPic = m_ppcTEncTop[i]->getListPic();
            TComPic *lowerLayerPic = pcSlice->getRefPic(*cListPic, pcSlice->getPOC());
            if( lowerLayerPic && pcSlice->getVPS()->getDirectDependencyFlag(pcSlice->getLayerId(), i) )
            {
              if( ( lowerLayerPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_TSA_N ) ||
                  ( lowerLayerPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_TSA_R ) 
                )
              {
                if(pcSlice->getTemporalLayerNonReferenceFlag() )
                {
                  pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TSA_N);
                }
                else
                {
                  pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TSA_R );
                }
                oneRefLayerTSA = true;
              }
              else
              {
                oneRefLayerNotTSA = true;
              }
            }
          }
          assert( !( oneRefLayerNotTSA && oneRefLayerTSA ) ); // Only one variable should be true - failure of this assert means
                                                                // that two independent reference layers that are not dependent on
                                                                // each other, but are reference for current layer have inconsistency
          if( oneRefLayerNotTSA /*&& !oneRefLayerTSA*/ )          // No reference layer is TSA - set current as TRAIL
          {
            if(pcSlice->getTemporalLayerNonReferenceFlag() )
            {
              pcSlice->setNalUnitType( NAL_UNIT_CODED_SLICE_TRAIL_N );
            }
            else
            {
              pcSlice->setNalUnitType( NAL_UNIT_CODED_SLICE_TRAIL_R );
            }
          }
          else  // This means there is no reference layer picture for current picture in this AU
          {
            if(pcSlice->getTemporalLayerNonReferenceFlag() )
            {
              pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TSA_N);
            }
            else
            {
              pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TSA_R );
            }
          }
        }
#else
        if(pcSlice->getTemporalLayerNonReferenceFlag())
        {
          pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TSA_N);
        }
        else
        {
          pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TSA_R);
        }
#endif
      }
      else if(pcSlice->isStepwiseTemporalLayerSwitchingPointCandidate(rcListPic))
      {
        Bool isSTSA=true;
        for(Int ii=iGOPid+1;(ii<m_pcCfg->getGOPSize() && isSTSA==true);ii++)
        {
          Int lTid= m_pcCfg->getGOPEntry(ii).m_temporalId;
          if(lTid==pcSlice->getTLayer()) 
          {
            TComReferencePictureSet* nRPS = pcSlice->getSPS()->getRPSList()->getReferencePictureSet(ii);
            for(Int jj=0;jj<nRPS->getNumberOfPictures();jj++)
            {
              if(nRPS->getUsed(jj)) 
              {
                Int tPoc=m_pcCfg->getGOPEntry(ii).m_POC+nRPS->getDeltaPOC(jj);
                Int kk=0;
                for(kk=0;kk<m_pcCfg->getGOPSize();kk++)
                {
                  if(m_pcCfg->getGOPEntry(kk).m_POC==tPoc)
                    break;
                }
                Int tTid=m_pcCfg->getGOPEntry(kk).m_temporalId;
                if(tTid >= pcSlice->getTLayer())
                {
                  isSTSA=false;
                  break;
                }
              }
            }
          }
        }
        if(isSTSA==true)
        {    
#if SVC_EXTENSION && !Q0108_TSA_STSA
          if( pcSlice->getLayerId() > 0 )
          {
            Bool oneRefLayerSTSA = false, oneRefLayerNotSTSA = false;
            for( Int i = 0; i < pcSlice->getLayerId(); i++)
            {
              TComList<TComPic *> *cListPic = m_ppcTEncTop[i]->getListPic();
              TComPic *lowerLayerPic = pcSlice->getRefPic(*cListPic, pcSlice->getPOC());
              if( lowerLayerPic && pcSlice->getVPS()->getDirectDependencyFlag(pcSlice->getLayerId(), i) )
              {
                if( ( lowerLayerPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_STSA_N ) ||
                    ( lowerLayerPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_STSA_R ) 
                  )
                {
                  if(pcSlice->getTemporalLayerNonReferenceFlag() )
                  {
                    pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_N);
                  }
                  else
                  {
                    pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_R );
                  }
                  oneRefLayerSTSA = true;
                }
                else
                {
                  oneRefLayerNotSTSA = true;
                }
              }
            }
            assert( !( oneRefLayerNotSTSA && oneRefLayerSTSA ) ); // Only one variable should be true - failure of this assert means
                                                                  // that two independent reference layers that are not dependent on
                                                                  // each other, but are reference for current layer have inconsistency
            if( oneRefLayerNotSTSA /*&& !oneRefLayerSTSA*/ )          // No reference layer is STSA - set current as TRAIL
            {
              if(pcSlice->getTemporalLayerNonReferenceFlag() )
              {
                pcSlice->setNalUnitType( NAL_UNIT_CODED_SLICE_TRAIL_N );
              }
              else
              {
                pcSlice->setNalUnitType( NAL_UNIT_CODED_SLICE_TRAIL_R );
              }
            }
            else  // This means there is no reference layer picture for current picture in this AU
            {
              if(pcSlice->getTemporalLayerNonReferenceFlag() )
              {
                pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_N);
              }
              else
              {
                pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_R );
              }
            }
          }
#else
          if(pcSlice->getTemporalLayerNonReferenceFlag())
          {
            pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_N);
          }
          else
          {
            pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_R);
          }
#endif
        }
      }
    }

    arrangeLongtermPicturesInRPS(pcSlice, rcListPic);
    TComRefPicListModification* refPicListModification = pcSlice->getRefPicListModification();
    refPicListModification->setRefPicListModificationFlagL0(0);
    refPicListModification->setRefPicListModificationFlagL1(0);
    pcSlice->setNumRefIdx(REF_PIC_LIST_0,min(m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive,pcSlice->getRPS()->getNumberOfPictures()));
    pcSlice->setNumRefIdx(REF_PIC_LIST_1,min(m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive,pcSlice->getRPS()->getNumberOfPictures()));

#if SVC_EXTENSION
    if( m_layerId > 0 && pcSlice->getActiveNumILRRefIdx() )
    {
#if POC_RESET_FLAG || POC_RESET_IDC_ENCODER
      if ( pocCurr > 0 && pcSlice->isRADL() && pcPic->getSlice(0)->getBaseColPic(pcPic->getSlice(0)->getInterLayerPredLayerIdc(0))->getSlice(0)->isRASL())
#else
      if (pcSlice->getPOC()>0  && pcSlice->isRADL() && pcPic->getSlice(0)->getBaseColPic(pcPic->getSlice(0)->getInterLayerPredLayerIdc(0))->getSlice(0)->isRASL())
#endif
      {
        pcSlice->setActiveNumILRRefIdx(0);
        pcSlice->setInterLayerPredEnabledFlag(0);
      }
      if( pcSlice->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && pcSlice->getNalUnitType() <= NAL_UNIT_CODED_SLICE_CRA )
      {
        pcSlice->setNumRefIdx(REF_PIC_LIST_0, pcSlice->getActiveNumILRRefIdx());
        pcSlice->setNumRefIdx(REF_PIC_LIST_1, pcSlice->getActiveNumILRRefIdx());
      }
      else
      {
        pcSlice->setNumRefIdx(REF_PIC_LIST_0, pcSlice->getNumRefIdx(REF_PIC_LIST_0)+pcSlice->getActiveNumILRRefIdx());
        pcSlice->setNumRefIdx(REF_PIC_LIST_1, pcSlice->getNumRefIdx(REF_PIC_LIST_1)+pcSlice->getActiveNumILRRefIdx());
      }

      // check for the reference pictures whether there is at least one either temporal picture or ILRP with sample prediction type
      if( pcSlice->getNumRefIdx( REF_PIC_LIST_0 ) - pcSlice->getActiveNumILRRefIdx() == 0 && pcSlice->getNumRefIdx( REF_PIC_LIST_1 ) - pcSlice->getActiveNumILRRefIdx() == 0 )
      {
        Bool foundSamplePredPicture = false;                

        for( Int i = 0; i < pcSlice->getActiveNumILRRefIdx(); i++ )
        {
          if( m_ppcTEncTop[m_layerId]->getSamplePredEnabledFlag( pcSlice->getVPS()->getRefLayerId( m_layerId, pcSlice->getInterLayerPredLayerIdc(i) ) ) )
          {
            foundSamplePredPicture = true;
            break;
          }
        }

        if( !foundSamplePredPicture )
        {
          pcSlice->setSliceType(I_SLICE);
          pcSlice->setInterLayerPredEnabledFlag(0);
          pcSlice->setActiveNumILRRefIdx(0);
        }
      }
    }
#endif //SVC_EXTENSION

#if Q0108_TSA_STSA
   if( ( pcSlice->getTLayer() == 0 && pcSlice->getLayerId() > 0  )    // only for enhancement layer and with temporal layer 0
     && !( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_N     
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_R
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R 
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA
          )
        )
    {
        Bool isSTSA=true;
        Bool isIntra=false;

        for( Int i = 0; i < pcSlice->getLayerId(); i++)
        {
          TComList<TComPic *> *cListPic = m_ppcTEncTop[i]->getListPic();
          TComPic *lowerLayerPic = pcSlice->getRefPic(*cListPic, pcSlice->getPOC());
          if( lowerLayerPic && pcSlice->getVPS()->getDirectDependencyFlag(pcSlice->getLayerId(), i) )
          {
            if( lowerLayerPic->getSlice(0)->getSliceType() == I_SLICE)
            { 
              isIntra = true;
            }
          }
        }

        for(Int ii=iGOPid+1; ii < m_pcCfg->getGOPSize() && isSTSA; ii++)
        {
          Int lTid= m_pcCfg->getGOPEntry(ii).m_temporalId;
          if(lTid==pcSlice->getTLayer()) 
          {
            TComReferencePictureSet* nRPS = pcSlice->getSPS()->getRPSList()->getReferencePictureSet(ii);
            for(Int jj=0; jj<nRPS->getNumberOfPictures(); jj++)
            {
              if(nRPS->getUsed(jj)) 
              {
                Int tPoc=m_pcCfg->getGOPEntry(ii).m_POC+nRPS->getDeltaPOC(jj);
                Int kk=0;
                for(kk=0; kk<m_pcCfg->getGOPSize(); kk++)
                {
                  if(m_pcCfg->getGOPEntry(kk).m_POC==tPoc)
                  {
                    break;
                  }
                }
                Int tTid=m_pcCfg->getGOPEntry(kk).m_temporalId;
                if(tTid >= pcSlice->getTLayer())
                {
                  isSTSA = false;
                  break;
                }
              }
            }
          }
        }
        if(isSTSA==true && isIntra == false)
        {    
          if(pcSlice->getTemporalLayerNonReferenceFlag())
          {
            pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_N);
          }
          else
          {
            pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_R);
          }
        }
    }
#endif

#if ADAPTIVE_QP_SELECTION
    pcSlice->setTrQuant( m_pcEncTop->getTrQuant() );
#endif      

#if SVC_EXTENSION
    if( pcSlice->getSliceType() == B_SLICE )
    {
      pcSlice->setColFromL0Flag(1-uiColDir);
    }

    //  Set reference list
    if(m_layerId ==  0 || ( m_layerId > 0 && pcSlice->getActiveNumILRRefIdx() == 0 ) )
    {
      pcSlice->setRefPicList( rcListPic );
    }

    if( m_layerId > 0 && pcSlice->getActiveNumILRRefIdx() )
    {
      pcSlice->setILRPic( m_pcEncTop->getIlpList() );
#if !REF_IDX_MFM
      //  Set reference list
      pcSlice->setRefPicList ( rcListPic );
#endif
      pcSlice->setRefPicListModificationSvc();
      pcSlice->setRefPicList( rcListPic, false, m_pcEncTop->getIlpList());

#if REF_IDX_MFM
      if( pcSlice->getMFMEnabledFlag() )
      {
        Bool found         = false;
        UInt ColFromL0Flag = pcSlice->getColFromL0Flag();
        UInt ColRefIdx     = pcSlice->getColRefIdx();

        for(Int colIdx = 0; colIdx < pcSlice->getNumRefIdx( RefPicList(1 - ColFromL0Flag) ); colIdx++) 
        {
          RefPicList refList = RefPicList(1 - ColFromL0Flag);
          TComPic* refPic = pcSlice->getRefPic(refList, colIdx);

          // It is a requirement of bitstream conformance when the collocated picture, used for temporal motion vector prediction, is an inter-layer reference picture, 
          // VpsInterLayerMotionPredictionEnabled[ LayerIdxInVps[ currLayerId ] ][ LayerIdxInVps[ rLId ] ] shall be equal to 1, where rLId is set equal to nuh_layer_id of the inter-layer picture.
          if( refPic->isILR(m_layerId) && m_ppcTEncTop[m_layerId]->getMotionPredEnabledFlag( refPic->getLayerId() )
#if MFM_ENCCONSTRAINT
            && pcSlice->getBaseColPic( *m_ppcTEncTop[refPic->getLayerId()]->getListPic() )->checkSameRefInfo() == true 
#endif
            ) 
          { 
            ColRefIdx = colIdx; 
            found = true;
            break; 
          }
        }

        if( found == false )
        {
          ColFromL0Flag = 1 - ColFromL0Flag;
          for(Int colIdx = 0; colIdx < pcSlice->getNumRefIdx( RefPicList(1 - ColFromL0Flag) ); colIdx++) 
          {
            RefPicList refList = RefPicList(1 - ColFromL0Flag);
            TComPic* refPic = pcSlice->getRefPic(refList, colIdx);

            // It is a requirement of bitstream conformance when the collocated picture, used for temporal motion vector prediction, is an inter-layer reference picture, 
            // VpsInterLayerMotionPredictionEnabled[ LayerIdxInVps[ currLayerId ] ][ LayerIdxInVps[ rLId ] ] shall be equal to 1, where rLId is set equal to nuh_layer_id of the inter-layer picture.
            if( refPic->isILR(m_layerId) && m_ppcTEncTop[m_layerId]->getMotionPredEnabledFlag( refPic->getLayerId() )
#if MFM_ENCCONSTRAINT
              && pcSlice->getBaseColPic( *m_ppcTEncTop[refPic->getLayerId()]->getListPic() )->checkSameRefInfo() == true 
#endif
              ) 
            { 
              ColRefIdx = colIdx; 
              found = true; 
              break; 
            } 
          }
        }

        if(found == true)
        {
          pcSlice->setColFromL0Flag(ColFromL0Flag);
          pcSlice->setColRefIdx(ColRefIdx);
        }
      }
#endif
    }
#else //SVC_EXTENSION
    //  Set reference list
    pcSlice->setRefPicList ( rcListPic );
#endif //#if SVC_EXTENSION

    //  Slice info. refinement
    if ( (pcSlice->getSliceType() == B_SLICE) && (pcSlice->getNumRefIdx(REF_PIC_LIST_1) == 0) )
    {
      pcSlice->setSliceType ( P_SLICE );
    }

    if (pcSlice->getSliceType() == B_SLICE)
    {
#if !SVC_EXTENSION
      pcSlice->setColFromL0Flag(1-uiColDir);
#endif
      Bool bLowDelay = true;
      Int  iCurrPOC  = pcSlice->getPOC();
      Int iRefIdx = 0;

      for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_0) && bLowDelay; iRefIdx++)
      {
        if ( pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx)->getPOC() > iCurrPOC )
        {
          bLowDelay = false;
        }
      }
      for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_1) && bLowDelay; iRefIdx++)
      {
        if ( pcSlice->getRefPic(REF_PIC_LIST_1, iRefIdx)->getPOC() > iCurrPOC )
        {
          bLowDelay = false;
        }
      }

      pcSlice->setCheckLDC(bLowDelay);  
    }
    else
    {
      pcSlice->setCheckLDC(true);  
    }

    uiColDir = 1-uiColDir;

    //-------------------------------------------------------------
    pcSlice->setRefPOCList();

    pcSlice->setList1IdxToList0Idx();

    if (m_pcEncTop->getTMVPModeId() == 2)
    {
      if (iGOPid == 0) // first picture in SOP (i.e. forward B)
      {
        pcSlice->setEnableTMVPFlag(0);
      }
      else
      {
        // Note: pcSlice->getColFromL0Flag() is assumed to be always 0 and getcolRefIdx() is always 0.
        pcSlice->setEnableTMVPFlag(1);
      }
      pcSlice->getSPS()->setTMVPFlagsPresent(1);
    }
    else if (m_pcEncTop->getTMVPModeId() == 1)
    {
      pcSlice->getSPS()->setTMVPFlagsPresent(1);
#if SVC_EXTENSION
      if( pcSlice->getIdrPicFlag() )
      {
        pcSlice->setEnableTMVPFlag(0);
      }
      else
#endif
      pcSlice->setEnableTMVPFlag(1);
    }
    else
    {
      pcSlice->getSPS()->setTMVPFlagsPresent(0);
      pcSlice->setEnableTMVPFlag(0);
    }

#if SVC_EXTENSION
    if( m_layerId > 0 && !pcSlice->isIntra() )
    {
      Int colFromL0Flag = 1;
      Int colRefIdx = 0;

      // check whether collocated picture is valid
      if( pcSlice->getEnableTMVPFlag() )
      {
        colFromL0Flag = pcSlice->getColFromL0Flag();
        colRefIdx = pcSlice->getColRefIdx();

        TComPic* refPic = pcSlice->getRefPic(RefPicList(1-colFromL0Flag), colRefIdx);

        assert( refPic );

        // It is a requirement of bitstream conformance when the collocated picture, used for temporal motion vector prediction, is an inter-layer reference picture, 
        // VpsInterLayerMotionPredictionEnabled[ LayerIdxInVps[ currLayerId ] ][ LayerIdxInVps[ rLId ] ] shall be equal to 1, where rLId is set equal to nuh_layer_id of the inter-layer picture.
        if( refPic->isILR(m_layerId) && !m_ppcTEncTop[m_layerId]->getMotionPredEnabledFlag(refPic->getLayerId()) )
        {
          pcSlice->setEnableTMVPFlag(false);
          pcSlice->setMFMEnabledFlag(false);
          colRefIdx = 0;
        }
      }

      // remove motion only ILRP from the end of the colFromL0Flag reference picture list
      RefPicList refList = RefPicList(colFromL0Flag);
      Int numRefIdx = pcSlice->getNumRefIdx(refList);

      if( numRefIdx > 0 )
      {
        for( Int refIdx = pcSlice->getNumRefIdx(refList) - 1; refIdx > 0; refIdx-- )
        {
          TComPic* refPic = pcSlice->getRefPic(refList, refIdx);

          if( !refPic->isILR(m_layerId) || ( refPic->isILR(m_layerId) && m_ppcTEncTop[m_layerId]->getSamplePredEnabledFlag( refPic->getLayerId() ) ) )
          {
            break;
          }
          else
          {
            assert( numRefIdx > 1 );
            numRefIdx--;              
          }
        }

        pcSlice->setNumRefIdx( refList, numRefIdx );
      }

      // remove motion only ILRP from the end of the (1-colFromL0Flag) reference picture list up to colRefIdx
      refList = RefPicList(1 - colFromL0Flag);
      numRefIdx = pcSlice->getNumRefIdx(refList);

      if( numRefIdx > 0 )
      {
        for( Int refIdx = pcSlice->getNumRefIdx(refList) - 1; refIdx > colRefIdx; refIdx-- )
        {
          TComPic* refPic = pcSlice->getRefPic(refList, refIdx);

          if( !refPic->isILR(m_layerId) || ( refPic->isILR(m_layerId) && m_ppcTEncTop[m_layerId]->getSamplePredEnabledFlag( refPic->getLayerId() ) ) )
          {
            break;
          }
          else
          {
            assert( numRefIdx > 1 );
            numRefIdx--;              
          }
        }

        pcSlice->setNumRefIdx( refList, numRefIdx );
      }

      assert( pcSlice->getNumRefIdx(REF_PIC_LIST_0) > 0 && ( pcSlice->isInterP() || (pcSlice->isInterB() && pcSlice->getNumRefIdx(REF_PIC_LIST_1) > 0) ) );
    }
#endif

    /////////////////////////////////////////////////////////////////////////////////////////////////// Compress a slice
    //  Slice compression
    if (m_pcCfg->getUseASR())
    {
      m_pcSliceEncoder->setSearchRange(pcSlice);
    }

    Bool bGPBcheck=false;
    if ( pcSlice->getSliceType() == B_SLICE)
    {
      if ( pcSlice->getNumRefIdx(RefPicList( 0 ) ) == pcSlice->getNumRefIdx(RefPicList( 1 ) ) )
      {
        bGPBcheck=true;
        Int i;
        for ( i=0; i < pcSlice->getNumRefIdx(RefPicList( 1 ) ); i++ )
        {
          if ( pcSlice->getRefPOC(RefPicList(1), i) != pcSlice->getRefPOC(RefPicList(0), i) ) 
          {
            bGPBcheck=false;
            break;
          }
        }
      }
    }
    if(bGPBcheck)
    {
      pcSlice->setMvdL1ZeroFlag(true);
    }
    else
    {
      pcSlice->setMvdL1ZeroFlag(false);
    }
    pcPic->getSlice(pcSlice->getSliceIdx())->setMvdL1ZeroFlag(pcSlice->getMvdL1ZeroFlag());

    Double lambda            = 0.0;
    Int actualHeadBits       = 0;
    Int actualTotalBits      = 0;
    Int estimatedBits        = 0;
    Int tmpBitsBeforeWriting = 0;
    if ( m_pcCfg->getUseRateCtrl() )
    {
      Int frameLevel = m_pcRateCtrl->getRCSeq()->getGOPID2Level( iGOPid );
      if ( pcPic->getSlice(0)->getSliceType() == I_SLICE )
      {
        frameLevel = 0;
      }
      m_pcRateCtrl->initRCPic( frameLevel );
      estimatedBits = m_pcRateCtrl->getRCPic()->getTargetBits();

      Int sliceQP = m_pcCfg->getInitialQP();
#if POC_RESET_FLAG || POC_RESET_IDC_ENCODER
      if ( ( pocCurr == 0 && m_pcCfg->getInitialQP() > 0 ) || ( frameLevel == 0 && m_pcCfg->getForceIntraQP() ) ) // QP is specified
#else
      if ( ( pcSlice->getPOC() == 0 && m_pcCfg->getInitialQP() > 0 ) || ( frameLevel == 0 && m_pcCfg->getForceIntraQP() ) ) // QP is specified
#endif
      {
        Int    NumberBFrames = ( m_pcCfg->getGOPSize() - 1 );
        Double dLambda_scale = 1.0 - Clip3( 0.0, 0.5, 0.05*(Double)NumberBFrames );
        Double dQPFactor     = 0.57*dLambda_scale;
        Int    SHIFT_QP      = 12;
        Int    bitdepth_luma_qp_scale = 0;
        Double qp_temp = (Double) sliceQP + bitdepth_luma_qp_scale - SHIFT_QP;
        lambda = dQPFactor*pow( 2.0, qp_temp/3.0 );
      }
      else if ( frameLevel == 0 )   // intra case, but use the model
      {
        m_pcSliceEncoder->calCostSliceI(pcPic);
        if ( m_pcCfg->getIntraPeriod() != 1 )   // do not refine allocated bits for all intra case
        {
          Int bits = m_pcRateCtrl->getRCSeq()->getLeftAverageBits();
          bits = m_pcRateCtrl->getRCPic()->getRefineBitsForIntra( bits );
          if ( bits < 200 )
          {
            bits = 200;
          }
          m_pcRateCtrl->getRCPic()->setTargetBits( bits );
        }

        list<TEncRCPic*> listPreviousPicture = m_pcRateCtrl->getPicList();
        m_pcRateCtrl->getRCPic()->getLCUInitTargetBits();
        lambda  = m_pcRateCtrl->getRCPic()->estimatePicLambda( listPreviousPicture, pcSlice->getSliceType());
        sliceQP = m_pcRateCtrl->getRCPic()->estimatePicQP( lambda, listPreviousPicture );
      }
      else    // normal case
      {
        list<TEncRCPic*> listPreviousPicture = m_pcRateCtrl->getPicList();
        lambda  = m_pcRateCtrl->getRCPic()->estimatePicLambda( listPreviousPicture, pcSlice->getSliceType());
        sliceQP = m_pcRateCtrl->getRCPic()->estimatePicQP( lambda, listPreviousPicture );
      }

#if REPN_FORMAT_IN_VPS
      sliceQP = Clip3( -pcSlice->getQpBDOffsetY(), MAX_QP, sliceQP );
#else
      sliceQP = Clip3( -pcSlice->getSPS()->getQpBDOffsetY(), MAX_QP, sliceQP );
#endif
      m_pcRateCtrl->getRCPic()->setPicEstQP( sliceQP );

      m_pcSliceEncoder->resetQP( pcPic, sliceQP, lambda );
    }

    UInt uiNumSlices = 1;

    UInt uiInternalAddress = pcPic->getNumPartInCU()-4;
    UInt uiExternalAddress = pcPic->getPicSym()->getNumberOfCUsInFrame()-1;
    UInt uiPosX = ( uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
    UInt uiPosY = ( uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
#if REPN_FORMAT_IN_VPS
    UInt uiWidth = pcSlice->getPicWidthInLumaSamples();
    UInt uiHeight = pcSlice->getPicHeightInLumaSamples();
#else
    UInt uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
    UInt uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();
#endif
    while(uiPosX>=uiWidth||uiPosY>=uiHeight) 
    {
      uiInternalAddress--;
      uiPosX = ( uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
      uiPosY = ( uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
    }
    uiInternalAddress++;
    if(uiInternalAddress==pcPic->getNumPartInCU()) 
    {
      uiInternalAddress = 0;
      uiExternalAddress++;
    }
    UInt uiRealEndAddress = uiExternalAddress*pcPic->getNumPartInCU()+uiInternalAddress;

    Int  p, j;
    UInt uiEncCUAddr;

    pcPic->getPicSym()->initTiles(pcSlice->getPPS());

#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
    if (m_pcCfg->getInterLayerConstrainedTileSetsSEIEnabled())
    {
      xBuildTileSetsMap(pcPic->getPicSym());
    }
#endif

    // Allocate some coders, now we know how many tiles there are.
#if WPP_FIX
    const Int iNumSubstreams = pcSlice->getPPS()->getNumSubstreams();
#else
    Int iNumSubstreams = pcSlice->getPPS()->getNumSubstreams();
#endif

    //generate the Coding Order Map and Inverse Coding Order Map
    for(p=0, uiEncCUAddr=0; p<pcPic->getPicSym()->getNumberOfCUsInFrame(); p++, uiEncCUAddr = pcPic->getPicSym()->xCalculateNxtCUAddr(uiEncCUAddr))
    {
      pcPic->getPicSym()->setCUOrderMap(p, uiEncCUAddr);
      pcPic->getPicSym()->setInverseCUOrderMap(uiEncCUAddr, p);
    }
    pcPic->getPicSym()->setCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());    
    pcPic->getPicSym()->setInverseCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());

    // Allocate some coders, now we know how many tiles there are.
    m_pcEncTop->createWPPCoders(iNumSubstreams);
    pcSbacCoders = m_pcEncTop->getSbacCoders();
    pcSubstreamsOut = new TComOutputBitstream[iNumSubstreams];

    UInt startCUAddrSliceIdx = 0; // used to index "m_uiStoredStartCUAddrForEncodingSlice" containing locations of slice boundaries
    UInt startCUAddrSlice    = 0; // used to keep track of current slice's starting CU addr.
    pcSlice->setSliceCurStartCUAddr( startCUAddrSlice ); // Setting "start CU addr" for current slice
    m_storedStartCUAddrForEncodingSlice.clear();

    UInt startCUAddrSliceSegmentIdx = 0; // used to index "m_uiStoredStartCUAddrForEntropyEncodingSlice" containing locations of slice boundaries
    UInt startCUAddrSliceSegment    = 0; // used to keep track of current Dependent slice's starting CU addr.
    pcSlice->setSliceSegmentCurStartCUAddr( startCUAddrSliceSegment ); // Setting "start CU addr" for current Dependent slice

    m_storedStartCUAddrForEncodingSliceSegment.clear();
    UInt nextCUAddr = 0;
    m_storedStartCUAddrForEncodingSlice.push_back (nextCUAddr);
    startCUAddrSliceIdx++;
    m_storedStartCUAddrForEncodingSliceSegment.push_back(nextCUAddr);
    startCUAddrSliceSegmentIdx++;
#if AVC_BASE
#if VPS_AVC_BL_FLAG_REMOVAL
    if( m_layerId == 0 && m_pcEncTop->getVPS()->getNonHEVCBaseLayerFlag() )
#else
    if( m_layerId == 0 && m_pcEncTop->getVPS()->getAvcBaseLayerFlag() )
#endif
    {
      pcPic->getPicYuvOrg()->copyToPic( pcPic->getPicYuvRec() );
#if O0194_WEIGHTED_PREDICTION_CGS
      // Calculate for the base layer to be used in EL as Inter layer reference
      if( m_pcEncTop->getInterLayerWeightedPredFlag() )
      {
        m_pcSliceEncoder->estimateILWpParam( pcSlice );
      }
#endif

      if( pcSubstreamsOut )
      {
        delete[] pcSubstreamsOut;
        pcSubstreamsOut = NULL;
      }

      if( pcBitstreamRedirect )
      {
        delete pcBitstreamRedirect;
        pcBitstreamRedirect = NULL;
      }

      return;
    }
#endif

    while(nextCUAddr<uiRealEndAddress) // determine slice boundaries
    {
      pcSlice->setNextSlice       ( false );
      pcSlice->setNextSliceSegment( false );
      assert(pcPic->getNumAllocatedSlice() == startCUAddrSliceIdx);
      m_pcSliceEncoder->precompressSlice( pcPic );
      m_pcSliceEncoder->compressSlice   ( pcPic );

      Bool bNoBinBitConstraintViolated = (!pcSlice->isNextSlice() && !pcSlice->isNextSliceSegment());
      if (pcSlice->isNextSlice() || (bNoBinBitConstraintViolated && m_pcCfg->getSliceMode()==FIXED_NUMBER_OF_LCU))
      {
        startCUAddrSlice = pcSlice->getSliceCurEndCUAddr();
        // Reconstruction slice
        m_storedStartCUAddrForEncodingSlice.push_back(startCUAddrSlice);
        startCUAddrSliceIdx++;
        // Dependent slice
        if (startCUAddrSliceSegmentIdx>0 && m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx-1] != startCUAddrSlice)
        {
          m_storedStartCUAddrForEncodingSliceSegment.push_back(startCUAddrSlice);
          startCUAddrSliceSegmentIdx++;
        }

        if (startCUAddrSlice < uiRealEndAddress)
        {
          pcPic->allocateNewSlice();          
          pcPic->setCurrSliceIdx                  ( startCUAddrSliceIdx-1 );
          m_pcSliceEncoder->setSliceIdx           ( startCUAddrSliceIdx-1 );
          pcSlice = pcPic->getSlice               ( startCUAddrSliceIdx-1 );
          pcSlice->copySliceInfo                  ( pcPic->getSlice(0)      );
          pcSlice->setSliceIdx                    ( startCUAddrSliceIdx-1 );
          pcSlice->setSliceCurStartCUAddr         ( startCUAddrSlice      );
          pcSlice->setSliceSegmentCurStartCUAddr  ( startCUAddrSlice      );
          pcSlice->setSliceBits(0);
#if SVC_EXTENSION
          // copy reference list modification info from the first slice, assuming that this information is the same across all slices in the picture
          memcpy( pcSlice->getRefPicListModification(), pcPic->getSlice(0)->getRefPicListModification(), sizeof(TComRefPicListModification) );
#endif
          uiNumSlices ++;
        }
      }
      else if (pcSlice->isNextSliceSegment() || (bNoBinBitConstraintViolated && m_pcCfg->getSliceSegmentMode()==FIXED_NUMBER_OF_LCU))
      {
        startCUAddrSliceSegment                                                     = pcSlice->getSliceSegmentCurEndCUAddr();
        m_storedStartCUAddrForEncodingSliceSegment.push_back(startCUAddrSliceSegment);
        startCUAddrSliceSegmentIdx++;
        pcSlice->setSliceSegmentCurStartCUAddr( startCUAddrSliceSegment );
      }
      else
      {
        startCUAddrSlice                                                            = pcSlice->getSliceCurEndCUAddr();
        startCUAddrSliceSegment                                                     = pcSlice->getSliceSegmentCurEndCUAddr();
      }        

      nextCUAddr = (startCUAddrSlice > startCUAddrSliceSegment) ? startCUAddrSlice : startCUAddrSliceSegment;
    }
    m_storedStartCUAddrForEncodingSlice.push_back( pcSlice->getSliceCurEndCUAddr());
    startCUAddrSliceIdx++;
    m_storedStartCUAddrForEncodingSliceSegment.push_back(pcSlice->getSliceCurEndCUAddr());
    startCUAddrSliceSegmentIdx++;

    pcSlice = pcPic->getSlice(0);

    // SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
    if( pcSlice->getSPS()->getUseSAO() && m_pcCfg->getSaoLcuBoundary() )
    {
      m_pcSAO->getPreDBFStatistics(pcPic);
    }
    //-- Loop filter
    Bool bLFCrossTileBoundary = pcSlice->getPPS()->getLoopFilterAcrossTilesEnabledFlag();
    m_pcLoopFilter->setCfg(bLFCrossTileBoundary);
    if ( m_pcCfg->getDeblockingFilterMetric() )
    {
      dblMetric(pcPic, uiNumSlices);
    }
    m_pcLoopFilter->loopFilterPic( pcPic );

    /////////////////////////////////////////////////////////////////////////////////////////////////// File writing
    // Set entropy coder
    m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );

    /* write various header sets. */
    if ( m_bSeqFirst )
    {
#if SVC_EXTENSION
      OutputNALUnit nalu( NAL_UNIT_VPS, 0, 0 ); // The value of nuh_layer_id of VPS NAL unit shall be equal to 0.
#if AVC_BASE
#if VPS_AVC_BL_FLAG_REMOVAL
      if( ( m_layerId == 1 && m_pcEncTop->getVPS()->getNonHEVCBaseLayerFlag() ) || ( m_layerId == 0 && !m_pcEncTop->getVPS()->getNonHEVCBaseLayerFlag() ) )
#else
      if( ( m_layerId == 1 && m_pcEncTop->getVPS()->getAvcBaseLayerFlag() ) || ( m_layerId == 0 && !m_pcEncTop->getVPS()->getAvcBaseLayerFlag() ) )
#endif
#else
      if( m_layerId == 0 )
#endif
      {
#else
      OutputNALUnit nalu(NAL_UNIT_VPS);
#endif
#if VPS_VUI_OFFSET
      // The following code also calculates the VPS VUI offset
#endif
#if !P0125_REVERT_VPS_EXTN_OFFSET_TO_RESERVED
#if VPS_EXTN_OFFSET_CALC
      OutputNALUnit tempNalu(NAL_UNIT_VPS, 0, 0        ); // The value of nuh_layer_id of VPS NAL unit shall be equal to 0.
      m_pcEntropyCoder->setBitstream(&tempNalu.m_Bitstream);
      m_pcEntropyCoder->encodeVPS(m_pcEncTop->getVPS());  // Use to calculate the VPS extension offset
#endif
#endif
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
      m_pcEntropyCoder->encodeVPS(m_pcEncTop->getVPS());
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
      actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;
#if SVC_EXTENSION
      }
#endif

#if SVC_EXTENSION
      nalu = NALUnit(NAL_UNIT_SPS, 0, m_layerId);
#else
      nalu = NALUnit(NAL_UNIT_SPS);
#endif
#if Q0078_ADD_LAYER_SETS
      if (m_pcEncTop->getVPS()->getNumDirectRefLayers(m_layerId) == 0 && m_pcEncTop->getVPS()->getNumAddLayerSets() > 0)
      {
        nalu.m_layerId = 0; // For independent base layer rewriting
      }
#endif
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
      if (m_bSeqFirst)
      {
        pcSlice->getSPS()->setNumLongTermRefPicSPS(m_numLongTermRefPicSPS);
        for (Int k = 0; k < m_numLongTermRefPicSPS; k++)
        {
          pcSlice->getSPS()->setLtRefPicPocLsbSps(k, m_ltRefPicPocLsbSps[k]);
          pcSlice->getSPS()->setUsedByCurrPicLtSPSFlag(k, m_ltRefPicUsedByCurrPicFlag[k]);
        }
      }
      if( m_pcCfg->getPictureTimingSEIEnabled() || m_pcCfg->getDecodingUnitInfoSEIEnabled() )
      {
        UInt maxCU = m_pcCfg->getSliceArgument() >> ( pcSlice->getSPS()->getMaxCUDepth() << 1);
        UInt numDU = ( m_pcCfg->getSliceMode() == 1 ) ? ( pcPic->getNumCUsInFrame() / maxCU ) : ( 0 );
        if( pcPic->getNumCUsInFrame() % maxCU != 0 || numDU == 0 )
        {
          numDU ++;
        }
        pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->setNumDU( numDU );
        pcSlice->getSPS()->setHrdParameters( m_pcCfg->getFrameRate(), numDU, m_pcCfg->getTargetBitrate(), ( m_pcCfg->getIntraPeriod() > 0 ) );
      }
      if( m_pcCfg->getBufferingPeriodSEIEnabled() || m_pcCfg->getPictureTimingSEIEnabled() || m_pcCfg->getDecodingUnitInfoSEIEnabled() )
      {
        pcSlice->getSPS()->getVuiParameters()->setHrdParametersPresentFlag( true );
      }
#if O0092_0094_DEPENDENCY_CONSTRAINT
      assert( pcSlice->getSPS()->getLayerId() == 0 || pcSlice->getSPS()->getLayerId() == m_layerId || m_pcEncTop->getVPS()->getRecursiveRefLayerFlag(m_layerId, pcSlice->getSPS()->getLayerId()) );
#endif
      m_pcEntropyCoder->encodeSPS(pcSlice->getSPS());
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
      actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;

#if SVC_EXTENSION
      nalu = NALUnit(NAL_UNIT_PPS, 0, m_layerId);
#else
      nalu = NALUnit(NAL_UNIT_PPS);
#endif
#if Q0078_ADD_LAYER_SETS
      if (m_pcEncTop->getVPS()->getNumDirectRefLayers(m_layerId) == 0 && m_pcEncTop->getVPS()->getNumAddLayerSets() > 0)
      {
        nalu.m_layerId = 0; // For independent base layer rewriting
      }
#endif
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if O0092_0094_DEPENDENCY_CONSTRAINT
      assert( pcSlice->getPPS()->getPPSId() == 0 || pcSlice->getPPS()->getPPSId() == m_layerId || m_pcEncTop->getVPS()->getRecursiveRefLayerFlag(m_layerId, pcSlice->getPPS()->getPPSId()) );
#endif
      m_pcEntropyCoder->encodePPS(pcSlice->getPPS()
#if Q0048_CGS_3D_ASYMLUT
        , & m_Enc3DAsymLUTPPS
#endif
        );
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
      actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;

      xCreateLeadingSEIMessages(accessUnit, pcSlice->getSPS());

#if O0164_MULTI_LAYER_HRD
      if (pcSlice->getLayerId() == 0 && m_pcEncTop->getVPS()->getVpsVuiBspHrdPresentFlag())
      {
#if VPS_VUI_BSP_HRD_PARAMS
        TComVPS *vps = m_pcEncTop->getVPS();
        for(Int i = 0; i < vps->getNumOutputLayerSets(); i++)
        {
          for(Int k = 0; k < vps->getNumSignalledPartitioningSchemes(i); k++)
          {
            for(Int l = 0; l < vps->getNumPartitionsInSchemeMinus1(i, k)+1; l++)
            {
#endif
              nalu = NALUnit(NAL_UNIT_PREFIX_SEI, 0, 1);
              m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
              m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if VPS_VUI_BSP_HRD_PARAMS
              SEIScalableNesting *scalableBspNestingSei = xCreateBspNestingSEI(pcSlice, i, k, l);
#else
              SEIScalableNesting *scalableBspNestingSei = xCreateBspNestingSEI(pcSlice);
#endif
              m_seiWriter.writeSEImessage(nalu.m_Bitstream, *scalableBspNestingSei, m_pcEncTop->getVPS(), pcSlice->getSPS());
              writeRBSPTrailingBits(nalu.m_Bitstream);

              UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
              UInt offsetPosition = m_activeParameterSetSEIPresentInAU 
                + m_bufferingPeriodSEIPresentInAU 
                + m_pictureTimingSEIPresentInAU
                + m_nestedPictureTimingSEIPresentInAU;  // Insert SEI after APS, BP and PT SEI
              AccessUnit::iterator it;
              for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
              {
                it++;
              }
              accessUnit.insert(it, new NALUnitEBSP(nalu));
#if VPS_VUI_BSP_HRD_PARAMS
            }
          }
        }
#endif
      }
#endif

      m_bSeqFirst = false;
    }
#if Q0048_CGS_3D_ASYMLUT
    else if( m_pcCfg->getCGSFlag() && pcSlice->getLayerId() && pcSlice->getCGSOverWritePPS() )
    {
#if SVC_EXTENSION
      OutputNALUnit nalu(NAL_UNIT_PPS, 0, m_layerId);
#endif

      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
      m_pcEntropyCoder->encodePPS(pcSlice->getPPS() , &m_Enc3DAsymLUTPPS );
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
    }
#endif

    if (writeSOP) // write SOP description SEI (if enabled) at the beginning of GOP
    {
      Int SOPcurrPOC = pocCurr;

      OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
      m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

      SEISOPDescription SOPDescriptionSEI;
      SOPDescriptionSEI.m_sopSeqParameterSetId = pcSlice->getSPS()->getSPSId();

      UInt i = 0;
      UInt prevEntryId = iGOPid;
      for (j = iGOPid; j < m_iGopSize; j++)
      {
        Int deltaPOC = m_pcCfg->getGOPEntry(j).m_POC - m_pcCfg->getGOPEntry(prevEntryId).m_POC;
        if ((SOPcurrPOC + deltaPOC) < m_pcCfg->getFramesToBeEncoded())
        {
          SOPcurrPOC += deltaPOC;
          SOPDescriptionSEI.m_sopDescVclNaluType[i] = getNalUnitType(SOPcurrPOC, m_iLastIDR, isField);
          SOPDescriptionSEI.m_sopDescTemporalId[i] = m_pcCfg->getGOPEntry(j).m_temporalId;
          SOPDescriptionSEI.m_sopDescStRpsIdx[i] = m_pcEncTop->getReferencePictureSetIdxForSOP(pcSlice, SOPcurrPOC, j);
          SOPDescriptionSEI.m_sopDescPocDelta[i] = deltaPOC;

          prevEntryId = j;
          i++;
        }
      }

      SOPDescriptionSEI.m_numPicsInSopMinus1 = i - 1;

#if O0164_MULTI_LAYER_HRD
      m_seiWriter.writeSEImessage( nalu.m_Bitstream, SOPDescriptionSEI, m_pcEncTop->getVPS(), pcSlice->getSPS());
#else
      m_seiWriter.writeSEImessage( nalu.m_Bitstream, SOPDescriptionSEI, pcSlice->getSPS());
#endif
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));

      writeSOP = false;
    }
#if Q0189_TMVP_CONSTRAINTS
   if( m_pcEncTop->getTMVPConstraintsSEIEnabled() == 1 &&
      (m_pcEncTop->getTMVPModeId() == 1 || m_pcEncTop->getTMVPModeId() == 2) &&
      pcSlice->getLayerId() >0 && 
      (pcSlice->getNalUnitType() ==  NAL_UNIT_CODED_SLICE_IDR_W_RADL || pcSlice->getNalUnitType() ==  NAL_UNIT_CODED_SLICE_IDR_N_LP))
   {
      OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
      SEITMVPConstrains seiTMVPConstrains;
      m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
      seiTMVPConstrains.no_intra_layer_col_pic_flag = 1;
      seiTMVPConstrains.prev_pics_not_used_flag = 1;
#if   O0164_MULTI_LAYER_HRD
      m_seiWriter.writeSEImessage( nalu.m_Bitstream, seiTMVPConstrains, m_pcEncTop->getVPS(), pcSlice->getSPS() );
#else
      m_seiWriter.writeSEImessage( nalu.m_Bitstream, seiTMVPConstrains, pcSlice->getSPS() );
#endif
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
   }
#endif
#if Q0247_FRAME_FIELD_INFO
    if(  pcSlice->getLayerId()> 0 &&
     ( (m_pcCfg->getProgressiveSourceFlag() && m_pcCfg->getInterlacedSourceFlag()) || m_pcCfg->getFrameFieldInfoPresentFlag()))
    {
      OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
      SEIFrameFieldInfo seiFFInfo;
      m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
      seiFFInfo.m_ffinfo_picStruct = (isField && pcSlice->getPic()->isTopField())? 1 : isField? 2 : 0;
#if   O0164_MULTI_LAYER_HRD
      m_seiWriter.writeSEImessage( nalu.m_Bitstream, seiFFInfo, m_pcEncTop->getVPS(), pcSlice->getSPS() );
#else
      m_seiWriter.writeSEImessage( nalu.m_Bitstream, seiFFInfo, pcSlice->getSPS() );
#endif
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
    }
#endif

    if( ( m_pcCfg->getPictureTimingSEIEnabled() || m_pcCfg->getDecodingUnitInfoSEIEnabled() ) &&
        ( pcSlice->getSPS()->getVuiParametersPresentFlag() ) &&
        ( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() ) 
       || ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) )
    {
      if( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getSubPicCpbParamsPresentFlag() )
      {
        UInt numDU = pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNumDU();
        pictureTimingSEI.m_numDecodingUnitsMinus1     = ( numDU - 1 );
        pictureTimingSEI.m_duCommonCpbRemovalDelayFlag = false;

        if( pictureTimingSEI.m_numNalusInDuMinus1 == NULL )
        {
          pictureTimingSEI.m_numNalusInDuMinus1       = new UInt[ numDU ];
        }
        if( pictureTimingSEI.m_duCpbRemovalDelayMinus1  == NULL )
        {
          pictureTimingSEI.m_duCpbRemovalDelayMinus1  = new UInt[ numDU ];
        }
        if( accumBitsDU == NULL )
        {
          accumBitsDU                                  = new UInt[ numDU ];
        }
        if( accumNalsDU == NULL )
        {
          accumNalsDU                                  = new UInt[ numDU ];
        }
      }
      pictureTimingSEI.m_auCpbRemovalDelay = std::min<Int>(std::max<Int>(1, m_totalCoded - m_lastBPSEI), static_cast<Int>(pow(2, static_cast<double>(pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getCpbRemovalDelayLengthMinus1()+1)))); // Syntax element signalled as minus, hence the .
#if POC_RESET_FLAG || POC_RESET_IDC_ENCODER
      pictureTimingSEI.m_picDpbOutputDelay = pcSlice->getSPS()->getNumReorderPics(pcSlice->getSPS()->getMaxTLayers()-1) + pocCurr - m_totalCoded;
#else
      pictureTimingSEI.m_picDpbOutputDelay = pcSlice->getSPS()->getNumReorderPics(pcSlice->getSPS()->getMaxTLayers()-1) + pcSlice->getPOC() - m_totalCoded;
#endif
#if EFFICIENT_FIELD_IRAP
      if(IRAPGOPid > 0 && IRAPGOPid < m_iGopSize)
      {
        // if pictures have been swapped there is likely one more picture delay on their tid. Very rough approximation
        pictureTimingSEI.m_picDpbOutputDelay ++;
      }
#endif
      Int factor = pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getTickDivisorMinus2() + 2;
      pictureTimingSEI.m_picDpbOutputDuDelay = factor * pictureTimingSEI.m_picDpbOutputDelay;
      if( m_pcCfg->getDecodingUnitInfoSEIEnabled() )
      {
        picSptDpbOutputDuDelay = factor * pictureTimingSEI.m_picDpbOutputDelay;
      }
    }

    if( ( m_pcCfg->getBufferingPeriodSEIEnabled() ) && ( pcSlice->getSliceType() == I_SLICE ) &&
        ( pcSlice->getSPS()->getVuiParametersPresentFlag() ) && 
        ( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() ) 
       || ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) )
    {
      OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
      m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

      SEIBufferingPeriod sei_buffering_period;
      
      UInt uiInitialCpbRemovalDelay = (90000/2);                      // 0.5 sec
      sei_buffering_period.m_initialCpbRemovalDelay      [0][0]     = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialCpbRemovalDelayOffset[0][0]     = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialCpbRemovalDelay      [0][1]     = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialCpbRemovalDelayOffset[0][1]     = uiInitialCpbRemovalDelay;

      Double dTmp = (Double)pcSlice->getSPS()->getVuiParameters()->getTimingInfo()->getNumUnitsInTick() / (Double)pcSlice->getSPS()->getVuiParameters()->getTimingInfo()->getTimeScale();

      UInt uiTmp = (UInt)( dTmp * 90000.0 ); 
      uiInitialCpbRemovalDelay -= uiTmp;
      uiInitialCpbRemovalDelay -= uiTmp / ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getTickDivisorMinus2() + 2 );
      sei_buffering_period.m_initialAltCpbRemovalDelay      [0][0]  = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialAltCpbRemovalDelayOffset[0][0]  = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialAltCpbRemovalDelay      [0][1]  = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialAltCpbRemovalDelayOffset[0][1]  = uiInitialCpbRemovalDelay;

      sei_buffering_period.m_rapCpbParamsPresentFlag              = 0;
      //for the concatenation, it can be set to one during splicing.
      sei_buffering_period.m_concatenationFlag = 0;
      //since the temporal layer HRD is not ready, we assumed it is fixed
      sei_buffering_period.m_auCpbRemovalDelayDelta = 1;
      sei_buffering_period.m_cpbDelayOffset = 0;
      sei_buffering_period.m_dpbDelayOffset = 0;

#if O0164_MULTI_LAYER_HRD
      m_seiWriter.writeSEImessage( nalu.m_Bitstream, sei_buffering_period, m_pcEncTop->getVPS(), pcSlice->getSPS());
#else
      m_seiWriter.writeSEImessage( nalu.m_Bitstream, sei_buffering_period, pcSlice->getSPS());
#endif
      writeRBSPTrailingBits(nalu.m_Bitstream);
      {
      UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
      UInt offsetPosition = m_activeParameterSetSEIPresentInAU;   // Insert BP SEI after APS SEI
      AccessUnit::iterator it;
      for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
      {
        it++;
      }
      accessUnit.insert(it, new NALUnitEBSP(nalu));
      m_bufferingPeriodSEIPresentInAU = true;
      }

      if (m_pcCfg->getScalableNestingSEIEnabled())
      {
        OutputNALUnit naluTmp(NAL_UNIT_PREFIX_SEI);
        m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
        m_pcEntropyCoder->setBitstream(&naluTmp.m_Bitstream);
        scalableNestingSEI.m_nestedSEIs.clear();
        scalableNestingSEI.m_nestedSEIs.push_back(&sei_buffering_period);
#if O0164_MULTI_LAYER_HRD
        m_seiWriter.writeSEImessage( naluTmp.m_Bitstream, scalableNestingSEI, m_pcEncTop->getVPS(), pcSlice->getSPS());
#else
        m_seiWriter.writeSEImessage( naluTmp.m_Bitstream, scalableNestingSEI, pcSlice->getSPS());
#endif
        writeRBSPTrailingBits(naluTmp.m_Bitstream);
        UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
        UInt offsetPosition = m_activeParameterSetSEIPresentInAU + m_bufferingPeriodSEIPresentInAU + m_pictureTimingSEIPresentInAU;   // Insert BP SEI after non-nested APS, BP and PT SEIs
        AccessUnit::iterator it;
        for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
        {
          it++;
        }
        accessUnit.insert(it, new NALUnitEBSP(naluTmp));
        m_nestedBufferingPeriodSEIPresentInAU = true;
      }

      m_lastBPSEI = m_totalCoded;
      m_cpbRemovalDelay = 0;
    }
    m_cpbRemovalDelay ++;
    if( ( m_pcEncTop->getRecoveryPointSEIEnabled() ) && ( pcSlice->getSliceType() == I_SLICE ) )
    {
      if( m_pcEncTop->getGradualDecodingRefreshInfoEnabled() && !pcSlice->getRapPicFlag() )
      {
        // Gradual decoding refresh SEI 
        OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
        m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
        m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

        SEIGradualDecodingRefreshInfo seiGradualDecodingRefreshInfo;
        seiGradualDecodingRefreshInfo.m_gdrForegroundFlag = true; // Indicating all "foreground"

#if O0164_MULTI_LAYER_HRD
        m_seiWriter.writeSEImessage( nalu.m_Bitstream, seiGradualDecodingRefreshInfo, m_pcEncTop->getVPS(), pcSlice->getSPS() );
#else
        m_seiWriter.writeSEImessage( nalu.m_Bitstream, seiGradualDecodingRefreshInfo, pcSlice->getSPS() );
#endif
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
      }
    // Recovery point SEI
      OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
      m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

      SEIRecoveryPoint sei_recovery_point;
      sei_recovery_point.m_recoveryPocCnt    = 0;
#if POC_RESET_FLAG || POC_RESET_IDC_ENCODER
      sei_recovery_point.m_exactMatchingFlag = ( pocCurr == 0 ) ? (true) : (false);
#else
      sei_recovery_point.m_exactMatchingFlag = ( pcSlice->getPOC() == 0 ) ? (true) : (false);
#endif
      sei_recovery_point.m_brokenLinkFlag    = false;
#if ALLOW_RECOVERY_POINT_AS_RAP
      if(m_pcCfg->getDecodingRefreshType() == 3)
      {
        m_iLastRecoveryPicPOC = pocCurr;
      }
#endif

#if O0164_MULTI_LAYER_HRD
      m_seiWriter.writeSEImessage( nalu.m_Bitstream, sei_recovery_point, m_pcEncTop->getVPS(), pcSlice->getSPS() );
#else
      m_seiWriter.writeSEImessage( nalu.m_Bitstream, sei_recovery_point, pcSlice->getSPS() );
#endif
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
    }

    /* use the main bitstream buffer for storing the marshalled picture */
    m_pcEntropyCoder->setBitstream(NULL);

    startCUAddrSliceIdx = 0;
    startCUAddrSlice    = 0; 

    startCUAddrSliceSegmentIdx = 0;
    startCUAddrSliceSegment    = 0; 
    nextCUAddr                 = 0;
    pcSlice = pcPic->getSlice(startCUAddrSliceIdx);

    Int processingState = (pcSlice->getSPS()->getUseSAO())?(EXECUTE_INLOOPFILTER):(ENCODE_SLICE);
    Bool skippedSlice=false;
    while (nextCUAddr < uiRealEndAddress) // Iterate over all slices
    {
      switch(processingState)
      {
      case ENCODE_SLICE:
        {
          pcSlice->setNextSlice       ( false );
          pcSlice->setNextSliceSegment( false );
          if (nextCUAddr == m_storedStartCUAddrForEncodingSlice[startCUAddrSliceIdx])
          {
            pcSlice = pcPic->getSlice(startCUAddrSliceIdx);
            if(startCUAddrSliceIdx > 0 && pcSlice->getSliceType()!= I_SLICE)
            {
              pcSlice->checkColRefIdx(startCUAddrSliceIdx, pcPic);
            }
            pcPic->setCurrSliceIdx(startCUAddrSliceIdx);
            m_pcSliceEncoder->setSliceIdx(startCUAddrSliceIdx);
            assert(startCUAddrSliceIdx == pcSlice->getSliceIdx());
            // Reconstruction slice
            pcSlice->setSliceCurStartCUAddr( nextCUAddr );  // to be used in encodeSlice() + context restriction
            pcSlice->setSliceCurEndCUAddr  ( m_storedStartCUAddrForEncodingSlice[startCUAddrSliceIdx+1 ] );
            // Dependent slice
            pcSlice->setSliceSegmentCurStartCUAddr( nextCUAddr );  // to be used in encodeSlice() + context restriction
            pcSlice->setSliceSegmentCurEndCUAddr  ( m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx+1 ] );

            pcSlice->setNextSlice       ( true );

            startCUAddrSliceIdx++;
            startCUAddrSliceSegmentIdx++;
          } 
          else if (nextCUAddr == m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx])
          {
            // Dependent slice
            pcSlice->setSliceSegmentCurStartCUAddr( nextCUAddr );  // to be used in encodeSlice() + context restriction
            pcSlice->setSliceSegmentCurEndCUAddr  ( m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx+1 ] );

            pcSlice->setNextSliceSegment( true );

            startCUAddrSliceSegmentIdx++;
          }
#if SVC_EXTENSION
          pcSlice->setNumMotionPredRefLayers(m_pcEncTop->getNumMotionPredRefLayers());
#endif
          pcSlice->setRPS(pcPic->getSlice(0)->getRPS());
          pcSlice->setRPSidx(pcPic->getSlice(0)->getRPSidx());
          UInt uiDummyStartCUAddr;
          UInt uiDummyBoundingCUAddr;
          m_pcSliceEncoder->xDetermineStartAndBoundingCUAddr(uiDummyStartCUAddr,uiDummyBoundingCUAddr,pcPic,true);

          uiInternalAddress = pcPic->getPicSym()->getPicSCUAddr(pcSlice->getSliceSegmentCurEndCUAddr()-1) % pcPic->getNumPartInCU();
          uiExternalAddress = pcPic->getPicSym()->getPicSCUAddr(pcSlice->getSliceSegmentCurEndCUAddr()-1) / pcPic->getNumPartInCU();
          uiPosX = ( uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
          uiPosY = ( uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];

#if REPN_FORMAT_IN_VPS
          uiWidth = pcSlice->getPicWidthInLumaSamples();
          uiHeight = pcSlice->getPicHeightInLumaSamples();
#else
          uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
          uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();
#endif
          while(uiPosX>=uiWidth||uiPosY>=uiHeight)
          {
            uiInternalAddress--;
            uiPosX = ( uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
            uiPosY = ( uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
          }
          uiInternalAddress++;
          if(uiInternalAddress==pcPic->getNumPartInCU())
          {
            uiInternalAddress = 0;
            uiExternalAddress = pcPic->getPicSym()->getCUOrderMap(pcPic->getPicSym()->getInverseCUOrderMap(uiExternalAddress)+1);
          }
          UInt endAddress = pcPic->getPicSym()->getPicSCUEncOrder(uiExternalAddress*pcPic->getNumPartInCU()+uiInternalAddress);
          if(endAddress<=pcSlice->getSliceSegmentCurStartCUAddr()) 
          {
            UInt boundingAddrSlice, boundingAddrSliceSegment;
            boundingAddrSlice          = m_storedStartCUAddrForEncodingSlice[startCUAddrSliceIdx];          
            boundingAddrSliceSegment = m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx];          
            nextCUAddr               = min(boundingAddrSlice, boundingAddrSliceSegment);
            if(pcSlice->isNextSlice())
            {
              skippedSlice=true;
            }
            continue;
          }
          if(skippedSlice) 
          {
            pcSlice->setNextSlice       ( true );
            pcSlice->setNextSliceSegment( false );
          }
          skippedSlice=false;
          pcSlice->allocSubstreamSizes( iNumSubstreams );
          for ( UInt ui = 0 ; ui < iNumSubstreams; ui++ )
          {
            pcSubstreamsOut[ui].clear();
          }

          m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );
          m_pcEntropyCoder->resetEntropy      ();
          /* start slice NALunit */
#if SVC_EXTENSION
          OutputNALUnit nalu( pcSlice->getNalUnitType(), pcSlice->getTLayer(), m_layerId );
#else
          OutputNALUnit nalu( pcSlice->getNalUnitType(), pcSlice->getTLayer() );
#endif
          Bool sliceSegment = (!pcSlice->isNextSlice());
          if (!sliceSegment)
          {
            uiOneBitstreamPerSliceLength = 0; // start of a new slice
          }
          m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

#if SETTING_NO_OUT_PIC_PRIOR
          pcSlice->setNoRaslOutputFlag(false);
          if (pcSlice->isIRAP())
          {
            if (pcSlice->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && pcSlice->getNalUnitType() <= NAL_UNIT_CODED_SLICE_IDR_N_LP)
            {
              pcSlice->setNoRaslOutputFlag(true);
            }
            //the inference for NoOutputPriorPicsFlag
            // KJS: This cannot happen at the encoder
            if (!m_bFirst && pcSlice->isIRAP() && pcSlice->getNoRaslOutputFlag())
            {
              if (pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA)
              {
                pcSlice->setNoOutputPriorPicsFlag(true);
              }
            }
          }
#endif

          tmpBitsBeforeWriting = m_pcEntropyCoder->getNumberOfWrittenBits();
          m_pcEntropyCoder->encodeSliceHeader(pcSlice);
          actualHeadBits += ( m_pcEntropyCoder->getNumberOfWrittenBits() - tmpBitsBeforeWriting );

          // is it needed?
          {
            if (!sliceSegment)
            {
              pcBitstreamRedirect->writeAlignOne();
            }
            else
            {
              // We've not completed our slice header info yet, do the alignment later.
            }
            m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
            m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcSlice );
            m_pcEntropyCoder->resetEntropy    ();
#if WPP_FIX
            for ( UInt ui = 0 ; ui < iNumSubstreams ; ui++ ) 
#else
            for ( UInt ui = 0 ; ui < pcSlice->getPPS()->getNumSubstreams() ; ui++ )
#endif
            {
              m_pcEntropyCoder->setEntropyCoder ( &pcSbacCoders[ui], pcSlice );
              m_pcEntropyCoder->resetEntropy    ();
            }
          }

          if(pcSlice->isNextSlice())
          {
            // set entropy coder for writing
            m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
            {
#if WPP_FIX
              for ( UInt ui = 0 ; ui < iNumSubstreams ; ui++ )
#else
              for ( UInt ui = 0 ; ui < pcSlice->getPPS()->getNumSubstreams() ; ui++ )
#endif
              {
                m_pcEntropyCoder->setEntropyCoder ( &pcSbacCoders[ui], pcSlice );
                m_pcEntropyCoder->resetEntropy    ();
              }
              pcSbacCoders[0].load(m_pcSbacCoder);
              m_pcEntropyCoder->setEntropyCoder ( &pcSbacCoders[0], pcSlice );  //ALF is written in substream #0 with CABAC coder #0 (see ALF param encoding below)
            }
            m_pcEntropyCoder->resetEntropy    ();
            // File writing
            if (!sliceSegment)
            {
              m_pcEntropyCoder->setBitstream(pcBitstreamRedirect);
            }
            else
            {
              m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
            }
            // for now, override the TILES_DECODER setting in order to write substreams.
            m_pcEntropyCoder->setBitstream    ( &pcSubstreamsOut[0] );

          }
          pcSlice->setFinalized(true);

          m_pcSbacCoder->load( &pcSbacCoders[0] );

          pcSlice->setTileOffstForMultES( uiOneBitstreamPerSliceLength );
          pcSlice->setTileLocationCount ( 0 );
          m_pcSliceEncoder->encodeSlice(pcPic, pcSubstreamsOut);

          {
            // Construct the final bitstream by flushing and concatenating substreams.
            // The final bitstream is either nalu.m_Bitstream or pcBitstreamRedirect;
            UInt* puiSubstreamSizes = pcSlice->getSubstreamSizes();
            UInt uiTotalCodedSize = 0; // for padding calcs.
            UInt uiNumSubstreamsPerTile = iNumSubstreams;
            if (iNumSubstreams > 1)
            {
              uiNumSubstreamsPerTile /= pcPic->getPicSym()->getNumTiles();
            }
            for ( UInt ui = 0 ; ui < iNumSubstreams; ui++ )
            {
              // Flush all substreams -- this includes empty ones.
              // Terminating bit and flush.
              m_pcEntropyCoder->setEntropyCoder   ( &pcSbacCoders[ui], pcSlice );
              m_pcEntropyCoder->setBitstream      (  &pcSubstreamsOut[ui] );
              m_pcEntropyCoder->encodeTerminatingBit( 1 );
              m_pcEntropyCoder->encodeSliceFinish();

              pcSubstreamsOut[ui].writeByteAlignment();   // Byte-alignment in slice_data() at end of sub-stream
              // Byte alignment is necessary between tiles when tiles are independent.
              uiTotalCodedSize += pcSubstreamsOut[ui].getNumberOfWrittenBits();

              Bool bNextSubstreamInNewTile = ((ui+1) < iNumSubstreams)&& ((ui+1)%uiNumSubstreamsPerTile == 0);
#if WPP_FIX
              if (bNextSubstreamInNewTile &&  !pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag() ) 
#else
              if (bNextSubstreamInNewTile)
#endif
              {
                pcSlice->setTileLocation(ui/uiNumSubstreamsPerTile, pcSlice->getTileOffstForMultES()+(uiTotalCodedSize>>3));
              }
#if WPP_FIX
              if (ui+1 < iNumSubstreams)
              {
                puiSubstreamSizes[ui] = pcSubstreamsOut[ui].getNumberOfWrittenBits() + (pcSubstreamsOut[ui].countStartCodeEmulations()<<3);
              }
#else
              if (ui+1 < pcSlice->getPPS()->getNumSubstreams())
              {
                puiSubstreamSizes[ui] = pcSubstreamsOut[ui].getNumberOfWrittenBits() + (pcSubstreamsOut[ui].countStartCodeEmulations()<<3);
              }
#endif
            }

            // Complete the slice header info.
            m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );
            m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if !POC_RESET_IDC_SIGNALLING
            m_pcEntropyCoder->encodeTilesWPPEntryPoint( pcSlice );
#else
            tmpBitsBeforeWriting = m_pcEntropyCoder->getNumberOfWrittenBits();
            m_pcEntropyCoder->encodeTilesWPPEntryPoint( pcSlice );
            actualHeadBits += ( m_pcEntropyCoder->getNumberOfWrittenBits() - tmpBitsBeforeWriting );
            m_pcEntropyCoder->encodeSliceHeaderExtn( pcSlice, actualHeadBits );
#endif

            // Substreams...
            TComOutputBitstream *pcOut = pcBitstreamRedirect;
#if WPP_FIX 
            Int numZeroSubstreamsAtStartOfSlice = 0; 
            Int numSubstreamsToCode = pcSlice->getPPS()->getNumSubstreams(); 
            if (pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag()) 
            { 
              Int  maxNumParts                      = pcPic->getNumPartInCU(); 
              numZeroSubstreamsAtStartOfSlice  = pcPic->getSubstreamForLCUAddr(pcSlice->getSliceSegmentCurStartCUAddr()/maxNumParts, false, pcSlice); 
              // 1st line present for WPP. 
              numSubstreamsToCode  = pcSlice->getNumEntryPointOffsets()+1; 
            } 
            for ( UInt ui = 0 ; ui < numSubstreamsToCode; ui++ ) 
            { 
              pcOut->addSubstream(&pcSubstreamsOut[ui+numZeroSubstreamsAtStartOfSlice]); 
            } 
#else 
            Int offs = 0;
          Int nss = pcSlice->getPPS()->getNumSubstreams();
          if (pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag())
          {
            // 1st line present for WPP.
            offs = pcSlice->getSliceSegmentCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU()/pcSlice->getPic()->getFrameWidthInCU();
            nss  = pcSlice->getNumEntryPointOffsets()+1;
          }
          for ( UInt ui = 0 ; ui < nss; ui++ )
          {
            pcOut->addSubstream(&pcSubstreamsOut[ui+offs]);
            }
#endif
          }

          UInt boundingAddrSlice, boundingAddrSliceSegment;
          boundingAddrSlice        = m_storedStartCUAddrForEncodingSlice[startCUAddrSliceIdx];          
          boundingAddrSliceSegment = m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx];          
          nextCUAddr               = min(boundingAddrSlice, boundingAddrSliceSegment);
          // If current NALU is the first NALU of slice (containing slice header) and more NALUs exist (due to multiple dependent slices) then buffer it.
          // If current NALU is the last NALU of slice and a NALU was buffered, then (a) Write current NALU (b) Update an write buffered NALU at approproate location in NALU list.
          Bool bNALUAlignedWrittenToList    = false; // used to ensure current NALU is not written more than once to the NALU list.
          xAttachSliceDataToNalUnit(nalu, pcBitstreamRedirect);
          accessUnit.push_back(new NALUnitEBSP(nalu));
          actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;
          bNALUAlignedWrittenToList = true; 
          uiOneBitstreamPerSliceLength += nalu.m_Bitstream.getNumberOfWrittenBits(); // length of bitstream after byte-alignment

          if (!bNALUAlignedWrittenToList)
          {
            {
              nalu.m_Bitstream.writeAlignZero();
            }
            accessUnit.push_back(new NALUnitEBSP(nalu));
            uiOneBitstreamPerSliceLength += nalu.m_Bitstream.getNumberOfWrittenBits() + 24; // length of bitstream after byte-alignment + 3 byte startcode 0x000001
          }

          if( ( m_pcCfg->getPictureTimingSEIEnabled() || m_pcCfg->getDecodingUnitInfoSEIEnabled() ) &&
              ( pcSlice->getSPS()->getVuiParametersPresentFlag() ) &&
              ( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() ) 
             || ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) &&
              ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getSubPicCpbParamsPresentFlag() ) )
          {
              UInt numNalus = 0;
            UInt numRBSPBytes = 0;
            for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
            {
              UInt numRBSPBytes_nal = UInt((*it)->m_nalUnitData.str().size());
              if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
              {
                numRBSPBytes += numRBSPBytes_nal;
                numNalus ++;
              }
            }
            accumBitsDU[ pcSlice->getSliceIdx() ] = ( numRBSPBytes << 3 );
            accumNalsDU[ pcSlice->getSliceIdx() ] = numNalus;   // SEI not counted for bit count; hence shouldn't be counted for # of NALUs - only for consistency
          }
          processingState = ENCODE_SLICE;
          }
          break;
        case EXECUTE_INLOOPFILTER:
          {
            // set entropy coder for RD
            m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcSlice );
#if HIGHER_LAYER_IRAP_SKIP_FLAG
            if ( pcSlice->getSPS()->getUseSAO() && !( m_pcEncTop->getSkipPictureAtArcSwitch() && m_pcEncTop->getAdaptiveResolutionChange() > 0 && pcSlice->getLayerId() == 1 && pcSlice->getPOC() == m_pcEncTop->getAdaptiveResolutionChange()) )
#else
            if ( pcSlice->getSPS()->getUseSAO() )
#endif
            {
              m_pcEntropyCoder->resetEntropy();
              m_pcEntropyCoder->setBitstream( m_pcBitCounter );
              Bool sliceEnabled[NUM_SAO_COMPONENTS];
              m_pcSAO->initRDOCabacCoder(m_pcEncTop->getRDGoOnSbacCoder(), pcSlice);
              m_pcSAO->SAOProcess(pcPic
                , sliceEnabled
                , pcPic->getSlice(0)->getLambdas()
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
                , m_pcCfg->getSaoLcuBoundary()
#endif
              );
              m_pcSAO->PCMLFDisableProcess(pcPic);   

              //assign SAO slice header
              for(Int s=0; s< uiNumSlices; s++)
              {
                pcPic->getSlice(s)->setSaoEnabledFlag(sliceEnabled[SAO_Y]);
                assert(sliceEnabled[SAO_Cb] == sliceEnabled[SAO_Cr]);
                pcPic->getSlice(s)->setSaoEnabledFlagChroma(sliceEnabled[SAO_Cb]);
              }
            }
            processingState = ENCODE_SLICE;
          }
          break;
        default:
          {
            printf("Not a supported encoding state\n");
            assert(0);
            exit(-1);
          }
        }
      } // end iteration over slices
      pcPic->compressMotion(); 
      
      //-- For time output for each slice
      Double dEncTime = (Double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;

      const Char* digestStr = NULL;
      if (m_pcCfg->getDecodedPictureHashSEIEnabled())
      {
        /* calculate MD5sum for entire reconstructed picture */
        SEIDecodedPictureHash sei_recon_picture_digest;
        if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 1)
        {
          sei_recon_picture_digest.method = SEIDecodedPictureHash::MD5;
          calcMD5(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
          digestStr = digestToString(sei_recon_picture_digest.digest, 16);
        }
        else if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 2)
        {
          sei_recon_picture_digest.method = SEIDecodedPictureHash::CRC;
          calcCRC(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
          digestStr = digestToString(sei_recon_picture_digest.digest, 2);
        }
        else if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 3)
        {
          sei_recon_picture_digest.method = SEIDecodedPictureHash::CHECKSUM;
          calcChecksum(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
          digestStr = digestToString(sei_recon_picture_digest.digest, 4);
        }
#if SVC_EXTENSION
        OutputNALUnit nalu(NAL_UNIT_SUFFIX_SEI, pcSlice->getTLayer(), m_layerId);
#else
        OutputNALUnit nalu(NAL_UNIT_SUFFIX_SEI, pcSlice->getTLayer());
#endif

        /* write the SEI messages */
        m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
#if O0164_MULTI_LAYER_HRD
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei_recon_picture_digest, m_pcEncTop->getVPS(), pcSlice->getSPS());
#else
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei_recon_picture_digest, pcSlice->getSPS());
#endif
        writeRBSPTrailingBits(nalu.m_Bitstream);

        accessUnit.insert(accessUnit.end(), new NALUnitEBSP(nalu));
      }
      if (m_pcCfg->getTemporalLevel0IndexSEIEnabled())
      {
        SEITemporalLevel0Index sei_temporal_level0_index;
        if (pcSlice->getRapPicFlag())
        {
          m_tl0Idx = 0;
          m_rapIdx = (m_rapIdx + 1) & 0xFF;
        }
        else
        {
          m_tl0Idx = (m_tl0Idx + (pcSlice->getTLayer() ? 0 : 1)) & 0xFF;
        }
        sei_temporal_level0_index.tl0Idx = m_tl0Idx;
        sei_temporal_level0_index.rapIdx = m_rapIdx;

        OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI); 

        /* write the SEI messages */
        m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
#if O0164_MULTI_LAYER_HRD
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei_temporal_level0_index, m_pcEncTop->getVPS(), pcSlice->getSPS());
#else
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei_temporal_level0_index, pcSlice->getSPS());
#endif
        writeRBSPTrailingBits(nalu.m_Bitstream);

        /* insert the SEI message NALUnit before any Slice NALUnits */
        AccessUnit::iterator it = find_if(accessUnit.begin(), accessUnit.end(), mem_fun(&NALUnit::isSlice));
        accessUnit.insert(it, new NALUnitEBSP(nalu));
      }

      xCalculateAddPSNR( pcPic, pcPic->getPicYuvRec(), accessUnit, dEncTime );

      //In case of field coding, compute the interlaced PSNR for both fields
    if (isField && ((!pcPic->isTopField() && isTff) || (pcPic->isTopField() && !isTff)) && (pcPic->getPOC()%m_iGopSize != 1))
      {
        //get complementary top field
        TComPic* pcPicTop;
        TComList<TComPic*>::iterator   iterPic = rcListPic.begin();
        while ((*iterPic)->getPOC() != pcPic->getPOC()-1)
        {
          iterPic ++;
        }
        pcPicTop = *(iterPic);
        xCalculateInterlacedAddPSNR(pcPicTop, pcPic, pcPicTop->getPicYuvRec(), pcPic->getPicYuvRec(), accessUnit, dEncTime );
      }
    else if (isField && pcPic->getPOC()!= 0 && (pcPic->getPOC()%m_iGopSize == 0))
    {
      //get complementary bottom field
      TComPic* pcPicBottom;
      TComList<TComPic*>::iterator   iterPic = rcListPic.begin();
      while ((*iterPic)->getPOC() != pcPic->getPOC()+1)
      {
        iterPic ++;
      }
      pcPicBottom = *(iterPic);
      xCalculateInterlacedAddPSNR(pcPic, pcPicBottom, pcPic->getPicYuvRec(), pcPicBottom->getPicYuvRec(), accessUnit, dEncTime );
    }

      if (digestStr)
      {
        if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 1)
        {
          printf(" [MD5:%s]", digestStr);
        }
        else if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 2)
        {
          printf(" [CRC:%s]", digestStr);
        }
        else if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 3)
        {
          printf(" [Checksum:%s]", digestStr);
        }
      }
      if ( m_pcCfg->getUseRateCtrl() )
      {
        Double avgQP     = m_pcRateCtrl->getRCPic()->calAverageQP();
        Double avgLambda = m_pcRateCtrl->getRCPic()->calAverageLambda();
        if ( avgLambda < 0.0 )
        {
          avgLambda = lambda;
        }
        m_pcRateCtrl->getRCPic()->updateAfterPicture( actualHeadBits, actualTotalBits, avgQP, avgLambda, pcSlice->getSliceType());
        m_pcRateCtrl->getRCPic()->addToPictureLsit( m_pcRateCtrl->getPicList() );

        m_pcRateCtrl->getRCSeq()->updateAfterPic( actualTotalBits );
        if ( pcSlice->getSliceType() != I_SLICE )
        {
          m_pcRateCtrl->getRCGOP()->updateAfterPicture( actualTotalBits );
        }
        else    // for intra picture, the estimated bits are used to update the current status in the GOP
        {
          m_pcRateCtrl->getRCGOP()->updateAfterPicture( estimatedBits );
        }
      }

      if( ( m_pcCfg->getPictureTimingSEIEnabled() || m_pcCfg->getDecodingUnitInfoSEIEnabled() ) &&
          ( pcSlice->getSPS()->getVuiParametersPresentFlag() ) &&
          ( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() ) 
         || ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) )
      {
        TComVUI *vui = pcSlice->getSPS()->getVuiParameters();
        TComHRD *hrd = vui->getHrdParameters();

        if( hrd->getSubPicCpbParamsPresentFlag() )
        {
          Int i;
          UInt64 ui64Tmp;
          UInt uiPrev = 0;
          UInt numDU = ( pictureTimingSEI.m_numDecodingUnitsMinus1 + 1 );
          UInt *pCRD = &pictureTimingSEI.m_duCpbRemovalDelayMinus1[0];
          UInt maxDiff = ( hrd->getTickDivisorMinus2() + 2 ) - 1;

          for( i = 0; i < numDU; i ++ )
          {
            pictureTimingSEI.m_numNalusInDuMinus1[ i ]       = ( i == 0 ) ? ( accumNalsDU[ i ] - 1 ) : ( accumNalsDU[ i ] - accumNalsDU[ i - 1] - 1 );
          }

          if( numDU == 1 )
          {
            pCRD[ 0 ] = 0; /* don't care */
          }
          else
          {
            pCRD[ numDU - 1 ] = 0;/* by definition */
            UInt tmp = 0;
            UInt accum = 0;

            for( i = ( numDU - 2 ); i >= 0; i -- )
            {
              ui64Tmp = ( ( ( accumBitsDU[ numDU - 1 ]  - accumBitsDU[ i ] ) * ( vui->getTimingInfo()->getTimeScale() / vui->getTimingInfo()->getNumUnitsInTick() ) * ( hrd->getTickDivisorMinus2() + 2 ) ) / ( m_pcCfg->getTargetBitrate() ) );
              if( (UInt)ui64Tmp > maxDiff )
              {
                tmp ++;
              }
            }
            uiPrev = 0;

            UInt flag = 0;
            for( i = ( numDU - 2 ); i >= 0; i -- )
            {
              flag = 0;
              ui64Tmp = ( ( ( accumBitsDU[ numDU - 1 ]  - accumBitsDU[ i ] ) * ( vui->getTimingInfo()->getTimeScale() / vui->getTimingInfo()->getNumUnitsInTick() ) * ( hrd->getTickDivisorMinus2() + 2 ) ) / ( m_pcCfg->getTargetBitrate() ) );

              if( (UInt)ui64Tmp > maxDiff )
              {
                if(uiPrev >= maxDiff - tmp)
                {
                  ui64Tmp = uiPrev + 1;
                  flag = 1;
                }
                else                            ui64Tmp = maxDiff - tmp + 1;
              }
              pCRD[ i ] = (UInt)ui64Tmp - uiPrev - 1;
              if( (Int)pCRD[ i ] < 0 )
              {
                pCRD[ i ] = 0;
              }
              else if (tmp > 0 && flag == 1) 
              {
                tmp --;
              }
              accum += pCRD[ i ] + 1;
              uiPrev = accum;
            }
          }
        }
        if( m_pcCfg->getPictureTimingSEIEnabled() )
        {
          {
            OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI, pcSlice->getTLayer());
          m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
          pictureTimingSEI.m_picStruct = (isField && pcSlice->getPic()->isTopField())? 1 : isField? 2 : 0;
#if O0164_MULTI_LAYER_HRD
          m_seiWriter.writeSEImessage(nalu.m_Bitstream, pictureTimingSEI, m_pcEncTop->getVPS(), pcSlice->getSPS());
#else
          m_seiWriter.writeSEImessage(nalu.m_Bitstream, pictureTimingSEI, pcSlice->getSPS());
#endif
          writeRBSPTrailingBits(nalu.m_Bitstream);
          UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
          UInt offsetPosition = m_activeParameterSetSEIPresentInAU 
                                    + m_bufferingPeriodSEIPresentInAU;    // Insert PT SEI after APS and BP SEI
          AccessUnit::iterator it;
          for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
          {
            it++;
          }
          accessUnit.insert(it, new NALUnitEBSP(nalu));
          m_pictureTimingSEIPresentInAU = true;
        }
          if ( m_pcCfg->getScalableNestingSEIEnabled() ) // put picture timing SEI into scalable nesting SEI
          {
            OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI, pcSlice->getTLayer());
            m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
            scalableNestingSEI.m_nestedSEIs.clear();
            scalableNestingSEI.m_nestedSEIs.push_back(&pictureTimingSEI);
#if O0164_MULTI_LAYER_HRD
            m_seiWriter.writeSEImessage(nalu.m_Bitstream, scalableNestingSEI, m_pcEncTop->getVPS(), pcSlice->getSPS());
#else
            m_seiWriter.writeSEImessage(nalu.m_Bitstream, scalableNestingSEI, pcSlice->getSPS());
#endif
            writeRBSPTrailingBits(nalu.m_Bitstream);
            UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
            UInt offsetPosition = m_activeParameterSetSEIPresentInAU 
              + m_bufferingPeriodSEIPresentInAU + m_pictureTimingSEIPresentInAU + m_nestedBufferingPeriodSEIPresentInAU;    // Insert PT SEI after APS and BP SEI
            AccessUnit::iterator it;
            for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
            {
              it++;
            }
            accessUnit.insert(it, new NALUnitEBSP(nalu));
            m_nestedPictureTimingSEIPresentInAU = true;
          }
        }
        if( m_pcCfg->getDecodingUnitInfoSEIEnabled() && hrd->getSubPicCpbParamsPresentFlag() )
        {             
          m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
          for( Int i = 0; i < ( pictureTimingSEI.m_numDecodingUnitsMinus1 + 1 ); i ++ )
          {
            OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI, pcSlice->getTLayer());

            SEIDecodingUnitInfo tempSEI;
            tempSEI.m_decodingUnitIdx = i;
            tempSEI.m_duSptCpbRemovalDelay = pictureTimingSEI.m_duCpbRemovalDelayMinus1[i] + 1;
            tempSEI.m_dpbOutputDuDelayPresentFlag = false;
            tempSEI.m_picSptDpbOutputDuDelay = picSptDpbOutputDuDelay;

            AccessUnit::iterator it;
            // Insert the first one in the right location, before the first slice
            if(i == 0)
            {
              // Insert before the first slice. 
#if O0164_MULTI_LAYER_HRD
              m_seiWriter.writeSEImessage(nalu.m_Bitstream, tempSEI, m_pcEncTop->getVPS(), pcSlice->getSPS());
#else
              m_seiWriter.writeSEImessage(nalu.m_Bitstream, tempSEI, pcSlice->getSPS());
#endif
              writeRBSPTrailingBits(nalu.m_Bitstream);

              UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
              UInt offsetPosition = m_activeParameterSetSEIPresentInAU 
                                    + m_bufferingPeriodSEIPresentInAU 
                                    + m_pictureTimingSEIPresentInAU;  // Insert DU info SEI after APS, BP and PT SEI
              for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
              {
                it++;
              }
              accessUnit.insert(it, new NALUnitEBSP(nalu));
            }
            else
            {
              Int ctr;
              // For the second decoding unit onwards we know how many NALUs are present
              for (ctr = 0, it = accessUnit.begin(); it != accessUnit.end(); it++)
              {            
                if(ctr == accumNalsDU[ i - 1 ])
                {
                  // Insert before the first slice. 
#if O0164_MULTI_LAYER_HRD
                  m_seiWriter.writeSEImessage(nalu.m_Bitstream, tempSEI, m_pcEncTop->getVPS(), pcSlice->getSPS());
#else
                  m_seiWriter.writeSEImessage(nalu.m_Bitstream, tempSEI, pcSlice->getSPS());
#endif
                  writeRBSPTrailingBits(nalu.m_Bitstream);

                  accessUnit.insert(it, new NALUnitEBSP(nalu));
                  break;
                }
                if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
                {
                  ctr++;
                }
              }
            }            
          }
        }
      }
      xResetNonNestedSEIPresentFlags();
      xResetNestedSEIPresentFlags();
      pcPic->getPicYuvRec()->copyToPic(pcPicYuvRecOut);

#if M0040_ADAPTIVE_RESOLUTION_CHANGE
      pcPicYuvRecOut->setReconstructed(true);
#endif
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
      m_pcEncTop->setFirstPicInLayerDecodedFlag(true);
#endif
      pcPic->setReconMark   ( true );
      m_bFirst = false;
      m_iNumPicCoded++;
      m_totalCoded ++;
      /* logging: insert a newline at end of picture period */
      printf("\n");
      fflush(stdout);

      delete[] pcSubstreamsOut;

#if EFFICIENT_FIELD_IRAP
    if(IRAPtoReorder)
    {
      if(swapIRAPForward)
      {
        if(iGOPid == IRAPGOPid)
        {
          iGOPid = IRAPGOPid +1;
          IRAPtoReorder = false;
        }
        else if(iGOPid == IRAPGOPid +1)
        {
          iGOPid --;
        }
      }
      else
      {
        if(iGOPid == IRAPGOPid)
        {
          iGOPid = IRAPGOPid -1;
        }
        else if(iGOPid == IRAPGOPid -1)
        {
          iGOPid = IRAPGOPid;
          IRAPtoReorder = false;
        }
      }
    }
#endif
  }
  delete pcBitstreamRedirect;

  if( accumBitsDU != NULL) delete accumBitsDU;
  if( accumNalsDU != NULL) delete accumNalsDU;

#if SVC_EXTENSION
  assert ( m_iNumPicCoded <= 1 || (isField && iPOCLast == 1) );
#else
  assert ( (m_iNumPicCoded == iNumPicRcvd) || (isField && iPOCLast == 1) );
#endif
}

#if POC_RESET_IDC_ENCODER
Void TEncGOP::determinePocResetIdc(Int const pocCurr, TComSlice *const slice)
{
  // If one picture in the AU is IDR, and another picture is not IDR, set the poc_reset_idc to 1 or 2
  // If BL picture in the AU is IDR, and another picture is not IDR, set the poc_reset_idc to 2
  // If BL picture is IRAP, and another picture is non-IRAP, then the poc_reset_idc is equal to 1 or 2.
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  slice->setPocMsbNeeded(false);
#endif
  if( slice->getSliceIdx() == 0 ) // First slice - compute, copy for other slices
  {
    Int needReset = false;
    Int resetDueToBL = false;
    if( slice->getVPS()->getMaxLayers() > 1 )
    {
      // If IRAP is refreshed in this access unit for base layer
      if( (m_ppcTEncTop[0]->getGOPEncoder()->getIntraRefreshType() == 1 || m_ppcTEncTop[0]->getGOPEncoder()->getIntraRefreshType() == 2)
        && ( pocCurr % m_ppcTEncTop[0]->getGOPEncoder()->getIntraRefreshInterval() == 0 )
        )
      {
        // Check if the IRAP refresh interval of any layer does not match that of the base layer
        for(Int i = 1; i < slice->getVPS()->getMaxLayers(); i++)
        {
          Bool refreshIntervalFlag = ( pocCurr % m_ppcTEncTop[i]->getGOPEncoder()->getIntraRefreshInterval() == 0 );
          Bool refreshTypeFlag     = ( m_ppcTEncTop[0]->getGOPEncoder()->getIntraRefreshType() == m_ppcTEncTop[i]->getGOPEncoder()->getIntraRefreshType() );
          if( !(refreshIntervalFlag && refreshTypeFlag) )
          {
            needReset = true;
            resetDueToBL = true;
            break;
          }
        }
      }
    }
    
    if( !needReset )// No need reset due to base layer IRAP
    {
      // Check if EL IDRs results in POC Reset
      for(Int i = 1; i < slice->getVPS()->getMaxLayers() && !needReset; i++)
      {
        Bool idrFlag = ( (m_ppcTEncTop[i]->getGOPEncoder()->getIntraRefreshType() == 2) 
                        && ( pocCurr % m_ppcTEncTop[i]->getGOPEncoder()->getIntraRefreshInterval() == 0 )
                        );
        for(Int j = 0; j < slice->getVPS()->getMaxLayers(); j++)
        {
          if( j == i )
          {
            continue;
          }
          Bool idrOtherPicFlag = ( (m_ppcTEncTop[j]->getGOPEncoder()->getIntraRefreshType() == 2) 
                                  && ( pocCurr % m_ppcTEncTop[j]->getGOPEncoder()->getIntraRefreshInterval() == 0 )
                                  );

          if( idrFlag != idrOtherPicFlag )
          {
            needReset = true;
            break;
          }
        }
      }
    }
    if( needReset )
    {
      if( m_ppcTEncTop[0]->getGOPEncoder()->getIntraRefreshType() == 2 )  // BL IDR refresh, assuming BL picture present
      {
        if( resetDueToBL )
        {
          slice->setPocResetIdc( 2 ); // Full reset needed
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
          if (slice->getVPS()->getVpsPocLsbAlignedFlag() && slice->getVPS()->getNumDirectRefLayers(slice->getLayerId()) == 0)
          {
            slice->setPocMsbNeeded(true);  // Force msb writing
          }
#endif
        }
        else
        {
          slice->setPocResetIdc( 1 ); // Due to IDR in EL
        }
      }
      else
      {
        slice->setPocResetIdc( 1 ); // Only MSB reset
      }

      // Start a new POC reset period
      if (m_layerId == 0)   // Assuming BL picture is always present at encoder; for other AU structures, need to change this
      {
        Int periodId = rand() % 64;
        m_lastPocPeriodId = (periodId == m_lastPocPeriodId) ? (periodId + 1) % 64 : periodId ;

#if P0297_VPS_POC_LSB_ALIGNED_FLAG
        for (UInt i = 0; i < MAX_LAYERS; i++)
        {
          m_ppcTEncTop[i]->setPocDecrementedInDPBFlag(false);
        }
#endif
      }
      else
      {
        m_lastPocPeriodId = m_ppcTEncTop[0]->getGOPEncoder()->getLastPocPeriodId();
      }
      slice->setPocResetPeriodId(m_lastPocPeriodId);
    }
    else
    {
      slice->setPocResetIdc( 0 );
    }
  }
}

Void TEncGOP::updatePocValuesOfPics(Int const pocCurr, TComSlice *const slice)
{
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  UInt affectedLayerList[MAX_NUM_LAYER_IDS];
  Int  numAffectedLayers;

  affectedLayerList[0] = m_layerId;
  numAffectedLayers = 1;

  if (m_pcEncTop->getVPS()->getVpsPocLsbAlignedFlag())
  {
    for (UInt j = 0; j < m_pcEncTop->getVPS()->getNumPredictedLayers(m_layerId); j++)
    {
      affectedLayerList[j + 1] = m_pcEncTop->getVPS()->getPredictedLayerId(m_layerId, j);
    }
    numAffectedLayers = m_pcEncTop->getVPS()->getNumPredictedLayers(m_layerId) + 1;
  }
#endif

  Int pocAdjustValue = pocCurr - m_pcEncTop->getPocAdjustmentValue();

  // New POC reset period
  Int maxPocLsb, pocLsbVal, pocMsbDelta, pocLsbDelta, deltaPocVal;

  maxPocLsb   = 1 << slice->getSPS()->getBitsForPOC();

#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  Int adjustedPocValue = pocCurr;

  if (m_pcEncTop->getFirstPicInLayerDecodedFlag())
  {
#endif

  pocLsbVal   = (slice->getPocResetIdc() == 3)
                ? slice->getPocLsbVal()
                : pocAdjustValue % maxPocLsb; 
  pocMsbDelta = pocAdjustValue - pocLsbVal;
  pocLsbDelta = (slice->getPocResetIdc() == 2 || ( slice->getPocResetIdc() == 3 && slice->getFullPocResetFlag() )) 
                ? pocLsbVal 
                : 0; 
  deltaPocVal = pocMsbDelta  + pocLsbDelta;

#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  Int origDeltaPocVal = deltaPocVal;  // original value needed when updating POC adjustment value

  if (slice->getPocMsbNeeded())  // IDR picture in base layer, non-IDR picture in other layers, poc_lsb_aligned_flag = 1
  {
    if (slice->getLayerId() == 0)
    {
      Int highestPoc = INT_MIN;
      // Find greatest POC in DPB for layer 0
      for (TComList<TComPic*>::iterator iterPic = m_pcEncTop->getListPic()->begin(); iterPic != m_pcEncTop->getListPic()->end(); ++iterPic)
      {
        TComPic *dpbPic = *iterPic;
        if (dpbPic->getReconMark() && dpbPic->getLayerId() == 0 && dpbPic->getPOC() > highestPoc)
        {
          highestPoc = dpbPic->getPOC();
        }
      }
      deltaPocVal = (highestPoc - (highestPoc & (maxPocLsb - 1))) + 1*maxPocLsb;
      m_pcEncTop->setCurrPocMsb(deltaPocVal);
    }
    else
    {
      deltaPocVal = m_ppcTEncTop[0]->getCurrPocMsb();  // copy from base layer
    }
    slice->setPocMsbVal(deltaPocVal);
  }
#endif

#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  for (UInt layerIdx = 0; layerIdx < numAffectedLayers; layerIdx++)
  {
    if (!m_ppcTEncTop[affectedLayerList[layerIdx]]->getPocDecrementedInDPBFlag())
    {
      m_ppcTEncTop[affectedLayerList[layerIdx]]->setPocDecrementedInDPBFlag(true);

      // Decrement value of associatedIrapPoc of the TEncGop object
      m_ppcTEncTop[affectedLayerList[layerIdx]]->getGOPEncoder()->m_associatedIRAPPOC -= deltaPocVal;

      // Decrememnt the value of m_pocCRA
      m_ppcTEncTop[affectedLayerList[layerIdx]]->getGOPEncoder()->m_pocCRA -= deltaPocVal;

      TComList<TComPic*>::iterator  iterPic = m_ppcTEncTop[affectedLayerList[layerIdx]]->getListPic()->begin();
      while (iterPic != m_ppcTEncTop[affectedLayerList[layerIdx]]->getListPic()->end())
#else
  // Decrement value of associatedIrapPoc of the TEncGop object
  this->m_associatedIRAPPOC -= deltaPocVal;

  // Decrememnt the value of m_pocCRA
  this->m_pocCRA -= deltaPocVal;

  // Iterate through all pictures in the DPB
  TComList<TComPic*>::iterator  iterPic = getListPic()->begin();  
  while( iterPic != getListPic()->end() )
#endif
  {
    TComPic *dpbPic = *iterPic;
    
    if( dpbPic->getReconMark() )
    {
      for(Int i = dpbPic->getNumAllocatedSlice() - 1; i >= 0; i--)
      {
        TComSlice *dpbPicSlice = dpbPic->getSlice( i );
        TComReferencePictureSet *dpbPicRps = dpbPicSlice->getRPS();

        // Decrement POC of slice
        dpbPicSlice->setPOC( dpbPicSlice->getPOC() - deltaPocVal );

        // Decrement POC value stored in the RPS of each such slice
        for( Int j = dpbPicRps->getNumberOfPictures() - 1; j >= 0; j-- )
        {
          dpbPicRps->setPOC( j, dpbPicRps->getPOC(j) - deltaPocVal );
        }

        // Decrement value of refPOC
        dpbPicSlice->decrementRefPocValues( deltaPocVal );

        // Update value of associatedIrapPoc of each slice
        dpbPicSlice->setAssociatedIRAPPOC( dpbPicSlice->getAssociatedIRAPPOC() - deltaPocVal );

#if P0297_VPS_POC_LSB_ALIGNED_FLAG
        if (slice->getPocMsbNeeded())
        {
          // this delta value is needed when computing delta POCs in reference picture set initialization
          dpbPicSlice->setPocResetDeltaPoc(dpbPicSlice->getPocResetDeltaPoc() + (deltaPocVal - pocLsbVal));
        }
#endif
      }
    }
    iterPic++;
  }
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
    }
  }
#endif

  // Actual POC value before reset
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  adjustedPocValue = pocCurr - m_pcEncTop->getPocAdjustmentValue();
#else
  Int adjustedPocValue = pocCurr - m_pcEncTop->getPocAdjustmentValue();
#endif

  // Set MSB value before reset
  Int tempLsbVal = adjustedPocValue & (maxPocLsb - 1);
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  if (!slice->getPocMsbNeeded())  // set poc msb normally if special msb handling is not needed
  {
#endif
    slice->setPocMsbVal(adjustedPocValue - tempLsbVal);
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  }
#endif

  // Set LSB value before reset - this is needed in the case of resetIdc = 2
  slice->setPicOrderCntLsb( tempLsbVal );

  // Cumulative delta
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  deltaPocVal = origDeltaPocVal;  // restore deltaPoc for correct adjustment value update
#endif
  m_pcEncTop->setPocAdjustmentValue( m_pcEncTop->getPocAdjustmentValue() + deltaPocVal );

#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  }
#endif

  // New LSB value, after reset
  adjustedPocValue = pocCurr - m_pcEncTop->getPocAdjustmentValue();
  Int newLsbVal = adjustedPocValue & (maxPocLsb - 1);

  // Set value of POC current picture after RESET
  if( slice->getPocResetIdc() == 1 )
  {
    slice->setPOC( newLsbVal );
  }
  else if( slice->getPocResetIdc() == 2 )
  {
    slice->setPOC( 0 );
  }
  else if( slice->getPocResetIdc() == 3 )
  {
    Int picOrderCntMsb = slice->getCurrMsb( newLsbVal, 
                                        slice->getFullPocResetFlag() ? 0 : slice->getPocLsbVal(), 
                                        0,
                                        maxPocLsb );
    slice->setPOC( picOrderCntMsb + newLsbVal );
  }
  else
  {
    assert(0);
  }
}
#endif


#if !SVC_EXTENSION
Void TEncGOP::printOutSummary(UInt uiNumAllPicCoded, Bool isField)
{
  assert (uiNumAllPicCoded == m_gcAnalyzeAll.getNumPic());
  
    
  //--CFG_KDY
  if(isField)
  {
    m_gcAnalyzeAll.setFrmRate( m_pcCfg->getFrameRate() * 2);
    m_gcAnalyzeI.setFrmRate( m_pcCfg->getFrameRate() * 2);
    m_gcAnalyzeP.setFrmRate( m_pcCfg->getFrameRate() * 2);
    m_gcAnalyzeB.setFrmRate( m_pcCfg->getFrameRate() * 2);
  }
   else
  {
    m_gcAnalyzeAll.setFrmRate( m_pcCfg->getFrameRate() );
    m_gcAnalyzeI.setFrmRate( m_pcCfg->getFrameRate() );
    m_gcAnalyzeP.setFrmRate( m_pcCfg->getFrameRate() );
    m_gcAnalyzeB.setFrmRate( m_pcCfg->getFrameRate() );
  }
  
  //-- all
  printf( "\n\nSUMMARY --------------------------------------------------------\n" );
  m_gcAnalyzeAll.printOut('a');
  
  printf( "\n\nI Slices--------------------------------------------------------\n" );
  m_gcAnalyzeI.printOut('i');
  
  printf( "\n\nP Slices--------------------------------------------------------\n" );
  m_gcAnalyzeP.printOut('p');
  
  printf( "\n\nB Slices--------------------------------------------------------\n" );
  m_gcAnalyzeB.printOut('b');
  
#if _SUMMARY_OUT_
  m_gcAnalyzeAll.printSummaryOut();
#endif
#if _SUMMARY_PIC_
  m_gcAnalyzeI.printSummary('I');
  m_gcAnalyzeP.printSummary('P');
  m_gcAnalyzeB.printSummary('B');
#endif

  if(isField)
  {
    //-- interlaced summary
    m_gcAnalyzeAll_in.setFrmRate( m_pcCfg->getFrameRate());
    printf( "\n\nSUMMARY INTERLACED ---------------------------------------------\n" );
    m_gcAnalyzeAll_in.printOutInterlaced('a',  m_gcAnalyzeAll.getBits());
    
#if _SUMMARY_OUT_
    m_gcAnalyzeAll_in.printSummaryOutInterlaced();
#endif
  }

  printf("\nRVM: %.3lf\n" , xCalculateRVM());
}
#endif

Void TEncGOP::preLoopFilterPicAll( TComPic* pcPic, UInt64& ruiDist, UInt64& ruiBits )
{
  TComSlice* pcSlice = pcPic->getSlice(pcPic->getCurrSliceIdx());
  Bool bCalcDist = false;
  m_pcLoopFilter->setCfg(m_pcCfg->getLFCrossTileBoundaryFlag());
  m_pcLoopFilter->loopFilterPic( pcPic );
  
  m_pcEntropyCoder->setEntropyCoder ( m_pcEncTop->getRDGoOnSbacCoder(), pcSlice );
  m_pcEntropyCoder->resetEntropy    ();
  m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );
  m_pcEntropyCoder->resetEntropy    ();
  ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
  
  if (!bCalcDist)
    ruiDist = xFindDistortionFrame(pcPic->getPicYuvOrg(), pcPic->getPicYuvRec());
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================


Void TEncGOP::xInitGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, Bool isField )
{
  assert( iNumPicRcvd > 0 );
  //  Exception for the first frames
  if ( ( isField && (iPOCLast == 0 || iPOCLast == 1) ) || (!isField  && (iPOCLast == 0))  )
  {
    m_iGopSize    = 1;
  }
  else
  {
    m_iGopSize    = m_pcCfg->getGOPSize();
  }
  assert (m_iGopSize > 0);
  
  return;
}

Void TEncGOP::xGetBuffer( TComList<TComPic*>&      rcListPic,
                         TComList<TComPicYuv*>&    rcListPicYuvRecOut,
                         Int                       iNumPicRcvd,
                         Int                       iTimeOffset,
                         TComPic*&                 rpcPic,
                         TComPicYuv*&              rpcPicYuvRecOut,
                         Int                       pocCurr,
                         Bool                      isField)
{
  Int i;
  //  Rec. output
  TComList<TComPicYuv*>::iterator     iterPicYuvRec = rcListPicYuvRecOut.end();

  if (isField)
  {
    for ( i = 0; i < ( (pocCurr == 0 ) || (pocCurr == 1 ) ? (iNumPicRcvd - iTimeOffset + 1) : (iNumPicRcvd - iTimeOffset + 2) ); i++ )
    {
      iterPicYuvRec--;
    }
  }
  else
  {
    for ( i = 0; i < (iNumPicRcvd - iTimeOffset + 1); i++ )
    {
      iterPicYuvRec--;
    }
    
  }
  
  if (isField)
  {
    if(pocCurr == 1)
    {
      iterPicYuvRec++;
    }
  }
  rpcPicYuvRecOut = *(iterPicYuvRec);
  
  //  Current pic.
  TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
  while (iterPic != rcListPic.end())
  {
    rpcPic = *(iterPic);
    rpcPic->setCurrSliceIdx(0);
    if (rpcPic->getPOC() == pocCurr)
    {
      break;
    }
    iterPic++;
  }
  
  assert( rpcPic != NULL );
  assert( rpcPic->getPOC() == pocCurr );
  
  return;
}

UInt64 TEncGOP::xFindDistortionFrame (TComPicYuv* pcPic0, TComPicYuv* pcPic1)
{
  Int     x, y;
  Pel*  pSrc0   = pcPic0 ->getLumaAddr();
  Pel*  pSrc1   = pcPic1 ->getLumaAddr();
  UInt  uiShift = 2 * DISTORTION_PRECISION_ADJUSTMENT(g_bitDepthY-8);
  Int   iTemp;
  
  Int   iStride = pcPic0->getStride();
  Int   iWidth  = pcPic0->getWidth();
  Int   iHeight = pcPic0->getHeight();
  
  UInt64  uiTotalDiff = 0;
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  uiShift = 2 * DISTORTION_PRECISION_ADJUSTMENT(g_bitDepthC-8);
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  
  pSrc0  = pcPic0->getCbAddr();
  pSrc1  = pcPic1->getCbAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  pSrc0  = pcPic0->getCrAddr();
  pSrc1  = pcPic1->getCrAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  return uiTotalDiff;
}

#if VERBOSE_RATE
static const Char* nalUnitTypeToString(NalUnitType type)
{
  switch (type)
  {
    case NAL_UNIT_CODED_SLICE_TRAIL_R:    return "TRAIL_R";
    case NAL_UNIT_CODED_SLICE_TRAIL_N:    return "TRAIL_N";
    case NAL_UNIT_CODED_SLICE_TSA_R:      return "TSA_R";
    case NAL_UNIT_CODED_SLICE_TSA_N:      return "TSA_N";
    case NAL_UNIT_CODED_SLICE_STSA_R:     return "STSA_R";
    case NAL_UNIT_CODED_SLICE_STSA_N:     return "STSA_N";
    case NAL_UNIT_CODED_SLICE_BLA_W_LP:   return "BLA_W_LP";
    case NAL_UNIT_CODED_SLICE_BLA_W_RADL: return "BLA_W_RADL";
    case NAL_UNIT_CODED_SLICE_BLA_N_LP:   return "BLA_N_LP";
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL: return "IDR_W_RADL";
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:   return "IDR_N_LP";
    case NAL_UNIT_CODED_SLICE_CRA:        return "CRA";
    case NAL_UNIT_CODED_SLICE_RADL_R:     return "RADL_R";
    case NAL_UNIT_CODED_SLICE_RADL_N:     return "RADL_N";
    case NAL_UNIT_CODED_SLICE_RASL_R:     return "RASL_R";
    case NAL_UNIT_CODED_SLICE_RASL_N:     return "RASL_N";
    case NAL_UNIT_VPS:                    return "VPS";
    case NAL_UNIT_SPS:                    return "SPS";
    case NAL_UNIT_PPS:                    return "PPS";
    case NAL_UNIT_ACCESS_UNIT_DELIMITER:  return "AUD";
    case NAL_UNIT_EOS:                    return "EOS";
    case NAL_UNIT_EOB:                    return "EOB";
    case NAL_UNIT_FILLER_DATA:            return "FILLER";
    case NAL_UNIT_PREFIX_SEI:             return "SEI";
    case NAL_UNIT_SUFFIX_SEI:             return "SEI";
    default:                              return "UNK";
  }
}
#endif

Void TEncGOP::xCalculateAddPSNR( TComPic* pcPic, TComPicYuv* pcPicD, const AccessUnit& accessUnit, Double dEncTime )
{
  Int     x, y;
  UInt64 uiSSDY  = 0;
  UInt64 uiSSDU  = 0;
  UInt64 uiSSDV  = 0;
  
  Double  dYPSNR  = 0.0;
  Double  dUPSNR  = 0.0;
  Double  dVPSNR  = 0.0;
  
  //===== calculate PSNR =====
  Pel*  pOrg    = pcPic ->getPicYuvOrg()->getLumaAddr();
  Pel*  pRec    = pcPicD->getLumaAddr();
  Int   iStride = pcPicD->getStride();
  
  Int   iWidth;
  Int   iHeight;
  
  iWidth  = pcPicD->getWidth () - m_pcEncTop->getPad(0);
  iHeight = pcPicD->getHeight() - m_pcEncTop->getPad(1);
  
  Int   iSize   = iWidth*iHeight;
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDY   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }
  
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  pOrg  = pcPic ->getPicYuvOrg()->getCbAddr();
  pRec  = pcPicD->getCbAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDU   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }
  
  pOrg  = pcPic ->getPicYuvOrg()->getCrAddr();
  pRec  = pcPicD->getCrAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDV   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }
  
  Int maxvalY = 255 << (g_bitDepthY-8);
  Int maxvalC = 255 << (g_bitDepthC-8);
  Double fRefValueY = (Double) maxvalY * maxvalY * iSize;
  Double fRefValueC = (Double) maxvalC * maxvalC * iSize / 4.0;
  dYPSNR            = ( uiSSDY ? 10.0 * log10( fRefValueY / (Double)uiSSDY ) : 99.99 );
  dUPSNR            = ( uiSSDU ? 10.0 * log10( fRefValueC / (Double)uiSSDU ) : 99.99 );
  dVPSNR            = ( uiSSDV ? 10.0 * log10( fRefValueC / (Double)uiSSDV ) : 99.99 );

  /* calculate the size of the access unit, excluding:
   *  - any AnnexB contributions (start_code_prefix, zero_byte, etc.,)
   *  - SEI NAL units
   */
  UInt numRBSPBytes = 0;
  for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
  {
    UInt numRBSPBytes_nal = UInt((*it)->m_nalUnitData.str().size());
#if VERBOSE_RATE
    printf("*** %6s numBytesInNALunit: %u\n", nalUnitTypeToString((*it)->m_nalUnitType), numRBSPBytes_nal);
#endif
    if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
    {
      numRBSPBytes += numRBSPBytes_nal;
    }
  }

  UInt uibits = numRBSPBytes * 8;
  m_vRVM_RP.push_back( uibits );

  //===== add PSNR =====
#if SVC_EXTENSION
  m_gcAnalyzeAll[m_layerId].addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  TComSlice*  pcSlice = pcPic->getSlice(0);
  if (pcSlice->isIntra())
  {
    m_gcAnalyzeI[m_layerId].addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if (pcSlice->isInterP())
  {
    m_gcAnalyzeP[m_layerId].addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if (pcSlice->isInterB())
  {
    m_gcAnalyzeB[m_layerId].addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
#else
  m_gcAnalyzeAll.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  TComSlice*  pcSlice = pcPic->getSlice(0);
  if (pcSlice->isIntra())
  {
    m_gcAnalyzeI.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if (pcSlice->isInterP())
  {
    m_gcAnalyzeP.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if (pcSlice->isInterB())
  {
    m_gcAnalyzeB.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
#endif

  Char c = (pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B');
  if (!pcSlice->isReferenced()) c += 32;

#if SVC_EXTENSION
#if ADAPTIVE_QP_SELECTION  
  printf("POC %4d LId: %1d TId: %1d ( %c-SLICE %s, nQP %d QP %d ) %10d bits",
         pcSlice->getPOC(),
         pcSlice->getLayerId(),
         pcSlice->getTLayer(),
         c,
         NaluToStr( pcSlice->getNalUnitType() ).data(),
         pcSlice->getSliceQpBase(),
         pcSlice->getSliceQp(),
         uibits );
#else
  printf("POC %4d LId: %1d TId: %1d ( %c-SLICE %s, QP %d ) %10d bits",
         pcSlice->getPOC()-pcSlice->getLastIDR(),
         pcSlice->getLayerId(),
         pcSlice->getTLayer(),
         c,
         NaluToStr( pcSlice->getNalUnitType() ).data(),
         pcSlice->getSliceQp(),
         uibits );
#endif
#else
#if ADAPTIVE_QP_SELECTION
  printf("POC %4d TId: %1d ( %c-SLICE, nQP %d QP %d ) %10d bits",
         pcSlice->getPOC(),
         pcSlice->getTLayer(),
         c,
         pcSlice->getSliceQpBase(),
         pcSlice->getSliceQp(),
         uibits );
#else
  printf("POC %4d TId: %1d ( %c-SLICE, QP %d ) %10d bits",
         pcSlice->getPOC()-pcSlice->getLastIDR(),
         pcSlice->getTLayer(),
         c,
         pcSlice->getSliceQp(),
         uibits );
#endif
#endif

  printf(" [Y %6.4lf dB    U %6.4lf dB    V %6.4lf dB]", dYPSNR, dUPSNR, dVPSNR );
  printf(" [ET %5.0f ]", dEncTime );
  
  for (Int iRefList = 0; iRefList < 2; iRefList++)
  {
    printf(" [L%d ", iRefList);
    for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(RefPicList(iRefList)); iRefIndex++)
    {
#if SVC_EXTENSION
#if VPS_EXTN_DIRECT_REF_LAYERS
      if( pcSlice->getRefPic(RefPicList(iRefList), iRefIndex)->isILR(m_layerId) )
      {
#if POC_RESET_IDC_ENCODER
        UInt refLayerId = pcSlice->getRefPic(RefPicList(iRefList), iRefIndex)->getLayerId();
        UInt refLayerIdc = pcSlice->getReferenceLayerIdc(refLayerId);
        assert( g_posScalingFactor[refLayerIdc][0] );
        assert( g_posScalingFactor[refLayerIdc][1] );

        printf( "%d(%d, {%1.2f, %1.2f}x)", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex), refLayerId, 65536.0/g_posScalingFactor[refLayerIdc][0], 65536.0/g_posScalingFactor[refLayerIdc][1] );
#else
        printf( "%d(%d)", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex)-pcSlice->getLastIDR(), pcSlice->getRefPic(RefPicList(iRefList), iRefIndex)->getLayerId() );
#endif
      }
      else
      {
#if POC_RESET_IDC_ENCODER
        printf ("%d", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex));
#else
        printf ("%d", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex)-pcSlice->getLastIDR());
#endif
      }
#endif
      if( pcSlice->getEnableTMVPFlag() && iRefList == 1 - pcSlice->getColFromL0Flag() && iRefIndex == pcSlice->getColRefIdx() )
      {
        printf( "c" );
      }

      printf( " " );
#else
      printf ("%d ", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex)-pcSlice->getLastIDR());
#endif
    }
    printf("]");
  }
#if Q0048_CGS_3D_ASYMLUT
  pcPic->setFrameBit( (Int)uibits );
  if( m_layerId && pcSlice->getPPS()->getCGSFlag() )
  {
#if R0179_ENC_OPT_3DLUT_SIZE
      m_Enc3DAsymLUTPicUpdate.update3DAsymLUTParam( &m_Enc3DAsymLUTPPS );
#else
    if( m_Enc3DAsymLUTPPS.getPPSBit() > 0 )
      m_Enc3DAsymLUTPicUpdate.copy3DAsymLUT( &m_Enc3DAsymLUTPPS );
#endif
    m_Enc3DAsymLUTPicUpdate.updatePicCGSBits( pcSlice , m_Enc3DAsymLUTPPS.getPPSBit() );
  }
#endif
}


Void reinterlace(Pel* top, Pel* bottom, Pel* dst, UInt stride, UInt width, UInt height, Bool isTff)
{
  
  for (Int y = 0; y < height; y++)
  {
    for (Int x = 0; x < width; x++)
    {
      dst[x] = isTff ? top[x] : bottom[x];
      dst[stride+x] = isTff ? bottom[x] : top[x];
    }
    top += stride;
    bottom += stride;
    dst += stride*2;
  }
}

Void TEncGOP::xCalculateInterlacedAddPSNR( TComPic* pcPicOrgTop, TComPic* pcPicOrgBottom, TComPicYuv* pcPicRecTop, TComPicYuv* pcPicRecBottom, const AccessUnit& accessUnit, Double dEncTime )
{
  Int     x, y;
  
  UInt64 uiSSDY_in  = 0;
  UInt64 uiSSDU_in  = 0;
  UInt64 uiSSDV_in  = 0;
  
  Double  dYPSNR_in  = 0.0;
  Double  dUPSNR_in  = 0.0;
  Double  dVPSNR_in  = 0.0;
  
  /*------ INTERLACED PSNR -----------*/
  
  /* Luma */
  
  Pel*  pOrgTop = pcPicOrgTop->getPicYuvOrg()->getLumaAddr();
  Pel*  pOrgBottom = pcPicOrgBottom->getPicYuvOrg()->getLumaAddr();
  Pel*  pRecTop = pcPicRecTop->getLumaAddr();
  Pel*  pRecBottom = pcPicRecBottom->getLumaAddr();
  
  Int   iWidth;
  Int   iHeight;
  Int iStride;
  
  iWidth  = pcPicOrgTop->getPicYuvOrg()->getWidth () - m_pcEncTop->getPad(0);
  iHeight = pcPicOrgTop->getPicYuvOrg()->getHeight() - m_pcEncTop->getPad(1);
  iStride = pcPicOrgTop->getPicYuvOrg()->getStride();
  Int   iSize   = iWidth*iHeight;
  bool isTff = pcPicOrgTop->isTopField();
  
  TComPicYuv* pcOrgInterlaced = new TComPicYuv;
#if AUXILIARY_PICTURES
  pcOrgInterlaced->create( iWidth, iHeight << 1, pcPicOrgTop->getChromaFormat(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#else
  pcOrgInterlaced->create( iWidth, iHeight << 1, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#endif
  
  TComPicYuv* pcRecInterlaced = new TComPicYuv;
#if AUXILIARY_PICTURES
  pcRecInterlaced->create( iWidth, iHeight << 1, pcPicOrgTop->getChromaFormat(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#else
  pcRecInterlaced->create( iWidth, iHeight << 1, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#endif
  
  Pel* pOrgInterlaced = pcOrgInterlaced->getLumaAddr();
  Pel* pRecInterlaced = pcRecInterlaced->getLumaAddr();
  
  //=== Interlace fields ====
  reinterlace(pOrgTop, pOrgBottom, pOrgInterlaced, iStride, iWidth, iHeight, isTff);
  reinterlace(pRecTop, pRecBottom, pRecInterlaced, iStride, iWidth, iHeight, isTff);
  
  //===== calculate PSNR =====
  for( y = 0; y < iHeight << 1; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrgInterlaced[x] - pRecInterlaced[x] );
      uiSSDY_in   += iDiff * iDiff;
    }
    pOrgInterlaced += iStride;
    pRecInterlaced += iStride;
  }
  
  /*Chroma*/
  
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  
  pOrgTop = pcPicOrgTop->getPicYuvOrg()->getCbAddr();
  pOrgBottom = pcPicOrgBottom->getPicYuvOrg()->getCbAddr();
  pRecTop = pcPicRecTop->getCbAddr();
  pRecBottom = pcPicRecBottom->getCbAddr();
  pOrgInterlaced = pcOrgInterlaced->getCbAddr();
  pRecInterlaced = pcRecInterlaced->getCbAddr();
  
  //=== Interlace fields ====
  reinterlace(pOrgTop, pOrgBottom, pOrgInterlaced, iStride, iWidth, iHeight, isTff);
  reinterlace(pRecTop, pRecBottom, pRecInterlaced, iStride, iWidth, iHeight, isTff);
  
  //===== calculate PSNR =====
  for( y = 0; y < iHeight << 1; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrgInterlaced[x] - pRecInterlaced[x] );
      uiSSDU_in   += iDiff * iDiff;
    }
    pOrgInterlaced += iStride;
    pRecInterlaced += iStride;
  }
  
  pOrgTop = pcPicOrgTop->getPicYuvOrg()->getCrAddr();
  pOrgBottom = pcPicOrgBottom->getPicYuvOrg()->getCrAddr();
  pRecTop = pcPicRecTop->getCrAddr();
  pRecBottom = pcPicRecBottom->getCrAddr();
  pOrgInterlaced = pcOrgInterlaced->getCrAddr();
  pRecInterlaced = pcRecInterlaced->getCrAddr();
  
  //=== Interlace fields ====
  reinterlace(pOrgTop, pOrgBottom, pOrgInterlaced, iStride, iWidth, iHeight, isTff);
  reinterlace(pRecTop, pRecBottom, pRecInterlaced, iStride, iWidth, iHeight, isTff);
  
  //===== calculate PSNR =====
  for( y = 0; y < iHeight << 1; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrgInterlaced[x] - pRecInterlaced[x] );
      uiSSDV_in   += iDiff * iDiff;
    }
    pOrgInterlaced += iStride;
    pRecInterlaced += iStride;
  }
  
  Int maxvalY = 255 << (g_bitDepthY-8);
  Int maxvalC = 255 << (g_bitDepthC-8);
  Double fRefValueY = (Double) maxvalY * maxvalY * iSize*2;
  Double fRefValueC = (Double) maxvalC * maxvalC * iSize*2 / 4.0;
  dYPSNR_in            = ( uiSSDY_in ? 10.0 * log10( fRefValueY / (Double)uiSSDY_in ) : 99.99 );
  dUPSNR_in            = ( uiSSDU_in ? 10.0 * log10( fRefValueC / (Double)uiSSDU_in ) : 99.99 );
  dVPSNR_in            = ( uiSSDV_in ? 10.0 * log10( fRefValueC / (Double)uiSSDV_in ) : 99.99 );
  
  /* calculate the size of the access unit, excluding:
   *  - any AnnexB contributions (start_code_prefix, zero_byte, etc.,)
   *  - SEI NAL units
   */
  UInt numRBSPBytes = 0;
  for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
  {
    UInt numRBSPBytes_nal = UInt((*it)->m_nalUnitData.str().size());
    
    if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
      numRBSPBytes += numRBSPBytes_nal;
  }
  
  UInt uibits = numRBSPBytes * 8 ;
  
  //===== add PSNR =====
  m_gcAnalyzeAll_in.addResult (dYPSNR_in, dUPSNR_in, dVPSNR_in, (Double)uibits);
  
  printf("\n                                      Interlaced frame %d: [Y %6.4lf dB    U %6.4lf dB    V %6.4lf dB]", pcPicOrgBottom->getPOC()/2 , dYPSNR_in, dUPSNR_in, dVPSNR_in );
  
  pcOrgInterlaced->destroy();
  delete pcOrgInterlaced;
  pcRecInterlaced->destroy();
  delete pcRecInterlaced;
}



/** Function for deciding the nal_unit_type.
 * \param pocCurr POC of the current picture
 * \returns the nal unit type of the picture
 * This function checks the configuration and returns the appropriate nal_unit_type for the picture.
 */
NalUnitType TEncGOP::getNalUnitType(Int pocCurr, Int lastIDR, Bool isField)
{
  if (pocCurr == 0)
  {
    return NAL_UNIT_CODED_SLICE_IDR_W_RADL;
  }
#if EFFICIENT_FIELD_IRAP
  if(isField && pocCurr == 1)
  {
    // to avoid the picture becoming an IRAP
    return NAL_UNIT_CODED_SLICE_TRAIL_R;
  }
#endif

#if ALLOW_RECOVERY_POINT_AS_RAP
  if(m_pcCfg->getDecodingRefreshType() != 3 && (pocCurr - isField) % m_pcCfg->getIntraPeriod() == 0)
#else
  if ((pocCurr - isField) % m_pcCfg->getIntraPeriod() == 0)
#endif
  {
    if (m_pcCfg->getDecodingRefreshType() == 1)
    {
      return NAL_UNIT_CODED_SLICE_CRA;
    }
    else if (m_pcCfg->getDecodingRefreshType() == 2)
    {
      return NAL_UNIT_CODED_SLICE_IDR_W_RADL;
    }
  }

#if POC_RESET_IDC_ENCODER
  if(m_pocCraWithoutReset > 0 && this->m_associatedIRAPType == NAL_UNIT_CODED_SLICE_CRA)
  {
    if(pocCurr < m_pocCraWithoutReset)
#else
  if(m_pocCRA>0)
  {
    if(pocCurr<m_pocCRA)
#endif
    {
      // All leading pictures are being marked as TFD pictures here since current encoder uses all 
      // reference pictures while encoding leading pictures. An encoder can ensure that a leading 
      // picture can be still decodable when random accessing to a CRA/CRANT/BLA/BLANT picture by 
      // controlling the reference pictures used for encoding that leading picture. Such a leading 
      // picture need not be marked as a TFD picture.
      return NAL_UNIT_CODED_SLICE_RASL_R;
    }
  }
  if (lastIDR>0)
  {
    if (pocCurr < lastIDR)
    {
      return NAL_UNIT_CODED_SLICE_RADL_R;
    }
  }
  return NAL_UNIT_CODED_SLICE_TRAIL_R;
}

Double TEncGOP::xCalculateRVM()
{
  Double dRVM = 0;
  
  if( m_pcCfg->getGOPSize() == 1 && m_pcCfg->getIntraPeriod() != 1 && m_pcCfg->getFramesToBeEncoded() > RVM_VCEGAM10_M * 2 )
  {
    // calculate RVM only for lowdelay configurations
    std::vector<Double> vRL , vB;
    size_t N = m_vRVM_RP.size();
    vRL.resize( N );
    vB.resize( N );
    
    Int i;
    Double dRavg = 0 , dBavg = 0;
    vB[RVM_VCEGAM10_M] = 0;
    for( i = RVM_VCEGAM10_M + 1 ; i < N - RVM_VCEGAM10_M + 1 ; i++ )
    {
      vRL[i] = 0;
      for( Int j = i - RVM_VCEGAM10_M ; j <= i + RVM_VCEGAM10_M - 1 ; j++ )
        vRL[i] += m_vRVM_RP[j];
      vRL[i] /= ( 2 * RVM_VCEGAM10_M );
      vB[i] = vB[i-1] + m_vRVM_RP[i] - vRL[i];
      dRavg += m_vRVM_RP[i];
      dBavg += vB[i];
    }
    
    dRavg /= ( N - 2 * RVM_VCEGAM10_M );
    dBavg /= ( N - 2 * RVM_VCEGAM10_M );
    
    Double dSigamB = 0;
    for( i = RVM_VCEGAM10_M + 1 ; i < N - RVM_VCEGAM10_M + 1 ; i++ )
    {
      Double tmp = vB[i] - dBavg;
      dSigamB += tmp * tmp;
    }
    dSigamB = sqrt( dSigamB / ( N - 2 * RVM_VCEGAM10_M ) );
    
    Double f = sqrt( 12.0 * ( RVM_VCEGAM10_M - 1 ) / ( RVM_VCEGAM10_M + 1 ) );
    
    dRVM = dSigamB / dRavg * f;
  }
  
  return( dRVM );
}

/** Attaches the input bitstream to the stream in the output NAL unit
    Updates rNalu to contain concatenated bitstream. rpcBitstreamRedirect is cleared at the end of this function call.
 *  \param codedSliceData contains the coded slice data (bitstream) to be concatenated to rNalu
 *  \param rNalu          target NAL unit
 */
Void TEncGOP::xAttachSliceDataToNalUnit (OutputNALUnit& rNalu, TComOutputBitstream*& codedSliceData)
{
  // Byte-align
  rNalu.m_Bitstream.writeByteAlignment();   // Slice header byte-alignment

  // Perform bitstream concatenation
  if (codedSliceData->getNumberOfWrittenBits() > 0)
    {
    rNalu.m_Bitstream.addSubstream(codedSliceData);
  }

  m_pcEntropyCoder->setBitstream(&rNalu.m_Bitstream);

  codedSliceData->clear();
}

// Function will arrange the long-term pictures in the decreasing order of poc_lsb_lt, 
// and among the pictures with the same lsb, it arranges them in increasing delta_poc_msb_cycle_lt value
Void TEncGOP::arrangeLongtermPicturesInRPS(TComSlice *pcSlice, TComList<TComPic*>& rcListPic)
{
  TComReferencePictureSet *rps = pcSlice->getRPS();
  if(!rps->getNumberOfLongtermPictures())
  {
    return;
  }

  // Arrange long-term reference pictures in the correct order of LSB and MSB,
  // and assign values for pocLSBLT and MSB present flag
  Int longtermPicsPoc[MAX_NUM_REF_PICS], longtermPicsLSB[MAX_NUM_REF_PICS], indices[MAX_NUM_REF_PICS];
  Int longtermPicsMSB[MAX_NUM_REF_PICS];
  Bool mSBPresentFlag[MAX_NUM_REF_PICS];
  ::memset(longtermPicsPoc, 0, sizeof(longtermPicsPoc));    // Store POC values of LTRP
  ::memset(longtermPicsLSB, 0, sizeof(longtermPicsLSB));    // Store POC LSB values of LTRP
  ::memset(longtermPicsMSB, 0, sizeof(longtermPicsMSB));    // Store POC LSB values of LTRP
  ::memset(indices        , 0, sizeof(indices));            // Indices to aid in tracking sorted LTRPs
  ::memset(mSBPresentFlag , 0, sizeof(mSBPresentFlag));     // Indicate if MSB needs to be present

  // Get the long-term reference pictures 
  Int offset = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures();
  Int i, ctr = 0;
  Int maxPicOrderCntLSB = 1 << pcSlice->getSPS()->getBitsForPOC();
  for(i = rps->getNumberOfPictures() - 1; i >= offset; i--, ctr++)
  {
    longtermPicsPoc[ctr] = rps->getPOC(i);                                  // LTRP POC
    longtermPicsLSB[ctr] = getLSB(longtermPicsPoc[ctr], maxPicOrderCntLSB); // LTRP POC LSB
    indices[ctr]      = i; 
    longtermPicsMSB[ctr] = longtermPicsPoc[ctr] - longtermPicsLSB[ctr];
  }
  Int numLongPics = rps->getNumberOfLongtermPictures();
  assert(ctr == numLongPics);

  // Arrange pictures in decreasing order of MSB; 
  for(i = 0; i < numLongPics; i++)
  {
    for(Int j = 0; j < numLongPics - 1; j++)
    {
      if(longtermPicsMSB[j] < longtermPicsMSB[j+1])
      {
        std::swap(longtermPicsPoc[j], longtermPicsPoc[j+1]);
        std::swap(longtermPicsLSB[j], longtermPicsLSB[j+1]);
        std::swap(longtermPicsMSB[j], longtermPicsMSB[j+1]);
        std::swap(indices[j]        , indices[j+1]        );
      }
    }
  }

  for(i = 0; i < numLongPics; i++)
  {
    // Check if MSB present flag should be enabled.
    // Check if the buffer contains any pictures that have the same LSB.
    TComList<TComPic*>::iterator  iterPic = rcListPic.begin();  
    TComPic*                      pcPic;
    while ( iterPic != rcListPic.end() )
    {
      pcPic = *iterPic;
      if( (getLSB(pcPic->getPOC(), maxPicOrderCntLSB) == longtermPicsLSB[i])   &&     // Same LSB
                                      (pcPic->getSlice(0)->isReferenced())     &&    // Reference picture
                                        (pcPic->getPOC() != longtermPicsPoc[i])    )  // Not the LTRP itself
      {
        mSBPresentFlag[i] = true;
        break;
      }
      iterPic++;      
    }
  }

  // tempArray for usedByCurr flag
  Bool tempArray[MAX_NUM_REF_PICS]; ::memset(tempArray, 0, sizeof(tempArray));
  for(i = 0; i < numLongPics; i++)
  {
    tempArray[i] = rps->getUsed(indices[i]);
  }
  // Now write the final values;
  ctr = 0;
  Int currMSB = 0, currLSB = 0;
  // currPicPoc = currMSB + currLSB
  currLSB = getLSB(pcSlice->getPOC(), maxPicOrderCntLSB);  
  currMSB = pcSlice->getPOC() - currLSB;

  for(i = rps->getNumberOfPictures() - 1; i >= offset; i--, ctr++)
  {
    rps->setPOC                   (i, longtermPicsPoc[ctr]);
    rps->setDeltaPOC              (i, - pcSlice->getPOC() + longtermPicsPoc[ctr]);
    rps->setUsed                  (i, tempArray[ctr]);
    rps->setPocLSBLT              (i, longtermPicsLSB[ctr]);
    rps->setDeltaPocMSBCycleLT    (i, (currMSB - (longtermPicsPoc[ctr] - longtermPicsLSB[ctr])) / maxPicOrderCntLSB);
    rps->setDeltaPocMSBPresentFlag(i, mSBPresentFlag[ctr]);     

    assert(rps->getDeltaPocMSBCycleLT(i) >= 0);   // Non-negative value
  }
  for(i = rps->getNumberOfPictures() - 1, ctr = 1; i >= offset; i--, ctr++)
  {
    for(Int j = rps->getNumberOfPictures() - 1 - ctr; j >= offset; j--)
    {
      // Here at the encoder we know that we have set the full POC value for the LTRPs, hence we 
      // don't have to check the MSB present flag values for this constraint.
      assert( rps->getPOC(i) != rps->getPOC(j) ); // If assert fails, LTRP entry repeated in RPS!!!
    }
  }
}

/** Function for finding the position to insert the first of APS and non-nested BP, PT, DU info SEI messages.
 * \param accessUnit Access Unit of the current picture
 * This function finds the position to insert the first of APS and non-nested BP, PT, DU info SEI messages.
 */
Int TEncGOP::xGetFirstSeiLocation(AccessUnit &accessUnit)
{
  // Find the location of the first SEI message
  AccessUnit::iterator it;
  Int seiStartPos = 0;
  for(it = accessUnit.begin(); it != accessUnit.end(); it++, seiStartPos++)
  {
     if ((*it)->isSei() || (*it)->isVcl())
     {
       break;
     }               
  }
//  assert(it != accessUnit.end());  // Triggers with some legit configurations
  return seiStartPos;
}

Void TEncGOP::dblMetric( TComPic* pcPic, UInt uiNumSlices )
{
  TComPicYuv* pcPicYuvRec = pcPic->getPicYuvRec();
  Pel* Rec    = pcPicYuvRec->getLumaAddr( 0 );
  Pel* tempRec = Rec;
  Int  stride = pcPicYuvRec->getStride();
  UInt log2maxTB = pcPic->getSlice(0)->getSPS()->getQuadtreeTULog2MaxSize();
  UInt maxTBsize = (1<<log2maxTB);
  const UInt minBlockArtSize = 8;
  const UInt picWidth = pcPicYuvRec->getWidth();
  const UInt picHeight = pcPicYuvRec->getHeight();
  const UInt noCol = (picWidth>>log2maxTB);
  const UInt noRows = (picHeight>>log2maxTB);
  assert(noCol > 1);
  assert(noRows > 1);
  UInt64 *colSAD = (UInt64*)malloc(noCol*sizeof(UInt64));
  UInt64 *rowSAD = (UInt64*)malloc(noRows*sizeof(UInt64));
  UInt colIdx = 0;
  UInt rowIdx = 0;
  Pel p0, p1, p2, q0, q1, q2;
  
  Int qp = pcPic->getSlice(0)->getSliceQp();
  Int bitdepthScale = 1 << (g_bitDepthY-8);
  Int beta = TComLoopFilter::getBeta( qp ) * bitdepthScale;
  const Int thr2 = (beta>>2);
  const Int thr1 = 2*bitdepthScale;
  UInt a = 0;
  
  memset(colSAD, 0, noCol*sizeof(UInt64));
  memset(rowSAD, 0, noRows*sizeof(UInt64));
  
  if (maxTBsize > minBlockArtSize)
  {
    // Analyze vertical artifact edges
    for(Int c = maxTBsize; c < picWidth; c += maxTBsize)
    {
      for(Int r = 0; r < picHeight; r++)
      {
        p2 = Rec[c-3];
        p1 = Rec[c-2];
        p0 = Rec[c-1];
        q0 = Rec[c];
        q1 = Rec[c+1];
        q2 = Rec[c+2];
        a = ((abs(p2-(p1<<1)+p0)+abs(q0-(q1<<1)+q2))<<1);
        if ( thr1 < a && a < thr2)
        {
          colSAD[colIdx] += abs(p0 - q0);
        }
        Rec += stride;
      }
      colIdx++;
      Rec = tempRec;
    }
    
    // Analyze horizontal artifact edges
    for(Int r = maxTBsize; r < picHeight; r += maxTBsize)
    {
      for(Int c = 0; c < picWidth; c++)
      {
        p2 = Rec[c + (r-3)*stride];
        p1 = Rec[c + (r-2)*stride];
        p0 = Rec[c + (r-1)*stride];
        q0 = Rec[c + r*stride];
        q1 = Rec[c + (r+1)*stride];
        q2 = Rec[c + (r+2)*stride];
        a = ((abs(p2-(p1<<1)+p0)+abs(q0-(q1<<1)+q2))<<1);
        if (thr1 < a && a < thr2)
        {
          rowSAD[rowIdx] += abs(p0 - q0);
        }
      }
      rowIdx++;
    }
  }
  
  UInt64 colSADsum = 0;
  UInt64 rowSADsum = 0;
  for(Int c = 0; c < noCol-1; c++)
  {
    colSADsum += colSAD[c];
  }
  for(Int r = 0; r < noRows-1; r++)
  {
    rowSADsum += rowSAD[r];
  }
  
  colSADsum <<= 10;
  rowSADsum <<= 10;
  colSADsum /= (noCol-1);
  colSADsum /= picHeight;
  rowSADsum /= (noRows-1);
  rowSADsum /= picWidth;
  
  UInt64 avgSAD = ((colSADsum + rowSADsum)>>1);
  avgSAD >>= (g_bitDepthY-8);
  
  if ( avgSAD > 2048 )
  {
    avgSAD >>= 9;
    Int offset = Clip3(2,6,(Int)avgSAD);
    for (Int i=0; i<uiNumSlices; i++)
    {
      pcPic->getSlice(i)->setDeblockingFilterOverrideFlag(true);
      pcPic->getSlice(i)->setDeblockingFilterDisable(false);
      pcPic->getSlice(i)->setDeblockingFilterBetaOffsetDiv2( offset );
      pcPic->getSlice(i)->setDeblockingFilterTcOffsetDiv2( offset );
    }
  }
  else
  {
    for (Int i=0; i<uiNumSlices; i++)
    {
      pcPic->getSlice(i)->setDeblockingFilterOverrideFlag(false);
      pcPic->getSlice(i)->setDeblockingFilterDisable(        pcPic->getSlice(i)->getPPS()->getPicDisableDeblockingFilterFlag() );
      pcPic->getSlice(i)->setDeblockingFilterBetaOffsetDiv2( pcPic->getSlice(i)->getPPS()->getDeblockingFilterBetaOffsetDiv2() );
      pcPic->getSlice(i)->setDeblockingFilterTcOffsetDiv2(   pcPic->getSlice(i)->getPPS()->getDeblockingFilterTcOffsetDiv2()   );
    }
  }
  
  free(colSAD);
  free(rowSAD);
}
#if SVC_EXTENSION
#if LAYERS_NOT_PRESENT_SEI
SEILayersNotPresent* TEncGOP::xCreateSEILayersNotPresent ()
{
  UInt i = 0;
  SEILayersNotPresent *seiLayersNotPresent = new SEILayersNotPresent(); 
  seiLayersNotPresent->m_activeVpsId = m_pcCfg->getVPS()->getVPSId(); 
  seiLayersNotPresent->m_vpsMaxLayers = m_pcCfg->getVPS()->getMaxLayers();
  for ( ; i < seiLayersNotPresent->m_vpsMaxLayers; i++)
  {
    seiLayersNotPresent->m_layerNotPresentFlag[i] = true; 
  }
  for ( ; i < MAX_LAYERS; i++)
  {
    seiLayersNotPresent->m_layerNotPresentFlag[i] = false; 
  }
  return seiLayersNotPresent;
}
#endif

#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
SEIInterLayerConstrainedTileSets* TEncGOP::xCreateSEIInterLayerConstrainedTileSets()
{
  SEIInterLayerConstrainedTileSets *seiInterLayerConstrainedTileSets = new SEIInterLayerConstrainedTileSets();
  seiInterLayerConstrainedTileSets->m_ilAllTilesExactSampleValueMatchFlag = false;
  seiInterLayerConstrainedTileSets->m_ilOneTilePerTileSetFlag = false;
  if (!seiInterLayerConstrainedTileSets->m_ilOneTilePerTileSetFlag)
  {
    seiInterLayerConstrainedTileSets->m_ilNumSetsInMessageMinus1 = m_pcCfg->getIlNumSetsInMessage() - 1;
    if (seiInterLayerConstrainedTileSets->m_ilNumSetsInMessageMinus1)
    {
      seiInterLayerConstrainedTileSets->m_skippedTileSetPresentFlag = m_pcCfg->getSkippedTileSetPresentFlag();
    }
    else
    {
      seiInterLayerConstrainedTileSets->m_skippedTileSetPresentFlag = false;
    }
    seiInterLayerConstrainedTileSets->m_ilNumSetsInMessageMinus1 += seiInterLayerConstrainedTileSets->m_skippedTileSetPresentFlag ? 1 : 0;
    for (UInt i = 0; i < m_pcCfg->getIlNumSetsInMessage(); i++)
    {
      seiInterLayerConstrainedTileSets->m_ilctsId[i] = i;
      seiInterLayerConstrainedTileSets->m_ilNumTileRectsInSetMinus1[i] = 0;
      for( UInt j = 0; j <= seiInterLayerConstrainedTileSets->m_ilNumTileRectsInSetMinus1[i]; j++)
      {
        seiInterLayerConstrainedTileSets->m_ilTopLeftTileIndex[i][j]     = m_pcCfg->getTopLeftTileIndex(i);
        seiInterLayerConstrainedTileSets->m_ilBottomRightTileIndex[i][j] = m_pcCfg->getBottomRightTileIndex(i);
      }
      seiInterLayerConstrainedTileSets->m_ilcIdc[i] = m_pcCfg->getIlcIdc(i);
      if (seiInterLayerConstrainedTileSets->m_ilAllTilesExactSampleValueMatchFlag)
      {
        seiInterLayerConstrainedTileSets->m_ilExactSampleValueMatchFlag[i] = false;
      }
    }
  }

  return seiInterLayerConstrainedTileSets;
}

Void TEncGOP::xBuildTileSetsMap(TComPicSym* picSym)
{
  Int numCUs = picSym->getFrameWidthInCU() * picSym->getFrameHeightInCU();

  for (Int i = 0; i < numCUs; i++)
  {
    picSym->setTileSetIdxMap(i, -1, 0, false);
  }

  for (Int i = 0; i < m_pcCfg->getIlNumSetsInMessage(); i++)
  {
    TComTile* topLeftTile     = picSym->getTComTile(m_pcCfg->getTopLeftTileIndex(i));
    TComTile* bottomRightTile = picSym->getTComTile(m_pcCfg->getBottomRightTileIndex(i));
    Int tileSetLeftEdgePosInCU = topLeftTile->getRightEdgePosInCU() - topLeftTile->getTileWidth() + 1;
    Int tileSetRightEdgePosInCU = bottomRightTile->getRightEdgePosInCU();
    Int tileSetTopEdgePosInCU = topLeftTile->getBottomEdgePosInCU() - topLeftTile->getTileHeight() + 1;
    Int tileSetBottomEdgePosInCU = bottomRightTile->getBottomEdgePosInCU();
    assert(tileSetLeftEdgePosInCU < tileSetRightEdgePosInCU && tileSetTopEdgePosInCU < tileSetBottomEdgePosInCU);
    for (Int j = tileSetTopEdgePosInCU; j <= tileSetBottomEdgePosInCU; j++)
    {
      for (Int k = tileSetLeftEdgePosInCU; k <= tileSetRightEdgePosInCU; k++)
      {
        picSym->setTileSetIdxMap(j * picSym->getFrameWidthInCU() + k, i, m_pcCfg->getIlcIdc(i), false);
      }
    }
  }
  
  if (m_pcCfg->getSkippedTileSetPresentFlag())
  {
    Int skippedTileSetIdx = m_pcCfg->getIlNumSetsInMessage();
    for (Int i = 0; i < numCUs; i++)
    {
      if (picSym->getTileSetIdxMap(i) < 0)
      {
        picSym->setTileSetIdxMap(i, skippedTileSetIdx, 0, true);
      }
    }
  }
}
#endif

#if O0164_MULTI_LAYER_HRD
#if VPS_VUI_BSP_HRD_PARAMS
SEIScalableNesting* TEncGOP::xCreateBspNestingSEI(TComSlice *pcSlice, Int olsIdx, Int partitioningSchemeIdx, Int bspIdx)
#else
SEIScalableNesting* TEncGOP::xCreateBspNestingSEI(TComSlice *pcSlice)
#endif
{
  SEIScalableNesting *seiScalableNesting = new SEIScalableNesting();
  SEIBspInitialArrivalTime *seiBspInitialArrivalTime = new SEIBspInitialArrivalTime();
  SEIBspNesting *seiBspNesting = new SEIBspNesting();
  SEIBufferingPeriod *seiBufferingPeriod = new SEIBufferingPeriod();

  // Scalable nesting SEI

  seiScalableNesting->m_bitStreamSubsetFlag           = 1;      // If the nested SEI messages are picture buffereing SEI mesages, picure timing SEI messages or sub-picture timing SEI messages, bitstream_subset_flag shall be equal to 1
  seiScalableNesting->m_nestingOpFlag                 = 1;
  seiScalableNesting->m_defaultOpFlag                 = 0;
  seiScalableNesting->m_nestingNumOpsMinus1           = 0;      //nesting_num_ops_minus1
#if VPS_VUI_BSP_HRD_PARAMS
  seiScalableNesting->m_nestingOpIdx[0]               = pcSlice->getVPS()->getOutputLayerSetIdx(olsIdx);
  seiScalableNesting->m_nestingMaxTemporalIdPlus1[0]  = 6 + 1;
#else
  seiScalableNesting->m_nestingOpIdx[0]               = 1;
#endif
  seiScalableNesting->m_allLayersFlag                 = 0;
  seiScalableNesting->m_nestingNoOpMaxTemporalIdPlus1 = 6 + 1;  //nesting_no_op_max_temporal_id_plus1
  seiScalableNesting->m_nestingNumLayersMinus1        = 1 - 1;  //nesting_num_layers_minus1
  seiScalableNesting->m_nestingLayerId[0]             = 0;
  seiScalableNesting->m_callerOwnsSEIs                = true;

  // Bitstream partition nesting SEI

  seiBspNesting->m_bspIdx = 0;
  seiBspNesting->m_callerOwnsSEIs = true;

  // Buffering period SEI

  UInt uiInitialCpbRemovalDelay = (90000/2);                      // 0.5 sec
  seiBufferingPeriod->m_initialCpbRemovalDelay      [0][0]     = uiInitialCpbRemovalDelay;
  seiBufferingPeriod->m_initialCpbRemovalDelayOffset[0][0]     = uiInitialCpbRemovalDelay;
  seiBufferingPeriod->m_initialCpbRemovalDelay      [0][1]     = uiInitialCpbRemovalDelay;
  seiBufferingPeriod->m_initialCpbRemovalDelayOffset[0][1]     = uiInitialCpbRemovalDelay;

  Double dTmp = (Double)pcSlice->getSPS()->getVuiParameters()->getTimingInfo()->getNumUnitsInTick() / (Double)pcSlice->getSPS()->getVuiParameters()->getTimingInfo()->getTimeScale();

  UInt uiTmp = (UInt)( dTmp * 90000.0 ); 
  uiInitialCpbRemovalDelay -= uiTmp;
  uiInitialCpbRemovalDelay -= uiTmp / ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getTickDivisorMinus2() + 2 );
  seiBufferingPeriod->m_initialAltCpbRemovalDelay      [0][0]  = uiInitialCpbRemovalDelay;
  seiBufferingPeriod->m_initialAltCpbRemovalDelayOffset[0][0]  = uiInitialCpbRemovalDelay;
  seiBufferingPeriod->m_initialAltCpbRemovalDelay      [0][1]  = uiInitialCpbRemovalDelay;
  seiBufferingPeriod->m_initialAltCpbRemovalDelayOffset[0][1]  = uiInitialCpbRemovalDelay;

  seiBufferingPeriod->m_rapCpbParamsPresentFlag              = 0;
  //for the concatenation, it can be set to one during splicing.
  seiBufferingPeriod->m_concatenationFlag = 0;
  //since the temporal layer HRD is not ready, we assumed it is fixed
  seiBufferingPeriod->m_auCpbRemovalDelayDelta = 1;
  seiBufferingPeriod->m_cpbDelayOffset = 0;
  seiBufferingPeriod->m_dpbDelayOffset = 0;

  // Intial arrival time SEI message

  seiBspInitialArrivalTime->m_nalInitialArrivalDelay[0] = 0;
  seiBspInitialArrivalTime->m_vclInitialArrivalDelay[0] = 0;


  seiBspNesting->m_nestedSEIs.push_back(seiBufferingPeriod);
  seiBspNesting->m_nestedSEIs.push_back(seiBspInitialArrivalTime);
#if VPS_VUI_BSP_HRD_PARAMS
  seiBspNesting->m_bspIdx = bspIdx;
  seiBspNesting->m_seiOlsIdx = olsIdx;
  seiBspNesting->m_seiPartitioningSchemeIdx = partitioningSchemeIdx;
#endif
  seiScalableNesting->m_nestedSEIs.push_back(seiBspNesting); // BSP nesting SEI is contained in scalable nesting SEI

  return seiScalableNesting;
}
#endif

#if Q0048_CGS_3D_ASYMLUT
Void TEncGOP::xDetermin3DAsymLUT( TComSlice * pSlice , TComPic * pCurPic , UInt refLayerIdc , TEncCfg * pCfg , Bool bSignalPPS )
{
  Int nCGSFlag = pSlice->getPPS()->getCGSFlag();
  m_Enc3DAsymLUTPPS.setPPSBit( 0 );
  Double dErrorUpdatedPPS = 0 , dErrorPPS = 0;

#if R0179_ENC_OPT_3DLUT_SIZE
  Int nTLthres = m_pcCfg->getCGSLutSizeRDO() ? 2:7;
  Double dFrameLambda; 
#if FULL_NBIT
  Int    SHIFT_QP = 12 + 6 * (pSlice->getBitDepthY() - 8);
#else
  Int    SHIFT_QP = 12; 
#endif 
  Int QP = pSlice->getSliceQp();

  // set frame lambda
  dFrameLambda = 0.68 * pow (2, (QP  - SHIFT_QP) / 3.0) * (m_pcCfg->getGOPSize() > 1 && pSlice->isInterB()? 2 : 1);

  if(m_pcCfg->getCGSLutSizeRDO() == 1 && (!bSignalPPS && (pSlice->getDepth() < nTLthres))) 
    dErrorUpdatedPPS = m_Enc3DAsymLUTPicUpdate.derive3DAsymLUT( pSlice , pCurPic , refLayerIdc , pCfg , bSignalPPS , m_pcEncTop->getElRapSliceTypeB(), dFrameLambda );
  else if (pSlice->getDepth() >= nTLthres)
    dErrorUpdatedPPS = MAX_DOUBLE;
  else // if (m_pcCfg->getCGSLutSizeRDO() = 0 || bSignalPPS)
#endif   
    dErrorUpdatedPPS = m_Enc3DAsymLUTPicUpdate.derive3DAsymLUT( pSlice , pCurPic , refLayerIdc , pCfg , bSignalPPS , m_pcEncTop->getElRapSliceTypeB() );


  if( bSignalPPS )
  {
    m_Enc3DAsymLUTPPS.copy3DAsymLUT( &m_Enc3DAsymLUTPicUpdate );
    pSlice->setCGSOverWritePPS( 1 ); // regular PPS update
  }
  else if( nCGSFlag )
  {
#if R0179_ENC_OPT_3DLUT_SIZE
    if(pSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R || pSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N) 
    {
      pSlice->setCGSOverWritePPS( 0 ); 
    }
    else if (pSlice->getDepth() >= nTLthres) 
    {
      pSlice->setCGSOverWritePPS( 0 ); 
    }
    else
    {
#endif    
      dErrorPPS = m_Enc3DAsymLUTPPS.estimateDistWithCur3DAsymLUT( pCurPic , refLayerIdc );
      Double dFactor = pCfg->getIntraPeriod() == 1 ? 0.99 : 0.9;

#if R0179_ENC_OPT_3DLUT_SIZE 
      if( m_pcCfg->getCGSLutSizeRDO() )
      {
        dErrorPPS = dErrorPPS/m_Enc3DAsymLUTPicUpdate.getDistFactor(pSlice->getSliceType(), pSlice->getDepth()); 
      }
#endif
      pSlice->setCGSOverWritePPS( dErrorUpdatedPPS < dFactor * dErrorPPS );
#if R0179_ENC_OPT_3DLUT_SIZE
    }
#endif
    if( pSlice->getCGSOverWritePPS() )
    {
      m_Enc3DAsymLUTPPS.copy3DAsymLUT( &m_Enc3DAsymLUTPicUpdate );
    }
  }
  pSlice->getPPS()->setCGSOutputBitDepthY( m_Enc3DAsymLUTPPS.getOutputBitDepthY() );
  pSlice->getPPS()->setCGSOutputBitDepthC( m_Enc3DAsymLUTPPS.getOutputBitDepthC() );
}

Void TEncGOP::downScalePic( TComPicYuv* pcYuvSrc, TComPicYuv* pcYuvDest)
{
  Int inputBitDepth = g_bitDepthYLayer[m_layerId];
  Int outputBitDepth = g_bitDepthYLayer[m_layerId];
  {
    pcYuvSrc->setBorderExtension(false);
    pcYuvSrc->extendPicBorder   (); // extend the border.
    pcYuvSrc->setBorderExtension(false);

    Int iWidth = pcYuvSrc->getWidth();
    Int iHeight =pcYuvSrc->getHeight(); 

    if(!m_temp)
    {
      initDs(iWidth, iHeight, m_pcCfg->getIntraPeriod()>1);
    }

    filterImg(pcYuvSrc->getLumaAddr(), pcYuvSrc->getStride(), pcYuvDest->getLumaAddr(), pcYuvDest->getStride(), iHeight, iWidth,  inputBitDepth-outputBitDepth, 0);
    filterImg(pcYuvSrc->getCbAddr(), pcYuvSrc->getCStride(), pcYuvDest->getCbAddr(), pcYuvDest->getCStride(), iHeight>>1, iWidth>>1, inputBitDepth-outputBitDepth, 1);
    filterImg(pcYuvSrc->getCrAddr(), pcYuvSrc->getCStride(), pcYuvDest->getCrAddr(), pcYuvDest->getCStride(), iHeight>>1, iWidth>>1, inputBitDepth-outputBitDepth, 2);  
  }
}
const Int TEncGOP::m_phase_filter_0_t0[4][13]={
  {0,  2,  -3,  -9,   6,  39,  58,  39,   6,  -9,  -3,  2,  0},  
  {0, 0,  0,  -2,  8,-20, 116, 34, -10,  2,  0, 0,  0},                      //{0,  1,  -1,  -8,  -1,  31,  57,  47,  13,  -7,  -5,  1,  0},  //
  {0,  1,   0,  -7,  -5,  22,  53,  53,  22,  -5,  -7,  0,  1},  
  {0,  0,   1,  -5,  -7,  13,  47,  57,  31,  -1,  -8,-1,  1}  
};

const Int TEncGOP::m_phase_filter_0_t1[4][13]={
  {0,  4,  0,  -12, 0,  40,  64,  40, 0, -12,  0,  4,  0},
  {0, 0,  0,  -2,  8,-20, 116,34,-10,  2,  0, 0,  0},                      //{0,  1,  -1,  -8,  -1,  31,  57,  47,  13,  -7,  -5,  1,  0},  //
  {0,  1,   0,  -7,  -5,  22,  53,  53,  22,  -5,  -7,  0,  1},  
  {0,  0,   1,  -5,  -7,  13,  47,  57,  31,  -1,  -8,-1,  1}  
};
const Int TEncGOP::m_phase_filter_0_t1_chroma[4][13]={
  {0,  0,  0,   0,  0,   0,  128, 0,  0,  0,  0,  0,  0},
  {0, 0,  0,  -2,  8,-20, 116,34,-10,  2,  0, 0,  0},                      //{0,  1,  -1,  -8,  -1,  31,  57,  47,  13,  -7,  -5,  1,  0},  //
  {0,  1,   0,  -7,  -5,  22,  53,  53,  22,  -5,  -7,  0,  1},  
  {0,  0,   1,  -5,  -7,  13,  47,  57,  31,  -1,  -8,-1,  1}  
};

const Int TEncGOP::m_phase_filter_1[8][13]={
  {0,   0,  5,  -6,  -10,37,  76,  37,-10,   -6, 5,  0,   0},    
  {0,  -1,  5,  -3,  -12,29,  75,  45,  -7,   -8, 5,  0,   0},    
  {0,  -1,  4,  -1,  -13,22,  73,  52,  -3,  -10, 4,  1,   0},    
  {0,  -1,  4,   1,  -13,14,  70,  59,   2,  -12, 3,  2,  -1},  
  {0,  -1,  3,   2,  -13, 8,  65,  65,   8,  -13, 2,  3,  -1},    
  {0,  -1,  2,   3,  -12, 2,  59,  70,  14,  -13, 1,  4,  -1},    
  {0,   0,  1,   4,  -10,-3,  52,  73,  22,  -13,-1,  4,  -1},    
  {0,   0,  0,   5,   -8,-7,  45,  75,  29,  -12,-3,  5,  -1}    
};

#if CGS_GCC_NO_VECTORIZATION  
#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GCC_VERSION > 40600
__attribute__((optimize("no-tree-vectorize")))
#endif
#endif
#endif
Void TEncGOP::filterImg(
    Pel           *src,
    Int           iSrcStride,
    Pel           *dst,
    Int           iDstStride,
    Int           height1,  
    Int           width1,  
    Int           shift,
    Int           plane)
{
  Int length = m_iTap;
  Int height2,width2;
  Int k,iSum;
  Int i0, div_i0, i1;
  Int j0, div_j0, j1;
  const Int *p_filter;
  Pel *p_src, *p_dst;
  Pel *p_src_line, *p_dst_line;
  Int **p_temp, *p_tmp;
  Int shift2 = 2*7+shift;
  Int shift_round = (1 << (shift2 - 1));
  Int iMax = (1<<(g_bitDepthY-shift))-1;
  height2 = (height1 * m_iM) / m_iN;
  width2  = (width1  * m_iM) / m_iN;

  m_phase_filter = plane? m_phase_filter_chroma : m_phase_filter_luma;

  // horizontal filtering
  p_src_line = src;
  for(j1 = 0; j1 < height1; j1++)
  {
    i0=-m_iN;
    p_tmp = m_temp[j1];
    
    for(i1 = 0; i1 < width2; i1++)
    {
      i0      += m_iN;
      div_i0   = (i0 / m_iM);
      p_src    =  p_src_line + ( div_i0 - (length >> 1));
      p_filter = m_phase_filter[i0 - div_i0 * m_iM]; // phase_filter[i0 % M]
      iSum     = 0;
      for(k = 0; k < length; k++)
      {
        iSum += (*p_src++) * (*p_filter++);
      }
      *p_tmp++ = iSum;
    }
    p_src_line +=  iSrcStride;
  }

  // pad temp (vertical)
  for (k=-(length>>1); k<0; k++)
  {
    memcpy(m_temp[k], m_temp[0], width2*sizeof(Int));
  }
  for (k=height1; k<(height1+(length>>1)); k++)
  {
    memcpy(m_temp[k], m_temp[k-1], (width2)* sizeof(Int));
  }

  // vertical filtering
  j0 = (plane == 0) ? -m_iN : -(m_iN-1);
  
  p_dst_line = dst;
  for(j1 = 0; j1 < height2; j1++)
  {
    j0      += m_iN;
    div_j0   = (j0 / m_iM);
    p_dst = p_dst_line;
    p_temp   = &m_temp[div_j0 - (length>>1)];
    p_filter = m_phase_filter[j0 - div_j0 * m_iM]; // phase_filter[j0 % M]
    for(i1 = 0; i1 < width2;i1++)
    {
      iSum=0;
      for(k = 0; k < length; k++)
      {
        iSum += p_temp[k][i1] * p_filter[k];
      }
      iSum=((iSum + shift_round) >> shift2);
      *p_dst++ = (Short)(iSum > iMax ? iMax : (iSum < 0 ? 0 : iSum));
    }
    p_dst_line += iDstStride;
  }
}

Void TEncGOP::initDs(Int iWidth, Int iHeight, Int iType)
{
  m_iTap = 13;
  if(g_posScalingFactor[0][0] == (1<<15))
  {
    m_iM = 4;
    m_iN = 8;
    m_phase_filter_luma = iType? m_phase_filter_0_t1 : m_phase_filter_0_t0;
    m_phase_filter_chroma = m_phase_filter_0_t1_chroma; 
  }
  else
  {
    m_iM = 8;
    m_iN = 12;
    m_phase_filter_luma = m_phase_filter_chroma =  m_phase_filter_1;
    m_phase_filter = m_phase_filter_1;
  }

  get_mem2DintWithPad (&m_temp, iHeight, iWidth*m_iM/m_iN,   m_iTap>>1, 0);
}

Int TEncGOP::get_mem2DintWithPad(Int ***array2D, Int dim0, Int dim1, Int iPadY, Int iPadX)
{
  Int i;
  Int *curr = NULL;
  Int iHeight, iWidth;

  iHeight = dim0+2*iPadY;
  iWidth = dim1+2*iPadX;
  (*array2D) = (Int**)malloc(iHeight*sizeof(Int*));
  *(*array2D) = (Int* )xMalloc(Int, iHeight*iWidth);

  (*array2D)[0] += iPadX;
  curr = (*array2D)[0];
  for(i = 1 ; i < iHeight; i++)
  {
    curr += iWidth;
    (*array2D)[i] = curr;
  }
  (*array2D) = &((*array2D)[iPadY]);

  return 0;
}

Void TEncGOP::free_mem2DintWithPad(Int **array2D, Int iPadY, Int iPadX)
{
  if (array2D)
  {
    if (*array2D)
    {
      xFree(array2D[-iPadY]-iPadX);
    }
    else 
    {
      printf("free_mem2DintWithPad: trying to free unused memory\r\nPress Any Key\r\n");
    }

    free (&array2D[-iPadY]);
  } 
  else
  {
    printf("free_mem2DintWithPad: trying to free unused memory\r\nPress Any Key\r\n");
  }
}
#endif
#endif //SVC_EXTENSION

//! \}
