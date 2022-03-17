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

/** \file     TEncCfg.h
    \brief    encoder configuration class (header)
*/

#ifndef __TENCCFG__
#define __TENCCFG__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComSlice.h"
#include <assert.h>

struct GOPEntry
{
  Int m_POC;
  Int m_QPOffset;
  Double m_QPFactor;
  Int m_tcOffsetDiv2;
  Int m_betaOffsetDiv2;
  Int m_temporalId;
  Bool m_refPic;
  Int m_numRefPicsActive;
  Char m_sliceType;
  Int m_numRefPics;
  Int m_referencePics[MAX_NUM_REF_PICS];
  Int m_usedByCurrPic[MAX_NUM_REF_PICS];
#if AUTO_INTER_RPS
  Int m_interRPSPrediction;
#else
  Bool m_interRPSPrediction;
#endif
  Int m_deltaRPS;
  Int m_numRefIdc;
  Int m_refIdc[MAX_NUM_REF_PICS+1];
  GOPEntry()
  : m_POC(-1)
  , m_QPOffset(0)
  , m_QPFactor(0)
  , m_tcOffsetDiv2(0)
  , m_betaOffsetDiv2(0)
  , m_temporalId(0)
  , m_refPic(false)
  , m_numRefPicsActive(0)
  , m_sliceType('P')
  , m_numRefPics(0)
  , m_interRPSPrediction(false)
  , m_deltaRPS(0)
  , m_numRefIdc(0)
  {
    ::memset( m_referencePics, 0, sizeof(m_referencePics) );
    ::memset( m_usedByCurrPic, 0, sizeof(m_usedByCurrPic) );
    ::memset( m_refIdc,        0, sizeof(m_refIdc) );
  }
};

std::istringstream &operator>>(std::istringstream &in, GOPEntry &entry);     //input
//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder configuration class
class TEncCfg
{
protected:
  //==== File I/O ========
  Int       m_iFrameRate;
  Int       m_FrameSkip;
  Int       m_iSourceWidth;
  Int       m_iSourceHeight;
  Window    m_conformanceWindow;
  Int       m_framesToBeEncoded;
  Double    m_adLambdaModifier[ MAX_TLAYER ];

  /* profile & level */
  Profile::Name m_profile;
  Level::Tier   m_levelTier;
  Level::Name   m_level;
  Bool m_progressiveSourceFlag;
  Bool m_interlacedSourceFlag;
  Bool m_nonPackedConstraintFlag;
  Bool m_frameOnlyConstraintFlag;

  //====== Coding Structure ========
  UInt      m_uiIntraPeriod;
  UInt      m_uiDecodingRefreshType;            ///< the type of decoding refresh employed for the random access.
  Int       m_iGOPSize;
  GOPEntry  m_GOPList[MAX_GOP];
  Int       m_extraRPSs;
  Int       m_maxDecPicBuffering[MAX_TLAYER];
  Int       m_numReorderPics[MAX_TLAYER];
  
  Int       m_iQP;                              //  if (AdaptiveQP == OFF)
  
  Int       m_aiPad[2];
  

  Int       m_iMaxRefPicNum;                     ///< this is used to mimic the sliding mechanism used by the decoder
                                                 // TODO: We need to have a common sliding mechanism used by both the encoder and decoder

  Int       m_maxTempLayer;                      ///< Max temporal layer
  Bool m_useAMP;
  //======= Transform =============
  UInt      m_uiQuadtreeTULog2MaxSize;
  UInt      m_uiQuadtreeTULog2MinSize;
  UInt      m_uiQuadtreeTUMaxDepthInter;
  UInt      m_uiQuadtreeTUMaxDepthIntra;
  
  //====== Loop/Deblock Filter ========
  Bool      m_bLoopFilterDisable;
  Bool      m_loopFilterOffsetInPPS;
  Int       m_loopFilterBetaOffsetDiv2;
  Int       m_loopFilterTcOffsetDiv2;
  Bool      m_DeblockingFilterControlPresent;
  Bool      m_DeblockingFilterMetric;
  Bool      m_bUseSAO;
  Int       m_maxNumOffsetsPerPic;
  Bool      m_saoLcuBoundary;
  //====== Motion search ========
  Int       m_iFastSearch;                      //  0:Full search  1:Diamond  2:PMVFAST
  Int       m_iSearchRange;                     //  0:Full frame
  Int       m_bipredSearchRange;

  //====== Quality control ========
  Int       m_iMaxDeltaQP;                      //  Max. absolute delta QP (1:default)
  Int       m_iMaxCuDQPDepth;                   //  Max. depth for a minimum CuDQP (0:default)

  Int       m_chromaCbQpOffset;                 //  Chroma Cb QP Offset (0:default)
  Int       m_chromaCrQpOffset;                 //  Chroma Cr Qp Offset (0:default)

#if ADAPTIVE_QP_SELECTION
  Bool      m_bUseAdaptQpSelect;
#endif

  Bool      m_bUseAdaptiveQP;
  Int       m_iQPAdaptationRange;
  
  //====== Tool list ========
  Bool      m_bUseASR;
  Bool      m_bUseHADME;
  Bool      m_useRDOQ;
  Bool      m_useRDOQTS;
  UInt      m_rdPenalty;
  Bool      m_bUseFastEnc;
  Bool      m_bUseEarlyCU;
  Bool      m_useFastDecisionForMerge;
  Bool      m_bUseCbfFastMode;
  Bool      m_useEarlySkipDetection;
  Bool      m_useTransformSkip;
  Bool      m_useTransformSkipFast;
  Int*      m_aidQP;
  UInt      m_uiDeltaQpRD;
  
  Bool      m_bUseConstrainedIntraPred;
  Bool      m_usePCM;
  UInt      m_pcmLog2MaxSize;
  UInt      m_uiPCMLog2MinSize;
  //====== Slice ========
  Int       m_sliceMode;
  Int       m_sliceArgument; 
  //====== Dependent Slice ========
  Int       m_sliceSegmentMode;
  Int       m_sliceSegmentArgument;
  Bool      m_bLFCrossSliceBoundaryFlag;

  Bool      m_bPCMInputBitDepthFlag;
  UInt      m_uiPCMBitDepthLuma;
  UInt      m_uiPCMBitDepthChroma;
  Bool      m_bPCMFilterDisableFlag;
  Bool      m_loopFilterAcrossTilesEnabledFlag;
  Bool      m_tileUniformSpacingFlag;
  Int       m_iNumColumnsMinus1;
  Int       m_iNumRowsMinus1;
  std::vector<Int> m_tileColumnWidth;
  std::vector<Int> m_tileRowHeight;

  Int       m_iWaveFrontSynchro;
  Int       m_iWaveFrontSubstreams;

