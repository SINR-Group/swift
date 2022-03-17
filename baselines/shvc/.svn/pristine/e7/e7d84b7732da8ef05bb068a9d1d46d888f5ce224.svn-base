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

/** \file     TAppEncCfg.h
    \brief    Handle encoder configuration parameters (header)
*/

#ifndef __TAPPENCCFG__
#define __TAPPENCCFG__

#include "TLibCommon/CommonDef.h"

#include "TLibEncoder/TEncCfg.h"
#if SVC_EXTENSION
#include "TAppEncLayerCfg.h"
#endif
#include <sstream>
#include <vector>
//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder configuration class
class TAppEncCfg
{
protected:
  // file I/O
#if SVC_EXTENSION
  TAppEncLayerCfg m_acLayerCfg [MAX_LAYERS];
  Int       m_numLayers;                                      ///< number of layers
  Int       m_scalabilityMask[MAX_VPS_NUM_SCALABILITY_TYPES]; ///< scalability_mask
  Char*     m_pBitstreamFile;                                 ///< output bitstream file
  Double    m_adLambdaModifier[ MAX_TLAYER ];                 ///< Lambda modifier array for each temporal layer
  // source specification
  UInt      m_FrameSkip;                                      ///< number of skipped frames from the beginning
  Int       m_framesToBeEncoded;                              ///< number of encoded frames
#if AVC_BASE
#if VPS_AVC_BL_FLAG_REMOVAL
  Int       m_nonHEVCBaseLayerFlag;                           ///< non HEVC BL
#else
  Int       m_avcBaseLayerFlag;                               ///< avc_baselayer_flag
#endif
#endif
  Bool      m_maxTidRefPresentFlag; 
#if Q0078_ADD_LAYER_SETS
  Int       m_numLayerSets;
  Int       m_numLayerInIdList[MAX_VPS_LAYER_SETS_PLUS1];
  Int       m_layerSetLayerIdList[MAX_VPS_LAYER_SETS_PLUS1][MAX_VPS_LAYER_ID_PLUS1];
  Int       m_numAddLayerSets;
  Int       m_numHighestLayerIdx[MAX_VPS_LAYER_SETS_PLUS1];
  Int       m_highestLayerIdx[MAX_VPS_LAYER_SETS_PLUS1][MAX_VPS_LAYER_ID_PLUS1];
#endif
#if OUTPUT_LAYER_SETS_CONFIG
  std::vector<Int>                m_outputLayerSetIdx;
  Int       m_defaultTargetOutputLayerIdc;
  Int       m_numOutputLayerSets;
  std::vector<Int>                m_numLayersInOutputLayerSet;
  std::vector< std::vector<Int> > m_listOfOutputLayers;
#endif
#else
  Char*     m_pchInputFile;                                   ///< source file name
  Char*     m_pchBitstreamFile;                               ///< output bitstream file
  Char*     m_pchReconFile;                                   ///< output reconstruction file
  Double    m_adLambdaModifier[ MAX_TLAYER ];                 ///< Lambda modifier array for each temporal layer
  // source specification
  Int       m_iFrameRate;                                     ///< source frame-rates (Hz)
  UInt      m_FrameSkip;                                      ///< number of skipped frames from the beginning
  Int       m_iSourceWidth;                                   ///< source width in pixel
  Int       m_iSourceHeight;                                  ///< source height in pixel (when interlaced = field height)
  
  Int       m_iSourceHeightOrg;                               ///< original source height in pixel (when interlaced = frame height)
  
  
  Int       m_conformanceWindowMode;
  Int       m_confWinLeft;
  Int       m_confWinRight;
  Int       m_confWinTop;
  Int       m_confWinBottom;
  Int       m_framesToBeEncoded;                              ///< number of encoded frames
  Int       m_aiPad[2];                                       ///< number of padded pixels for width and height
#endif  
#if AUXILIARY_PICTURES
  ChromaFormat m_InputChromaFormatIDC;
#endif
  Bool      m_isField;                                        ///< enable field coding
  Bool      m_isTopFieldFirst;

  // profile/level
  Profile::Name m_profile;
  Level::Tier   m_levelTier;
  Level::Name   m_level;
  Bool m_progressiveSourceFlag;
  Bool m_interlacedSourceFlag;
  Bool m_nonPackedConstraintFlag;
  Bool m_frameOnlyConstraintFlag;
  
