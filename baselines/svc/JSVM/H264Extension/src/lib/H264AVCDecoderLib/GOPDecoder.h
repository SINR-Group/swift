
#if !defined(AFX_GOPDECODER_H__339878FC_BA98_4ABE_8530_E1676196576F__INCLUDED_)
#define AFX_GOPDECODER_H__339878FC_BA98_4ABE_8530_E1676196576F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "H264AVCCommonLib/MbDataCtrl.h"
#include "H264AVCCommonLib/Frame.h"
#include "DownConvert.h"
#ifdef SHARP_AVC_REWRITE_OUTPUT
#include "../H264AVCEncoderLib/H264AVCEncoder.h"
#endif

// TMM_ESS
#include "ResizeParameters.h"


H264AVC_NAMESPACE_BEGIN


class H264AVCDecoder;
class SliceReader;
class SliceDecoder;
class PocCalculator;
class LoopFilter;
class HeaderSymbolReadIf;
class ParameterSetMng;
class NalUnitParser;
class ControlMngIf;
class MotionCompensation;
class Frame;
class ReconstructionBypass;
class YuvBufferCtrl;
class SliceDataNALUnit;



class H264AVCDECODERLIB_API DPBUnit
{
  friend class CurrDPBUnit;

protected:
  DPBUnit         ( YuvBufferCtrl& rcYuvBufferCtrl );
  virtual ~DPBUnit();

public:
  static ErrVal     create            ( DPBUnit*&                   rpcDPBUnit,
                                        YuvBufferCtrl&              rcYuvBufferCtrl,
                                        const SequenceParameterSet& rcSPS,
                                        Bool                        bBaseLayer );
  ErrVal            destroy           ();
  ErrVal            uninit            ();
  ErrVal            dump              ( UInt                        uiNumber,
                                        Bool                        bLineBefore,
                                        Bool                        bLineAfter );

  Frame*            getFrame          ()                                    { return  m_pcFrame; }
  MbDataCtrl*       getMbDataCtrlBase ()                                    { return  m_pcMbDataCtrlBaseLayer; }

  const Frame*      getFrame          ()                            const   { return  m_pcFrame; }
  const MbDataCtrl* getMbDataCtrlBase ()                            const   { return  m_pcMbDataCtrlBaseLayer; }
  PicType           getPicStatus      ()                            const   { return  m_ePicStatus; }
  UInt              getFrameNum       ()                            const   { return  m_uiFrameNum; }
  Int               getLongTermIndex  ()                            const   { return  m_iLongTermFrameIdx; }
  Bool              isExisting        ()                            const   { return  m_bExisting; }
  Bool              isRefBasePicUnit  ()                            const   { return  m_bBaseRepresentation; }
  Bool              isWaitingForOutput()                            const   { return  m_bWaitForOutput; }
  Bool              isRefPicUnit      ()                            const   { return  m_bRefPic; }
  Bool              isRefFrame        ()                            const   { return  m_abNeededForReference[0] && m_abNeededForReference[1]; }

  Bool              isRequired        ()                            const;
  Bool              isUsedForRef      ()                            const;
  Bool              isShortTermUnit   ()                            const;
  Bool              isLongTermUnit    ()                            const;
  Int               getMaxPoc         ( Bool    bPocMode0 )         const;
  Int               getPoc            ()                            const;
  Int               getFrameNumWrap   ( UInt    uiCurrFrameNum,
                                        UInt    uiMaxFrameNum   )   const;
  Bool              isShortTermRef    ( PicType ePicType )          const;
  Bool              isLongTermRef     ( PicType ePicType )          const;
  Bool              isRefPic          ( PicType ePicType )          const;

  ErrVal            setNonExisting    ( UInt                        uiFrameNum,
                                        Int                         iTopFieldPoc,
                                        Int                         iBotFieldPoc );
  ErrVal            output            ( PicBufferList&              rcPicBufferInputList,
                                        PicBufferList&              rcPicBufferOutputList,
                                        PicBufferList&              rcPicBufferUnusedList );
  ErrVal            markUnusedForRef  ( PicType                     ePicType,
                                        Bool                        bRemoveOutputFlag = false );
  ErrVal            markLongTerm      ( PicType                     ePicType,
                                        Int                         iLongTermFrameIdx );
  ErrVal            decreasePoc       ( Int                         iMMCO5Poc );
  ErrVal            checkStatus       ( Int                         iMaxLongTermFrameIdx );

protected:
  ErrVal            xCreateData       ( const SequenceParameterSet& rcSPS,
                                        Bool                        bBaseLayer );
  ErrVal            xDeleteData       ();

protected:
  YuvBufferCtrl&    m_rcYuvBufferCtrl;
  PicType           m_ePicStatus;
  UInt              m_uiFrameNum;
  Int               m_iLongTermFrameIdx;
  Bool              m_bExisting;
  Bool              m_bBaseRepresentation;
  Bool              m_bWaitForOutput;
  Bool              m_bRefPic;
  Int               m_aiPoc                 [2];
  Bool              m_abUseBasePred         [2];
  Bool              m_abNeededForReference  [2];
  Bool              m_abLongTerm            [2];
  Frame*            m_pcFrame;
  MbDataCtrl*       m_pcMbDataCtrlBaseLayer;
};



