
#if !defined(AFX_H264AVCENCODER_H__FBF0345F_A5E5_4D18_8BEC_4A68790901F7__INCLUDED_)
#define AFX_H264AVCENCODER_H__FBF0345F_A5E5_4D18_8BEC_4A68790901F7__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#include "H264AVCEncoderLib.h"
#include "H264AVCCommonLib/TraceFile.h"
#include "GOPEncoder.h"

#ifdef SHARP_AVC_REWRITE_OUTPUT
#include "MbCoder.h"
#include "CreaterH264AVCEncoder.h"
#endif

H264AVC_NAMESPACE_BEGIN



class PocCalculator;
class CodingParameter;
class NalUnitEncoder;
class ControlMngIf;
class ParameterSetMng;
class LayerEncoder;
class MotionVectorCalculation;


#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif




class H264AVCENCODERLIB_API H264AVCEncoder
{
protected:
	H264AVCEncoder();
	virtual ~H264AVCEncoder();

public:
  static  ErrVal create ( H264AVCEncoder*&  rpcH264AVCEncoder );
  virtual ErrVal destroy();
  virtual ErrVal init   ( LayerEncoder*     apcLayerEncoder[MAX_LAYERS],
                          ParameterSetMng*  pcParameterSetMng,
                          PocCalculator*    pcPocCalculator,
                          NalUnitEncoder*   pcNalUnitEncoder,
                          ControlMngIf*     pcControlMng,
                          CodingParameter*  pcCodingParameter,
                          // JVT-V068 {
                          StatBuf<Scheduler*, MAX_SCALABLE_LAYERS>* apcScheduler
                          // JVT-V068 }
                        );
  virtual ErrVal uninit ();

  ErrVal writeParameterSets ( ExtBinDataAccessor*       pcExtBinDataAccessor,
                              SequenceParameterSet*&    rpcAVCSPS,
                              Bool&                     rbMoreSets );
  ErrVal process            ( ExtBinDataAccessorList&   rcExtBinDataAccessorList,
                              PicBuffer*                apcOriginalPicBuffer   [MAX_LAYERS],
                              PicBuffer*                apcReconstructPicBuffer[MAX_LAYERS],
                              PicBufferList*            apcPicBufferOutputList,
                              PicBufferList*            apcPicBufferUnusedList );
  ErrVal finish             ( ExtBinDataAccessorList&   rcExtBinDataAccessorList,
                              PicBufferList*            apcPicBufferOutputList,
                              PicBufferList*            apcPicBufferUnusedList,
                              UInt&                     ruiNumCodedFrames,
                              Double&                   rdHighestLayerOutputRate );

  UInt    getPicCodingType    ( UInt          uiBaseLayerId,
                                UInt          uiTemporalId,
                                UInt          uiFrameIdInTLayer );
  UInt    getMaxSliceSize     ( UInt          uiLayerId,
                                UInt          uiAUIndex );
  ErrVal  getBaseLayerStatus  ( UInt&         ruiBaseLayerId,
																UInt          uiLayerId,
																PicType       ePicType,
																UInt					uiTemporalId );
  Bool    hasMGSEnhancementLayer( UInt        uiLayerId,
                                  UInt&       ruiMaxLevelIdc );
  ErrVal getBaseLayerLevelIdc ( UInt          uiBaseLayerId,
                                UInt&         uiLevelIdc,
                                Bool&         bBiPred8x8Disable,
                                Bool&         bMCBlks8x8Disable );

  ErrVal  getBaseLayerDataAvlb( Frame*&       pcFrame,
																Frame*&       pcResidual,
																MbDataCtrl*&  pcMbDataCtrl,
																UInt          uiBaseLayerId,
																Bool&         bBaseDataAvailable,
																PicType       ePicType,
																UInt					uiTemporalId );

	ErrVal  getBaseLayerData    ( SliceHeader&  rcELSH,
                                Frame*&       pcFrame,
																Frame*&       pcResidual,
																MbDataCtrl*&  pcMbDataCtrl,
																Bool          bSpatialScalability,
																UInt          uiBaseLayerId,
																PicType       ePicType,
																UInt					uiTemporalId );

	ErrVal getBaseLayerResidual( Frame*&      pcResidual, UInt            uiBaseLayerId);

	UInt    getNewBits          ( UInt          uiBaseLayerId );

  Frame* getLowPassRec     ( UInt uiLayerId, UInt uiLowPassIndex );
  Frame* getELRefPic       ( UInt uiLayerId, UInt uiTemporalId, UInt uiFrameIdInTId );

