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

/** \file     TEncTop.h
    \brief    encoder class (header)
*/

#ifndef __TENCTOP__
#define __TENCTOP__

// Include files
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/AccessUnit.h"

#include "TLibVideoIO/TVideoIOYuv.h"

#include "TEncCfg.h"
#include "TEncGOP.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"
#include "TEncSearch.h"
#include "TEncSampleAdaptiveOffset.h"
#include "TEncPreanalyzer.h"
#include "TEncRateCtrl.h"
//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder class
class TEncTop : public TEncCfg
{
private:
  // picture
  Int                     m_iPOCLast;                     ///< time index (POC)
  Int                     m_iNumPicRcvd;                  ///< number of received pictures
  UInt                    m_uiNumAllPicCoded;             ///< number of coded pictures
  TComList<TComPic*>      m_cListPic;                     ///< dynamic list of pictures
 
  // encoder search
  TEncSearch              m_cSearch;                      ///< encoder search class
  //TEncEntropy*            m_pcEntropyCoder;                     ///< entropy encoder
  TEncCavlc*              m_pcCavlcCoder;                       ///< CAVLC encoder  
  // coding tool
  TComTrQuant             m_cTrQuant;                     ///< transform & quantization class
  TComLoopFilter          m_cLoopFilter;                  ///< deblocking filter class
  TEncSampleAdaptiveOffset m_cEncSAO;                     ///< sample adaptive offset class
  TEncEntropy             m_cEntropyCoder;                ///< entropy encoder
  TEncCavlc               m_cCavlcCoder;                  ///< CAVLC encoder
  TEncSbac                m_cSbacCoder;                   ///< SBAC encoder
  TEncBinCABAC            m_cBinCoderCABAC;               ///< bin coder CABAC
  TEncSbac*               m_pcSbacCoders;                 ///< SBAC encoders (to encode substreams )
  TEncBinCABAC*           m_pcBinCoderCABACs;             ///< bin coders CABAC (one per substream)
  
  // processing unit
  TEncGOP                 m_cGOPEncoder;                  ///< GOP encoder
  TEncSlice               m_cSliceEncoder;                ///< slice encoder
  TEncCu                  m_cCuEncoder;                   ///< CU encoder
  // SPS
  TComSPS                 m_cSPS;                         ///< SPS
  TComPPS                 m_cPPS;                         ///< PPS
  // RD cost computation
  TComBitCounter          m_cBitCounter;                  ///< bit counter for RD optimization
  TComRdCost              m_cRdCost;                      ///< RD cost computation class
  TEncSbac***             m_pppcRDSbacCoder;              ///< temporal storage for RD computation
  TEncSbac                m_cRDGoOnSbacCoder;             ///< going on SBAC model for RD stage
#if FAST_BIT_EST
  TEncBinCABACCounter***  m_pppcBinCoderCABAC;            ///< temporal CABAC state storage for RD computation
  TEncBinCABACCounter     m_cRDGoOnBinCoderCABAC;         ///< going on bin coder CABAC for RD stage
#else
  TEncBinCABAC***         m_pppcBinCoderCABAC;            ///< temporal CABAC state storage for RD computation
  TEncBinCABAC            m_cRDGoOnBinCoderCABAC;         ///< going on bin coder CABAC for RD stage
#endif
  Int                     m_iNumSubstreams;                ///< # of top-level elements allocated.
  TComBitCounter*         m_pcBitCounters;                 ///< bit counters for RD optimization per substream
  TComRdCost*             m_pcRdCosts;                     ///< RD cost computation class per substream
  TEncSbac****            m_ppppcRDSbacCoders;             ///< temporal storage for RD computation per substream
  TEncSbac*               m_pcRDGoOnSbacCoders;            ///< going on SBAC model for RD stage per substream
  TEncBinCABAC****        m_ppppcBinCodersCABAC;           ///< temporal CABAC state storage for RD computation per substream
  TEncBinCABAC*           m_pcRDGoOnBinCodersCABAC;        ///< going on bin coder CABAC for RD stage per substream

  // quality control
  TEncPreanalyzer         m_cPreanalyzer;                 ///< image characteristics analyzer for TM5-step3-like adaptive QP

