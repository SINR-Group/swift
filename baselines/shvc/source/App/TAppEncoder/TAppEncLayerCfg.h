
/** \file     TAppEncLayerCfg.h
    \brief    Handle encoder layer configuration parameters (header)
*/
#ifndef __TAPPENCLAYERCFG__
#define __TAPPENCLAYERCFG__

#if SVC_EXTENSION
#include "TLibCommon/CommonDef.h"
#include "TLibEncoder/TEncCfg.h"
#include <sstream>

using namespace std;
class TAppEncCfg;
//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder layer configuration class
class TAppEncLayerCfg
{
  friend class TAppEncCfg;
  friend class TAppEncTop;
protected:
  // file I/O0
  string    m_cInputFile;                                     ///< source file name
  string    m_cReconFile;                                     ///< output reconstruction file

  Int       m_iFrameRate;                                     ///< source frame-rates (Hz)
  Int       m_iSourceWidth;                                   ///< source width in pixel
  Int       m_iSourceHeight;                                  ///< source height in pixel (when interlaced = field height)
  Int       m_iSourceHeightOrg;                               ///< original source height in pixel (when interlaced = frame height)
  Int       m_conformanceMode;
  Int       m_confWinLeft;
  Int       m_confWinRight;
  Int       m_confWinTop;
  Int       m_confWinBottom;
  Int       m_aiPad[2];                                       ///< number of padded pixels for width and height
  Int       m_iIntraPeriod;                                   ///< period of I-slice (random access period)
  Double    m_fQP;                                            ///< QP value of key-picture (floating point)
#if AUXILIARY_PICTURES
  ChromaFormat m_chromaFormatIDC;
  ChromaFormat m_InputChromaFormat;
  Int          m_auxId;
#endif
#if VPS_EXTN_DIRECT_REF_LAYERS
  Int       *m_samplePredRefLayerIds;
  Int       m_numSamplePredRefLayers;
  Int       *m_motionPredRefLayerIds;
  Int       m_numMotionPredRefLayers;
  Int       *m_predLayerIds;
  Int       m_numActiveRefLayers;
#endif

#if LAYER_CTB
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

#if RC_SHVC_HARMONIZATION
  Bool      m_RCEnableRateControl;                ///< enable rate control or not
  Int       m_RCTargetBitrate;                    ///< target bitrate when rate control is enabled
  Bool      m_RCKeepHierarchicalBit;              ///< whether keeping hierarchical bit allocation structure or not
  Bool      m_RCLCULevelRC;                       ///< true: LCU level rate control; false: picture level rate control
  Bool      m_RCUseLCUSeparateModel;              ///< use separate R-lambda model at LCU level
  Int       m_RCInitialQP;                        ///< inital QP for rate control
  Bool      m_RCForceIntraQP;                     ///< force all intra picture to use initial QP or not
#endif

  Int       m_maxTidIlRefPicsPlus1;
  Int       m_waveFrontSynchro;                   ///< 0: no WPP. >= 1: WPP is enabled, the "Top right" from which inheritance occurs is this LCU offset in the line above the current.
  Int       m_iWaveFrontSubstreams;               ///< If iWaveFrontSynchro, this is the number of substreams per frame (dependent tiles) or per tile (independent tiles).

  Int       m_iQP;                                            ///< QP value of key-picture (integer)
  char*     m_pchdQPFile;                                     ///< QP offset for each slice (initialized from external file)
  Int*      m_aidQP;                                          ///< array of slice QP values
  TAppEncCfg* m_cAppEncCfg;                                   ///< pointer to app encoder config
  Int       m_numScaledRefLayerOffsets  ;
#if O0098_SCALED_REF_LAYER_ID
  Int       m_scaledRefLayerId          [MAX_LAYERS];
#endif
  Int       m_scaledRefLayerLeftOffset  [MAX_LAYERS];
  Int       m_scaledRefLayerTopOffset   [MAX_LAYERS];
  Int       m_scaledRefLayerRightOffset [MAX_LAYERS];
  Int       m_scaledRefLayerBottomOffset[MAX_LAYERS];
#if REF_REGION_OFFSET
  Bool      m_scaledRefLayerOffsetPresentFlag [MAX_LAYERS];
  Bool      m_refRegionOffsetPresentFlag      [MAX_LAYERS];
  Int       m_refRegionLeftOffset  [MAX_LAYERS];
  Int       m_refRegionTopOffset   [MAX_LAYERS];
  Int       m_refRegionRightOffset [MAX_LAYERS];
  Int       m_refRegionBottomOffset[MAX_LAYERS];
#endif
#if P0312_VERT_PHASE_ADJ
  Bool      m_vertPhasePositionEnableFlag[MAX_LAYERS];
#endif
#if R0209_GENERIC_PHASE
  Int       m_phaseHorLuma  [MAX_LAYERS];
  Int       m_phaseVerLuma  [MAX_LAYERS];
  Int       m_phaseHorChroma[MAX_LAYERS];
  Int       m_phaseVerChroma[MAX_LAYERS];
  Bool      m_resamplePhaseSetPresentFlag [MAX_LAYERS];
#endif

#if O0194_DIFFERENT_BITDEPTH_EL_BL
  Int       m_inputBitDepthY;                               ///< bit-depth of input file (luma component)
  Int       m_inputBitDepthC;                               ///< bit-depth of input file (chroma component)
  Int       m_internalBitDepthY;                            ///< bit-depth codec operates at in luma (input/output files will be converted)
  Int       m_internalBitDepthC;                            ///< bit-depth codec operates at in chroma (input/output files will be converted)
  Int       m_outputBitDepthY;                              ///< bit-depth of output file (luma component)
  Int       m_outputBitDepthC;                              ///< bit-depth of output file (chroma component)
#endif
#if REPN_FORMAT_IN_VPS
  Int       m_repFormatIdx;
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  string    m_colourRemapSEIFile;                           ///< Colour Remapping Information SEI message parameters file
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
  TAppEncLayerCfg();
  virtual ~TAppEncLayerCfg();

public:
  Void  create    ();                                         ///< create option handling class
  Void  destroy   ();                                         ///< destroy option handling class
  bool  parseCfg  ( const string& cfgFileName );              ///< parse layer configuration file to fill member variables