  Void setScalableSEIMessage  ()       { m_bScalableSeiMessage = true; }
// JVT-V068 HRD {
  Void setBufferPeriodSEIMessage()     { m_bWriteBufferingPeriodSEI = true; }
  ErrVal writeAVCCompatibleHRDSEI( ExtBinDataAccessor* pcExtBinDataAccessor, SequenceParameterSet& rcSPS );
// JVT-V068 HRD }
	Bool bGetScalableSeiMessage	() const { return m_bScalableSeiMessage; }
	Void SetVeryFirstCall				()			 { m_bVeryFirstCall = true; }

	UInt   getScalableLayerId( UInt uiLayer, UInt uiTempLevel, UInt uiFGS ) const { return m_aaauiScalableLayerId[uiLayer][uiTempLevel][uiFGS]; }
  Void   setBitrateRep( UInt uiLayer, UInt uiTL, UInt uiQL, Double dVal ) { m_aaadLayerBitrateRep[uiLayer][uiTL][uiQL] = dVal; }
  Double getBitrateRep( UInt uiLayer, UInt uiTL, UInt uiQL ) const { return m_aaadLayerBitrateRep[uiLayer][uiTL][uiQL]; }

	//JVT-W052
	CodingParameter*  getCodingParameter()  { return m_pcCodingParameter;}
	SEI::IntegrityCheckSEI * getIntegrityCheckSEI() {return m_pcIntegrityCheckSEI; }
	//JVT-W052

// JVT-S080 LMI {
  ErrVal xWriteScalableSEILayersNotPresent( ExtBinDataAccessor* pcExtBinDataAccessor, UInt uiInputLayers, UInt* m_layer_id);
  ErrVal xWriteScalableSEIDependencyChange( ExtBinDataAccessor* pcExtBinDataAccessor, UInt uiNumLayers, UInt* uiLayerId, Bool* pbLayerDependencyInfoPresentFlag,
												  UInt* uiNumDirectDependentLayers, UInt** puiDirectDependentLayerIdDeltaMinus1, UInt* puiLayerDependencyInfoSrcLayerIdDeltaMinus1);
// JVT-S080 LMI }

  // JVT-AD021 {
  ErrVal getBaseLayerQpPredData ( UInt uiBaseLayerId , Double & dQpPredData , Int & uiPOC , UInt & uiFrameSizeInMB );
  // JVT-AD021 }

protected:
  ErrVal xInitParameterSets ();
// JVT-V068 HRD {
	ErrVal xInitLayerInfoForHrd(SequenceParameterSet* pcSPS, UInt uiLayer );
// JVT-V068 HRD }
  ErrVal xWriteScalableSEI  ( ExtBinDataAccessor*       pcExtBinDataAccessor );
	ErrVal xWriteSubPicSEI		( ExtBinDataAccessor*				pcExtBinDataAccessor );
	ErrVal xWriteSubPicSEI( ExtBinDataAccessor* pcExtBinDataAccessor, UInt layer_id ) ;
	ErrVal xWriteMotionSEI( ExtBinDataAccessor* pcExtBinDataAccessor, UInt sg_id ) ;
// JVT-V068 HRD {
  ErrVal xWriteBufferingPeriodSEI ( ExtBinDataAccessor*  pcExtBinDataAccessor );
// JVT-V068 HRD }

  ErrVal xProcessGOP        ( PicBufferList*            apcPicBufferOutputList,
                              PicBufferList*            apcPicBufferUnusedList );

protected:
  std::list<SequenceParameterSet*>  m_cUnWrittenSPS;
  std::list<PictureParameterSet*>   m_cUnWrittenPPS;
  PicBufferList                     m_acOrgPicBufferList[MAX_LAYERS];
  PicBufferList                     m_acRecPicBufferList[MAX_LAYERS];
  ParameterSetMng*                  m_pcParameterSetMng;
  PocCalculator*                    m_pcPocCalculator;
  NalUnitEncoder*                   m_pcNalUnitEncoder;
  ControlMngIf*                     m_pcControlMng;
  CodingParameter*                  m_pcCodingParameter;
  Bool                              m_bVeryFirstCall;
  Bool                              m_bInitDone;
  Bool                              m_bTraceEnable;

