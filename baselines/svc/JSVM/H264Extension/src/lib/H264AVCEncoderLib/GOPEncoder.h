
#if !defined(AFX_GOPENCODER_H__75F41F36_C28D_41F9_AB5E_4C90D66D160C__INCLUDED_)
#define AFX_GOPENCODER_H__75F41F36_C28D_41F9_AB5E_4C90D66D160C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#include "H264AVCCommonLib/TraceFile.h"
#include "H264AVCCommonLib/MbDataCtrl.h"
#include "H264AVCCommonLib/Frame.h"
#include "H264AVCCommonLib/ParameterSetMng.h"
#include "DownConvert.h"
#include "H264AVCCommonLib/Sei.h"

#include <algorithm>
#include <list>

#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif


H264AVC_NAMESPACE_BEGIN


class H264AVCEncoder;
class SliceHeader;
class SliceEncoder;
class PocCalculator;
class LoopFilter;
class CodingParameter;
class LayerParameters;
class RateDistortionIf;
class HeaderSymbolWriteIf;
class NalUnitEncoder;
class ControlMngIf;
class ParameterSetMng;
class ControlMngH264AVCEncoder;
class MotionEstimation;
class Frame;
class ReconstructionBypass;
class Scheduler;

typedef MyList<UInt> UIntList;


class H264AVCENCODERLIB_API AccessUnitData
{
public:
  AccessUnitData( UInt uiAUIndex )
    : m_uiAUIndex       ( uiAUIndex )
    , m_pcNonRequiredSei( 0 )
  {}
  ~AccessUnitData() {}

  UInt                    getAUIndex            () const  { return m_uiAUIndex; }
  ExtBinDataAccessorList& getBPSEINalUnitList   ()        { return m_cBPSEINalUnitList; }
  ExtBinDataAccessorList& getSEINalUnitList     ()        { return m_cSEINalUnitList; }
  ExtBinDataAccessorList& getNalUnitList        ()        { return m_cNalUnitList; }
  ExtBinDataAccessorList& getRedNalUnitList     ()        { return m_cRedNalUnitList; }
  SEI::NonRequiredSei*	  getNonRequiredSei     ()				{ return m_pcNonRequiredSei; }
  SEI::IntegrityCheckSEI* getIntegrityCheckSei  ()        { return m_pcIntegrityCheckSei; }
  ErrVal				          CreatNonRequiredSei   ()				{ RNOK( SEI::NonRequiredSei::   create( m_pcNonRequiredSei    ) ); return Err::m_nOK; }
	ErrVal					        CreatIntegrityCheckSei()        { RNOK( SEI::IntegrityCheckSEI::create( m_pcIntegrityCheckSei ) ); return Err::m_nOK; }

private:
  UInt                    m_uiAUIndex;
  ExtBinDataAccessorList  m_cBPSEINalUnitList;
  ExtBinDataAccessorList  m_cSEINalUnitList;
  ExtBinDataAccessorList  m_cNalUnitList;
  ExtBinDataAccessorList  m_cRedNalUnitList;
  SEI::NonRequiredSei*	  m_pcNonRequiredSei;
	SEI::IntegrityCheckSEI* m_pcIntegrityCheckSei;
};

class H264AVCENCODERLIB_API AccessUnitDataList
{
public:
  AccessUnitDataList  ()  {}
  ~AccessUnitDataList ()  {}

  Void clear() { m_cAccessUnitDataList.clear(); }
  
  AccessUnitData& getAccessUnitData( UInt uiAUIndex )
  {
    //===== hack for EIDR (this class should be removed completely =====
    std::list<AccessUnitData>::iterator  iter = m_cAccessUnitDataList.begin();
    std::list<AccessUnitData>::iterator  end  = m_cAccessUnitDataList.end  ();
    for( ; iter != end; iter++ )
    {
      if( (*iter).getAUIndex() == uiAUIndex )
      {
        return (*iter);
      }
    }
    AccessUnitData cAUData( uiAUIndex );
    m_cAccessUnitDataList.push_back( cAUData );
    return m_cAccessUnitDataList.back();
  }