  Int       m_decodedPictureHashSEIEnabled;              ///< Checksum(3)/CRC(2)/MD5(1)/disable(0) acting on decoded picture hash SEI message
  Int       m_bufferingPeriodSEIEnabled;
  Int       m_pictureTimingSEIEnabled;
  Int       m_recoveryPointSEIEnabled;
  Bool      m_toneMappingInfoSEIEnabled;
  Int       m_toneMapId;
  Bool      m_toneMapCancelFlag;
  Bool      m_toneMapPersistenceFlag;
  Int       m_codedDataBitDepth;
  Int       m_targetBitDepth;
  Int       m_modelId; 
  Int       m_minValue;
  Int       m_maxValue;
  Int       m_sigmoidMidpoint;
  Int       m_sigmoidWidth;
  Int       m_numPivots;
  Int       m_cameraIsoSpeedIdc;
  Int       m_cameraIsoSpeedValue;
  Int       m_exposureIndexIdc;
  Int       m_exposureIndexValue;
  Int       m_exposureCompensationValueSignFlag;
  Int       m_exposureCompensationValueNumerator;
  Int       m_exposureCompensationValueDenomIdc;
  Int       m_refScreenLuminanceWhite;
  Int       m_extendedRangeWhiteLevel;
  Int       m_nominalBlackLevelLumaCodeValue;
  Int       m_nominalWhiteLevelLumaCodeValue;
  Int       m_extendedWhiteLevelLumaCodeValue;
  Int*      m_startOfCodedInterval;
  Int*      m_codedPivotValue;
  Int*      m_targetPivotValue;
  Int       m_framePackingSEIEnabled;
  Int       m_framePackingSEIType;
  Int       m_framePackingSEIId;
  Int       m_framePackingSEIQuincunx;
  Int       m_framePackingSEIInterpretation;
  Int       m_displayOrientationSEIAngle;
  Int       m_temporalLevel0IndexSEIEnabled;
  Int       m_gradualDecodingRefreshInfoEnabled;
  Int       m_decodingUnitInfoSEIEnabled;
  Int       m_SOPDescriptionSEIEnabled;
  Int       m_scalableNestingSEIEnabled;
  //====== Weighted Prediction ========
  Bool      m_useWeightedPred;       //< Use of Weighting Prediction (P_SLICE)
  Bool      m_useWeightedBiPred;    //< Use of Bi-directional Weighting Prediction (B_SLICE)
  UInt      m_log2ParallelMergeLevelMinus2;       ///< Parallel merge estimation region
  UInt      m_maxNumMergeCand;                    ///< Maximum number of merge candidates
  Int       m_useScalingListId;            ///< Using quantization matrix i.e. 0=off, 1=default, 2=file.
  Char*     m_scalingListFile;          ///< quantization matrix file name
  Int       m_TMVPModeId;
  Int       m_signHideFlag;
  Bool      m_RCEnableRateControl;
  Int       m_RCTargetBitrate;
  Int       m_RCKeepHierarchicalBit;
  Bool      m_RCLCULevelRC;
  Bool      m_RCUseLCUSeparateModel;
  Int       m_RCInitialQP;
  Bool      m_RCForceIntraQP;
  Bool      m_TransquantBypassEnableFlag;                     ///< transquant_bypass_enable_flag setting in PPS.
  Bool      m_CUTransquantBypassFlagForce;                    ///< if transquant_bypass_enable_flag, then, if true, all CU transquant bypass flags will be set to true.
#if SVC_EXTENSION
  static TComVPS             m_cVPS;
#else
  TComVPS                    m_cVPS;
#endif
  Bool      m_recalculateQPAccordingToLambda;                 ///< recalculate QP value according to the lambda value
  Int       m_activeParameterSetsSEIEnabled;                  ///< enable active parameter set SEI message 
  Bool      m_vuiParametersPresentFlag;                       ///< enable generation of VUI parameters
  Bool      m_aspectRatioInfoPresentFlag;                     ///< Signals whether aspect_ratio_idc is present
  Int       m_aspectRatioIdc;                                 ///< aspect_ratio_idc
  Int       m_sarWidth;                                       ///< horizontal size of the sample aspect ratio
  Int       m_sarHeight;                                      ///< vertical size of the sample aspect ratio
  Bool      m_overscanInfoPresentFlag;                        ///< Signals whether overscan_appropriate_flag is present
  Bool      m_overscanAppropriateFlag;                        ///< Indicates whether conformant decoded pictures are suitable for display using overscan
  Bool      m_videoSignalTypePresentFlag;                     ///< Signals whether video_format, video_full_range_flag, and colour_description_present_flag are present
  Int       m_videoFormat;                                    ///< Indicates representation of pictures
  Bool      m_videoFullRangeFlag;                             ///< Indicates the black level and range of luma and chroma signals
  Bool      m_colourDescriptionPresentFlag;                   ///< Signals whether colour_primaries, transfer_characteristics and matrix_coefficients are present
  Int       m_colourPrimaries;                                ///< Indicates chromaticity coordinates of the source primaries
  Int       m_transferCharacteristics;                        ///< Indicates the opto-electronic transfer characteristics of the source
  Int       m_matrixCoefficients;                             ///< Describes the matrix coefficients used in deriving luma and chroma from RGB primaries
  Bool      m_chromaLocInfoPresentFlag;                       ///< Signals whether chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field are present
  Int       m_chromaSampleLocTypeTopField;                    ///< Specifies the location of chroma samples for top field
  Int       m_chromaSampleLocTypeBottomField;                 ///< Specifies the location of chroma samples for bottom field
  Bool      m_neutralChromaIndicationFlag;                    ///< Indicates that the value of all decoded chroma samples is equal to 1<<(BitDepthCr-1)
  Window    m_defaultDisplayWindow;                           ///< Represents the default display window parameters
  Bool      m_frameFieldInfoPresentFlag;                      ///< Indicates that pic_struct and other field coding related values are present in picture timing SEI messages
  Bool      m_pocProportionalToTimingFlag;                    ///< Indicates that the POC value is proportional to the output time w.r.t. first picture in CVS
  Int       m_numTicksPocDiffOneMinus1;                       ///< Number of ticks minus 1 that for a POC difference of one
  Bool      m_bitstreamRestrictionFlag;                       ///< Signals whether bitstream restriction parameters are present
  Bool      m_tilesFixedStructureFlag;                        ///< Indicates that each active picture parameter set has the same values of the syntax elements related to tiles
  Bool      m_motionVectorsOverPicBoundariesFlag;             ///< Indicates that no samples outside the picture boundaries are used for inter prediction
  Int       m_minSpatialSegmentationIdc;                      ///< Indicates the maximum size of the spatial segments in the pictures in the coded video sequence
  Int       m_maxBytesPerPicDenom;                            ///< Indicates a number of bytes not exceeded by the sum of the sizes of the VCL NAL units associated with any coded picture
  Int       m_maxBitsPerMinCuDenom;                           ///< Indicates an upper bound for the number of bits of coding_unit() data
  Int       m_log2MaxMvLengthHorizontal;                      ///< Indicate the maximum absolute value of a decoded horizontal MV component in quarter-pel luma units
  Int       m_log2MaxMvLengthVertical;                        ///< Indicate the maximum absolute value of a decoded vertical MV component in quarter-pel luma units

  Bool      m_useStrongIntraSmoothing;                        ///< enable the use of strong intra smoothing (bi_linear interpolation) for 32x32 blocks when reference samples are flat.
#if SVC_EXTENSION
  UInt      m_layerId;   
  UInt      m_numLayer;
  Int       m_elRapSliceBEnabled;
#if M0040_ADAPTIVE_RESOLUTION_CHANGE
  Int       m_adaptiveResolutionChange;
#endif
#if O0153_ALT_OUTPUT_LAYER_FLAG
  Bool      m_altOutputLayerFlag;
#endif
#if HIGHER_LAYER_IRAP_SKIP_FLAG
  Int       m_skipPictureAtArcSwitch;
#endif
#if O0149_CROSS_LAYER_BLA_FLAG
  Bool      m_crossLayerBLAFlag;
#endif
#if VPS_EXTN_DIRECT_REF_LAYERS
  Int       m_numDirectRefLayers;
  Int       m_refLayerId[MAX_VPS_LAYER_ID_PLUS1];

  Int       m_numActiveRefLayers;
  Int       m_predLayerId[MAX_VPS_LAYER_ID_PLUS1];
  Int       m_numSamplePredRefLayers;
  Int       m_samplePredRefLayerId[MAX_VPS_LAYER_ID_PLUS1];
  Int       m_numMotionPredRefLayers;
  Int       m_motionPredRefLayerId[MAX_VPS_LAYER_ID_PLUS1];
  Bool      m_samplePredEnabledFlag[MAX_VPS_LAYER_ID_PLUS1];
  Bool      m_motionPredEnabledFlag[MAX_VPS_LAYER_ID_PLUS1];
#endif
  Int       m_maxTidIlRefPicsPlus1;
#if AUXILIARY_PICTURES
  ChromaFormat m_chromaFormatIDC;
#endif
#if FAST_INTRA_SHVC
  Bool      m_useFastIntraScalable;
#endif
#if LAYERS_NOT_PRESENT_SEI
  Int       m_layersNotPresentSEIEnabled;
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
  Bool      m_interLayerConstrainedTileSetsSEIEnabled;
  UInt      m_ilNumSetsInMessage;
  Bool      m_skippedTileSetPresentFlag;
  UInt      m_topLeftTileIndex[1024];
  UInt      m_bottomRightTileIndex[1024];
  UInt      m_ilcIdc[1024];
#endif
#if Q0048_CGS_3D_ASYMLUT
  Int  m_nCGSFlag;
  Int  m_nCGSMaxOctantDepth;
  Int  m_nCGSMaxYPartNumLog2;
  Int  m_nCGSLUTBit;
#if R0151_CGS_3D_ASYMLUT_IMPROVE
  Int  m_nCGSAdaptiveChroma;
#endif
#if R0179_ENC_OPT_3DLUT_SIZE
  Int  m_nCGSLutSizeRDO;
#endif
#endif
#if P0050_KNEE_FUNCTION_SEI
  Bool      m_kneeSEIEnabled;
  Int       m_kneeSEIId;
  Bool      m_kneeSEICancelFlag;
  Bool      m_kneeSEIPersistenceFlag;
  Bool      m_kneeSEIMappingFlag;
  Int       m_kneeSEIInputDrange;
  Int       m_kneeSEIInputDispLuminance;
  Int       m_kneeSEIOutputDrange;
  Int       m_kneeSEIOutputDispLuminance;
  Int       m_kneeSEINumKneePointsMinus1;
  Int*      m_kneeSEIInputKneePoint;
  Int*      m_kneeSEIOutputKneePoint;
#endif
#if Q0189_TMVP_CONSTRAINTS
  Int       m_TMVPConstraintsSEIEnabled;
#endif
#endif //SVC_EXTENSION
#if Q0074_COLOUR_REMAPPING_SEI
  Char*     m_colourRemapSEIFile;          ///< SEI Colour Remapping File (initialized from external file)
  Int       m_colourRemapSEIId;
  Bool      m_colourRemapSEICancelFlag;
  Bool      m_colourRemapSEIPersistenceFlag;
  Bool      m_colourRemapSEIVideoSignalInfoPresentFlag;
  Bool      m_colourRemapSEIFullRangeFlag;
  Int       m_colourRemapSEIPrimaries;
  Int       m_colourRemapSEITransferFunction;
  Int       m_colourRemapSEIMatrixCoefficients;
  Int       m_colourRemapSEIInputBitDepth;
  Int       m_colourRemapSEIBitDepth;
  Int       m_colourRemapSEIPreLutNumValMinus1[3];
  Int*      m_colourRemapSEIPreLutCodedValue[3];
  Int*      m_colourRemapSEIPreLutTargetValue[3];
  Bool      m_colourRemapSEIMatrixPresentFlag;
  Int       m_colourRemapSEILog2MatrixDenom;
  Int       m_colourRemapSEICoeffs[3][3];
  Int       m_colourRemapSEIPostLutNumValMinus1[3];
  Int*      m_colourRemapSEIPostLutCodedValue[3];
  Int*      m_colourRemapSEIPostLutTargetValue[3];
#endif

public:
  TEncCfg()
  : m_tileColumnWidth()
  , m_tileRowHeight()
  {}