  Void  xPrintParameter();
  Bool  xCheckParameter( Bool isField );

  Void    setAppEncCfg(TAppEncCfg* p) {m_cAppEncCfg = p;          }

  string  getInputFile()              {return m_cInputFile;       }
  string  getReconFile()              {return m_cReconFile;       }
  Int     getFrameRate()              {return m_iFrameRate;       }
  Int     getSourceWidth()            {return m_iSourceWidth;     }
  Int     getSourceHeight()           {return m_iSourceHeight;    }
  Int     getSourceHeightOrg()        {return m_iSourceHeightOrg; }
  Int     getConformanceMode()        { return m_conformanceMode; }
  Int*    getPad()                    {return m_aiPad;            }
  Double  getFloatQP()                {return m_fQP;              }
  Int     getConfWinLeft()            {return m_confWinLeft;         }
  Int     getConfWinRight()           {return m_confWinRight;        }
  Int     getConfWinTop()             {return m_confWinTop;          }
  Int     getConfWinBottom()          {return m_confWinBottom;       }
#if AUXILIARY_PICTURES
  ChromaFormat getInputChromaFormat()   {return m_InputChromaFormat;}
  ChromaFormat getChromaFormatIDC()     {return m_chromaFormatIDC;  }
  Int          getAuxId()               {return m_auxId;            }
#endif

  Int     getIntQP()                  {return m_iQP;              } 
  Int*    getdQPs()                   {return m_aidQP;            }
#if VPS_EXTN_DIRECT_REF_LAYERS
  Int     getNumSamplePredRefLayers()    {return m_numSamplePredRefLayers;   }
  Int*    getSamplePredRefLayerIds()     {return m_samplePredRefLayerIds;    }
  Int     getSamplePredRefLayerId(Int i) {return m_samplePredRefLayerIds[i]; }
  Int     getNumMotionPredRefLayers()    {return m_numMotionPredRefLayers;   }
  Int*    getMotionPredRefLayerIds()     {return m_motionPredRefLayerIds;    }
  Int     getMotionPredRefLayerId(Int i) {return m_motionPredRefLayerIds[i]; }

  Int     getNumActiveRefLayers()     {return m_numActiveRefLayers;}
  Int*    getPredLayerIds()           {return m_predLayerIds;     }
  Int     getPredLayerId(Int i)       {return m_predLayerIds[i];  }
#endif
#if RC_SHVC_HARMONIZATION
  Bool    getRCEnableRateControl()    {return m_RCEnableRateControl;   }
  Int     getRCTargetBitrate()        {return m_RCTargetBitrate;       }
  Bool    getRCKeepHierarchicalBit()  {return m_RCKeepHierarchicalBit; }
  Bool    getRCLCULevelRC()           {return m_RCLCULevelRC;          }
  Bool    getRCUseLCUSeparateModel()  {return m_RCUseLCUSeparateModel; }
  Int     getRCInitialQP()            {return m_RCInitialQP;           }
  Bool    getRCForceIntraQP()         {return m_RCForceIntraQP;        }
#endif
#if REPN_FORMAT_IN_VPS
  Int     getRepFormatIdx()           { return m_repFormatIdx;  }
  Void    setRepFormatIdx(Int x)      { m_repFormatIdx = x;     }
  Void    setSourceWidth(Int x)       { m_iSourceWidth = x;     }
  Void    setSourceHeight(Int x)      { m_iSourceHeight = x;    }
#endif
  Int     getMaxTidIlRefPicsPlus1()   { return m_maxTidIlRefPicsPlus1; }
#if LAYER_CTB
  UInt    getMaxCUWidth()             {return m_uiMaxCUWidth;      }
  UInt    getMaxCUHeight()            {return m_uiMaxCUHeight;     }
  UInt    getMaxCUDepth()             {return m_uiMaxCUDepth;      }
#endif
}; // END CLASS DEFINITION TAppEncLayerCfg

#endif //SVC_EXTENSION

//! \}

#endif // __TAPPENCLAYERCFG__