  // coding structure
#if !SVC_EXTENSION
  Int       m_iIntraPeriod;                                   ///< period of I-slice (random access period)
#endif
  Int       m_iDecodingRefreshType;                           ///< random access type
  Int       m_iGOPSize;                                       ///< GOP size of hierarchical structure

#if !Q0108_TSA_STSA
  Int       m_extraRPSs;                                      ///< extra RPSs added to handle CRA
#else
  Int       m_extraRPSs[MAX_LAYERS];                          ///< extra RPSs added to handle CRA
#endif

  GOPEntry  m_GOPList[MAX_GOP];                               ///< the coding structure entries from the config file
#if Q0108_TSA_STSA
  GOPEntry  m_EhGOPList[MAX_LAYERS][MAX_GOP];                 ///< the enhancement layer coding structure entries from the config file
  Int       m_inheritCodingStruct[MAX_LAYERS];
#endif

  Int       m_numReorderPics[MAX_TLAYER];                     ///< total number of reorder pictures
  Int       m_maxDecPicBuffering[MAX_TLAYER];                 ///< total number of pictures in the decoded picture buffer
  Bool      m_useTransformSkip;                               ///< flag for enabling intra transform skipping
  Bool      m_useTransformSkipFast;                           ///< flag for enabling fast intra transform skipping
  Bool      m_enableAMP;
  // coding quality
#if !SVC_EXTENSION
  Double    m_fQP;                                            ///< QP value of key-picture (floating point)
  Int       m_iQP;                                            ///< QP value of key-picture (integer)
  Char*     m_pchdQPFile;                                     ///< QP offset for each slice (initialized from external file)
  Int*      m_aidQP;                                          ///< array of slice QP values
#endif
  Int       m_iMaxDeltaQP;                                    ///< max. |delta QP|
  UInt      m_uiDeltaQpRD;                                    ///< dQP range for multi-pass slice QP optimization
  Int       m_iMaxCuDQPDepth;                                 ///< Max. depth for a minimum CuDQPSize (0:default)

  Int       m_cbQpOffset;                                     ///< Chroma Cb QP Offset (0:default) 
  Int       m_crQpOffset;                                     ///< Chroma Cr QP Offset (0:default)

#if ADAPTIVE_QP_SELECTION
  Bool      m_bUseAdaptQpSelect;
#endif

  Bool      m_bUseAdaptiveQP;                                 ///< Flag for enabling QP adaptation based on a psycho-visual model
  Int       m_iQPAdaptationRange;                             ///< dQP range by QP adaptation
  
  Int       m_maxTempLayer;                                  ///< Max temporal layer
#if Q0108_TSA_STSA
  Int       m_EhMaxTempLayer[MAX_LAYERS];                    ///< Max temporal layer
#endif

#if !LAYER_CTB
  // coding unit (CU) definition
  UInt      m_uiMaxCUWidth;                                   ///< max. CU width in pixel
  UInt      m_uiMaxCUHeight;                                  ///< max. CU height in pixel
  UInt      m_uiMaxCUDepth;                                   ///< max. CU depth
  
  // transfom unit (TU) definition
  UInt      m_uiQuadtreeTULog2MaxSize;
  UInt      m_uiQuadtreeTULog2MinSize;
  
  UInt      m_uiQuadtreeTUMaxDepthInter;
  UInt      m_uiQuadtreeTUMaxDepthIntra;
#endif
  
  // coding tools (bit-depth)
#if !O0194_DIFFERENT_BITDEPTH_EL_BL
  Int       m_inputBitDepthY;                               ///< bit-depth of input file (luma component)
  Int       m_inputBitDepthC;                               ///< bit-depth of input file (chroma component)
  Int       m_outputBitDepthY;                              ///< bit-depth of output file (luma component)
  Int       m_outputBitDepthC;                              ///< bit-depth of output file (chroma component)
  Int       m_internalBitDepthY;                            ///< bit-depth codec operates at in luma (input/output files will be converted)
  Int       m_internalBitDepthC;                            ///< bit-depth codec operates at in chroma (input/output files will be converted)
#endif
#if AUXILIARY_PICTURES
  ChromaFormat m_chromaFormatIDC;
#endif

  // coding tools (PCM bit-depth)
  Bool      m_bPCMInputBitDepthFlag;                          ///< 0: PCM bit-depth is internal bit-depth. 1: PCM bit-depth is input bit-depth.