  virtual ~TEncCfg()
  {}
  
  Void setProfile(Profile::Name profile) { m_profile = profile; }
  Void setLevel(Level::Tier tier, Level::Name level) { m_levelTier = tier; m_level = level; }

  Void      setFrameRate                    ( Int   i )      { m_iFrameRate = i; }
  Void      setFrameSkip                    ( UInt i ) { m_FrameSkip = i; }
  Void      setSourceWidth                  ( Int   i )      { m_iSourceWidth = i; }
  Void      setSourceHeight                 ( Int   i )      { m_iSourceHeight = i; }

  Window   &getConformanceWindow()                           { return m_conformanceWindow; }
#if P0312_VERT_PHASE_ADJ && !R0209_GENERIC_PHASE
  Void      setConformanceWindow (Int confLeft, Int confRight, Int confTop, Int confBottom ) { m_conformanceWindow.setWindow (confLeft, confRight, confTop, confBottom, false); }
#else
  Void      setConformanceWindow (Int confLeft, Int confRight, Int confTop, Int confBottom ) { m_conformanceWindow.setWindow (confLeft, confRight, confTop, confBottom); }
#endif

  Void      setFramesToBeEncoded            ( Int   i )      { m_framesToBeEncoded = i; }
  
  //====== Coding Structure ========
  Void      setIntraPeriod                  ( Int   i )      { m_uiIntraPeriod = (UInt)i; }
  Void      setDecodingRefreshType          ( Int   i )      { m_uiDecodingRefreshType = (UInt)i; }
  Void      setGOPSize                      ( Int   i )      { m_iGOPSize = i; }
  Void      setGopList                      ( GOPEntry*  GOPList ) {  for ( Int i = 0; i < MAX_GOP; i++ ) m_GOPList[i] = GOPList[i]; }
  Void      setExtraRPSs                    ( Int   i )      { m_extraRPSs = i; }
  GOPEntry  getGOPEntry                     ( Int   i )      { return m_GOPList[i]; }
  Void      setMaxDecPicBuffering           ( UInt u, UInt tlayer ) { m_maxDecPicBuffering[tlayer] = u;    }
  Void      setNumReorderPics               ( Int  i, UInt tlayer ) { m_numReorderPics[tlayer] = i;    }
  
  Void      setQP                           ( Int   i )      { m_iQP = i; }
  
  Void      setPad                          ( Int*  iPad                   )      { for ( Int i = 0; i < 2; i++ ) m_aiPad[i] = iPad[i]; }
  
  Int       getMaxRefPicNum                 ()                              { return m_iMaxRefPicNum;           }
  Void      setMaxRefPicNum                 ( Int iMaxRefPicNum )           { m_iMaxRefPicNum = iMaxRefPicNum;  }

  Int       getMaxTempLayer                 ()                              { return m_maxTempLayer;              } 
  Void      setMaxTempLayer                 ( Int maxTempLayer )            { m_maxTempLayer = maxTempLayer;      }
  //======== Transform =============
  Void      setQuadtreeTULog2MaxSize        ( UInt  u )      { m_uiQuadtreeTULog2MaxSize = u; }
  Void      setQuadtreeTULog2MinSize        ( UInt  u )      { m_uiQuadtreeTULog2MinSize = u; }
  Void      setQuadtreeTUMaxDepthInter      ( UInt  u )      { m_uiQuadtreeTUMaxDepthInter = u; }
  Void      setQuadtreeTUMaxDepthIntra      ( UInt  u )      { m_uiQuadtreeTUMaxDepthIntra = u; }
  
  Void setUseAMP( Bool b ) { m_useAMP = b; }
  
  //====== Loop/Deblock Filter ========
  Void      setLoopFilterDisable            ( Bool  b )      { m_bLoopFilterDisable       = b; }
  Void      setLoopFilterOffsetInPPS        ( Bool  b )      { m_loopFilterOffsetInPPS      = b; }
  Void      setLoopFilterBetaOffset         ( Int   i )      { m_loopFilterBetaOffsetDiv2  = i; }
  Void      setLoopFilterTcOffset           ( Int   i )      { m_loopFilterTcOffsetDiv2    = i; }
  Void      setDeblockingFilterControlPresent ( Bool b ) { m_DeblockingFilterControlPresent = b; }
  Void      setDeblockingFilterMetric       ( Bool  b )      { m_DeblockingFilterMetric = b; }

  //====== Motion search ========
  Void      setFastSearch                   ( Int   i )      { m_iFastSearch = i; }
  Void      setSearchRange                  ( Int   i )      { m_iSearchRange = i; }
  Void      setBipredSearchRange            ( Int   i )      { m_bipredSearchRange = i; }

  //====== Quality control ========
  Void      setMaxDeltaQP                   ( Int   i )      { m_iMaxDeltaQP = i; }
  Void      setMaxCuDQPDepth                ( Int   i )      { m_iMaxCuDQPDepth = i; }

  Void      setChromaCbQpOffset             ( Int   i )      { m_chromaCbQpOffset = i; }
  Void      setChromaCrQpOffset             ( Int   i )      { m_chromaCrQpOffset = i; }

#if ADAPTIVE_QP_SELECTION
  Void      setUseAdaptQpSelect             ( Bool   i ) { m_bUseAdaptQpSelect    = i; }
  Bool      getUseAdaptQpSelect             ()           { return   m_bUseAdaptQpSelect; }
#endif

  Void      setUseAdaptiveQP                ( Bool  b )      { m_bUseAdaptiveQP = b; }
  Void      setQPAdaptationRange            ( Int   i )      { m_iQPAdaptationRange = i; }
  
  //====== Sequence ========
  Int       getFrameRate                    ()      { return  m_iFrameRate; }
  UInt      getFrameSkip                    ()      { return  m_FrameSkip; }
  Int       getSourceWidth                  ()      { return  m_iSourceWidth; }
  Int       getSourceHeight                 ()      { return  m_iSourceHeight; }
  Int       getFramesToBeEncoded            ()      { return  m_framesToBeEncoded; }
  void setLambdaModifier                    ( UInt uiIndex, Double dValue ) { m_adLambdaModifier[ uiIndex ] = dValue; }
  Double getLambdaModifier                  ( UInt uiIndex ) const { return m_adLambdaModifier[ uiIndex ]; }

  //==== Coding Structure ========
  UInt      getIntraPeriod                  ()      { return  m_uiIntraPeriod; }
  UInt      getDecodingRefreshType          ()      { return  m_uiDecodingRefreshType; }
  Int       getGOPSize                      ()      { return  m_iGOPSize; }
  Int       getMaxDecPicBuffering           (UInt tlayer) { return m_maxDecPicBuffering[tlayer]; }
  Int       getNumReorderPics               (UInt tlayer) { return m_numReorderPics[tlayer]; }
  Int       getQP                           ()      { return  m_iQP; }
  
  Int       getPad                          ( Int i )      { assert (i < 2 );                      return  m_aiPad[i]; }
  
  //======== Transform =============
  UInt      getQuadtreeTULog2MaxSize        ()      const { return m_uiQuadtreeTULog2MaxSize; }
  UInt      getQuadtreeTULog2MinSize        ()      const { return m_uiQuadtreeTULog2MinSize; }
  UInt      getQuadtreeTUMaxDepthInter      ()      const { return m_uiQuadtreeTUMaxDepthInter; }
  UInt      getQuadtreeTUMaxDepthIntra      ()      const { return m_uiQuadtreeTUMaxDepthIntra; }
  
  //==== Loop/Deblock Filter ========
  Bool      getLoopFilterDisable            ()      { return  m_bLoopFilterDisable;       }
  Bool      getLoopFilterOffsetInPPS        ()      { return m_loopFilterOffsetInPPS; }
  Int       getLoopFilterBetaOffset         ()      { return m_loopFilterBetaOffsetDiv2; }
  Int       getLoopFilterTcOffset           ()      { return m_loopFilterTcOffsetDiv2; }
  Bool      getDeblockingFilterControlPresent()  { return  m_DeblockingFilterControlPresent; }
  Bool      getDeblockingFilterMetric       ()      { return m_DeblockingFilterMetric; }

  //==== Motion search ========
  Int       getFastSearch                   ()      { return  m_iFastSearch; }
  Int       getSearchRange                  ()      { return  m_iSearchRange; }

