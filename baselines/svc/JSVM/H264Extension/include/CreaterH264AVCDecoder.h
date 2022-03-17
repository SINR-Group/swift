
#if !defined(AFX_CREATERH264AVCDECODER_H__0366BFA9_45D9_4834_B404_8DE3914C1E58__INCLUDED_)
#define AFX_CREATERH264AVCDECODER_H__0366BFA9_45D9_4834_B404_8DE3914C1E58__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "H264AVCCommonLib/Sei.h"

#define MAX_ROI_NUM  8

class H264AVCDecoder;
class ControlMngH264AVCDecoder;
class SliceReader;
class SliceDecoder;
class UvlcReader;
class MbParser;
class MbDecoder;
class NalUnitParser;
class BitReadBuffer;
class CabacReader;
class CabaDecoder;

class MbData;
class LoopFilter;
class Transform;
class IntraPrediction;
class MotionCompensation;
class YuvBufferCtrl;
class QuarterPelFilter;
class SampleWeighting;
class ParameterSetMng;
class PocCalculator;

class DecodedPicBuffer;
class LayerDecoder;
class ReconstructionBypass;
#ifdef SHARP_AVC_REWRITE_OUTPUT
class RewriteEncoder;
#endif


H264AVC_NAMESPACE_BEGIN


class H264AVCDECODERLIB_API NALUnit
{
protected:
  NALUnit() : m_pInstance( 0 ) {}
public:
  virtual ~NALUnit()  {}
  virtual Bool        isVCLNALUnit  () const = 0;
  virtual NalUnitType getNalUnitType() const = 0;
  Void*               getInstance   ()          { return m_pInstance; }
protected:
  Void                setInstance   ( Void* p ) { m_pInstance = p;    }
private:
  Void*   m_pInstance;
};


class H264AVCDECODERLIB_API NonVCLNALUnit : public NALUnit
{
public:
  NonVCLNALUnit( BinData* pcBinData, Bool bSEI, Bool bScalableSEI, Bool bBufferingPeriod );
  virtual ~NonVCLNALUnit();

  Void        destroyNALOnly    ();

  Bool        isVCLNALUnit      ()  const { return false; }
  BinData*    getBinData        ()        { return m_pcBinData; }
  NalUnitType getNalUnitType    ()  const { return m_eNalUnitType; }
  Bool        isSEI             ()  const { return m_bIsSEI; }
  Bool        isScalableSEI     ()  const { return m_bScalableSEI; }
  Bool        isBufferingPeriod ()  const { return m_bBufferingPeriod; }

private:
  BinData*    m_pcBinData;
  NalUnitType m_eNalUnitType;
  Bool        m_bIsSEI;
  Bool        m_bScalableSEI;
  Bool        m_bBufferingPeriod;
};


class H264AVCDECODERLIB_API SliceDataNALUnit : public NALUnit
{
public:
  SliceDataNALUnit( BinData* pcBinData,       const SliceHeader&  rcSliceHeader  );
  SliceDataNALUnit( BinData* pcBinDataPrefix, const PrefixHeader& rcPrefixHeader,
                    BinData* pcBinData,       const SliceHeader&  rcSliceHeader  );
  virtual ~SliceDataNALUnit();