  Void emptyNALULists( ExtBinDataAccessorList& rcOutputList )
  {
    while( ! m_cAccessUnitDataList.empty() )
    {
      ExtBinDataAccessorList& rcBPSEINaluList = m_cAccessUnitDataList.front().getBPSEINalUnitList();
      rcOutputList += rcBPSEINaluList;
      rcBPSEINaluList.clear();
      ExtBinDataAccessorList& rcSEINaluList = m_cAccessUnitDataList.front().getSEINalUnitList();
      rcOutputList += rcSEINaluList;
      rcSEINaluList.clear();
      ExtBinDataAccessorList& rcNaluList = m_cAccessUnitDataList.front().getNalUnitList();
      rcOutputList += rcNaluList;
      rcNaluList.clear();
      ExtBinDataAccessorList& rcRedNaluList = m_cAccessUnitDataList.front().getRedNalUnitList();
      rcOutputList += rcRedNaluList;
      rcRedNaluList.clear();
      m_cAccessUnitDataList.pop_front();
    }
  }

private:
  std::list<AccessUnitData>  m_cAccessUnitDataList;
};


class PicOutputData
{
public:
  PicOutputData()
    : uiBaseQualityLevel( MSYS_UINT_MAX )
    , uiBaseLayerId     ( MSYS_UINT_MAX )
  {}
public:
  Bool    FirstPicInAU;
  Int     Poc;
  Char    FrameType[3];
  Int     DependencyId;
  Int     QualityId;
  Int     TemporalId;
  Int     Qp;
  Int     Bits;
  Double  YPSNR;
  Double  UPSNR;
  Double  VPSNR;
  Int     iPicType;
  UInt    uiBaseQualityLevel;
  UInt    uiBaseLayerId;
};

typedef MyList<PicOutputData> PicOutputDataList;




class H264AVCENCODERLIB_API LayerEncoder
{
  enum
  {
    NUM_TMP_FRAMES  = 9
  };
  enum RefListUsage
  {
    RLU_UNDEFINED         = 0,
    RLU_MOTION_ESTIMATION = 1,
    RLU_GET_RESIDUAL      = 2,
    RLU_RECONSTRUCTION    = 3,
    RLU_REF_BASE_PIC      = 4
  };

protected:
	LayerEncoder          ();
	virtual ~LayerEncoder ();

public:
  static ErrVal create              ( LayerEncoder*&                  rpcLayerEncoder );
  ErrVal        destroy             ();
  ErrVal        init                ( CodingParameter*                pcCodingParameter,
                                      LayerParameters*                pcLayerParameters,
                                      H264AVCEncoder*                 pcH264AVCEncoder,
                                      SliceEncoder*                   pcSliceEncoder,
                                      LoopFilter*                     pcLoopFilter,
                                      PocCalculator*                  pcPocCalculator,
                                      NalUnitEncoder*                 pcNalUnitEncoder,
                                      YuvBufferCtrl*                  pcYuvFullPelBufferCtrl,
                                      YuvBufferCtrl*                  pcYuvHalfPelBufferCtrl,
                                      QuarterPelFilter*               pcQuarterPelFilter,
                                      MotionEstimation*               pcMotionEstimation,
                                      ReconstructionBypass*           pcReconstructionBypass,
                                      StatBuf<Scheduler*, MAX_SCALABLE_LAYERS>* apcScheduler );
  ErrVal        initParameterSets   ( const SequenceParameterSet&     rcSPS,
                                      const PictureParameterSet&      rcPPS );
  ErrVal        uninit              ();

  ErrVal        addParameterSetBits ( UInt                            uiParameterSetBits );
  Bool          firstGOPCoded       ()                                { return m_bFirstGOPCoded; }
  ErrVal        initGOP             ( AccessUnitData&                 rcAccessUnitData,
                                      PicBufferList&                  rcPicBufferInputList );
  ErrVal        process             ( UInt                            uiAUIndex,
                                      AccessUnitData&                 rcAccessUnitData,
                                      PicBufferList&                  rcPicBufferInputList,
                                      PicBufferList&                  rcPicBufferOutputList,
                                      PicBufferList&                  rcPicBufferUnusedList,
									                    ParameterSetMng*                pcParameterSetMng );
  ErrVal        finish              ( UInt&                           ruiNumCodedFrames,
                                      Double&                         rdOutputRate,
                                      Double                          aaadOutputFramerate[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS],
                                      Double                          aaadBits[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS] );

