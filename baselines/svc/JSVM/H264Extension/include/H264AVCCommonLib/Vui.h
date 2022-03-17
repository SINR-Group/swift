
#if !defined(AFX_VUI_H__3B47C8E4_5FE6_487B_81D8_597C908E65AD__INCLUDED_)
#define AFX_VUI_H__3B47C8E4_5FE6_487B_81D8_597C908E65AD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/Hrd.h"

// h264 namespace begin
H264AVC_NAMESPACE_BEGIN

class SequenceParameterSet;


class H264AVCCOMMONLIB_API VUI
{
public:

  class H264AVCCOMMONLIB_API BitstreamRestriction
  {
  public:

    BitstreamRestriction( SequenceParameterSet* pcSPS );
    ErrVal write( HeaderSymbolWriteIf* pcWriteIf ) const;
    ErrVal read( HeaderSymbolReadIf* pcReadIf );

    Bool    isSame( const BitstreamRestriction& rcBitstreamRestriction )  const;
    ErrVal  copy  ( const BitstreamRestriction& rcBitstreamRestriction );

    Bool getMotionVectorsOverPicBoundariesFlag() const { return m_bMotionVectorsOverPicBoundariesFlag; }
    UInt getMaxBytesPerPicDenom()               const { return m_uiMaxBytesPerPicDenom; }
    UInt getMaxBitsPerMbDenom()                 const { return m_uiMaxBitsPerMbDenom; }
    UInt getLog2MaxMvLengthHorizontal()         const { return m_uiLog2MaxMvLengthHorizontal; }
    UInt getLog2MaxMvLengthVertical()           const { return m_uiLog2MaxMvLengthVertical; }
    UInt getMaxDecFrameReordering()             const { return m_uiMaxDecFrameReordering; }
    UInt getMaxDecFrameBuffering()              const { return m_uiMaxDecFrameBuffering; }


    Void setMotionVectorsOverPicBoundariesFlag ( Bool bMotionVectorsOverPicBoundariesFlag ) { m_bMotionVectorsOverPicBoundariesFlag = bMotionVectorsOverPicBoundariesFlag; }
    Void setMaxBytesPerPicDenom ( UInt uiMaxBytesPerPicDenom )                        { m_uiMaxBytesPerPicDenom = uiMaxBytesPerPicDenom; }
    Void setMaxBitsPerMbDenom ( UInt uiMaxBitsPerMbDenom )                            { m_uiMaxBitsPerMbDenom = uiMaxBitsPerMbDenom; }
    Void setLog2MaxMvLengthHorizontal ( UInt uiLog2MaxMvLengthHorizontal )            { m_uiLog2MaxMvLengthHorizontal = uiLog2MaxMvLengthHorizontal; }
    Void setLog2MaxMvLengthVertical ( UInt uiLog2MaxMvLengthVertical )                { m_uiLog2MaxMvLengthVertical = uiLog2MaxMvLengthVertical; }
    Void setMaxDecFrameReordering ( UInt uiMaxDecFrameReordering )                    { m_uiMaxDecFrameReordering = uiMaxDecFrameReordering; }
    Void setMaxDecFrameBuffering ( UInt uiMaxDecFrameBuffering )                      { m_uiMaxDecFrameBuffering = uiMaxDecFrameBuffering; }

  protected:
    Bool m_bBitstreamRestrictionFlag;
    Bool m_bMotionVectorsOverPicBoundariesFlag;
    UInt m_uiMaxBytesPerPicDenom;
    UInt m_uiMaxBitsPerMbDenom;
    UInt m_uiLog2MaxMvLengthHorizontal;
    UInt m_uiLog2MaxMvLengthVertical;
    UInt m_uiMaxDecFrameReordering;
    UInt m_uiMaxDecFrameBuffering;
  };

  class H264AVCCOMMONLIB_API AspectRatioInfo
  {
  public:
    AspectRatioInfo();
    ErrVal write( HeaderSymbolWriteIf* pcWriteIf ) const;
    ErrVal read( HeaderSymbolReadIf* pcReadIf );

    Bool    isSame( const AspectRatioInfo& rcAspectRatioInfo )  const;
    ErrVal  copy  ( const AspectRatioInfo& rcAspectRatioInfo );