  Bool          isVCLNALUnit                ()  const { return true; }
  BinData*      getBinDataPrefix            ()        { return m_pcBinDataPrefix; }
  BinData*      getBinData                  ()        { return m_pcBinData; }
  NalRefIdc     getNalRefIdc                ()  const { return m_eNalRefIdc; }
  NalUnitType   getNalUnitType              ()  const { return m_eNalUnitType; }
  Bool          getIdrFlag                  ()  const { return m_bIdrFlag; }
  UInt          getPriorityId               ()  const { return m_uiPriorityId; }
  Bool          getNoInterLayerPredFlag     ()  const { return m_bNoInterLayerPredFlag; }
  UInt          getDependencyId             ()  const { return m_uiDependencyId; }
  UInt          getQualityId                ()  const { return m_uiQualityId; }
  UInt          getDQId                     ()  const { return m_uiQualityId + ( m_uiDependencyId << 4 ); }
  UInt          getTemporalId               ()  const { return m_uiTemporalId; }
  Bool          getUseRefBasePicFlag        ()  const { return m_bUseRefBasePicFlag; }
  Bool          getDiscardableFlag          ()  const { return m_bDiscardableFlag; }
  Bool          getOutputFlag               ()  const { return m_bOutputFlag; }
  Bool          getTCoeffLevelPredictionFlag()  const { return m_bTCoeffLevelPredictionFlag; }
  UInt          getPPSId                    ()  const { return m_uiPPSId; }
  UInt          getSPSId                    ()  const { return m_uiSPSId; }
  UInt          getFrameNum                 ()  const { return m_uiFrameNum; }
  UInt          getRedundantPicCnt          ()  const { return m_uiRedundantPicCnt; }
  UInt          getRefLayerDQId             ()  const { return m_uiRefLayerDQId; }
  UInt          getFrameWidthInMb           ()  const { return m_uiFrameWidthInMb; }
  UInt          getFrameHeightInMb          ()  const { return m_uiFrameHeightInMb; }
  const UInt*   getCroppingRectangle        ()  const { return m_auiCroppingRectangle; }
  UInt          getAllocFrameWidthInMbs     ()  const { return m_uiAllocFrameWidthInMbs; }
  UInt          getAllocFrameHeightInMbs    ()  const { return m_uiAllocFrameHeightInMbs; }

  Bool  isDQIdMax                             ()  const { return m_bIsDQIdMax; }
  Bool  isDependencyIdMax                     ()  const { return m_bIsDependencyIdMax; }
  Bool  isLastSliceInLayerRepresentation      ()  const { return m_bLastSliceInLayerRepresentation; }
  Bool  isLastSliceInDependencyRepresentation ()  const { return m_bLastSliceInDependencyRepresentation; }
  Bool  isLastSliceInAccessUnit               ()  const { return m_bLastSliceInAccessUnit; }
  Bool  isLastAccessUnitInStream              ()  const { return m_bLastAccessUnitInStream; }
  Bool  isPartOfIDRAccessUnit                 ()  const { return m_bPartOfIDRAccessUnit; }
  Bool  isHighestRewriteLayer                 ()  const { return m_bHighestRewriteLayer; }

  Void  setAllocFrameWidthInMbs               ( UInt uiMbX )  { m_uiAllocFrameWidthInMbs  = uiMbX; }
  Void  setAllocFrameHeightInMbs              ( UInt uiMbY )  { m_uiAllocFrameHeightInMbs = uiMbY; }

  Void  setDQIdMax                            ( Bool  bIsDQIdMax                            ) { m_bIsDQIdMax                            = bIsDQIdMax; }
  Void  setDependencyIdMax                    ( Bool  bIsDependencyIdMax                    ) { m_bIsDependencyIdMax                    = bIsDependencyIdMax; }
  Void  setLastSliceInLayerRepresentation     ( Bool  bLastSliceInLayerRepresentation       ) { m_bLastSliceInLayerRepresentation       = bLastSliceInLayerRepresentation; }
  Void  setLastSliceInDependencyRepresentation( Bool  bLastSliceInDependencyRepresentation  ) { m_bLastSliceInDependencyRepresentation  = bLastSliceInDependencyRepresentation; }
  Void  setLastSliceInAccessUnit              ( Bool  bLastSliceInAccessUnit                ) { m_bLastSliceInAccessUnit                = bLastSliceInAccessUnit; }
  Void  setLastAccessUnitInStream             ( Bool  bLastAccessUnitInStream               ) { m_bLastAccessUnitInStream               = bLastAccessUnitInStream; }
  Void  setPartOfIDRAccessUnit                ( Bool  bPartOfIDRAccessUnit                  ) { m_bPartOfIDRAccessUnit                  = bPartOfIDRAccessUnit; }
  Void  setHighestRewriteLayer                ( Bool  bHighestRewriteLayer                  ) { m_bHighestRewriteLayer                  = bHighestRewriteLayer; }

private:
  //===== slice based parameters =====
  BinData*    m_pcBinDataPrefix;
  BinData*    m_pcBinData;
  NalRefIdc   m_eNalRefIdc;
  NalUnitType m_eNalUnitType;
  Bool        m_bIdrFlag;
  UInt        m_uiPriorityId;
  Bool        m_bNoInterLayerPredFlag;
  UInt        m_uiDependencyId;
  UInt        m_uiQualityId;
  UInt        m_uiTemporalId;
  Bool        m_bUseRefBasePicFlag;
  Bool        m_bDiscardableFlag;
  Bool        m_bOutputFlag;
  Bool        m_bTCoeffLevelPredictionFlag;
  UInt        m_uiPPSId;
  UInt        m_uiSPSId;
  UInt        m_uiFrameNum;
  UInt        m_uiRedundantPicCnt;
  UInt        m_uiRefLayerDQId;
  UInt        m_uiFrameWidthInMb;
  UInt        m_uiFrameHeightInMb;
  UInt        m_auiCroppingRectangle[4];
  //===== access unit based parameters =====
  Bool        m_bIsDQIdMax;
  Bool        m_bIsDependencyIdMax;
  Bool        m_bLastSliceInLayerRepresentation;
  Bool        m_bLastSliceInDependencyRepresentation;
  Bool        m_bLastSliceInAccessUnit;
  Bool        m_bLastAccessUnitInStream;
  Bool        m_bPartOfIDRAccessUnit;
  Bool        m_bHighestRewriteLayer;
  //===== for determination of buffer sizes =====
  UInt        m_uiAllocFrameWidthInMbs;
  UInt        m_uiAllocFrameHeightInMbs;
};