class H264AVCDECODERLIB_API CurrDPBUnit : public DPBUnit
{
protected:
  CurrDPBUnit         ( YuvBufferCtrl& rcYuvBufferCtrl );
  virtual ~CurrDPBUnit();

public:
  static  ErrVal      create          ( CurrDPBUnit*&               rpcCurrDPBUnit,
                                        YuvBufferCtrl&              rcYuvBufferCtrl,
                                        const SequenceParameterSet& rcSPS,
                                        Bool                        bCreateRefBasePicBuffer,
                                        Bool                        bBaseLayer );
  virtual ErrVal      destroy         ();
  ErrVal              init            ( SliceHeader&                rcSliceHeader );
  ErrVal              reinit          ( SliceHeader&                rcSliceHeader,
                                        Bool                        bNewLayerRepresentation );
  ErrVal              resetMMCO5      ( SliceHeader&                rcSliceHeader );
  ErrVal              setComplete     ( CurrDPBUnit&                rcILPredUnit,
                                        Bool                        bDependencyRepresentationFinished );
  ErrVal              store           ( DPBUnit&                    rcDPBUnit,
                                        Bool                        bRefBasePic = false );
  ErrVal              uninit          ();

  Frame*              getRefBasePic   ()          { return  m_pcRefBasePicFrame; }
  ControlData&        getCtrlData     ()          { return *m_pcControlData; }
  SliceHeader*        getSliceHeader  ()          { return  m_pcControlData->getSliceHeader (); }
  MbDataCtrl*         getMbDataCtrl   ()          { return  m_pcControlData->getMbDataCtrl  (); }

  const Frame*        getRefBasePic   ()  const   { return  m_pcRefBasePicFrame; }
  const ControlData&  getCtrlData     ()  const   { return *m_pcControlData; }
  const SliceHeader*  getSliceHeader  ()  const   { return  m_pcControlData->getSliceHeader (); }
  const MbDataCtrl*   getMbDataCtrl   ()  const   { return  m_pcControlData->getMbDataCtrl  (); }
  Bool                inCurrentUse    ()  const   { return  m_bInUse && !m_bCompleted; }
  Bool                isCompleted     ()  const   { return  m_bInUse &&  m_bCompleted; }
  Bool                isUninitialized ()  const   { return !m_bInUse && !m_bCompleted; }

protected:
  ErrVal              xCreateData     ( const SequenceParameterSet& rcSPS,
                                        Bool                        bCreateRefBasePicBuffer,
                                        Bool                        bBaseLayer );
  ErrVal              xDeleteData     ();
  ErrVal              xStoreFrame     ( DPBUnit&                    rcDPBUnit,
                                        Bool                        bRefBasePic );
  ErrVal              xStore2ndField  ( DPBUnit&                    rcDPBUnit,
                                        Bool                        bRefBasePic );

private:
  Bool                m_bInUse;
  Bool                m_bRefBasePicInUse;
  Bool                m_bCompleted;
  UInt                m_uiQualityId;
  ControlData*        m_pcControlData;
  Frame*              m_pcRefBasePicFrame;
};


class H264AVCDECODERLIB_API RefPicEntry
{
public:
  RefPicEntry ( PicType ePicType, DPBUnit* pcDPBUnit );
  RefPicEntry ( const RefPicEntry& rcRefPicEntry );
  ~RefPicEntry();
  PicType   getPicType()  { return m_ePicType; }
  DPBUnit*  getDPBUnit()  { return m_pcDPBUnit; }
private:
  PicType   m_ePicType;
  DPBUnit*  m_pcDPBUnit;
};


typedef MyList<DPBUnit*>    DPBUnitList;
typedef MyList<RefPicEntry> RefPicEntryList;