    Bool getAspectRatioInfoPresentFlag()        const { return m_bAspectRatioInfoPresentFlag; }
    UInt getAspectRatioIdc()                    const { return m_uiAspectRatioIdc; }
    UInt getSarWith()                           const { return m_uiSarWith; }
    UInt getSarHeight()                         const { return m_uiSarHeight; }
    Void setAspectRatioInfoPresentFlag ( Bool bAspectRatioInfoPresentFlag )           { m_bAspectRatioInfoPresentFlag = bAspectRatioInfoPresentFlag; }
    Void setAspectRatioIdc ( UInt uiAspectRatioIdc )                                  { m_uiAspectRatioIdc = uiAspectRatioIdc; }
    Void setSarWith ( UInt uiSarWith )                                                { m_uiSarWith = uiSarWith; }
    Void setSarHeight ( UInt uiSarHeight )                                            { m_uiSarHeight = uiSarHeight; }

  protected:
    Bool m_bAspectRatioInfoPresentFlag;
    UInt m_uiAspectRatioIdc;
    UInt m_uiSarWith;
    UInt m_uiSarHeight;
  };

  class H264AVCCOMMONLIB_API VideoSignalType
  {
  public:
    VideoSignalType();
    ErrVal write( HeaderSymbolWriteIf* pcWriteIf ) const;
    ErrVal read( HeaderSymbolReadIf* pcReadIf );

    Bool    isSame( const VideoSignalType& rcVideoSignalType )  const;
    ErrVal  copy  ( const VideoSignalType& rcVideoSignalType );

    Bool getVideoSignalTypePresentFlag()        const { return m_bVideoSignalTypePresentFlag; }
    UInt getVideoFormat()                       const { return m_uiVideoFormat; }
    Bool getVideoFullRangeFlag()                const { return m_bVideoFullRangeFlag; }
    Bool getColourDescriptionPresentFlag()      const { return m_bColourDescriptionPresentFlag; }
    UInt getColourPrimaries()                   const { return m_uiColourPrimaries; }
    UInt getTransferCharacteristics()           const { return m_uiTransferCharacteristics; }
    UInt getMatrixCoefficients()                const { return m_uiMatrixCoefficients; }
    Void setVideoSignalTypePresentFlag ( Bool bVideoSignalTypePresentFlag )           { m_bVideoSignalTypePresentFlag = bVideoSignalTypePresentFlag; }
    Void setVideoFormat ( UInt uiVideoFormat )                                        { m_uiVideoFormat = uiVideoFormat; }
    Void setVideoFullRangeFlag ( Bool bVideoFullRangeFlag )                           { m_bVideoFullRangeFlag = bVideoFullRangeFlag; }
    Void setColourDescriptionPresentFlag ( Bool bColourDescriptionPresentFlag )       { m_bColourDescriptionPresentFlag = bColourDescriptionPresentFlag; }
    Void setColourPrimaries ( UInt uiColourPrimaries )                                { m_uiColourPrimaries = uiColourPrimaries; }
    Void setTransferCharacteristics ( UInt uiTransferCharacteristics )                { m_uiTransferCharacteristics = uiTransferCharacteristics; }
    Void setMatrixCoefficients ( UInt uiMatrixCoefficients )                          { m_uiMatrixCoefficients = uiMatrixCoefficients; }

  protected:
    Bool m_bVideoSignalTypePresentFlag;
    UInt m_uiVideoFormat;
    Bool m_bVideoFullRangeFlag;
    Bool m_bColourDescriptionPresentFlag;
    UInt m_uiColourPrimaries;
    UInt m_uiTransferCharacteristics;
    UInt m_uiMatrixCoefficients;
  };

  class H264AVCCOMMONLIB_API ChromaLocationInfo
  {
  public:
    ChromaLocationInfo();
    ErrVal write( HeaderSymbolWriteIf* pcWriteIf ) const;
    ErrVal read( HeaderSymbolReadIf* pcReadIf );

    Bool    isSame( const ChromaLocationInfo& rcChromaLocationInfo )  const;
    ErrVal  copy  ( const ChromaLocationInfo& rcChromaLocationInfo );

    Bool getChromaLocationInfoPresentFlag()     const { return m_bChromaLocationInfoPresentFlag; }
    UInt getChromaLocationFrame()               const { return m_uiChromaLocationFrame; }
    UInt getChromaLocationField()               const { return m_uiChromaLocationField; }
    Void setChromaLocationInfoPresentFlag ( Bool bChromaLocationInfoPresentFlag )     { m_bChromaLocationInfoPresentFlag = bChromaLocationInfoPresentFlag; }
    Void setChromaLocationFrame ( UInt uiChromaLocationFrame )                        { m_uiChromaLocationFrame = uiChromaLocationFrame; }
    Void setChromaLocationField ( UInt uiChromaLocationField )                        { m_uiChromaLocationField = uiChromaLocationField; }
  protected:
    Bool m_bChromaLocationInfoPresentFlag;
    UInt m_uiChromaLocationFrame;
    UInt m_uiChromaLocationField;
  };