  TComScalingList         m_scalingList;                 ///< quantization matrix information
  TEncRateCtrl            m_cRateCtrl;                    ///< Rate control class
  
#if SVC_EXTENSION
  static Int              m_iSPSIdCnt;                    ///< next Id number for SPS    
  static Int              m_iPPSIdCnt;                    ///< next Id number for PPS    
  TEncTop**               m_ppcTEncTop;
  TEncTop*                getLayerEnc(UInt layer)   { return m_ppcTEncTop[layer]; }
  TComPic*                m_cIlpPic[MAX_NUM_REF];                    ///<  Inter layer Prediction picture =  upsampled picture 
#if REF_IDX_MFM
  Bool                    m_bMFMEnabledFlag;
#endif
  UInt                    m_numScaledRefLayerOffsets;
#if O0098_SCALED_REF_LAYER_ID
  UInt                    m_scaledRefLayerId[MAX_LAYERS];
#endif
  Window                  m_scaledRefLayerWindow[MAX_LAYERS];
#if REF_REGION_OFFSET
  UInt                    m_numRefLayerOffsets;
  UInt                    m_refLayerId[MAX_LAYERS];
  Window                  m_refLayerWindow[MAX_LAYERS];
  Bool                    m_scaledRefLayerOffsetPresentFlag[MAX_LAYERS];
  Bool                    m_refRegionOffsetPresentFlag[MAX_LAYERS];
#endif
#if R0209_GENERIC_PHASE
  Int                     m_phaseHorLuma  [MAX_LAYERS];
  Int                     m_phaseVerLuma  [MAX_LAYERS];
  Int                     m_phaseHorChroma[MAX_LAYERS];
  Int                     m_phaseVerChroma[MAX_LAYERS];
  Int                     m_resamplePhaseSetPresentFlag[MAX_LAYERS];
#endif
#if P0312_VERT_PHASE_ADJ
  Bool                    m_vertPhasePositionEnableFlag[MAX_LAYERS];
#endif
#if POC_RESET_FLAG || POC_RESET_IDC_ENCODER
  Int                     m_pocAdjustmentValue;
#endif
#if NO_CLRAS_OUTPUT_FLAG
  Bool                    m_noClrasOutputFlag;
  Bool                    m_layerInitializedFlag;
  Bool                    m_firstPicInLayerDecodedFlag;
  Bool                    m_noOutputOfPriorPicsFlags;
#endif
#if O0194_WEIGHTED_PREDICTION_CGS
  Bool                    m_interLayerWeightedPredFlag;
#endif
#if Q0078_ADD_LAYER_SETS
  int                     m_numAddLayerSets;
#endif
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  Bool                    m_pocDecrementedInDPBFlag;
  Int                     m_currPocMsb;
#endif
#endif //SVC_EXTENSION
protected:
  Void  xGetNewPicBuffer  ( TComPic*& rpcPic );           ///< get picture buffer which will be processed
  Void  xInitSPS          ();                             ///< initialize SPS from encoder options
  Void  xInitPPS          ();                             ///< initialize PPS from encoder options
  
  Void  xInitPPSforTiles  ();
  Void  xInitRPS          (Bool isFieldCoding);           ///< initialize PPS from encoder options
#if SVC_EXTENSION
  Void  xInitILRP();
#endif
public:
  TEncTop();
  virtual ~TEncTop();
  
  Void      create          ();
  Void      destroy         ();
  Void      init            (Bool isFieldCoding);
  Void      deletePicBuffer ();

  Void      createWPPCoders(Int iNumSubstreams);
  
  // -------------------------------------------------------------------------------------------------------------------
  // member access functions
  // -------------------------------------------------------------------------------------------------------------------
  
  TComList<TComPic*>*     getListPic            () { return  &m_cListPic;             }
  TEncSearch*             getPredSearch         () { return  &m_cSearch;              }
  
  TComTrQuant*            getTrQuant            () { return  &m_cTrQuant;             }
  TComLoopFilter*         getLoopFilter         () { return  &m_cLoopFilter;          }
  TEncSampleAdaptiveOffset* getSAO              () { return  &m_cEncSAO;              }
  TEncGOP*                getGOPEncoder         () { return  &m_cGOPEncoder;          }
  TEncSlice*              getSliceEncoder       () { return  &m_cSliceEncoder;        }
  TEncCu*                 getCuEncoder          () { return  &m_cCuEncoder;           }
  TEncEntropy*            getEntropyCoder       () { return  &m_cEntropyCoder;        }
  TEncCavlc*              getCavlcCoder         () { return  &m_cCavlcCoder;          }
  TEncSbac*               getSbacCoder          () { return  &m_cSbacCoder;           }
  TEncBinCABAC*           getBinCABAC           () { return  &m_cBinCoderCABAC;       }
  TEncSbac*               getSbacCoders     () { return  m_pcSbacCoders;      }
  TEncBinCABAC*           getBinCABACs          () { return  m_pcBinCoderCABACs;      }
  