  UInt          getPicCodingType    ( UInt                            uiTemporalId,
                                      UInt                            uiFrmIdInTLayer );
  ErrVal        updateMaxSliceSize  ( UInt                            uiAUIndex,
                                      UInt&                           ruiMaxSliceSize );
  ErrVal        getBaseLayerStatus  ( Bool&                           bExists,
																			PicType                         ePicType,
																			UInt														uiTemporalId );
  Bool          isMGSEnhancementLayer( UInt&                          ruiLevelIdc );
  ErrVal        getBaseLayerLevelIdc( UInt&                           uiLevelIdc,
                                      Bool&                           bBiPred8x8Disable,
                                      Bool&                           bMCBlks8x8Disable );
	ErrVal        getBaseLayerDataAvlb( Frame*&                         pcFrame,
  																	  Frame*&                         pcResidual,
																			MbDataCtrl*&                    pcMbDataCtrl,
																			PicType                         ePicType,
																			UInt											      uiTemporalId );
  ErrVal        getBaseLayerData    ( SliceHeader&                    rcELSH,
                                      Frame*&                         pcFrame,
																			Frame*&                         pcResidual,
																			MbDataCtrl*&                    pcMbDataCtrl,
																			Bool                            bSpatialScalability,
																			PicType                         ePicType,
																			UInt												    uiTemporalId );

  UInt          getNewBits          ()        { UInt ui = m_uiNewlyCodedBits; m_uiNewlyCodedBits = 0; return ui; }
	UInt          getScalableLayer    ()  const { return m_uiScalableLayerId; }

  Frame*        getMGSLPRec         ( UInt uiLowPassIndex );
  Frame*        getRefPic           ( UInt uiTemporalId, UInt uiFrameIdInTId );

  Void			    setNonRequiredWrite ( UInt ui )       { m_uiNonRequiredWrite = ui; } //NonRequired JVT-Q066 (06-04-08)
  //Bug_Fix JVT-R057{
  Bool          getLARDOEnable      ( )               { return m_bLARDOEnable; }
  Void          setLARDOEnable      ( Bool bEnable )  { m_bLARDOEnable= bEnable; }
  //Bug_Fix JVT-R057{
//JVT-T054{
  Void          setLayerCGSSNR(UInt ui) { m_uiLayerCGSSNR = ui;}
  Void          setQualityLevelCGSSNR(UInt ui) { m_uiQualityLevelCGSSNR = ui;}
  UInt          getLayerCGSSNR() { return m_uiLayerCGSSNR;}
  UInt          getQualityLevelCGSSNR() { return m_uiQualityLevelCGSSNR;}
  Void          setBaseLayerCGSSNR(UInt ui) { m_uiBaseLayerCGSSNR = ui;}
  Void          setBaseQualityLevelCGSSNR(UInt ui) { m_uiBaseQualityLevelCGSSNR = ui;}
  UInt          getBaseLayerCGSSNR() { return m_uiBaseLayerCGSSNR;}
  UInt          getBaseQualityLevelCGSSNR() { return m_uiBaseQualityLevelCGSSNR;}
//JVT-T054}

  Frame*        getBaseLayerResidual()  { return m_pcBaseLayerResidual;} // this one is upsampled base layer's residual

  // JVT-AD021 {
  ErrVal        getCurQpPredData    ( Double& dQpPredData, Int& iPOC, UInt& uiFrameSizeInMB ) const;
  // JVT-AD021 }

protected:
  //===== data management =====
  ErrVal  xCreateData                   ( const SequenceParameterSet& rcSPS );
  ErrVal  xDeleteData                   ();
  ErrVal  xInitBitCounts                ();
  ErrVal  xInitGOP                      ( PicBufferList&              rcPicBufferInputList );
  ErrVal  xFinishGOP                    ( PicBufferList&              rcPicBufferInputList,
                                          PicBufferList&              rcPicBufferOutputList,
                                          PicBufferList&              rcPicBufferUnusedList );
  ErrVal  xInitExtBinDataAccessor       ( ExtBinDataAccessor&         rcExtBinDataAccessor );
  ErrVal  xAppendNewExtBinDataAccessor  ( ExtBinDataAccessorList&     rcExtBinDataAccessorList,
                                          ExtBinDataAccessor*         pcExtBinDataAccessor );

  //===== coding and related =====
  ErrVal  xEncodePicture                ( Bool&                       rbPictureCoded,
                                          UInt                        uiTemporalId,
                                          UInt                        uiFrameIdInGOP,
                                          AccessUnitData&             rcAccessUnitData,
                                          PicBufferList&              rcPicBufferInputList );
  ErrVal  xStoreReconstruction          ( PicBufferList&              rcPicBufferOutputList );
  