	Bool															m_bScalableSeiMessage;
public:
	Double														m_aaadFinalFramerate[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  Double                            m_aaadSeqBits[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
protected:
  LayerEncoder*                     m_apcLayerEncoder   [MAX_LAYERS];
  AccessUnitDataList                m_cAccessUnitDataList;

  // ICU / ETRI ROI
  Bool    m_bWrteROISEI;
// JVT-V068 HRD {
  Bool    m_bWriteBufferingPeriodSEI;
  StatBuf<Scheduler*, MAX_SCALABLE_LAYERS>* m_apcScheduler;
// JVT-V068 HRD }
  UInt    m_loop_roi_sei;
	//JVT-W051 {
	UInt		m_uiProfileIdc[MAX_LAYERS];
	UInt		m_uiLevelIdc[MAX_LAYERS];
	Bool		m_bConstraint0Flag[MAX_LAYERS];
	Bool		m_bConstraint1Flag[MAX_LAYERS];
	Bool		m_bConstraint2Flag[MAX_LAYERS];
	Bool		m_bConstraint3Flag[MAX_LAYERS];
	Bool		m_bIsFirstGOP;
	//JVT-W051 }

  Double m_aaadLayerBitrateRep [MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
	UInt   m_aaauiScalableLayerId[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];

	//JVT-W052
public:
	UInt    m_uicrcVal[MAX_LAYERS];
	UInt    m_uiNumofCGS[MAX_LAYERS];
	SEI::IntegrityCheckSEI * m_pcIntegrityCheckSEI;
	//JVT-W052
};



#ifdef SHARP_AVC_REWRITE_OUTPUT

class H264AVCENCODERLIB_API RewriteEncoder
{
protected:
  RewriteEncoder();
  virtual ~RewriteEncoder();

public:
  static ErrVal   create            ( RewriteEncoder*&            rpcRewriteEncoder );
  ErrVal          destroy           ();
  ErrVal          init              ();
  ErrVal          uninit            ();

  ErrVal          startPicture      ( const SequenceParameterSet& rcSPS );
  ErrVal          finishPicture     ( BinDataList&                rcBinDataList );
  ErrVal          rewriteMb         ( MbDataAccess&               rcMbDataAccessSource,
                                      Bool                        bSendEOS );

private:
  ErrVal          xCreate           ();

  ErrVal          xStartSlice       ( MbDataAccess&               rcMbDataAccessSource );
  ErrVal          xFinishSlice      ();
  ErrVal          xInitNALUnit      ();
  ErrVal          xCloseNALUnit     ();
  Bool            xIsRewritten      ( const Void*                 pParameterSet );
  ErrVal          xRewriteSPS       ( const SequenceParameterSet& rcSPS );
  ErrVal          xRewritePPS       ( const PictureParameterSet&  rcPPS );

  ErrVal          xInitMb           ( MbDataAccess*&              rpcMbDataAccessRewrite,
                                      MbDataAccess&               rcMbDataAccessSource );
  ErrVal          xAdjustMb         ( MbDataAccess&               rcMbDataAccessRewrite,
                                      Bool                        bBaseLayer );
  ErrVal          xEncodeMb         ( MbDataAccess&               rcMbDataAccessRewrite,
                                      Bool                        bLastMbInSlice,
                                      Bool                        bSendEOS );

private:
  Bool                      m_bInitialized;
  Bool                      m_bPictureInProgress;
  Bool                      m_bSliceInProgress;
  UInt                      m_uiBinDataSize;
  BitWriteBuffer*           m_pcBitWriteBuffer;
  BitCounter*               m_pcBitCounter;
  NalUnitEncoder*           m_pcNalUnitEncoder;
  UvlcWriter*               m_pcUvlcWriter;
  UvlcWriter*               m_pcUvlcTester;
  CabacWriter*              m_pcCabacWriter;
  MotionVectorCalculation*  m_pcMotionVectorCalculation;
  MbCoder*                  m_pcMbCoder;
  RateDistortion*           m_pcRateDistortion;
  MbDataCtrl*               m_pcMbDataCtrl;
  MyList<const Void*>       m_cRewrittenParameterSets;

  BinData*                  m_pcBinData;
  BinDataAccessor*          m_pcBinDataAccessor;
  BinDataList               m_cBinDataList;
  SliceHeader*              m_pcSliceHeader;
  MbSymbolWriteIf*          m_pcMbSymbolWriteIf;

  Bool                      m_bTraceEnable;
};

#endif


#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif


H264AVC_NAMESPACE_END


#endif // !defined(AFX_H264AVCENCODER_H__FBF0345F_A5E5_4D18_8BEC_4A68790901F7__INCLUDED_)