class H264AVCDECODERLIB_API AccessUnit
{
public:
  AccessUnit();
  virtual ~AccessUnit();

  ErrVal  update( BinData* pcBinData, PrefixHeader& rcPrefixHeader );
  ErrVal  update( BinData* pcBinData, SliceHeader&  rcSliceHeader );
  ErrVal  update( BinData* pcBinData, Bool bSEI, Bool bScalableSEI, Bool bBufferingPeriod );

  ErrVal  getAndRemoveNextNalUnit( NALUnit*& rpcNalUnit );

  Bool                    isEndOfStream       ()  const   { return m_bEndOfStream; }
  Bool                    isComplete          ()  const   { return m_bComplete; }
  Bool                    isEmpty             ()  const   { return m_cNalUnitList.empty(); }
  const SliceDataNALUnit* getLastVCLNalUnit   ()  const   { return m_pcLastVCLNALUnit; }
  const PrefixHeader*     getLastPrefixHeader ()  const   { return m_pcLastPrefixHeader; }
  const SliceHeader*      getLastSliceHeader  ()  const   { return m_pcLastSliceHeader; }

private:
  Void  xSetComplete      ( SliceDataNALUnit* pcFirstSliceDataNALUnitOfNextAccessUnit = 0,
                            SliceHeader*      pcFirstSliceHeaderOfNextAccessUnit      = 0 );
  Void  xSetParameters    ();
  Void  xRemoveRedundant  ();
  Void  xReInit           ();

private:
  Bool              m_bEndOfStream;
  Bool              m_bComplete;
  MyList<NALUnit*>  m_cNalUnitList;
  MyList<NALUnit*>  m_cStartOfNewAccessUnit;
  SliceDataNALUnit* m_pcLastVCLNALUnit;
  PrefixHeader*     m_pcLastPrefixHeader;
  SliceHeader*      m_pcLastSliceHeader;
};


class H264AVCDECODERLIB_API CreaterH264AVCDecoder
{
protected:
  CreaterH264AVCDecoder();
  virtual ~CreaterH264AVCDecoder();

public:
  static ErrVal create  ( CreaterH264AVCDecoder*& rpcCreaterH264AVCDecoder );
  ErrVal        destroy ();
  ErrVal        init    ( Bool bOpenTrace  );
  ErrVal        uninit  ( Bool bCloseTrace );