  ErrVal  xEncodeLayerRepresentation    ( ExtBinDataAccessorList&     rcOutExtBinDataAccessorList,
                                          ControlData&                rcControlData,
                                          Frame*                      pcOrgFrame,
                                          Frame*                      pcFrame,
                                          Frame*                      pcResidualLF,
                                          Frame*                      pcResidualILPred,
                                          Frame*                      pcPredSignal,
                                          UInt&                       ruiBits,
                                          PicOutputDataList&          rcPicOutputDataList,
                                          UInt                        uiFrameIdInGOP,
                                          PicType                     ePicType );
  ErrVal  xCalculateAndAddPSNR          ( UInt                        uiStage,
                                          UInt                        uiFrame,
                                          PicBufferList&              rcPicBufferInputList,
                                          PicOutputDataList&          rcPicOutputDataList );
  ErrVal  xOutputPicData                ( PicOutputDataList&          rcPicOutputDataList );


  //===== data initialization =====
  ErrVal  xSetScalingFactors            ();
  ErrVal  xSetScalingFactors            ( UInt                        uiBaseLevel );
  // JVT-AD021 {
  Double xGetGamma                      ( Double                      dQpPredData,
                                          ControlData&                rcControlData );
  // JVT-AD021 }
  Void    xPAffDecision                 ( UInt                        uiFrame );
  ErrVal  xGetAndSetPredictionLists     ( ControlData&                rcControlData,
                                          UInt                        uiFrameIdInGOP,
                                          PicType                     ePicType,
                                          Bool                        bHalfPel );
  ErrVal  xGetPredictionLists           ( RefFrameList&               rcRefList0,
                                          RefFrameList&               rcRefList1,
                                          UInt&                       ruiFrameIdCol,
                                          UInt                        uiFrameIdInGOP,
                                          PicType                     ePicType,
                                          RefListUsage                eRefListUsage,
                                          Bool                        bHalfPel = false );
 	ErrVal  xSetBaseLayerData             ( UInt                        uiFrameIdInGOP,
		                                      PicType                     ePicType );
  ErrVal  xInitBaseLayerData            ( ControlData&                rcControlData,
																					PicType                     ePicType );
  ErrVal  xInitControlData              ( UInt                        uiFrameIdInGOP,
                                          UInt                        uiTemporalId,
                                          PicType                     ePicType );

  //===== basic encoding =====
  ErrVal  xFillAndUpsampleFrame         ( Frame*                   pcFrame,
		                                      PicType                     ePicType,
																					Bool                        bFrameMbsOnlyFlag );

  ErrVal  xFillAndExtendFrame           ( Frame*                   pcFrame,
		                                      PicType                     ePicType,
																					Bool                        bFrameMbsOnlyFlag );
  ErrVal  xClearBufferExtensions        ();

  ErrVal  xUpdateLowPassRec             ( UInt                        uiLowPassIndex );
  Frame*  xGetRefFrame                  ( UInt                        uiRefIndex,
                                          RefListUsage                eRefListUsage );
  ErrVal  xClearELPics                  ();
  ErrVal  xUpdateELPics                 ();


  //===== slice header =====
  ErrVal  xWriteSEI                     ( ExtBinDataAccessorList& rcOutExtBinDataAccessorList, SliceHeader& rcSH, UInt& ruiBit );
  ErrVal	xWritePrefixUnit              ( ExtBinDataAccessorList& rcOutExtBinDataAccessorList, SliceHeader& rcSH, UInt& ruiBit );//prefix unit
  //NonRequired JVT-Q066 (06-04-08){{
  ErrVal	xWriteNonRequiredSEI          ( ExtBinDataAccessorList& rcOutExtBinDataAccessorList, SEI::NonRequiredSei* pcNonRequiredSei, UInt& ruiBit );
  ErrVal	xSetNonRequiredSEI            ( SliceHeader* pcSliceHeader, SEI::NonRequiredSei* pcNonRequiredSei);
  //NonRequired JVT-Q066 (06-04-08)}}
  // JVT-V068 HRD {
  ErrVal  xWriteSEI                     ( ExtBinDataAccessorList &rcOutExtBinDataAccessorList, SEI::MessageList& rcSEIMessageList, UInt &ruiBits );
  ErrVal  xWriteNestingSEIforHrd        ( ExtBinDataAccessorList &rcOutExtBinDataAccessorList, SEI::SEIMessage *pcSEIMessage, UInt uiuiDependencyId, UInt uiQualityLevel, UInt uiTemporalLevel, UInt &ruiBits );
  ErrVal  xWriteSEIforAVCCompatibleHrd  ( ExtBinDataAccessorList &rcOutExtBinDataAccessorList, SEI::SEIMessage* pcSEIMessage, UInt &ruiBits );
  ErrVal  writeTl0DepRepIdxSEI              ( ExtBinDataAccessorList &rcOutExtBinDataAccessorList, UInt uiTl0DepRepIdx, UInt uiEfIdrPicId, UInt& ruiBits );
  ErrVal  writeNestingTl0DepRepIdxSEIMessage( ExtBinDataAccessorList &rcOutExtBinDataAccessorList, UInt uiTId, UInt uiDId, UInt uiTl0DepRepIdx, UInt uiEfIdrPicId, UInt& ruiBits );
  ErrVal  writeNestingSceneInfoSEIMessage   ( ExtBinDataAccessorList &rcOutExtBinDataAccessorList, UInt uiTId, UInt uiDId, UInt& ruiBits );
  ErrVal  xCalculateTiming              ( PicOutputDataList&  rcPicOutputDataList, UInt uiFrame );
  // JVT- V068 HRD }
	ErrVal  xWriteRedundantKeyPicSEI      ( ExtBinDataAccessorList&  rcOutExtBinDataAccessorList, UInt &ruiBits ); //JVT-W049
	//JVT-W052 wxwan
	ErrVal  xWriteIntegrityCheckSEI       ( ExtBinDataAccessorList &rcOutExtBinDataAccessorList, SEI::SEIMessage *pcSEIMessage, UInt &ruiBits );
	//JVT-W052 wxwan
  Void    xAssignSimplePriorityId       ( SliceHeader *pcSliceHeader );