class H264AVCDECODERLIB_API DecodedPicBuffer
{
protected:
  DecodedPicBuffer          ();
  virtual ~DecodedPicBuffer ();

public:
  static  ErrVal      create                    ( DecodedPicBuffer*&          rpcDecodedPicBuffer );
  virtual ErrVal      destroy                   ();
  ErrVal              init                      ( YuvBufferCtrl*              pcYuvBufferCtrl,
                                                  UInt                        uiDependencyId );
  ErrVal              uninit                    ();

  ErrVal              initCurrDPBUnit           ( CurrDPBUnit*&               rpcCurrDPBUnit,
                                                  PicBuffer*&                 rpcPicBuffer,
                                                  SliceHeader&                rcSliceHeader,
                                                  PocCalculator&              rcPocCalculator,
                                                  PicBufferList&              rcOutputList,
                                                  PicBufferList&              rcUnusedList,
                                                  Bool                        bFirstSliceInDependencyRepresentation,
                                                  Bool                        bFirstSliceInLayerRepresentation );
  ErrVal              storeCurrDPBUnit          ( Bool                        bDependencyRepresentationFinished );
  ErrVal              updateBuffer              ( PocCalculator&              rcPocCalculator,
                                                  PicBufferList&              rcOutputList,
                                                  PicBufferList&              rcUnusedList );
  ErrVal              finish                    ( PicBufferList&              rcOutputList,
                                                  PicBufferList&              rcUnusedList );