  // coding tool (SAO)
  Bool      m_bUseSAO; 
  Int       m_maxNumOffsetsPerPic;                            ///< SAO maximun number of offset per picture
  Bool      m_saoLcuBoundary;                                 ///< SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
  // coding tools (loop filter)
  Bool      m_bLoopFilterDisable;                             ///< flag for using deblocking filter
  Bool      m_loopFilterOffsetInPPS;                         ///< offset for deblocking filter in 0 = slice header, 1 = PPS
  Int       m_loopFilterBetaOffsetDiv2;                     ///< beta offset for deblocking filter
  Int       m_loopFilterTcOffsetDiv2;                       ///< tc offset for deblocking filter
  Bool      m_DeblockingFilterControlPresent;                 ///< deblocking filter control present flag in PPS
  Bool      m_DeblockingFilterMetric;                         ///< blockiness metric in encoder
 
  // coding tools (PCM)
  Bool      m_usePCM;                                         ///< flag for using IPCM
  UInt      m_pcmLog2MaxSize;                                 ///< log2 of maximum PCM block size
  UInt      m_uiPCMLog2MinSize;                               ///< log2 of minimum PCM block size
  Bool      m_bPCMFilterDisableFlag;                          ///< PCM filter disable flag

  // coding tools (encoder-only parameters)
  Bool      m_bUseASR;                                        ///< flag for using adaptive motion search range
  Bool      m_bUseHADME;                                      ///< flag for using HAD in sub-pel ME
  Bool      m_useRDOQ;                                       ///< flag for using RD optimized quantization
  Bool      m_useRDOQTS;                                     ///< flag for using RD optimized quantization for transform skip
  Int       m_rdPenalty;                                      ///< RD-penalty for 32x32 TU for intra in non-intra slices (0: no RD-penalty, 1: RD-penalty, 2: maximum RD-penalty) 
  Int       m_iFastSearch;                                    ///< ME mode, 0 = full, 1 = diamond, 2 = PMVFAST
  Int       m_iSearchRange;                                   ///< ME search range
  Int       m_bipredSearchRange;                              ///< ME search range for bipred refinement
  Bool      m_bUseFastEnc;                                    ///< flag for using fast encoder setting
  Bool      m_bUseEarlyCU;                                    ///< flag for using Early CU setting
  Bool      m_useFastDecisionForMerge;                        ///< flag for using Fast Decision Merge RD-Cost 
  Bool      m_bUseCbfFastMode;                              ///< flag for using Cbf Fast PU Mode Decision
  Bool      m_useEarlySkipDetection;                         ///< flag for using Early SKIP Detection
#if FAST_INTRA_SHVC
  Bool      m_useFastIntraScalable;                          ///< flag for using Fast Intra Decision for Scalable HEVC
#endif
  Int       m_sliceMode;                                     ///< 0: no slice limits, 1 : max number of CTBs per slice, 2: max number of bytes per slice, 
                                                             ///< 3: max number of tiles per slice
  Int       m_sliceArgument;                                 ///< argument according to selected slice mode
  Int       m_sliceSegmentMode;                              ///< 0: no slice segment limits, 1 : max number of CTBs per slice segment, 2: max number of bytes per slice segment, 
                                                             ///< 3: max number of tiles per slice segment
  Int       m_sliceSegmentArgument;                          ///< argument according to selected slice segment mode