  TComBitCounter*         getBitCounter         () { return  &m_cBitCounter;          }
  TComRdCost*             getRdCost             () { return  &m_cRdCost;              }
  TEncSbac***             getRDSbacCoder        () { return  m_pppcRDSbacCoder;       }
  TEncSbac*               getRDGoOnSbacCoder    () { return  &m_cRDGoOnSbacCoder;     }
  TComBitCounter*         getBitCounters        () { return  m_pcBitCounters;         }
  TComRdCost*             getRdCosts            () { return  m_pcRdCosts;             }
  TEncSbac****            getRDSbacCoders       () { return  m_ppppcRDSbacCoders;     }
  TEncSbac*               getRDGoOnSbacCoders   () { return  m_pcRDGoOnSbacCoders;   }
  TEncRateCtrl*           getRateCtrl           () { return &m_cRateCtrl;             }
  TComSPS*                getSPS                () { return  &m_cSPS;                 }
  TComPPS*                getPPS                () { return  &m_cPPS;                 }
  Void selectReferencePictureSet(TComSlice* slice, Int POCCurr, Int GOPid );
  Int getReferencePictureSetIdxForSOP(TComSlice* slice, Int POCCurr, Int GOPid );
  TComScalingList*        getScalingList        () { return  &m_scalingList;         }
  // -------------------------------------------------------------------------------------------------------------------
  // encoder function
  // -------------------------------------------------------------------------------------------------------------------

  /// encode several number of pictures until end-of-sequence
#if SVC_EXTENSION
  Void      setLayerEnc(TEncTop** p)            { m_ppcTEncTop = p;                  }
  TEncTop** getLayerEnc()                       { return m_ppcTEncTop;               }
  Int       getPOCLast            ()            { return m_iPOCLast;                 }
  Int       getNumPicRcvd         ()            { return m_iNumPicRcvd;              }
  Void      setNumPicRcvd         ( Int num )   { m_iNumPicRcvd = num;               }
  Void      setNumScaledRefLayerOffsets(Int x)  { m_numScaledRefLayerOffsets = x;    }
  UInt      getNumScaledRefLayerOffsets()       { return m_numScaledRefLayerOffsets; }
#if O0098_SCALED_REF_LAYER_ID
  Void      setScaledRefLayerId(Int x, UInt id) { m_scaledRefLayerId[x] = id;   }
  UInt      getScaledRefLayerId(Int x)          { return m_scaledRefLayerId[x]; }
  Window&   getScaledRefLayerWindowForLayer(Int layerId);
#endif
  Window&   getScaledRefLayerWindow(Int x)                 { return m_scaledRefLayerWindow[x];        }
#if REF_REGION_OFFSET
  Void      setNumRefLayerOffsets(Int x) { m_numRefLayerOffsets = x; }
  UInt      getNumRefLayerOffsets() { return m_numRefLayerOffsets; }
  Void      setRefLayerId(Int x, UInt id) { m_refLayerId[x] = id;   }
  UInt      getRefLayerId(Int x)          { return m_refLayerId[x]; }
  Window&   getRefLayerWindowForLayer(Int layerId);
  Window&   getRefLayerWindow(Int x)            { return m_refLayerWindow[x]; }
  Bool      getScaledRefLayerOffsetPresentFlag(Int x) { return m_scaledRefLayerOffsetPresentFlag[x]; }
  Void      setScaledRefLayerOffsetPresentFlag(Int x, Bool b) { m_scaledRefLayerOffsetPresentFlag[x] = b; }
  Bool      getRefRegionOffsetPresentFlag(Int x) { return m_refRegionOffsetPresentFlag[x]; }
  Void      setRefRegionOffsetPresentFlag(Int x, Bool b) { m_refRegionOffsetPresentFlag[x] = b; }
#endif
#if P0312_VERT_PHASE_ADJ
  Void      setVertPhasePositionEnableFlag(Int x, Bool b)  { m_vertPhasePositionEnableFlag[x] = b;    }
  UInt      getVertPhasePositionEnableFlag(Int x)          { return m_vertPhasePositionEnableFlag[x]; }
#endif
#if R0209_GENERIC_PHASE
  Int getPhaseHorLuma(Int x) { return m_phaseHorLuma[x]; }
  Int getPhaseVerLuma(Int x) { return m_phaseVerLuma[x]; }
  Int getPhaseHorChroma(Int x) { return m_phaseHorChroma[x]; }
  Int getPhaseVerChroma(Int x) { return m_phaseVerChroma[x]; }
  Void setPhaseHorLuma(Int x, Int val) { m_phaseHorLuma[x] = val; }
  Void setPhaseVerLuma(Int x, Int val) { m_phaseVerLuma[x] = val; }
  Void setPhaseHorChroma(Int x, Int val) { m_phaseHorChroma[x] = val; }
  Void setPhaseVerChroma(Int x, Int val) { m_phaseVerChroma[x] = val; }
  Bool getResamplePhaseSetPresentFlag(Int x) { return m_resamplePhaseSetPresentFlag[x]; }
  Void setResamplePhaseSetPresentFlag(Int x, Bool b) { m_resamplePhaseSetPresentFlag[x] = b; }
#endif