  ErrVal              setPrdRefLists            ( CurrDPBUnit*                pcCurrDPBUnit );
  CurrDPBUnit*        getILPredDPBUnit          ();

protected:
  //===== data creation/deletion =====
  Bool                xNewBufferDimension       ( const SequenceParameterSet& rcSPS );
  ErrVal              xInitBuffer               ( const SliceHeader&          rcSliceHeader );
  ErrVal              xCreateData               ( const SequenceParameterSet& rcSPS,
                                                  UInt                        uiDPBSizeInFrames,
                                                  Bool                        bBaseLayer );
  ErrVal              xDeleteData               ();
  //===== check for complementary field pair =====
  Bool                xIs2ndFieldOfCompFieldPair( const SliceHeader&          rcSliceHeader );
  Bool                xIs2ndFieldOfCompFieldPair( Bool                        bRefBasePic = false );
  //===== insert pictures =====
  ErrVal              xInsertNonExistingFrame   ( const SliceHeader*          pcSliceHeader,
                                                  UInt                        uiFrameNum );
  ErrVal              xInsertCurrentInNewBuffer ( DPBUnit*&                   rpcStoredDPBUnit,
                                                  Bool                        bRefBasePic = false );
  //===== update and check buffer status ======
  ErrVal              xCheckGapsInFrameNum      ( const SliceHeader&          rcSliceHeader,
                                                  PocCalculator&              rcPocCalculator,
                                                  PicBufferList&              rcOutputList,
                                                  PicBufferList&              rcUnusedList );
  ErrVal              xUpdateAndStoreCurrentPic ( PocCalculator&              rcPocCalculator,
                                                  PicBufferList&              rcOutputList,
                                                  PicBufferList&              rcUnusedList );
  ErrVal              xCheckBufferStatus        ();
  //===== output =====
  ErrVal              xBumpingOutput            ( PicBufferList&              rcOutputList,
                                                  PicBufferList&              rcUnusedList,
                                                  Bool                        bOutputAll = false );
  //===== memory management =====
  ErrVal              xMarkAllUnusedForRef      ( Bool                        bRemoveOutputFlag = false,
                                                  PicBufferList*              pcUnusedList      = 0 );
  ErrVal              xSlidingWindow            ( UInt                        uiCurrFrameNum );
  ErrVal              xMMCO                     ( PocCalculator&              rcPocCalculator,
                                                  SliceHeader&                rcSliceHeader,
                                                  Bool&                       rbMMCO6,
                                                  Bool                        bRefBasePic = false );
  ErrVal              xMarkShortTermUnused      ( UInt                        uiCurrFrameNum,               // MMCO 1
                                                  PicType                     eCurrPicType,
                                                  UInt                        uiPicNumDiff,
                                                  Bool                        bRefBasePic );
  ErrVal              xMarkLongTermUnused       ( PicType                     eCurrPicType,                 // MMCO 2
                                                  UInt                        uiLongTermPicNum,
                                                  Bool                        bRefBasePic );
  ErrVal              xAssignLongTermIndex      ( UInt                        uiCurrFrameNum,               // MMCO 3
                                                  PicType                     eCurrPicType,
                                                  UInt                        uiPicNumDiff,
                                                  UInt                        uiLongTermFrameIndex );
  ErrVal              xSetMaxLongTermIndex      ( UInt                        uiMaxLongTermFrameIdxPlus1 ); // MMCO 4
  ErrVal              xReset                    ( PocCalculator&              rcPocCalculator,              // MMCO 5
                                                  SliceHeader&                rcSliceHeader );
  ErrVal              xStoreCurrentLongTerm     ( UInt                        uiLongTermFrameIndex,         // MMCO 6
                                                  Bool                        bStoreRefBasePic );
  //===== reference list construction =====
  Bool                xExistsRefBasePicShortTerm( UInt                        uiFrameNum );
  Bool                xExistsRefBasePicLongTerm ( Int                         iLongTermFrameIdx );
  Bool                xIsAvailableForRefLists   ( const DPBUnit*              pcDPBUnit,
                                                  Bool                        bFieldPicture,
                                                  Bool                        bExcludeNonExisting,
                                                  Bool                        bUseRefBasePic );
  ErrVal              xCreateOrderedDPBUnitLists( DPBUnitList&                rcOrderedShortTermList0,
                                                  DPBUnitList&                rcOrderedShortTermList1,
                                                  DPBUnitList&                rcOrderedLongTermList,
                                                  Bool&                       rbShortTermListIdentical,
                                                  SliceHeader&                rcSliceHeader );
  DPBUnit*            xGetDPBEntry              ( DPBUnitList&                rcDPBUnitList,
                                                  UInt                        uiIndex );
  ErrVal              xGetRefPicEntryList       ( RefPicEntryList&            rcRefPicEntryList,
                                                  DPBUnitList&                rcDPBUnitList,
                                                  PicType                     eCurrPicType );
  ErrVal              xCreateInitialRefPicLists ( RefPicEntryList&            rcRefPicEntryList0,
                                                  RefPicEntryList&            rcRefPicEntryList1,
                                                  SliceHeader&                rcSliceHeader );
  ErrVal              xModifyRefPicList         ( RefPicEntryList&            rcRefPicEntryList,
                                                  UInt&                       ruiRefIdx,
                                                  Bool                        bLongTerm,
                                                  PicType                     ePicType,
                                                  UInt                        uiFrameNumOrLTIdx );
  ErrVal              xRefPicListModification   ( RefPicEntryList&            rcRefPicEntryList,
                                                  SliceHeader&                rcSliceHeader,
                                                  ListIdx                     eListIdx );
  ErrVal              xSetReferencePictureList  ( RefFrameList&               rcRefFrameList,
                                                  RefPicEntryList&            rcRefPicEntryList,
                                                  UInt                        uiNumActiveEntries );
  ErrVal              xSetMbDataCtrlEntry0      ( MbDataCtrl*&                rpcMbDataCtrl,
                                                  RefPicEntryList&            rcRefPicEntryList );
  //===== debugging =====
  ErrVal              xDumpDPB                  ( const Char*                 pcString = 0 );
  ErrVal              xDumpRefPicList           ( RefFrameList&               rcRefFrameList,
                                                  ListIdx                     eListIdx,
                                                  const Char*                 pcString = 0 );

private:
  Bool                m_bInitDone;
  Bool                m_bInitBufferDone;
  Bool                m_bDebugOutput;
  YuvBufferCtrl*      m_pcYuvBufferCtrl;
  UInt                m_uiDependencyId;

  UInt                m_uiFrameWidthInMbs;
  UInt                m_uiFrameHeightInMbs;
  UInt                m_uiBufferSizeInFrames;
  UInt                m_uiDPBSizeInFrames;
  UInt                m_uiMaxNumRefFrames;
  UInt                m_uiMaxFrameNum;
  UInt                m_uiLastRefFrameNum;
  Int                 m_iMaxLongTermFrameIdx;
  DPBUnit*            m_pcLastDPBUnit;
  DPBUnit*            m_pcLastDPBUnitRefBasePic;

  PicBufferList       m_cPicBufferList;
  DPBUnitList         m_cUsedDPBUnitList;
  DPBUnitList         m_cFreeDPBUnitList;
  CurrDPBUnit*        m_pcCurrDPBUnit;
  CurrDPBUnit*        m_pcCurrDPBUnitILPred;
};