  Bool      m_bLFCrossSliceBoundaryFlag;  ///< 1: filter across slice boundaries 0: do not filter across slice boundaries
  Bool      m_bLFCrossTileBoundaryFlag;   ///< 1: filter across tile boundaries  0: do not filter across tile boundaries
  Bool      m_tileUniformSpacingFlag;
  Int       m_numTileColumnsMinus1;
  Int       m_numTileRowsMinus1;
  std::vector<Int> m_tileColumnWidth;
  std::vector<Int> m_tileRowHeight;
#if !SVC_EXTENSION
  Int       m_iWaveFrontSynchro; //< 0: no WPP. >= 1: WPP is enabled, the "Top right" from which inheritance occurs is this LCU offset in the line above the current.
  Int       m_iWaveFrontSubstreams; //< If iWaveFrontSynchro, this is the number of substreams per frame (dependent tiles) or per tile (independent tiles).
#endif
  Bool      m_bUseConstrainedIntraPred;                       ///< flag for using constrained intra prediction
  Int       m_decodedPictureHashSEIEnabled;                    ///< Checksum(3)/CRC(2)/MD5(1)/disable(0) acting on decoded picture hash SEI message
  Int       m_recoveryPointSEIEnabled;
  Int       m_bufferingPeriodSEIEnabled;
  Int       m_pictureTimingSEIEnabled;
  Bool      m_toneMappingInfoSEIEnabled;
  Int       m_toneMapId;
  Bool      m_toneMapCancelFlag;
  Bool      m_toneMapPersistenceFlag;
  Int       m_toneMapCodedDataBitDepth;
  Int       m_toneMapTargetBitDepth;
  Int       m_toneMapModelId; 
  Int       m_toneMapMinValue;
  Int       m_toneMapMaxValue;
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
#if LAYERS_NOT_PRESENT_SEI
  Int       m_layersNotPresentSEIEnabled;
#endif
  Int       m_SOPDescriptionSEIEnabled;
  Int       m_scalableNestingSEIEnabled;
#if Q0189_TMVP_CONSTRAINTS
  Int       m_TMVPConstraintsSEIEnabled;
#endif
  // weighted prediction
  Bool      m_useWeightedPred;                    ///< Use of weighted prediction in P slices
  Bool      m_useWeightedBiPred;                  ///< Use of bi-directional weighted prediction in B slices
  
  UInt      m_log2ParallelMergeLevel;                         ///< Parallel merge estimation region
  UInt      m_maxNumMergeCand;                                ///< Max number of merge candidates

  Int       m_TMVPModeId;
  Int       m_signHideFlag;
#if !RC_SHVC_HARMONIZATION
  Bool      m_RCEnableRateControl;                ///< enable rate control or not
  Int       m_RCTargetBitrate;                    ///< target bitrate when rate control is enabled
  Int       m_RCKeepHierarchicalBit;              ///< 0: equal bit allocation; 1: fixed ratio bit allocation; 2: adaptive ratio bit allocation
  Bool      m_RCLCULevelRC;                       ///< true: LCU level rate control; false: picture level rate control
  Bool      m_RCUseLCUSeparateModel;              ///< use separate R-lambda model at LCU level
  Int       m_RCInitialQP;                        ///< inital QP for rate control
  Bool      m_RCForceIntraQP;                     ///< force all intra picture to use initial QP or not
#endif
  Int       m_useScalingListId;                               ///< using quantization matrix
  Char*     m_scalingListFile;                                ///< quantization matrix file name

  Bool      m_TransquantBypassEnableFlag;                     ///< transquant_bypass_enable_flag setting in PPS.
  Bool      m_CUTransquantBypassFlagForce;                    ///< if transquant_bypass_enable_flag, then, if true, all CU transquant bypass flags will be set to true.