  //==== Quality control ========
  Int       getMaxDeltaQP                   ()      { return  m_iMaxDeltaQP; }
  Int       getMaxCuDQPDepth                ()      { return  m_iMaxCuDQPDepth; }
  Bool      getUseAdaptiveQP                ()      { return  m_bUseAdaptiveQP; }
  Int       getQPAdaptationRange            ()      { return  m_iQPAdaptationRange; }
  
  //==== Tool list ========
  Void      setUseASR                       ( Bool  b )     { m_bUseASR     = b; }
  Void      setUseHADME                     ( Bool  b )     { m_bUseHADME   = b; }
  Void      setUseRDOQ                      ( Bool  b )     { m_useRDOQ    = b; }
  Void      setUseRDOQTS                    ( Bool  b )     { m_useRDOQTS  = b; }
  Void      setRDpenalty                 ( UInt  b )     { m_rdPenalty  = b; }
  Void      setUseFastEnc                   ( Bool  b )     { m_bUseFastEnc = b; }
  Void      setUseEarlyCU                   ( Bool  b )     { m_bUseEarlyCU = b; }
  Void      setUseFastDecisionForMerge      ( Bool  b )     { m_useFastDecisionForMerge = b; }
  Void      setUseCbfFastMode            ( Bool  b )     { m_bUseCbfFastMode = b; }
  Void      setUseEarlySkipDetection        ( Bool  b )     { m_useEarlySkipDetection = b; }
  Void      setUseConstrainedIntraPred      ( Bool  b )     { m_bUseConstrainedIntraPred = b; }
  Void      setPCMInputBitDepthFlag         ( Bool  b )     { m_bPCMInputBitDepthFlag = b; }
  Void      setPCMFilterDisableFlag         ( Bool  b )     {  m_bPCMFilterDisableFlag = b; }
  Void      setUsePCM                       ( Bool  b )     {  m_usePCM = b;               }
  Void      setPCMLog2MaxSize               ( UInt u )      { m_pcmLog2MaxSize = u;      }
  Void      setPCMLog2MinSize               ( UInt u )     { m_uiPCMLog2MinSize = u;      }
  Void      setdQPs                         ( Int*  p )     { m_aidQP       = p; }
  Void      setDeltaQpRD                    ( UInt  u )     {m_uiDeltaQpRD  = u; }
  Bool      getUseASR                       ()      { return m_bUseASR;     }
  Bool      getUseHADME                     ()      { return m_bUseHADME;   }
  Bool      getUseRDOQ                      ()      { return m_useRDOQ;    }
  Bool      getUseRDOQTS                    ()      { return m_useRDOQTS;  }
  Int      getRDpenalty                  ()      { return m_rdPenalty;  }
  Bool      getUseFastEnc                   ()      { return m_bUseFastEnc; }
  Bool      getUseEarlyCU                   ()      { return m_bUseEarlyCU; }
  Bool      getUseFastDecisionForMerge      ()      { return m_useFastDecisionForMerge; }
  Bool      getUseCbfFastMode           ()      { return m_bUseCbfFastMode; }
  Bool      getUseEarlySkipDetection        ()      { return m_useEarlySkipDetection; }
  Bool      getUseConstrainedIntraPred      ()      { return m_bUseConstrainedIntraPred; }
  Bool      getPCMInputBitDepthFlag         ()      { return m_bPCMInputBitDepthFlag;   }
  Bool      getPCMFilterDisableFlag         ()      { return m_bPCMFilterDisableFlag;   } 
  Bool      getUsePCM                       ()      { return m_usePCM;                 }
  UInt      getPCMLog2MaxSize               ()      { return m_pcmLog2MaxSize;  }
  UInt      getPCMLog2MinSize               ()      { return  m_uiPCMLog2MinSize;  }

  Bool getUseTransformSkip                             ()      { return m_useTransformSkip;        }
  Void setUseTransformSkip                             ( Bool b ) { m_useTransformSkip  = b;       }
  Bool getUseTransformSkipFast                         ()      { return m_useTransformSkipFast;    }
  Void setUseTransformSkipFast                         ( Bool b ) { m_useTransformSkipFast  = b;   }
  Int*      getdQPs                         ()      { return m_aidQP;       }
  UInt      getDeltaQpRD                    ()      { return m_uiDeltaQpRD; }

  //====== Slice ========
  Void  setSliceMode                   ( Int  i )       { m_sliceMode = i;              }
  Void  setSliceArgument               ( Int  i )       { m_sliceArgument = i;          }
  Int   getSliceMode                   ()              { return m_sliceMode;           }
  Int   getSliceArgument               ()              { return m_sliceArgument;       }
  //====== Dependent Slice ========
  Void  setSliceSegmentMode            ( Int  i )      { m_sliceSegmentMode = i;       }
  Void  setSliceSegmentArgument        ( Int  i )      { m_sliceSegmentArgument = i;   }
  Int   getSliceSegmentMode            ()              { return m_sliceSegmentMode;    }
  Int   getSliceSegmentArgument        ()              { return m_sliceSegmentArgument;}
  Void      setLFCrossSliceBoundaryFlag     ( Bool   bValue  )    { m_bLFCrossSliceBoundaryFlag = bValue; }
  Bool      getLFCrossSliceBoundaryFlag     ()                    { return m_bLFCrossSliceBoundaryFlag;   }