class H264AVCDECODERLIB_API LayerDecoder
{
  enum { NUM_TMP_FRAMES = 2 };

protected:
	LayerDecoder         ();
	virtual ~LayerDecoder();

public:
  //===== general functions ======
  static  ErrVal  create        ( LayerDecoder*&          rpcLayerDecoder );
  ErrVal          destroy       ();
  ErrVal          init          ( UInt                    uiDependencyId,
                                  H264AVCDecoder*         pcH264AVCDecoder,
                                  NalUnitParser*          pcNalUnitParser,
                                  SliceReader*            pcSliceReader,
                                  SliceDecoder*           pcSliceDecoder,
                                  ControlMngIf*           pcControlMng,
                                  LoopFilter*             pcLoopFilter,
                                  HeaderSymbolReadIf*     pcHeaderSymbolReadIf,
                                  ParameterSetMng*        pcParameterSetMng,
                                  PocCalculator*          pcPocCalculator,
                                  YuvBufferCtrl*          pcYuvFullPelBufferCtrl,
                                  DecodedPicBuffer*       pcDecodedPictureBuffer,
                                  MotionCompensation*     pcMotionCompensation,
								                  ReconstructionBypass*   pcReconstructionBypass
#ifdef SHARP_AVC_REWRITE_OUTPUT
                                  ,RewriteEncoder*        pcRewriteEncoder
#endif
                                  );
  ErrVal          uninit        ();

  //===== main processing functions =====
  ErrVal  processSliceData      ( PicBuffer*              pcPicBuffer,
                                  PicBufferList&          rcPicBufferOutputList,
                                  PicBufferList&          rcPicBufferUnusedList,
                                  BinDataList&            rcBinDataList,
                                  SliceDataNALUnit&       rcSliceDataNALUnit );
  ErrVal  updateDPB             ( PicBufferList&          rcPicBufferOutputList,
                                  PicBufferList&          rcPicBufferUnusedList );
  ErrVal  finishProcess         ( PicBufferList&          rcPicBufferOutputList,
                                  PicBufferList&          rcPicBufferUnusedList );

  //===== returning data =====
  ErrVal            getBaseLayerData              ( SliceHeader&      rcELSH,
                                                    Frame*&           pcFrame,
                                                    Frame*&           pcResidual,
                                                    MbDataCtrl*&      pcMbDataCtrl,
                                                    ResizeParameters& rcResizeParameters );
  ErrVal            getBaseSliceHeader            ( SliceHeader*&     rpcSliceHeader );

  UInt              getFrameWidth                 ()  const { return m_uiFrameWidthInMb*16; }
  UInt              getFrameHeight                ()  const { return m_uiFrameHeightInMb*16; }

private:
  //===== create data arrays =====
  ErrVal  xCreateData                 ( const SequenceParameterSet&   rcSPS );
  ErrVal  xDeleteData                 ();

  //===== initialization =====
  ErrVal  xReadSliceHeader            ( SliceHeader*&           rpcSliceHeader,
                                        SliceDataNALUnit&       rcSliceDataNalUnit );
  ErrVal  xInitSliceHeader            ( SliceHeader&            rcSliceHeader );
  ErrVal  xInitSPS                    ( const SliceHeader&      rcSliceHeader );
  ErrVal  xInitDPBUnit                ( SliceHeader&            rcSliceHeader,
                                        PicBuffer*              pcPicBuffer,
                                        PicBufferList&          rcPicBufferOutputList,
                                        PicBufferList&          rcPicBufferUnusedList );

  //===== slice processing =====
  ErrVal  xInitSlice                  ( SliceHeader*&           rpcSliceHeader,
                                        PicBuffer*              pcPicBuffer,
                                        PicBufferList&          rcPicBufferOutputList,
                                        PicBufferList&          rcPicBufferUnusedList,
                                        SliceDataNALUnit&       rcSliceDataNalUnit );
  ErrVal  xParseSlice                 ( SliceHeader&            rcSliceHeader );
  ErrVal  xDecodeSlice                ( SliceHeader&            rcSliceHeader,
                                        const SliceDataNALUnit& rcSliceDataNalUnit,
                                        Bool                    bFirstSliceInLayerRepresentation );
  ErrVal  xFinishLayerRepresentation  ( SliceHeader&            rcSliceHeader,
                                        PicBufferList&          rcPicBufferOutputList,
                                        PicBufferList&          rcPicBufferUnusedList,
                                        const SliceDataNALUnit& rcSliceDataNalUnit,
                                        BinDataList&            rcBinDataList );
  ErrVal  xFinishSlice                ( SliceHeader&            rcSliceHeader,
                                        PicBufferList&          rcPicBufferOutputList,
                                        PicBufferList&          rcPicBufferUnusedList,
                                        const SliceDataNALUnit& rcSliceDataNalUnit,
                                        BinDataList&            rcBinDataList );