  UInt		getPreAndSuffixUnitEnable     ()	        { return m_uiPreAndSuffixUnitEnable;} //JVT-S036 lsj
  Bool		getMMCOBaseEnable		          ()	const	  { return m_bMMCOBaseEnable; } //JVT-S036 lsj
  Bool		getMMCOEnable		              ()	const	  { return m_bMMCOEnable; } //JVT-S036 lsj
  //S051{
  Bool	  xSIPCheck	                    ( UInt POC );
  //S051}
  Void    setMCResizeParameters         ( ResizeParameters*			resizeParameters);

  ErrVal  xInitCodingOrder              ( UInt                  uiDecompositionStages,
                                          UInt                  uiTemporalSubSampling,
                                          UInt                  uiMaxFrameDelay );
  ErrVal  xInitRefFrameLists            ( UInt                  uiFrameIdInGOP,
                                          UInt                  uiTemporalId );
  ErrVal  xSetReorderingCommandsST      ( RefPicListReOrdering& rcReOrdering,
                                          UIntList&             cPicNumList,
                                          UInt                  uiCurrPicNum,
                                          Bool                  bNoReordering );
  ErrVal  xSetReorderingCommandsLT      ( RefPicListReOrdering& rcReOrdering,
                                          UIntList&             cLTPicNumList );
  ErrVal  xSetDecRefPicMarkingST        ( DecRefPicMarking&     rcDecRefPicMarking,
                                          UIntList&             cPicNumList,
                                          UInt                  uiCurrPicNum,
                                          UInt                  uiMaxPicNum );
  ErrVal  xSetDecRefPicMarkingLT        ( DecRefPicMarking&     rcDecRefPicMarking,
                                          UInt                  uiLongTermFrameNum,
                                          Bool                  bSendMaxLongTermIdx,
                                          Int                   iMaxLongTermIdx );
  ErrVal  xSetSliceTypeAndRefLists      ( SliceHeader&          rcSliceHeader,
                                          UInt                  uiFrameIdInGOP,
                                          PicType               ePicType );
  ErrVal  xSetSliceHeaderBase           ( SliceHeader&          rcSliceHeader,
                                          UInt                  uiFrameIdInGOP,
                                          PicType               ePicType );
  ErrVal  xSetMMCOAndUpdateParameters   ( SliceHeader&          rcSliceHeader,
                                          UInt                  uiFrameIdInGOP,
                                          PicType               ePicType );
  ErrVal  xInitSliceHeader              ( UInt                  uiFrameIdInGOP,
                                          PicType               ePicType );
  ErrVal  xInitSliceHeadersAndRefLists  ();

  ErrVal  xGetInitialOutputDelay        ( UInt&                 ruiInitDelay,
                                          UInt                  uiTemporalLevel );
  ErrVal  xSetCodingNumbers             ( UInt                  uiCodingIndex );
  ErrVal  xGetRelativeOutputDelay       ( UInt&                 ruiDelay,
                                          UInt                  uiFrameIdInGOP,
                                          UInt                  uiTemporalLevel );

protected:
  //----- instances -----
  ExtBinDataAccessor            m_cExtBinDataAccessor;
  BinData                       m_cBinData;
  DownConvert                   m_cDownConvert;