  Void      setUseSAO                  (Bool bVal)     {m_bUseSAO = bVal;}
  Bool      getUseSAO                  ()              {return m_bUseSAO;}
  Void  setMaxNumOffsetsPerPic                   (Int iVal)            { m_maxNumOffsetsPerPic = iVal; }
  Int   getMaxNumOffsetsPerPic                   ()                    { return m_maxNumOffsetsPerPic; }
  Void  setSaoLcuBoundary              (Bool val)      { m_saoLcuBoundary = val; }
  Bool  getSaoLcuBoundary              ()              { return m_saoLcuBoundary; }
  Void  setLFCrossTileBoundaryFlag               ( Bool   val  )       { m_loopFilterAcrossTilesEnabledFlag = val; }
  Bool  getLFCrossTileBoundaryFlag               ()                    { return m_loopFilterAcrossTilesEnabledFlag;   }
  Void  setTileUniformSpacingFlag      ( Bool b )          { m_tileUniformSpacingFlag = b; }
  Bool  getTileUniformSpacingFlag      ()                  { return m_tileUniformSpacingFlag; }
  Void  setNumColumnsMinus1            ( Int i )           { m_iNumColumnsMinus1 = i; }
  Int   getNumColumnsMinus1            ()                  { return m_iNumColumnsMinus1; }
  Void  setColumnWidth ( const std::vector<Int>& columnWidth ) { m_tileColumnWidth = columnWidth; }
  UInt  getColumnWidth                 ( UInt columnIdx )      { return m_tileColumnWidth[columnIdx]; }
  Void  setNumRowsMinus1               ( Int i )           { m_iNumRowsMinus1 = i; }
  Int   getNumRowsMinus1               ()                  { return m_iNumRowsMinus1; }
  Void  setRowHeight ( const std::vector<Int>& rowHeight)      { m_tileRowHeight = rowHeight; }
  UInt  getRowHeight                   ( UInt rowIdx )         { return m_tileRowHeight[rowIdx]; }
  Void  xCheckGSParameters();
  Void  setWaveFrontSynchro(Int iWaveFrontSynchro)       { m_iWaveFrontSynchro = iWaveFrontSynchro; }
  Int   getWaveFrontsynchro()                            { return m_iWaveFrontSynchro; }
  Void  setWaveFrontSubstreams(Int iWaveFrontSubstreams) { m_iWaveFrontSubstreams = iWaveFrontSubstreams; }
  Int   getWaveFrontSubstreams()                         { return m_iWaveFrontSubstreams; }
  Void  setDecodedPictureHashSEIEnabled(Int b)           { m_decodedPictureHashSEIEnabled = b; }
  Int   getDecodedPictureHashSEIEnabled()                { return m_decodedPictureHashSEIEnabled; }
  Void  setBufferingPeriodSEIEnabled(Int b)              { m_bufferingPeriodSEIEnabled = b; }
  Int   getBufferingPeriodSEIEnabled()                   { return m_bufferingPeriodSEIEnabled; }
  Void  setPictureTimingSEIEnabled(Int b)                { m_pictureTimingSEIEnabled = b; }
  Int   getPictureTimingSEIEnabled()                     { return m_pictureTimingSEIEnabled; }
  Void  setRecoveryPointSEIEnabled(Int b)                { m_recoveryPointSEIEnabled = b; }
  Int   getRecoveryPointSEIEnabled()                     { return m_recoveryPointSEIEnabled; }
  Void  setToneMappingInfoSEIEnabled(Bool b)                 {  m_toneMappingInfoSEIEnabled = b;  }
  Bool  getToneMappingInfoSEIEnabled()                       {  return m_toneMappingInfoSEIEnabled;  }
  Void  setTMISEIToneMapId(Int b)                            {  m_toneMapId = b;  }
  Int   getTMISEIToneMapId()                                 {  return m_toneMapId;  }
  Void  setTMISEIToneMapCancelFlag(Bool b)                   {  m_toneMapCancelFlag=b;  }
  Bool  getTMISEIToneMapCancelFlag()                         {  return m_toneMapCancelFlag;  }
  Void  setTMISEIToneMapPersistenceFlag(Bool b)              {  m_toneMapPersistenceFlag = b;  }
  Bool   getTMISEIToneMapPersistenceFlag()                   {  return m_toneMapPersistenceFlag;  }
  Void  setTMISEICodedDataBitDepth(Int b)                    {  m_codedDataBitDepth = b;  }
  Int   getTMISEICodedDataBitDepth()                         {  return m_codedDataBitDepth;  }
  Void  setTMISEITargetBitDepth(Int b)                       {  m_targetBitDepth = b;  }
  Int   getTMISEITargetBitDepth()                            {  return m_targetBitDepth;  }
  Void  setTMISEIModelID(Int b)                              {  m_modelId = b;  }
  Int   getTMISEIModelID()                                   {  return m_modelId;  }
  Void  setTMISEIMinValue(Int b)                             {  m_minValue = b;  }
  Int   getTMISEIMinValue()                                  {  return m_minValue;  }
  Void  setTMISEIMaxValue(Int b)                             {  m_maxValue = b;  }
  Int   getTMISEIMaxValue()                                  {  return m_maxValue;  }
  Void  setTMISEISigmoidMidpoint(Int b)                      {  m_sigmoidMidpoint = b;  }
  Int   getTMISEISigmoidMidpoint()                           {  return m_sigmoidMidpoint;  }
  Void  setTMISEISigmoidWidth(Int b)                         {  m_sigmoidWidth = b;  }
  Int   getTMISEISigmoidWidth()                              {  return m_sigmoidWidth;  }
  Void  setTMISEIStartOfCodedInterva( Int*  p )              {  m_startOfCodedInterval = p;  }
  Int*  getTMISEIStartOfCodedInterva()                       {  return m_startOfCodedInterval;  }
  Void  setTMISEINumPivots(Int b)                            {  m_numPivots = b;  }
  Int   getTMISEINumPivots()                                 {  return m_numPivots;  }
  Void  setTMISEICodedPivotValue( Int*  p )                  {  m_codedPivotValue = p;  }
  Int*  getTMISEICodedPivotValue()                           {  return m_codedPivotValue;  }
  Void  setTMISEITargetPivotValue( Int*  p )                 {  m_targetPivotValue = p;  }
  Int*  getTMISEITargetPivotValue()                          {  return m_targetPivotValue;  }
  Void  setTMISEICameraIsoSpeedIdc(Int b)                    {  m_cameraIsoSpeedIdc = b;  }
  Int   getTMISEICameraIsoSpeedIdc()                         {  return m_cameraIsoSpeedIdc;  }
  Void  setTMISEICameraIsoSpeedValue(Int b)                  {  m_cameraIsoSpeedValue = b;  }
  Int   getTMISEICameraIsoSpeedValue()                       {  return m_cameraIsoSpeedValue;  }
  Void  setTMISEIExposureIndexIdc(Int b)                     {  m_exposureIndexIdc = b;  }
  Int   getTMISEIExposurIndexIdc()                           {  return m_exposureIndexIdc;  }
  Void  setTMISEIExposureIndexValue(Int b)                   {  m_exposureIndexValue = b;  }
  Int   getTMISEIExposurIndexValue()                         {  return m_exposureIndexValue;  }
  Void  setTMISEIExposureCompensationValueSignFlag(Int b)    {  m_exposureCompensationValueSignFlag = b;  }
  Int   getTMISEIExposureCompensationValueSignFlag()         {  return m_exposureCompensationValueSignFlag;  }
  Void  setTMISEIExposureCompensationValueNumerator(Int b)   {  m_exposureCompensationValueNumerator = b;  }
  Int   getTMISEIExposureCompensationValueNumerator()        {  return m_exposureCompensationValueNumerator;  }
  Void  setTMISEIExposureCompensationValueDenomIdc(Int b)    {  m_exposureCompensationValueDenomIdc =b;  }
  Int   getTMISEIExposureCompensationValueDenomIdc()         {  return m_exposureCompensationValueDenomIdc;  }
  Void  setTMISEIRefScreenLuminanceWhite(Int b)              {  m_refScreenLuminanceWhite = b;  }
  Int   getTMISEIRefScreenLuminanceWhite()                   {  return m_refScreenLuminanceWhite;  }
  Void  setTMISEIExtendedRangeWhiteLevel(Int b)              {  m_extendedRangeWhiteLevel = b;  }
  Int   getTMISEIExtendedRangeWhiteLevel()                   {  return m_extendedRangeWhiteLevel;  }
  Void  setTMISEINominalBlackLevelLumaCodeValue(Int b)       {  m_nominalBlackLevelLumaCodeValue = b;  }
  Int   getTMISEINominalBlackLevelLumaCodeValue()            {  return m_nominalBlackLevelLumaCodeValue;  }
  Void  setTMISEINominalWhiteLevelLumaCodeValue(Int b)       {  m_nominalWhiteLevelLumaCodeValue = b;  }
  Int   getTMISEINominalWhiteLevelLumaCodeValue()            {  return m_nominalWhiteLevelLumaCodeValue;  }
  Void  setTMISEIExtendedWhiteLevelLumaCodeValue(Int b)      {  m_extendedWhiteLevelLumaCodeValue =b;  }
  Int   getTMISEIExtendedWhiteLevelLumaCodeValue()           {  return m_extendedWhiteLevelLumaCodeValue;  }
#if P0050_KNEE_FUNCTION_SEI
  Void  setKneeSEIEnabled(Int b)                             { m_kneeSEIEnabled = b; }
  Bool  getKneeSEIEnabled()                                  { return m_kneeSEIEnabled; }
  Void  setKneeSEIId(Int b)                                  { m_kneeSEIId = b; }
  Int   getKneeSEIId()                                       { return m_kneeSEIId; }
  Void  setKneeSEICancelFlag(Bool b)                         { m_kneeSEICancelFlag=b; }
  Bool  getKneeSEICancelFlag()                               { return m_kneeSEICancelFlag; }
  Void  setKneeSEIPersistenceFlag(Bool b)                    { m_kneeSEIPersistenceFlag = b; }
  Bool  getKneeSEIPersistenceFlag()                          { return m_kneeSEIPersistenceFlag; }
  Void  setKneeSEIMappingFlag(Bool b)                        { m_kneeSEIMappingFlag = b; }
  Bool  getKneeSEIMappingFlag()                              { return m_kneeSEIMappingFlag; }
  Void  setKneeSEIInputDrange(Int b)                         { m_kneeSEIInputDrange = b; }
  Int   getKneeSEIInputDrange()                              { return m_kneeSEIInputDrange; }
  Void  setKneeSEIInputDispLuminance(Int b)                  { m_kneeSEIInputDispLuminance = b; }
  Int   getKneeSEIInputDispLuminance()                       { return m_kneeSEIInputDispLuminance; }
  Void  setKneeSEIOutputDrange(Int b)                        { m_kneeSEIOutputDrange = b; }
  Int   getKneeSEIOutputDrange()                             { return m_kneeSEIOutputDrange; }
  Void  setKneeSEIOutputDispLuminance(Int b)                 { m_kneeSEIOutputDispLuminance = b; }
  Int   getKneeSEIOutputDispLuminance()                      { return m_kneeSEIOutputDispLuminance; }
  Void  setKneeSEINumKneePointsMinus1(Int b)                 { m_kneeSEINumKneePointsMinus1 = b; }
  Int   getKneeSEINumKneePointsMinus1()                      { return m_kneeSEINumKneePointsMinus1; }
  Void  setKneeSEIInputKneePoint(Int *p)                     { m_kneeSEIInputKneePoint = p; }
  Int*  getKneeSEIInputKneePoint()                           { return m_kneeSEIInputKneePoint; }
  Void  setKneeSEIOutputKneePoint(Int *p)                    { m_kneeSEIOutputKneePoint = p; }
  Int*  getKneeSEIOutputKneePoint()                          { return m_kneeSEIOutputKneePoint; }
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  Void  setCRISEIFile( Char* pch )                           { m_colourRemapSEIFile = pch; }
  Char* getCRISEIFile()                                      { return m_colourRemapSEIFile; }
  Void  setCRISEIId(Int i)                                   { m_colourRemapSEIId = i; }
  Int   getCRISEIId()                                        { return m_colourRemapSEIId; }
  Void  setCRISEICancelFlag(Bool b)                          { m_colourRemapSEICancelFlag = b; }
  Bool  getCRISEICancelFlag()                                { return m_colourRemapSEICancelFlag; }
  Void  setCRISEIPersistenceFlag(Bool b)                     { m_colourRemapSEIPersistenceFlag = b; }
  Bool  getCRISEIPersistenceFlag()                           { return m_colourRemapSEIPersistenceFlag; }
  Void  setCRISEIVideoSignalInfoPresentFlag(Bool b)          { m_colourRemapSEIVideoSignalInfoPresentFlag = b; }
  Bool  getCRISEIVideoSignalInfoPresentFlag()                { return m_colourRemapSEIVideoSignalInfoPresentFlag; }
  Void  setCRISEIFullRangeFlag(Bool b)                       { m_colourRemapSEIFullRangeFlag = b; }
  Bool  getCRISEIFullRangeFlag()                             { return m_colourRemapSEIFullRangeFlag; }
  Void  setCRISEIPrimaries(Int i)                            { m_colourRemapSEIPrimaries = i; }
  Int   getCRISEIPrimaries()                                 { return m_colourRemapSEIPrimaries; }  
  Void  setCRISEITransferFunction(Int i)                     { m_colourRemapSEITransferFunction = i; }
  Int   getCRISEITransferFunction()                          { return m_colourRemapSEITransferFunction; }  
  Void  setCRISEIMatrixCoefficients(Int i)                   { m_colourRemapSEIMatrixCoefficients = i; }
  Int   getCRISEIMatrixCoefficients()                        { return m_colourRemapSEIMatrixCoefficients; }
  Void  setCRISEIInputBitDepth(Int i)                        { m_colourRemapSEIInputBitDepth = i; }
  Int   getCRISEIInputBitDepth()                             { return m_colourRemapSEIInputBitDepth; } 
  Void  setCRISEIBitDepth(Int i)                             { m_colourRemapSEIBitDepth = i; }
  Int   getCRISEIBitDepth()                                  { return m_colourRemapSEIBitDepth; } 
  Void  setCRISEIPreLutNumValMinus1(Int *i)                  { for(Int c=0 ; c<3 ; c++) m_colourRemapSEIPreLutNumValMinus1[c] = i[c]; }
  Int   getCRISEIPreLutNumValMinus1(Int i)                   { return m_colourRemapSEIPreLutNumValMinus1[i]; }
  Void  setCRISEIPreLutCodedValue(Int **i)                   { for(Int c=0 ; c<3 ; c++) m_colourRemapSEIPreLutCodedValue[c] = i[c]; }
  Int*  getCRISEIPreLutCodedValue(Int i)                     { return m_colourRemapSEIPreLutCodedValue[i]; }
  Void  setCRISEIPreLutTargetValue(Int **i)                  { for(Int c=0 ; c<3 ; c++) m_colourRemapSEIPreLutTargetValue[c] = i[c]; }
  Int*  getCRISEIPreLutTargetValue(Int i)                    { return m_colourRemapSEIPreLutTargetValue[i]; }
  Void  setCRISEIMatrixPresentFlag(Bool b)                   { m_colourRemapSEIMatrixPresentFlag = b; }
  Bool  getCRISEIMatrixPresentFlag()                         { return m_colourRemapSEIMatrixPresentFlag; }
  Void  setCRISEILog2MatrixDenom(Int i)                      { m_colourRemapSEILog2MatrixDenom = i; }
  Int   getCRISEILog2MatrixDenom()                           { return m_colourRemapSEILog2MatrixDenom; } 
  Void  setCRISEICoeffs(Int i[3][3])                         { for(Int c=0 ; c<3 ; c++) for(Int j=0 ; j<3 ; j++) m_colourRemapSEICoeffs[c][j] = i[c][j]; }
  Int*  getCRISEICoeffs(Int i)                               { return m_colourRemapSEICoeffs[i]; }
  Void  setCRISEIPostLutNumValMinus1(Int *i)                 { for(Int c=0 ; c<3 ; c++) m_colourRemapSEIPostLutNumValMinus1[c] = i[c]; }
  Int   getCRISEIPostLutNumValMinus1(Int i)                  { return m_colourRemapSEIPostLutNumValMinus1[i]; }
  Void  setCRISEIPostLutCodedValue(Int **i)                  { for(Int c=0 ; c<3 ; c++) m_colourRemapSEIPostLutCodedValue[c] = i[c]; }
  Int*  getCRISEIPostLutCodedValue(Int i)                    { return m_colourRemapSEIPostLutCodedValue[i]; }
  Void  setCRISEIPostLutTargetValue(Int **i)                 { for(Int c=0 ; c<3 ; c++) m_colourRemapSEIPostLutTargetValue[c] = i[c]; }
  Int*  getCRISEIPostLutTargetValue(Int i)                   { return m_colourRemapSEIPostLutTargetValue[i]; }
#endif
  Void  setFramePackingArrangementSEIEnabled(Int b)      { m_framePackingSEIEnabled = b; }
  Int   getFramePackingArrangementSEIEnabled()           { return m_framePackingSEIEnabled; }
  Void  setFramePackingArrangementSEIType(Int b)         { m_framePackingSEIType = b; }
  Int   getFramePackingArrangementSEIType()              { return m_framePackingSEIType; }
  Void  setFramePackingArrangementSEIId(Int b)           { m_framePackingSEIId = b; }
  Int   getFramePackingArrangementSEIId()                { return m_framePackingSEIId; }
  Void  setFramePackingArrangementSEIQuincunx(Int b)     { m_framePackingSEIQuincunx = b; }
  Int   getFramePackingArrangementSEIQuincunx()          { return m_framePackingSEIQuincunx; }
  Void  setFramePackingArrangementSEIInterpretation(Int b)  { m_framePackingSEIInterpretation = b; }
  Int   getFramePackingArrangementSEIInterpretation()    { return m_framePackingSEIInterpretation; }
  Void  setDisplayOrientationSEIAngle(Int b)             { m_displayOrientationSEIAngle = b; }
  Int   getDisplayOrientationSEIAngle()                  { return m_displayOrientationSEIAngle; }
  Void  setTemporalLevel0IndexSEIEnabled(Int b)          { m_temporalLevel0IndexSEIEnabled = b; }
  Int   getTemporalLevel0IndexSEIEnabled()               { return m_temporalLevel0IndexSEIEnabled; }
  Void  setGradualDecodingRefreshInfoEnabled(Int b)      { m_gradualDecodingRefreshInfoEnabled = b;    }
  Int   getGradualDecodingRefreshInfoEnabled()           { return m_gradualDecodingRefreshInfoEnabled; }
  Void  setDecodingUnitInfoSEIEnabled(Int b)                { m_decodingUnitInfoSEIEnabled = b;    }
  Int   getDecodingUnitInfoSEIEnabled()                     { return m_decodingUnitInfoSEIEnabled; }
  Void  setSOPDescriptionSEIEnabled(Int b)                { m_SOPDescriptionSEIEnabled = b; }
  Int   getSOPDescriptionSEIEnabled()                     { return m_SOPDescriptionSEIEnabled; }
  Void  setScalableNestingSEIEnabled(Int b)                { m_scalableNestingSEIEnabled = b; }
#if Q0189_TMVP_CONSTRAINTS
  void setTMVPConstraintsSEIEnabled(Int b)                { m_TMVPConstraintsSEIEnabled = b; }
#endif
  Int   getScalableNestingSEIEnabled()                     { return m_scalableNestingSEIEnabled; }
#if Q0189_TMVP_CONSTRAINTS
  Int   getTMVPConstraintsSEIEnabled()                {  return m_TMVPConstraintsSEIEnabled; }
#endif
  Void      setUseWP               ( Bool b )    { m_useWeightedPred   = b;    }
  Void      setWPBiPred            ( Bool b )    { m_useWeightedBiPred = b;    }
  Bool      getUseWP               ()            { return m_useWeightedPred;   }
  Bool      getWPBiPred            ()            { return m_useWeightedBiPred; }
  Void      setLog2ParallelMergeLevelMinus2   ( UInt u )    { m_log2ParallelMergeLevelMinus2       = u;    }
  UInt      getLog2ParallelMergeLevelMinus2   ()            { return m_log2ParallelMergeLevelMinus2;       }
  Void      setMaxNumMergeCand                ( UInt u )    { m_maxNumMergeCand = u;      }
  UInt      getMaxNumMergeCand                ()            { return m_maxNumMergeCand;   }
  Void      setUseScalingListId    ( Int  u )    { m_useScalingListId       = u;   }
  Int       getUseScalingListId    ()            { return m_useScalingListId;      }
  Void      setScalingListFile     ( Char*  pch ){ m_scalingListFile     = pch; }
  Char*     getScalingListFile     ()            { return m_scalingListFile;    }
  Void      setTMVPModeId ( Int  u ) { m_TMVPModeId = u;    }
  Int       getTMVPModeId ()         { return m_TMVPModeId; }
  Void      setSignHideFlag( Int signHideFlag ) { m_signHideFlag = signHideFlag; }
  Int       getSignHideFlag()                    { return m_signHideFlag; }
  Bool      getUseRateCtrl         ()              { return m_RCEnableRateControl;   }
  Void      setUseRateCtrl         ( Bool b )      { m_RCEnableRateControl = b;      }
  Int       getTargetBitrate       ()              { return m_RCTargetBitrate;       }
  Void      setTargetBitrate       ( Int bitrate ) { m_RCTargetBitrate  = bitrate;   }
  Int       getKeepHierBit         ()              { return m_RCKeepHierarchicalBit; }
  Void      setKeepHierBit         ( Int i )       { m_RCKeepHierarchicalBit = i;    }
  Bool      getLCULevelRC          ()              { return m_RCLCULevelRC; }
  Void      setLCULevelRC          ( Bool b )      { m_RCLCULevelRC = b; }
  Bool      getUseLCUSeparateModel ()              { return m_RCUseLCUSeparateModel; }
  Void      setUseLCUSeparateModel ( Bool b )      { m_RCUseLCUSeparateModel = b;    }
  Int       getInitialQP           ()              { return m_RCInitialQP;           }
  Void      setInitialQP           ( Int QP )      { m_RCInitialQP = QP;             }
  Bool      getForceIntraQP        ()              { return m_RCForceIntraQP;        }
  Void      setForceIntraQP        ( Bool b )      { m_RCForceIntraQP = b;           }
  Bool      getTransquantBypassEnableFlag()           { return m_TransquantBypassEnableFlag; }
  Void      setTransquantBypassEnableFlag(Bool flag)  { m_TransquantBypassEnableFlag = flag; }
  Bool      getCUTransquantBypassFlagForceValue()          { return m_CUTransquantBypassFlagForce; }
  Void      setCUTransquantBypassFlagForceValue(Bool flag) { m_CUTransquantBypassFlagForce = flag; }
  Void setVPS(TComVPS *p) { m_cVPS = *p; }
  TComVPS *getVPS() { return &m_cVPS; }
  Void      setUseRecalculateQPAccordingToLambda ( Bool b ) { m_recalculateQPAccordingToLambda = b;    }
  Bool      getUseRecalculateQPAccordingToLambda ()         { return m_recalculateQPAccordingToLambda; }