  //===== picture processing =====
  ErrVal  xCheckForMissingSlices      ( const SliceDataNALUnit& rcSliceDataNalUnit,
                                        PicBufferList&          rcPicBufferOutputList,
                                        PicBufferList&          rcPicBufferUnusedList );
  ErrVal  xSetLoopFilterQPs           ( SliceHeader&            rcSliceHeader,
                                        MbDataCtrl&             rcMbDataCtrl );
#ifdef SHARP_AVC_REWRITE_OUTPUT
  ErrVal  xRewritePicture             ( BinDataList&            rcBinDataList,
                                        MbDataCtrl&             rcMbDataCtrl );
#endif

  //===== base layer processing =====
  ErrVal  xInitESSandCroppingWindow   ( SliceHeader&            rcSliceHeader,
                                        MbDataCtrl&             rcMbDataCtrl,
                                        ControlData&            rcControlData );
  ErrVal  xInitBaseLayer              ( ControlData&            rcControlData,
                                        Bool                    bFirstSliceInLayerRepresentation );
  ErrVal  xGetBaseLayerData           ( ControlData&            rcControlData,
                                        Frame*&                 rpcBaseFrame,
                                        Frame*&                 rpcBaseResidual,
                                        MbDataCtrl*&            rpcBaseDataCtrl,
                                        ResizeParameters&       rcResizeParameters );
  Void    xSetMCResizeParameters      ( ResizeParameters&				rcResizeParameters );

protected:
  //----- references -----
  H264AVCDecoder*       m_pcH264AVCDecoder;
  NalUnitParser*        m_pcNalUnitParser;
  SliceReader*          m_pcSliceReader;
  SliceDecoder*         m_pcSliceDecoder;
  ControlMngIf*         m_pcControlMng;
  LoopFilter*           m_pcLoopFilter;
  HeaderSymbolReadIf*   m_pcHeaderSymbolReadIf;
  ParameterSetMng*      m_pcParameterSetMng;
  PocCalculator*        m_pcPocCalculator;
  YuvBufferCtrl*        m_pcYuvFullPelBufferCtrl;
  DecodedPicBuffer*     m_pcDecodedPictureBuffer;
  MotionCompensation*   m_pcMotionCompensation;
  ReconstructionBypass* m_pcReconstructionBypass;
#ifdef SHARP_AVC_REWRITE_OUTPUT
  RewriteEncoder*       m_pcRewriteEncoder;
#endif
  DownConvert           m_cDownConvert;

  //----- general parameters -----
  Bool                  m_bInitialized;
  Bool                  m_bSPSInitialized;
  Bool                  m_bDependencyRepresentationInitialized;
  Bool                  m_bLayerRepresentationInitialized;
  Bool                  m_bFirstILPredSliceInLayerRep;
  Bool                  m_bInterLayerPredLayerRep;
  Bool                  m_bRewritingLayerRep;
  UInt                  m_uiFrameWidthInMb;
  UInt                  m_uiFrameHeightInMb;
  UInt                  m_uiMbNumber;
  UInt                  m_uiDependencyId;
  UInt                  m_uiQualityId;

  //----- macroblock status and slice headers -----
  MbStatus*             m_pacMbStatus;
  MyList<SliceHeader*>  m_cSliceHeaderList;

  //----- frame memories, control data, and references  -----
  ResizeParameters      m_cResizeParameters;
  ResizeParameters      m_cResizeParametersAtQ0;
  CurrDPBUnit*          m_pcCurrDPBUnit;
  MbDataCtrl*           m_pcBaseLayerCtrl;
  MbDataCtrl*           m_pcBaseLayerCtrlField;
  Frame*                m_pcResidualLF;
  Frame*                m_pcResidualILPred;
  Frame*                m_pcILPrediction;
  Frame*                m_pcBaseLayerFrame;
  Frame*                m_pcBaseLayerResidual;
  Frame*                m_apcFrameTemp[NUM_TMP_FRAMES];
  Bool*                 m_apabBaseModeFlagAllowedArrays[2];
};

H264AVC_NAMESPACE_END


#endif // !defined(AFX_GOPDECODER_H__339878FC_BA98_4ABE_8530_E1676196576F__INCLUDED_)