  //----- references -----
  const SequenceParameterSet*   m_pcSPS;
  const PictureParameterSet*    m_pcPPS;

  YuvBufferCtrl*                m_pcYuvFullPelBufferCtrl;
  YuvBufferCtrl*                m_pcYuvHalfPelBufferCtrl;
  PocCalculator*                m_pcPocCalculator;
  H264AVCEncoder*               m_pcH264AVCEncoder;
  SliceEncoder*                 m_pcSliceEncoder;
  NalUnitEncoder*               m_pcNalUnitEncoder;
  LoopFilter*                   m_pcLoopFilter;
  QuarterPelFilter*             m_pcQuarterPelFilter;
  MotionEstimation*             m_pcMotionEstimation;
  CodingParameter*              m_pcCodingParameter;
  LayerParameters*              m_pcLayerParameters;

  //----- fixed control parameters ----
  Bool                          m_bTraceEnable;                       // trace file
  Bool                          m_bFrameMbsOnlyFlag;                  // frame macroblocks only block
  UInt                          m_uiDependencyId;                     // layer id for current layer
  UInt                          m_uiScalableLayerId;                  // scalable layer id for current layer
  UInt                          m_uiLayerId;                          // current layer id
  UInt                          m_uiBaseLayerId;                      // layer id of base layer
  UInt                          m_uiBaseQualityLevel;                 // quality level of the base layer
  UInt                          m_uiFrameWidthInMb;                   // frame width in macroblocks
  UInt                          m_uiFrameHeightInMb;                  // frame height in macroblocks
  UInt                          m_uiMaxGOPSize;                       // maximum possible GOP size (specified by the level)
  UInt                          m_uiDecompositionStages;              // number of decomposition stages
  UInt                          m_uiTemporalResolution;               // temporal subsampling in comparison to highest layer
  UInt                          m_uiNotCodedStages;                   // number of stages that are only used for temporal downsampling
  UInt                          m_uiFrameDelay;                       // maximum possible delay in frames
  UInt                          m_uiMaxNumRefFrames;                  // maximum number of active reference pictures in a list
  UInt                          m_uiLowPassIntraPeriod;               // intra period for lowest temporal resolution
  UInt                          m_uiNumMaxIter;                       // maximum number of iteration for bi-directional search
  UInt                          m_uiIterSearchRange;                  // search range for iterative search
  UInt                          m_uiMaxDeltaQp;                       // maximum QP changing
  Bool                          m_bH264AVCCompatible;                 // H.264/AVC compatibility
  Bool                          m_bInterLayerPrediction;              // inter-layer prediction
  Bool                          m_bAdaptiveBaseModeFlag;              // adaptive base mode flag
  Bool                          m_bAdaptiveMotionPredFlag;            // adaptive motion prediction flag
  Bool                          m_bAdaptiveResidualPredFlag;          // adaptive residual prediction flag
  Bool                          m_bDefaultBaseModeFlag;               // default base mode flag
  Bool                          m_bDefaultMotionPredFlag;             // default motion prediction flag
  Bool                          m_bDefaultResidualPredFlag;           // default residual prediction flag
  Bool                          m_bForceReOrderingCommands;           // always write re-ordering commands (error robustness)
  Bool                          m_bWriteSubSequenceSei;               // Subsequence SEI message (H.264/AVC base layer)
  Double                        m_adBaseQpLambdaMotion[MAX_DSTAGES];  // base QP's for mode decision and motion estimation
  Double                        m_dBaseQpLambdaMotionLP;              // base QP for mode decision and motion estimation of key pics
  Double                        m_dBaseQPResidual;                    // base residual QP

  UInt                          m_uiFilterIdc;                        // de-blocking filter idc
  Int                           m_iAlphaOffset;                       // alpha offset for de-blocking filter
  Int                           m_iBetaOffset;                        // beta offset for de-blocking filter
  UInt                          m_uiInterLayerFilterIdc;              // de-blocking filter idc
  Int                           m_iInterLayerAlphaOffset;             // alpha offset for de-blocking filter
  Int                           m_iInterLayerBetaOffset;              // beta offset for de-blocking filter