  Bool      m_recalculateQPAccordingToLambda;                 ///< recalculate QP value according to the lambda value
  Bool      m_useStrongIntraSmoothing;                        ///< enable strong intra smoothing for 32x32 blocks where the reference samples are flat
  Int       m_activeParameterSetsSEIEnabled;

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
  Bool      m_defaultDisplayWindowFlag;                       ///< Indicates the presence of the default window parameters
  Int       m_defDispWinLeftOffset;                           ///< Specifies the left offset from the conformance window of the default window
  Int       m_defDispWinRightOffset;                          ///< Specifies the right offset from the conformance window of the default window
  Int       m_defDispWinTopOffset;                            ///< Specifies the top offset from the conformance window of the default window
  Int       m_defDispWinBottomOffset;                         ///< Specifies the bottom offset from the conformance window of the default window
  Bool      m_frameFieldInfoPresentFlag;                      ///< Indicates that pic_struct values are present in picture timing SEI messages
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
#if O0153_ALT_OUTPUT_LAYER_FLAG
  Bool      m_altOutputLayerFlag;                             ///< Specifies the value of alt_output_laye_flag in VPS extension
#endif

#if SVC_EXTENSION
  Int       m_elRapSliceBEnabled;
#endif
#if Q0074_COLOUR_REMAPPING_SEI
#if !SVC_EXTENSION
  string    m_colourRemapSEIFile;
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
#endif
  // internal member functions
#if LAYER_CTB
  Void  xSetGlobal      (UInt layerId);                       ///< set global variables
#else
  Void  xSetGlobal      ();                                   ///< set global variables
#endif
  Void  xCheckParameter ();                                   ///< check validity of configuration values
  Void  xPrintParameter ();                                   ///< print configuration values
  Void  xPrintUsage     ();                                   ///< print usage
#if SVC_EXTENSION
#if M0040_ADAPTIVE_RESOLUTION_CHANGE
  Int       m_adaptiveResolutionChange;                       ///< Indicate adaptive resolution change frame
#endif
#if HIGHER_LAYER_IRAP_SKIP_FLAG
  Bool      m_skipPictureAtArcSwitch;                         ///< Indicates that when ARC up-switching is performed the higher layer picture is a skip picture
#endif
#if REPN_FORMAT_IN_VPS
  RepFormatCfg m_repFormatCfg[16];                            ///< Rep_format structures
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
  Bool      m_interLayerConstrainedTileSetsSEIEnabled;
  UInt      m_ilNumSetsInMessage;
  Bool      m_skippedTileSetPresentFlag;
  UInt      m_topLeftTileIndex[1024];
  UInt      m_bottomRightTileIndex[1024];
  UInt      m_ilcIdc[1024];
#endif
#if O0215_PHASE_ALIGNMENT
  Bool      m_phaseAlignFlag;
#endif
#if O0223_PICTURE_TYPES_ALIGN_FLAG
  Bool      m_crossLayerPictureTypeAlignFlag;
#endif
  Bool      m_crossLayerIrapAlignFlag;
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
#if P0068_CROSS_LAYER_ALIGNED_IDR_ONLY_FOR_IRAP_FLAG
  Bool      m_crossLayerAlignedIdrOnlyFlag;
#endif
#if O0149_CROSS_LAYER_BLA_FLAG
  Bool      m_crossLayerBLAFlag;
#endif
#if O0194_WEIGHTED_PREDICTION_CGS
  Bool      m_useInterLayerWeightedPred;
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
#endif //SVC_EXTENSION
public:
  TAppEncCfg();
  virtual ~TAppEncCfg();
  
public:
  Void  create    ();                                         ///< create option handling class
  Void  destroy   ();                                         ///< destroy option handling class
  Bool  parseCfg  ( Int argc, Char* argv[] );                 ///< parse configuration file to fill member variables
  
#if SVC_EXTENSION
  Int  getNumFrameToBeEncoded()    {return m_framesToBeEncoded; }
  Int  getNumLayer()               {return m_numLayers;        }
  Int  getGOPSize()                {return m_iGOPSize;          }
#if O0194_DIFFERENT_BITDEPTH_EL_BL
  UInt getInternalBitDepthY(Int iLayer)      {return m_acLayerCfg[iLayer].m_internalBitDepthY; }
  UInt getInternalBitDepthC(Int iLayer)      {return m_acLayerCfg[iLayer].m_internalBitDepthC; }
  Bool getPCMInputBitDepthFlag()             {return m_bPCMInputBitDepthFlag;                  }
#else
  UInt getInternalBitDepthY()      {return m_internalBitDepthY; }
  UInt getInternalBitDepthC()      {return m_internalBitDepthC; }
#endif
#if !LAYER_CTB
  UInt getMaxCUWidth()             {return m_uiMaxCUWidth;      }
  UInt getMaxCUHeight()            {return m_uiMaxCUHeight;     }
  UInt getMaxCUDepth()             {return m_uiMaxCUDepth;      }
#endif
  Int  getDecodingRefreshType()    {return m_iDecodingRefreshType; }
  Int  getWaveFrontSynchro(Int layerId)        { return m_acLayerCfg[layerId].m_waveFrontSynchro; }
  Void getDirFilename(string& filename, string& dir, const string path);
#if OUTPUT_LAYER_SETS_CONFIG
  Bool scanStringToArray(string const cfgString, Int const numEntries, const char* logString, Int * const returnArray);
  Bool scanStringToArray(string const cfgString, Int const numEntries, const char* logString, std::vector<Int> &  returnVector);
  Void cfgStringToArray(Int **arr, string const cfgString, Int const numEntries, const char* logString);
#else
  Void cfgStringToArray(Int **arr, string cfgString, Int numEntries, const char* logString);
#endif
#if REPN_FORMAT_IN_VPS
  RepFormatCfg* getRepFormatCfg(Int i)  { return &m_repFormatCfg[i]; }
#endif
#if LAYER_CTB
  Bool getUsePCM()                  { return m_usePCM;               }
  UInt getPCMLog2MinSize  ()        { return  m_uiPCMLog2MinSize;    }
#endif
#endif
};// END CLASS DEFINITION TAppEncCfg

//! \}

#endif // __TAPPENCCFG__