  class H264AVCCOMMONLIB_API TimingInfo
  {
  public:
    TimingInfo();
    ErrVal write( HeaderSymbolWriteIf* pcWriteIf ) const;
    ErrVal read( HeaderSymbolReadIf* pcReadIf );

    Bool    isSame( const TimingInfo& rcTimingInfo )  const;
    ErrVal  copy  ( const TimingInfo& rcTimingInfo );

    UInt getNumUnitsInTick()                    const { return m_uiNumUnitsInTick; }
    UInt getTimeScale()                         const { return m_uiTimeScale; }
    Bool getFixedFrameRateFlag()                const { return m_bFixedFrameRateFlag; }
    Void setNumUnitsInTick ( UInt uiNumUnitsInTick )                                  { m_uiNumUnitsInTick = uiNumUnitsInTick; }
    Void setTimeScale ( UInt uiTimeScale )                                            { m_uiTimeScale = uiTimeScale; }
    Void setFixedFrameRateFlag ( Bool bFixedFrameRateFlag )                           { m_bFixedFrameRateFlag = bFixedFrameRateFlag; }

	  Void setTimingInfoPresentFlag(Bool bTimingInfoPresentFlag)                        { m_bTimingInfoPresentFlag = bTimingInfoPresentFlag; }
	  Bool getTimingInfoPresentFlag()                                             const { return m_bTimingInfoPresentFlag; }
 protected:
    UInt m_uiNumUnitsInTick;
    UInt m_uiTimeScale;
    Bool m_bFixedFrameRateFlag;
	  Bool m_bTimingInfoPresentFlag;
  };


  class H264AVCCOMMONLIB_API LayerInfo
  {
  public:

    LayerInfo() {}
    ErrVal write( HeaderSymbolWriteIf* pcWriteIf ) const;
    ErrVal read( HeaderSymbolReadIf* pcReadIf );

    Void setDependencyID(UInt uiDependencyID)             { m_uiDependencyID = uiDependencyID; }
    Void setTemporalId(UInt uiTemporalLevel)           { m_uiTemporalId = uiTemporalLevel; }
    Void setQualityLevel(UInt uiQualityLevel)             { m_uiQualityId = uiQualityLevel; }

    UInt getDependencyID()                        const { return m_uiDependencyID; }
    UInt getTemporalId()                       const { return m_uiTemporalId; }
    UInt getQualityId()                        const { return m_uiQualityId; }

  protected:

    UInt m_uiDependencyID;
    UInt m_uiTemporalId;
    UInt m_uiQualityId;
  };


public:
  VUI( SequenceParameterSet* pcSPS );
  virtual ~VUI();

  ErrVal init( UInt uiNumTemporalLevels, UInt uiNumFGSLevels );

  ErrVal write( HeaderSymbolWriteIf* pcWriteIf )          const;
  ErrVal read( HeaderSymbolReadIf* pcReadIf );

  Bool    isSameExceptHRDParametersAndSVCExt( const VUI& rcVUI )  const;
  ErrVal  copyExceptHRDParametersAndSVCExt  ( const VUI& rcVUI );

  ErrVal writeSVCExtension( HeaderSymbolWriteIf* pcWriteIf )          const;
  ErrVal readSVCExtension( HeaderSymbolReadIf* pcReadIf );

  Bool getOverscanInfoPresentFlag()                 const { return m_bOverscanInfoPresentFlag; }
  Bool getOverscanAppropriateFlag()                 const { return m_bOverscanAppropriateFlag; }
  Bool getLowDelayHrdFlag(UInt uiIndex)             const { return m_abLowDelayHrdFlag.get(uiIndex); }
  Bool getPicStructPresentFlag(UInt uiIndex)        const { return m_abPicStructPresentFlag.get(uiIndex); }

  const LayerInfo&              getLayerInfo(UInt uiIndex)            const { return m_acLayerInfo.get(uiIndex); }
  const TimingInfo&             getTimingInfo(UInt uiIndex)           const { return m_acTimingInfo.get(uiIndex); }
  const HRD&                    getNalHrd(UInt uiIndex)               const { return m_acNalHrd.get(uiIndex); }
  const HRD&                    getVclHrd(UInt uiIndex)               const { return m_acVclHrd.get(uiIndex); }

  const AspectRatioInfo&        getAspectRatioInfo()      const { return m_cAspectRatioInfo; }
  const VideoSignalType&        getVideoSignalType()      const { return m_cVideoSignalType; }
  const ChromaLocationInfo&     getChromaLocationInfo()   const { return m_cChromaLocationInfo; }
  const BitstreamRestriction&   getBitstreamRestriction() const { return m_cBitstreamRestriction; }