  //----- variable control parameters -----
  Bool                          m_bInitDone;                          // initilisation
  Bool                          m_bFirstGOPCoded;                     // true if first GOP of a sequence has been coded
  UInt                          m_uiGOPSize;                          // current GOP size
  UInt                          m_uiFrameCounter;                     // current frame counter
  UInt                          m_uiFrameNum;                         // current value of syntax element frame_num
  UInt                          m_uiGOPNumber;                        // number of coded GOP's
  Bool                          m_abIsRef[MAX_DSTAGES];               // state of temporal layer (H.264/AVC base layer)
  UIntList                      m_cLPFrameNumList;                    // list of frame_num for low-pass frames
	UInt													m_uiIdrPicId;	//EIDR 0619
  UInt                          m_uiLastIdrPicId;
  UInt                          m_uiTl0DepRepIdx;

  //----- frame memories -----
  Frame*                        m_apcFrameTemp[NUM_TMP_FRAMES];       // auxiliary frame memories
  Frame**                       m_papcFrame;                          // frame stores
  Frame**                       m_papcELFrame;                        // higher layer reference frames
  Frame*                        m_pcResidualLF;                       // frame stores for residual data
  Frame*                        m_pcResidualILPred;                   // frame stores for residual data
  Frame*                        m_pcSubband;                          // reconstructed subband pictures
  Frame*                        m_apcBaseFrame[2];                    // base reconstruction of last low-pass picture
//TMM_WP
  Bool                          m_bBaseLayerWp;
//TMM_WP
  Frame*                        m_pcAnchorFrameOriginal;              // original anchor frame
  Frame*                        m_pcAnchorFrameReconstructed;         // reconstructed anchor frame
  Frame*                        m_pcBaseLayerFrame;                   // base layer frame
  Frame*                        m_pcBaseLayerResidual;                // base layer residual

  //----- control data arrays -----
  ControlData*                  m_pacControlData;                     // control data arrays
  MbDataCtrl*                   m_pcBaseLayerCtrl;                    // macroblock data of the base layer pictures
  MbDataCtrl*                   m_pcBaseLayerCtrlField;
  MbDataCtrl*                   m_pcRedundantCtrl;

  //----- auxiliary buffers -----
  UInt                          m_uiWriteBufferSize;                  // size of temporary write buffer
  UChar*                        m_pucWriteBuffer;                     // write buffer

  //----- PSNR & rate  -----
  Double                        m_fOutputFrameRate;
  UInt                          m_uiParameterSetBits;
  UInt                          m_auiNumFramesCoded [MAX_DSTAGES+1];
  UInt                          m_uiNewlyCodedBits;
  Double                        m_adPSNRSumY        [MAX_DSTAGES+1];
  Double                        m_adPSNRSumU        [MAX_DSTAGES+1];
  Double                        m_adPSNRSumV        [MAX_DSTAGES+1];

  //----- ESS -----
  ResizeParameters*				      m_pcResizeParameters;
  FILE*                         m_pESSFile;
  Bool*                         m_pbFieldPicFlag;
// JVT-Q065 EIDR{
  Int           						  	m_iIDRPeriod;
// JVT-Q065 EIDR}
  Int                           m_iLayerIntraPeriod;
  //JVT-R057 LA-RDO{
  Bool                          m_bLARDOEnable;
  //JVT-R057 LA-RD}
  UInt                          m_uiEssRPChkEnable;
  UInt                          m_uiMVThres;
  UInt							            m_uiNonRequiredWrite; //NonRequired JVT-Q066 (06-04-08)
  UInt							            m_uiPreAndSuffixUnitEnable; //JVT-S036 lsj
  Bool							            m_bMMCOBaseEnable;  //JVT-S036 lsj
  Bool                          m_bMMCOEnable; // (non-base MMCO)
  //S051{
  UInt							            m_uiTotalFrame;
  UInt*							            m_auiFrameBits;
  UIntList					            m_cPocList;
  UInt						            	m_uiAnaSIP;
  Bool						            	m_bEncSIP;
  std::string				            m_cInSIPFileName;
  std::string				            m_cOutSIPFileName;
  //S051}
  // JVT-S054 (ADD) ->
  UInt*                         m_puiFirstMbInSlice;
  // JVT-S054 (ADD) <-
//JVT-T054{
  UInt                          m_uiLayerCGSSNR;
  UInt                          m_uiQualityLevelCGSSNR;
  UInt                          m_uiBaseLayerCGSSNR;
  UInt                          m_uiBaseQualityLevelCGSSNR;
//JVT-T054}
//DS_FIX_FT_09_2007
  Bool                          m_bDiscardable;
//~DS_FIX_FT_09_2007
// JVT-U085 LMI
  Bool                          m_bTlevelNestingFlag;
// JVT-U116 W062 LMI
  Bool                          m_bTl0DepRepIdxEnable;
  //JVT-U106 Behaviour at slice boundaries{
  Bool                          m_bCIUFlag;
  ReconstructionBypass*         m_pcReconstructionBypass;
  //JVT-U106 Behaviour at slice boundaries}
  Bool*                         m_apabBaseModeFlagAllowedArrays[2];
  UInt                          m_uiMinScalableLayer;
  UInt                          m_uiFramesInCompleteGOPsProcessed;
  Bool                          m_bGOPInitialized;
  MbDataCtrl*                   m_pcBaseMbDataCtrl;