  Void      setUseStrongIntraSmoothing ( Bool b ) { m_useStrongIntraSmoothing = b;    }
  Bool      getUseStrongIntraSmoothing ()         { return m_useStrongIntraSmoothing; }

  Void      setActiveParameterSetsSEIEnabled ( Int b )  { m_activeParameterSetsSEIEnabled = b; }  
  Int       getActiveParameterSetsSEIEnabled ()         { return m_activeParameterSetsSEIEnabled; }
  Bool      getVuiParametersPresentFlag()                 { return m_vuiParametersPresentFlag; }
  Void      setVuiParametersPresentFlag(Bool i)           { m_vuiParametersPresentFlag = i; }
  Bool      getAspectRatioInfoPresentFlag()               { return m_aspectRatioInfoPresentFlag; }
  Void      setAspectRatioInfoPresentFlag(Bool i)         { m_aspectRatioInfoPresentFlag = i; }
  Int       getAspectRatioIdc()                           { return m_aspectRatioIdc; }
  Void      setAspectRatioIdc(Int i)                      { m_aspectRatioIdc = i; }
  Int       getSarWidth()                                 { return m_sarWidth; }
  Void      setSarWidth(Int i)                            { m_sarWidth = i; }
  Int       getSarHeight()                                { return m_sarHeight; }
  Void      setSarHeight(Int i)                           { m_sarHeight = i; }
  Bool      getOverscanInfoPresentFlag()                  { return m_overscanInfoPresentFlag; }
  Void      setOverscanInfoPresentFlag(Bool i)            { m_overscanInfoPresentFlag = i; }
  Bool      getOverscanAppropriateFlag()                  { return m_overscanAppropriateFlag; }
  Void      setOverscanAppropriateFlag(Bool i)            { m_overscanAppropriateFlag = i; }
  Bool      getVideoSignalTypePresentFlag()               { return m_videoSignalTypePresentFlag; }
  Void      setVideoSignalTypePresentFlag(Bool i)         { m_videoSignalTypePresentFlag = i; }
  Int       getVideoFormat()                              { return m_videoFormat; }
  Void      setVideoFormat(Int i)                         { m_videoFormat = i; }
  Bool      getVideoFullRangeFlag()                       { return m_videoFullRangeFlag; }
  Void      setVideoFullRangeFlag(Bool i)                 { m_videoFullRangeFlag = i; }
  Bool      getColourDescriptionPresentFlag()             { return m_colourDescriptionPresentFlag; }
  Void      setColourDescriptionPresentFlag(Bool i)       { m_colourDescriptionPresentFlag = i; }
  Int       getColourPrimaries()                          { return m_colourPrimaries; }
  Void      setColourPrimaries(Int i)                     { m_colourPrimaries = i; }
  Int       getTransferCharacteristics()                  { return m_transferCharacteristics; }
  Void      setTransferCharacteristics(Int i)             { m_transferCharacteristics = i; }
  Int       getMatrixCoefficients()                       { return m_matrixCoefficients; }
  Void      setMatrixCoefficients(Int i)                  { m_matrixCoefficients = i; }
  Bool      getChromaLocInfoPresentFlag()                 { return m_chromaLocInfoPresentFlag; }
  Void      setChromaLocInfoPresentFlag(Bool i)           { m_chromaLocInfoPresentFlag = i; }
  Int       getChromaSampleLocTypeTopField()              { return m_chromaSampleLocTypeTopField; }
  Void      setChromaSampleLocTypeTopField(Int i)         { m_chromaSampleLocTypeTopField = i; }
  Int       getChromaSampleLocTypeBottomField()           { return m_chromaSampleLocTypeBottomField; }
  Void      setChromaSampleLocTypeBottomField(Int i)      { m_chromaSampleLocTypeBottomField = i; }
  Bool      getNeutralChromaIndicationFlag()              { return m_neutralChromaIndicationFlag; }
  Void      setNeutralChromaIndicationFlag(Bool i)        { m_neutralChromaIndicationFlag = i; }
  Window   &getDefaultDisplayWindow()                     { return m_defaultDisplayWindow; }
  Void      setDefaultDisplayWindow (Int offsetLeft, Int offsetRight, Int offsetTop, Int offsetBottom ) { m_defaultDisplayWindow.setWindow (offsetLeft, offsetRight, offsetTop, offsetBottom); }
  Bool      getFrameFieldInfoPresentFlag()                { return m_frameFieldInfoPresentFlag; }
  Void      setFrameFieldInfoPresentFlag(Bool i)          { m_frameFieldInfoPresentFlag = i; }  
  Bool      getPocProportionalToTimingFlag()              { return m_pocProportionalToTimingFlag; }
  Void      setPocProportionalToTimingFlag(Bool x)        { m_pocProportionalToTimingFlag = x;    }
  Int       getNumTicksPocDiffOneMinus1()                 { return m_numTicksPocDiffOneMinus1;    }
  Void      setNumTicksPocDiffOneMinus1(Int x)            { m_numTicksPocDiffOneMinus1 = x;       }
  Bool      getBitstreamRestrictionFlag()                 { return m_bitstreamRestrictionFlag; }
  Void      setBitstreamRestrictionFlag(Bool i)           { m_bitstreamRestrictionFlag = i; }
  Bool      getTilesFixedStructureFlag()                  { return m_tilesFixedStructureFlag; }
  Void      setTilesFixedStructureFlag(Bool i)            { m_tilesFixedStructureFlag = i; }
  Bool      getMotionVectorsOverPicBoundariesFlag()       { return m_motionVectorsOverPicBoundariesFlag; }
  Void      setMotionVectorsOverPicBoundariesFlag(Bool i) { m_motionVectorsOverPicBoundariesFlag = i; }
  Int       getMinSpatialSegmentationIdc()                { return m_minSpatialSegmentationIdc; }
  Void      setMinSpatialSegmentationIdc(Int i)           { m_minSpatialSegmentationIdc = i; }
  Int       getMaxBytesPerPicDenom()                      { return m_maxBytesPerPicDenom; }
  Void      setMaxBytesPerPicDenom(Int i)                 { m_maxBytesPerPicDenom = i; }
  Int       getMaxBitsPerMinCuDenom()                     { return m_maxBitsPerMinCuDenom; }
  Void      setMaxBitsPerMinCuDenom(Int i)                { m_maxBitsPerMinCuDenom = i; }
  Int       getLog2MaxMvLengthHorizontal()                { return m_log2MaxMvLengthHorizontal; }
  Void      setLog2MaxMvLengthHorizontal(Int i)           { m_log2MaxMvLengthHorizontal = i; }
  Int       getLog2MaxMvLengthVertical()                  { return m_log2MaxMvLengthVertical; }
  Void      setLog2MaxMvLengthVertical(Int i)             { m_log2MaxMvLengthVertical = i; }
  