  TComPic** getIlpList() { return m_cIlpPic; }
#if REF_IDX_MFM
  Void      setMFMEnabledFlag       (Bool flag)   { m_bMFMEnabledFlag = flag; }
  Bool      getMFMEnabledFlag()                   { return m_bMFMEnabledFlag; }    
#endif
#if O0194_WEIGHTED_PREDICTION_CGS
  Void      setInterLayerWeightedPredFlag(Bool flag)   { m_interLayerWeightedPredFlag = flag; }
  Bool      getInterLayerWeightedPredFlag()            { return m_interLayerWeightedPredFlag; }
#endif
  Void      encode( TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut, Int iPicIdInGOP );
  Void      encodePrep( TComPicYuv* pcPicYuvOrg );
  Void      encode( TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut, Int iPicIdInGOP, Bool isTff );
  Void      encodePrep( TComPicYuv* pcPicYuvOrg, Bool isTff );
#if VPS_EXTN_DIRECT_REF_LAYERS
  TEncTop*  getRefLayerEnc(UInt refLayerIdc);
#endif
#if POC_RESET_FLAG || POC_RESET_IDC_ENCODER
  Int       getPocAdjustmentValue()      { return m_pocAdjustmentValue;}
  Void      setPocAdjustmentValue(Int x) { m_pocAdjustmentValue = x;   }
#endif
#if NO_CLRAS_OUTPUT_FLAG
  Int  getNoClrasOutputFlag()                { return m_noClrasOutputFlag;}
  Void setNoClrasOutputFlag(Bool x)          { m_noClrasOutputFlag = x;   }
  Int  getLayerInitializedFlag()             { return m_layerInitializedFlag;}
  Void setLayerInitializedFlag(Bool x)       { m_layerInitializedFlag = x;   }
  Int  getFirstPicInLayerDecodedFlag()       { return m_firstPicInLayerDecodedFlag;}
  Void setFirstPicInLayerDecodedFlag(Bool x) { m_firstPicInLayerDecodedFlag = x;   }
  Int  getNoOutputOfPriorPicsFlags()         { return m_noOutputOfPriorPicsFlags;}
  Void setNoOutputOfPriorPicsFlags(Bool x)   { m_noOutputOfPriorPicsFlags = x;   }
#endif
#if Q0078_ADD_LAYER_SETS
  Void setNumAddLayerSets(Int x)             { m_numAddLayerSets = x; }
  Int  getNumAddLayerSets()                  { return m_numAddLayerSets; }
#endif
#if P0297_VPS_POC_LSB_ALIGNED_FLAG
  Void setPocDecrementedInDPBFlag(Bool x)    { m_pocDecrementedInDPBFlag = x; }
  Bool getPocDecrementedInDPBFlag()          { return m_pocDecrementedInDPBFlag; }
  Void setCurrPocMsb(Int poc)                { m_currPocMsb = poc; }
  Int  getCurrPocMsb()                       { return m_currPocMsb; }
#endif
#else //SVC_EXTENSION
  Void encode( Bool bEos, TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut,
              std::list<AccessUnit>& accessUnitsOut, Int& iNumEncoded );

  /// encode several number of pictures until end-of-sequence
  Void encode( Bool bEos, TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut,
              std::list<AccessUnit>& accessUnitsOut, Int& iNumEncoded, Bool isTff);

  Void printSummary(Bool isField) { m_cGOPEncoder.printOutSummary (m_uiNumAllPicCoded, isField); }
#endif //#if SVC_EXTENSION
};

//! \}

#endif // __TENCTOP__