  UInt                          m_uiEncodeKeyPictures;
  Bool                          m_bSameResBL;
  Bool                          m_bSameResEL;
  Bool                          m_bMGS;
  UInt                          m_uiMGSKeyPictureControl;
  Bool                          m_bHighestMGSLayer;
  Bool                          m_abCoded[(1<<MAX_DSTAGES)+1];
  Bool                          m_bExplicitQPCascading;
  Double                        m_adDeltaQPTLevel[MAX_TEMP_LEVELS];

  // JVT-V068 HRD {
  StatBuf<Scheduler*, MAX_SCALABLE_LAYERS>* m_apcScheduler;
  ParameterSetMng*              m_pcParameterSetMng;
  Bool                          m_bEnableHrd;
  // JVT-V068 HRD }
	// JVT-W049 {
  UInt                          m_uiNumberLayersCnt;
  // JVT-W049 }

  Bool          m_bNominalBiPred8x8Disable;
  Bool          m_bNominalMCBlks8x8Disable;
  Bool          m_bBiPred8x8Disable;
  Bool          m_bMCBlks8x8Disable;
  Bool          m_bBotFieldFirst;
  Bool          m_bUseLongTermPics;
  Int           m_iMaxLongTermFrmIdx;
  UInt          m_uiNextTL0LongTermIdx;
  UInt          m_uiPicCodingType; // 0:B / 1:Hier.P / 2:B on top of Hier. P
  UInt          m_uiFrameIdWithSmallestTId;
  UInt          m_uiNumFramesLeftInGOP;
  UInt          m_auiPicCodingType            [(1<<MAX_DSTAGES)+1];
  UInt          m_auiFrameIdToTemporalId      [(1<<MAX_DSTAGES)+1];
  UInt          m_auiCodingIndexToFrameId     [(1<<MAX_DSTAGES)+1];
  UInt          m_auiFrameIdToFrameNumOrLTIdx [(1<<MAX_DSTAGES)+1];
  UIntList      m_acRefPicListFrameId_L0      [(1<<MAX_DSTAGES)+1];
  UIntList      m_acRefPicListFrameId_L1      [(1<<MAX_DSTAGES)+1];
  Bool          m_abMaxLongTermIdxSend        [    MAX_DSTAGES +1];
  UIntList      m_acActiveRefListFrameNum     [    MAX_DSTAGES +1];
  UInt          m_auiCodingNumber             [    MAX_DSTAGES +1];
  UInt          m_auiInitialDelay             [    MAX_DSTAGES +1];
  UIntList      m_cActiveRefBasePicListFrameNum;


  // JVT-AD021 {1
  UInt			    m_uiMultiLayerLambda;
  Double		    m_dQpPredData;
  Int			      m_iCurPOC;
  UInt			    m_uiHighestTempLevel;
  // JVT-AD021 }

  //JVT-W051 {
public:
	UInt		m_uiProfileIdc;
	UInt		m_uiLevelIdc;
	Bool		m_bConstraint0Flag;
	Bool		m_bConstraint1Flag;
	Bool		m_bConstraint2Flag;
	Bool		m_bConstraint3Flag;
	//JVT-W051 }
public:
	Bool    m_bOutputFlag;//JVT-W047
	//JVT-X046 {
  UInt    m_uiSliceMode;
  UInt    m_uiSliceArgument;
  //JVT-X046 }
  UInt    m_uiLastCodedFrameIdInGOP;
  UInt    m_uiLastCodedTemporalId;

  Bool    m_bInterlaced;
};

#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif

H264AVC_NAMESPACE_END

#endif // !defined(AFX_GOPENCODER_H__75F41F36_C28D_41F9_AB5E_4C90D66D160C__INCLUDED_)