  Bool getProgressiveSourceFlag() const { return m_progressiveSourceFlag; }
  Void setProgressiveSourceFlag(Bool b) { m_progressiveSourceFlag = b; }
  
  Bool getInterlacedSourceFlag() const { return m_interlacedSourceFlag; }
  Void setInterlacedSourceFlag(Bool b) { m_interlacedSourceFlag = b; }
  
  Bool getNonPackedConstraintFlag() const { return m_nonPackedConstraintFlag; }
  Void setNonPackedConstraintFlag(Bool b) { m_nonPackedConstraintFlag = b; }
  
  Bool getFrameOnlyConstraintFlag() const { return m_frameOnlyConstraintFlag; }
  Void setFrameOnlyConstraintFlag(Bool b) { m_frameOnlyConstraintFlag = b; }

#if SVC_EXTENSION
  UInt      getLayerId            () { return m_layerId;              }
  Void      setLayerId            (UInt layer) { m_layerId = layer; }
  UInt      getNumLayer           () { return m_numLayer;             }  
  Void      setNumLayer           (UInt uiNum)   { m_numLayer = uiNum;  }
  Void      setConformanceWindow(Window& conformanceWindow ) { m_conformanceWindow = conformanceWindow; }
  Void      setElRapSliceTypeB(Int bEnabled) {m_elRapSliceBEnabled = bEnabled;}
  Int       getElRapSliceTypeB()              {return m_elRapSliceBEnabled;}
#if M0040_ADAPTIVE_RESOLUTION_CHANGE
  Void      setAdaptiveResolutionChange(Int x) { m_adaptiveResolutionChange = x;    }
  Int       getAdaptiveResolutionChange()      { return m_adaptiveResolutionChange; }
#endif
#if HIGHER_LAYER_IRAP_SKIP_FLAG
  Void      setSkipPictureAtArcSwitch(Int x) { m_skipPictureAtArcSwitch = x;    }
  Int       getSkipPictureAtArcSwitch()      { return m_skipPictureAtArcSwitch; }
#endif
#if AUXILIARY_PICTURES
  Void         setChromaFormatIDC(ChromaFormat x) { m_chromaFormatIDC = x;    }
  ChromaFormat getChromaFormatIDC()               { return m_chromaFormatIDC; }
#endif
#if O0153_ALT_OUTPUT_LAYER_FLAG
  Bool      getAltOuputLayerFlag() const { return m_altOutputLayerFlag; }
  Void      setAltOuputLayerFlag(Bool b) { m_altOutputLayerFlag = b;    }
#endif
#if O0149_CROSS_LAYER_BLA_FLAG
  Bool      getCrossLayerBLAFlag() const { return m_crossLayerBLAFlag; }
  Void      setCrossLayerBLAFlag(Bool b) { m_crossLayerBLAFlag = b;    }
#endif
#if FAST_INTRA_SHVC
  Bool      getUseFastIntraScalable         ()      { return m_useFastIntraScalable; }
  Void      setUseFastIntraScalable         ( Bool  b )     { m_useFastIntraScalable = b; }
#endif
#if VPS_EXTN_DIRECT_REF_LAYERS
  Int       getNumDirectRefLayers           ()                              { return m_numDirectRefLayers;      }
  Void      setNumDirectRefLayers           (Int num)                       { m_numDirectRefLayers = num;       }