  LayerInfo&                    getLayerInfo(UInt uiSchedulerID)                  { return m_acLayerInfo[uiSchedulerID]; }
  TimingInfo&                   getTimingInfo(UInt uiSchedulerID)                 { return m_acTimingInfo[uiSchedulerID]; }
  HRD&                          getNalHrd(UInt uiSchedulerID)                     { return m_acNalHrd[uiSchedulerID]; }
  HRD&                          getVclHrd(UInt uiSchedulerID)                     { return m_acVclHrd[uiSchedulerID]; }
  Bool&                         getLowDelayHrdFlag ( UInt uiSchedulerID )         { return m_abLowDelayHrdFlag[uiSchedulerID]; }
  Bool&                         getPicStructPresentFlag( UInt uiSchedulerID )     { return m_abPicStructPresentFlag[uiSchedulerID]; }
  UInt                          getNumLayers()          const                     { return m_uiNumTemporalLevels*m_uiNumFGSLevels; }
  UInt                          getNumTemporalLevels()  const                     { return m_uiNumTemporalLevels; }
  UInt                          getNumFGSLevels()       const                     { return m_uiNumFGSLevels; }

  AspectRatioInfo&              getAspectRatioInfo()            { return m_cAspectRatioInfo; }
  VideoSignalType&              getVideoSignalType()            { return m_cVideoSignalType; }

	Void                          setProfileIdc ( Profile eProfileIdc) { m_eProfileIdc = eProfileIdc; }
	UInt                          getProfileIdc () const { return m_eProfileIdc; }

  Void                          setLevelIdc ( UInt eLevelIdc) { m_uiLevelIdc = eLevelIdc; }
  UInt                          getLevelIdc () const { return m_uiLevelIdc; }

  Void setOverscanInfoPresentFlag ( Bool bOverscanInfoPresentFlag )                 { m_bOverscanInfoPresentFlag = bOverscanInfoPresentFlag; }
  Void setOverscanAppropriateFlag ( Bool bOverscanAppropriateFlag )                 { m_bOverscanAppropriateFlag = bOverscanAppropriateFlag; }

  Bool  getVuiParametersPresentFlag() const { return m_bVuiParametersPresentFlag; }
  Void  setVuiParametersPresentFlag( Bool bVuiParametersPresentFlag ) { m_bVuiParametersPresentFlag = bVuiParametersPresentFlag; }
  Void  setHrdParameterPresentFlag ( Bool nalHrdParametersPresentFlag, Bool vclHrdParametersPresentFlag);
  Void  setHrdCpbCntSize           ( UInt nalHrdCpbCntSize, UInt vclHrdCpbCntSize);
  Void  setHRDSimulationOn            ();
  Void  setVUITimingInfoSimulationOn  ();
  Void  setVUITimingInfoPresentFlag( Bool bVUITimingInfoPresentFlag);
  ErrVal InitHrd( UInt uiIndex, HRD::HrdParamType eHrdType, UInt uiBitRate, UInt uiCpbSize);

  Void setDefaultIdx( UInt ui ) { m_uiDefaultIdx = ui; }
  UInt getDefaultIdx() const { return m_uiDefaultIdx; }

  UInt  getNumEntries() const { return m_acLayerInfo.size(); }

protected:
  Bool m_bVuiParametersPresentFlag;
  AspectRatioInfo m_cAspectRatioInfo;
  Bool m_bOverscanInfoPresentFlag;
  Bool m_bOverscanAppropriateFlag;
  VideoSignalType m_cVideoSignalType;

  ChromaLocationInfo m_cChromaLocationInfo;
  DynBuf<LayerInfo> m_acLayerInfo;
	DynBuf<TimingInfo> m_acTimingInfo;
  DynBuf<HRD> m_acNalHrd;
  DynBuf<HRD> m_acVclHrd;
  DynBuf<Bool> m_abLowDelayHrdFlag;
  DynBuf<Bool> m_abPicStructPresentFlag;

  BitstreamRestriction m_cBitstreamRestriction;

  UInt m_uiNumTemporalLevels;
  UInt m_uiNumFGSLevels;
  UInt m_uiDefaultIdx;

	Profile m_eProfileIdc;
  UInt  m_uiLevelIdc;
};

// h264 namespace end
H264AVC_NAMESPACE_END

#endif // !defined(AFX_VUI_H__3B47C8E4_5FE6_487B_81D8_597C908E65AD__INCLUDED_)