  ErrVal  initNALUnit     ( BinData*&         rpcBinData,
                            AccessUnit&       rcAccessUnit );
  ErrVal  processNALUnit  ( PicBuffer*        pcPicBuffer,
                            PicBufferList&    rcPicBufferOutputList,
                            PicBufferList&    rcPicBufferUnusedList,
                            BinDataList&      rcBinDataList,
                            NALUnit&          rcNALUnit );

protected:
  ErrVal  xCreateDecoder  ();

protected:
  H264AVCDecoder*           m_pcH264AVCDecoder;
  DecodedPicBuffer*         m_apcDecodedPicBuffer     [MAX_LAYERS];
  LayerDecoder*             m_apcLayerDecoder         [MAX_LAYERS];
  ParameterSetMng*          m_pcParameterSetMngAUInit;
  ParameterSetMng*          m_pcParameterSetMngDecode;
  PocCalculator*            m_apcPocCalculator        [MAX_LAYERS];
  SliceReader*              m_pcSliceReader;
  NalUnitParser*            m_pcNalUnitParser;
  SliceDecoder*             m_pcSliceDecoder;
  ControlMngH264AVCDecoder* m_pcControlMng;
  BitReadBuffer*            m_pcBitReadBuffer;
  UvlcReader*               m_pcUvlcReader;
  MbParser*                 m_pcMbParser;
  LoopFilter*               m_pcLoopFilter;
  MbDecoder*                m_pcMbDecoder;
  Transform*                m_pcTransform;
  IntraPrediction*          m_pcIntraPrediction;
  MotionCompensation*       m_pcMotionCompensation;
  YuvBufferCtrl*            m_apcYuvFullPelBufferCtrl [MAX_LAYERS];
  QuarterPelFilter*         m_pcQuarterPelFilter;
  CabacReader*              m_pcCabacReader;
  SampleWeighting*          m_pcSampleWeighting;
  ReconstructionBypass*     m_pcReconstructionBypass;
#ifdef SHARP_AVC_REWRITE_OUTPUT
  RewriteEncoder*           m_pcRewriteEncoder;
#endif
};







struct PacketDescription
{
  Bool  ParameterSet;
  Bool  Scalable;
  UInt  Layer;
  UInt  Level;
  UInt  FGSLayer;
  Bool  ApplyToNext;
  UInt  NalUnitType;
  UInt  SPSid;
  UInt  PPSid;
  UInt  SPSidRefByPPS[256];
	UInt  auiPriorityLevelPR[MAX_NUM_RD_LEVELS];
  UInt  uiNumLevelsQL;
  UInt  uiPId;
  Bool  bDiscardable;
  UInt  uiFirstMb;
  Bool  bDiscardableHRDSEI;
};



class H264AVCDECODERLIB_API H264AVCPacketAnalyzer
{
protected:
  H264AVCPacketAnalyzer();
  virtual ~H264AVCPacketAnalyzer();

public:
  static ErrVal create  ( H264AVCPacketAnalyzer*&  rpcH264AVCPacketAnalyzer );
  ErrVal        destroy ();
  ErrVal        init    ();
  ErrVal        uninit  ();
  ErrVal        process ( BinData*              pcBinData,
                          PacketDescription&    rcPacketDescription,
                          SEI::SEIMessage*&     pcScalableSEIMessage );

  SEI::NonRequiredSei*  getNonRequiredSEI     ()  { return m_pcNonRequiredSEI; }
  UInt                  getNonRequiredSeiFlag ()  { return m_uiNonRequiredSeiFlag; }

  //==== VERY BAD: public members !!!!! =====
  Int     m_layer_id;
  UInt    m_uiNum_layers;
  UInt    m_uiNumSliceGroupsMinus1;
  Int     m_ID_ROI              [MAX_SCALABLE_LAYERS];
  Int     m_ID_Dependency       [MAX_SCALABLE_LAYERS];
  Int     m_silceIDOfSubPicLayer[MAX_SCALABLE_LAYERS];
  UInt    uiaAddrFirstMBofROIs  [256][MAX_ROI_NUM];

protected:
  ErrVal  xCreate ();

protected:
  BitReadBuffer*        m_pcBitReadBuffer;
  UvlcReader*           m_pcUvlcReader;
  NalUnitParser*        m_pcNalUnitParser;
  UInt                  m_uiStdAVCOffset;
  SEI::NonRequiredSei*  m_pcNonRequiredSEI;
  UInt                  m_uiNonRequiredSeiFlag;
  UInt                  m_uiPrevPicLayer;
  UInt                  m_uiCurrPicLayer;
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_CREATERH264AVCDECODER_H__0366BFA9_45D9_4834_B404_8DE3914C1E58__INCLUDED_)