  Int       getRefLayerId                   (Int i)                         { return m_refLayerId[i];           }
  Void      setRefLayerId                   (Int i, Int refLayerId)         { m_refLayerId[i] = refLayerId;     }

  Int       getNumActiveRefLayers           ()                              { return m_numActiveRefLayers;      }
  Void      setNumActiveRefLayers           (Int num)                       { m_numActiveRefLayers = num;       }

  Int       getPredLayerId                  (Int i)                         { return m_predLayerId[i];          }
  Void      setPredLayerId                  (Int i, Int refLayerId)         { m_predLayerId[i] = refLayerId;    }

  Int       getNumSamplePredRefLayers       ()                              { return m_numSamplePredRefLayers;  }
  Void      setNumSamplePredRefLayers       (Int num)                       { m_numSamplePredRefLayers = num;   }

  Int       getSamplePredRefLayerId         (Int i)                         { return m_samplePredRefLayerId[i];       }
  Void      setSamplePredRefLayerId         (Int i, Int refLayerId)         { m_samplePredRefLayerId[i] = refLayerId; }

  Int       getNumMotionPredRefLayers       ()                              { return m_numMotionPredRefLayers;  }
  Void      setNumMotionPredRefLayers       (Int num)                       { m_numMotionPredRefLayers = num;   }

  Int       getMotionPredRefLayerId         (Int i)                         { return m_motionPredRefLayerId[i];       }
  Void      setMotionPredRefLayerId         (Int i, Int refLayerId)         { m_motionPredRefLayerId[i] = refLayerId; }

  Bool      getSamplePredEnabledFlag        (Int i)                         { return m_samplePredEnabledFlag[i];  }
  Void      setSamplePredEnabledFlag        (Int i,Bool flag)               { m_samplePredEnabledFlag[i] = flag;  }

  Bool      getMotionPredEnabledFlag        (Int i)                         { return m_motionPredEnabledFlag[i];  }
  Void      setMotionPredEnabledFlag        (Int i,Bool flag)               { m_motionPredEnabledFlag[i] = flag;  }
#endif
  Int       getMaxTidIlRefPicsPlus1         ()                              { return m_maxTidIlRefPicsPlus1; }
  Void      setMaxTidIlRefPicsPlus1         (Int num)                       { m_maxTidIlRefPicsPlus1 = num;  }
#if LAYERS_NOT_PRESENT_SEI
  Void  setLayersNotPresentSEIEnabled(Int b)             { m_layersNotPresentSEIEnabled = b; }
  Int   getLayersNotPresentSEIEnabled()                  { return m_layersNotPresentSEIEnabled; }
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
  Void  setInterLayerConstrainedTileSetsSEIEnabled(Bool b) { m_interLayerConstrainedTileSetsSEIEnabled = b; }
  Bool  getInterLayerConstrainedTileSetsSEIEnabled()       { return m_interLayerConstrainedTileSetsSEIEnabled; }
  Void  setIlNumSetsInMessage(UInt b)                      { m_ilNumSetsInMessage = b; }
  Int   getIlNumSetsInMessage()                            { return m_ilNumSetsInMessage; }
  Void  setSkippedTileSetPresentFlag(Bool b)               { m_skippedTileSetPresentFlag = b; }
  Bool  getSkippedTileSetPresentFlag()                     { return m_skippedTileSetPresentFlag; }
  Void  setTopLeftTileIndex(UInt *b)
  {
    for (UInt i = 0; i < m_ilNumSetsInMessage; i++)
    {
      m_topLeftTileIndex[i] = b[i];
    }
  }
  UInt  getTopLeftTileIndex(UInt b)                        { return m_topLeftTileIndex[b]; }
  Void  setBottomRightTileIndex(UInt *b)
  {
    for (UInt i = 0; i < m_ilNumSetsInMessage; i++)
    {
      m_bottomRightTileIndex[i] = b[i];
    }
  }
  UInt  getBottomRightTileIndex(UInt b)                    { return m_bottomRightTileIndex[b]; }
  Void  setIlcIdc(UInt *b)
  {
    for (UInt i = 0; i < m_ilNumSetsInMessage; i++)
    {
      m_ilcIdc[i] = b[i];
    }
  }
  UInt  getIlcIdc(UInt b)                                  { return m_ilcIdc[b]; }
#endif
#if Q0048_CGS_3D_ASYMLUT
  Void      setCGSFlag(Int n)             { m_nCGSFlag = n;    }
  Int       getCGSFlag()                  { return m_nCGSFlag; }
  Void      setCGSMaxOctantDepth(Int n)   { m_nCGSMaxOctantDepth = n;    }
  Int       getCGSMaxOctantDepth()        { return m_nCGSMaxOctantDepth; }
  Void      setCGSMaxYPartNumLog2(Int n)  { m_nCGSMaxYPartNumLog2 = n;    }
  Int       getCGSMaxYPartNumLog2()       { return m_nCGSMaxYPartNumLog2; }
  Void      setCGSLUTBit(Int n)           { m_nCGSLUTBit = n;    }
  Int       getCGSLUTBit()                { return m_nCGSLUTBit; }
#if R0151_CGS_3D_ASYMLUT_IMPROVE
  Void      setCGSAdaptChroma(Int n)      { m_nCGSAdaptiveChroma = n;  }
  Int       getCGSAdaptChroma()           { return m_nCGSAdaptiveChroma; }
#endif
#if R0179_ENC_OPT_3DLUT_SIZE
  Void      setCGSLutSizeRDO(Int n)      { m_nCGSLutSizeRDO = n;  }
  Int       getCGSLutSizeRDO()           { return m_nCGSLutSizeRDO; }
#endif
#endif
#endif
};

#if SVC_EXTENSION
#if REPN_FORMAT_IN_VPS
struct RepFormatCfg
{
  Int   m_chromaFormatIdc;
  Bool  m_separateColourPlaneFlag;
  Int   m_picWidthInLumaSamples;
  Int   m_picHeightInLumaSamples;
  Int   m_bitDepthLuma;
  Int   m_bitDepthChroma;
  RepFormatCfg()
    : m_chromaFormatIdc         (CHROMA_420)
    , m_separateColourPlaneFlag (0)
    , m_picWidthInLumaSamples   (352)
    , m_picHeightInLumaSamples  (288)
    , m_bitDepthLuma            (8)
    , m_bitDepthChroma          (8)
  {}
};
std::istringstream &operator>>(std::istringstream &in, RepFormatCfg &repFormatCfg);
#endif
#endif //SVC_EXTENSION

//! \}

#endif // !defined(AFX_TENCCFG_H__6B99B797_F4DA_4E46_8E78_7656339A6C41__INCLUDED_)
